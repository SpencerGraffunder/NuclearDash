#include "haltech_can.h"
#include "main.h"
#include <SPI.h>
#include <mcp_can.h>
#include "screen.h"

SPIClass *customSPI = new SPIClass(HSPI);

MCP_CAN CAN0(customSPI, CS_PIN);

unsigned long KAinterval = 150;             // 50ms interval for keep aliv frame
unsigned long ButtonInfoInterval = 30;      // 30ms interval for button info frame
unsigned long KAintervalMillis = 0;         // storage for millis counter
unsigned long ButtonInfoIntervalMillis = 0; // storage for millis counter

const char *ht_names[] = {
    "RPM",
    "Manifold Pressure",
    "Throttle Position",
    "Coolant Pressure",
    "Fuel Pressure",
    "Oil Pressure",
    "Engine Demand",
    "Wastegate Pressure",
    "Injection State 1 Duty Cycle",
    "Injection State 2 Duty Cycle",
    "Ignition Angle",
    "Wheel Slip",
    "Wheel Diff",
    "Launch Control End RPM",
    "Wideband Sensor 1",
    "Knock Level 1",
    "Wheel Speed Rear Right",
    "Boost Control Output",
    "Vehicle Speed",
    "Intake Cam Angle 1",
    "Battery Voltage",
    "Target Boost Level",
    "Coolant Temperature",
    "Air Temperature",
    "Oil Temperature",
    "Fuel Trim Short Term Bank 1",
    "Fuel Trim Long Term Bank 1",
    "Ignition Angle Bank 1",
    "Wideband Overall",
    "Gear",
    "Water Injection Advanced Solenoid Duty Cycle",
    "None"};

const char *ht_names_short[] = {
    "RPM",
    "ManifPres",
    "ThrotPos",
    "CoolPres",
    "FuelPres",
    "OilPres",
    "EngDemand",
    "WastePres",
    "InjState1",
    "InjState2",
    "IgnAngle",
    "WheelSlip",
    "WheelDiff",
    "LaunchRPM",
    "WidebandS1",
    "KnockLvl1",
    "WheelSpRR",
    "BoostCtrl",
    "VehSpeed",
    "IntCamAng1",
    "BattVolt",
    "TargBoost",
    "CoolTemp",
    "AirTemp",
    "OilTemp",
    "FuelTrimST",
    "FuelTrimLT",
    "IgnAngleB1",
    "WBOverall",
    "Gear",
    "WaterInjDC",
    "None"};

