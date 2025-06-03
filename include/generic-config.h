/*
 *   ESP32 Template
 *   Firmware, DeepSleep and Serial Settings
 */
#ifndef GENERIC_CONFIG_H
#define GENERIC_CONFIG_H

#include <Arduino.h>

//
// Firmware Version Information
//
#define FIRMWARE_NAME "ESP32-BMS-Controller"
#define FIRMWARE_VERSION "2.7.0" // Add firmware version info of your custom code here
#define TEMPLATE_VERSION "1.4.0"

//
// Serial Output configuration
//
#define BAUD_RATE 115200
#ifdef SERIAL_OUT
#define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
#define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#define DEBUG_PRINTLN(...)
#endif

//
// Handle #define dependencies
//
#ifdef BOOT_WIFI_OFF
#undef WAIT_FOR_SUBSCRIPTIONS
#undef NET_OUTAGE // to avoid compiler warning
#define NET_OUTAGE 1
#endif
#ifdef SLEEP_UNTIL
#undef NTP_CLT // to avoid compiler warning
#define NTP_CLT
#endif

#endif // GENERIC_CONFIG_H