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
};

constexpr uint8_t nButtons = 16;

constexpr ButtonConfiguration defaultButtonConfigs[nButtons] = {
    {HT_MANIFOLD_PRESSURE,    UNIT_PSI,        2},
    {HT_RPM,                  UNIT_RPM,        0},
    {HT_THROTTLE_POSITION,    UNIT_PERCENT,    0},
    {HT_COOLANT_TEMPERATURE,  UNIT_FAHRENHEIT, 1},
    {HT_OIL_PRESSURE,         UNIT_PSI,        1},
    {HT_OIL_TEMPERATURE,      UNIT_FAHRENHEIT, 1},
    {HT_WIDEBAND_OVERALL,     UNIT_LAMBDA,     2},
    {HT_AIR_TEMPERATURE,      UNIT_FAHRENHEIT, 1},
    {HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT,    0},
    {HT_TARGET_BOOST_LEVEL,   UNIT_PSI,        1},
    {HT_ECU_TEMPERATURE,      UNIT_CELSIUS,    1},
    {HT_BATTERY_VOLTAGE,      UNIT_VOLTS,      2},
    {HT_INTAKE_CAM_ANGLE_1,   UNIT_DEGREES,    1},
    {HT_VEHICLE_SPEED,        UNIT_MPH,        1},
    {HT_TOTAL_FUEL_USED,      UNIT_GALLONS,    4},
    {HT_KNOCK_LEVEL_1,        UNIT_DB,         2}
};

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

  loadLayout(tft, buttonWidth, buttonHeight);
}

void screenLoop() {
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 20; // Adjust as needed
  
  // Non-blocking debounce
  if (millis() - lastDebounceTime < debounceDelay) {
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
        htButtons[buttonIndex].pressedTime = millis();
      }
      
      // Redraw button with appropriate state
      htButtons[buttonIndex].drawButton(htButtons[buttonIndex].isPressed());
    }

    // Optional: Long press handling (you can expand this as needed)
    if (htButtons[buttonIndex].isPressed() && 
        (millis() - htButtons[buttonIndex].pressedTime > longPressThresholdTime)) {
      // Long press detected
    }
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
                defaultButtonConfigs[i].decimalPlaces
            };
        
            // Set up default layout
            htButtons[i].initButton(&tft, 
                i % 4 * buttonWidth, 
                i / 4 * buttonHeight, 
                buttonWidth, 
                buttonHeight, 
                TFT_GREEN, 
                TFT_BLACK, 
                TFT_WHITE, 
                1, 
                &dashValues[defaultButtonConfigs[i].displayType], 
                defaultButtonConfigs[i].unit,
                defaultButtonConfigs[i].decimalPlaces);
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
            currentButtonConfigs[i].decimalPlaces);
        htButtons[i].drawButton();
    }

    return true;
}
