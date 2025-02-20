#include <Arduino.h>
#include "haltech_can.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "screen.h"
#include "webpage.h"

HaltechCan htc;

void setup() {
  // Use serial port
  Serial.begin(115200);

  screenSetup();

  while (!htc.begin(1000E3)) {
    Serial.println("Haltech CAN init failed");
    delay(500);
  }

  webpageSetup();

  Serial.println("setup");
}

void loop(void) {
  Serial.println("htc");
  htc.process();
  Serial.println("webpage");
  webpageLoop();
  Serial.println("screen");
  screenLoop();
}
