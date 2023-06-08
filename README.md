# Introduction 
This project is intended to be used as an WiFi/MQTT interface to the [Daly Smart BMS Systems](https://www.aliexpress.com/store/4165007) and Victron VE.Direct devices based on my PlatformIO ESP32 Template.  
Primarily, the code reads status data from the Daly BMS, Victron VE.Direct PV charger and SmartShunt and sends it to your MQTT broker at a configurable interval. It automatically switches the load-SSR on if the batteries are fully charged (2 configurable SOC limits) and off if the batteries are drained (also configureable SOC limit). Load-SSR can also be switched manually by setting the corresponding MQTT topic to `on` or `off`. The primary source for the battery SOC is the **Victron SmartShunt**.  
Last but not least, the firmware also allows you to handle an external active balancer through an additional SSR as recommended by [Andy](https://www.youtube.com/watch?v=yPmwrPOwC3g). However, i have added a second condition beside the 3.4V cell voltage: we're only enabling the active balancer at a configurable mimimum solar power threshold. This ensures that balancing only kicks in when the battery is reasonably charging.  
To configure the firmware for your needs, see files `user-config.h` and `mqtt-ota-config.h`, also see [**PlatformIO ESP32 Template**](https://github.com/juepi/PIO-ESP32-Template) readme (WiFi setup etc.).

## Mandatory Hardware Requirements
- 1x [WEMOS S2 Mini](https://www.wemos.cc/en/latest/s2/s2_mini.html) or any other ESP32 should work (adopt `platformio.ini`to your needs)
- 1x [Daly Smart BMS Systems](https://www.aliexpress.com/store/4165007) with UART port
- a Lithium battery fitting your requirements and BMS
- 1x [Victron SmartSolar](https://www.victronenergy.com/solar-charge-controllers/smartsolar-100-30-100-50) charger (or any other charger supporting the text version of the VE.Direct protocol)
- 1x [Victron SmartShunt](https://www.victronenergy.com/battery-monitors/smart-battery-shunt) with the amps required for your setup
- 3x [ADUM1201 UART Isolators](https://aliexpress.com/item/1005003649567525.html) or any other digital isolators you prefer
- 1x [AQW212EH Optocoupler](https://industry.panasonic.eu/de/components/relays/relays/photomos-relays/photomos-ge-2-form/aqw212eh-aqw212eh)
- 1x [Active Balancer](https://aliexpress.com/item/4001176521939.html)
- a local WiFi network with a MQTT broker
- Sunlight. :wink:

## Status
What you are looking at here is the **second major release** of this firmware. If you want to see how it all started or you're interested in the problems I've encountered up to this point, take a look at the [final v1 release readme](https://github.com/juepi/PIO-ESP32-BMS_Ctrl/tree/v1.3.0).  
The hardware part is also finished now, see [KiCad folder](https://github.com/juepi/PIO-ESP32-BMS_Ctrl/tree/main/KiCad).

## Safety Guide
Keep in mind that you are working with potentially dangerous currents depending on the hardware you use. Take any precautions necessary!

## Local Requirements
You need to do the initial battery setup on the Daly BMS using the included Bluetooth adapter and Daly's SmartBMS app, as the used [library](https://github.com/maland16/daly-bms-uart) does not support setting configuration parameters. Victron devices can use the VictronConnect App in parallel to the (read only) serial output on the VE.Direct port.  
An active balancer is highly recommended, as the internal passive balancer of the Daly BMS is useless. [Andy from the Off-Grid Garage](https://www.youtube.com/watch?v=yPmwrPOwC3g) has picked up this topic recently, and i've decided to add the active-balancing control as he suggests it into the firmware. Note that you will need an active balancer that will allow you to start the balancing by shorting 2 pins, like [this one from AliExpress](https://aliexpress.com/item/4001176521939.html). You will need to remove the solder chunk from the pads labeled "Run", this will reveal 2 seperate soldering pads. Solder 2 wires to the pads and connect them to an optocoupler output, in example [AQW212EH](https://industry.panasonic.eu/de/components/relays/relays/photomos-relays/photomos-ge-2-form/aqw212eh-aqw212eh).

## An important notice on connecting the ESP32 to your BMS and VE.Direct
Take caution when connecting the ESP to the UART ports: the internal electronics of the BMS (including UART port) use "battery ground", whereas other connected devices (i.e. load, charger) use the "power ground". **NEVER EVER** connect these different grounds or magic smoke will leave your electronics! To avoid this, make sure to isolate **anything** coming from or going into the UART ports of the Daly BMS and your VE.Direct devices. For UART RX/TX, a **ADUM1201** isolator chip should work well. See [this site](https://cppdig.com/c/esp-bridge-allowing-daly-smart-bms-to-be-used-with-a-sofar-invertercharger-and-others-that-use-sma-canbus-protocol) for some details.  

## Credits
Thanks to [maland16 and softwarecrash](https://github.com/maland16/daly-bms-uart) for providing a library to interact with the Daly Smart BMS.  
Thanks to [cterwilliger](https://github.com/cterwilliger/VeDirectFrameHandler/tree/master) for his Victron VE.Direct frame handler, which has been integrated into this project since v1.2.0. Note that the source is in the lib directory, not from github, as i needed to include [the buffer overflow patch](https://github.com/cterwilliger/VeDirectFrameHandler/pull/10). I have also added a frameCounter which increments when a valid frame is decoded (to ease handling of fresh data) and a function which looks up array index numbers by Name label (required for SmartShunt!).

# Bugs and Workarounds
- Both data frame arrays of the VeDirectFrameHandler (Charger and SmartShunt) keep increasing over time due to transmission/decoding errors (I assume!) leading to new (non-existing) data labels/values that are added to the arrays. VeDirectFrameHandler maxes out at 40 Labels, so this should not lead to any problems (in terms of buffer overflow), not sure how this is possible however as every frame has a checksum. Due to this problem, also garbage values will be decoded every now and then, so I've added additional validity checks when new data was received (especially SoC).
- VeDirectFrameHandler data from the SmartShunt is "sorted" randomly in the veValue/veName arrays, which breaks the "hardcoded" index numbers I used to get the values from the data arrays. I've fixed this by adding a function to the library that allows you to fetch the arrays index number from a given veName tag. I've not encountered this behavior with the SmartSolar charger yet.
- OneWire doesn't seem to work on high-numbered GPIO pins (tested with > 36); using IO4 works fine!
- It seems (in version **v2.3.0**) that a high frequency toggling of SSR3 relay occured at firmware boot - i have not been able to identify the root cause, it seemed to happen only when SSR3 has been enabled automatically at firmware boot. I think this might occur when changing the SSRs "setState" Topic on the broker (and in the sketch) - after setting, the value is re-read by the firmware as soon as the broker sends out the received value. As the dedicated "on firmware boot" handling of the SSR states was unneccessary anyways, i have completely removed it (**v2.3.1**) and added some additional delay when sending the SSR setStates to the broker in addition to publishing the initial "all off" state to the MQTT broker at firmware boot. The error does not seem to occur any more.


# Version History

## v2.0.0
- Initial Commit, removed overhead from v1
- Improved handling of SSRs (manual switching through MQTT)

## v2.0.1
- Added condition to enable balancer on large cell voltage diff
- Removed condition to enable balancer on PV power threshold
- removed balancer "minimum ON duration" timer
- cleaned up serial connection state handling
- added more textual output to `CtrlStatTXT` topic; now publishes info on every action the controller takes (like enable/disable load or balancer)

## v2.0.2
- MQTT topic `Daly_dV` now reports mV
- Sending int instead of float  to MQTT topics for Daly temperature and SOC
- Added error detection for VE.Direct data readouts (SOC)
- Added some currently unused pin defines to match KiCad schematics
- Added additional plausibility check for SoC data from SmartShunt

## v2.0.3
- Added optional OneWire support for DS18B20 temperature sensors (enable in `platformio.ini`) - doesn't work yet

## v2.0.4
- Moved OneWire GPIO to IO4, this one works

## v2.1.0
- Fixed missing VeDirectFrameHandler lib (**see Note below!**), which i've trashed somehow as it seems..; now loaded through `platformio.ini`
- Fixed Hostname limitation of ESP32-Template
- Renamed `user_setup.h` to more suitable `user-config.h`

## v2.1.1
- Added SSR3 and SSR4 initialization (default state OFF)
- Fixed high delays due to OneWire `getTempCByIndex()` calls when no sensors are available
- Added Timeout handling for OneWire sensors
- Improved VE.Direct data error and connection handling
- Bugfix for WiFi hostname setup (compile error for v2.1.0 when using hostname with dash)
- Added SSR3 for manual switching through MQTT
- Added safety shutdown of loads when communication to Daly BMS or Victron SmartShunt runs into timeout

## v2.2.0
- Reworked Load and Balancer SSR handling (now fully configurable through MQTT topics)
- Updated MQTT subscription handling (PIO-ESP32-Template v1.1.0)
- OTA Updates are now mandatory
- Removed MQTT control of Daly BMS FETs (they are a safety feature, you shouldn't mess with them manually)
- Added additional safety features (Balancer alarm mode for high cellDiff, load shutdown for minimum battery voltage)

## v2.2.1
- Re-added periodic publishing of SSR states (basically for plots in FHEM)
- Bugfix on SSR state handling at firmware boot

## v2.3.0
- Added ability to run without WiFi / MQTT broker connection (based on [**PIO-ESP32-Template v1.2.0**](https://github.com/juepi/PIO-ESP32-Template))

## v2.3.1
- Removed unneccesary SSR handling at firmware boot (just let auto-mode do the magic)
- Setting all SSR state topics to "OFF" on the broker at startup to match the actual firmware boot state 
- High frequency toggling of SSR3 state at firmware boot hopefully fixed

# NOTE on missing VeDirectFrameHandler library

I somehow managed to not to include the VeDirectFrameHandler library into previous commits as it was intended. **This affects all versions since adding VE.Direct support in v1.2.0!**  
If you want to build an older release, you can easily fix this by adding my [fork of VeDirectFrameHandler](https://github.com/juepi/VeDirectFrameHandler) into the library section of `platformio.ini`:

```
lib_deps =
    https://github.com/juepi/VeDirectFrameHandler
    ...
```


Have fun,  
Juergen