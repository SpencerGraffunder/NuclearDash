#ifndef HALTECH_CAN_H
#define HALTECH_CAN_H

#include <Arduino.h>

#define CS_PIN 15
#define MOSI_PIN 14
#define MISO_PIN 35
#define SCK_PIN 12
#define CAN0_INT 5

#define DEBUG(x, ...) Serial.printf(x, ##__VA_ARGS__)

typedef enum
{
    HT_RPM = 0,
    HT_MANIFOLD_PRESSURE,
    HT_THROTTLE_POSITION,
    HT_COOLANT_PRESSURE,
    HT_FUEL_PRESSURE,
    HT_OIL_PRESSURE,
    HT_ENGINE_DEMAND,
    HT_WASTEGATE_PRESSURE,
    HT_INJ_STATE_1_DUTY,
    HT_INJ_STATE_2_DUTY,
    HT_IGNITION_ANGLE,
    HT_WHEEL_SLIP,
    HT_WHEEL_DIFF,
    HT_LAUNCH_CONTROL_END_RPM,
    HT_INJ1_AVG_TIME, 
    HT_INJ2_AVG_TIME, 
    HT_INJ3_AVG_TIME, 
    HT_INJ4_AVG_TIME, 
    HT_WIDEBAND_SENSOR_1,
    HT_WIDEBAND_SENSOR_2,
    HT_WIDEBAND_SENSOR_3,
    HT_WIDEBAND_SENSOR_4,
    HT_TRIGGER_ERROR_COUNT,
    HT_TRIGGER_COUNTER,
    HT_TRIGGER_SYNC_LEVEL,
    HT_KNOCK_LEVEL_1,
    HT_KNOCK_LEVEL_2,
    HT_BRAKE_PRESSURE_FRONT,
    HT_NOS_PRESSURE_1,
    HT_TURBO_SPEED_1,
    HT_LATERAL_G,
    HT_WHEEL_SPEED_FL,
    HT_WHEEL_SPEED_FR,
    HT_WHEEL_SPEED_RL,
    HT_WHEEL_SPEED_RR,
    HT_EXHAUST_CAM_ANGLE_1,
    HT_EXHAUST_CAM_ANGLE_2,
    HT_ENGINE_LIM_ACTIVE,
    HT_LC_IGN_RETARD,
    HT_LC_FUEL_ENRICH,
    HT_LONGITUDINAL_G,
    HT_GENERIC_OUTPUT_1_DUTY,
    HT_BOOST_CONTROL_OUTPUT,
    HT_VEHICLE_SPEED,
    HT_INTAKE_CAM_ANGLE_1,
    HT_INTAKE_CAM_ANGLE_2,
    HT_FUEL_FLOW,
    HT_FUEL_FLOW_RETURN,
    HT_BATTERY_VOLTAGE,
    HT_TARGET_BOOST_LEVEL,
    HT_BARO_PRESSURE,
    HT_EGT_SENSOR_1,
    HT_EGT_SENSOR_2,
    HT_AMBIENT_AIR_TEMP,
    HT_RELATIVE_HUMIDITY,
    HT_SPECIFIC_HUMIDITY,
    HT_ABSOLUTE_HUMIDITY,
    HT_COOLANT_TEMPERATURE,
    HT_AIR_TEMPERATURE,
    HT_FUEL_TEMPERATURE,
    HT_OIL_TEMPERATURE,
    HT_GEARBOX_OIL_TEMP,
    HT_DIFF_OIL_TEMPERATURE,
    HT_FUEL_COMPOSITION,
    HT_FUEL_LEVEL,
    HT_FUEL_TRIM_SHORT_TERM_1,
    HT_FUEL_TRIM_SHORT_TERM_2,
    HT_FUEL_TRIM_LONG_TERM_1,
    HT_FUEL_TRIM_LONG_TERM_2,
    HT_TORQUE_MGMT_KNOB,
    HT_GEARBOX_LINE_PRESSURE,
    HT_INJ_STAGE_3_DUTY,
    HT_INJ_STAGE_4_DUTY,
    HT_CRANK_CASE_PRESSURE,
    HT_RACE_TIMER,
    HT_IGNITION_ANGLE_BANK_1,
    HT_IGNITION_ANGLE_BANK_2,
    HT_TM_RPM_TARGET,
    HT_TM_RPM_ERROR,         
    HT_TM_RPM_ERROR_IGN_CORR,
    HT_TM_RPM_TIMED_IGN_CORR,
    HT_TM_COMBINED_IGN_CORR, 
    HT_WIDEBAND_SENSOR_5,    
    HT_WIDEBAND_SENSOR_6,    
    HT_WIDEBAND_SENSOR_7,    
    HT_WIDEBAND_SENSOR_8,    
    HT_WIDEBAND_SENSOR_9,    
    HT_WIDEBAND_SENSOR_10,   
    HT_WIDEBAND_SENSOR_11,   
    HT_WIDEBAND_SENSOR_12,   
    HT_SHOCK_TRAVEL_FL_UNCAL,
    HT_SHOCK_TRAVEL_FR_UNCAL,
    HT_SHOCK_TRAVEL_RL_UNCAL,
    HT_SHOCK_TRAVEL_RR_UNCAL,
    HT_SHOCK_TRAVEL_FL,      
    HT_SHOCK_TRAVEL_FR,      
    HT_SHOCK_TRAVEL_RL,      
    HT_SHOCK_TRAVEL_RR,      
    HT_ECU_TEMPERATURE,      
    HT_WIDEBAND_OVERALL,
    HT_WIDEBAND_BANK_1,
    HT_WIDEBAND_BANK_2,
    HT_GEAR_SELECTOR_POS,
    HT_GEAR,
    HT_PRESSURE_DIFFERENTIAL,
    HT_ACCELERATOR_PEDAL_POS,
    HT_EXHAUST_MANIFOLD_PRESS,
    HT_CC_TARGET_SPEED,
    HT_CC_LAST_TARGET_SPEED,
    HT_CC_SPEED_ERROR,
    HT_CC_STATE,
    HT_CC_INPUT_STATE,
    HT_TOTAL_FUEL_USED,
    HT_ROLLING_AL_SW_STATE,
    HT_AL_SWITCH_STATE,
    HT_AL_OUTPUT_STATE,
    HT_TC_SWITCH_STATE,
    HT_PRI_FP_OUTPUT_STATE,
    HT_AUX1_FP_OUTPUT_STATE,
    HT_AUX2_FP_OUTPUT_STATE,
    HT_AUX3_FP_OUTPUT_STATE,
    HT_N2O_ENABLE1_SW_STATE,
    HT_N2O_ENABLE1_OUT_STATE,
    HT_N2O_ENABLE2_SW_STATE,
    HT_N2O_ENABLE2_OUT_STATE,
    HT_N2O_ENABLE3_SW_STATE,
    HT_N2O_ENABLE3_OUT_STATE,
    HT_N2O_ENABLE4_SW_STATE,
    HT_N2O_ENABLE4_OUT_STATE,
    HT_N2O_OVR1_SW_STATE,      
    HT_N2O_OVR1_OUT_STATE,     
    HT_N2O_OVR2_SW_STATE,      
    HT_N2O_OVR2_OUT_STATE,     
    HT_N2O_OVR3_SW_STATE,      
    HT_N2O_OVR3_OUT_STATE,     
    HT_N2O_OVR4_SW_STATE,      
    HT_N2O_OVR4_OUT_STATE,     
    HT_WATER_INJ_EN_SWITCH,    
    HT_WATER_INJ_EN_OUTPUT,    
    HT_WATER_INJ_OVR_SWITCH,   
    HT_WATER_INJ_OVR_OUTPUT,   
    HT_CUT_PERCENTAGE_METHOD,  
    HT_VERTICAL_G,             
    HT_PITCH_RATE,             
    HT_ROLL_RATE,              
    HT_YAW_RATE,               
    HT_PRIMARY_FUEL_PUMP_DUTY, 
    HT_AUX1_FUEL_PUMP_DUTY,    
    HT_AUX2_FUEL_PUMP_DUTY,    
    HT_AUX3_FUEL_PUMP_DUTY,    
    HT_BRAKE_PRESSURE_REAR,    
    HT_BRAKE_PRESSURE_F_RATIO, 
    HT_BRAKE_PRESSURE_R_RATIO, 
    HT_BRAKE_PRESSURE_DIFF,    
    HT_ENGINE_LIMIT_MAX,       
    HT_CUT_PERCENTAGE,         
    HT_ENGINE_LIMIT_FUNCTION,  
    HT_RPM_LIMIT_FUNCTION,     
    HT_CUT_PERCENTAGE_FUNC,    
    HT_ENGINE_LIMIT_METHOD,    
    HT_RPM_LIMIT_METHOD,       
    HT_TIRE_PRESSURE_FL,       
    HT_TIRE_PRESSURE_FR,       
    HT_TIRE_PRESSURE_RL,       
    HT_TIRE_PRESSURE_RR,       
    HT_TIRE_TEMPERATURE_FL,    
    HT_TIRE_TEMPERATURE_FR,    
    HT_TIRE_TEMPERATURE_RL,    
    HT_TIRE_TEMPERATURE_RR,    
    HT_TIRE_SENSOR_BATTERY_FL, 
    HT_TIRE_SENSOR_BATTERY_FR, 
    HT_TIRE_SENSOR_BATTERY_RL, 
    HT_TIRE_SENSOR_BATTERY_RR, 
    HT_REC_TIRE_PRESSURE_F,    
    HT_REC_TIRE_PRESSURE_R,    
    HT_TIRE_LEAK_RR,           
    HT_TIRE_LEAK_RL,           
    HT_TIRE_LEAK_FR,           
    HT_TIRE_LEAK_FL,           
    HT_ENGINE_PROTECTION_SEV,  
    HT_ENGINE_PROTECTION_REA,  
    HT_LIGHT_STATE,            
    HT_TOTAL_FUEL_USED_T1,     
    HT_TRIP_METER_1,           
    HT_GEN_OUT_STATES,
    HT_CALCULATED_AIR_TEMP,
    HT_WATER_INJ_ADV_DUTY,
    HT_EXHAUST_CUTOUT_STATE,
    HT_N2O_BOTTLE_OPEN_STATE,
    HT_NONE
} HaltechDisplayType_e;

