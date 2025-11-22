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

// Function to connect to MQTT Broker and subscribe to Topics
bool MqttConnectToBroker()
{
    bool RetVal = false;
    int ConnAttempt = 0;
    // Try to connect x times, then return error
    while (ConnAttempt < MAXCONNATTEMPTS)
    {
        DEBUG_PRINT("Connecting to MQTT broker..");
        if (mqttClt.connect(MQTT_CLTNAME))
        {
            DEBUG_PRINTLN("connected");
            // Reset subscribed/received Topics counters
            int SubscribedTopics = 0;
            for (int i = 0; i < SubscribedTopicCnt; i++)
            {
                MqttSubscriptions[i].Subscribed = false;
                MqttSubscriptions[i].MsgRcvd = 0;
            }
            // Subscribe to all configured Topics
            while ((SubscribedTopics < SubscribedTopicCnt) && mqttClt.connected())
            {
                for (int i = 0; i < SubscribedTopicCnt; i++)
                {
                    // Make sure broker is still connected to avoid looping endlessly
                    if (!mqttClt.connected())
                    {
                        DEBUG_PRINTLN("Lost connection while subscribing to topics, reconnecting!");
                        break;
                    }
                    if (!MqttSubscriptions[i].Subscribed)
                    {
                        if (mqttClt.subscribe(MqttSubscriptions[i].Topic, SUB_QOS))
                        {
                            MqttSubscriptions[i].Subscribed = true;
                            SubscribedTopics++;
                            yield();
                        }
                    }
                }
            }
            if (SubscribedTopics == SubscribedTopicCnt)
            {
                // All done
                mqttClt.loop();
                delay(100);
                RetVal = true;
                break;
            }
            else
            {
                DEBUG_PRINTLN("Something went wrong, restarting..");
                delay(1000);
                ConnAttempt++;
            }
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
            // have retained messages and no one posts a message (disable in platformio.ini)
            NetState = NET_UP;
#ifdef WAIT_FOR_SUBSCRIPTIONS
            // ATTN: only try for MAX_TOP_RCV_ATTEMPTS then end according to NETFAILACTION
            DEBUG_PRINT("Waiting for messages from subscribed topics..");
            int TopicRcvAttempts = 0;
            bool MissingTopics = true;
            while (TopicRcvAttempts < MAX_TOP_RCV_ATTEMPTS)
            {
                MissingTopics = false;
                for (int i = 0; i < SubscribedTopicCnt; i++)
                {
                    if (MqttSubscriptions[i].MsgRcvd == 0)
                    {
                        MissingTopics = true;
                        break;
                    }
                }
                if (MissingTopics)
                {
                    DEBUG_PRINT(".:T!:.");
                    TopicRcvAttempts++;
                    if (!mqttClt.loop())
                    {
                        DEBUG_PRINTLN("  Lost connection to broker while waiting for topics, reconnecting.");
                        MqttConnectToBroker();
                    }
#ifdef ONBOARD_LED
                    ToggleLed(LED, 50, 2);
#else
                    delay(100);
#endif
                }
                else
                {
                    DEBUG_PRINTLN("");
                    DEBUG_PRINTLN("Messages for all subscribed topics received.");
                    break;
                }
            }
            if (MissingTopics)
            {
                if (NetFailAction == 0)
                {
#ifdef E32_DEEP_SLEEP
                    esp_deep_sleep((uint64_t)DS_DURATION_MIN * 60000000);
#else
                    ESP.restart();
#endif
                }
                else
                {
                    DEBUG_PRINTLN("Unable fetch messages for all subscribed topics, continuing offline");
                    NetState = NET_FAIL;
                }
            }
#endif // WAIT_FOR_SUBSCRIPTIONS
            DEBUG_PRINTLN("");
            DEBUG_PRINTLN("Messages for all subscribed topics received.");
        }
        else
        {
            DEBUG_PRINTLN("Unable to connect to MQTT broker.");
#ifdef ONBOARD_LED
            ToggleLed(LED, 100, 4);
#endif
            if (NetFailAction == 0)
            {
#ifdef E32_DEEP_SLEEP
                esp_deep_sleep((uint64_t)DS_DURATION_MIN * 60000000);
#else
                ESP.restart();
#endif
            }
            else
            {
                DEBUG_PRINTLN("Unable to connect to Broker, continuing offline");
                NetState = NET_FAIL;
            }
        }
    }
    else
    {
        mqttClt.loop();
        yield();
    }
}

// Function to periodically handle MQTT stuff while delaying
// This causes some inaccuracy in the delay of course.
void MqttDelay(uint32_t delayms)
{
    //  Call MqttUpdater every 200ms
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
}

