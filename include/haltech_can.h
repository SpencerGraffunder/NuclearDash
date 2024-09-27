#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>
#include <CAN.h>
#include <map>
#include "haltech.h"

#define DEBUG(x, ...) Serial.printf(x, ##__VA_ARGS__)

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