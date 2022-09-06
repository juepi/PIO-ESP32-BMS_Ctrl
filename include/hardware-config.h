/*
 *   ESP32 Template
 *   Hardware / Board specific Settings
 */
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

// WEMOS Lolin32 Board
#ifdef WEMOS_LOLIN32
// Onboard LED
#define LED 5
// LED is active low
#define LEDON LOW
#define LEDOFF HIGH

// Pin used for reading VCC (ADC1 channel 7, Pin 35)
// ==================================================
// LOLIN32 uses 100k/27k voltage divider (factor 4.7) on board
// DOCS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html
// and: https://randomnerdtutorials.com/esp32-adc-analog-read-arduino-ide/
// BOARD: https://metalab.at/wiki/Wemos_LOLIN32
#define VBAT_ADC_PIN 35
#define VDIV 4.7f
#define ADC_ATTENUATION ADC_ATTEN_DB_0
#define VFULL_SCALE 1.1f
#define ADC_CHAN ADC1_CHANNEL_7
#define ADC_RESOLUTION ADC_WIDTH_BIT_12
#define ADC_MAXVAL 4095

#endif // WEMOS_LOLIN32

// WEMOS S2 Mini (ESP32S2) Board
#ifdef WEMOS_S2MINI
// Onboard LED
#define LED 15
// LED is active high
#define LEDON HIGH
#define LEDOFF LOW

// Pin used for reading VCC (ADC2 channel 7, GPIO 18)
// ==================================================
// This board doesn't seem to support reading VCC out of the box (i think..?)
#undef READVCC

#endif // WEMOS_S2MINI

// Use RTC RAM to store Variables that should survive DeepSleep
// =============================================================
// ATTN: define KEEP_RTC_SLOWMEM or vars will be lost (PowerDomain disabled)
//#define KEEP_RTC_SLOWMEM

#ifdef KEEP_RTC_SLOWMEM
extern RTC_DATA_ATTR int SaveMe;
#endif

#endif // HARDWARE_CONFIG_H