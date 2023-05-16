/*
 *   ESP32 Template
 *   User specific defines and Function declarations
 */
#ifndef USER_CONFIG_H
#define USER_CONFIG_H

#include "mqtt-ota-config.h"

//
// Define required user libraries here
// Don't forget to add them into platformio.ini or the /lib directory
//
#include <daly-bms-uart.h>
#include <VeDirectFrameHandler.h>
#include <SoftwareSerial.h>
#ifdef ENA_ONEWIRE // Optional OneWire support
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

//
// Declare user_setup, user_loop and custom global functions
//
extern void user_setup();
extern void user_loop();

//
// Declare global user specific objects
//
extern Daly_BMS_UART bms;
extern SoftwareSerial VEDSer_Chrg1;
extern VeDirectFrameHandler VED_Chrg1;
extern SoftwareSerial VEDSer_Shnt;
extern VeDirectFrameHandler VED_Shnt;
#ifdef ENA_ONEWIRE // Optional OneWire support
extern OneWire oneWire;
extern DallasTemperature OWtemp;
#endif

//
// Hardware Setup - User SSR
// HIGH level -> SSR is switched ON
//
#define PIN_SSR1 5 // Battery Load Switch 1
#define PIN_SSR2 3 // Active Balancer Enable
#define PIN_SSR3 6 // Battery Load Switch 2
#define PIN_SSR4 7 // unused - defined to match KiCad board

//
// Controller specific
//
// MQTT Topics for published data
#define t_Ctrl_StatT TOPTREE "CtrlStatTXT" // ESP Controller status text provided through MQTT
#define t_Ctrl_StatU TOPTREE "CtrlStatUpt" // ESP Controller uptime provided through MQTT

//
// MQTT Data update interval
//
#define DATA_UPDATE_INTERVAL 120 // Send MQTT data ever x seconds

//
// OneWire Bus
//
#ifdef ENA_ONEWIRE           // Optional OneWire support - doesn't work yet (doesn't detect sensor)!
#define PIN_OWDATA 4         // GPIO for OneWire communication (Note: high GPIOs >36 do not work!)
#define OWRES 9              // Use 9 bits resolution (0.5°C)
#define NUM_OWTEMP 2         // Amount of connected DS18B20 sensors
#define OW_UPDATE_INTERVAL 5 // sensor readout interval in seconds
#define OW_TIMEOUT 60        // If no data update for all sensors occur within this timespan (seconds), connection to OneWire considered dead
// MQTT Topics for published data
#define t_OW_TEMP_Templ TOPTREE "OW_T" // will be extended with sensor numbers starting at 1
#define t_OW_CSTAT TOPTREE "OW_CSTAT"
#endif // ENA_ONEWIRE

//
// Daly BMS Settings
//
// UART
// (defaults to GPIO17 for TXD and GPIO18 for RXD on ESP32-S2)
#define DALY_UART Serial1
// Data readout interval in seconds
#define DALY_UPDATE_INTERVAL 5
#define DALY_TIMEOUT 60 // If no data update occurs within this timespan (seconds), connection to Daly BMS considered dead

// Battery Safety: Shutdown all loads when reaching this threshold
#define SAFETY_BAT_MIN_V 24.8f

// MQTT Publish only
#define t_DSOC TOPTREE "Daly_SOC"      // BMS Battery State of Charge(useless, see current)
#define t_DV TOPTREE "Daly_V"          // BMS Battery Voltage
#define t_DdV TOPTREE "Daly_dV"        // BMS Voltage diff between highest and lowest cell voltage
#define t_DI TOPTREE "Daly_I"          // BMS Battery Current(useless, only shows currents > 1.1A!)
#define t_DV_C_Templ TOPTREE "Daly_C"  // Cell Voltage template; adding cell number + "V" at the end (transmitted cell number starts with 1)
#define t_DLSw TOPTREE "Daly_LSw"      // Actual "switch state" for load MOSFETs (0/1)
#define t_DCSw TOPTREE "Daly_CSw"      // Actual "switch state" for charging MOSFETs (0/1)
#define t_DTemp TOPTREE "Daly_Temp"    // Temperature sensor of the BMS
#define t_D_CSTAT TOPTREE "Daly_CSTAT" // Daly Serial connection status

//
// Load Configuration (SSR1 + SSR3)
// Updated by MQTT Topics at firmware boot
//
#define DEF_OFF_LOAD_SOC 10   // Default SOC at which to disable the load
#define DEF_LP_ON_LOAD_SOC 90 // SOC at which to enable the load at low PV power
#define DEF_HP_ON_LOAD_SOC 60 // SOC at which to enable the load at high PV power
#define DEF_BOOT_ON_SOC 90    // If SOC is >= this value, load will be enabled at firmware startup

