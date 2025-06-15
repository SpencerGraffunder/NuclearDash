#include "webpage.h"
#include <SPIFFS.h>
#include <vector>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "screen.h"

std::vector<WiFiClient> sseClients;

// WiFi Configuration
const char* ssid = "Little House On The Quarry";
const char* password = "heckifiknow";
const char* hostname = "NuclearDash";
const char* selfssid = "NuclearDash";
const char* selfpassword = "nucleard";
const char* versionUrl = "https://raw.githubusercontent.com/SpencerGraffunder/NuclearDash/refs/heads/master/version.json";

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
  Serial.println("handling root");
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

// Add this new endpoint
void handleUploadStatus() {
  String status = "";
  if (updateInProgress) {
    float progress = (updateWritten * 100.0) / updateSize;
    status = "Upload in progress: " + String(progress, 1) + "%";
  } else {
    status = "No upload in progress";
  }
  server.send(200, "text/plain", status);
}

void handleUpdateUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {      
      Serial.printf("Starting OTA update. File size: %u bytes\n", upload.totalSize);
      updateInProgress = true;
      updateSize = upload.totalSize;
      updateWritten = 0;

      if (!Update.begin()) {
          Serial.println("Update.begin() failed!");
          Update.printError(Serial);
          updateInProgress = false;
      } else {
          Serial.println("Update.begin() succeeded.");
      }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
      Serial.printf("Writing %u bytes...\n", upload.currentSize);
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
          updateInProgress = false;
      }
      updateWritten += upload.currentSize;
  } else if (upload.status == UPLOAD_FILE_END) {
      Serial.printf("Upload complete. Total written: %u bytes\n", updateWritten);
      if (Update.end(true)) {
          Serial.println("Update successful! Rebooting...");
          server.send(200, "text/plain", "Update Successful! Rebooting...");
          delay(1000);
          ESP.restart();
      } else {
          Update.printError(Serial);
          server.send(500, "text/plain", "Update Failed");
          updateInProgress = false;
      }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
      Serial.println("Upload aborted.");
      Update.end();
      updateInProgress = false;
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

void createAccessPoint() {
  Serial.println("Creating Access Point.");
  
  // Stop any previous WiFi connections
  WiFi.disconnect(true);
  
  // Configure soft access point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(selfssid, selfpassword);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  // Set up mDNS for access point
  if (!MDNS.begin(hostname)) {
    Serial.println("Error setting up MDNS responder for AP!");
  }

  // Server routes for AP mode (these would be the same as in normal mode)
  server.on("/", handleRoot);
  server.on("/ota", handleOTAPage);
  server.on("/events", HTTP_GET, handleSSE);
  server.on("/uploadStatus", HTTP_GET, handleUploadStatus);
  server.on("/update", HTTP_POST, [](){
    // Dummy handler for POST request
  }, handleUpdateUpload);
  server.onNotFound(handleNotFound);
  
  server.begin();

  setupOTA();
}

void webpageSetup() {
  // to load the html
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return;
  }

  // WiFi connection attempt
  WiFi.setHostname(hostname);
  WiFi.setSleep(false);
  WiFi.begin(ssid, password);

  // Wait for WiFi connection with timeout
  unsigned long startAttemptTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.print(".");
    
    // If connection fails after 1 seconds, create access point
    if (millis() - startAttemptTime > 1000) {
      Serial.printf("\nFailed to connect to WiFi. ");
      createAccessPoint();
      return;
    }
  }

  // WiFi Connected Successfully
  Serial.println("\nWiFi Connected");
  Serial.println("IP Address: " + WiFi.localIP().toString());

  // Set up mDNS
  if (!MDNS.begin(hostname)) {
    Serial.println("Error setting up MDNS responder!");
  }

  // Server routes
  server.on("/", handleRoot);
  server.on("/ota", handleOTAPage);
  server.on("/events", HTTP_GET, handleSSE);
  server.on("/uploadStatus", HTTP_GET, handleUploadStatus);
  server.on("/update", HTTP_POST, [](){
    // Dummy handler for POST request
  }, handleUpdateUpload);
  server.onNotFound(handleNotFound);
  
  server.begin();

  setupOTA();

  if (checkForUpdate()) {
    Serial.println("Update available, starting OTA...");
    delay(1000);
    performOTAUpdate();
  } else {
    Serial.println("No update available or check failed.");
  }
}

