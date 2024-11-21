#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>
//#include <CAN.h>
#include <map>

#define DEBUG(x, ...) Serial.printf(x, ##__VA_ARGS__)

typedef enum {
    HT_RPM = 0,
    HT_MANIFOLD_PRESSURE,
    HT_THROTTLE_POSITION,
    HT_COOLANT_PRESSURE,
    HT_FUEL_PRESSURE,
    HT_OIL_PRESSURE,
    HT_ENGINE_DEMAND,
    HT_WASTEGATE_PRESSURE,
    HT_INJECTION_STATE_1_DUTY_CYCLE,
    HT_INJECTION_STATE_2_DUTY_CYCLE,
    HT_IGNITION_ANGLE,
    HT_WHEEL_SLIP,
    HT_WHEEL_DIFF,
    HT_LAUNCH_CONTROL_END_RPM,
    HT_WIDEBAND_SENSOR_1,
    HT_KNOCK_LEVEL_1,
    HT_WHEEL_SPEED_REAR_RIGHT,
    HT_BOOST_CONTROL_OUTPUT,
    HT_VEHICLE_SPEED,
    HT_INTAKE_CAM_ANGLE_1,
    HT_BATTERY_VOLTAGE,
    HT_TARGET_BOOST_LEVEL,
    HT_COOLANT_TEMPERATURE,
    HT_AIR_TEMPERATURE,
    HT_OIL_TEMPERATURE,
    HT_FUEL_TRIM_SHORT_TERM_BANK_1,
    HT_FUEL_TRIM_LONG_TERM_BANK_1,
    HT_IGNITION_ANGLE_BANK_1,
    HT_WIDEBAND_OVERALL,
    HT_GEAR,
    HT_WATER_INJECTION_ADVANCED_SOLENOID_DUTY_CYCLE,
    HT_NONE
} HaltechDisplayType_e;

typedef enum {
    UNIT_RPM,
    UNIT_KPA_ABS,
    UNIT_KPA,
    UNIT_PERCENT,
    UNIT_DEGREES,
    UNIT_KPH,
    UNIT_MS,
    UNIT_LAMBDA,
    UNIT_RAW,
    UNIT_DB,
    UNIT_MPS2,
    UNIT_BOOLEAN,
    UNIT_CCPM,
    UNIT_VOLTS,
    UNIT_K,
    UNIT_PPM,
    UNIT_GPM3,
    UNIT_LITERS,
    UNIT_SECONDS,
    UNIT_ENUM,
    UNIT_MM,
    UNIT_BIT_FIELD,
    UNIT_CC,
    UNIT_METERS,
    UNIT_NONE,
} HaltechUnit_e;

struct CANValue {
    uint32_t can_id;
    uint8_t start_byte;
    uint8_t end_byte;
    HaltechUnit_e incomingUnit;
    uint16_t frequency;  // New field for message frequency in Hz
    float scale_factor;
    float offset;
    unsigned long update_interval;
    unsigned long last_update_time;
    float scaled_value;
    bool justUpdated;
};

class HaltechCan {
public:
    HaltechCan();
    bool begin(long baudRate = 1000E3);
    void process();
    void addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, HaltechDisplayType_e name,  HaltechUnit_e incomingUnit, uint16_t frequency = 50, float scale_factor = 1.0f, float offset = 1.0f);
    std::map<HaltechDisplayType_e, CANValue> values;

private:
    unsigned long lastProcessTime;

    uint32_t extractValue(const uint8_t* buffer, uint8_t start_byte, uint8_t end_byte);
};

#endif // HALTECH_CAN_H