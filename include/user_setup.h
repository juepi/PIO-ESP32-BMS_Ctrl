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
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <daly-bms-uart.h>
#include <INA226.h>
#ifdef VEDIR_CHRG
#include <VeDirectFrameHandler.h>
#include <SoftwareSerial.h>
#endif // VEDIR_CHRG

//
// Declare user_setup, user_loop and custom global functions
//
extern void user_setup();
extern void user_loop();

//
// Declare global user specific objects
//
extern SSD1306AsciiWire oled;
extern Daly_BMS_UART bms;
extern INA226 ina;
#ifdef VEDIR_CHRG
extern SoftwareSerial VEDSer_Chrg1;
extern VeDirectFrameHandler VED_Chrg1;
#endif // VEDIR_CHRG
#ifdef VEDIR_SHUNT
extern SoftwareSerial VEDSer_Shnt;
extern VeDirectFrameHandler VED_Shnt;
#endif // VEDIR_SHUNT

//
// Global user vars
//
extern bool INA_avail; // INA226 successfully initialized?
// Set by MQTT topics
extern int Ctrl_CSw;   // Desired Daly MOSFET switch states, either on, off or dnc (do not change)
extern int Ctrl_LSw;   // if set to on/off, will be set ONCE by the ESP, then reset to dnc
extern bool Ctrl_SSR1; // SSR1 switch state (on/off)
extern bool Ctrl_SSR2; // SSR2 switch state (on/off)

//
// I2C settings
//
// Pins
#define I2C_SCL 39
#define I2C_SDA 37

//
// User Button (uses internal Pulldown)
//
#define BUT1 7 // Pull to 3V3 to enable display

//
// User SSR
// HIGH level -> SSR is switched ON
//
#define SSR1 5 // Battery Load Switch
#define SSR2 3 // Active Balancer Enable

//
// UART Connection to Daly BMS
// (defaults to GPIO17 for TXD and GPIO18 for RXD on ESP32-S2)
//
#define DALY_UART Serial1

//
// OLED settings
//
#define OLED_ADDRESS 0x3C
#define DISPLAY_REFRESH_INTERVAL 5 // Switch displayed DataSets every x seconds

//
// INA226 wattmeter settings
//
#define INA_ADDRESS 0x40
#define INA_SHUNT 0.002 // 2mOhm shunt (INA configuration setting)
#define INA_MAX_I 5     // max. expected current 5A (INA configuration setting)
#define INA_MIN_I 0.005 // measured currents below +/-5mA will be discarded

//
// Battery settings
//
#define BAT_DESIGN_CAP 85        // rough Wh (Watt hours) of the connected battery; just a starting value, will be updated during charge/discharge cycles
#define BAT_FULL_V 14.35f        // Full charge voltage (measured by INA226); this is the peak voltage at which your solar charger cuts off charging
#define BAT_EMPTY_V 12.3f        // Battery empty voltage; CSOC will be set to 0, load will be disabled (NOTE: may be set lower! quite high due to my special setup!)
#define BAT_NEARLY_EMPTY_V 13.0f // Battery nearly drained; if INA226 voltage is below that threshold *at firmware boot*, CSOC will be set to 0%

//
// Load settings (SSR1)
//
#define ENABLE_LOAD_CSOC 95     // calculated SOC at which to enable the load (SSR1)
#define DISABLE_LOAD_CSOC 0     // CSOC at which load will be disconnected
#define HIGH_PV_AVG_PWR 30      // If the average charging power is higher than this..
#define HIGH_PV_EN_LOAD_CSOC 80 //.. enable the load at an earlier SOC to avoid wasting PV energy

//
// Active Balancer Settings (SSR2)
//
#define BAL_ON_CELLV 3400    // ENABLE balancer if a cell has reached this voltage level [mV]
#define BAL_ON_MIN_PWRAVG 10 // AND the minimum power average of the last hour is +10W (-> battery charging)
#define BAL_OFF_CELLV 3300   // DISABLE balancer if a cell has fallen below this voltage level [mV]
#define BAL_MIN_ON_DUR 1800  // Minimum duration to keep balancer enabled [s]

//
// Victron VE.Direct settings
//
// Global Settings
#define VED_BAUD 19200  // Baud rate (used for all VE.Direct devices)
#define VED_TIMEOUT 180 // If no data update occurs within this timespan (seconds), connection to VE.Direct device is considered dead

// SmartSolar 75/15 Charger settings (charger #1)
#ifdef VEDIR_CHRG       // Enabled in platformio.ini?
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
#endif // VEDIR_CHRG

// SmartShunt 500 settings
#ifdef VEDIR_SHUNT     // Enabled in platformio.ini?
#define VED_SHNT_RX 35 // RX for SoftwareSerial
#define VED_SHNT_TX 34 // TX for SoftwareSerial (unused)
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

#endif // VEDIR_SHUNT

//
// MQTT Data update interval
//
#define DATA_UPDATE_INTERVAL 120 // Send MQTT data ever x seconds

