#include <Arduino.h>

/*
  The TFT_eSPI library incorporates an Adafruit_GFX compatible
  button handling class, this sketch is based on the Arduin-o-phone
  example.

  This example displays a keypad where numbers can be entered and
  sent to the Serial Monitor window.

  The sketch has been tested on the ESP8266 (which supports SPIFFS)

  The minimum screen size is 320 x 240 as that is the keypad size.

  TOUCH_CS and SPI_TOUCH_FREQUENCY must be defined in the User_Setup.h file
  for the touch functions to do anything.
*/

// The SPIFFS (FLASH filing system) is used to hold touch screen
// calibration data

#include "haltech_can.h"
#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include "screen.h"
#include "main.h"


HaltechCan htc;

void setup() {
  // Use serial port
  Serial.begin(115200);
  delay(500);

  Serial.println("setup");

  screenSetup();

  if (!htc.begin(1000E3)) {
    while (1) {
      Serial.println("Haltech CAN init failed");
      delay(500);
    }
  }
}

void loop(void) {
  screenLoop();
  htc.process();
}