HaltechCan::HaltechCan() : lastProcessTime(0)
{
  this->addValue(0x360, 0, 1, HT_RPM, UNIT_PERCENT);
  this->addValue(0x360, 2, 3, HT_MANIFOLD_PRESSURE, UNIT_KPA_ABS, 50, 0.1);
  this->addValue(0x360, 4, 5, HT_THROTTLE_POSITION, UNIT_PERCENT, 50, 0.1);
  this->addValue(0x360, 6, 7, HT_COOLANT_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
  this->addValue(0x361, 0, 1, HT_FUEL_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
  this->addValue(0x361, 2, 3, HT_OIL_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
  this->addValue(0x361, 4, 5, HT_ENGINE_DEMAND, UNIT_PERCENT, 50, 0.1);
  this->addValue(0x361, 2, 3, HT_WASTEGATE_PRESSURE, UNIT_KPA, 50, 0.1, -101.3);
  this->addValue(0x362, 0, 1, HT_INJECTION_STATE_1_DUTY_CYCLE, UNIT_PERCENT, 50, 0.1);
  this->addValue(0x362, 2, 3, HT_INJECTION_STATE_2_DUTY_CYCLE, UNIT_PERCENT, 50, 0.1);
  this->addValue(0x362, 4, 5, HT_IGNITION_ANGLE, UNIT_DEGREES, 50, 0.1);
  this->addValue(0x363, 0, 1, HT_WHEEL_SLIP, UNIT_KPH, 20, 0.1);
  this->addValue(0x363, 2, 3, HT_WHEEL_DIFF, UNIT_KPH, 20, 0.1);
  this->addValue(0x363, 2, 3, HT_LAUNCH_CONTROL_END_RPM, UNIT_RPM, 20);
  this->addValue(0x368, 0, 1, HT_WIDEBAND_SENSOR_1, UNIT_LAMBDA, 20, 0.001);
  this->addValue(0x36A, 0, 1, HT_KNOCK_LEVEL_1, UNIT_DB, 20, 0.01);
  this->addValue(0x36C, 6, 7, HT_WHEEL_SPEED_REAR_RIGHT, UNIT_KPH, 20, 0.01);
  this->addValue(0x36F, 2, 3, HT_BOOST_CONTROL_OUTPUT, UNIT_PERCENT, 20, 0.1);
  this->addValue(0x370, 0, 1, HT_VEHICLE_SPEED, UNIT_KPH, 20, 0.1);
  this->addValue(0x370, 2, 3, HT_INTAKE_CAM_ANGLE_1, UNIT_DEGREES, 20, 0.1);
  this->addValue(0x372, 0, 1, HT_BATTERY_VOLTAGE, UNIT_VOLTS, 10, 0.1);
  this->addValue(0x372, 4, 5, HT_TARGET_BOOST_LEVEL, UNIT_KPA, 10, 0.1);
  this->addValue(0x3E0, 0, 1, HT_COOLANT_TEMPERATURE, UNIT_K, 5, 0.1);
  this->addValue(0x3E0, 2, 3, HT_AIR_TEMPERATURE, UNIT_K, 5, 0.1);
  this->addValue(0x3E0, 6, 7, HT_OIL_TEMPERATURE, UNIT_K, 5, 0.1);
  this->addValue(0x3E3, 0, 1, HT_FUEL_TRIM_SHORT_TERM_BANK_1, UNIT_PERCENT, 5, 0.1);
  this->addValue(0x3E3, 4, 5, HT_FUEL_TRIM_LONG_TERM_BANK_1, UNIT_PERCENT, 5, 0.1);
  this->addValue(0x3EB, 4, 5, HT_IGNITION_ANGLE_BANK_1, UNIT_DEGREES, 50, 0.1);
  this->addValue(0x470, 0, 1, HT_WIDEBAND_OVERALL, UNIT_LAMBDA, 20, 0.001);
  this->addValue(0x470, 7, 7, HT_GEAR, UNIT_ENUM, 20);
  this->addValue(0x6F7, 6, 7, HT_WATER_INJECTION_ADVANCED_SOLENOID_DUTY_CYCLE, UNIT_PERCENT, 10, 0.1);
}

bool HaltechCan::begin(long baudRate)
{
  customSPI->setFrequency(100000);
  customSPI->begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  delay(1000);

  // initialize canbus with 1000kbit and 16mhz xtal
  if (CAN0.begin(MCP_ANY, CAN_1000KBPS, MCP_8MHZ) == CAN_OK)
    Serial.println("MCP2515 Initialized Successfully!");
  else
    Serial.println("Error Initializing MCP2515...");

  // Set operation mode to normal so the MCP2515 sends acks to received data.
  CAN0.setMode(MCP_NORMAL);

  pinMode(CAN0_INT, INPUT);     // set INT pin to be an input
  digitalWrite(CAN0_INT, HIGH); // set INT pin high to enable interna pullup

  DEBUG("Haltech CAN initialized\n");
  return true;
}

void HaltechCan::addValue(uint32_t can_id, uint8_t start_byte, uint8_t end_byte, HaltechDisplayType_e type, HaltechUnit_e incomingUnit, uint16_t frequency, float scale_factor, float offset)
{
  unsigned long update_interval = (frequency > 0) ? (1000 / frequency) : 0;
  dashValues[type] = {can_id, start_byte, end_byte, incomingUnit, scale_factor, offset, update_interval, 0, 0.0f, type};
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
  unsigned long currentMillis = millis(); // Get current time in milliseconds

  // Execute keepalive frame every 50 ms
  if (currentMillis - KAintervalMillis >= KAinterval)
  {
    KAintervalMillis = currentMillis;
    SendKeepAlive();
    //Serial.println("ka");
  }

  // Execute buttoninfo frame every 30 ms
  if (currentMillis - ButtonInfoIntervalMillis >= ButtonInfoInterval)
  {
    ButtonInfoIntervalMillis = currentMillis;
    SendButtonInfo();
    //Serial.println("bi");
  }

  // read can buffer when interrupted and jump to canread for processing.
  if (!digitalRead(CAN0_INT)) // If CAN0_INT pin is low, read receive buffer
  {
    long unsigned int rxId;              // storage for can data
    unsigned char len = 0;               // storage for can data
    unsigned char rxBuf[8];              // storage for can data
    CAN0.readMsgBuf(&rxId, &len, rxBuf); // Read data: len = data length, buf = data byte(s)
    canRead(rxId, len, rxBuf);           // execute canRead function to negotiate with ecu
  }
}

void HaltechCan::canRead(long unsigned int rxId, unsigned char len, unsigned char* rxBuf)
{
  // CAN Input from Haltech canbus
  byte txBuf[8] = {0};

  // Serial.printf("ID: 0x%02X, Len: 0x%02X Data: ", rxId, len);
  // for (int i = 0; i < 8; i++) {
  //   Serial.print(rxBuf[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();

  bool idFound = false;
  for (int i = 0; i < HT_NONE; i++) // loop through the enum till the last one, none
  {
    HaltechDashValue& dashVal = dashValues[(HaltechDisplayType_e)i]; // actual value to draw
    if (dashVal.can_id == rxId) // if the incoming value belongs to this dash value
    {
      idFound = true;
      uint32_t rawVal = extractValue(rxBuf, dashVal.start_byte, dashVal.end_byte);
      dashVal.scaled_value = (float)rawVal * dashVal.scale_factor + dashVal.offset;
      Serial.printf("%s: raw: %lu, scaled: %f\n", ht_names_short[(HaltechDisplayType_e)i], rawVal, dashVal.scaled_value);
      for (int buttonIndex = 0; buttonIndex < nButtons; buttonIndex++) {
        //Serial.printf("htb type: %u, dv type: %u\n", htButtons[buttonIndex].type, dashVal.type);
        if (htButtons[buttonIndex].type == dashVal.type) {
          htButtons[buttonIndex].drawValue(dashVal.scaled_value);
        }
      }
    }
  }
  if (idFound = false) {
    Serial.printf("ID: 0x%04x\n", rxId);
  }

  // Keypad Configuration Section
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
    CAN0.sendMsgBuf(0x58C, 0x00, 0x08, txBuf);
  }
}

void HaltechCan::SendButtonInfo()
{
  byte ButtonInfo[3]; // declare an array for 3 bytes used for htButtons pressed information

  for (int j = 0; j < 2; j++) {
  static uint8_t prevButtonInfo[2] = {0}; // Local static array to store previous values
  for (int i = 0; i < nButtons / 2; i++) {
    bitWrite(ButtonInfo[j], i, htButtons[i + j * 8].isPressed());
  }
  // Only print if the value has changed
  if (ButtonInfo[j] != prevButtonInfo[j]) {
    Serial.printf("0x%02x\n", ButtonInfo[j]);
    prevButtonInfo[j] = ButtonInfo[j]; // Update the stored value
  }
}

  ButtonInfo[2] = 0;                        // byte 3 filled with 0
  CAN0.sendMsgBuf(0x18C, 0, 3, ButtonInfo); // send the 3 byte data buffer at address 18D
}

void HaltechCan::SendKeepAlive()
{                                          // send keep alive frame
  byte KeepAlive[1] = {5};                 // frame dat is 0x05 for byte 0
  CAN0.sendMsgBuf(0x70C, 0, 1, KeepAlive); // send the frame at 70D
}
