#include "config.h"
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
  delay(2000);
  Serial.printf("starting setup\n");

  screenSetup();
  Serial.printf("screen setup done\n");

  while (!htc.begin(1000E3)) {
    Serial.println("Haltech CAN init failed");
    delay(500);
  }

  Serial.printf("webpage setup\n");
  webpageSetup();

  Serial.println("setup done");
}

void loop(void) {
  //Serial.printf("htc %lu\n", millis());
  htc.process();
  //Serial.printf("webpage %lu\n", millis());
  webpageLoop();
  //Serial.printf("screen %lu\n", millis());
  screenLoop();
}
