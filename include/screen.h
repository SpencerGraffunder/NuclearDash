/*
  The TFT_eSPI library incorporates an Adafruit_GFX compatible
  button handling class, this sketch is based on the Arduin-o-phone
  example.

  This example diplays a keypad where numbers can be entered and
  send to the Serial Monitor window.

  The sketch has been tested on the ESP8266 (which supports SPIFFS)

  The minimum screen size is 320 x 240 as that is the keypad size.

  TOUCH_CS and SPI_TOUCH_FREQUENCY must be defined in the User_Setup.h file
  for the touch functions to do anything.
*/
#ifndef _SCREEN_H
#define _SCREEN_H

// The SPIFFS (FLASH filing system) is used to hold touch screen
// calibration data

#include "FS.h"

#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include "haltech_button.h"
#include "menu_button.h"

extern TFT_eSPI tft; // Invoke custom library

// This is the file name used to store the calibration data
// You can change this to create new calibration files.
// The SPIFFS file name must start with "/".
#define CALIBRATION_FILE "/TouchCalData2"

// Increment REPEAT_CAL to calibrate once more, no need to set back to false.
// Repeat calibration if you change the screen rotation.
//#define REPEAT_CAL false
#define REPEAT_CAL 2

// Keypad start position, key sizes and spacing
#define KEY_X 40 // Centre of key
#define KEY_Y 96
#define KEY_W 62 // Width and height
#define KEY_H 30
#define KEY_SPACING_X 18 // X and Y gap
#define KEY_SPACING_Y 20
#define KEY_TEXTSIZE 1   // Font size multiplier

// Using two fonts since numbers are nice when bold
#define LABEL1_FONT &FreeMonoBold12pt7b // Key label font 1
#define LABEL2_FONT &FreeMono9pt7b    // Key label font 2

// Numeric display box size and location
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN

// Number length, buffer for storing it and character index
#define NUM_LEN 12
extern char numberBuffer[NUM_LEN + 1];
extern uint8_t numberIndex;

// We have a status line for messages
#define STATUS_X 120 // Centred on this
#define STATUS_Y 65

// Create 15 keys for the keypad
extern char keyLabel[15][5];
extern uint16_t keyColor[15];

// Invoke the TFT_eSPI button class and create all the button objects
extern HaltechButton htButtons[16];

extern QueueHandle_t screenQueue;

extern const uint8_t nButtons;
extern HaltechDisplayType_e buttonDisplayTypes[];

typedef enum {
  STATE_NORMAL,
  STATE_MENU,
  STATE_VAL_SEL,
  STATE_BUTTON_TEXT_SEL,
} ScreenState_e;

typedef enum {
  MENU_BACK,
  MENU_VAL_SEL,
  MENU_ALERT_MIN_DOWN,
  MENU_ALERT_MIN_UP,
  MENU_ALERT_MAX_DOWN,
  MENU_ALERT_MAX_UP,
  MENU_ALERT_BEEP_OFF,
  MENU_ALERT_BEEP_ON,
  MENU_ALERT_FLASH_OFF,
  MENU_ALERT_FLASH_ON,
  MENU_DECIMALS_DOWN,
  MENU_DECIMALS_UP,
  MENU_UNITS_BACK,
  MENU_UNITS_FORWARD,
  MENU_BUTTON_MODE_NONE,
  MENU_BUTTON_MODE_MOMENTARY,
  MENU_BUTTON_MODE_TOGGLE,
  MENU_BUTTON_TEXT_SEL,
  MENU_NONE,
} menuButtonName_e;

typedef enum {
  VAL_SEL_1,
  VAL_SEL_2,
  VAL_SEL_3,
  VAL_SEL_4,
  VAL_SEL_5,
  VAL_SEL_6,
  VAL_SEL_7,
  VAL_SEL_8,
  VAL_SEL_9,
  VAL_SEL_10,
  VAL_SEL_11,
  VAL_SEL_12,
  VAL_SEL_13,
  VAL_SEL_14,
  VAL_SEL_15,
  VAL_SEL_16,
  VAL_SEL_17,
  VAL_SEL_18,
  VAL_SEL_BACK,
  VAL_SEL_PAGE_BACK,
  VAL_SEL_PAGE_FORWARD,
  VAL_SEL_NONE,
} valSelButtonName_e;

extern MenuButton menuButtons[MENU_NONE];
extern MenuButton valSelButtons[VAL_SEL_NONE];

extern ScreenState_e currScreenState;
extern uint8_t buttonToModifyIndex;

void screenSetup();
void screenLoop();
void touch_calibrate();
void drawMenu();
bool saveLayout();
bool loadLayout(TFT_eSPI &tft, int buttonWidth, int buttonHeight);

void setupSelectValueScreen();
void drawSelectValueScreen();
void handleValSelValueSelection(int valueIndex);
void navigateValSelToNextPage();
void navigateValSelToPreviousPage();

#endif // _SCREEN_H