#include "screen.h"
#include "haltech_can.h"
#include "haltech_button.h"
#include "config.h"

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library

struct ButtonConfiguration {
  HaltechDisplayType_e displayType;
  HaltechUnit_e displayUnit;
  uint8_t decimalPlaces;
  buttonMode_e mode;
  float alertMin;
  float alertMax;
  bool alertBeepEnabled;
  bool alertFlashEnabled;
};

constexpr ButtonConfiguration defaultButtonConfigs[N_BUTTONS] = {
  // Value                  Unit      Decimals  Mode              Alert Min  Alert Max  Beep  Flash
  {HT_MANIFOLD_PRESSURE,    UNIT_PSI,        2, BUTTON_MODE_NONE, -15, 15, false, false},
  {HT_RPM,                  UNIT_RPM,        0, BUTTON_MODE_NONE, -1, 8000, false, false},
  {HT_THROTTLE_POSITION,    UNIT_PERCENT,    0, BUTTON_MODE_NONE, -1, 101, false, false},
  {HT_COOLANT_TEMPERATURE,  UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE, 0, 230, false, false},
  {HT_OIL_PRESSURE,         UNIT_PSI,        1, BUTTON_MODE_NONE, -1, 150, false, false},
  {HT_OIL_TEMPERATURE,      UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE, -1, 300, false, false},
  {HT_WIDEBAND_OVERALL,     UNIT_LAMBDA,     2, BUTTON_MODE_NONE, -0.1, 2, false, false},
  {HT_AIR_TEMPERATURE,      UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE, -1, 300, false, false},
  {HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT,    0, BUTTON_MODE_TOGGLE, -1, 101, false, false},
  {HT_TARGET_BOOST_LEVEL,   UNIT_PSI,        1, BUTTON_MODE_NONE, -1, 30, false, false},
  {HT_IGNITION_ANGLE,       UNIT_DEGREES,    1, BUTTON_MODE_NONE, -10, 60, false, false},
  {HT_BATTERY_VOLTAGE,      UNIT_VOLTS,      2, BUTTON_MODE_NONE, 10, 20, false, false},
  {HT_INTAKE_CAM_ANGLE_1,   UNIT_DEGREES,    1, BUTTON_MODE_NONE, -1, 50, false, false},
  {HT_VEHICLE_SPEED,        UNIT_MPH,        1, BUTTON_MODE_NONE, -1, 60, false, false},
  {HT_TOTAL_FUEL_USED,      UNIT_GALLONS,    4, BUTTON_MODE_NONE, -1, 1000, false, false},
  {HT_KNOCK_LEVEL_1,        UNIT_DB,         2, BUTTON_MODE_NONE, -1, 100, false, false},
};

// Invoke the TFT_eSPI button class and create all the button objects
HaltechButton htButtons[N_BUTTONS];

MenuButton menuButtons[MENU_NONE];
MenuButton valSelButtons[VAL_SEL_NONE];

uint8_t buttonToModifyIndex;

ScreenState_e currScreenState = STATE_NORMAL;

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

// Common dimensions and spacing
// TFT_HEIGHT and TFT_WIDTH are swapped from what they should be because we're in landscape
const int BUTTON_WIDTH = TFT_HEIGHT / 5;
const int LEFT_MARGIN = TFT_HEIGHT / 32;
const int BUTTON_HEIGHT = TFT_WIDTH / 9;
const int TOP_MARGIN = LEFT_MARGIN / 2;
const int TEXT_YOFFSET = BUTTON_HEIGHT * 7 / 32;  // To center text vertically in the line

void screenSetup() {
  tft.init();
  
  tft.setRotation(1);

  touch_calibrate();

  tft.fillScreen(TFT_BLACK);

  pinMode(PIN_BEEP, OUTPUT);

  delay(20);

  digitalWrite(PIN_BEEP, HIGH);

  loadLayout(tft);

  tft.setFreeFont(LABEL2_FONT);
  
}

