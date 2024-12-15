#include "haltech_can.h"
#include "driver/gpio.h"
#include "driver/twai.h"
#include "screen.h"
#include "haltech_dash_values_init.h"
#include "webpage.h"

const char* unitDisplayStrings[] = {
    "RPM",       // UNIT_RPM
    "kPa (abs)", // UNIT_KPA_ABS
    "kPa",       // UNIT_KPA
    "%",         // UNIT_PERCENT
    "Deg",       // UNIT_DEGREES
    "km/h",      // UNIT_KPH
    "m/s",       // UNIT_MS
    "Lambda",    // UNIT_LAMBDA
    "Raw",       // UNIT_RAW
    "dB",        // UNIT_DB
    "m/s^2",     // UNIT_MPS2
    "Boolean",   // UNIT_BOOLEAN
    "CCPM",      // UNIT_CCPM
    "V",         // UNIT_VOLTS
    "K",         // UNIT_K
    "PPM",       // UNIT_PPM
    "g/m^3",     // UNIT_GPM3
    "L",         // UNIT_LITERS
    "s",         // UNIT_SECONDS
    "Enum",      // UNIT_ENUM
    "mm",        // UNIT_MM
    "Bit Field", // UNIT_BIT_FIELD
    "cc",        // UNIT_CC
    "m",         // UNIT_METERS
    "PSI",       // UNIT_PSI
    "PSI (abs)", // UNIT_PSI_ABS
    "Deg/s",     // UNIT_DEG_S
    "AFR",       // UNIT_AFR
    "C",         // UNIT_CELSIUS
    "F",         // UNIT_FAHRENHEIT
    "MPH",       // UNIT_MPH
    "Gal",       // UNIT_GALLONS
    "MPG",       // UNIT_MPG
    "Foot",      // UNIT_FEET
    "Inch",      // UNIT_INCHES
    "Mile"       // UNIT_MILES
    "",          // UNIT_NONE
};

unsigned long KAinterval = 150;             // 50ms interval for keep aliv frame
unsigned long ButtonInfoInterval = 30;      // 30ms interval for button info frame
unsigned long KAintervalMillis = 0;         // storage for millis counter
unsigned long ButtonInfoIntervalMillis = 0; // storage for millis counter

float HaltechDashValue::convertToUnit(HaltechUnit_e toUnit)
{
  // If units are the same, no conversion is needed
  if (this->incomingUnit == toUnit)
  {
    return this->scaled_value;
  }

  switch (this->incomingUnit)
  {
  case UNIT_KPA:
    if (toUnit == UNIT_PSI)
    {
      return this->scaled_value * 0.145038; // Convert kPa to PSI
    }
    else if (toUnit == UNIT_KPA_ABS)
    {
      return this->scaled_value + 101.325; // Assuming atmospheric pressure at sea level
    }
    else if (toUnit == UNIT_PSI_ABS)
    {
      return (this->scaled_value + 101.325) * 0.145038; // kPa gauge to PSI absolute
    }
    break;

  case UNIT_KPA_ABS:
    if (toUnit == UNIT_PSI_ABS)
    {
      return this->scaled_value * 0.145038; // Convert kPa absolute to PSI absolute
    }
    else if (toUnit == UNIT_KPA)
    {
      return this->scaled_value - 101.325; // Subtract atmospheric pressure to get kPa gauge
    }
    else if (toUnit == UNIT_PSI)
    {
      return (this->scaled_value - 101.325) * 0.145038; // kPa absolute to PSI gauge
    }
    break;

  case UNIT_K:
    if (toUnit == UNIT_CELSIUS)
    {
      return this->scaled_value - 273.15; // Kelvin to Celsius
    }
    else if (toUnit == UNIT_FAHRENHEIT)
    {
      return (this->scaled_value - 273.15) * 9.0 / 5.0 + 32.0; // Kelvin to Fahrenheit
    }
    break;

  case UNIT_CC:
    if (toUnit == UNIT_GALLONS) {
      return this->scaled_value / 3785.41; // Convert cubic centimeters to US gallons (1 gallon = 3785.41 cc)
    }
    break;

  case UNIT_KPH:
    if (toUnit == UNIT_MPH) {
      return this->scaled_value * 0.621371; // Convert KPH to MPH
    }
    break;

  case UNIT_RPM:
  case UNIT_PERCENT:
  case UNIT_DEGREES:
  case UNIT_MS:
    if (toUnit == UNIT_SECONDS) {
        return this->scaled_value / 1000.0; // Convert milliseconds to seconds
    }
    break;
  case UNIT_LAMBDA:
    if (toUnit == UNIT_AFR) {
        return this->scaled_value * 14.7; // Convert lambda to AFR (assuming stoichiometric value for gasoline is 14.7)
    }
    break;
  case UNIT_RAW:
    break;
  case UNIT_DB:
    break;
  case UNIT_MPS2:
    break;
  case UNIT_BOOLEAN:
    break;
  case UNIT_CCPM:
    break;
  case UNIT_VOLTS:
    break;
  case UNIT_PPM:
    break;
  case UNIT_GPM3:
    break;
  case UNIT_LITERS:
    break;
  case UNIT_SECONDS:
    break;
  case UNIT_ENUM:
    break;
  case UNIT_MM:
    if (toUnit == UNIT_INCHES) {
        return this->scaled_value / 25.4; // Convert millimeters to inches
    }
    break;
  case UNIT_BIT_FIELD:
    break;
  case UNIT_METERS:
    if (toUnit == UNIT_MILES) {
        return this->scaled_value * 0.000621371; // Convert meters to miles
    } else if (toUnit == UNIT_FEET) {
        return this->scaled_value * 3.28084; // Convert meters to feet
    }
    break;
  }

  Serial.printf("Add %s to %s for %s\n", unitDisplayStrings[this->incomingUnit], unitDisplayStrings[toUnit], this->short_name);
  return this->scaled_value;
}

