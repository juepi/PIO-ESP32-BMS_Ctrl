/*
 * ESP32 Template
 * Setup Functions
 */
#include "setup.h"

// Define WiFi Variables
const char *ssid = WIFI_SSID;
const char *password = WIFI_PSK;

// Define MQTT and OTA-update Variables
char message_buff[20];
#ifdef OTA_UPDATE
bool OTAupdate = false;
bool SentUpdateRequested = false;
bool OtaInProgress = false;
bool OtaIPsetBySketch = false;
bool SentOtaIPtrue = false;
#endif
#ifdef READVCC
float VCC = 3.333;
#endif
unsigned int SubscribedTopics = 0;
unsigned int ReceivedTopics = 0;

// Variables that should be saved during DeepSleep
#ifdef KEEP_RTC_SLOWMEM
RTC_DATA_ATTR int SaveMe = 0;
#endif

// Setup WiFi instance
WiFiClient WiFiClt;

// Setup PubSub Client instance
PubSubClient mqttClt(MQTT_BROKER, 1883, MqttCallback, WiFiClt);

void hardware_setup()
{
#ifdef READVCC
    // Setup ADC
    adc1_config_channel_atten(ADC_CHAN, ADC_ATTENUATION);
    adc1_config_width(ADC_RESOLUTION);
#endif

// Disable all power domains on ESP while in DeepSleep (actually Hibernation)
// wake up only by RTC
#ifndef KEEP_RTC_SLOWMEM
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
#endif
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
}

void wifi_setup()
{
    // Set WiFi (Modem) Sleep Mode
    WiFi.setSleep(WIFISLEEP);

    // Set WiFi Hostname
    WiFi.setHostname(WIFI_DHCPNAME);

    // Connect to WiFi network
    DEBUG_PRINTLN();
    DEBUG_PRINTLN("Connecting to " + String(ssid));
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(ssid, password);
    unsigned long end_connect = millis() + WIFI_CONNECT_TIMEOUT;
    while (!WiFi.isConnected())
    {
        if (millis() >= end_connect)
        {
            DEBUG_PRINTLN("");
            DEBUG_PRINTLN("Failed to connect to " + String(ssid));
#ifdef ONBOARD_LED
            ToggleLed(LED, 1000, 4);
#endif
#ifdef E32_DEEP_SLEEP
            DEBUG_PRINTLN("Good night for " + String(DS_DURATION_MIN) + " minutes.");
            ESP.deepSleep(DS_DURATION_MIN * 60000000);
            delay(3000);
#else
            ESP.restart();
#endif
        }
        delay(500);
        DEBUG_PRINT(".:W!:.");
    }
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("WiFi connected");
    DEBUG_PRINT("Device IP Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    DEBUG_PRINT("DHCP Hostname: ");
    DEBUG_PRINTLN(WIFI_DHCPNAME);
#ifdef ONBOARD_LED
    // WiFi connected - blink once
    ToggleLed(LED, 200, 2);
#endif
}

#ifdef OTA_UPDATE
void ota_setup()
{
    // Setup OTA Updates
    // ATTENTION: calling MQTT Publish function inside ArduinoOTA functions MIGHT NOT WORK!
    ArduinoOTA.setHostname(OTA_CLTNAME);
    ArduinoOTA.setPassword(OTA_PWD);
    ArduinoOTA.onStart([]()
                       {
        String type;
        if (ArduinoOTA.getCommand() == U_FLASH)
        {
            type = "sketch";
        }
        else
        { // U_SPIFFS
            type = "filesystem";
        } });
    ArduinoOTA.onEnd([]()
                     {
#ifdef ONBOARD_LED
                         ToggleLed(LED, 200, 4);
#else
                         // ATTENTION: calling MQTT Publish function here does NOT WORK!
                         delay(200);
#endif
                     });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
                          {
        int percentComplete = (progress / (total / 100));
        if (percentComplete == 100)
        {
            DEBUG_PRINTLN("Upload complete.");
            delay(500);
        } });
    ArduinoOTA.onError([](ota_error_t error)
                       {
        DEBUG_PRINTLN("Error: " + String(error));
        delay(500); });
    ArduinoOTA.begin();
}
#endif

/*
 * Setup
 * ========================================================================
 */
void setup()
{
// start serial port and digital Outputs
#ifdef SERIAL_OUT
    Serial.begin(BAUD_RATE);
#endif
    DEBUG_PRINTLN();
    DEBUG_PRINTLN(FIRMWARE_NAME);
    DEBUG_PRINTLN(FIRMWARE_VERSION);
#ifdef ONBOARD_LED
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LEDOFF);
#endif

    // hardware specific setup
    hardware_setup();

    // Startup WiFi
    wifi_setup();

    // Setup OTA
#ifdef OTA_UPDATE
    ota_setup();
#endif

    // Setup user specific stuff
    user_setup();

#ifdef ONBOARD_LED
    // Signal setup finished
    ToggleLed(LED, 200, 6);
#endif
}