void setupMenu() {
  tft.fillScreen(TFT_BLACK);
  
  int currentY = 0;  // Starting Y position
  HaltechButton* buttonToModify = &htButtons[buttonToModifyIndex];

  tft.setTextDatum(TL_DATUM);
  // tft.setFreeFont(LABEL1_FONT);

  menuButtons[MENU_BACK].initButtonUL(&tft, 0, currentY,
                                      BUTTON_WIDTH*1.5, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Save/Exit"), 1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buttonconfigstr[17];
  sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex+1);
  tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH*1.5, currentY + TOP_MARGIN);

  currentY += BUTTON_HEIGHT;

  tft.drawString("Select Value", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_VAL_SEL].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*3, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>(""), 1);
  
  currentY += BUTTON_HEIGHT;

  tft.drawString("Alert Min:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_MIN_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_ALERT_MIN_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += BUTTON_HEIGHT;

  tft.drawString("Alert Max:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_MAX_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_ALERT_MAX_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += BUTTON_HEIGHT;

  tft.drawString("Alerts Type:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_BEEP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Beep"), 1);
  menuButtons[MENU_ALERT_FLASH].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Flash"), 1);
  
  currentY += BUTTON_HEIGHT;

  tft.drawString("Decimal Places:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_DECIMALS_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_DECIMALS_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += BUTTON_HEIGHT;

  tft.drawString("Units:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_UNITS_BACK].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_UNITS_FORWARD].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);

  currentY += BUTTON_HEIGHT;

  tft.drawString("Button Type:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  uint32_t buttontypebuttoncurrentx = TFT_HEIGHT - BUTTON_WIDTH*2.5;
  menuButtons[MENU_BUTTON_MODE_NONE].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("None"), 1);
  buttontypebuttoncurrentx += BUTTON_WIDTH;
  menuButtons[MENU_BUTTON_MODE_MOMENTARY].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Moment"), 1);
  buttontypebuttoncurrentx += BUTTON_WIDTH;
  menuButtons[MENU_BUTTON_MODE_TOGGLE].initButton(&tft, buttontypebuttoncurrentx, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Toggle"), 1);

  currentY += BUTTON_HEIGHT;

  tft.drawString("Button Text:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_BUTTON_TEXT_SEL].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*3, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Select"), 1);

  drawMenu();
}

void drawMenu() {
  HaltechButton* buttonToModify = &htButtons[buttonToModifyIndex];

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  // tft.setFreeFont(LABEL1_FONT);

  // Draw current button value
  char valueStr[10];
  float convertedValue = buttonToModify->dashValue->convertToUnit(buttonToModify->displayUnit);
  sprintf(valueStr, "%.*f", buttonToModify->decimalPlaces, convertedValue);
  tft.drawString(valueStr, TFT_HEIGHT - 100, TOP_MARGIN);

  // Draw Alert Min value
  char minStr[10];
  sprintf(minStr, "%.*f", buttonToModify->decimalPlaces, buttonToModify->alertMin);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(minStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, BUTTON_HEIGHT*2 + TEXT_YOFFSET);

  // Draw Alert Max value
  char maxStr[10];
  sprintf(maxStr, "%.*f", buttonToModify->decimalPlaces, buttonToModify->alertMax);
  tft.drawString(maxStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, BUTTON_HEIGHT*3 + TEXT_YOFFSET);

  // Draw Decimal Places value
  tft.drawString(String(buttonToModify->decimalPlaces), TFT_HEIGHT - BUTTON_WIDTH*1.5, BUTTON_HEIGHT*5 + TEXT_YOFFSET);
  
  // Draw Units 
  // tft.setFreeFont(LABEL2_FONT);
  tft.drawString(unitDisplayStrings[buttonToModify->displayUnit], TFT_HEIGHT - BUTTON_WIDTH*1.5, BUTTON_HEIGHT*6 + TEXT_YOFFSET);

  // tft.setFreeFont(LABEL1_FONT);
  for (int i = 0; i < MENU_NONE; i++) {
    // Serial.printf("drawing button %u\n", i);
    switch (i) {
      case MENU_VAL_SEL:
        menuButtons[i].drawButton(false, buttonToModify->dashValue->name);
        break;
      case MENU_BUTTON_MODE_MOMENTARY:
        menuButtons[i].drawButton(false, "", buttonToModify->mode == BUTTON_MODE_MOMENTARY);
        // Serial.printf("drawing button type momentary %u\n", buttonToModify->mode == BUTTON_MODE_MOMENTARY);
        break;
      case MENU_BUTTON_MODE_TOGGLE:
        menuButtons[i].drawButton(false, "", buttonToModify->mode == BUTTON_MODE_TOGGLE);
        // Serial.printf("drawing button type toggle %u\n", buttonToModify->mode == BUTTON_MODE_TOGGLE);
        break;
      case MENU_BUTTON_MODE_NONE:
        menuButtons[i].drawButton(false, "", buttonToModify->mode == BUTTON_MODE_NONE);
        // Serial.printf("drawing button type none %u\n", buttonToModify->mode == BUTTON_MODE_NONE);
        break;
      case MENU_ALERT_BEEP:
        menuButtons[i].drawButton(false, "", htButtons[buttonToModifyIndex].alertBeepEnabled);
        break;
      case MENU_ALERT_FLASH:
        menuButtons[i].drawButton(false, "", htButtons[buttonToModifyIndex].alertFlashEnabled);
        break;
      default:
        menuButtons[i].drawButton();
    }
  }
}

void screenLoop() {

  // process any incoming CAN messages
  HaltechDashValue dashValue;
  // Update the screen with the new dash value
  // Find the corresponding button and update its value
  for (uint8_t i = 0; i < N_BUTTONS; i++) {
    if (htButtons[i].dashValue->can_id == dashValue.can_id) {
      htButtons[i].dashValue->scaled_value = dashValue.scaled_value;
      htButtons[i].drawValue();
      break;
    }
  }

  static unsigned long lastDebounceTime = 0;
  static ScreenState_e lastScreenState = STATE_NONE;

  // beep is global so save states here instead of in the button object
  static bool beepState = false;
  static bool flashState = false;
  static uint64_t lastBeepTime = 0;
  static uint64_t lastFlashTime = 0;

  const unsigned long debounceDelay = 10;
  HaltechButton* buttonToModify;
  uint16_t t_x = 0, t_y = 0;
  bool isValidTouch = tft.getTouch(&t_x, &t_y);

  static bool waitingForTouchRelease = false;
  bool justChangedStates = false;
  // Handle state transitions and touch release
  if (lastScreenState != currScreenState) {
    waitingForTouchRelease = true;  // Set flag on state change
    justChangedStates = true;
  }
  lastScreenState = currScreenState;

  // If waiting for release and no touch detected, clear the flag
  if (waitingForTouchRelease && !isValidTouch) {
    waitingForTouchRelease = false;
  }
      
  bool isAButtonBeeping = false;

  // process drawing
  switch (currScreenState) {
    case STATE_NORMAL:
      if (justChangedStates) {
        tft.fillScreen(TFT_BLACK);
        for (uint8_t i = 0; i < N_BUTTONS; i++) {
          htButtons[i].drawButton();
        }
      }

      for (uint8_t buttonIndex = 0; buttonIndex < N_BUTTONS; buttonIndex++) {
        // check if we need to be beeping (when beep is enabled and alerting state)
        if (htButtons[buttonIndex].alertBeepEnabled && htButtons[buttonIndex].alertConditionMet) {
          isAButtonBeeping = true;
          Serial.printf("button %d is beeping\n", buttonIndex);
        }
        if (htButtons[buttonIndex].alertFlashEnabled && htButtons[buttonIndex].alertConditionMet && flashState != htButtons[buttonIndex].isInverted) {
          htButtons[buttonIndex].drawButton(flashState);
        }
      }
      break;
    case STATE_MENU:
      if (justChangedStates) {
        // draw menu
        setupMenu();
        lastDebounceTime = millis(); // Reset debounce time when entering menu
      }
      break;
    case STATE_VAL_SEL:
      if (justChangedStates) {
        setupSelectValueScreen();
        drawSelectValueScreen();
      }
      break;
    case STATE_BUTTON_TEXT_SEL:
      if (justChangedStates) {
        // setupSelectButtonTextScreen();
        // drawSelectButtonTextScreen();
      }
      break;
  }

  // process touch
  if (!waitingForTouchRelease) {
    switch (currScreenState) {
      case STATE_NORMAL:
        for (uint8_t buttonIndex = 0; buttonIndex < N_BUTTONS; buttonIndex++) {
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
            htButtons[buttonIndex].drawButton();
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
        for (uint8_t buttonIndex = 0; buttonIndex < MENU_NONE; buttonIndex++) {
          bool wasPressed = menuButtons[buttonIndex].isPressed();
          bool buttonContainsTouch = isValidTouch && menuButtons[buttonIndex].contains(t_x, t_y);
          
          // Combine touch detection and button state update
          menuButtons[buttonIndex].press(buttonContainsTouch);

          // Only process actions on button state change or just pressed
          if (menuButtons[buttonIndex].justPressed()) {
            Serial.printf("menu button %u pressed\n", buttonIndex);
            buttonToModify = &htButtons[buttonToModifyIndex];

            switch(buttonIndex) {
              case MENU_BACK:
                updateButtonConfig(buttonToModifyIndex, buttonToModify);
                saveLayout();
                currScreenState = STATE_NORMAL;
                break;
              case MENU_VAL_SEL:
                currScreenState = STATE_VAL_SEL;
                break;
              case MENU_BUTTON_TEXT_SEL:
                //currScreenState = STATE_BUTTON_TEXT_SEL;
                break;
              case MENU_ALERT_MIN_DOWN: {
                float increment = pow(10, -buttonToModify->decimalPlaces);
                buttonToModify->alertMin -= increment;
                drawMenu();
                break;
              }
              case MENU_ALERT_MIN_UP: {
                float increment = pow(10, -buttonToModify->decimalPlaces);
                buttonToModify->alertMin += increment;
                drawMenu();
                break;
              }
              case MENU_ALERT_MAX_DOWN: {
                float increment = pow(10, -buttonToModify->decimalPlaces);
                buttonToModify->alertMax -= increment;
                drawMenu();
                break;
              }
              case MENU_ALERT_MAX_UP: {
                float increment = pow(10, -buttonToModify->decimalPlaces);
                buttonToModify->alertMax += increment;
                drawMenu();
                break;
              }
              case MENU_ALERT_BEEP:
                buttonToModify->alertBeepEnabled = !buttonToModify->alertBeepEnabled;
                drawMenu();
                break;
              case MENU_ALERT_FLASH:
                buttonToModify->alertFlashEnabled = !buttonToModify->alertFlashEnabled;
                drawMenu();
                break;
              case MENU_DECIMALS_DOWN:
                if (buttonToModify->decimalPlaces > 0) {
                  buttonToModify->decimalPlaces -= 1;
                }
                drawMenu();
                Serial.println("decimals down");
                break;
              case MENU_DECIMALS_UP:
                if (buttonToModify->decimalPlaces < 5) { // Reasonable upper limit
                  buttonToModify->decimalPlaces += 1;
                }
                drawMenu();
                Serial.println("decimals up");
                break;
              case MENU_UNITS_BACK:
                buttonToModify->changeUnits(DIRECTION_PREVIOUS);
                drawMenu();
                break;
              case MENU_UNITS_FORWARD:
                buttonToModify->changeUnits(DIRECTION_NEXT);
                drawMenu();
                break;
              case MENU_BUTTON_MODE_NONE:
                buttonToModify->mode = BUTTON_MODE_NONE;
                drawMenu();
                break;
              case MENU_BUTTON_MODE_MOMENTARY:
                buttonToModify->mode = BUTTON_MODE_MOMENTARY;
                drawMenu();
                break;
              case MENU_BUTTON_MODE_TOGGLE:
                buttonToModify->mode = BUTTON_MODE_TOGGLE;
                drawMenu();
                break;
            }
          }
        }
        break;

      case STATE_VAL_SEL:
        for (uint8_t buttonIndex = 0; buttonIndex < VAL_SEL_NONE; buttonIndex++) {
          bool wasPressed = valSelButtons[buttonIndex].isPressed();
          bool buttonContainsTouch = isValidTouch && valSelButtons[buttonIndex].contains(t_x, t_y);
          
          // Combine touch detection and button state update
          valSelButtons[buttonIndex].press(buttonContainsTouch);

          // Only process actions on button state change or just pressed
          if (valSelButtons[buttonIndex].justPressed()) {
            // Serial.printf("valsel button %u pressed\n", buttonIndex);
            buttonToModify = &htButtons[buttonToModifyIndex];

            switch (buttonIndex) {
              case VAL_SEL_BACK:
                Serial.println("val sel exit");
                currScreenState = STATE_MENU;
                break;
              case VAL_SEL_PAGE_BACK:
                navigateValSelToPreviousPage();
                break;
              case VAL_SEL_PAGE_FORWARD:
                navigateValSelToNextPage();
                break;
              default:
                // Check if the button index is within the valid range
                if (buttonIndex < VAL_SEL_NONE) {
                  // Serial.printf("buttonindex %d\n", buttonIndex);
                  handleValSelValueSelection(buttonIndex);
                } else {
                  // Handle invalid button index case
                  Serial.printf("Invalid button index: %d\n", buttonIndex);
                }

                break;
            }
            break;
          }
        }

        break;
      case STATE_BUTTON_TEXT_SEL:
        break;
    }
  }

  if (millis() - lastBeepTime > 100) {
    // Serial.printf("changing beep state\n");
    beepState = !beepState;
    lastBeepTime = millis();
  }

  if (millis() - lastFlashTime > 200) {
    // Serial.printf("changing flash state\n");
    flashState = !flashState;
    lastFlashTime = millis();
  }

  if (isAButtonBeeping) {
    //Serial.printf("button is beeping\n");
    digitalWrite(PIN_BEEP, beepState);
  } else {
    digitalWrite(PIN_BEEP, HIGH);
  }
  
  // Update last debounce time
  lastDebounceTime = millis();
}

ButtonConfiguration currentButtonConfigs[N_BUTTONS];

bool saveLayout() {
  Serial.printf("saving layout\n");

  // Ensure SPIFFS is mounted
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return false;
  }

  // Save config version
  uint8_t configVersion = CURRENT_VERSION;
  File configFile = SPIFFS.open("/config_info.bin", FILE_WRITE);
  if (!configFile) {
    Serial.println("Failed to open config info file for writing");
    return false;
  }
  configFile.write(&configVersion, sizeof(configVersion));
  configFile.close();

  // Open file for writing
  File layoutFile = SPIFFS.open("/button_layout.bin", FILE_WRITE);
  if (!layoutFile) {
    Serial.println("Failed to open layout file for writing");
    return false;
  }

  // Write entire configuration array
  layoutFile.write(reinterpret_cast<const uint8_t*>(currentButtonConfigs), sizeof(currentButtonConfigs));
  layoutFile.close();

  Serial.println("Layout saved successfully");

  return true;
}

bool loadLayout(TFT_eSPI &tft) {
  // Ensure SPIFFS is mounted
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
    return false;
  }

  // Read config version
  uint8_t savedConfigVersion = 0;
  if (SPIFFS.exists("/config_info.bin")) {
    File configFile = SPIFFS.open("/config_info.bin", FILE_READ);
    if (configFile) {
      configFile.read(&savedConfigVersion, sizeof(savedConfigVersion));
      configFile.close();
      Serial.printf("Loaded config version: %u\n", savedConfigVersion);
    } else {
      Serial.println("Failed to open config info file for reading");
    }
  } else {
    Serial.println("No config info file found, using default version 0");
  }

  // Check if layout file exists
  if (!SPIFFS.exists("/button_layout.bin") || savedConfigVersion > CURRENT_VERSION) {
    Serial.println("Too new or no saved layout found. Using default.");
    
    // Copy default layout
    for (uint8_t i = 0; i < N_BUTTONS; i++) {
      currentButtonConfigs[i] = {
        defaultButtonConfigs[i].displayType,
        defaultButtonConfigs[i].displayUnit,
        defaultButtonConfigs[i].decimalPlaces,
        defaultButtonConfigs[i].mode,
        defaultButtonConfigs[i].alertMin,
        defaultButtonConfigs[i].alertMax,
        defaultButtonConfigs[i].alertBeepEnabled,
        defaultButtonConfigs[i].alertFlashEnabled
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
  for (uint8_t i = 0; i < N_BUTTONS; i++) {
    htButtons[i].initButton(&tft, 
        i % 4 * TFT_HEIGHT / 4,
        i / 4 * TFT_WIDTH / 4,
        TFT_HEIGHT / 4,
        TFT_WIDTH / 4,
        TFT_GREEN,
        TFT_BLACK,
        TFT_WHITE,
        1,
        &dashValues[currentButtonConfigs[i].displayType],
        currentButtonConfigs[i].displayUnit,
        currentButtonConfigs[i].decimalPlaces,
        currentButtonConfigs[i].mode,
        currentButtonConfigs[i].alertMin,
        currentButtonConfigs[i].alertMax,
        currentButtonConfigs[i].alertBeepEnabled,
        currentButtonConfigs[i].alertFlashEnabled);
    //htButtons[i].drawButton();
  }

  return true;
}

bool showUpdateScreen(uint32_t remoteVersion, uint32_t currentVersion) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setFreeFont(LABEL1_FONT);

  tft.drawString("Update Now?", TFT_HEIGHT / 2, TFT_WIDTH / 8);
  
  char remoteVersionString[6];
  sprintf(remoteVersionString, "%u", remoteVersion);
  tft.drawString("Remote Version:", TFT_HEIGHT / 2, TFT_WIDTH * 3 / 8 - 12);
  tft.drawString(remoteVersionString, TFT_HEIGHT / 2, TFT_WIDTH * 3 / 8 + 12);
  
  char currentVersionString[6];
  sprintf(currentVersionString, "%u", currentVersion);
  tft.drawString("Current Version:", TFT_HEIGHT / 2, TFT_WIDTH * 5 / 8 - 12);
  tft.drawString(currentVersionString, TFT_HEIGHT / 2, TFT_WIDTH * 5 / 8 + 12);

  // add buttons for yes or no
  MenuButton yesButton;
  yesButton.initButtonUL(&tft, TFT_HEIGHT * 3 / 4, TFT_WIDTH * 7 / 8,
                         TFT_HEIGHT / 4, TFT_WIDTH / 8, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                         const_cast<char*>("Yes"), 1);

  MenuButton noButton;
  noButton.initButtonUL(&tft, TFT_HEIGHT * 0 / 4, TFT_WIDTH * 7 / 8,
                        TFT_HEIGHT / 4, TFT_WIDTH / 8, TFT_RED, TFT_BLACK, TFT_WHITE,
                        const_cast<char*>("No"), 1);
  
  yesButton.drawButton();
  noButton.drawButton();

  bool doUpdate = false;
  while (true) {
    uint16_t t_x = 0, t_y = 0;
    bool isValidTouch = tft.getTouch(&t_x, &t_y);

    bool yesContains = yesButton.contains(t_x, t_y);
    bool noContains = noButton.contains(t_x, t_y);

    Serial.printf("yes: %d no: %d\n", yesContains, noContains);
    // Update button state with current touch
    yesButton.press(isValidTouch && yesContains);
    noButton.press(isValidTouch && noContains);

    if (isValidTouch) {
      Serial.printf("Touch at: (%d, %d)\n", t_x, t_y);
      if (yesButton.isPressed()) {
        doUpdate = true;
        break;
      } else if (noButton.isPressed()) {
        doUpdate = false;
        break;
      }
    }
    delay(20);
  }

  return doUpdate;
}

int currentPage = 0;
const int valuesPerPage = 16; // 2 columns * 8 rows

void setupSelectValueScreen() {
    Serial.println("setting up sel val screen");
    tft.fillScreen(TFT_BLACK);
    int currentY = 0;  // Starting Y position
    tft.setTextDatum(TL_DATUM);

    valSelButtons[VAL_SEL_BACK].initButtonUL(&tft, 0, currentY,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Back"), 1);
    valSelButtons[VAL_SEL_PAGE_BACK].initButtonUL(&tft, TFT_HEIGHT*4/6, currentY, TFT_HEIGHT/6, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE, const_cast<char*>("<"), 1);
    valSelButtons[VAL_SEL_PAGE_FORWARD].initButtonUL(&tft, TFT_HEIGHT*5/6, currentY, TFT_HEIGHT/6, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE, const_cast<char*>(">"), 1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buttonconfigstr[17];
    sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex+1);
    tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

    currentY += BUTTON_HEIGHT;

    for (int i = VAL_SEL_1; i <= valuesPerPage - 1; i++) {
        Serial.printf("init button %d\n", i);
        int index = currentPage * valuesPerPage + i;
        if (index >= HT_NONE) break; // No more values to display

        int x = (i % 2) * (TFT_HEIGHT / 2); // 2 columns
        int y = currentY + (i / 2) * BUTTON_HEIGHT;

        valSelButtons[i].initButtonUL(&tft, x, y,
                                      TFT_HEIGHT / 2, BUTTON_HEIGHT,
                                      TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>(dashValues[index].name), 1);
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void drawSelectValueScreen() {
    Serial.println("drawing sel val screen");
    tft.fillScreen(TFT_BLACK);
    int currentY = 0;  // Starting Y position
    tft.setTextDatum(TL_DATUM);
    // tft.setFreeFont(LABEL1_FONT);

    valSelButtons[VAL_SEL_BACK].drawButton();

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buttonconfigstr[17];
    sprintf(buttonconfigstr, "Select Value");
    tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

    valSelButtons[VAL_SEL_PAGE_BACK].drawButton();
    valSelButtons[VAL_SEL_PAGE_FORWARD].drawButton();

    // tft.setFreeFont(LABEL2_FONT);

    currentY += BUTTON_HEIGHT;
    int pageOffset = currentPage * valuesPerPage;

    for (int i = VAL_SEL_1; i <= valuesPerPage - 1; i++) {
      if (i + pageOffset >= HT_NONE) {
        break;
      }
      // Serial.printf("drawing val sel button %d with name %s from dashval[%d]\n", i, dashValues[i+pageOffset].name, i);
      valSelButtons[i].drawButton(false, dashValues[i+pageOffset].name, false);
      int x = (i % 2) * (TFT_HEIGHT / 2); // 2 columns
      int y = currentY + (i / 2) * BUTTON_HEIGHT;
    }
    
    // Serial.println("end drawing val sel");
}

void updateButtonConfig(uint8_t buttonToModifyIndex, HaltechButton* buttonToModify) {
  Serial.printf("updating button %u\n", buttonToModifyIndex);
  // Update the button configuration with the selected value
  currentButtonConfigs[buttonToModifyIndex].displayType = buttonToModify->dashValue->type;
  currentButtonConfigs[buttonToModifyIndex].displayUnit = buttonToModify->displayUnit;
  currentButtonConfigs[buttonToModifyIndex].decimalPlaces = buttonToModify->decimalPlaces;
  currentButtonConfigs[buttonToModifyIndex].mode = buttonToModify->mode;
  currentButtonConfigs[buttonToModifyIndex].alertMin = buttonToModify->alertMin;
  currentButtonConfigs[buttonToModifyIndex].alertMax = buttonToModify->alertMax;
  currentButtonConfigs[buttonToModifyIndex].alertBeepEnabled = buttonToModify->alertBeepEnabled;
  currentButtonConfigs[buttonToModifyIndex].alertFlashEnabled = buttonToModify->alertFlashEnabled;
}

void handleValSelValueSelection(int valueIndex) {
    // Calculate the actual index in the dashValues array based on the current page
    int actualIndex = currentPage * valuesPerPage + valueIndex;

    // Ensure the index is within bounds
    if (actualIndex >= HT_NONE) {
        Serial.printf("Invalid value index: %d (actual index: %d)\n", valueIndex, actualIndex);
        return;
    }

    Serial.printf("handling value select for button %u with value index %d (actual index: %d)\n", buttonToModifyIndex, valueIndex, actualIndex);

    // Update the button configuration with the selected value
    currentButtonConfigs[buttonToModifyIndex].displayType = static_cast<HaltechDisplayType_e>(actualIndex);

    // Update the button's dashValue to the selected value
    htButtons[buttonToModifyIndex].dashValue = &dashValues[actualIndex];

    // Change units on the button to get a valid unit
    htButtons[buttonToModifyIndex].changeUnits(DIRECTION_NEXT);

    // Save and reload the layout
    saveLayout();
    loadLayout(tft);

    // Return to the menu screen
    currScreenState = STATE_MENU;
}

void navigateValSelToNextPage() {
    if ((currentPage + 1) * valuesPerPage < HT_NONE) {
        currentPage++;
        printf("advancing to page %d\n", currentPage);
        drawSelectValueScreen();
    }
}

void navigateValSelToPreviousPage() {
    if (currentPage > 0) {
        currentPage--;
        printf("returning to page %d\n", currentPage);
        drawSelectValueScreen();
    }
}