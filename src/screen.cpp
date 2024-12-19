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

TFT_eSPI_Button menuButtons[N_MENU_BUTTONS];
TFT_eSPI_Button valSelButtons[26];

uint8_t buttonToModifyIndex;

ScreenState_e currScreenState;

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

typedef enum {
  MENU_EXIT,
  MENU_SEL_VAL,
  MENU_ALERT_MIN_DOWN,
  MENU_ALERT_MIN_UP,
  MENU_ALERT_MAX_DOWN,
  MENU_ALERT_MAX_UP,
  MENU_ALERT_BEEP_OFF,
  MENU_ALERT_BEEP_ON,
  MENU_ALERT_FLASH_OFF,
  MENU_ALERT_FLASH_ON,
  MENU_PRECISION_DOWN,
  MENU_PRECISION_UP,
  MENU_UNITS_BACK,
  MENU_UNITS_FORWARD,
  MENU_NONE,
} menuButtonName_e;

void setupMenu() {
  tft.fillScreen(TFT_BLACK);
  
  // Common dimensions and spacing
  const int BUTTON_HEIGHT = TFT_HEIGHT / 12;
  const int BUTTON_WIDTH = TFT_WIDTH / 14;
  const int SPACING = TFT_WIDTH / 5;
  const int TEXT_HEIGHT = TFT_HEIGHT / 10;  // Height for each line
  const int LEFT_MARGIN = TFT_WIDTH / 10;
  const int TEXT_YOFFSET = TEXT_HEIGHT / 2;  // To center text vertically in the line
  
  int currentY = TEXT_YOFFSET;  // Starting Y position
  HaltechButton* currentButton = &htButtons[buttonToModifyIndex];
  HaltechDashValue* dashValue = currentButton->dashValue;

  menuButtons[MENU_EXIT].initButton(&tft, LEFT_MARGIN + BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED, TFT_RED, TFT_WHITE,
                                      const_cast<char*>("Exit"), 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Button Config", LEFT_MARGIN + BUTTON_WIDTH + SPACING, currentY + TEXT_YOFFSET, 2);
  char valueStr[10];
  sprintf(valueStr, "%.*f", currentButton->decimalPlaces, dashValue->scaled_value);
  tft.drawString(valueStr, TFT_WIDTH - 100, currentY + TEXT_YOFFSET, 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Select Value", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  char namecopy[30];
  strcpy(namecopy, currentButton->dashValue->name);
  menuButtons[MENU_SEL_VAL].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*2, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      namecopy, 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Min:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_MIN_DOWN].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("-"), 2);
  char minStr[10];
  sprintf(minStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMin);
  tft.drawString(minStr, TFT_WIDTH - BUTTON_WIDTH*1.2, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_MIN_UP].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("+"), 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Max:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_MAX_DOWN].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("-"), 2);
  char maxStr[10];
  sprintf(maxStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMax);
  tft.drawString(maxStr, TFT_WIDTH - BUTTON_WIDTH*1.2, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_MAX_UP].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("+"), 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Beep:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_BEEP_OFF].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, currentButton->alertBeep ? TFT_GREEN : TFT_BLUE,
                                      currentButton->alertBeep ? TFT_GREEN : TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("OFF"), 2);
  menuButtons[MENU_ALERT_BEEP_ON].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, currentButton->alertBeep ? TFT_BLUE : TFT_GREEN,
                                      currentButton->alertBeep ? TFT_BLUE : TFT_GREEN, TFT_WHITE,
                                      const_cast<char*>("ON"), 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Flash:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_ALERT_FLASH_OFF].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, currentButton->alertFlash ? TFT_GREEN : TFT_BLUE,
                                      currentButton->alertFlash ? TFT_GREEN : TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("OFF"), 2);
  menuButtons[MENU_ALERT_FLASH_ON].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, currentButton->alertFlash ? TFT_BLUE : TFT_GREEN,
                                      currentButton->alertFlash ? TFT_BLUE : TFT_GREEN, TFT_WHITE,
                                      const_cast<char*>("ON"), 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Precision:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_PRECISION_DOWN].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("-"), 2);
  tft.drawString(String(currentButton->decimalPlaces), TFT_WIDTH - BUTTON_WIDTH*1.2, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_PRECISION_UP].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("+"), 2);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Units:", LEFT_MARGIN, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_UNITS_BACK].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH*2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("-"), 2);
  tft.drawString(dashValue->name, TFT_WIDTH - BUTTON_WIDTH*1.2, currentY + TEXT_YOFFSET, 2);
  menuButtons[MENU_UNITS_FORWARD].initButton(&tft, TFT_WIDTH - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_BLUE, TFT_BLUE, TFT_WHITE,
                                      const_cast<char*>("+"), 2);

  // Draw all buttons
  for (int i = 0; i < MENU_NONE-1; i++) {
    menuButtons[i].drawButton();
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

      for (uint8_t buttonIndex = 0; buttonIndex < N_MENU_BUTTONS; buttonIndex++) {
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
        
            // // Set up default layout
            // htButtons[i].initButton(&tft, 
            //     i % 4 * buttonWidth, 
            //     i / 4 * buttonHeight, 
            //     buttonWidth, 
            //     buttonHeight, 
            //     TFT_GREEN, 
            //     TFT_BLACK, 
            //     TFT_WHITE, 
            //     1, 
            //     &dashValues[defaultButtonConfigs[i].displayType], 
            //     defaultButtonConfigs[i].unit,
            //     defaultButtonConfigs[i].decimalPlaces);
        }

        saveLayout();
        return false;
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
