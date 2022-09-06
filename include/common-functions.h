/*
 *   ESP32 Template
 *   Common Function declarations
 */
#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <Arduino.h>
#include "mqtt-ota-config.h"

// Declare common functions
extern void ToggleLed(int PIN, int WaitTime, int Count);
extern void MqttCallback(char *topic, byte *payload, unsigned int length);
extern bool MqttSubscribe(const char *Topic);
extern bool MqttConnectToBroker();
extern void MqttUpdater();
extern void MqttDelay(uint32_t delayms);
#ifdef OTA_UPDATE
extern bool OTAUpdateHandler();
#endif

#endif // COMMON_FUNCTIONS_H