HaltechCan::HaltechCan() : lastProcessTime(0)
{
}

bool HaltechCan::begin(long baudRate)
{
  delay(1000);

  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_13, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_1MBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  // Install TWAI driver
  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
      printf("Driver installed\n");
  } else {
      printf("Failed to install driver\n");
      return false;
  }

  // Start TWAI driver
  if (twai_start() == ESP_OK) {
      printf("Driver started\n");
  } else {
      printf("Failed to start driver\n");
      return false;
  }

  return true;
}

uint32_t HaltechCan::extractValue(const uint8_t *buffer, uint8_t start_byte, uint8_t end_byte)
{
  uint32_t result = 0;
  for (int i = start_byte; i <= end_byte; i++)
  {
    result = (result << 8) | buffer[i];
  }
  return result;
}

void HaltechCan::process()
{
  unsigned long preemptLimit = 50; // break out of loop if this long has passed since we exited last
  static unsigned long lastPreemptTime = 0;
  while (lastPreemptTime + preemptLimit > millis()) { // escape with breaks or when it's gone for too long
    twai_message_t message;
    esp_err_t result = twai_receive(&message, pdMS_TO_TICKS(10)); // Short timeout to check for messages
    //printf("TWAI: 0x%04x\n", result);

    // Break the loop if no message is available
    if (result != ESP_OK) {
        if (result == ESP_ERR_TIMEOUT) {
            break; // No more messages
        }
        // More detailed error handling if needed
        if (result == ESP_ERR_INVALID_STATE) {
            // Handle driver not started or other state issues
            printf("TWAI driver in invalid state\n");
        } else {
            printf("Error receiving message: %s\n", esp_err_to_name(result));
        }
        break;
    }
    processCANData(message.identifier, 
                    message.data_length_code, 
                    message.data);
  }
  lastPreemptTime = millis();

  // Keep alive and button info timing remains the same
  if (millis() - KAintervalMillis >= KAinterval)
  {
    KAintervalMillis = millis();
    SendKeepAlive();
  }

  if (millis() - ButtonInfoIntervalMillis >= ButtonInfoInterval)
  {
    ButtonInfoIntervalMillis = millis();
    SendButtonInfo();
  }
}

