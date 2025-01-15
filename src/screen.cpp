#include "screen.h"
#include "haltech_can.h"
#include "haltech_button.h"

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

struct ButtonConfiguration {
    HaltechDisplayType_e displayType;
    HaltechUnit_e unit;
    uint8_t decimalPlaces;
    bool isToggleable;
};

constexpr uint8_t nButtons = 16;

constexpr ButtonConfiguration defaultButtonConfigs[nButtons] = {
  // Value                    Unit      Decimals  Toggleable
    {HT_MANIFOLD_PRESSURE,    UNIT_PSI,        2, false},
    {HT_RPM,                  UNIT_RPM,        0, false},
    {HT_THROTTLE_POSITION,    UNIT_PERCENT,    0, false},
    {HT_COOLANT_TEMPERATURE,  UNIT_FAHRENHEIT, 1, false},
    {HT_OIL_PRESSURE,         UNIT_PSI,        1, false},
    {HT_OIL_TEMPERATURE,      UNIT_FAHRENHEIT, 1, false},
    {HT_WIDEBAND_OVERALL,     UNIT_LAMBDA,     2, false},
    {HT_AIR_TEMPERATURE,      UNIT_FAHRENHEIT, 1, false},
    {HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT,    0, true},
    {HT_TARGET_BOOST_LEVEL,   UNIT_PSI,        1, false},
    {HT_IGNITION_ANGLE,       UNIT_DEGREES,    1, false},
    {HT_BATTERY_VOLTAGE,      UNIT_VOLTS,      2, false},
    {HT_INTAKE_CAM_ANGLE_1,   UNIT_DEGREES,    1, false},
    {HT_VEHICLE_SPEED,        UNIT_MPH,        1, false},
    {HT_TOTAL_FUEL_USED,      UNIT_GALLONS,    4, false},
    {HT_KNOCK_LEVEL_1,        UNIT_DB,         2, false}
};

// Invoke the TFT_eSPI button class and create all the button objects
HaltechButton htButtons[16];

TFT_eSPI_Button menuButtons[MENU_NONE];
TFT_eSPI_Button valSelButtons[26];

uint8_t buttonToModifyIndex;

ScreenState_e currScreenState = STATE_MENU;

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
    File f = SPIFFS.open(CALIBRATION_FILE, "r");
    if (f) {
      if (f.readBytes((char *)calData, 14) == 14) {
        calDataOK = 1;
      }
      f.close();
      Serial.println("cal data: ");
      for (int i = 0; i < 5; i++)
        Serial.printf("%02x", calData[i]);
      Serial.printf("\n");
    }
  }

  if (calDataOK && REPEAT_CAL <= calData[0]) {
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

    calData[0] = REPEAT_CAL + 1;

    // store data
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
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

  loadLayout(tft, buttonWidth, buttonHeight);
}

