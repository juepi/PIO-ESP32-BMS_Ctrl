/*
 *   ESP32 Template
 *   MQTT and OTA-Flash Settings
 */
#ifndef MQTT_OTA_CONFIG_H
#define MQTT_OTA_CONFIG_H

#include <Arduino.h>
#include <PubSubClient.h>
#include "wifi-config.h"
#include "macro-handling.h"
#include "user-config.h"

//
// MQTT Broker Settings
//
// MQTT Client name used to subscribe to topics
#define MQTT_CLTNAME TEXTIFY(CLTNAME)
// Maximum connection attempts to MQTT broker before going to sleep
#define MAXCONNATTEMPTS 3
#ifdef WAIT_FOR_SUBSCRIPTIONS
// Maximum retry attempts to receive messages for all subscribed topics; ESP will continue according to NET_OUTAGE setting afterwards
// default setting of 300 should try for ~30sec to fetch messages for all subscribed topics
#define MAX_TOP_RCV_ATTEMPTS 300
#endif
// Message buffer for incoming Data from MQTT subscriptions
// increase if you receive larger messages for subscribed topics
// alternatively defined in user-config.h
#ifndef MQTT_MAX_MSG_SIZE
#define MQTT_MAX_MSG_SIZE 20
#endif
extern char message_buff[MQTT_MAX_MSG_SIZE];

// MQTT Topic Tree prepended to all topics
// ATTN: Must end with "/"!
// alternatively defined in user-config.h
#ifndef TOPTREE
#define TOPTREE "HB7/Test/"
#endif

// Default interval to publish MQTT data in seconds (currently used for VCC if enabled)
#define MQTT_PUB_INTERVAL 900

// Default QoS for MQTT subscriptions (see platformio.ini)
#ifndef SUB_QOS
#define SUB_QOS 0
#endif

//
// OTA-Update MQTT Topics and corresponding global vars
//
// OTA Client Name
#define OTA_CLTNAME TEXTIFY(CLTNAME)
// OTA Update specific vars
// to start an OTA update on the ESP, you will need to set ota_topic to "on"
// (don't forget to add the "retain" flag, especially if you want a sleeping ESP to enter flash mode at next boot)
#define ota_topic TOPTREE "OTAupdate" // local BOOL, MQTT either "on" or "off"
extern bool OTAupdate;
#define otaStatus_topic TOPTREE "OTAstatus" // textual OTA-update status sent to broker
// OTAstatus strings sent by sketch
#define UPDATEREQ "update_requested"  // Waiting for binary upload
#define UPDATECANC "update_cancelled" // Update cancelled by user (OTAupdate reset to off befor upload)
#define UPDATEOK "update_success"     // Update successful
// An additional "external flag" is required to "remind" a freshly running sketch that it was just OTA-flashed..
// during an OTA update, PubSubClient functions do not ru (or cannot access the network)
// so this flag will be set to ON when actually waiting for the OTA update to start
// it will be reset if OtaInProgress and OTAupdate are true (in that case, ESP has most probably just been successfully flashed)
#define otaInProgress_topic TOPTREE "OTAinProgress" // local BOOL, MQTT either "on" or "off"
extern bool OtaInProgress;
// Internal helpers
extern bool SentUpdateRequested;
extern bool OtaIPsetBySketch;
extern bool SentOtaIPtrue;

//
// Configuration struct for MQTT subscriptions
//
struct MqttSubCfg
{
    const char *Topic; // Topic to subscribe to
    int Type;          // Type of message data received: 0=bool (message "on/off"); 1=int; 2=float; 3=time_t; 4=string
    bool Subscribed;   // true if successfully subscribed to topic
    uint32_t MsgRcvd;  // true if a message has been received for topic
    union              // Pointer to Variable which should be updated with the decoded message (only one applies acc. to "Type")
    {
        bool *BoolPtr;
        int *IntPtr;
        float *FloatPtr;
        long *TimePtr;
        char *stringPtr;
    };
};

extern const int SubscribedTopicCnt; // Number of elements in MqttSubscriptions array (define in mqtt-subscriptions.cpp)
extern MqttSubCfg MqttSubscriptions[];

//
// "Sleep until" MQTT Topic and corresponding global var
//
#ifdef SLEEP_UNTIL
// "sleep until" given epoch time in sleep_until topic
// the message will be decoded as hex, message may start with "0x", upper and lower case supported
#define sleep_until_topic TOPTREE "SleepUntil"
extern time_t SleepUntilEpoch; // stores time until ESP will sleep
// The following topics are only used for MEASURE_SLEEP_CLOCK_SKEW (publish only)
#define start_sleep_topic TOPTREE "StartSleepEpoch"          // actual epoch when sending ESP to sleep
#define set_sleep_dur_topic TOPTREE "DesiredSleepForSeconds" // expected sleep duration in seconds
#define end_sleep_topic TOPTREE "WakeEpoch"                  // actual epoch after wakeup
#define boot_dur_topic TOPTREE "BootDuration"                // in milli seconds
#endif

//
// Reading battery voltage (VCC when powered from a single LFP cell) Topic and corresponding global var
//
#ifdef READVCC
// Topic where VCC will be published
#define vcc_topic TOPTREE "Vbat"
extern float VCC;
#endif

#endif // MQTT_OTA_CONFIG_H