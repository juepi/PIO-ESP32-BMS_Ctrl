# Introduction 
This project is intended to be used as an WiFi/MQTT interface to the [Daly Smart BMS Systems](https://www.aliexpress.com/store/4165007) based on my [PlatformIO ESP32 Template](https://github.com/juepi/PIO-ESP32-Template).  

## Local Requirements
Please see [PlatformIO ESP32 Template README](https://github.com/juepi/PIO-ESP32-Template) for details. Obviously you will also need a Daly Smart BMS, i have bought the "small" 40A 4S LiFePo4 version for first testing. You will probably need to do the initial battery setup using the included Bluetooth adapter, as the used [library](https://github.com/maland16/daly-bms-uart) to interface to the BMS lateron does not support setting configuration parameters.

## An important notice on connecting the ESP32 to your BMS
Take caution when connecting the ESP to the UART port of the BMS: the internal electronics of the BMS (including UART port) use "battery ground", whereas other connected devices (i.e. load, charger) use the "power ground". **NEVER EVER** connect these different grounds or magic smoke will leave your electronics! To avoid this, make sure to isolate **anything** coming from or going into the UART port of the Daly BMS. For UART RX/TX, a **ADUM1201** isolator chip should suite well, if you want to use the 3.3V power provided by the BMS to power your ESP, also use an isolating DC/DC converter (like the "B0303S-1W" on ali; **see hint below!**). See [this site](https://cppdig.com/c/esp-bridge-allowing-daly-smart-bms-to-be-used-with-a-sofar-invertercharger-and-others-that-use-sma-canbus-protocol) for some details.  
**SUGGESTION:** as watching a few videos from [the off grid garage](https://www.youtube.com/c/OffGridGarageAustralia) it seems to me that the BMS internal electronics are powered by only a **single cell** of your battery pack (draining ~17mA). This will lead to a unbalanced cell situation by design (probably only relevant for batteries with little capacity though), so i'd recommend you to not power your ESP microcontroller (or any peripherals) from the 3.3V rail coming from the UART connector of the BMS as this would worsen things. Instead, use a small DC/DC converter and power it from the battery pack directly (Battery+ and P- from BMS, where also your charger and load is connected). You have to keep in mind that in an error-situation the BMS will probably cut off the load output, so you have to think of a way to avoid losing the ESP power supply. In my case, i'll be running an AC supply in parallel, but you could also add a supercap in example so that you get the ESP at least to send the error situation to the MQTT broker before shutting off.

## Credits
Thanks to [maland16 and softwarecrash](https://github.com/maland16/daly-bms-uart) for providing a library to interact with the Daly Smart BMS.

## Safety Guide
Keep in mind that you are working with potentially dangerous currents depending on the hardware you use. Take any precautions necessary!

## Status
The code is finished so far, the last (self caused) problem with occasional ESP resets has been solved (hopefully). When i'm in the mood (and have enough spare time), I'll start adding a KiCAD project to the repo to combine all the hardware parts together.

## Insights
- Do not rely on the current reported by the Daly BMS, it is quite inaccurate at low levels. Even worse, **Daly does not recognize charge and discharge currents below 1.1A**, so at small setups like mine, this also means that you cannot rely on the SOC reported by Daly.
- Instead, you could use a [Smart Shunt](https://www.victronenergy.com/battery-monitors/smart-battery-shunt) (expensive solution) or - for small currents - an [INA226](https://www.ti.com/lit/ds/symlink/ina226.pdf) breakout board.
- Firmware-upgrading the ESP rarely breaks communication to the Daly BMS. In that case, you need to re-connect the BT-coin and press the button on it, then reconnect the ESP and it will work again. So it would probably be a good idea to wire the "button" (yellow wire on my original Daly BT cable) to an ESP GPIO if possible.. this is a bit nasty and happens every now and then after flashing. I've tried to send an BMS reset command to the Daly when this error occurs, but as expected this does not cure the fault condition.
- set the "sleep waiting time" on the Daly BMS to **35535** to avoid sleep mode
- ~~Disabling the DISCHARGE MOSFET on the Daly-BMS also seems to break charging for my setup. I use a small BQ24650 based MPPT-PV charger, and i assume that disabling the DISCHARGE FET confuses the charger-IC. atm the charger needs a reset by shorting the PV input daily. :-/~~
- Problem above solved by switching the load through a dedicated SSR (GPIO) and not touching the Daly's Discharge FET. **ADVICE on buying solid state relays:** Do **NOT** buy cheap SSRs on Amazon or Ali! They are all bullshit if not even dangerous! There are obviously good reasons why reasonable SSR cost 50EUR and above at serious electronic stores.. I've tried 2 cheap ones from Ali (dead after < 20 switching cycles) and Amazon (caused 0,75W power loss at 1 (!!!) amp load - was supposed to handle 100amps according to seller!). I finally ended up using an [optocoupler](https://industry.panasonic.eu/de/components/relays/relays/photomos-relays/photomos-ge-2-form/aqw212eh-aqw212eh) with a [P-channel MOSFET](https://www.infineon.com/dgdl/irf5210pbf.pdf?fileId=5546d462533600a4015355e3576b198b) to turn the load on and off, which works perfectly well. 
- Daly BMS will enter sleeping mode when battery/cell reaches configured undervoltage. Serial comm will fail at that stage, but recovers when undervoltage condition resolves and BMS comes up again.

## What the code does
Primarily, the code reads Battery-pack and cell status from the Daly BMS and sends it to your MQTT broker and to a locally attached OLED display at a configurable interval. It automatically switches the load-SSR on if the batteries are fully charged (Daly BMS reports `bms.alarm.levelOneCellVoltageTooHigh`) and off if the batteries are drained (measured battery current by the INA226 reaches 0 and battery voltage less than 12.3V). As mentioned, the SOC reported by the Daly is incorrect, especially when charging / discharging the battery with < 1.1A. For this reason, the firmware maintains a "calculated SOC", which is based on more accurate voltage and current readouts of the INA226.  
Additionally, you can manually switch Daly Charge and Discharge FETs on/off through dedicated MQTT topics, but be warned that this might confuse your charger, i think due to the Low-side switching nature of the Daly BMS.  
Last but not least, you can manually switch the load-SSR through a dedicated MQTT topic.  
To configure the firmware for your needs, see files `user_setup.h` and `mqtt_ota_config.h`, also see [PlatformIO ESP32 Template](https://github.com/juepi/PIO-ESP32-Template) readme (WiFi setup etc.).

# Version History

## v0.1.0
- Initial Commit, only integrated a small 128x32 OLED which will be used to display some status later on

## v0.2.0
- Added Daly BMS library and some code - compiles, but untested (my hardware still not arrived yet)

## v0.3.0
- Communication to Daly BMS works
- INA226 for more accurate power measurements integrated

## v0.4.0
- Code improvements
- Extended MQTT communication
- Added ability to manually set charge / discharge MOSFET states on Daly though MQTT
- solved possible ESP crash/reset after several hours caused by excessive `oled.clear()` calls

## v0.5.0
- Added MQTT-switchable SSR through GPIO5
- minor improvements
- ESP still resetting itself rarely, suspecting unstable power supply (changed supply to 5V instead of 3.3V -> did not help!)

## v1.0.0
- Seems that the resets are fixed now - added some `delay()` to the code to grant CPU time for background WIFI tasks (Doh.)