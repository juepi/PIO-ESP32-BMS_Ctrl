# KiCad Project Files
The hardware part of the BMS-Controller constists of 2 boards, where the small and simple DE-9 Adaptor board is meant to be placed in your battery box. It will only concentrate all required wires on a 9pin D-Sub connector, allowing you to use a common (straight) serial cable to connect to the actual BMS-Controller board with the ESP32 (which is located outside of the metal battery box in my case).  
Schematics should be straight forward and documented. Don't forget to add isolators into the wires to/from VE.Direct devices and your Daly BMS (i have soldered them directly into the wires, so they are not on the PCBs). Both PCBs only use THT parts (no SMT).

## Optional Features
I have added a second AQW212EH opto-coupler as well as 2 additional dual P-channel MOSFET driven power outputs. This hardware is optional. Depending on your load currents, you may install the paralleled MOSFETs (Q4-Q6) or not. I will have to run some load tests to see how much current can be drawn over the PCB, i have no idea honestly, but 10A should be possible.  
Additionally, i have prepared a pin header connector on the DE-9 adaptor board which will let you connect up to 5 [DS18B20](https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf) OneWire temperature sensors in parasitic mode (untested!).  
J4 and J5 can be used if you have an "external use" for the OptoMOS outputs of U2. Be sure not to place any other components on the if you want to use the connector (for J4 -> R2, R6 and R7, and of course Q2 and Q5).

## Production Files
Gerber files are prepared for PCB manufacturing at [jlcpcb.com](https://jlcpcb.com). If you are expecting higher currents on the power outputs, you may want to double up the **copper weight to 2oz** for the BMS-Controller PCB on your order.

# Version History

## v1.1
- Initial commit with some improvements to allow more current on BMS-Controller board (SSR1, SSR3 and SSR4)
