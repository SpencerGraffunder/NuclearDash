#include "screen.h"
#include "main.h"
#include "haltech_can.h"


TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

const uint8_t nButtons = 16;

HaltechDisplayType_e buttonValues[nButtons] = {
  HT_RPM, HT_MANIFOLD_PRESSURE, HT_THROTTLE_POSITION, HT_OIL_PRESSURE, HT_IGNITION_ANGLE, HT_WIDEBAND_OVERALL, HT_VEHICLE_SPEED, HT_INTAKE_CAM_ANGLE_1, HT_BATTERY_VOLTAGE, HT_COOLANT_TEMPERATURE, HT_AIR_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE};

// Invoke the TFT_eSPI button class and create all the button objects
HaltechButton key[16];

void touch_calibrate()
{
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  // check if calibration file exists and size is correct
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL)
    {
      // Delete if we want to re-calibrate
      SPIFFS.remove(CALIBRATION_FILE);
    }
    else
    {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    // calibration data valid
    tft.setTouch(calData);
  } else {
    // data not valid so recalibrate
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    tft.println("Touch corners as indicated");

    tft.setTextFont(1);
    tft.println();

    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }

    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}

// Print something in the mini status bar
void status(const char *msg) {
  tft.setTextPadding(240);
  //tft.setCursor(STATUS_X, STATUS_Y);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextFont(0);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(1);
  tft.drawString(msg, STATUS_X, STATUS_Y);
}

void screenSetup() {
  // Initialise the TFT screen
  tft.init();

  // Set the rotation before we calibrate
  tft.setRotation(3);

  // Calibrate the touch screen and retrieve the scaling factors
  touch_calibrate();

  // Clear the screen
  tft.fillScreen(TFT_BLACK);

  uint8_t nCols = 4;
  uint8_t nRows = 4;
  uint16_t buttonWidth = TFT_HEIGHT / nCols;
  uint16_t buttonHeight = TFT_WIDTH / nRows;
  for (uint8_t row = 0; row < nRows; row++) {
    for (uint8_t col = 0; col < nCols; col++) {
      uint8_t index = col + row * nCols;
      tft.setFreeFont(LABEL2_FONT);    
      key[index].initButtonUL(&tft, col * buttonWidth, row * buttonHeight, buttonWidth, buttonHeight, TFT_GREEN, TFT_BLACK, TFT_WHITE, 1, HaltechScreenEntity((HaltechDisplayType_e)(row*nRows+col)));
      key[index].drawButton();
    }
  }
}

void screenLoop() {
  uint16_t t_x = 0, t_y = 0; // To store the touch coordinates

  // will be set true if there is a valid touch on the screen
  bool isValidTouch = tft.getTouch(&t_x, &t_y);

  // Check if any key coordinate boxes contain the touch coordinates
  for (uint8_t buttonIndex = 0; buttonIndex < nButtons; buttonIndex++) {
    if (isValidTouch && key[buttonIndex].contains(t_x, t_y)) {
      key[buttonIndex].press(true);  // tell the button it is pressed
    } else {
      key[buttonIndex].press(false);  // tell the button it is NOT pressed
    }
  }

  for (uint8_t buttonIndex = 0; buttonIndex < nButtons; buttonIndex++) {
    if (key[buttonIndex].justReleased()) {
      key[buttonIndex].drawButton();     // draw normal
    }
    if (key[buttonIndex].justPressed()) {
      key[buttonIndex].pressedTime = millis();
      key[buttonIndex].drawButton(true);  // draw invert
      htc.dashValues[buttonValues[buttonIndex]].scaled_value = !htc.dashValues[buttonValues[buttonIndex]].scaled_value;
      htc.dashValues[buttonValues[buttonIndex]].justUpdated = true;
    }
    if (htc.dashValues[buttonValues[buttonIndex]].justUpdated) {
      // if (key[buttonIndex].htEntity.isValueNew()) {
        key[buttonIndex].drawValue(key[buttonIndex].htEntity.getValue());
        Serial.printf("Drawing %f\n", key[buttonIndex].htEntity.getValue());
      // }
      //key[buttonIndex].drawValue(htc.dashValues[buttonValues[buttonIndex]].scaled_value);
    }

    if (millis() > key[buttonIndex].pressedTime + HaltechButton::longPressTime) {

    }

    delay(3); // UI debouncing
  }
}