#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>
#include <CAN.h>
#include <vector>
#include <main.h>

#define DEBUG(x, ...) Serial.printf(x, ##__VA_ARGS__)


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
    uint8_t name;
    HaltechUnit_e incomingUnit;
    uint16_t frequency;  // New field for message frequency in Hz
    float scale_factor;
    float offset;
    unsigned long update_interval;
    unsigned long last_update_time;
    float scaled_value;
};

class HaltechCan {
public:
    HaltechCan();
    bool begin(long baudRate = 1000E3);
    void process();
    void addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, uint8_t name,  HaltechUnit_e incomingUnit, uint16_t frequency = 50, float scale_factor = 1.0f, float offset = 1.0f);

private:
    unsigned long lastProcessTime;
    std::vector<CANValue> values;

    uint32_t extractValue(const uint8_t* buffer, uint8_t start_byte, uint8_t end_byte);
};

extern HaltechCan htc;

HaltechCan::HaltechCan() : lastProcessTime(0) {
    htc.addValue(0x360, 0, 1, HT_RPM, UNIT_PERCENT);
    htc.addValue(0x360, 2, 3, HT_MANIFOLD_PRESSURE, UNIT_KPA_ABS, 50, 0.1);
    htc.addValue(0x360, 4, 5, HT_THROTTLE_POSITION, UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x360, 6, 7, HT_COOLANT_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 0, 1, HT_FUEL_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 2, 3, HT_OIL_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 4, 5, HT_ENGINE_DEMAND, UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x361, 2, 3, HT_WASTEGATE_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x362, 0, 1, HT_INJECTION_STATE_1_DUTY_CYCLE, UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x362, 2, 3, HT_INJECTION_STATE_2_DUTY_CYCLE, UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x362, 4, 5, HT_IGNITION_ANGLE, UNIT_DEGREES, 50, 0.1);
    htc.addValue(0x363, 0, 1, HT_WHEEL_SLIP, UNIT_KPH, 20, 0.1);
    htc.addValue(0x363, 2, 3, HT_WHEEL_DIFF, UNIT_KPH, 20, 0.1);
    htc.addValue(0x363, 2, 3, HT_LAUNCH_CONTROL_END_RPM, UNIT_RPM, 20);
    htc.addValue(0x368, 0, 1, HT_WIDEBAND_SENSOR_1, UNIT_LAMBDA, 20, 0.001);
    htc.addValue(0x36A, 0, 1, HT_KNOCK_LEVEL_1, UNIT_DB, 20, 0.01);
    htc.addValue(0x36C, 6, 7, HT_WHEEL_SPEED_REAR_RIGHT, UNIT_KPH, 20, 0.01);
    htc.addValue(0x36F, 2, 3, HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT, 20, 0.1);
    htc.addValue(0x370, 0, 1, HT_VEHICLE_SPEED, UNIT_KPH, 20, 0.1);
    htc.addValue(0x370, 2, 3, HT_INTAKE_CAM_ANGLE_1, UNIT_DEGREES, 20, 0.1);
    htc.addValue(0x372, 0, 1, HT_BATTERY_VOLTAGE, UNIT_VOLTS, 10, 0.1);
    htc.addValue(0x372, 4, 5, HT_TARGET_BOOST_LEVEL, UNIT_KPA, 10, 0.1);
    htc.addValue(0x3E0, 0, 1, HT_COOLANT_TEMPERATURE, UNIT_K, 5, 0.1);
    htc.addValue(0x3E0, 2, 3, HT_AIR_TEMPERATURE, UNIT_K, 5, 0.1);
    htc.addValue(0x3E0, 6, 7, HT_OIL_TEMPERATURE, UNIT_K, 5, 0.1);
    htc.addValue(0x3E3, 0, 1, HT_FUEL_TRIM_SHORT_TERM_BANK_1, UNIT_PERCENT, 5, 0.1);
    htc.addValue(0x3E3, 4, 5, HT_FUEL_TRIM_LONG_TERM_BANK_1, UNIT_PERCENT, 5, 0.1);
    htc.addValue(0x3EB, 4, 5, HT_IGNITION_ANGLE_BANK_1, UNIT_DEGREES, 50, 0.1);
    htc.addValue(0x470, 0, 1, HT_WIDEBAND_OVERALL, UNIT_LAMBDA, 20, 0.001);
    htc.addValue(0x470, 7, 7, HT_GEAR, UNIT_ENUM, 20);
    htc.addValue(0x6F7, 6, 7, HT_WATER_INJECTION_ADVANCED_SOLENOID_DUTY_CYCLE, UNIT_PERCENT, 10, 0.1);
}

bool HaltechCan::begin(long baudRate) {
    
    if (!CAN.begin(baudRate)) {
        DEBUG("Starting CAN failed!\n");
        return false;
    }
    
    // Set CAN pins for ESP32
    CAN.setPins(13, 33); // RX, TX
    
    DEBUG("Haltech CAN initialized\n");
    return true;
}

void HaltechCan::addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, uint8_t name,  HaltechUnit_e incomingUnit, uint16_t frequency, float scale_factor, float offset) {
    unsigned long update_interval = (frequency > 0) ? (1000 / frequency) : 0;
    values.push_back({can_id, start_byte, end_byte, name, incomingUnit, frequency, scale_factor, offset, update_interval, 0, 0.0f});
}

uint32_t HaltechCan::extractValue(const uint8_t* buffer, uint8_t start_byte, uint8_t end_byte) {
    uint32_t result = 0;
    for (int i = start_byte; i <= end_byte; i++) {
        result = (result << 8) | buffer[i];
    }
    return result;
}

void HaltechCan::process() {
    unsigned long currentTime = millis();

    if (CAN.parsePacket()) {
        uint32_t packetId = CAN.packetId();
        
        for (auto& value : values) {
            if (packetId == value.can_id && (currentTime - value.last_update_time >= value.update_interval)) {
                uint8_t buffer[8];
                int bytesRead = CAN.readBytes(buffer, 8);
                
                if (bytesRead >= value.end_byte + 1) {
                    uint32_t raw_value = extractValue(buffer, value.start_byte, value.end_byte);
                    float scaled_value = raw_value * value.scale_factor + value.offset;
                    DEBUG("%s: %.2f\n", value.name, scaled_value);
                    value.last_update_time = currentTime;
                }
            }
        }
    }
}

#endif // HALTECH_CAN_H