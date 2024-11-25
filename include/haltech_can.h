/* 
This class needs to:
- When the screen updates a button state
    - Send button updates to the ECU
- When the ECU sends a button update
    - Update the button state
- When the ECU sends a value
    - Read, convert, update the value in "dashValues"
*/ 


#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>
#include <map>

#define CS_PIN 15
#define MOSI_PIN 14
#define MISO_PIN 35
#define SCK_PIN 12
#define CAN0_INT 5

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

// Each displayable field needs to have an instance of this struct
struct HaltechDashValue {
    uint32_t can_id;            // From Haltech's documentation
    uint8_t start_byte;
    uint8_t end_byte;
    HaltechUnit_e incomingUnit; // Unit that the raw data will be converted to using the scale factor and offset
    float scale_factor;         // Factor to scale the raw data
    float offset;               // Add? to raw data after scaling
    unsigned long update_interval; // Interval between expected updates from the ECU
    unsigned long last_update_time; // Millis when last updated
    float scaled_value; // Value after scaling has been applied
    bool justUpdated;   // Tracks whether the display needs to be updated
};

class HaltechCan {
public:
    HaltechCan();
    bool begin(long baudRate = 1000E3);
    void process();
    void addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, HaltechDisplayType_e name,  HaltechUnit_e incomingUnit, uint16_t frequency = 50, float scale_factor = 1.0f, float offset = 0.0f);
    std::map<HaltechDisplayType_e, HaltechDashValue> dashValues;

private:
    unsigned long lastProcessTime;

    uint32_t extractValue(const uint8_t* buffer, uint8_t start_byte, uint8_t end_byte);

    void canReadDemo();
    void SendButtonInfoDemo();
    void SendKeepAliveDemo();
};

#endif // HALTECH_CAN_H