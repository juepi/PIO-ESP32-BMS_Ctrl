/*
 *   ESP32 Template
 *   Hardware / Board specific Settings
 */
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

//
// WEMOS Lolin32 Board
//
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
#define ADC_ATTENUATION ADC_0db
#define VFULL_SCALE 1.1f
#define ADC_CHAN ADC1_CHANNEL_7
#define ADC_RESOLUTION 12
#define ADC_MAXVAL 4095
// if READ_THROUGH_GPIO is defined, VCC will be read through the configured GPIO (must be connected to VBAT_ADC_PIN of course). The GPIO will be configured as output and set HIGH in the hardware_setup routine
// can have an advantage when running on batteries, as the external attenuator only draws current when the ESP is active
#define READ_THROUGH_GPIO 5

#endif // WEMOS_LOLIN32

//
// WEMOS S2 Mini (ESP32S2) Board
//
#ifdef WEMOS_S2MINI
// Onboard LED
#define LED 15
// LED is active high
#define LEDON HIGH
#define LEDOFF LOW

// Pin used for reading VCC (ADC1 channel 2, GPIO3)
// ==================================================
// Documentation: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html#about
//
#define VBAT_ADC_PIN 3
#define VDIV 4.348f               // external attenuator on ESP-Mini-Base (https://github.com/juepi/ESP-Mini-Base); use 1 if no external attenuator is used
#define ADC_ATTENUATION ADC_2_5db // measureable voltage @2.5dB: 0..1050mV; adopt to your needs
#define VFULL_SCALE 1.1312f       // adopt to your hardware to increase accuracy
#define ADC_RESOLUTION 12         // bits
#define ADC_MAXVAL 4095
// if READ_THROUGH_GPIO is defined, VCC will be read through the configured GPIO (must be connected to VBAT_ADC_PIN of course). The GPIO will be configured as output and set HIGH in the hardware_setup routine
// can have an advantage when running on batteries, as the external attenuator only draws current when the ESP is active
#define READ_THROUGH_GPIO 5

#endif // WEMOS_S2MINI

//
// Additional Helpers for ESP-Mini-Base
// see: https://github.com/juepi/ESP-Mini-Base
//
#ifdef ESP_Mini_Base
#define EMB_FET_Q1 1      // Default IO gate Q1 FET
#define EMB_FET_Q2 2      // Default IO gate Q2 FET
#define EMB_ADC 3         // ADC1_2 with external attenuator
#define EMB_FET_Q3 4      // Default IO gate Q3 FET
#define EMB_VCC_MEASURE 5 // Suggested IO for measuring VCC (needs manual wiring to EMB_ADC)
#define EMB_SSR_U1 13     // Default IO for switching SSR U1 (high --> SSR short)
#define EMB_PWS_U2 14     // Default IO for switching supply-switch U2 (high -> output enabled)
#define EMB_SW1 16        // Default IO for Button SW1 (external pullup -> active low!)
#define EMB_LS_U4_1 11    // Levelshifter U4 port 1
#define EMB_LS_U4_2 8     // Levelshifter U4 port 2 (I2C SDA)
#define EMB_LS_U4_3 9     // Levelshifter U4 port 3 (I2C SCL)
#define EMB_LS_U4_4 7     // Levelshifter U4 port 4
#define EMB_LS_U5_1 37    // Levelshifter U5 port 1 (SPI MISO)
#define EMB_LS_U5_2 36    // Levelshifter U5 port 2 (SPI SCK)
#define EMB_LS_U5_3 35    // Levelshifter U5 port 3 (SPI MOSI)
#define EMB_LS_U5_4 33    // Levelshifter U5 port 4
#endif                    // ESP_Mini_Base

//
// ESP32-C6 based boards (Tested with AliExpress SuperMini boards)
//
#ifdef ESP32C6
// Onboard LED
#define LED 15
// Onboard WS2812 based RGB-LED
#define RGBLED 8
// LED is active high
#define LEDON HIGH
#define LEDOFF LOW
// Pin used for reading VCC (GPIO2)
// ==================================================
// Documentation: https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html#about
//
#define VBAT_ADC_PIN A2
#define VDIV 4.348f               // external attenuator on ESP-Mini-Base (https://github.com/juepi/ESP-Mini-Base); use 1 if no external attenuator is used
#define ADC_ATTENUATION ADC_2_5db // measureable voltage @2.5dB: 0..1050mV; adopt to your needs
#define VFULL_SCALE 1.1312f       // adopt to your hardware to increase accuracy
#define ADC_RESOLUTION 12         // bits
#define ADC_MAXVAL 4095
// if READ_THROUGH_GPIO is defined, VCC will be read through the configured GPIO (must be connected to VBAT_ADC_PIN of course). The GPIO will be configured as output and set HIGH in the hardware_setup routine
// can have an advantage when running on batteries, as the external attenuator only draws current when the ESP is active
#define READ_THROUGH_GPIO 5
#endif // ESP32C6

#endif // HARDWARE_CONFIG_H