void HaltechCan::processCANData(long unsigned int rxId, unsigned char len, unsigned char *rxBuf)
{
  // CAN Input from Haltech canbus
  byte txBuf[8] = {0};

  for (int i = 0; i < HT_NONE; i++) // loop through the enum till the last one, none
  {
    auto &dashVal = dashValues[(HaltechDisplayType_e)i];
    if (dashVal.can_id == rxId) // if the incoming value belongs to this dash value
    {
      uint32_t rawVal = extractValue(rxBuf, dashVal.start_byte, dashVal.end_byte);
      dashVal.scaled_value = (float)rawVal * dashVal.scale_factor + dashVal.offset;
      for (int buttonIndex = 0; buttonIndex < nButtons; buttonIndex++)
      {
        if (dashVal.type == htButtons[buttonIndex].dashValue->type)
        {
          htButtons[buttonIndex].drawValue();
          updateWebpageValue(buttonIndex, htButtons[buttonIndex].dashValue->scaled_value, 2);
          if (htButtons[buttonIndex].dashValue->last_update_time + htButtons[buttonIndex].dashValue->update_period * 2 < millis()) {
            printf("Late update: %s over: %lu\n", htButtons[buttonIndex].dashValue->short_name, millis() - htButtons[buttonIndex].dashValue->last_update_time + htButtons[buttonIndex].dashValue->update_period);
          }
          htButtons[buttonIndex].dashValue->last_update_time = millis();
        }
      }
    }
  }

  // Keypad
  if (rxId == 0x60C)
  {
    if ((rxBuf[0]) == 0x22)
    {
      txBuf[0] = 0x60;
      txBuf[1] = (rxBuf[1]);
      txBuf[2] = (rxBuf[2]);
      txBuf[3] = (rxBuf[3]);
    }
    else if ((rxBuf[0]) == 0x42)
    {
      txBuf[0] = 0x43;
      txBuf[1] = (rxBuf[1]);
      txBuf[2] = (rxBuf[2]);
      txBuf[3] = (rxBuf[3]);

      if ((txBuf[1] == 0x18) && (txBuf[2] == 0x10))
      {
        if (txBuf[3] == 0x01)
        {
          txBuf[4] = 0x07;
          txBuf[5] = 0x03;
        }
        else if (txBuf[3] == 0x02)
        {
          txBuf[4] = 0x48;
          txBuf[5] = 0x33;
        }
        else if (txBuf[3] == 0x03)
        {
          txBuf[4] = 0x01;
        }
        else if (txBuf[3] == 0x04)
        {
          txBuf[4] = 0xCF;
          txBuf[5] = 0xB8;
          txBuf[6] = 0x19;
          txBuf[7] = 0x0C;
        }
      }
      else if ((txBuf[1] == 0x00) && (txBuf[2] == 0x18) && (txBuf[3] == 0x01))
      {
        txBuf[4] = 0x8C;
        txBuf[5] = 0x01;
        txBuf[7] = 0x40;
      }
    }
    else if (((rxBuf[0]) == 0x00) && ((rxBuf[7]) == 0xC8))
    {
      txBuf[0] = 0x80;
      txBuf[4] = 0x01;
      txBuf[6] = 0x04;
      txBuf[7] = 0x05;
    }
    sendMsgBuf(0x58C, 0x00, 0x08, txBuf);
  }
}

void HaltechCan::SendButtonInfo()
{
  byte ButtonInfo[3]; // declare an array for 3 bytes used for htButtons pressed information

  for (int j = 0; j < 2; j++)
  {
    static uint8_t prevButtonInfo[2] = {0}; // Local static array to store previous values
    for (int i = 0; i < nButtons / 2; i++)
    {
      bool buttonStatus = false;
      if (htButtons[i + j * 8].isToggleable) {
        buttonStatus = htButtons[i + j * 8].toggledState;
      } else {
        buttonStatus = htButtons[i + j * 8].isPressed();
      }
      bitWrite(ButtonInfo[j], i, buttonStatus);
    }
  }
  // Serial.printf("0x%04x\n", ButtonInfo[0] << 8 | ButtonInfo[1]);

  ButtonInfo[2] = 0;                        // byte 3 filled with 0
  sendMsgBuf(0x18C, 0, 3, ButtonInfo); // send the 3 byte data buffer at address 18D
}

void HaltechCan::SendKeepAlive()
{                                          // send keep alive frame
  byte KeepAlive[1] = {5};                 // frame dat is 0x05 for byte 0
  sendMsgBuf(0x70C, 0, 1, KeepAlive); // send the frame at 70D
}

bool HaltechCan::sendMsgBuf(long unsigned int id, unsigned char ext, unsigned char len, byte *buf)
{
    twai_message_t message;
    message.identifier = id;
    message.extd = ext ? 1 : 0;
    message.data_length_code = len;
    
    // Copy the buffer contents
    memcpy(message.data, buf, len);
    
    // Send the message
    esp_err_t result = twai_transmit(&message, pdMS_TO_TICKS(100));
    
    return (result == ESP_OK);
}
