/***************************************************************************************
** Code for the GFX button UI element
** Grabbed from Adafruit_GFX library and enhanced to handle any label font
***************************************************************************************/

#include "main.h"
#include "haltech_button.h"
#include <sstream>
#include <iomanip>

HaltechButton::HaltechButton()
    : _gfx(nullptr),
      _xd(0),
      _yd(0),
      _textdatum(MC_DATUM),
      pressedState(false),
      previousPressedState(false) 
{

}

void HaltechButton::initButton(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechDashValue* dashValue)
{
  _x1             = x1;
  _y1             = y1;
  _w              = w;
  _h              = h;
  _outlinecolor   = outline;
  _fillcolor      = fill;
  _textcolor      = textcolor;
  _textsize       = textsize;
  _gfx            = gfx;
  this->dashValue = dashValue;
}

// Adjust text datum and x, y deltas
void HaltechButton::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
  _xd        = x_delta;
  _yd        = y_delta;
  _textdatum = datum;
}

void HaltechButton::drawValue(float val) {
  char buffer[10];
  snprintf(buffer, sizeof(buffer), "%.2f", val);

  // Set text datum to middle center for perfect centering
  _gfx->setTextDatum(MC_DATUM);
  
  _gfx->drawString(buffer, _x1 + (_w/2) + _xd, _y1 + (_h/2) - 4 + _yd);
}

void HaltechButton::drawButton(bool inverted) {
  uint16_t fill, outline, text;

  if(!inverted) {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  } else {
    fill    = _textcolor;
    outline = _outlinecolor;
    text    = _fillcolor;
  }

  uint8_t r = min(_w, _h) / 16; // Corner radius
  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

  if (_gfx->textfont == 255) {
    _gfx->setCursor(_x1 + (_w / 8),
                    _y1 + (_h / 4));
    _gfx->setTextColor(text);
    _gfx->setTextSize(_textsize);
    _gfx->print(this->dashValue->short_name);
  }
  else {
    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);

    uint8_t tempdatum = _gfx->getTextDatum();
    _gfx->setTextDatum(_textdatum);
    uint16_t tempPadding = _gfx->getTextPadding();
    _gfx->setTextPadding(0);

    _gfx->drawString(this->dashValue->short_name, _x1 + (_w/2) + _xd, _y1 + (_h/4) - 4 + _yd);

    _gfx->setTextDatum(tempdatum);
    _gfx->setTextPadding(tempPadding);
  }
}

bool HaltechButton::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void HaltechButton::press(bool p) {
  previousPressedState = pressedState;
  pressedState = p;
}

bool HaltechButton::isPressed()    {
  return pressedState;
}
bool HaltechButton::justPressed()  { return (pressedState && !previousPressedState); }
bool HaltechButton::justReleased() { return (!pressedState && previousPressedState); }
