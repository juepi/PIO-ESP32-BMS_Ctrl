/*
 *   ESP32 Template
 *   User specific defines and Function declarations
 */
#ifndef USER_SETUP_H
#define USER_SETUP_H

#include "mqtt-ota-config.h"

// Define required user libraries here
// Don't forget to add them into platformio.ini
#include <Wire.h>
#include <SSD1306Ascii.h>
#include <SSD1306AsciiWire.h>
#include <daly-bms-uart.h>
#include <INA226.h>

// Declare user setup, main and custom functions
extern void user_setup();
extern void user_loop();

// Declare global user specific objects
// extern abc xyz;
extern SSD1306AsciiWire oled;
extern Daly_BMS_UART bms;
extern INA226 ina;

// Global user vars
extern bool INA_avail;
// Set by MQTT topics
// Desired Daly MOSFET switch states, either on, off or dnc (do not change)
// if set to on/off, will be set ONCE by the ESP, then reset to dnc
extern int Ctrl_CSw;
extern int Ctrl_LSw;

// I2C Pins
#define I2C_SCL 39
#define I2C_SDA 37

// User Button (use internal Pulldown)
// HIGH level -> enable display
#define BUT1 7

// UART Connection to BMS (defaults to GPIO17 for TXD and GPIO18 for RXD on ESP32-S2)
#define DALY_UART Serial1

// OLED Settings
#define OLED_ADDRESS 0x3C
// Switch displayed DataSets every x seconds
#define DISPLAY_REFRESH_INTERVAL 5

// INA226 wattmeter settings
#define INA_ADDRESS 0x40
#define INA_SHUNT 0.002 // 2mOhm shunt
#define INA_MAX_I 5     // max. expected current 5A
#define INA_MIN_I 0.005 // measured currents below 5mA will be discarded

// Battery settings
#define BAT_ESTIMATED_WS 150000 // rough Ws (Watt seconds) of the connected battery; use 50% of actual capacity - just a starting value

// MQTT Data update interval
// Send MQTT data ever x seconds
#define DATA_UPDATE_INTERVAL 30

// MQTT Topics for BMS Controlling and Monitoring
// Publish only
#define t_DSOC TOPTREE "Daly_SOC"      // BMS Battery State of Charge(useless, see current)
#define t_DV TOPTREE "Daly_V"          // BMS Battery Voltage
#define t_DdV TOPTREE "Daly_dV"        // BMS Voltage diff between highest and lowest cell voltage
#define t_DI TOPTREE "Daly_I"          // BMS Battery Current(useless, only shows currents > 1.1A!)
#define t_DV_C1 TOPTREE "Daly_C1V"     // Cell 1 voltage
#define t_DV_C2 TOPTREE "Daly_C2V"     // Cell 2 voltage
#define t_DV_C3 TOPTREE "Daly_C3V"     // Cell 3 voltage
#define t_DV_C4 TOPTREE "Daly_C4V"     // Cell 4 voltage
#define t_DLSw TOPTREE "Daly_LSw"      // Actual "switch state" for load MOSFETs (0/1)
#define t_DCSw TOPTREE "Daly_CSw"      // Actual "switch state" for charging MOSFETs (0/1)
#define t_DTemp TOPTREE "Daly_Temp"    // Temperature sensor of the BMS
#define t_IV TOPTREE "INA_V"           // Battery voltage reported by INA
#define t_II TOPTREE "INA_I"           // Battery current reported by INA
#define t_IP TOPTREE "INA_P"           // Power reported by INA
#define t_C_Wh TOPTREE "Calc_Wh"       // Currently calculated energy stored in the battery
#define t_C_SOC TOPTREE "Calc_SOC"     // Calculated SOC based ina INA data
#define t_C_MaxWh TOPTREE "Calc_maxWh" // Store the calculated battery capacity on the broker (not yet a subscription)
#define t_Ctrl_Stat TOPTREE "CtrlStatTXT"   // ESP Controller status text provided through MQTT (basically to check if UART to Daly BMS is OK)
// Subscriptions
#define t_Ctrl_CSw TOPTREE "Ctrl_CSw" // User-desired state of charging MOSFETs (on/off or dnc for "do not change")
#define t_Ctrl_LSw TOPTREE "Ctrl_LSw" // User-desired state of load / discharging MOSFETs (on/off or dnc)

// Data structures
struct INA226_Raw
{
    float V = 0;
    float I = 0;
    // float P; not usable, reports only positive values
};

struct Calculations
{
    float SOC = 0;
    float Ws = 0;
    float max_Ws = BAT_ESTIMATED_WS;
    float P = 0;
};

struct SwitchStateHelper
{
    bool wasSet = false;
    bool prevState = true;
};

#endif // USER_SETUP_H