//
// MQTT Topics for BMS, Load and Balancer Controlling and Monitoring
//
// Publish only
#define t_DSOC TOPTREE "Daly_SOC"          // BMS Battery State of Charge(useless, see current)
#define t_DV TOPTREE "Daly_V"              // BMS Battery Voltage
#define t_DdV TOPTREE "Daly_dV"            // BMS Voltage diff between highest and lowest cell voltage
#define t_DI TOPTREE "Daly_I"              // BMS Battery Current(useless, only shows currents > 1.1A!)
#define t_DV_C1 TOPTREE "Daly_C1V"         // Cell 1 voltage
#define t_DV_C2 TOPTREE "Daly_C2V"         // Cell 2 voltage
#define t_DV_C3 TOPTREE "Daly_C3V"         // Cell 3 voltage
#define t_DV_C4 TOPTREE "Daly_C4V"         // Cell 4 voltage
#define t_DLSw TOPTREE "Daly_LSw"          // Actual "switch state" for load MOSFETs (0/1)
#define t_DCSw TOPTREE "Daly_CSw"          // Actual "switch state" for charging MOSFETs (0/1)
#define t_DTemp TOPTREE "Daly_Temp"        // Temperature sensor of the BMS
#define t_IV TOPTREE "INA_V"               // Battery voltage reported by INA
#define t_II TOPTREE "INA_I"               // Battery current reported by INA
#define t_IP TOPTREE "INA_P"               // Power reported by INA
#define t_C_Wh TOPTREE "Calc_Wh"           // Currently calculated energy stored in the battery
#define t_C_MaxWh TOPTREE "Calc_maxWh"     // Calculated battery capacity
#define t_C_SOC TOPTREE "Calc_SOC"         // Calculated SOC based on INA data ("relative state of charge")
#define t_Ctrl_StatT TOPTREE "CtrlStatTXT" // ESP Controller status text and last reset reason provided through MQTT (basically to check if UART to Daly BMS is OK)
#define t_Ctrl_StatU TOPTREE "CtrlStatUpt" // ESP Controller uptime provided through MQTT
#define t_C_AvgP TOPTREE "Calc_AvgP"       // Calculated 1hr power average
// Subscriptions
#define t_Ctrl_CSw TOPTREE "Ctrl_CSw"   // User-desired state of Daly charging MOSFETs (on/off or dnc for "do not change")
#define t_Ctrl_LSw TOPTREE "Ctrl_LSw"   // User-desired state of Daly discharging MOSFETs (on/off or dnc)
#define t_Ctrl_SSR1 TOPTREE "Ctrl_SSR1" // User-desired state of SSR1 GPIO (on/off)
#define t_Ctrl_SSR2 TOPTREE "Ctrl_SSR2" // User-desired state of SSR2 GPIO (on/off)

//
// Data structures
//
struct INA226_Raw
{
    float V = 0;
    float I = 0;
    // float P; not usable, reports only positive values
};

struct Calculations
{
    float SOC = 100;                      // Calculated SOC (relative SOC) - assume a fully charged battery at firmware start
    float Ws = BAT_DESIGN_CAP * 3600;     // usable Energy stored in battery (counting energy when battery voltage is between BAT_EMPTY_V and BAT_FULL_V)
    float max_Ws = BAT_DESIGN_CAP * 3600; // initial battery design capacity (updated after every charge cycle)
    float P = 0;                          // currently measures power (updated every second)
    float P_Avg_1h = 0;                   // 1h power average (updated every 6 minutes)
    float P_Avg_Arr[10] = {0};            // helpers for power average calculation
    float P_Avg_Prev_Ws = 0;
};

struct VED_Shunt_data
{
    float V = 0; // useful data copied from VE.Direct frame
    float I = 0;
    int P = 0;
    float CE = 0;
    float SOC = 0;
    int TTG = 0;
    int iV = 255; // All i* vars point to the index of the corresponding values in VEdirectFrameHandler .veValue array
    int iI = 255;
    int iP = 255;
    int iCE = 255;
    int iSOC = 255;
    int iTTG = 255;
    int iALARM = 255;
    int iAR = 255;
    unsigned long lastUpdate = 0;  // to verify connection is active
    unsigned long lastPublish = 0; // and data needs to be published
    int ConnStat = 0;              // Connection Status
};

struct VED_Charger_data
{
    int iVB = i_CHRG_LBL_VB; // All i* vars point to the index of the corresponding values in VEdirectFrameHandler .veValue array
    int iIB = i_CHRG_LBL_IB; // for the charger, the data order is always the same, so static values can be used here from defines above
    int iPPV = i_CHRG_LBL_PPV;
    int iCS = i_CHRG_LBL_CS;
    int iMPPT = i_CHRG_LBL_MPPT;
    int iERR = i_CHRG_LBL_ERR;
    int iH20 = i_CHRG_LBL_H20;
    unsigned long lastUpdate = 0;  // to verify connection is active
    unsigned long lastPublish = 0; // and data needs to be published
    int ConnStat = 0;              // Connection Status
};

#endif // USER_SETUP_H