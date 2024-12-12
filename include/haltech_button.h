#ifndef __HALTECH_BUTTON_H
#define __HALTECH_BUTTON_H

#include "haltech_can.h"
#include <string>
#include "TFT_eSPI.h"

typedef enum
{
  BUTTON_STATUS_COLOR_GREEN = 0,
  BUTTON_STATUS_COLOR_AMBER,
  BUTTON_STATUS_COLOR_RED,
} buttonStatusColor_e;

const unsigned long longPressThresholdTime = 1000;

class HaltechButton
{
public:
  HaltechButton(void);
  void initButton(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechDashValue* dashValue, HaltechUnit_e unit, uint8_t decimalPlaces);
  void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
  void drawButton(bool inverted = false);
  bool contains(int16_t x, int16_t y);
  void press(bool p);
  bool isPressed();
  bool justPressed();
  bool justReleased();
  void drawValue();
  long pressedTime;
  bool isToggleable = true;
  bool toggledState = false;
  bool pressedState = false;
  HaltechDashValue* dashValue = nullptr;
  HaltechUnit_e displayUnit;
  uint8_t decimalPlaces = 1;

private:
  TFT_eSPI *_gfx;
  int16_t _x1, _y1;              // Coordinates of top-left corner of button
  int16_t _xd, _yd;              // Button text datum offsets (wrt centre of button)
  uint16_t _w, _h;               // Width and height of button
  uint8_t _textsize, _textdatum; // Text size multiplier and text datum for button
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  //bool pressedState;
  bool previousPressedState;
  bool ledStates[3]; // Store states of the 3 "LEDs" like the real keypad (green, amber, red)
};

#endif // __HALTECH_BUTTON_H
