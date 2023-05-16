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

// MQTT Broker Settings
//
// MQTT Client name used to subscribe to topics
#define MQTT_CLTNAME TEXTIFY(CLTNAME)
// Maximum connection attempts to MQTT broker before going to sleep
#define MAXCONNATTEMPTS 3
// Message buffer for incoming Data from MQTT subscriptions
extern char message_buff[20];

// Keep an eye on subscribed / received topics
extern unsigned int SubscribedTopics;
extern unsigned int ReceivedTopics;

// MQTT Topic Tree prepended to all topics
// ATTN: Must end with "/"!
#define TOPTREE "HB7/UG/BMS/"

// MQTT Topics and corresponding local vars
// ===========================================
// OTA Client Name
#define OTA_CLTNAME TEXTIFY(CLTNAME)
// OTA Update specific vars
// to start an OTA update on the ESP, you will need to set ota_topic to "on"
// (don't forget to add the "retain" flag, especially if you want a sleeping ESP to enter flash mode at next boot)
#define ota_topic TOPTREE "OTAupdate" // local BOOL, MQTT either "on" or "off"
extern bool OTAupdate;
#define otaStatus_topic TOPTREE "OTAstatus"
// OTAstatus strings sent by sketch
// Waiting for binary upload
#define UPDATEREQ "update_requested"
// Update cancelled before upload
#define UPDATECANC "update_cancelled"
// Update successful
#define UPDATEOK "update_success"
extern bool SentUpdateRequested;
// An additional "external flag" is required to "remind" a freshly running sketch that it was just OTA-flashed..
// during an OTA update, PubSubClient functions do not ru (or cannot access the network)
// so this flag will be set to ON when actually waiting for the OTA update to start
// it will be reset if OtaInProgress and OTAupdate are true (in that case, ESP has most probably just been successfully flashed)
#define otaInProgress_topic TOPTREE "OTAinProgress" // local BOOL, MQTT either "on" or "off"
extern bool OtaInProgress;
extern bool OtaIPsetBySketch;
extern bool SentOtaIPtrue;

//
// Configuration struct for MQTT subscriptions
//
struct MqttSubCfg
{
    const char *Topic; // Topic to subscribe to
    int Type;          // Type of message data received: 0=bool (message "on/off"); 1=int; 2=float
    bool Subscribed;   // true if successfully subscribed to topic
    bool MsgRcvd;      // true if a message has been received for topic
    union              // Pointer to Variable which should be updated with the decoded message (only one applies acc. to "Type")
    {
        bool *BoolPtr;
        int *IntPtr;
        float *FloatPtr;
    };
};

extern const int SubscribedTopicCnt; // Number of elements in MqttSubscriptions array (define in mqtt-subscriptions.cpp)
extern MqttSubCfg MqttSubscriptions[];

#ifdef READVCC
// Topic where VCC will be published (not yet working with ESP32!)
#define vcc_topic TOPTREE "Vbat"
extern float VCC;
#endif

#endif // MQTT_OTA_CONFIG_H