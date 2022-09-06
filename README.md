# Introduction 
This project is intended to be used as an WiFi/MQTT interface to the [Daly Smart BMS Systems](https://www.aliexpress.com/store/4165007) based on my [PlatformIO ESP32 Template](https://github.com/juepi/PIO-ESP32-Template).  

## Local Requirements
Please see [PlatformIO ESP32 Template README](https://github.com/juepi/PIO-ESP32-Template) for details. Obviously you will also need a Daly Smart BMS, i have bought the "small" 40A 4S LiFePo4 version for first testing. You will probably need to do the initial battery setup using the included Bluetooth adapter, as the used [library](https://github.com/maland16/daly-bms-uart) to interface to the BMS lateron does not support setting configuration parameters.

## An important notice on connecting the ESP32 to your BMS
Take caution when connecting the ESP to the UART port of the BMS: the internal electronics of the BMS (including UART port) use "battery ground", whereas other connected devices (i.e. load, charger) use the "power ground". **NEVER EVER** connect these different grounds or magic smoke will leave your electronics! To avoid this, make sure to isolate **anything** coming from or going into the UART port of the Daly BMS. For UART RX/TX, a ADUM1201 isolator chip should suite well, if you want to use the 3.3V power provided by the BMS to power your ESP, also use an isolating DC/DC converter (like the "B0303S-1W" on ali). See [this site](https://cppdig.com/c/esp-bridge-allowing-daly-smart-bms-to-be-used-with-a-sofar-invertercharger-and-others-that-use-sma-canbus-protocol) for some details.

## Safety Guide
Keep in mind that you are working with potentially dangerous currents depending on the hardware you use. Take any precautions necessary!

## Status
This is early work-in-progress, as my Daly-BMS along with all other required parts have not yet arrived. If everything works as expected, a hardware part might follow, that combines all required bits and pieces on a single PCB.

# Version History

## v0.1.0
- Initial Commit, only integrated a small 128x32 OLED which will be used to display some status later on
