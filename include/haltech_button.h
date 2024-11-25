/***************************************************************************************
// The following button class has been ported over from the Adafruit_GFX library so
// should be compatible.
// A slightly different implementation in this TFT_eSPI library allows the button
// legends to be in any font, allow longer labels and to adjust text positioning
// within button
***************************************************************************************/

#include "haltech_can.h"
#include <string>
#include "TFT_eSPI.h"

typedef enum {
  BUTTON_STATUS_OFF = 0,
  BUTTON_STATUS_ON,
} buttonStatus_e;

typedef enum {
  BUTTON_STATUS_COLOR_GREEN = 0,
  BUTTON_STATUS_COLOR_AMBER,
  BUTTON_STATUS_COLOR_RED,
} buttonStatusColor_e;

class HaltechScreenEntity {
  public:
    HaltechScreenEntity(HaltechDisplayType_e type);
    float getValue();
    std::string getValueString(uint8_t decimalPlaces = 1);
    bool isButton();
    void updateValue(float value);
    bool isButtonStatusOn(buttonStatusColor_e color);
    HaltechDisplayType_e type;
  private:
    float value;
    buttonStatus_e buttonStatus[3];
    std::string displayString = "button";
};

class HaltechButton
{
 public:
  HaltechButton(void);
  // "Classic" initButton() uses centre & size
  void     initButton(TFT_eSPI *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechScreenEntity htEntity);

  // New/alt initButton() uses upper-left corner & size
  void     initButtonUL(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechScreenEntity htEntity);
  
  // Adjust text datum and x, y deltas
  void     setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
  void     drawButton(bool inverted = false, String long_name = "");
  bool     contains(int16_t x, int16_t y);
  void     press(bool p);
  bool     isPressed();
  bool     justPressed();
  bool     justReleased();
  void     drawValue(float val);
  HaltechScreenEntity htEntity;
  long     pressedTime;
  static const long longPressTime = 1000;

 private:
  TFT_eSPI *_gfx;
  int16_t  _x1, _y1; // Coordinates of top-left corner of button
  int16_t  _xd, _yd; // Button text datum offsets (wrt centre of button)
  uint16_t _w, _h;   // Width and height of button
  uint8_t _textsize, _textdatum; // Text size multiplier and text datum for button
  uint16_t _outlinecolor, _fillcolor, _textcolor;
  bool currstate;
  bool laststate;
  float lastVal;
  char _label[12];
};
