/*
 * ESP32 Template
 * Common Functions
 */
#include "setup.h"

// Function to toggle a LED (GPIO pin)
void ToggleLed(int PIN, int WaitTime, int Count)
{
    // Toggle digital output
    for (int i = 0; i < Count; i++)
    {
        digitalWrite(PIN, !digitalRead(PIN));
        delay(WaitTime);
    }
}

// Function to subscribe to MQTT topics
bool MqttSubscribe(const char *Topic)
{
    if (mqttClt.subscribe(Topic))
    {
        DEBUG_PRINTLN("Subscribed to " + String(Topic));
        // Success, update global subscription counter
        SubscribedTopics++;
        mqttClt.loop();
        return true;
    }
    else
    {
        DEBUG_PRINTLN("Failed to subscribe to " + String(Topic));
        delay(100);
        return false;
    }
}

// Function to connect to MQTT Broker and subscribe to Topics
bool MqttConnectToBroker()
{
    // Reset subscribed/received Topics counters
    SubscribedTopics = 0;
    ReceivedTopics = 0;
    bool RetVal = false;
    int ConnAttempt = 0;

    // Try to connect x times, then return error
    while (ConnAttempt < MAXCONNATTEMPTS)
    {
        DEBUG_PRINT("Connecting to MQTT broker..");
        // Attempt to connect
        if (mqttClt.connect(MQTT_CLTNAME))
        {
            DEBUG_PRINTLN("connected");
            RetVal = true;

// Subscribe to Topics
#ifdef OTA_UPDATE
            MqttSubscribe(ota_topic);
            MqttSubscribe(otaInProgress_topic);
#endif // OTA_UPDATE
            delay(200);
            break;
        }
        else
        {
            DEBUG_PRINTLN("failed, rc=" + String(mqttClt.state()));
            DEBUG_PRINTLN("Sleeping 2 seconds..");
            delay(2000);
            ConnAttempt++;
        }
    }
    return RetVal;
}

// Function to handle MQTT stuff (broker connections, subscriptions, updates), called in main loop
void MqttUpdater()
{
    if (!mqttClt.connected())
    {
        if (MqttConnectToBroker())
        {
            // New connection to broker, fetch topics
            // ATTN: will run endlessly if subscribed topics
            // does not have retained messages and no one posts a message
            DEBUG_PRINT("Waiting for topics..");
            while (ReceivedTopics < SubscribedTopics)
            {
                DEBUG_PRINT(".:T!:.");
                mqttClt.loop();
#ifdef ONBOARD_LED
                ToggleLed(LED, 100, 2);
#else
                delay(100);
#endif
            }
            DEBUG_PRINTLN("");
            DEBUG_PRINTLN("All topics received.");
        }
        else
        {
            DEBUG_PRINTLN("Unable to connect to MQTT broker.");
#ifdef ONBOARD_LED
            ToggleLed(LED, 100, 40);
#endif
#ifdef DEEP_SLEEP
            ESP.deepSleep(DS_DURATION_MIN * 60000000);
            delay(3000);
#else
            ESP.restart();
#endif
        }
    }
    else
    {
        mqttClt.loop();
    }
}

// Function to periodically handle MQTT stuff while delaying
// This causes some inaccuracy in the delay of course.
void MqttDelay(uint32_t delayms)
{
    //unsigned long md_start = millis();
    // Call MqttUpdater every 200ms
    int Counter = delayms / 200;
    if (Counter == 0)
    {
        // Delay less than 200ms requested, just run delay and return
        delay(delayms);
        return;
    }
    else
    {
        for (int i = 0; i < Counter; i++)
        {
            MqttUpdater();
            delay(200);
        }
    }
    //unsigned long real_delay = millis() - md_start;
    //DEBUG_PRINTLN("MqttDelay requested: " + String(delayms));
    //DEBUG_PRINTLN("MqttDelay Counter: " + String(Counter));
    //DEBUG_PRINTLN("MqttDelay duration: " + String(real_delay));    
}

