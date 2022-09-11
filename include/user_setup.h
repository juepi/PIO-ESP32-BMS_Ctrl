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
//#include <HardwareSerial.h>

// Declare user, setup and main loop functions
extern void user_setup();
extern void user_loop();
extern void oled_sys_stat();
extern void oled_bms_stat();

// Declare global user specific objects
// extern abc xyz;
extern SSD1306AsciiWire oled;
extern Daly_BMS_UART bms;

// Global user vars
extern bool BMSresponding;

// I2C Pins
#define I2C_SCL 39
#define I2C_SDA 37

// UART Connection to BMS (defaults to GPIO17 for TXD and GPIO18 for RXD on ESP32-S2?)
#define DALY_UART Serial1


// OLED Settings
#define OLED_ADDRESS 0x3C

// MQTT Topics for BMS Controlling and Monitoring
#define SOC_topic TOPTREE "BatSoC" // BMS Battery State of Charge - publish only
#define V_topic TOPTREE "BatV" // BMS Battery Voltage - publish only
#define I_topic TOPTREE "BatI" // BMS Battery Current - publish only
#define LoadSw_topic TOPTREE "LoadSwitch" // Desired "switch state" for load (on/off) - subscription
#define ChargeSw_topic TOPTREE "ChargeSwitch" // Desired "switch state" for charging (on/off) - subscription

#endif // USER_SETUP_H