#include "haltech_can.h"
#include "main.h"
#include <SPI.h>
#include <mcp_can.h>
#include "screen.h"

HaltechDashValue dashValues[] = {
    // Long Name,                    Short Name,    Enum,                      CAN ID, Start B, End B, Incoming Unit,  Scale,  Offset,  Rate, Signed, Last, Scaled Value
    {"RPM",                          "RPM",         HT_RPM,                    0x360,  0,       1,     UNIT_RPM,       1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Manifold Pressure",            "MAP",         HT_MANIFOLD_PRESSURE,      0x360,  2,       3,     UNIT_KPA_ABS,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Throttle Position",            "TPS",         HT_THROTTLE_POSITION,      0x360,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Coolant Pressure",             "CoolPres",    HT_COOLANT_PRESSURE,       0x360,  6,       7,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Fuel Pressure",                "FuelPres",    HT_FUEL_PRESSURE,          0x361,  0,       1,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Oil Pressure",                 "OilPres",     HT_OIL_PRESSURE,           0x361,  2,       3,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Engine Demand",                "EngDemand",   HT_ENGINE_DEMAND,          0x361,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Wastegate Pressure",           "WastePres",   HT_WASTEGATE_PRESSURE,     0x361,  6,       7,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Injection Stage 1 Duty Cycle", "InjS1Duty",   HT_INJ_STATE_1_DUTY,       0x362,  0,       1,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Injection Stage 2 Duty Cycle", "InjS2Duty",   HT_INJ_STATE_2_DUTY,       0x362,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Ignition Angle (Leading)",     "IgnAngle",    HT_IGNITION_ANGLE,         0x362,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Wheel Slip",                   "WhlSlip",     HT_WHEEL_SLIP,             0x363,  0,       1,     UNIT_KPH,       0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Wheel Diff",                   "WhlDiff",     HT_WHEEL_DIFF,             0x363,  2,       3,     UNIT_KPH,       0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Launch Control End RPM",       "LCEndRPM",    HT_LAUNCH_CONTROL_END_RPM, 0x363,  6,       7,     UNIT_RPM,       1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Injection Stage 1 Avg Time",   "Inj1AvgTim",  HT_INJ1_AVG_TIME,          0x364,  0,       1,     UNIT_MS,        0.001f, 0.0f,    50,   false,  0,    0.0f},
    {"Injection Stage 2 Avg Time",   "Inj2AvgTim",  HT_INJ2_AVG_TIME,          0x364,  2,       3,     UNIT_MS,        0.001f, 0.0f,    50,   false,  0,    0.0f},
    {"Injection Stage 3 Avg Time",   "Inj3AvgTim",  HT_INJ3_AVG_TIME,          0x364,  4,       5,     UNIT_MS,        0.001f, 0.0f,    50,   false,  0,    0.0f},
    {"Injection Stage 4 Avg Time",   "Inj4AvgTim",  HT_INJ4_AVG_TIME,          0x364,  6,       7,     UNIT_MS,        0.001f, 0.0f,    50,   false,  0,    0.0f},
    {"Wideband Sensor 1",            "WB1",         HT_WIDEBAND_SENSOR_1,      0x368,  0,       1,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 2",            "WB2",         HT_WIDEBAND_SENSOR_2,      0x368,  2,       3,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 3",            "WB3",         HT_WIDEBAND_SENSOR_3,      0x368,  4,       5,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 4",            "WB4",         HT_WIDEBAND_SENSOR_4,      0x368,  6,       7,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Trigger System Error Count",   "TrigErrCnt",  HT_TRIGGER_ERROR_COUNT,    0x369,  0,       1,     UNIT_RAW,       1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Trigger Counter",              "TrigCnt",     HT_TRIGGER_COUNTER,        0x369,  2,       3,     UNIT_RAW,       1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Trigger Sync Level",           "TrigSyncLvl", HT_TRIGGER_SYNC_LEVEL,     0x369,  6,       7,     UNIT_RAW,       1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Knock Level 1",                "KnockLvl1",   HT_KNOCK_LEVEL_1,          0x36A,  0,       1,     UNIT_DB,        0.01f,  0.0f,    20,   false,  0,    0.0f},
    {"Knock Level 2",                "KnockLvl2",   HT_KNOCK_LEVEL_2,          0x36A,  2,       3,     UNIT_DB,        0.01f,  0.0f,    20,   false,  0,    0.0f},
    {"Brake Pressure Front",         "BrakePressF", HT_BRAKE_PRESSURE_FRONT,   0x36B,  0,       1,     UNIT_KPA,       1.0f,   -101.3f, 20,   false,  0,    0.0f},
    {"NOS Pressure Sensor 1",        "NOSPress1",   HT_NOS_PRESSURE_1,         0x36B,  2,       3,     UNIT_KPA,       0.22f,  -101.3f, 20,   false,  0,    0.0f},
    {"Turbo Speed Sensor 1",         "TurboSpd1",   HT_TURBO_SPEED_1,          0x36B,  4,       5,     UNIT_RPM,       10.0f,  0.0f,    20,   false,  0,    0.0f},
    {"Lateral G",                    "LatG",        HT_LATERAL_G,              0x36B,  6,       7,     UNIT_MPS2,      0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Wheel Speed Front Left",       "WhlSpdFL",    HT_WHEEL_SPEED_FL,         0x36C,  0,       1,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Wheel Speed Front Right",      "WhlSpdFR",    HT_WHEEL_SPEED_FR,         0x36C,  2,       3,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Wheel Speed Rear Left",        "WhlSpdRL",    HT_WHEEL_SPEED_RL,         0x36C,  4,       5,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Wheel Speed Rear Right",       "WhlSpdRR",    HT_WHEEL_SPEED_RR,         0x36C,  6,       7,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Exhaust Cam Angle 1",          "ExhCamAng1",  HT_EXHAUST_CAM_ANGLE_1,    0x36D,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Exhaust Cam Angle 2",          "ExhCamAng2",  HT_EXHAUST_CAM_ANGLE_2,    0x36D,  6,       7,     UNIT_DEGREES,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Engine Limiting Active",       "LimActive",   HT_ENGINE_LIM_ACTIVE,      0x36E,  0,       1,     UNIT_BOOLEAN,   1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Launch Control Ign Retard",    "LCIgnRetard", HT_LC_IGN_RETARD,          0x36E,  2,       3,     UNIT_DEGREES,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Launch Control Fuel Enrich",   "LCFuelEnr",   HT_LC_FUEL_ENRICH,         0x36E,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Longitudinal G",               "LongG",       HT_LONGITUDINAL_G,         0x36E,  6,       7,     UNIT_MPS2,      0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Generic Output 1 Duty Cycle",  "GenOut1Duty", HT_GENERIC_OUTPUT_1_DUTY,  0x36F,  0,       1,     UNIT_PERCENT,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Boost Control Output",         "BoostCtlOut", HT_BOOST_CONTROL_OUTPUT,   0x36F,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Vehicle Speed",                "VehSpeed",    HT_VEHICLE_SPEED,          0x370,  0,       1,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Intake Cam Angle 1",           "IntCamAng1",  HT_INTAKE_CAM_ANGLE_1,     0x370,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Intake Cam Angle 2",           "IntCamAng2",  HT_INTAKE_CAM_ANGLE_2,     0x370,  6,       7,     UNIT_DEGREES,   0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Fuel Flow",                    "FuelFlow",    HT_FUEL_FLOW,              0x371,  0,       1,     UNIT_CCPM,      1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Fuel Flow Return",             "FuelFlowRet", HT_FUEL_FLOW_RETURN,       0x371,  2,       3,     UNIT_CCPM,      1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Battery Voltage",              "BattVolt",    HT_BATTERY_VOLTAGE,        0x372,  0,       1,     UNIT_VOLTS,     0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Target Boost Level",           "BoostTarget", HT_TARGET_BOOST_LEVEL,     0x372,  4,       5,     UNIT_KPA,       0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Barometric Pressure",          "BaroPres",    HT_BARO_PRESSURE,          0x372,  6,       7,     UNIT_KPA_ABS,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"EGT Sensor 1",                 "EGT1",        HT_EGT_SENSOR_1,           0x373,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"EGT Sensor 2",                 "EGT2",        HT_EGT_SENSOR_2,           0x373,  2,       3,     UNIT_DEGREES,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Ambient Air Temperature",      "AmbientTemp", HT_AMBIENT_AIR_TEMP,       0x376,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Relative Humidity",            "RelHumidity", HT_RELATIVE_HUMIDITY,      0x376,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    10,   true,   0,    0.0f},
    {"Specific Humidity",            "SpeHumidity", HT_SPECIFIC_HUMIDITY,      0x376,  4,       5,     UNIT_PPM,       100.0f, 0.0f,    10,   false,  0,    0.0f},
    {"Absolute Humidity",            "AbsHumidity", HT_ABSOLUTE_HUMIDITY,      0x376,  6,       7,     UNIT_GPM3,      0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Coolant Temperature",          "CoolantTemp", HT_COOLANT_TEMPERATURE,    0x3E0,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Air Temperature",              "AirTemp",     HT_AIR_TEMPERATURE,        0x3E0,  2,       3,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Fuel Temperature",             "FuelTemp",    HT_FUEL_TEMPERATURE,       0x3E0,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Oil Temperature",              "OilTemp",     HT_OIL_TEMPERATURE,        0x3E0,  6,       7,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Gearbox Oil Temperature",      "GearOilTemp", HT_GEARBOX_OIL_TEMP,       0x3E1,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Diff Oil Temperature",         "DiffOilTemp", HT_DIFF_OIL_TEMPERATURE,   0x3E1,  2,       3,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Fuel Composition",             "FuelComp",    HT_FUEL_COMPOSITION,       0x3E1,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Fuel Level",                   "FuelLevel",   HT_FUEL_LEVEL,             0x3E2,  0,       1,     UNIT_LITERS,    0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Fuel Trim Short Term Bank 1",  "FuelTrimST1", HT_FUEL_TRIM_SHORT_TERM_1, 0x3E3,  0,       1,     UNIT_PERCENT,   0.1f,   0.0f,    5,    true,   0,    0.0f},
    {"Fuel Trim Short Term Bank 2",  "FuelTrimST2", HT_FUEL_TRIM_SHORT_TERM_2, 0x3E3,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    5,    true,   0,    0.0f},
    {"Fuel Trim Long Term Bank 1",   "FuelTrimLT1", HT_FUEL_TRIM_LONG_TERM_1,  0x3E3,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    5,    true,   0,    0.0f},
    {"Fuel Trim Long Term Bank 2",   "FuelTrimLT2", HT_FUEL_TRIM_LONG_TERM_2,  0x3E3,  6,       7,     UNIT_PERCENT,   0.1f,   0.0f,    5,    true,   0,    0.0f},
    {"TM Knob",                      "TrqMgmtKnob", HT_TORQUE_MGMT_KNOB,       0x3E9,  7,       7,     UNIT_RAW,       1.0f,   0.0f,    20,   true,   0,    0.0f},
    {"Gearbox Line Pressure",        "GearLinePrs", HT_GEARBOX_LINE_PRESSURE,  0x3EA,  0,       1,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Injection Stage 3 Duty Cycle", "InjS3Duty",   HT_INJ_STAGE_3_DUTY,       0x3EA,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Injection Stage 4 Duty Cycle", "InjS4Duty",   HT_INJ_STAGE_4_DUTY,       0x3EA,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Crank Case Pressure",          "CrankPres",   HT_CRANK_CASE_PRESSURE,    0x3EA,  6,       7,     UNIT_KPA,       0.1f,   -101.3f, 50,   false,  0,    0.0f},
    {"Race Timer",                   "RaceTimer",   HT_RACE_TIMER,             0x3EB,  0,       3,     UNIT_MS,        1.0f,   0.0f,    50,   false,  0,    0.0f},
    {"Ignition Angle Bank 1",        "IgnAngleB1",  HT_IGNITION_ANGLE_BANK_1,  0x3EB,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Ignition Angle Bank 2",        "IgnAngleB2",  HT_IGNITION_ANGLE_BANK_2,  0x3EB,  6,       7,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"TM Drive RPM Target",          "TMRPMTarget", HT_TM_RPM_TARGET,          0x3EC,  0,       1,     UNIT_RPM,       1.0f,   0.0f,    50,   true,   0,    0.0f},
    {"TM Drive RPM Target Error",    "TMRPMError",  HT_TM_RPM_ERROR,           0x3EC,  2,       3,     UNIT_RPM,       1.0f,   0.0f,    50,   true,   0,    0.0f},
    {"TM Drive RPM Tar Err Ign Cor", "TMRPMEIgCor", HT_TM_RPM_ERROR_IGN_CORR,  0x3EC,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"TM Drive RPM Timed Ign Cor",   "TMRPMTIgCor", HT_TM_RPM_TIMED_IGN_CORR,  0x3EC,  6,       7,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"TM Combined Ign Correction",   "TMCombIgCor", HT_TM_COMBINED_IGN_CORR,   0x3ED,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Wideband Sensor 5",            "Wideband5",   HT_WIDEBAND_SENSOR_5,      0x3EE,  0,       1,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 6",            "Wideband6",   HT_WIDEBAND_SENSOR_6,      0x3EE,  2,       3,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 7",            "Wideband7",   HT_WIDEBAND_SENSOR_7,      0x3EE,  4,       5,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 8",            "Wideband8",   HT_WIDEBAND_SENSOR_8,      0x3EE,  6,       7,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 9",            "Wideband9",   HT_WIDEBAND_SENSOR_9,      0x3EF,  0,       1,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 10",           "Wideband10",  HT_WIDEBAND_SENSOR_10,     0x3EF,  2,       3,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 11",           "Wideband11",  HT_WIDEBAND_SENSOR_11,     0x3EF,  4,       5,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Sensor 12",           "Wideband12",  HT_WIDEBAND_SENSOR_12,     0x3EF,  6,       7,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Shock Travel FL Uncalibrated", "STravFLUcal", HT_SHOCK_TRAVEL_FL_UNCAL,  0x3F0,  0,       1,     UNIT_MM,        0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Shock Travel FR Uncalibrated", "STravFRUcal", HT_SHOCK_TRAVEL_FR_UNCAL,  0x3F0,  2,       3,     UNIT_MM,        0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Shock Travel RL Uncalibrated", "STravRLUcal", HT_SHOCK_TRAVEL_RL_UNCAL,  0x3F0,  4,       5,     UNIT_MM,        0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Shock Travel RR Uncalibrated", "STravRRUcal", HT_SHOCK_TRAVEL_RR_UNCAL,  0x3F0,  6,       7,     UNIT_MM,        0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Shock Travel Front Left",      "ShockTravFL", HT_SHOCK_TRAVEL_FL,        0x3F1,  0,       1,     UNIT_MM,        0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Shock Travel Front Right",     "ShockTravFR", HT_SHOCK_TRAVEL_FR,        0x3F1,  2,       3,     UNIT_MM,        0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Shock Travel Rear Left",       "ShockTravRL", HT_SHOCK_TRAVEL_RL,        0x3F1,  4,       5,     UNIT_MM,        0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Shock Travel Rear Right",      "ShockTravRR", HT_SHOCK_TRAVEL_RR,        0x3F1,  6,       7,     UNIT_MM,        0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"ECU Temperature",              "ECUTemp",     HT_ECU_TEMPERATURE,        0x469,  0,       1,     UNIT_DEGREES,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Wideband Overall",             "WO2",         HT_WIDEBAND_OVERALL,       0x470,  0,       1,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Bank 1",              "WidebandB1",  HT_WIDEBAND_BANK_1,        0x470,  2,       3,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Wideband Bank 2",              "WidebandB2",  HT_WIDEBAND_BANK_2,        0x470,  4,       5,     UNIT_LAMBDA,    0.001f, 0.0f,    20,   false,  0,    0.0f},
    {"Gear Selector Position",       "GearSelPos",  HT_GEAR_SELECTOR_POS,      0x470,  6,       6,     UNIT_ENUM,      1.0f,   0.0f,    20,   true,   0,    0.0f},
    {"Gear",                         "Gear",        HT_GEAR,                   0x470,  7,       7,     UNIT_ENUM,      1.0f,   0.0f,    20,   true,   0,    0.0f},
    {"Injector Pressure Diff",       "InjPresDiff", HT_PRESSURE_DIFFERENTIAL,  0x471,  0,       1,     UNIT_KPA,       0.1f,   0.0f,    50,   true,   0,    0.0f},
    {"Accelerator Pedal Position",   "AccPedalPos", HT_ACCELERATOR_PEDAL_POS,  0x471,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Exhaust Manifold Pressure",    "ExhManiPres", HT_EXHAUST_MANIFOLD_PRESS, 0x471,  4,       5,     UNIT_KPA,       0.1f,   0.0f,    50,   false,  0,    0.0f},
    {"Cruise Control Target Speed",  "CCTarget",    HT_CC_TARGET_SPEED,        0x472,  0,       1,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Cruise Control Last Target",   "CCLastTar",   HT_CC_LAST_TARGET_SPEED,   0x472,  2,       3,     UNIT_KPH,       0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Cruise Control Speed Error",   "CCError",     HT_CC_SPEED_ERROR,         0x472,  4,       5,     UNIT_KPH,       0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Cruise Control State",         "CCState",     HT_CC_STATE,               0x472,  6,       6,     UNIT_ENUM,      1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Cruise Control Input State",   "CCInState",   HT_CC_INPUT_STATE,         0x472,  6,       7,     UNIT_BIT_FIELD, 1.0f,   0.0f,    20,   false,  0,    0.0f},
    {"Total Fuel Used",              "TotFuelUsed", HT_TOTAL_FUEL_USED,        0x473,  0,       3,     UNIT_CC,        1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Rolling Antilag Switch State", "RollingALSt", HT_ROLLING_AL_SW_STATE,    0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Antilag Switch State",         "ALSwitchSt",  HT_AL_SWITCH_STATE,        0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Antilag Output State",         "ALOutputSt",  HT_AL_OUTPUT_STATE,        0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"TC Switch State",              "TCSwitchSt",  HT_TC_SWITCH_STATE,        0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Primary Fuel Pump Out State",  "PriFPOutSt",  HT_PRI_FP_OUTPUT_STATE,    0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Aux 1 Fuel Pump Output State", "Aux1FPOutSt", HT_AUX1_FP_OUTPUT_STATE,   0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Aux 2 Fuel Pump Output State", "Aux2FPOutSt", HT_AUX2_FP_OUTPUT_STATE,   0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Aux 3 Fuel Pump Output State", "Aux3FPOutSt", HT_AUX3_FP_OUTPUT_STATE,   0x473,  4,       4,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 1 Switch State",    "N2OEn1SwSt",  HT_N2O_ENABLE1_SW_STATE,   0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 1 Output State",    "N2OEn1OutSt", HT_N2O_ENABLE1_OUT_STATE,  0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 2 Switch State",    "N2OEn2SwSt",  HT_N2O_ENABLE2_SW_STATE,   0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 2 Output State",    "N2OEn2OutSt", HT_N2O_ENABLE2_OUT_STATE,  0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 3 Switch State",    "N2OEn3SwSt",  HT_N2O_ENABLE3_SW_STATE,   0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 3 Output State",    "N2OEn3OutSt", HT_N2O_ENABLE3_OUT_STATE,  0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 4 Switch State",    "N2OEn4SwSt",  HT_N2O_ENABLE4_SW_STATE,   0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous En 4 Output State",    "N2OEn4OutSt", HT_N2O_ENABLE4_OUT_STATE,  0x473,  5,       5,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 1 Switch St", "N2OOv1SwSt",  HT_N2O_OVR1_SW_STATE,      0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 1 Output St", "N2OOv1OutSt", HT_N2O_OVR1_OUT_STATE,     0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 2 Switch St", "N2OOv2SwSt",  HT_N2O_OVR2_SW_STATE,      0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 2 Output St", "N2OOv2OutSt", HT_N2O_OVR2_OUT_STATE,     0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 3 Switch St", "N2OOv3SwSt",  HT_N2O_OVR3_SW_STATE,      0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 3 Output St", "N2OOv3OutSt", HT_N2O_OVR3_OUT_STATE,     0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 4 Switch St", "N2OOv4SwSt",  HT_N2O_OVR4_SW_STATE,      0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Nitrous Override 4 Output St", "N2OOv4OutSt", HT_N2O_OVR4_OUT_STATE,     0x473,  6,       6,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Water Inj Adv En Switch St",   "WIAEnSwSt",   HT_WATER_INJ_EN_SWITCH,    0x473,  7,       7,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Water Inj Adv En Output St",   "WIAEnOutSt",  HT_WATER_INJ_EN_OUTPUT,    0x473,  7,       7,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Water Inj Adv Ovr Switch St",  "WIAOvrSwSt",  HT_WATER_INJ_OVR_SWITCH,   0x473,  7,       7,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Water Inj Adv Ovr Output St",  "WIAOvrOutSt", HT_WATER_INJ_OVR_OUTPUT,   0x473,  7,       7,     UNIT_BOOLEAN,   1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Cut Percentage Method",        "CutPerMeth",  HT_CUT_PERCENTAGE_METHOD,  0x473,  7,       7,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Vertical G",                   "VerticalG",   HT_VERTICAL_G,             0x474,  0,       1,     UNIT_MPS2,      0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Pitch Rate",                   "PitchRate",   HT_PITCH_RATE,             0x474,  2,       3,     UNIT_DEG_S,     0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Roll Rate",                    "RollRate",    HT_ROLL_RATE,              0x474,  4,       5,     UNIT_DEG_S,     0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Yaw Rate",                     "YawRate",     HT_YAW_RATE,               0x474,  6,       7,     UNIT_DEG_S,     0.1f,   0.0f,    20,   true,   0,    0.0f},
    {"Primary Fuel Pump Duty Cycle", "PriFPDC",     HT_PRIMARY_FUEL_PUMP_DUTY, 0x475,  0,       1,     UNIT_PERCENT,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Aux 1 Fuel Pump Duty Cycle",   "Aux1FPDC",    HT_AUX1_FUEL_PUMP_DUTY,    0x475,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Aux 2 Fuel Pump Duty Cycle",   "Aux2FPDC",    HT_AUX2_FUEL_PUMP_DUTY,    0x475,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Aux 3 Fuel Pump Duty Cycle",   "Aux3FPDC",    HT_AUX3_FUEL_PUMP_DUTY,    0x475,  6,       7,     UNIT_PERCENT,   0.1f,   0.0f,    5,    false,  0,    0.0f},
    {"Brake Pressure Rear",          "BrkPresRear", HT_BRAKE_PRESSURE_REAR,    0x476,  0,       1,     UNIT_KPA,       1.0f,   -101.3f, 20,   false,  0,    0.0f},
    {"Brake Pressure Front Ratio",   "BrkPresFRat", HT_BRAKE_PRESSURE_F_RATIO, 0x476,  2,       3,     UNIT_PERCENT,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Brake Pressure Rear Ratio",    "BrkPresRRat", HT_BRAKE_PRESSURE_R_RATIO, 0x476,  4,       5,     UNIT_PERCENT,   0.1f,   0.0f,    20,   false,  0,    0.0f},
    {"Brake Pressure Difference",    "BrkPresDiff", HT_BRAKE_PRESSURE_DIFF,    0x476,  6,       7,     UNIT_KPA,       1.0f,   0.0f,    20,   true,   0,    0.0f},
    {"Eng Limit Max",                "EngLimMax",   HT_ENGINE_LIMIT_MAX,       0x477,  0,       1,     UNIT_RPM,       1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"Cut Percent",                  "CutPercent",  HT_CUT_PERCENTAGE,         0x477,  1,       2,     UNIT_PERCENT,   0.1f,   0.0f,    10,   false,  0,    0.0f}, 
    {"Eng Limit Function",           "EngLimFunc",  HT_ENGINE_LIMIT_FUNCTION,  0x477,  4,       4,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"RPM Lim Function",             "RPMLimFunc",  HT_RPM_LIMIT_FUNCTION,     0x477,  5,       5,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"Cut Percent Function",         "CutPercFunc", HT_CUT_PERCENTAGE_FUNC,    0x477,  6,       6,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"Eng Limit Method",             "EngLimMeth",  HT_ENGINE_LIMIT_METHOD,    0x477,  7,       7,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"RPM Limit Method",             "RPMLimMeth",  HT_RPM_LIMIT_METHOD,       0x477,  7,       7,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f}, 
    {"Tire Pressure FL",             "TirePresFL",  HT_TIRE_PRESSURE_FL,       0x6F0,  0,       1,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f}, 
    {"Tire Pressure FR",             "TirePresFR",  HT_TIRE_PRESSURE_FR,       0x6F0,  2,       3,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f}, 
    {"Tire Pressure RL",             "TirePresRL",  HT_TIRE_PRESSURE_RL,       0x6F0,  4,       5,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f}, 
    {"Tire Pressure RR",             "TirePresRR",  HT_TIRE_PRESSURE_RR,       0x6F0,  6,       7,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f}, 
    {"Tire Temperature FL",          "TireTempFL",  HT_TIRE_TEMPERATURE_FL,    0x6F1,  0,       1,     UNIT_K,         0.1f,   0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Temperature FR",          "TireTempFR",  HT_TIRE_TEMPERATURE_FR,    0x6F1,  2,       3,     UNIT_K,         0.1f,   0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Temperature RL",          "TireTempRL",  HT_TIRE_TEMPERATURE_RL,    0x6F1,  4,       5,     UNIT_K,         0.1f,   0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Temperature RR",          "TireTempRR",  HT_TIRE_TEMPERATURE_RR,    0x6F1,  6,       7,     UNIT_K,         0.1f,   0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Sensor Battery Volt FL",  "TireSBatVFL", HT_TIRE_SENSOR_BATTERY_FL, 0x6F2,  0,       1,     UNIT_VOLTS,     0.001f, 0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Sensor Battery Volt FR",  "TireSBatVFR", HT_TIRE_SENSOR_BATTERY_FR, 0x6F2,  2,       3,     UNIT_VOLTS,     0.001f, 0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Sensor Battery Volt RL",  "TireSBatVRL", HT_TIRE_SENSOR_BATTERY_RL, 0x6F2,  4,       5,     UNIT_VOLTS,     0.001f, 0.0f,    5,    false,  0,    0.0f}, 
    {"Tire Sensor Battery Volt RR",  "TireSBatVRR", HT_TIRE_SENSOR_BATTERY_RR, 0x6F2,  6,       7,     UNIT_VOLTS,     0.001f, 0.0f,    5,    false,  0,    0.0f}, 
    {"Recommended Tire Pres Front",  "RecTirePrF",  HT_REC_TIRE_PRESSURE_F,    0x6F3,  0,       1,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Recommended Tire Pres Rear",   "RecTirePrR",  HT_REC_TIRE_PRESSURE_R,    0x6F3,  2,       3,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Tire Leak Detected R Right",   "TireLeakRR",  HT_TIRE_LEAK_RR,           0x6F3,  4,       4,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Tire Leak Detected R Left",    "TireLeakRL",  HT_TIRE_LEAK_RL,           0x6F3,  4,       4,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Tire Leak Detected F Right",   "TireLeakFR",  HT_TIRE_LEAK_FR,           0x6F3,  4,       4,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Tire Leak Detected F Left",    "TireLeakFL",  HT_TIRE_LEAK_FL,           0x6F3,  4,       4,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Engine Protection Sev Level",  "EPSevLevel",  HT_ENGINE_PROTECTION_SEV,  0x6F3,  5,       5,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Engine Protection Reason",     "EPReason",    HT_ENGINE_PROTECTION_REA,  0x6F3,  6,       7,     UNIT_KPA,       0.1f,   -101.3f, 5,    false,  0,    0.0f},
    {"Light State",                  "LightState",  HT_LIGHT_STATE,            0x6F4,  0,       0,     UNIT_BOOLEAN,   1.0f,   0.0f,    100,  false,  0,    0.0f},
    {"Total Fuel Used T1",           "FUELUSE",     HT_TOTAL_FUEL_USED_T1,     0x6F6,  0,       3,     UNIT_CC,        1.0f,   0.0f,    5,    true,   0,    0.0f},
    {"Trip Meter 1",                 "TRIP",        HT_TRIP_METER_1,           0x6F6,  4,       7,     UNIT_METERS,    1.0f,   0.0f,    5,    true,   0,    0.0f},
    {"Generic Output 1-20 States",   "GenOut1-20",  HT_GEN_OUT_STATES,         0x6F7,  0,       3,     UNIT_ENUM,      1.0f,   0.0f,    10,   false,  0,    0.0f},
    {"Calculated Air Temperature",   "CalcAirTemp", HT_CALCULATED_AIR_TEMP,    0x6F7,  4,       5,     UNIT_DEGREES,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Water Injection Adv Duty",     "WaterADuty",  HT_WATER_INJ_ADV_DUTY,     0x6F7,  6,       7,     UNIT_PERCENT,   0.1f,   0.0f,    10,   false,  0,    0.0f},
    {"Exhaust Cutout State",         "EXHCUT",      HT_EXHAUST_CUTOUT_STATE,   0x6F8,  0,       0,     UNIT_ENUM,      -1.0f,  0.0f,    5,    true,   0,    0.0f},
    {"Nitrous Bottle Opener State",  "NSTATE",      HT_N2O_BOTTLE_OPEN_STATE,  0x6F8,  1,       1,     UNIT_ENUM,      1.0f,   0.0f,    5,    true,   0,    0.0f},
};

SPIClass *customSPI = new SPIClass(HSPI);

MCP_CAN CAN0(customSPI, CS_PIN);

unsigned long KAinterval = 150;             // 50ms interval for keep aliv frame
unsigned long ButtonInfoInterval = 30;      // 30ms interval for button info frame
unsigned long KAintervalMillis = 0;         // storage for millis counter
unsigned long ButtonInfoIntervalMillis = 0; // storage for millis counter

HaltechCan::HaltechCan() : lastProcessTime(0)
{

}

bool HaltechCan::begin(long baudRate)
{
  customSPI->setFrequency(8000000);
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
    // Serial.println("ka");
  }

  // Execute buttoninfo frame every 30 ms
  if (currentMillis - ButtonInfoIntervalMillis >= ButtonInfoInterval)
  {
    ButtonInfoIntervalMillis = currentMillis;
    SendButtonInfo();
    // Serial.println("bi");
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

void HaltechCan::canRead(long unsigned int rxId, unsigned char len, unsigned char *rxBuf)
{
  // CAN Input from Haltech canbus
  byte txBuf[8] = {0};

  // Serial.printf("ID: 0x%02X, Len: 0x%02X Data: ", rxId, len);
  // for (int i = 0; i < 8; i++) {
  //   Serial.print(rxBuf[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();

  for (int i = 0; i < HT_NONE; i++) // loop through the enum till the last one, none
  {
    auto &dashVal = dashValues[(HaltechDisplayType_e)i];
    if (dashVal.can_id == rxId) // if the incoming value belongs to this dash value
    {
      uint32_t rawVal = extractValue(rxBuf, dashVal.start_byte, dashVal.end_byte);
      dashVal.scaled_value = (float)rawVal * dashVal.scale_factor + dashVal.offset;
      Serial.printf("%lu %s %f\n", millis(), dashVal.name, dashVal.scaled_value);
      for (int buttonIndex = 0; buttonIndex < nButtons; buttonIndex++)
      {
        // Serial.printf("htb type: %u, dv type: %u\n", htButtons[buttonIndex].type, dashVal.type);
        if (dashVal.type == htButtons[buttonIndex].dashValue->type)
        {
          htButtons[buttonIndex].drawValue(dashVal.scaled_value);
        }
      }
    }
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

  for (int j = 0; j < 2; j++)
  {
    static uint8_t prevButtonInfo[2] = {0}; // Local static array to store previous values
    for (int i = 0; i < nButtons / 2; i++)
    {
      bitWrite(ButtonInfo[j], i, htButtons[i + j * 8].isPressed());
    }
    // Only print if the value has changed
    if (ButtonInfo[j] != prevButtonInfo[j])
    {
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