// MQTT Publish only
#define t_Ctrl_Cfg_SSR1_actState TOPTREE "Cfg/SSR1_actState" // Actual state of SSR1 GPIO (on/off)
#define t_Ctrl_Cfg_SSR3_actState TOPTREE "Cfg/SSR3_actState" // Actual state of SSR3 GPIO (on/off)

// MQTT Topics with subscriptions
// Remember: All of the subscribed topics need pre-defined (retained) values!
#define t_Ctrl_Cfg_SSR1_setState TOPTREE "Cfg/SSR1_setState" // User-desired state of SSR1 GPIO (on/off)
#define t_Ctrl_Cfg_SSR1_Auto TOPTREE "Cfg/SSR1_Auto"
#define t_Ctrl_Cfg_SSR1_LPOnSOC TOPTREE "Cfg/SSR1_LPOnSOC"
#define t_Ctrl_Cfg_SSR1_LPOffSOC TOPTREE "Cfg/SSR1_LPOffSOC"
#define t_Ctrl_Cfg_SSR1_HPOnSOC TOPTREE "Cfg/SSR1_HPOnSOC"
#define t_Ctrl_Cfg_SSR1_HPOffSOC TOPTREE "Cfg/SSR1_HPOffSOC"
#define t_Ctrl_Cfg_SSR1_BootOnSOC TOPTREE "Cfg/SSR1_BootOnSOC"

#define t_Ctrl_Cfg_SSR3_setState TOPTREE "Cfg/SSR3_setState" // Desired state of SSR3 GPIO (on/off)
#define t_Ctrl_Cfg_SSR3_Auto TOPTREE "Cfg/SSR3_Auto"
#define t_Ctrl_Cfg_SSR3_LPOnSOC TOPTREE "Cfg/SSR3_LPOnSOC"
#define t_Ctrl_Cfg_SSR3_LPOffSOC TOPTREE "Cfg/SSR3_LPOffSOC"
#define t_Ctrl_Cfg_SSR3_HPOnSOC TOPTREE "Cfg/SSR3_HPOnSOC"
#define t_Ctrl_Cfg_SSR3_HPOffSOC TOPTREE "Cfg/SSR3_HPOffSOC"
#define t_Ctrl_Cfg_SSR3_BootOnSOC TOPTREE "Cfg/SSR3_BootOnSOC"

#define t_Ctrl_Cfg_PV_LowPPV TOPTREE "Cfg/LowPPV"
#define t_Ctrl_Cfg_PV_HighPPV TOPTREE "Cfg/HighPPV"

//
// Active Balancer Settings (SSR2)
//
// Default settings, Updated by MQTT Topics at firmware boot
#define DEF_BAL_ON_CELLDIFF 30     // If cell voltage difference is higher than this [mV] AND
#define DEF_BAL_ON_CELLV 3400      // if a cell has reached this voltage level [mV], enable the balancer
#define DEF_BAL_OFF_CELLV 3200     // DISABLE balancer if a cell has fallen below this voltage level [mV]
#define DEF_BAL_ALARM_CELLDIFF 300 // If cell voltage difference is higher than this force-enable balancer
#define DEF_BAL_NO_ALARM_CD 10     // Exit alarm mode when Celldiff drops below this threshold

// MQTT Publish only
#define t_Ctrl_Cfg_SSR2_actState TOPTREE "Cfg/SSR2_actState" // Actual state of SSR2 GPIO (on/off)

// MQTT Topics with subscriptions
#define t_Ctrl_Cfg_SSR2_setState TOPTREE "Cfg/SSR2_setState"
#define t_Ctrl_Cfg_SSR2_Auto TOPTREE "Cfg/SSR2_Auto"
#define t_Ctrl_Cfg_SSR2_CVOn TOPTREE "Cfg/SSR2_CVOn"
#define t_Ctrl_Cfg_SSR2_CVOff TOPTREE "Cfg/SSR2_CVOff"
#define t_Ctrl_Cfg_SSR2_CdiffOn TOPTREE "Cfg/SSR2_CdiffOn"
#define t_Ctrl_Cfg_SSR2_AlrmCdiff TOPTREE "Cfg/SSR2_AlrmCdiff"

//
// Victron VE.Direct settings
//
// Global Settings
#define VED_BAUD 19200  // Baud rate (used for all VE.Direct devices)
#define VED_TIMEOUT 180 // If no data update occurs within this timespan (seconds), connection to VE.Direct device is considered dead

