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
    buttonMode_e mode;
};

constexpr uint8_t nButtons = 16;

constexpr ButtonConfiguration defaultButtonConfigs[nButtons] = {
  // Value                    Unit      Decimals  Mode
    {HT_MANIFOLD_PRESSURE,    UNIT_PSI,        2, BUTTON_MODE_NONE},
    {HT_RPM,                  UNIT_RPM,        0, BUTTON_MODE_NONE},
    {HT_THROTTLE_POSITION,    UNIT_PERCENT,    0, BUTTON_MODE_NONE},
    {HT_COOLANT_TEMPERATURE,  UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE},
    {HT_OIL_PRESSURE,         UNIT_PSI,        1, BUTTON_MODE_NONE},
    {HT_OIL_TEMPERATURE,      UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE},
    {HT_WIDEBAND_OVERALL,     UNIT_LAMBDA,     2, BUTTON_MODE_NONE},
    {HT_AIR_TEMPERATURE,      UNIT_FAHRENHEIT, 1, BUTTON_MODE_NONE},
    {HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT,    0, BUTTON_MODE_TOGGLE},
    {HT_TARGET_BOOST_LEVEL,   UNIT_PSI,        1, BUTTON_MODE_NONE},
    {HT_IGNITION_ANGLE,       UNIT_DEGREES,    1, BUTTON_MODE_NONE},
    {HT_BATTERY_VOLTAGE,      UNIT_VOLTS,      2, BUTTON_MODE_NONE},
    {HT_INTAKE_CAM_ANGLE_1,   UNIT_DEGREES,    1, BUTTON_MODE_NONE},
    {HT_VEHICLE_SPEED,        UNIT_MPH,        1, BUTTON_MODE_NONE},
    {HT_TOTAL_FUEL_USED,      UNIT_GALLONS,    4, BUTTON_MODE_NONE},
    {HT_KNOCK_LEVEL_1,        UNIT_DB,         2, BUTTON_MODE_NONE},
};

QueueHandle_t screenQueue;

// Invoke the TFT_eSPI button class and create all the button objects
HaltechButton htButtons[16];

MenuButton menuButtons[MENU_NONE];
MenuButton valSelButtons[26];

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

// Common dimensions and spacing
// TFT_HEIGHT and TFT_WIDTH are swapped from what they should be because we're in landscape
const int BUTTON_WIDTH = TFT_HEIGHT / 5;
const int LEFT_MARGIN = TFT_HEIGHT / 32;
const int BUTTON_HEIGHT = TFT_WIDTH / 10;
const int TEXT_HEIGHT = TFT_WIDTH / 10;  // Height for each line
const int TOP_MARGIN = LEFT_MARGIN / 2;
const int TEXT_YOFFSET = BUTTON_HEIGHT * 7 / 32;  // To center text vertically in the line

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

  // Create a queue for screen updates
  screenQueue = xQueueCreate(10, sizeof(HaltechDashValue));
}

void setupMenu() {
  tft.fillScreen(TFT_BLACK);
  
  int currentY = 0;  // Starting Y position
  HaltechButton* currentButton = &htButtons[buttonToModifyIndex];

  tft.setTextDatum(TL_DATUM);

  menuButtons[MENU_BACK].initButtonUL(&tft, 0, currentY,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Back"), 1);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  char buttonconfigstr[17];
  sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex+1);
  tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

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
  menuButtons[MENU_ALERT_MIN_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Max:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_MAX_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_ALERT_MAX_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Beep:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_BEEP_OFF].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("OFF"), 1);
  menuButtons[MENU_ALERT_BEEP_ON].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("ON"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Alert Flash:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_ALERT_FLASH_OFF].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("OFF"), 1);
  menuButtons[MENU_ALERT_FLASH_ON].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("ON"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Decimal Places:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_DECIMALS_DOWN].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_DECIMALS_UP].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);
  
  currentY += TEXT_HEIGHT;

  tft.drawString("Units:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_UNITS_BACK].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*2.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("-"), 1);
  menuButtons[MENU_UNITS_FORWARD].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH/2, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("+"), 1);

  currentY += TEXT_HEIGHT;

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

  currentY += TEXT_HEIGHT;

  tft.drawString("Button Text:", LEFT_MARGIN, currentY + TEXT_YOFFSET);
  menuButtons[MENU_BUTTON_TEXT_SEL].initButton(&tft, TFT_HEIGHT - BUTTON_WIDTH*1.5, currentY + BUTTON_HEIGHT/2,
                                      BUTTON_WIDTH*3, BUTTON_HEIGHT, TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Select"), 1);

  drawMenu();
}

