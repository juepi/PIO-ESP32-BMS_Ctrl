/*
 *   ESP32 Template
 *   Time, clock and NTP client Settings
 */
#ifndef TIME_CONFIG_H
#define TIME_CONFIG_H

#if defined SLEEP_UNTIL || defined E32_DEEP_SLEEP
#include <soc/rtc.h>
#include "driver/rtc_io.h"
#endif
#ifdef NTP_CLT
#include "time.h"
#include "esp_sntp.h"
#endif

//
// ESP DeepSleep Configuration
//
// DeepSleep duration in Minutes for E32_DEEP_SLEEP option (sleep after every loop execution)
#define DS_DURATION_MIN 2

//
// NTP Client configuration
//
// Timezone including DST rules (see: https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
#ifdef NTP_CLT
#define TIMEZONE "CET-1CEST,M3.5.0,M10.5.0/3"

// NTP update interval in ms
#define SYNC_INTERVAL 1800000

// Desired sync mode (SNTP_SYNC_MODE_IMMED or SNTP_SYNC_MODE_SMOOTH)
#define SYNC_MODE SNTP_SYNC_MODE_IMMED

// NTP server(s) to sync with (DNS name or IP)
#define NTPSRV_1 "192.168.152.1"
//#define NTPSRV_2 "de.pool.ntp.org"

// Constructor for NTP sync callback function
extern void NTP_Synced_Callback(struct timeval *t);

// Global var increased on each time sync (set in NTP_Synced_Callback)
extern unsigned int NTPSyncCounter;
// Global struct containing system time info
extern struct tm TimeInfo;
// Global var containing system epoch time
extern time_t EpochTime;
// Configuration vars
extern const char* NTPServer1;
#ifdef NTPSRV_2
extern const char* NTPServer2;
#endif
extern const char* time_zone;
#endif //NTP_CLT

//
// RTC slow clock correction settings
//
#if defined SLEEP_UNTIL || defined E32_DEEP_SLEEP
// Correction factor for 150kHz / 8Mhz oscillator to compensate clock skew during ESP sleep
// the default 150kHz oscillator is highly inaccurate and temperature-unstable, be aware that correction probably won't work well!
// TODO: automatically re-calculate factor after sleep and store in RTC_SLOWMEM
// NOTE: Not supported (ignored) for ESP32-C6!
#ifdef SLEEP_RTC_CLK_8M
#define CLK_CORR_FACTOR 1.003170777577f
#else
#define CLK_CORR_FACTOR 0.984157983649f
#endif

// define MEASURE_SLEEP_CLOCK_SKEW to get some time data (MQTT topics, see mqtt-ota-config.h) which allow you to calculate CLK_CORR_FACTOR for your ESP.
// subscribe to "TOPTREE#" on your broker to see the relevant topics. You need to set a (long) /TOPTREE/SleepUntil value (retained) to start the process.
// Time data will not be sent retained, so make sure your mqtt client remains connected to the broker during the whole process, or you may miss the infos from your ESP.

// Calculation: CLK_CORR_FACTOR = CLK_CORR_FACTOR * (WakeEpoch - StartSleepEpoch - BootDuration/1000) / DesiredSleepForSeconds
// ESP sleeps too short, factor needs to be decreased
// ESP sleeps too long, factor needs to be increased
// ATTN: MEASURE_SLEEP_CLOCK_SKEW requires SLEEP_UNTIL option!
// NOTE: Not supported for ESP32-C6!
//#define MEASURE_SLEEP_CLOCK_SKEW
#endif

#endif // TIME_CONFIG_H