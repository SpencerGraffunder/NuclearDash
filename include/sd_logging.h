#ifndef SD_LOGGING_H
#define SD_LOGGING_H

#include <Arduino.h>
#include <SD.h>

class SDLogging {
public:
    SDLogging();
    // initialize SD subsystem; won't erase existing log until first write
    bool begin();
    // log an entry: timestamp in micros, id, value (float)
    void log(uint32_t can_id, float value);
    // log the raw CAN frame: id, dlc, payload bytes
    void logFrame(uint32_t can_id, uint8_t len, const uint8_t *payload);

private:
    bool initialized;
    bool has_written;
    File logFile;
    void openNewLogFile();
    void ensureInit();
};

extern SDLogging sdLogger;

#endif // SD_LOGGING_H
