#include "webpage.h"

// WiFi Configuration
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

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
  String html = F(R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 Dashboard</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; padding: 20px; }
        .data-display { background-color: #f4f4f4; padding: 10px; margin: 10px 0; }
    </style>
</head>
<body>
    <h1>ESP32 Sensor Dashboard</h1>
    <div class="data-display">
        <h2>Sensor Readings</h2>
        <p>Temperature: <span id="temp">--</span>°C</p>
        <p>Humidity: <span id="humid">--</span>%</p>
    </div>
    <script>
        function updateData() {
            fetch('/data')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('temp').textContent = data.temperature.toFixed(1);
                    document.getElementById('humid').textContent = data.humidity.toFixed(1);
                });
        }
        
        // Update every 2 seconds
        setInterval(updateData, 2000);
        updateData(); // Initial fetch
    </script>
</body>
</html>
)");
  server.send(200, "text/html", html);
}

void handleOTAPage() {
  String otaHtml = F(R"(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Update</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 600px; margin: 0 auto; padding: 20px; }
        #status { margin: 10px 0; padding: 10px; background-color: #f4f4f4; }
    </style>
</head>
<body>
    <h1>OTA Firmware Update</h1>
    <form method='POST' action='/update' enctype='multipart/form-data'>
        <input type='file' name='update' accept='.bin'>
        <input type='submit' value='Update Firmware'>
    </form>
    <div id='status'></div>
    <script>
        document.querySelector('form').addEventListener('submit', function(e) {
            e.preventDefault();
            var formData = new FormData(this);
            
            fetch('/update', {
                method: 'POST',
                body: formData
            })
            .then(response => response.text())
            .then(result => {
                document.getElementById('status').textContent = result;
            })
            .catch(error => {
                document.getElementById('status').textContent = 'Update failed: ' + error;
            });
        });
    </script>
</body>
</html>
)");
  server.send(200, "text/html", otaHtml);
}

void handleSensorData() {
  String jsonResponse = "{\"temperature\":" + String(temperature) 
                      + ",\"humidity\":" + String(humidity) + "}";
  server.send(200, "application/json", jsonResponse);
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

void webpageSetup() {
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
  server.on("/data", handleSensorData);
  server.on("/update", HTTP_POST, [](){
    // Dummy handler for POST request
  }, handleUpdateUpload);
  
  server.begin();
  
}

void webpageLoop() {
  server.handleClient();
}