typedef enum
{
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
    UNIT_PSI,
    UNIT_PSI_ABS,
    UNIT_DEG_S,
} HaltechUnit_e;

// Each displayable field needs to have an instance of this struct
struct HaltechDashValue
{
    const char* name;
    const char* short_name;
    HaltechDisplayType_e type;
    uint32_t can_id; // From Haltech's documentation
    uint8_t start_byte;
    uint8_t end_byte;
    HaltechUnit_e incomingUnit;     // Unit that the raw data will be converted to using the scale factor and offset
    float scale_factor;             // Multiply to scale the raw data
    float offset;                   // Add to raw data after scaling
    unsigned long update_period;    // Period between expected updates from the ECU
    bool isSigned;
    unsigned long last_update_time; // Millis when last updated
    float scaled_value;             // Value after scaling has been applied
};

extern HaltechDashValue dashValues[HT_NONE];

class HaltechCan
{
public:
    HaltechCan();
    bool begin(long baudRate = 1000E3);
    void process();

private:
    unsigned long lastProcessTime;
    uint32_t extractValue(const uint8_t *buffer, uint8_t start_byte, uint8_t end_byte);
    void canRead(long unsigned int rxId, unsigned char len, unsigned char *rxBuf);
    void SendButtonInfo();
    void SendKeepAlive();
};

#endif // HALTECH_CAN_H