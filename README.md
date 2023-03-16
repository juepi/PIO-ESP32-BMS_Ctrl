# Introduction 
This project is intended to be used as an WiFi/MQTT interface to the [Daly Smart BMS Systems](https://www.aliexpress.com/store/4165007) and Victron VE.Direct devices based on my PlatformIO ESP32 Template.  
Primarily, the code reads status data from the Daly BMS, Victron VE.Direct PV charger and SmartShunt and sends it to your MQTT broker at a configurable interval. It automatically switches the load-SSR on if the batteries are fully charged (2 configurable SOC limits) and off if the batteries are drained (also configureable SOC limit). Load-SSR can also be switched manually by setting the corresponding MQTT topic to `on` or `off`. The primary source for the battery SOC is the **Victron SmartShunt**.  
Last but not least, the firmware also allows you to handle an external active balancer through an additional SSR as recommended by [Andy](https://www.youtube.com/watch?v=yPmwrPOwC3g). However, i have added a second condition beside the 3.4V cell voltage: we're only enabling the active balancer at a configurable mimimum solar power threshold. This ensures that balancing only kicks in when the battery is reasonably charging.  
To configure the firmware for your needs, see files `user_setup.h` and `mqtt_ota_config.h`, also see [**PlatformIO ESP32 Template**](https://github.com/juepi/PIO-ESP32-Template) readme (WiFi setup etc.).

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
The hardware part is still missing, but as i'm looking forward of finishing a "large" 2.4kWh 8S LFP battery, I'll need to work on this in the forseeable future.

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
- Both data frame arrays of the VeDirectFrameHandler (Charger and SmartShunt) keep increasing over time due to transmission/decoding errors (I assume!) leading to new (non-existing) data labels/values that are added to the arrays. VeDirectFrameHandler maxes out at 40 Labels, so this should not lead to any problems (in terms of buffer overflow), not sure how this is possible however as every frame has a checksum. Due to this problem, i assume  that also garbage values may be encountered (although this has not occured yet), so I've added additional validity checks for the important data.
- VeDirectFrameHandler data from the SmartShunt is "sorted" randomly in the veValue/veName arrays, which breaks the "hardcoded" index numbers I used to get the values from the data arrays. I've fixed this by adding a function to the library that allows you to fetch the arrays index number from a given veName tag. I've not encountered this behavior with the SmartSolar charger yet.


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