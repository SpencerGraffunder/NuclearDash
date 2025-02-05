#include <Arduino.h>
#include "haltech_can.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "screen.h"
#include "webpage.h"

HaltechCan htc;

void screenTask(void *pvParameters) {
  screenSetup();
  while (true) {
    screenLoop();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  // Use serial port
  Serial.begin(115200);

  // Create a task for the screen on core 1
  xTaskCreatePinnedToCore(
    screenTask,   // Task function
    "ScreenTask", // Name of the task
    10000,        // Stack size
    NULL,         // Task input parameter
    1,            // Priority of the task
    NULL,         // Task handle
    1             // Core to run the task on (1 for the second core)
  );

  if (!htc.begin(1000E3)) {
    while (1) {
      Serial.println("Haltech CAN init failed");
      delay(500);
    }
  }

  webpageSetup();

  Serial.println("setup");
}

void loop(void) {
  htc.process();
  webpageLoop();
}
