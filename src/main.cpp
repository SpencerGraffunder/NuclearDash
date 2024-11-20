#include <Arduino.h>
#include "haltech_can.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include "screen.h"
#include "main.h"

HaltechCan htc;

void setup() {
  // Use serial port
  Serial.begin(115200);
  delay(2000);

  Serial.println("setup");

  screenSetup();

  if (!htc.begin(1000E3)) {
    while (1) {
      Serial.println("Haltech CAN init failed");
      delay(1000);
    }
  }
}

void loop(void) {
  screenLoop();
  htc.process();
}
