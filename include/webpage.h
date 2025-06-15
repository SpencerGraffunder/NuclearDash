#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <ArduinoOTA.h>

// External variables for sensor data
extern float temperature;
extern float humidity;
extern unsigned long lastUpdateTime;

// WiFi Configuration (consider moving these to a config file)
extern const char* ssid;
extern const char* password;

// Webpage setup and loop functions
void webpageSetup();
void webpageLoop();
bool checkForUpdate();
void performOTAUpdate();

// Handler functions
void handleRoot();
void handleOTAPage();
void handleUpdateUpload();
void updateWebpageValue(int index, float value, int precision);

#endif // WEBPAGE_H