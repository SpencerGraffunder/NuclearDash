/***************************************************************************************
** Code for the GFX button UI element
** Grabbed from Adafruit_GFX library and enhanced to handle any label font
***************************************************************************************/

#include "main.h"
#include "haltech_button.h"

HaltechScreenEntity::HaltechScreenEntity(HaltechDisplayType_e type) {
    this->type = type;
}

float HaltechScreenEntity::getValue() {
    return value;
}

std::string HaltechScreenEntity::getValueString(uint8_t decimalPlaces) {
    return std::string(value, decimalPlaces);
}

void HaltechScreenEntity::updateValue(float value) {
    this->value = value;
    Serial.println("updating val");
}

bool HaltechScreenEntity::isButtonStatusOn(buttonStatusColor_e color) {
    return buttonStatus[color] == BUTTON_STATUS_ON;
}

HaltechButton::HaltechButton(void) : htEntity(HaltechScreenEntity(HT_NONE)) {
  _gfx       = nullptr;
  _xd        = 0;
  _yd        = 0;
  _textdatum = MC_DATUM;
  _label[11]  = '\0';
  currstate = false;
  laststate = false;
}

// Classic initButton() function: pass center & size
void HaltechButton::initButton(TFT_eSPI *gfx, int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechScreenEntity htEntity)
{
  // Tweak arguments and pass to the newer initButtonUL() function...
  initButtonUL(gfx, x - (w / 2), y - (h / 2), w, h, outline, fill, textcolor, textsize, htEntity);
}

// Newer function instead accepts upper-left corner & size
void HaltechButton::initButtonUL(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechScreenEntity htEntity)
{
  _x1            = x1;
  _y1            = y1;
  _w             = w;
  _h             = h;
  _outlinecolor  = outline;
  _fillcolor     = fill;
  _textcolor     = textcolor;
  _textsize      = textsize;
  _gfx           = gfx;
  this->htEntity = htEntity;
  strncpy(_label, ht_names_short[htEntity.type], sizeof(_label) - 1);
  _label[sizeof(_label) - 1] = '\0';
}

// Adjust text datum and x, y deltas
void HaltechButton::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
  _xd        = x_delta;
  _yd        = y_delta;
  _textdatum = datum;
}

void HaltechButton::drawValue(float val) {
  String valStr = String(val, 2);
  _gfx->drawString(valStr, _x1 + (_w/2) + _xd, _y1 + (_h*2/3) - 4 + _yd);
  lastVal = val;
  // this->htEntity.clearValueNew();
}

void HaltechButton::drawButton(bool inverted, String long_name) {
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
    _gfx->print(_label);
  }
  else {
    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);

    uint8_t tempdatum = _gfx->getTextDatum();
    _gfx->setTextDatum(_textdatum);
    uint16_t tempPadding = _gfx->getTextPadding();
    _gfx->setTextPadding(0);

    if (long_name == "") {
      _gfx->drawString(_label, _x1 + (_w/2) + _xd, _y1 + (_h/4) - 4 + _yd);
    } else {
      _gfx->drawString(long_name, _x1 + (_w/2) + _xd, _y1 + (_h/2) - 4 + _yd);
    }

    _gfx->setTextDatum(tempdatum);
    _gfx->setTextPadding(tempPadding);
  }
}

bool HaltechButton::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void HaltechButton::press(bool p) {
  laststate = currstate;
  currstate = p;
}

bool HaltechButton::isPressed()    { return currstate; }
bool HaltechButton::justPressed()  { return (currstate && !laststate); }
bool HaltechButton::justReleased() { return (!currstate && laststate); }