void setupMenu() {
  tft.fillScreen(TFT_BLACK);
  
  // Common dimensions and spacing
  // TFT_HEIGHT and TFT_WIDTH are swapped from what they should be because we're in landscape
  const int BUTTON_WIDTH = TFT_HEIGHT / 5;
  const int LEFT_MARGIN = TFT_HEIGHT / 32;
  const int BUTTON_HEIGHT = TFT_WIDTH / 10;
  const int TEXT_HEIGHT = TFT_WIDTH / 10;  // Height for each line
  const int TOP_MARGIN = LEFT_MARGIN / 2;
  const int TEXT_YOFFSET = BUTTON_HEIGHT * 7 / 32;  // To center text vertically in the line
  
  int currentY = 0;  // Starting Y position
  HaltechButton* currentButton = &htButtons[buttonToModifyIndex];
  HaltechDashValue* dashValue = currentButton->dashValue;

  tft.setTextDatum(TL_DATUM);

  menuButtons[MENU_EXIT].initButtonUL(&tft, 0, currentY,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Exit"), 1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buttonconfigstr[16];
  sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex+1);
  tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

  char valueStr[10];
  float convertedValue = currentButton->dashValue->convertToUnit(currentButton->displayUnit);
  sprintf(valueStr, "%.*f", currentButton->decimalPlaces, convertedValue);
  tft.drawString(valueStr, TFT_HEIGHT - 100, currentY + TOP_MARGIN);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Select Value", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_VAL_SEL].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*3, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>(""), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Min:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_MIN_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);

  char minStr[10];
  sprintf(minStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMin);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(minStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + TEXT_YOFFSET);

  menuButtons[MENU_ALERT_MIN_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.setTextDatum(TL_DATUM);
  tft.drawString("Alert Max:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_MAX_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);

  char maxStr[10];
  sprintf(maxStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMax);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(maxStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + TEXT_YOFFSET);

  menuButtons[MENU_ALERT_MAX_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.setTextDatum(TL_DATUM);
  tft.drawString("Alert Beep:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_BEEP_OFF].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN,
                                      currentButton->alertBeep ? TFT_GREEN : TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("OFF"), 1);
  menuButtons[MENU_ALERT_BEEP_ON].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN,
                                      currentButton->alertBeep ? TFT_BLACK : TFT_GREEN, TFT_WHITE,
                                      const_cast<char*>("ON"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Flash:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_FLASH_OFF].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN,
                                      currentButton->alertFlash ? TFT_GREEN : TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("OFF"), 1);
  menuButtons[MENU_ALERT_FLASH_ON].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN,
                                      currentButton->alertFlash ? TFT_BLACK : TFT_GREEN, TFT_WHITE,
                                      const_cast<char*>("ON"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Precision:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_PRECISION_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);

  tft.setTextDatum(TC_DATUM);
  tft.drawString(String(currentButton->decimalPlaces), TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + TEXT_YOFFSET);

  menuButtons[MENU_PRECISION_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.setTextDatum(TL_DATUM);
  tft.drawString("Units:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_UNITS_BACK].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);

  tft.setTextDatum(TC_DATUM);
  tft.setFreeFont(LABEL2_FONT);
  tft.drawString(unitDisplayStrings[currentButton->displayUnit], TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + TEXT_YOFFSET);

  tft.setFreeFont(LABEL1_FONT);
  menuButtons[MENU_UNITS_FORWARD].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);

  currentY += TEXT_HEIGHT;

  tft.setTextDatum(TL_DATUM);
  tft.drawString("Button Type:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  uint32_t buttontypebuttoncurrentx = TFT_HEIGHT - BUTTON_WIDTH*2.5;
  menuButtons[MENU_BUTTON_TYPE_NONE].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("None"), 1);
  buttontypebuttoncurrentx += BUTTON_WIDTH;
  menuButtons[MENU_BUTTON_TYPE_MOMENT].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Moment"), 1);
  buttontypebuttoncurrentx += BUTTON_WIDTH;
  menuButtons[MENU_BUTTON_TYPE_TOGGLE].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Toggle"), 1);

  currentY += TEXT_HEIGHT;

  tft.drawString("Button Text:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_BUTTON_TEXT_SEL].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*3, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Select"), 1);

  // Draw all buttons
  tft.setFreeFont(LABEL1_FONT);
  for (int i = 0; i < MENU_NONE; i++) {
    if (i == MENU_VAL_SEL) {
      menuButtons[i].drawButton(false, currentButton->dashValue->name);
    } else {
      menuButtons[i].drawButton();
    }
  }
}

void setupValSelection() {

}

void screenLoop() {
  static unsigned long lastDebounceTime = 0;
  static ScreenState_e lastScreenState;
  const unsigned long debounceDelay = 20; // Adjust as needed
  
  // Non-blocking debounce
  if (millis() - lastDebounceTime < debounceDelay) {
    return; // Skip processing if not enough time has passed
  }

  uint16_t t_x = 0, t_y = 0;
  bool isValidTouch = tft.getTouch(&t_x, &t_y);

  switch (currScreenState) {
    case STATE_NORMAL:
      if (lastScreenState != currScreenState) {
        for (uint8_t i = 0; i < nButtons; i++) {
          htButtons[i].drawButton();
        }
      }
      lastScreenState = currScreenState;
      
      for (uint8_t buttonIndex = 0; buttonIndex < nButtons; buttonIndex++) {
        bool wasPressed = htButtons[buttonIndex].isPressed();
        bool buttonContainsTouch = isValidTouch && htButtons[buttonIndex].contains(t_x, t_y);
        
        // Combine touch detection and button state update
        htButtons[buttonIndex].press(buttonContainsTouch);

        // Only redraw if button state has changed
        if (htButtons[buttonIndex].isPressed() != wasPressed) {
          // Track pressed time for potential long press functionality
          if (htButtons[buttonIndex].isPressed()) {
            htButtons[buttonIndex].pressedTime = millis();
          }
          
          // Redraw button with appropriate state
          htButtons[buttonIndex].drawButton(htButtons[buttonIndex].isPressed());
        }
        // Long press detected
        if (htButtons[buttonIndex].isPressed() && 
            (millis() - htButtons[buttonIndex].pressedTime > longPressThresholdTime)) {
          // Clear the screen
          tft.fillScreen(TFT_BLACK);

          // change the state
          currScreenState = STATE_MENU;
          Serial.println("going to menu");

          // So the menu knows which button/info to modify
          buttonToModifyIndex = buttonIndex;
          break;
        }
      }
      break;
    case STATE_MENU:
      if (lastScreenState != currScreenState) {
        // draw menu
        setupMenu();
      }
      lastScreenState = currScreenState;

      for (uint8_t buttonIndex = 0; buttonIndex < MENU_NONE; buttonIndex++) {
        bool wasPressed = menuButtons[buttonIndex].isPressed();
        bool buttonContainsTouch = isValidTouch && menuButtons[buttonIndex].contains(t_x, t_y);
        
        // Combine touch detection and button state update
        menuButtons[buttonIndex].press(buttonContainsTouch);
      }

      if (menuButtons[MENU_EXIT].isPressed()) {
        currScreenState = STATE_NORMAL;
        Serial.println("going to main screen");
      }
      
      break;

    case STATE_VAL_SEL:
      currScreenState = STATE_MENU;
      break;
  }
  
  // Update last debounce time
  lastDebounceTime = millis();
}

ButtonConfiguration currentButtonConfigs[nButtons];

bool saveLayout() {
    // Ensure SPIFFS is mounted
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return false;
    }

    // Open file for writing
    File layoutFile = SPIFFS.open("/button_layout.bin", FILE_WRITE);
    if (!layoutFile) {
        Serial.println("Failed to open layout file for writing");
        return false;
    }

    // Write entire configuration array
    layoutFile.write(reinterpret_cast<const uint8_t*>(currentButtonConfigs), sizeof(currentButtonConfigs));

    layoutFile.close();
    return true;
}

bool loadLayout(TFT_eSPI &tft, int buttonWidth, int buttonHeight) {
    // Ensure SPIFFS is mounted
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed");
        return false;
    }

    // Check if layout file exists
    if (!SPIFFS.exists("/button_layout.bin")) {
        Serial.println("No saved layout found. Using default.");
        
        // Copy default layout
        for (uint8_t i = 0; i < nButtons; i++) {
            currentButtonConfigs[i] = {
                defaultButtonConfigs[i].displayType,
                defaultButtonConfigs[i].unit,
                defaultButtonConfigs[i].decimalPlaces,
                defaultButtonConfigs[i].isToggleable
            };
        }

        saveLayout();
    }

    // Open file for reading
    File layoutFile = SPIFFS.open("/button_layout.bin", FILE_READ);
    if (!layoutFile) {
        Serial.println("Failed to open layout file for reading");
        return false;
    }

    // Read entire configuration array
    layoutFile.read(reinterpret_cast<uint8_t*>(currentButtonConfigs), sizeof(currentButtonConfigs));

    layoutFile.close();

    // Set up buttons with saved configuration
    for (uint8_t i = 0; i < nButtons; i++) {
        htButtons[i].initButton(&tft, 
            i % 4 * buttonWidth, 
            i / 4 * buttonHeight, 
            buttonWidth, 
            buttonHeight, 
            TFT_GREEN, 
            TFT_BLACK, 
            TFT_WHITE, 
            1, 
            &dashValues[currentButtonConfigs[i].displayType], 
            currentButtonConfigs[i].unit,
            currentButtonConfigs[i].decimalPlaces,
            currentButtonConfigs[i].isToggleable);
        htButtons[i].drawButton();
    }

    return true;
}
