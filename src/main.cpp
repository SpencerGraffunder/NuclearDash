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


const char* ht_names[] = {
    "RPM",
    "Manifold Pressure",
    "Throttle Position",
    "Coolant Pressure",
    "Fuel Pressure",
    "Oil Pressure",
    "Engine Demand",
    "Wastegate Pressure",
    "Injection State 1 Duty Cycle",
    "Injection State 2 Duty Cycle",
    "Ignition Angle",
    "Wheel Slip",
    "Wheel Diff",
    "Launch Control End RPM",
    "Wideband Sensor 1",
    "Knock Level 1",
    "Wheel Speed Rear Right",
    "Boost Control Output",
    "Vehicle Speed",
    "Intake Cam Angle 1",
    "Battery Voltage",
    "Target Boost Level",
    "Coolant Temperature",
    "Air Temperature",
    "Oil Temperature",
    "Fuel Trim Short Term Bank 1",
    "Fuel Trim Long Term Bank 1",
    "Ignition Angle Bank 1",
    "Wideband Overall",
    "Gear",
    "Water Injection Advanced Solenoid Duty Cycle"
};

const char* ht_names_short[] = {
    "RPM",
    "ManifPres",
    "ThrotPos",
    "CoolPres",
    "FuelPres",
    "OilPres",
    "EngDemand",
    "WastePres",
    "InjState1",
    "InjState2",
    "IgnAngle",
    "WheelSlip",
    "WheelDiff",
    "LaunchRPM",
    "WidebandS1",
    "KnockLvl1",
    "WheelSpRR",
    "BoostCtrl",
    "VehSpeed",
    "IntCamAng1",
    "BattVolt",
    "TargBoost",
    "CoolTemp",
    "AirTemp",
    "OilTemp",
    "FuelTrimST",
    "FuelTrimLT",
    "IgnAngleB1",
    "WBOverall",
    "Gear",
    "WaterInjDC"
};