/*
 *   ESP32 Template
 *   Setup Function declarations
 */
#ifndef SETUP_H
#define SETUP_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include "hardware-config.h"
#include "wifi-config.h"
#include "generic-config.h"
#include "mqtt-ota-config.h"
#include "common-functions.h"
#include "macro-handling.h"
#include "user-config.h"
#include "time-config.h"


// Declare setup functions
extern void wifi_setup();
extern void ota_setup();
extern void hardware_setup();
extern void setup();
#ifdef NTP_CLT
extern void ntp_setup();
#endif

// Declare global objects
extern WiFiClient WiFiClt;
extern PubSubClient mqttClt;

#endif // SETUP_H