void drawMenu() {
  HaltechButton* currentButton = &htButtons[buttonToModifyIndex];

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  int currentY = 0;  // Starting Y position

  // Draw current button value
  char valueStr[10];
  float convertedValue = currentButton->dashValue->convertToUnit(currentButton->displayUnit);
  sprintf(valueStr, "%.*f", currentButton->decimalPlaces, convertedValue);
  tft.drawString(valueStr, TFT_HEIGHT - 100, currentY + TOP_MARGIN);

  currentY += TEXT_HEIGHT;

  // Draw Alert Min value
  char minStr[10];
  sprintf(minStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMin);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(minStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, TEXT_HEIGHT*2 + TEXT_YOFFSET);

  // Draw Alert Max value
  char maxStr[10];
  sprintf(maxStr, "%.*f", currentButton->decimalPlaces, currentButton->alertMax);
  tft.drawString(maxStr, TFT_HEIGHT - BUTTON_WIDTH*1.5, TEXT_HEIGHT*3 + TEXT_YOFFSET);

  // Draw Decimal Places value
  tft.drawString(String(currentButton->decimalPlaces), TFT_HEIGHT - BUTTON_WIDTH*1.5, TEXT_HEIGHT*6 + TEXT_YOFFSET);

  // Draw Units 
  tft.setFreeFont(LABEL2_FONT);
  tft.drawString(unitDisplayStrings[currentButton->displayUnit], TFT_HEIGHT - BUTTON_WIDTH*1.5, TEXT_HEIGHT*7 + TEXT_YOFFSET);
  tft.setFreeFont(LABEL1_FONT);

  // Draw all buttons
  tft.setFreeFont(LABEL1_FONT);
  for (int i = 0; i < MENU_NONE; i++) {
    Serial.printf("drawing button %u\n", i);

    switch (i) {
      case MENU_VAL_SEL:
        menuButtons[i].drawButton(false, currentButton->dashValue->name);
        break;
      case MENU_BUTTON_MODE_MOMENTARY:
        menuButtons[i].drawButton(false, "", currentButton->mode == BUTTON_MODE_MOMENTARY);
        Serial.printf("drawing button type momentary %u\n", currentButton->mode == BUTTON_MODE_MOMENTARY);
        break;
      case MENU_BUTTON_MODE_TOGGLE:
        menuButtons[i].drawButton(false, "", currentButton->mode == BUTTON_MODE_TOGGLE);
        Serial.printf("drawing button type toggle %u\n", currentButton->mode == BUTTON_MODE_TOGGLE);
        break;
      case MENU_BUTTON_MODE_NONE:
        menuButtons[i].drawButton(false, "", currentButton->mode == BUTTON_MODE_NONE);
        Serial.printf("drawing button type none %u\n", currentButton->mode == BUTTON_MODE_NONE);
        break;
      
      default:
        menuButtons[i].drawButton();
    }
  }
}

void screenLoop() {

  // process any incoming CAN messages
  HaltechDashValue dashValue;
  while (xQueueReceive(screenQueue, &dashValue, 0) == pdTRUE) {
    // Update the screen with the new dash value
    // Find the corresponding button and update its value
    for (uint8_t i = 0; i < nButtons; i++) {
      if (htButtons[i].dashValue->can_id == dashValue.can_id) {
        htButtons[i].dashValue->scaled_value = dashValue.scaled_value;
        htButtons[i].drawValue();
        break;
      }
    }
  }

  static unsigned long lastDebounceTime = 0;
  static ScreenState_e lastScreenState;
  const unsigned long debounceDelay = 10;
  HaltechButton* currentButton;
  uint16_t t_x = 0, t_y = 0;
  bool isValidTouch = tft.getTouch(&t_x, &t_y);

  switch (currScreenState) {
    case STATE_NORMAL:
      if (lastScreenState != currScreenState) {
        tft.fillScreen(TFT_BLACK);
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
        lastDebounceTime = millis(); // Reset debounce time when entering menu
      }
      lastScreenState = currScreenState;

      for (uint8_t buttonIndex = 0; buttonIndex < MENU_NONE; buttonIndex++) {
        bool wasPressed = menuButtons[buttonIndex].isPressed();
        bool buttonContainsTouch = isValidTouch && menuButtons[buttonIndex].contains(t_x, t_y);
        
        // Combine touch detection and button state update
        menuButtons[buttonIndex].press(buttonContainsTouch);

        // Only process actions on button state change or just pressed
        if (menuButtons[buttonIndex].justPressed()) {
          Serial.printf("button %u pressed\n", buttonIndex);
          currentButton = &htButtons[buttonToModifyIndex];

          switch(buttonIndex) {
            case MENU_BACK:
              currScreenState = STATE_NORMAL;
              break;
            case MENU_VAL_SEL:
              currScreenState = STATE_VAL_SEL;
              break;
            case MENU_BUTTON_TEXT_SEL:
              //currScreenState = STATE_BUTTON_TEXT_SEL;
              break;
            case MENU_ALERT_MIN_DOWN: {
              float increment = pow(10, -currentButton->decimalPlaces);
              currentButton->alertMin -= increment;
              drawMenu();
              break;
            }
            case MENU_ALERT_MIN_UP: {
              float increment = pow(10, -currentButton->decimalPlaces);
              currentButton->alertMin += increment;
              drawMenu();
              break;
            }
            case MENU_ALERT_MAX_DOWN: {
              float increment = pow(10, -currentButton->decimalPlaces);
              currentButton->alertMax -= increment;
              drawMenu();
              break;
            }
            case MENU_ALERT_MAX_UP: {
              float increment = pow(10, -currentButton->decimalPlaces);
              currentButton->alertMax += increment;
              drawMenu();
              break;
            }
            case MENU_ALERT_BEEP_OFF:
              currentButton->alertBeep = false;
              drawMenu();
              break;
            case MENU_ALERT_BEEP_ON:
              currentButton->alertBeep = true;
              drawMenu();
              break;
            case MENU_ALERT_FLASH_OFF:
              currentButton->alertFlash = false;
              drawMenu();
              break;
            case MENU_ALERT_FLASH_ON:
              currentButton->alertFlash = true;
              drawMenu();
              break;
            case MENU_DECIMALS_DOWN:
              if (currentButton->decimalPlaces > 0) {
                currentButton->decimalPlaces -= 1;
              }
              drawMenu();
              Serial.println("decimals down");
              break;
            case MENU_DECIMALS_UP:
              if (currentButton->decimalPlaces < 5) { // Reasonable upper limit
                currentButton->decimalPlaces += 1;
              }
              drawMenu();
              Serial.println("decimals up");
              break;
            case MENU_UNITS_BACK:
              currentButton->changeUnits(DIRECTION_PREVIOUS);
              drawMenu();
              break;
            case MENU_UNITS_FORWARD:
              currentButton->changeUnits(DIRECTION_NEXT);
              drawMenu();
              break;
            case MENU_BUTTON_MODE_NONE:
              currentButton->mode = BUTTON_MODE_NONE;
              drawMenu();
              break;
            case MENU_BUTTON_MODE_MOMENTARY:
              currentButton->mode = BUTTON_MODE_MOMENTARY;
              drawMenu();
              break;
            case MENU_BUTTON_MODE_TOGGLE:
              currentButton->mode = BUTTON_MODE_TOGGLE;
              drawMenu();
              break;
          }
        }
      }
      break;

    case STATE_VAL_SEL:
      if (lastScreenState != currScreenState) {
        setupSelectValueScreen();
        drawSelectValueScreen();
      }
      //currScreenState = STATE_MENU;
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
                defaultButtonConfigs[i].mode
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
            currentButtonConfigs[i].mode);
        htButtons[i].drawButton();
    }

    return true;
}

