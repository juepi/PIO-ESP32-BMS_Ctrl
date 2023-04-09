/*
 *   ESP32 Template
 *   User specific defines and Function declarations
 */
#ifndef USER_SETUP_H
#define USER_SETUP_H

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
// Global user vars
//
// Modified by MQTT subscriptions
extern int Ctrl_DalyChSw;   // Desired Daly MOSFET switch states, either on, off or dnc (do not change)
extern int Ctrl_DalyLoadSw; // if set to on/off, will be set ONCE by the ESP, then reset to dnc
extern int Ctrl_SSR1;       // SSR1 switch state (on/off/dnc)
extern int Ctrl_SSR2;       // SSR2 switch state (on/off/dnc)

//
// User SSR
// HIGH level -> SSR is switched ON
//
#define SSR1 5 // Battery Load Switch
#define SSR2 3 // Active Balancer Enable
#define SSR3 6 // unused - defined to match KiCad board
#define SSR4 7 // unused - defined to match KiCad board

//
// OneWire Bus
//
#ifdef ENA_ONEWIRE           // Optional OneWire support - doesn't work yet (doesn't detect sensor)!
#define OWDATA 4             // GPIO for OneWire communication (Note: high GPIOs >36 do not work!)
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

//
// Load settings (SSR1)
//
#define ENABLE_LOAD_SOC 80     // SOC at which to enable the load (SSR1)
#define DISABLE_LOAD_SOC 10    // SOC at which load will be disconnected
#define HIGH_PV_AVG_PWR 20     // If the average charging power is higher than this..
#define HIGH_PV_EN_LOAD_SOC 50 //.. enable the load at an earlier SOC to avoid wasting PV energy
#define BOOT_EN_LOAD_SOC 30    // If SOC is >= this value, load will be enabled at firmware startup

//
// Active Balancer Settings (SSR2)
//
#define BAL_ON_CELLDIFF 30 // If cell voltage difference is higher than this [mV] AND
#define BAL_ON_CELLV 3400  // ..if a cell has reached this voltage level [mV], enable the balancer
#define BAL_OFF_CELLV 3300 // DISABLE balancer if a cell has fallen below this voltage level [mV]

//
// Victron VE.Direct settings
//
// Global Settings
#define VED_BAUD 19200  // Baud rate (used for all VE.Direct devices)
#define VED_TIMEOUT 180 // If no data update occurs within this timespan (seconds), connection to VE.Direct device is considered dead

// SmartSolar 75/15 Charger settings (charger #1)
#define VED_CHRG1_RX 33 // RX for SoftwareSerial
#define VED_CHRG1_TX 21 // TX for SoftwareSerial (unused)
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
#define VED_SHNT_RX 35 // RX for SoftwareSerial
#define VED_SHNT_TX 34 // TX for SoftwareSerial (unused)

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
// MQTT Data update interval
//
#define DATA_UPDATE_INTERVAL 120 // Send MQTT data ever x seconds

//
// MQTT Topics for BMS, Load and Balancer Controlling and Monitoring
//
// Publish only
#define t_DSOC TOPTREE "Daly_SOC"             // BMS Battery State of Charge(useless, see current)
#define t_DV TOPTREE "Daly_V"                 // BMS Battery Voltage
#define t_DdV TOPTREE "Daly_dV"               // BMS Voltage diff between highest and lowest cell voltage
#define t_DI TOPTREE "Daly_I"                 // BMS Battery Current(useless, only shows currents > 1.1A!)
#define t_DV_C_Templ TOPTREE "Daly_C"         // Cell Voltage template; adding cell number + "V" at the end (transmitted cell number starts with 1)
#define t_DLSw TOPTREE "Daly_LSw"             // Actual "switch state" for load MOSFETs (0/1)
#define t_DCSw TOPTREE "Daly_CSw"             // Actual "switch state" for charging MOSFETs (0/1)
#define t_DTemp TOPTREE "Daly_Temp"           // Temperature sensor of the BMS
#define t_D_CSTAT TOPTREE "Daly_CSTAT"        // Daly Serial connection status
#define t_Ctrl_StatT TOPTREE "CtrlStatTXT"    // ESP Controller status text provided through MQTT (basically to check if UART to Daly BMS is OK)
#define t_Ctrl_StatU TOPTREE "CtrlStatUpt"    // ESP Controller uptime provided through MQTT
#define t_Ctrl_actSSR1 TOPTREE "Ctrl_actSSR1" // Actual state of SSR1 GPIO (on/off)
#define t_Ctrl_actSSR2 TOPTREE "Ctrl_actSSR2" // Actual state of SSR2 GPIO (on/off)
// Subscriptions
#define t_Ctrl_CSw TOPTREE "Ctrl_CSw"   // User-desired state of Daly charging MOSFETs (on/off or dnc for "do not change")
#define t_Ctrl_LSw TOPTREE "Ctrl_LSw"   // User-desired state of Daly discharging MOSFETs (on/off or dnc)
#define t_Ctrl_SSR1 TOPTREE "Ctrl_SSR1" // User-desired state of SSR1 GPIO (on/off/dnc)
#define t_Ctrl_SSR2 TOPTREE "Ctrl_SSR2" // User-desired state of SSR2 GPIO (on/off/dnc)

//
// Data structures
//
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
    uint32_t lastUpdate = 0;  // to verify connection is active
    uint32_t lastValidFr = 0; // last valid frame decoded (uptime)
    int ConnStat = 0;         // Connection Status
};

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
    uint32_t lastUpdate = 0;  // to verify connection is active
    uint32_t lastValidFr = 0; // last valid frame decoded (uptime)
    int ConnStat = 0;         // Connection Status
};

#endif // USER_SETUP_H