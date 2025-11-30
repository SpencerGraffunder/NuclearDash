#include "sd_logging.h"
#include <SD.h>
#include <SPI.h>
#include "config.h"

SDLogging sdLogger;

SDLogging::SDLogging()
    : initialized(false), has_written(false)
{
}

bool SDLogging::begin()
{
    // Try to initialize SD using default pins. For ESP32, SD.begin() will
    // attempt the default HSPI pins; if the user's board uses SD_MMC, that
    // can be changed later. We don't create/erase any files here to preserve
    // existing logs until first write.
    if (initialized) return true;

    // Initialize SPI with the pins defined in config.h for SPI SD card
    // Use a static SPIClass instance so it persists after this function returns.
    const int SCK_PIN = 18; // typical VSPI SCK for ESP32
    Serial.printf("SD: begin() CS=%d MISO=%d MOSI=%d SCK=%d\n", PIN_SD_CS, PIN_SD_MISO, PIN_SD_MOSI, SCK_PIN);
    static SPIClass spi = SPIClass(VSPI);
    spi.begin(SCK_PIN, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);

    // Use SD.begin with explicit CS pin and SPI instance
    if (!SD.begin(PIN_SD_CS, spi)) {
        Serial.println("SD: begin failed (SPI)");
        initialized = false;
        return false;
    }
    Serial.println("SD: begin succeeded");

    initialized = true;
    has_written = false; // track whether we've written since boot
    return true;
}

void SDLogging::openNewLogFile()
{
    // Choose a new filename by scanning existing log_N.csv files and
    // using the next integer (max+1). This avoids relying on millis()
    // which may collide across reboots.
    const char *prefix = "/log_";
    const char *suffix = ".csv";
    int maxIndex = -1;

    // Open root directory and iterate files
    File root = SD.open("/");
    if (root) {
        File entry = root.openNextFile();
        while (entry) {
            if (!entry.isDirectory()) {
                const char *name = entry.name();
                // look for names that start with "log_" and end with ".csv"
                size_t len = strlen(name);
                size_t prelen = strlen(prefix);
                size_t suflen = strlen(suffix);
                if (len > prelen + suflen && strncmp(name, prefix, prelen) == 0 &&
                    strcmp(name + len - suflen, suffix) == 0) {
                    // extract middle part and parse integer
                    int num = -1;
                    char numbuf[16] = {0};
                    size_t numlen = len - prelen - suflen;
                    if (numlen < sizeof(numbuf)) {
                        memcpy(numbuf, name + prelen, numlen);
                        numbuf[numlen] = '\0';
                        num = atoi(numbuf);
                        if (num > maxIndex) maxIndex = num;
                    }
                }
            }
            entry.close();
            entry = root.openNextFile();
        }
        root.close();
    } else {
        Serial.println("SD: failed to open root directory when choosing filename");
    }

    int nextIndex = maxIndex + 1;
    if (nextIndex < 0) nextIndex = 0; // safety

    char fname[32];
    snprintf(fname, sizeof(fname), "/log_%d.csv", nextIndex);
    Serial.printf("SD: attempting to open %s (nextIndex=%d)\n", fname, nextIndex);
    logFile = SD.open(fname, FILE_WRITE);
    if (!logFile) {
        Serial.printf("SD: failed to open %s\n", fname);
    } else {
        Serial.printf("SD: logging to %s\n", fname);
        // write CSV header
        logFile.println("micros,can_id,value");
        logFile.flush();
    }
}

void SDLogging::ensureInit()
{
    if (!initialized) {
    // best-effort to init; caller may ignore return
    Serial.println("SD: ensureInit() - not initialized, calling begin()");
    begin();
    }
}

void SDLogging::log(uint32_t can_id, float value)
{
    ensureInit();
    if (!initialized) {
        Serial.println("SD: log() skipped - not initialized");
        return;
    }

    // On first write this boot, create a new file. If device booted but never
    // wrote, we leave the old file untouched.
    if (!has_written) {
        // Close any previously opened file pointer left over
        if (logFile) {
            logFile.close();
        }
        openNewLogFile();
        has_written = true;
    }

    if (!logFile) {
        Serial.println("SD: no file to write to");
        return;
    }

    Serial.printf("SD: log() id=0x%03lX value=%.3f\n", (unsigned long)can_id, value);

    unsigned long us = micros();
    // Format CSV: micros,0xID,value
    char line[64];
    int n = snprintf(line, sizeof(line), "%lu,0x%03lX,%.3f", us, (unsigned long)can_id, value);
    if (n > 0) {
        logFile.println(line);
        logFile.flush();
    }
}


void SDLogging::logFrame(uint32_t can_id, uint8_t len, const uint8_t *payload)
{
    Serial.printf("SD: logFrame() entry id=0x%03lX len=%u payload=%p\n", (unsigned long)can_id, (unsigned int)len, payload);
    ensureInit();
    if (!initialized) {
        Serial.println("SD: logFrame() skipped - SD not initialized");
        return;
    }

    if (!has_written) {
        if (logFile) logFile.close();
        openNewLogFile();
        has_written = true;
    }

    if (!logFile) {
        Serial.println("SD: logFrame() no open file");
        return;
    }

    unsigned long us = micros();
    // Build payload hex
    char payloadStr[64] = {0};
    char tmp[4];
    int pos = 0;
    for (uint8_t i = 0; i < len && pos + 3 < (int)sizeof(payloadStr); i++) {
        snprintf(tmp, sizeof(tmp), "%02X", payload[i]);
        payloadStr[pos++] = tmp[0];
        payloadStr[pos++] = tmp[1];
        if (i < len - 1) payloadStr[pos++] = ' ';
    }
    payloadStr[pos] = '\0';

    // CSV: micros,0xID,len,"payload"
    char line[128];
    int n = snprintf(line, sizeof(line), "%lu,0x%03lX,%u,\"%s\"", us, (unsigned long)can_id, (unsigned int)len, payloadStr);
    if (n > 0) {
        logFile.println(line);
        logFile.flush();
        // Serial.printf("SD: wrote frame line len=%d\n", n);
    } else {
        Serial.println("SD: failed to format frame line");
    }
}
