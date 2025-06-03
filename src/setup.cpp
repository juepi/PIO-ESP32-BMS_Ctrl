/*
 * ESP32 Template
 * Setup Functions
 */
#include "setup.h"

// Define generic global vars
bool JustBooted = true;      // Helper to let you know you're running the first iteration of the main loop()
bool DelayDeepSleep = false; // skips DeepSleep execution in main loop when true
uint32_t UptimeSeconds = 0;  // Uptime counter
// Define WiFi Variables
const char *ssid = WIFI_SSID;
const char *password = WIFI_PSK;
#ifdef BOOT_WIFI_OFF
int NetState = NET_DOWN;
#else
int NetState = NET_UP;
#endif
const int NetFailAction = NET_OUTAGE;
unsigned long NetRecoveryMillis = 0;

// Define MQTT and OTA-update Variables
char message_buff[MQTT_MAX_MSG_SIZE];
bool OTAupdate = false;
bool SentUpdateRequested = false;
bool OtaInProgress = false;
bool OtaIPsetBySketch = false;
bool SentOtaIPtrue = false;
#ifdef READVCC
float VCC = 3.333;
#endif

// Setup WiFi instance
WiFiClient WiFiClt;

// Setup PubSub Client instance
PubSubClient mqttClt(MQTT_BROKER, 1883, MqttCallback, WiFiClt);

#ifdef NTP_CLT
// Define vars for NTP client
const char *NTPServer1 = NTPSRV_1;
#ifdef NTPSRV_2
const char *NTPServer2 = NTPSRV_2;
#endif
const char *time_zone = TIMEZONE;
unsigned int NTPSyncCounter = 0;
struct tm TimeInfo;
time_t EpochTime;
#endif
// Vars for sleep-until function
#ifdef SLEEP_UNTIL
time_t SleepUntilEpoch = 0;
#endif

void hardware_setup()
{
#ifdef READVCC
    // Setup ADC
    analogSetPinAttenuation(VBAT_ADC_PIN, ADC_ATTENUATION);
    analogReadResolution(ADC_RESOLUTION);
#ifdef READ_THROUGH_GPIO
    pinMode(READ_THROUGH_GPIO, OUTPUT);
    digitalWrite(READ_THROUGH_GPIO, HIGH);
#endif
#endif

// Disable all power domains on ESP while in DeepSleep (actually Hibernation)
// wake up only by RTC timer
#ifndef ESP32C6 // TODO for ESP32-C6
#ifdef KEEP_RTC_SLOWMEM
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
#else
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
#endif
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
#endif // ESP32-C6
#ifdef SLEEP_RTC_CLK_8M
#ifndef ESP32C6
    // Enable 8MHz/256 oscillator
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC8M, ESP_PD_OPTION_ON);
    rtc_slow_freq_t rtcSlowFreq = RTC_SLOW_FREQ_8MD256;
    rtc_clk_8m_enable(true, true);
    rtc_clk_slow_freq_set(rtcSlowFreq);
#else
    // Enable 8MHz/256 oscillator
    esp_sleep_pd_config(ESP_PD_DOMAIN_RC_FAST, ESP_PD_OPTION_ON);
    rtc_clk_8m_enable(true);
#endif
#endif
#if defined SLEEP_UNTIL || defined E32_DEEP_SLEEP
#ifndef ESP32C6 // TODO for ESP32-C6
    // Measure clock period of RTC slow clock, add correction and save to RTC register
    uint32_t rtcClkPeriod = (uint32_t)(0.5 + rtc_clk_cal(RTC_CAL_RTC_MUX, 1024));
    uint32_t rtcClkPeriodCalib = rtcClkPeriod * CLK_CORR_FACTOR;
    REG_WRITE(RTC_CNTL_STORE1_REG, rtcClkPeriodCalib);
    delay(10);
#endif
#endif
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
            esp_deep_sleep((uint64_t)DS_DURATION_MIN * 60000000);
#else
            if (NetFailAction == 0)
            {
                ESP.restart();
            }
            else
            {
                DEBUG_PRINTLN("Unable to connect to WiFi, continuing");
                NetState = NET_FAIL;
            }
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
    NetState = NET_UP;
#ifdef ONBOARD_LED
    // WiFi connected - blink once
    ToggleLed(LED, 200, 2);
#endif
}

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

#ifdef NTP_CLT
void ntp_setup()
{
    // Configure NTP client
    sntp_set_time_sync_notification_cb(NTP_Synced_Callback);
    sntp_set_sync_mode(SYNC_MODE);
    sntp_set_sync_interval(SYNC_INTERVAL);
#ifdef NTPSRV_2
    configTzTime(time_zone, NTPServer1, NTPServer2);
#else
    configTzTime(time_zone, NTPServer1);
#endif
    // Run first sync
    getLocalTime(&TimeInfo, 200);
    time(&EpochTime);
}
#endif // NTP_CLT

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

#ifndef BOOT_WIFI_OFF
    // Startup WiFi
    wifi_setup();
    // Setup OTA
    ota_setup();
#ifdef NTP_CLT
    // Setup NTP
    ntp_setup();
#endif
#endif // NDEF BOOT_WIFI_OFF

    // Setup user specific stuff
    user_setup();

#ifdef ONBOARD_LED
    // Signal setup finished
    ToggleLed(LED, 200, 6);
#endif
}
