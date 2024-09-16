#ifndef _HALTECH_BUTTON_H
#define _HALTECH_BUTTON_H

#include "haltech_can.h"
#include <string>

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
    virtual bool isButton();
  private:
    
};

class HaltechValueDisplay : public HaltechScreenEntity {
  public:
    HaltechValueDisplay(HaltechDisplayType_e type);
    float getValue();
    std::string getValueString(uint8_t decimalPlaces = 1);
    bool isValueNew();
    void clearValueNew();
    bool isButton() override;
    void updateValue(float value);
  private:
    bool valueNew;
    float value;
    HaltechDisplayType_e type;
};

class HaltechButton : public HaltechScreenEntity {
  public:
    HaltechButton();
    bool isButtonStatusOn(buttonStatusColor_e color);
    bool isButton() override;
  private:
    buttonStatus_e buttonStatus[3];
    std::string displayString = "button";
};

#endif // _HALTECH_BUTTON_H