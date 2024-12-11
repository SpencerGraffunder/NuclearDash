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

  static bool buzz = true;
  digitalWrite(17, buzz);
  digitalWrite(4, buzz);
  buzz = !buzz;
  delay(1000);
}