// Function to handle OTA flashing (called in main loop)
// Returns TRUE while OTA-update was requested or in progress
bool OTAUpdateHandler()
{
    // If OTA Firmware Update is requested,
    // only loop through OTA function until finished (or reset by MQTT)
    if (OTAupdate)
    {
        DEBUG_PRINT("OTAupdate in progress, need to wait for all MQTT topics..");
        bool MissingTopics = true;
        while (MissingTopics)
        {
            MissingTopics = false;
            for (int i = 0; i < SubscribedTopicCnt; i++)
            {
                if (MqttSubscriptions[i].MsgRcvd == 0)
                {
                    MissingTopics = true;
                }
            }
            if (MissingTopics)
            {
                DEBUG_PRINT(".:OTA_T!:.");
                mqttClt.loop();
#ifdef ONBOARD_LED
                ToggleLed(LED, 50, 2);
#else
                delay(100);
#endif
            }
        }
        // got all topics, continue
        if (OtaInProgress && !OtaIPsetBySketch)
        {
            DEBUG_PRINTLN("OTA firmware update successful, resuming normal operation..");
            // Make sure that MQTT Broker is connected
            MqttUpdater();
            mqttClt.publish(otaStatus_topic, String(UPDATEOK).c_str(), true);
            mqttClt.publish(ota_topic, String("off").c_str(), true);
            mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
            OTAupdate = false;
            OtaInProgress = false;
            OtaIPsetBySketch = true;
            SentOtaIPtrue = false;
            SentUpdateRequested = false;
            delay(200);
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
            delay(200);
        }
        // call OTA function to receive upload
        ArduinoOTA.handle();
        return true;
    }
    else
    {
        if (SentUpdateRequested)
        {
            DEBUG_PRINTLN("OTA firmware update cancelled by user, cleaning up and rebooting..");
            // Make sure that MQTT Broker is connected
            MqttUpdater();
            mqttClt.publish(otaStatus_topic, String(UPDATECANC).c_str(), true);
            mqttClt.publish(otaInProgress_topic, String("off").c_str(), true);
            delay(200);
            // Reboot after cancelled update
            ESP.restart();
            delay(500);
            return false;
        }
    }
    return false;
}

//
// Functions to enable/disable WiFi
//
// Bring up WiFi and start services
void wifi_up()
{
    wifi_setup();
    if (NetState == NET_UP)
    {
        ota_setup();
#ifdef NTP_CLT
        ntp_setup();
#endif
    }
}

// Disconnect MQTT, stop services and disable WiFi
void wifi_down()
{
    mqttClt.disconnect();
    ArduinoOTA.end();
#ifdef NTP_CLT
    sntp_stop();
#endif
    WiFiClt.stop();
    // bring down radio
    WiFi.disconnect(true, false);
    NetState = NET_DOWN;
}

/*
 * Callback Functions
 * ========================================================================
 */
//
// MQTT Subscription callback function
//
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
    for (int i = 0; i < SubscribedTopicCnt; i++)
    {
        if (String(topic) == String(MqttSubscriptions[i].Topic))
        {
            // Topic found, handle message
            switch (MqttSubscriptions[i].Type)
            {
            case 0:
                // Handle subscription Type BOOL
                if (msgString == "on")
                {
                    *MqttSubscriptions[i].BoolPtr = true;
                    MqttSubscriptions[i].MsgRcvd++;
                }
                else if (msgString == "off")
                {
                    *MqttSubscriptions[i].BoolPtr = false;
                    MqttSubscriptions[i].MsgRcvd++;
                }
                else
                {
                    DEBUG_PRINTLN("MQTT: ERROR: Fetched invalid BOOL for topic [" + String(topic) + "]: " + String(msgString));
                }
                break;
            case 1:
                // Handle subscription of type INTEGER
                *MqttSubscriptions[i].IntPtr = (int)msgString.toInt();
                MqttSubscriptions[i].MsgRcvd++;
                break;
            case 2:
                // Handle subscriptions of type FLOAT
                *MqttSubscriptions[i].FloatPtr = msgString.toFloat();
                MqttSubscriptions[i].MsgRcvd++;
                break;
            case 3:
                // Handle subscriptions of type time_t (message decoded as hex!)
                *MqttSubscriptions[i].TimePtr = (time_t)strtol(msgString.c_str(), NULL, 16);
                MqttSubscriptions[i].MsgRcvd++;
                break;
            case 4:
                // Handle subscriptions of type string (copy from message_buff)
                strcpy(MqttSubscriptions[i].stringPtr, message_buff);
                MqttSubscriptions[i].MsgRcvd++;
                break;
            }
        }
    }
}

#ifdef NTP_CLT
// Callback function (gets called when time adjusts via NTP)
void NTP_Synced_Callback(struct timeval *t)
{
    // Update global time-synced flag
    NTPSyncCounter++;
}
#endif // NTP_CLT

/**
 * C++ Classes
 * =======================================================================
 */

/**
 * Moving Average Class
 * Constructor: Initializes the class and reserves memory.
 * @param capacity The maximum number of values (window size).
 */
MovingAverage::MovingAverage(size_t capacity)
    : capacity_(capacity), count_(0), next_index_(0), sum_(0.0f) // Using 0.0f for float literal
{
    // Reserves space for the values in the vector.
    if (capacity_ > 0)
    {
        // Resizes the vector to the capacity and initializes elements to 0.0f.
        values_.resize(capacity_, 0.0f);
    }
}

/**
 * Adds a new measurement value and updates the average.
 * Uses circular buffer logic and the running sum.
 * @param new_value The new measurement value.
 */
void MovingAverage::addValue(float new_value)
{
    if (capacity_ == 0)
    {
        return;
    }

    // 1. Remove the oldest value from the sum if capacity has been reached
    if (count_ == capacity_)
    {
        // Remove the value that is about to be overwritten
        sum_ -= values_[next_index_];
    }
    else
    {
        // Capacity not reached yet, increment count
        count_++;
    }

    // 2. Store the new value in the vector (overwrites the old value if buffer is full)
    values_[next_index_] = new_value;

    // 3. Add the new value to the sum
    sum_ += new_value;

    // 4. Update the index for the next value (circular buffer logic)
    next_index_ = (next_index_ + 1) % capacity_;
}

/**
 * Calculates and returns the current moving average.
 * @return The calculated average (float).
 */
float MovingAverage::getAverage() const
{
    if (count_ == 0)
    {
        return 0.0f;
    }
    // Uses the stored sum for O(1) access time.
    // Explicitly cast count_ to float for accurate division
    return sum_ / (float)count_;
}