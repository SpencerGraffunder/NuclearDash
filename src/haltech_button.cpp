/***************************************************************************************
** Code for the GFX button UI element
** Grabbed from Adafruit_GFX library and enhanced to handle any label font
***************************************************************************************/

#include "haltech_button.h"
#include <sstream>
#include "screen.h"
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

void HaltechButton::initButton(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechDashValue* dashValue, HaltechUnit_e unit, uint8_t decimalPlaces, buttonMode_e mode, float alertMin, float alertMax, bool alertBeepEnabled, bool alertFlashEnabled)
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
  this->displayUnit = unit;
  this->decimalPlaces = decimalPlaces;
  this->mode = mode;
  this->alertMin = alertMin;
  this->alertMax = alertMax;
  this->alertBeepEnabled = alertBeepEnabled;
  this->alertFlashEnabled = alertFlashEnabled;
}

// Adjust text datum and x, y deltas
void HaltechButton::setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum)
{
  _xd        = x_delta;
  _yd        = y_delta;
  _textdatum = datum;
}

void HaltechButton::drawValue() {
  if (currScreenState != STATE_NORMAL) {
    return;
  }

  char buffer[10];

  uint16_t fill, text;

  bool drawInverted = false;
  if (toggledState || pressedState) {
    drawInverted = true;
  }
  if (mode == BUTTON_MODE_NONE) {
    drawInverted = false;
  }

  if (isInverted) {
    drawInverted = !drawInverted;
  }

  if(drawInverted) {
    fill    = TFT_GREEN;
    text    = _fillcolor;
  } else {
    fill    = _fillcolor;
    text    = _textcolor;
  }

  tft.setFreeFont(LABEL1_FONT);
  _gfx->setTextColor(text, fill);

  float convertedValue = this->dashValue->convertToUnit(this->displayUnit);

  snprintf(buffer, sizeof(buffer), "%.*f", decimalPlaces, convertedValue);

  // update the overall alert state
  alertConditionMet = (convertedValue > alertMax || convertedValue < alertMin);

  // Set text datum to middle center for perfect centering
  _gfx->setTextDatum(MC_DATUM);
  
  _gfx->drawString(buffer, _x1 + (_w/2) + _xd, _y1 + (_h/2) - 4 + _yd);

  tft.setFreeFont(LABEL2_FONT);
}

void HaltechButton::drawGraph() {

}

void HaltechButton::drawBar() {
  
}

void HaltechButton::drawButton(bool flashState) {
  Serial.printf("drawing button flash %d\n", flashState);
  isInverted = flashState;
  uint16_t fill, outline, text;

  tft.setFreeFont(LABEL2_FONT);

  bool drawInverted = false;
  if ((mode == BUTTON_MODE_TOGGLE) && toggledState) {
    drawInverted = true;
  }
  if ((mode != BUTTON_MODE_TOGGLE) && pressedState) {
    drawInverted = true;
  }
  if (mode == BUTTON_MODE_NONE) {
    drawInverted = false;
  }

  if (flashState) {
    drawInverted = !drawInverted;
  }

  if(drawInverted) {
    fill    = TFT_GREEN;
    outline = _outlinecolor;
    text    = _fillcolor;
  } else {
    fill    = _fillcolor;
    outline = _outlinecolor;
    text    = _textcolor;
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
  } else {
    _gfx->setTextColor(text, fill);
    _gfx->setTextSize(_textsize);

    uint8_t tempdatum = _gfx->getTextDatum();
    _gfx->setTextDatum(_textdatum);
    //uint16_t tempPadding = _gfx->getTextPadding();
    _gfx->setTextPadding(0);

    // Draw name of value on top
    _gfx->drawString(this->dashValue->short_name, _x1 + (_w/2) + _xd, _y1 + (_h/4) - 4 + _yd);
    // Draw units on bottom
    _gfx->drawString(unitDisplayStrings[this->displayUnit], _x1 + (_w/2) + _xd, _y1 + (_h*3/4) - 4 + _yd);

    _gfx->setTextDatum(tempdatum);
    //_gfx->setTextPadding(tempPadding);
    _gfx->setTextPadding(this->_w - 2);

    // Draw value, even if it's 0. Needed to draw over when it's pressed or unpressed.
    drawValue();
  }
}

bool HaltechButton::contains(int16_t x, int16_t y) {
  return ((x >= _x1) && (x < (_x1 + _w)) &&
          (y >= _y1) && (y < (_y1 + _h)));
}

void HaltechButton::press(bool p) {
  if ((mode == BUTTON_MODE_TOGGLE) && pressedState && previousPressedState == false) {
    toggledState = !toggledState;
    // Serial.printf("ts=%u\n", toggledState);
  }
  previousPressedState = pressedState;
  pressedState = p;
}

bool HaltechButton::isPressed() {
  return pressedState;
}
bool HaltechButton::justPressed()  { return (pressedState && !previousPressedState); }
bool HaltechButton::justReleased() { return (!pressedState && previousPressedState); }

void HaltechButton::changeUnits(menuSelectionDirection_e direction) {
  uint8_t nextUnitIndex = -1;
  for (const UnitOption& option : unitOptions) {
    if (option.type == this->dashValue->type) {
      Serial.printf("found matching type %d\n", option.type);
      bool validUnitFound = false;

      for (uint8_t i = 0; i < option.count; i++) {
        if (option.units[i] == this->displayUnit) {
          validUnitFound = true;
          Serial.printf("found matching unit index %d\n", i);
          if (direction == DIRECTION_NEXT) {
            nextUnitIndex = (i + 1) % option.count; // loop around
          } else if (direction == DIRECTION_PREVIOUS) {
            nextUnitIndex = (i - 1 + option.count) % option.count; // add option count to make sure it doesn't go negative
          }
          Serial.printf("changing to unit %d\n", nextUnitIndex);
          this->displayUnit = option.units[nextUnitIndex];
          drawMenu();
          return;
        }
      }

      // Fallback: if no valid unit is found, set to the first unit in the list
      if (!validUnitFound) {
        Serial.printf("current unit is invalid, defaulting to first unit\n");
        this->displayUnit = option.units[0];
        drawMenu();
        return;
      }
    }
  }

  // Fallback: if no UnitOption exists for the current dashValue, use its default unit
  Serial.printf("no UnitOption found for type %d, searching dashValues for default unit\n", this->dashValue->type);

  // Search through dashValues for a matching type
  for (uint8_t i = 0; i < HT_NONE; i++) {
    if (dashValues[i].type == this->dashValue->type) {
      Serial.printf("found matching dashValue type %d, defaulting to unit %d\n", dashValues[i].type, dashValues[i].incomingUnit);
      this->displayUnit = dashValues[i].incomingUnit;
      drawMenu();
      return;
    }
  }
}