String downloadURL = "https://nuclearquads.github.io/dash/firmware.bin";

uint32_t getRemoteVersion() {
  HTTPClient http;
  http.begin(versionUrl);
  http.addHeader("User-Agent", "ESP32-OTA-Client");
  
  int httpCode = http.GET();
  uint32_t version = -1;
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      version = doc["latest_version"].as<uint32_t>();
      downloadURL = doc["download_url"].as<String>();
    } else {
      Serial.printf("JSON parsing failed: %s\n", error.c_str());
    }
  } else {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
  }
  
  http.end();
  return version;
}

bool checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    return false;
  }
  
  uint32_t remoteVersion = getRemoteVersion();
  if (remoteVersion == 0) {
    Serial.println("Failed to get remote version or version is 0");
    return false;
  }
  
  Serial.printf("Remote version: %u\n", remoteVersion);
  Serial.printf("Current version: %u\n", CURRENT_VERSION);
  
  // Only update if remote version is GREATER than current version
  bool updateNeeded = remoteVersion > CURRENT_VERSION;
  Serial.printf("Update needed: %s\n", updateNeeded ? "YES" : "NO");

  bool doUpdate = false;
  if (updateNeeded) {
    if (showUpdateScreen(remoteVersion, CURRENT_VERSION)) {
      doUpdate = true;
    } else {
      Serial.println("User chose not to update.");
    }
  }
  
  return doUpdate;
}

void performOTAUpdate() {
  HTTPClient http;
  http.begin(downloadURL);
  http.addHeader("User-Agent", "ESP32-OTA-Client");
  
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET failed: %d\n", httpCode);
    http.end();
    return;
  }
  
  int contentLength = http.getSize();
  if (contentLength <= 0) {
    Serial.println("Content length is 0 or unknown");
    http.end();
    return;
  }
  
  Serial.printf("Starting OTA update. Firmware size: %d bytes\n", contentLength);
  
  if (!Update.begin(contentLength)) {
    Serial.printf("Not enough space for OTA update. Required: %d\n", contentLength);
    http.end();
    return;
  }
  
  WiFiClient* client = http.getStreamPtr();
  size_t written = 0;
  uint8_t buffer[128];
  
  while (http.connected() && written < contentLength) {
    size_t available = client->available();
    if (available) {
      size_t bytesToRead = min(available, sizeof(buffer));
      size_t bytesRead = client->readBytes(buffer, bytesToRead);
      
      size_t bytesWritten = Update.write(buffer, bytesRead);
      written += bytesWritten;
      
      // Show progress
      int progress = (written * 100) / contentLength;
      char progressString[6];
      sprintf(progressString, "%d%%", progress);
      tft.drawString("Update Progress", TFT_HEIGHT / 2, TFT_WIDTH * 6 / 8);
      tft.drawString(progressString, TFT_HEIGHT / 2, TFT_WIDTH * 7 / 8);
      Serial.printf("Progress: %d%% (%d/%d bytes)\n", progress, written, contentLength);
      
      if (bytesWritten != bytesRead) {
        Serial.println("\nWrite error during OTA update");
        break;
      }
    }
    yield(); // Allow other tasks to run
  }
  
  Serial.println(); // New line after progress
  
  if (written == contentLength) {
    if (Update.end()) {
      if (Update.isFinished()) {
        Serial.println("OTA update completed successfully!");
        Serial.println("Rebooting in 3 seconds...");
        delay(3000);
        ESP.restart();
      } else {
        Serial.println("OTA update finished but not applied");
      }
    } else {
      Serial.printf("OTA update failed. Error: %s\n", Update.errorString());
    }
  } else {
    Serial.printf("OTA update incomplete. Written: %d, Expected: %d\n", written, contentLength);
  }
  
  http.end();
}

void webpageLoop() {
  server.handleClient();
  ArduinoOTA.handle();
}