// SmartSolar 75/15 Charger settings (charger #1)
#define PIN_VED_CHRG1_RX 33 // RX for SoftwareSerial
#define PIN_VED_CHRG1_TX 21 // TX for SoftwareSerial (unused)
// Array element indexes of VED_Chrg1.veValue which will be sent to the MQTT broker (ATTN: valid for SmartSolar 75/15 with firmware 1.61)
// See Victron Documentation: https://www.victronenergy.com/support-and-downloads/technical-information# --> VE.Direct protocol
#define i_CHRG_LBL_VB 3   // battery voltage
#define i_CHRG_LBL_IB 4   // charging current
#define i_CHRG_LBL_PPV 6  // present PV Power
#define i_CHRG_LBL_CS 7   // charger state
#define i_CHRG_LBL_MPPT 8 // MPPT state
#define i_CHRG_LBL_ERR 10 // error state
#define i_CHRG_LBL_H20 14 // yield today (in 10Wh increments)
// MQTT Topics for published data
#define t_VED_C1_PPV TOPTREE "VC1_PPV"
#define t_VED_C1_IB TOPTREE "VC1_IB"
#define t_VED_C1_VB TOPTREE "VC1_VB"
#define t_VED_C1_ERR TOPTREE "VC1_ERR"
#define t_VED_C1_CS TOPTREE "VC1_CS"
#define t_VED_C1_MPPT TOPTREE "VC1_MPPT"
#define t_VED_C1_H20 TOPTREE "VC1_H20"
#define t_VED_C1_CSTAT TOPTREE "VC1_CSTAT"
#define t_VED_C1_AvgPPV TOPTREE "VC1_AvgPPV" // Calculated 1hr PV power average

// SmartShunt 500 settings
#define PIN_VED_SHNT_RX 35 // RX for SoftwareSerial
#define PIN_VED_SHNT_TX 34 // TX for SoftwareSerial (unused)

// Readout error detection
#define VSS_MAX_SOC_DIFF 5 // max. allowed diff of SOC value between 2 readouts (larger diff -> new value ignored)

// Array element indexes of VED_Shnt.veValue which will be sent to the MQTT broker (ATTN: valid for SmartShunt 500 with firmware 4.12)
// See Victron Documentation: https://www.victronenergy.com/support-and-downloads/technical-information# --> VE.Direct protocol
// ATTN: This DOES NOT WORK with the SmartShunt! The data in .veName and .veValue arrays are in different order nearly every ESP boot
// the following defines refer to the elements in the VED_SS_Labels array, which is used to get the corresponding indexes from .veName
#define i_SS_LBL_PID 0   // Device PID
#define i_SS_LBL_V 1     // battery voltage [mV]
#define i_SS_LBL_I 2     // battery current [mA]
#define i_SS_LBL_P 3     // instantaneous power [W]
#define i_SS_LBL_CE 4    // consumed Ah [mAh]
#define i_SS_LBL_SOC 5   // battery SOC [‰]
#define i_SS_LBL_TTG 6   // time to empty [min] (-1 if not dicharging)
#define i_SS_LBL_ALARM 7 // Alarm active flag (ON/OFF)
#define i_SS_LBL_AR 8    // Alarm Reason

// MQTT Topics for published data
#define t_VED_SH_V TOPTREE "VSS_V"
#define t_VED_SH_I TOPTREE "VSS_I"
#define t_VED_SH_P TOPTREE "VSS_P"
#define t_VED_SH_CE TOPTREE "VSS_CE"
#define t_VED_SH_SOC TOPTREE "VSS_SOC"
#define t_VED_SH_TTG TOPTREE "VSS_TTG"
#define t_VED_SH_ALARM TOPTREE "VSS_ALARM"
#define t_VED_SH_AR TOPTREE "VSS_AR"
#define t_VED_SH_CSTAT TOPTREE "VSS_CSTAT" // SmartShunt connection status

//
// Data structures
//
// Struct for VE.Direct SmartShunt data
struct VED_Shunt_data
{
    float V = 0; // useful data copied from VE.Direct frame
    float I = 0;
    int P = 0;
    float CE = 0;
    float SOC = 0;
    int TTG = 0;
    int iV = 255; // All i* vars point to the index of the corresponding values in VEdirectFrameHandler .veValue array
    int iI = 255; // will be updated to valid values by the getIndexByName function at runtime
    int iP = 255;
    int iCE = 255;
    int iSOC = 255;
    int iTTG = 255;
    int iALARM = 255;
    int iAR = 255;
    uint32_t lastUpdate = 0;    // to verify connection is active
    uint32_t lastDecodedFr = 0; // uptime of last decoded frame
    int ConnStat = 0;           // Connection Status
};

