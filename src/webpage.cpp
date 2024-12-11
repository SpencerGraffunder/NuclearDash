#include "webpage.h"
#include <SPIFFS.h>
#include <vector>

std::vector<WiFiClient> sseClients;

// WiFi Configuration
const char* ssid = "Little House On The Quarry";
const char* password = "heckifiknow";

// Webserver setup
WebServer server(80);

// Sensor/Stream Data Variables
float temperature = 0.0;
float humidity = 0.0;
unsigned long lastUpdateTime = 0;

// OTA Update Variables
bool updateInProgress = false;
size_t updateSize = 0;
size_t updateWritten = 0;

void handleRoot() {
 File indexHtmlFile = SPIFFS.open("/index.html", "r");
 if (!indexHtmlFile) {
   server.send(500, "text/plain", "Failed to open index HTML file");
   return;
 }
 
 String html = indexHtmlFile.readString();
 indexHtmlFile.close();
 
 server.send(200, "text/html", html);
}

void handleOTAPage() {
  File otaHtmlFile = SPIFFS.open("/ota.html", "r");
  if (!otaHtmlFile) {
    server.send(500, "text/plain", "Failed to open OTA HTML file");
    return;
  }
  
  String otaHtml = otaHtmlFile.readString();
  otaHtmlFile.close();
  
  server.send(200, "text/html", otaHtml);
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    updateInProgress = true;
    updateSize = upload.totalSize;
    updateWritten = 0;
    
    // Note: This uses the Update library to handle OTA updates
    if (!Update.begin(updateSize)) {
      Update.printError(Serial);
      updateInProgress = false;
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
      updateInProgress = false;
    }
    updateWritten += upload.currentSize;
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      // Successful update
      server.send(200, "text/plain", "Update Successful! Rebooting...");
      delay(1000);
      ESP.restart();
    } else {
      Update.printError(Serial);
      server.send(500, "text/plain", "Update Failed");
      updateInProgress = false;
    }
  }
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void setupOTA() {
  ArduinoOTA.setPort(3232);  // Explicitly set OTA port
  ArduinoOTA.setHostname("ESP32-OTA");  // Optional: give a unique name
  
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Starting");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("OTA Update Completed");
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    switch(error) {
      case OTA_AUTH_ERROR: Serial.println("Auth Failed"); break;
      case OTA_BEGIN_ERROR: Serial.println("Begin Failed"); break;
      case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
      case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
      case OTA_END_ERROR: Serial.println("End Failed"); break;
    }
  });

  ArduinoOTA.begin();
}

void handleSSE() {
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/event-stream");
  client.println("Cache-Control: no-cache");
  client.println("Connection: keep-alive");
  client.println();
  client.flush();

  sseClients.push_back(client);
}

void updateWebpageValue(int index, float value, int precision) {
  String event = "event:update\n";
  event += "data:{\"index\":" + String(index) + 
           ",\"value\":" + String(value, precision) + 
           ",\"precision\":" + String(precision) + "}\n\n";

  for (size_t i = 0; i < sseClients.size(); ++i) {
    if (sseClients[i].connected()) {
      sseClients[i].print(event);
    } else {
      // Remove disconnected clients
      sseClients.erase(sseClients.begin() + i);
      --i;
    }
  }
}

void webpageSetup() {
  // to load the html
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  // wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  // server
  server.on("/", handleRoot);
  server.on("/ota", handleOTAPage);
  server.on("/events", HTTP_GET, handleSSE);
  server.on("/update", HTTP_POST, [](){
    // Dummy handler for POST request
  }, handleUpdateUpload);
  server.onNotFound(handleNotFound);
  
  server.begin();

  setupOTA();
}

void webpageLoop() {
  server.handleClient();
  ArduinoOTA.handle();
}