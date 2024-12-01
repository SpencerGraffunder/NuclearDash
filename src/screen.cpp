#include "screen.h"
#include "main.h"
#include "haltech_can.h"
#include "haltech_button.h"

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

const uint8_t nButtons = 16;
HaltechDisplayType_e defaultButtonLayout[nButtons] = {
  HT_RPM, HT_MANIFOLD_PRESSURE, HT_THROTTLE_POSITION, HT_OIL_PRESSURE, HT_IGNITION_ANGLE, HT_WIDEBAND_OVERALL, HT_VEHICLE_SPEED, HT_INTAKE_CAM_ANGLE_1, HT_BATTERY_VOLTAGE, HT_COOLANT_TEMPERATURE, HT_AIR_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE, HT_OIL_TEMPERATURE};

// Invoke the TFT_eSPI button class and create all the button objects
HaltechButton htButtons[16];

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
      htButtons[index].initButton(&tft, col * buttonWidth, row * buttonHeight, buttonWidth, buttonHeight, TFT_GREEN, TFT_BLACK, TFT_WHITE, 1, &dashValues[row*nRows+col]);
      htButtons[index].drawButton();
    }
  }
}

void screenLoop() {
  unsigned long currentMillis = millis();
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 50; // Adjust as needed
  
  // Non-blocking debounce
  if (currentMillis - lastDebounceTime < debounceDelay) {
    return; // Skip processing if not enough time has passed
  }

  uint16_t t_x = 0, t_y = 0;
  bool isValidTouch = tft.getTouch(&t_x, &t_y);

  for (uint8_t buttonIndex = 0; buttonIndex < nButtons; buttonIndex++) {
    bool wasPressed = htButtons[buttonIndex].isPressed();
    bool buttonContainsTouch = isValidTouch && htButtons[buttonIndex].contains(t_x, t_y);
    
    // Combine touch detection and button state update
    htButtons[buttonIndex].press(buttonContainsTouch);

    // Only redraw if button state has changed
    if (htButtons[buttonIndex].isPressed() != wasPressed) {
      // Track pressed time for potential long press functionality
      if (htButtons[buttonIndex].isPressed()) {
        htButtons[buttonIndex].pressedTime = currentMillis;
      }
      
      // Redraw button with appropriate state
      htButtons[buttonIndex].drawButton(htButtons[buttonIndex].isPressed());
    }

    // Optional: Long press handling (you can expand this as needed)
    if (htButtons[buttonIndex].isPressed() && 
        (currentMillis - htButtons[buttonIndex].pressedTime > longPressThresholdTime)) {
      // Long press detected - add your long press handling code here
    }
  }

  // Update last debounce time
  lastDebounceTime = currentMillis;
}