// Struct for VE.Direct Charger data
struct VED_Charger_data
{
    int PPV = 0;
    float Avg_PPV = 0; // 1h average PV power
    int Avg_PPV_Arr[10] = {0};
    int iVB = i_CHRG_LBL_VB; // All i* vars point to the index of the corresponding values in VEdirectFrameHandler .veValue array
    int iIB = i_CHRG_LBL_IB; // for the charger, the data order is always the same, so static values can be used here from defines above
    int iPPV = i_CHRG_LBL_PPV;
    int iCS = i_CHRG_LBL_CS;
    int iMPPT = i_CHRG_LBL_MPPT;
    int iERR = i_CHRG_LBL_ERR;
    int iH20 = i_CHRG_LBL_H20;
    uint32_t lastUpdate = 0;    // to verify connection is active
    uint32_t lastDecodedFr = 0; // uptime of last decoded frame
    int ConnStat = 0;           // Connection Status
};

// Additional Daly BMS data
struct Daly_BMS_data
{
    int ConnStat = 0; // Connection status
    // ATTENTION: BMS FET states should only be set by the BMS-Controller in case of emergency!
    int setLSw = 2;          // desired state of Daly Load (Discharge) FETs (0=off, 1=on, 2=dnc)
    int setCSw = 2;          // desired state of Daly Charge FETs (0=off, 1=on, 2=dnc)
    uint32_t lastUpdate = 0; // to verify connection is active
    uint32_t lastValid = 0;  // uptime of last valid frame
};

#ifdef ENA_ONEWIRE // Optional OneWire support
struct OneWire_data
{
    // Array for OneWire Temperature Sensor(s) addresses
    float Sensors[NUM_OWTEMP] = {0};
    int ConnStat = 0;
    uint32_t lastUpdate = 0;
    uint32_t lastValid = 0;
    bool ReadOk = true;
};
#endif

// Config struct for load switching SSRs
struct Load_SSR_Config
{
    bool actState = false;            // Currently active state of the SSR (GPIO)
    bool setState = false;            // desired (set) state of the SSR (either by this sketch or received by MQTT message)
    bool Auto = false;                // Automatic mode (enable/disable by MQTT)
    int LPOnSOC = DEF_LP_ON_LOAD_SOC; // Low PV power ON SOC (configured via MQTT topic)
    int LPOffSOC = DEF_OFF_LOAD_SOC;  // Low PV power OFF SOC (configured via MQTT topic)
    int HPOnSOC = DEF_HP_ON_LOAD_SOC; // High PV power ON SOC (configured via MQTT topic)
    int HPOffSOC = DEF_OFF_LOAD_SOC;  // High PV power OFF SOC (configured via MQTT topic)
    int BootOnSOC = DEF_BOOT_ON_SOC;  // SOC at which to enable SSR at firmware boot (configured via MQTT topic)
};

// Config struct for balancer (SSR2)
struct Balancer_Config
{
    bool actState = false;                  // Currently active state of the balancer SSR (GPIO)
    bool setState = false;                  // desired (set) state of the balancer (either by this sketch or received by MQTT message)
    bool Auto = false;                      // Automatic mode (enable/disable by MQTT)
    bool AlarmMode = false;                 // Alarm mode (AlrmCdiff reached); auto mode will be disabled if triggered
    int CVOn = DEF_BAL_ON_CELLV;            // Minimum single cell voltage at which to enable balancer (configured via MQTT topic)
    int CVOff = DEF_BAL_OFF_CELLV;          // Min. single cell voltage at which to disable balancer (configured via MQTT topic)
    int CdiffOn = DEF_BAL_ON_CELLDIFF;      // Minimum celldiff (diff between highest and lowest cell voltage) at which to enable balancer (configured via MQTT topic)
    int AlrmCdiff = DEF_BAL_ALARM_CELLDIFF; // Maximum celldiff at which to start balancer in alarm mode (configured via MQTT topic)
    int NoAlrmCdiff = DEF_BAL_NO_ALARM_CD;  // Celldiff at which to leave balancer alarm mode
};

// Config struct for PV specific settings
struct PV_Config
{
    int PwrLvl = 0;    // current power level (0=low, 1=medium, 2=high)
    int LowPPV = 100;  // < Avg_PPV considered as low (configured via MQTT topic)
    int HighPPV = 300; // > Avg_PPV considered as high (configured via MQTT topic)
};

//
// Global Structs
//
extern Load_SSR_Config SSR1; // Config struct for SSR1
extern Load_SSR_Config SSR3; // Config struct for SSR3
extern Balancer_Config SSR2; // Config struct for SSR2
extern PV_Config PV;         // Config struct for PV related stuff

#endif // USER_CONFIG_H