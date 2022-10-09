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
extern void oled_sys_stat();
extern void oled_bms_stat();
extern void oled_INA_stat();

// Declare global user specific objects
// extern abc xyz;
extern SSD1306AsciiWire oled;
extern Daly_BMS_UART bms;
extern INA226 ina;

// Global user vars
extern bool BMSresponding;
extern bool INA_avail;
extern float INA_Calc_Ws;
extern float INA_Max_Ws;
extern float INA_V;
extern float INA_I;
extern float INA_P;
extern float INA_SOC;
extern float INA_Calc_Ws;
extern bool EnableDisplay;
extern uint32_t LastDisplayChange;
extern uint32_t UptimeSeconds;
extern unsigned long oldMillis;
extern int DataSetDisplayed;

// I2C Pins
#define I2C_SCL 39
#define I2C_SDA 37

// User Button (use internal Pulldown)
// HIGH level -> enable display
#define BUT1 7

// UART Connection to BMS (defaults to GPIO17 for TXD and GPIO18 for RXD on ESP32-S2?)
#define DALY_UART Serial1

// OLED Settings
#define OLED_ADDRESS 0x3C
// Switch displayed DataSets every CHANGE_AFTER_SEC seconds
#define CHANGE_AFTER_SEC 5

// INA226 wattmeter settings
#define INA_ADDRESS 0x40
#define INA_SHUNT 0.002 // 2mOhm shunt
#define INA_MAX_I 5     // max. expected current 5A

// Battery settings
#define BAT_ESTIMATED_WS 55000 // rough Ws (Watt seconds) of the connected battery; better use 20% lower value than what you think the battery has

// MQTT Topics for BMS Controlling and Monitoring
#define t_DSOC TOPTREE "DalySoC"           // BMS Battery State of Charge - publish only
#define t_DV TOPTREE "DalyPackV"           // BMS Battery Voltage - publish only
#define t_DI TOPTREE "DalyBatI"            // BMS Battery Current - publish only (BULLSHIT, only shows currents > 1.1A!)
#define t_DV_C1 TOPTREE "Daly_Cell1V"      // Cell 1 voltage
#define t_DV_C2 TOPTREE "Daly_Cell2V"      // Cell 2 voltage
#define t_DV_C3 TOPTREE "Daly_Cell3V"      // Cell 3 voltage
#define t_DV_C4 TOPTREE "Daly_Cell4V"      // Cell 4 voltage
#define t_DLoadSw TOPTREE "DalyLoadSw"     // Desired "switch state" for load MOSFETs (on/off) - subscription
#define t_DChargeSw TOPTREE "DalyChargeSw" // Desired "switch state" for charging MOSFETs (on/off) - subscription
#define t_C_BatCap TOPTREE "Calc_BatCap"  // Store the calculated battery capacity on the broker (fetched at ESP startup) - subscription

/* // Class for INA226 related data
class INA_class
{
public:
    struct measure
    {
        // Data read from INA226
        float V;
        float I;
        float P;
    };
    struct calc
    {
        // Calculated data
        float SOC;
        float Ws;
        float Max_Ws;
    };
};
 */
// extern INA_class INADAT;
/* 
struct INA226_Raw
{
    float V;
    float I;
    float P;
};

struct Calculations
{
    float SOC;
    float Ws;
    float Max_Ws;
};

extern Calculations Calc;
extern INA226_Raw INADAT;
 */
#endif // USER_SETUP_H