// Function to handle OTA flashing (called in main loop)
// Returns TRUE while OTA-update was requested or in progress
#ifdef OTA_UPDATE
bool OTAUpdateHandler()
{
    // If OTA Firmware Update is requested,
    // only loop through OTA function until finished (or reset by MQTT)
    if (OTAupdate)
    {
        if (OtaInProgress && !OtaIPsetBySketch)
        {
            DEBUG_PRINTLN("OTA firmware update successful, resuming normal operation..");
            MqttUpdater();
            mqttClt.publish(otaStatus_topic, String(UPDATEOK).c_str(), true);
            mqttClt.publish(ota_topic, String("off").c_str(), true);
            mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
            OTAupdate = false;
            OtaInProgress = false;
            OtaIPsetBySketch = true;
            SentOtaIPtrue = false;
            SentUpdateRequested = false;
            delay(100);
            return false;
        }
        if (!SentUpdateRequested)
        {
            mqttClt.publish(otaStatus_topic, String(UPDATEREQ).c_str(), true);
            SentUpdateRequested = true;
        }
        DEBUG_PRINTLN("OTA firmware update requested, waiting for upload..");
#ifdef ONBOARD_LED
        // Signal OTA update requested
        ToggleLed(LED, 100, 5);
#endif
        // set MQTT reminder that OTA update was executed
        if (!SentOtaIPtrue)
        {
            DEBUG_PRINTLN("Setting MQTT OTA-update reminder flag on broker..");
            mqttClt.publish(otaInProgress_topic, String("on").c_str(), true);
            OtaInProgress = true;
            SentOtaIPtrue = true;
            OtaIPsetBySketch = true;
            delay(100);
        }
        // call OTA function to receive upload
        ArduinoOTA.handle();
        return true;
    }
    else
    {
        if (SentUpdateRequested)
        {
            DEBUG_PRINTLN("OTA firmware update cancelled by MQTT, resuming normal operation..");
            MqttUpdater();
            mqttClt.publish(otaStatus_topic, String(UPDATECANC).c_str(), true);
            mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
            OtaInProgress = false;
            OtaIPsetBySketch = true;
            SentOtaIPtrue = false;
            SentUpdateRequested = false;
            delay(100);
            return false;
        }
    }
    return false;
}
#endif // OTA_UPDATE

/*
 * Callback Functions
 * ========================================================================
 */
// MQTT Subscription callback function
void MqttCallback(char *topic, byte *payload, unsigned int length)
{
    unsigned int i = 0;
    // create character buffer with ending null terminator (string)
    for (i = 0; i < length; i++)
    {
        message_buff[i] = payload[i];
    }
    message_buff[i] = '\0';
    String msgString = String(message_buff);

    DEBUG_PRINTLN("MQTT: Message arrived [" + String(topic) + "]: " + String(msgString));

// run through topics
#ifdef OTA_UPDATE
    if (String(topic) == ota_topic)
    {
        if (msgString == "on")
        {
            OTAupdate = true;
            ReceivedTopics++;
        }
        else if (msgString == "off")
        {
            OTAupdate = false;
            ReceivedTopics++;
        }
        else
        {
            DEBUG_PRINTLN("MQTT: ERROR: Fetched invalid OTA-Update: " + String(msgString));
            delay(200);
        }
    }
    else if (String(topic) == otaInProgress_topic)
    {
        if (msgString == "on")
        {
            OtaInProgress = true;
            ReceivedTopics++;
        }
        else if (msgString == "off")
        {
            OtaInProgress = false;
            ReceivedTopics++;
        }
        else
        {
            DEBUG_PRINTLN("MQTT: ERROR: Fetched invalid OtaInProgress: " + String(msgString));
            delay(200);
        }
    }
    else
    {
        DEBUG_PRINTLN("ERROR: Unknown topic: " + String(topic));
        DEBUG_PRINTLN("ERROR: Unknown topic value: " + String(msgString));
        delay(200);
    }
#endif // OTA_UPDATE
}
