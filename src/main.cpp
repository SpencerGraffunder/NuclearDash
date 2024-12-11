#include <Arduino.h>
#include "haltech_can.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include "screen.h"
#include "webpage.h"

HaltechCan htc;

void setup() {
  // Use serial port
  Serial.begin(115200);

  screenSetup();

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
  screenLoop();
  htc.process();
  webpageLoop();
}
