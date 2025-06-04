/*
 *   ESP32 Template
 *   WiFi Settings
 */
#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "macro-handling.h"

// Set WiFi Sleep Mode
// ====================
// CAUTION: Light sleep might disconnect you from broker during sketch execution!
// Handle with care!
#define WIFISLEEP WIFI_PS_NONE
// #define WIFISLEEP WIFI_PS_MIN_MODEM

// WiFi Connection Timeout (milliseconds)
// See NET_OUTAGE action in platformio.ini on behavior when timeout is reached
#define WIFI_CONNECT_TIMEOUT 15000

// WLAN Network SSID and PSK
// ============================
extern const char *ssid;
extern const char *password;

// Behavior on network or MQTT broker failures
// ============================================
extern const int NetFailAction;         // Behavior on network / MQTT broker outages (defined in platformio.ini)
extern int NetState;                    // Global int defining network status (Up/down/failure)
extern unsigned long NetRecoveryMillis; // store MCU "uptime" of the last network recovery (informational only)
#define NET_RECONNECT_INTERVAL 60000    // try to reconnect to WiFi / Broker every minute at Netstate failure

// Defines for NetStates
#define NET_UP 0   // WiFi and MQTT broker connected
#define NET_DOWN 1 // WiFi disabled
#define NET_FAIL 2 // WiFi or MQTT broker failure

// DHCP Hostname to report
#define WIFI_DHCPNAME TEXTIFY(CLTNAME)

#endif // WIFI_CONFIG_H