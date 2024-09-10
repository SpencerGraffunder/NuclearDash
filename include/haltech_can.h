#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>
#include <CAN.h>
#include <vector>

#define DEBUG(x, ...) Serial.printf(x, ##__VA_ARGS__)

struct CANValue {
    uint32_t can_id;
    uint8_t start_byte;
    uint8_t end_byte;
    const char* name;
    HaltechUnit_e incomingUnit;
    uint16_t frequency;  // New field for message frequency in Hz
    float scale_factor;
    float offset;
    unsigned long update_interval;
    unsigned long last_update_time;
};

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

class HaltechCan {
public:
    HaltechCan();
    bool begin(long baudRate = 1000E3);
    void process();
    void addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, const char* name,  HaltechUnit_e incomingUnit, uint16_t frequency = 50, float scale_factor = 1.0f, float offset = 1.0f);

private:
    unsigned long lastProcessTime;
    std::vector<CANValue> values;

    uint32_t extractValue(const uint8_t* buffer, uint8_t start_byte, uint8_t end_byte);
};

HaltechCan::HaltechCan() : lastProcessTime(0) {
    htc.addValue(0x360, 0, 1, "RPM", UNIT_PERCENT);
    htc.addValue(0x360, 2, 3, "Manifold Pressure", UNIT_KPA_ABS, 50, 0.1);
    htc.addValue(0x360, 4, 5, "Throttle Position", UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x360, 6, 7, "Coolant Pressure", UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 0, 1, "Fuel Pressure", UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 2, 3, "Oil Pressure", UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x361, 4, 5, "Engine Demand", UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x361, 2, 3, "Wastegate Pressure", UNIT_KPA, 50, 0.1, -101.3);
    htc.addValue(0x362, 0, 1, "Injection State 1 Duty Cycle", UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x362, 2, 3, "Injection State 2 Duty Cycle", UNIT_PERCENT, 50, 0.1);
    htc.addValue(0x362, 4, 5, "Ignition Angle", UNIT_DEGREES, 50, 0.1);
    htc.addValue(0x363, 0, 1, "Wheel Slip", UNIT_KPH, 20, 0.1);
    htc.addValue(0x363, 2, 3, "Wheel Diff", UNIT_KPH, 20, 0.1);
    htc.addValue(0x363, 2, 3, "Launch Control End RPM", UNIT_RPM, 20);
    htc.addValue(0x368, 0, 1, "Wideband Sensor 1", UNIT_LAMBDA, 20, 0.001);
    htc.addValue(0x36A, 0, 1, "Knock Level 1", UNIT_DB, 20, 0.01);
    htc.addValue(0x36C, 6, 7, "Wheel Speed Rear Right", UNIT_KPH, 20, 0.01);
    htc.addValue(0x36F, 2, 3, "Boost Control Output", UNIT_PERCENT, 20, 0.1);
    htc.addValue(0x370, 0, 1, "Vehicle Speed", UNIT_KPH, 20, 0.1);
    htc.addValue(0x370, 2, 3, "Intake Cam Angle 1", UNIT_DEGREES, 20, 0.1);
    htc.addValue(0x372, 0, 1, "Battery Voltage", UNIT_VOLTS, 10, 0.1);
    htc.addValue(0x372, 4, 5, "Target Boost Level", UNIT_KPA, 10, 0.1);
    htc.addValue(0x3E0, 0, 1, "Coolant Temperature", UNIT_K, 5, 0.1);
    htc.addValue(0x3E0, 2, 3, "Air Temperature", UNIT_K, 5, 0.1);
    htc.addValue(0x3E0, 6, 7, "Oil Temperature", UNIT_K, 5, 0.1);
    htc.addValue(0x3E3, 0, 1, "Fuel Trim Short Term Bank 1", UNIT_PERCENT, 5, 0.1);
    htc.addValue(0x3E3, 4, 5, "Fuel Trim Long Term Bank 1", UNIT_PERCENT, 5, 0.1);
    htc.addValue(0x3EB, 4, 5, "Ignition Angle Bank 1", UNIT_DEGREES, 50, 0.1);
    htc.addValue(0x470, 0, 1, "Wideband Overall", UNIT_LAMBDA, 20, 0.001);
    htc.addValue(0x470, 7, 7, "Gear", UNIT_ENUM, 20);
    htc.addValue(0x6F7, 6, 7, "Water Injection Advanced Solenoid Duty Cycle", UNIT_PERCENT, 10, 0.1);
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

void HaltechCan::addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, const char* name,  HaltechUnit_e incomingUnit, uint16_t frequency, float scale_factor, float offset) {
    unsigned long update_interval = (frequency > 0) ? (1000 / frequency) : 0;
    values.push_back({can_id, start_byte, end_byte, name, incomingUnit, frequency, scale_factor, offset, update_interval, 0});
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