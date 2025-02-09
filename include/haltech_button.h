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

struct UnitOption {
    HaltechDisplayType_e type;
    HaltechUnit_e units[6]; // Maximum 6 units per value
    uint8_t count;
};

typedef enum {
  DIRECTION_NEXT,
  DIRECTION_PREVIOUS,
} menuSelectionDirection_e;

static const UnitOption unitOptions[] = {
    // Pressure measurements
    {HT_MANIFOLD_PRESSURE, {UNIT_KPA_ABS, UNIT_PSI_ABS}, 2},
    {HT_COOLANT_PRESSURE, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_FUEL_PRESSURE, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_OIL_PRESSURE, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_WASTEGATE_PRESSURE, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_BRAKE_PRESSURE_FRONT, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_BRAKE_PRESSURE_REAR, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_NOS_PRESSURE_1, {UNIT_KPA, UNIT_PSI}, 2},
    {HT_BARO_PRESSURE, {UNIT_KPA_ABS, UNIT_PSI_ABS}, 2},
    {HT_EXHAUST_MANIFOLD_PRESS, {UNIT_KPA, UNIT_PSI}, 2},
    
    // Speed measurements
    {HT_VEHICLE_SPEED, {UNIT_KPH, UNIT_MPH}, 2},
    {HT_CC_TARGET_SPEED, {UNIT_KPH, UNIT_MPH}, 2},
    
    // Temperature measurements
    {HT_COOLANT_TEMPERATURE, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_AIR_TEMPERATURE, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_FUEL_TEMPERATURE, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_OIL_TEMPERATURE, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_GEARBOX_OIL_TEMP, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_DIFF_OIL_TEMPERATURE, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_TIRE_TEMPERATURE_FL, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_TIRE_TEMPERATURE_FR, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_TIRE_TEMPERATURE_RL, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    {HT_TIRE_TEMPERATURE_RR, {UNIT_CELSIUS, UNIT_FAHRENHEIT}, 2},
    
    // Distance measurements
    {HT_SHOCK_TRAVEL_FL, {UNIT_MM, UNIT_INCHES}, 2},
    {HT_SHOCK_TRAVEL_FR, {UNIT_MM, UNIT_INCHES}, 2},
    {HT_SHOCK_TRAVEL_RL, {UNIT_MM, UNIT_INCHES}, 2},
    {HT_SHOCK_TRAVEL_RR, {UNIT_MM, UNIT_INCHES}, 2},
    
    // Air/Fuel measurements
    {HT_WIDEBAND_SENSOR_1, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_SENSOR_2, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_SENSOR_3, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_SENSOR_4, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_OVERALL, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_BANK_1, {UNIT_LAMBDA, UNIT_AFR}, 2},
    {HT_WIDEBAND_BANK_2, {UNIT_LAMBDA, UNIT_AFR}, 2},
    
    // Volume measurements
    {HT_FUEL_LEVEL, {UNIT_LITERS, UNIT_GALLONS}, 2},
    {HT_TOTAL_FUEL_USED, {UNIT_LITERS, UNIT_GALLONS}, 2},
    {HT_TOTAL_FUEL_USED_T1, {UNIT_LITERS, UNIT_GALLONS}, 2},
};

class HaltechButton
{
public:
  HaltechButton(void);
  void initButton(TFT_eSPI *gfx, int16_t x1, int16_t y1, uint16_t w, uint16_t h, uint16_t outline, uint16_t fill, uint16_t textcolor, uint8_t textsize, HaltechDashValue* dashValue, HaltechUnit_e unit, uint8_t decimalPlaces, buttonMode_e mode);
  void setLabelDatum(int16_t x_delta, int16_t y_delta, uint8_t datum = MC_DATUM);
  void drawButton(bool inverted = false);
  bool contains(int16_t x, int16_t y);
  void press(bool p);
  bool isPressed();
  bool justPressed();
  bool justReleased();
  void drawValue();
  void drawGraph();
  void drawBar();
  long pressedTime;
  buttonMode_e mode;
  bool toggledState = false;
  bool pressedState = false;
  HaltechDashValue* dashValue = nullptr;
  HaltechUnit_e displayUnit;
  uint8_t decimalPlaces = 1;
  float alertMin = -1;
  float alertMax = 1;
  bool alertState = false;
  bool alertBeep = false;
  // beep is global, flash is per-button so we need to track those states here
  bool alertFlash = false;
  bool flashState = false;
  uint64_t lastFlashTime = 0;
  void changeUnits(menuSelectionDirection_e direction);

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
