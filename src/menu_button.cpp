#include "menu_button.h"
#include <sstream>
#include "screen.h"
#include <iomanip>

MenuButton::MenuButton(void) {
  _gfx       = nullptr;
  _xd        = 0;
  _yd        = 0;
  _textdatum = MC_DATUM;
  _label[9]  = '\0';
  currstate = false;
  laststate = false;
}

// Classic initButton() function: pass center & size
void MenuButton::initButton(
 TFT_eSPI *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  // Tweak arguments and pass to the newer initButtonUL() function...
  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill,
    textcolor, label, textsize);
}

// Newer function instead accepts upper-left corner & size
void MenuButton::initButtonUL(
 TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h,
 uint16_t outline, uint16_t fill, uint16_t textcolor,
 char *label, uint8_t textsize)
{
  _x1           = x1;
  _y1           = y1;
  _w            = w;
  _h            = h;
  _outlinecolor = outline;
  _fillcolor    = fill;
  _textcolor    = textcolor;
  _textsize     = textsize;
  _gfx          = gfx;
  strncpy(_label, label, 9);
}

// Adjust text datum and x, y deltas
void MenuButton::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
  _xd        = x_delta;
  _yd        = y_delta;
  _textdatum = datum;
}

void MenuButton::drawButton(bool inverted, String long_name, bool selected) {
  uint16_t fill, outline, text;

  if (inverted) {
    fill = _fillcolor;
    outline = _outlinecolor;
    text = _textcolor;
  } else if (selected) {
    fill = _selectedcolor;
    outline = _outlinecolor;
    text = _textcolor;
  } else {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
  }

  uint8_t r = min(_w, _h) / 4; // Corner radius
  _gfx->fillRoundRect(_x1, _y1, _w, _h, r, fill);
  _gfx->drawRoundRect(_x1, _y1, _w, _h, r, outline);

  if (_gfx->textfont == 255) {
    _gfx->setCursor(_x1 + (_w / 8),
                    _y1 + (_h / 4));
    _gfx->setTextColor(text);
    _gfx->setTextSize(_textsize);
    _gfx->print(_label);
  }
  else {
    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);

    uint8_t tempdatum = _gfx->getTextDatum();
    _gfx->setTextDatum(_textdatum);
    uint16_t tempPadding = _gfx->getTextPadding();
    _gfx->setTextPadding(0);

    if (long_name == "")
      _gfx->drawString(_label, _x1 + (_w/2) + _xd, _y1 + (_h/2) - 4 + _yd);
    else
      _gfx->drawString(long_name, _x1 + (_w/2) + _xd, _y1 + (_h/2) - 4 + _yd);

    _gfx->setTextDatum(tempdatum);
    _gfx->setTextPadding(tempPadding);
  }
}

bool MenuButton::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void MenuButton::press(bool p) {
  laststate = currstate;
  currstate = p;
}

bool MenuButton::isPressed()    { return currstate; }
bool MenuButton::justPressed()  { return (currstate && !laststate); }
bool MenuButton::justReleased() { return (!currstate && laststate); }