int buttonToModify = -1;
int currentPage = 0;
const int valuesPerPage = 22; // 2 columns * 11 rows

void setupSelectValueScreen() {
    tft.fillScreen(TFT_BLACK);
    int currentY = 0;  // Starting Y position
    tft.setTextDatum(TL_DATUM);

    valSelButtons[0].initButtonUL(&tft, 0, currentY,
                                      BUTTON_WIDTH, BUTTON_HEIGHT, TFT_RED, TFT_BLACK, TFT_WHITE,
                                      const_cast<char*>("Back"), 1);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buttonconfigstr[17];
    sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex+1);
    tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

    currentY += TEXT_HEIGHT;

    for (int i = 0; i < valuesPerPage; ++i) {
        int index = currentPage * valuesPerPage + i;
        if (index >= HT_NONE) break; // No more values to display

        int x = (i % 2) * (TFT_HEIGHT / 2); // 2 columns
        int y = currentY + (i / 2) * BUTTON_HEIGHT;

        valSelButtons[i + 1].initButtonUL(&tft, x, y,
                                          TFT_HEIGHT / 2 - LEFT_MARGIN, BUTTON_HEIGHT,
                                          TFT_GREEN, TFT_BLACK, TFT_WHITE,
                                          const_cast<char*>(dashValues[index].name), 1);
    }
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void drawSelectValueScreen() {
    tft.fillScreen(TFT_BLACK);
    int currentY = 0;  // Starting Y position
    tft.setTextDatum(TL_DATUM);

    valSelButtons[0].drawButton();

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char buttonconfigstr[17];
    sprintf(buttonconfigstr, "Button %u Config", buttonToModifyIndex + 1);
    tft.drawString(buttonconfigstr, LEFT_MARGIN + BUTTON_WIDTH, currentY + TOP_MARGIN);

    currentY += TEXT_HEIGHT;

    int startIndex = currentPage * valuesPerPage;
    int endIndex = MIN(startIndex + valuesPerPage, HT_NONE);

    for (int i = startIndex; i < endIndex; ++i) {
        int x = (i % 2) * (TFT_HEIGHT / 2); // 2 columns
        int y = currentY + (i / 2) * BUTTON_HEIGHT;

        valSelButtons[i - startIndex + 1].drawButton();
    }
}

void handleValSelValueSelection(int valueIndex) {
    // Update the button configuration with the selected value
    currentButtonConfigs[buttonToModifyIndex].displayType = static_cast<HaltechDisplayType_e>(valueIndex);

    // Return to the menu screen
    currScreenState = STATE_MENU;
}

void navigateValSelToNextPage() {
    if ((currentPage + 1) * valuesPerPage < HT_NONE) {
        currentPage++;
        drawSelectValueScreen();
    }
}

void navigateValSelToPreviousPage() {
    if (currentPage > 0) {
        currentPage--;
        drawSelectValueScreen();
    }
}