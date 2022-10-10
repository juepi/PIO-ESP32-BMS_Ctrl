/*
 * ESP32 Template
 * ==================
 * User specific function "user_loop" called in main loop
 * User specific funtion "user_setup" called in setup loop
 * Add stuff you want to run here
 */
#include "setup.h"

// Setup OLED instance
SSD1306AsciiWire oled;

// Setup Daly BMS connector instance
Daly_BMS_UART bms(DALY_UART);

// Setup INA226 wattmeter instance (highly recommended! Daly far too inprecise!)
INA226 ina(Wire);

// Declare global user vars
bool BMSresponding = false;
bool INA_avail = false;
bool EnableDisplay = false;
// Power Calculations
// Global Watt-seconds pushed into / taken out of the battery
// will be used to calculate more accurate SOC
// INA226_Raw INADAT;
// INADAT.V = 0;
float INA_V = 0;
float INA_I = 0;
float INA_P = 0;
float INA_SOC = 0;
float INA_Calc_Ws = 0;
// Maximum "seen" Watt-seconds (battery full -> 100% SOC)
float INA_Max_Ws = BAT_ESTIMATED_WS;
// Other helpers
uint32_t LastDisplayChange = 0;
uint32_t LastDataUpdate = 0;
int DataSetDisplayed = 0;
uint32_t UptimeSeconds = 0;
unsigned long oldMillis = 0;

/*
 * User Setup Loop
 * ========================================================================
 */
void user_setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(BUT1, INPUT_PULLDOWN);
  oled.begin(&Adafruit128x32, OLED_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  bms.Init();
  EnableDisplay = digitalRead(BUT1);
  INA_avail = ina.begin(INA_ADDRESS);
  if (INA_avail)
  {
    ina.configure(INA226_AVERAGES_64, INA226_BUS_CONV_TIME_8244US, INA226_SHUNT_CONV_TIME_8244US, INA226_MODE_SHUNT_BUS_CONT);
    ina.calibrate(INA_SHUNT, INA_MAX_I);
    DEBUG_PRINTLN("INA226 initialized.");
  }
  else
  {
    DEBUG_PRINTLN("Failed to initialize INA226!");
  }
}

/*
 * User Main Loop
 * ========================================================================
 */
void user_loop()
{
  // Check if one second has passed and run required actions
  // TODO: get timer working..
  if ((millis() - oldMillis) >= 1000)
  {
    // unsigned long error = millis() - oldMillis; // timing inaccuracy evaluation
    oldMillis = millis();
    UptimeSeconds++;

    // Get data from INA226
    INA_V = ina.readBusVoltage();
    INA_I = ina.readShuntCurrent();
    // ATTN: readBusPower() always reports positive values - not usable for our purpose!
    // INA_P = ina.readBusPower();
    if (abs(INA_I) >= INA_MIN_I)
    {
      INA_P = INA_V * INA_I;
      // Calculations
      INA_Calc_Ws = INA_Calc_Ws + INA_P;
      if (INA_Calc_Ws > INA_Max_Ws)
      {
        // We've got a new max. Ws value for the battery, remember it
        INA_Max_Ws = INA_Calc_Ws;
      }
      INA_SOC = (INA_Calc_Ws / INA_Max_Ws) * 100;
    }
    else
    {
      INA_I = 0;
      INA_P = 0;
    }

    // Update data from BMS
    BMSresponding = bms.update();

    // Display enabled?
    EnableDisplay = (bool)digitalRead(BUT1);
    // DEBUG_PRINTLN("Uptime " + String(UptimeSeconds));
    // DEBUG_PRINTLN("Millis-Diff: " + String(error));
  }

  // Publish data to MQTT
  if ((UptimeSeconds - LastDataUpdate) >= DATA_UPDATE_INTERVAL && mqttClt.connected())
  {
    // Daly BMS Data
    mqttClt.publish(t_DSOC, String(bms.get.packSOC, 1).c_str(), true);
    mqttClt.publish(t_DV, String(bms.get.packVoltage, 2).c_str(), true);
    mqttClt.publish(t_DdV, String((bms.get.cellDiff / 1000), 3).c_str(), true);
    mqttClt.publish(t_DI, String(bms.get.packCurrent, 2).c_str(), true);
    mqttClt.publish(t_DV_C1, String((bms.get.cellVmV[0] / 1000), 3).c_str(), true);
    mqttClt.publish(t_DV_C2, String((bms.get.cellVmV[1] / 1000), 3).c_str(), true);
    mqttClt.publish(t_DV_C3, String((bms.get.cellVmV[2] / 1000), 3).c_str(), true);
    mqttClt.publish(t_DV_C4, String((bms.get.cellVmV[3] / 1000), 3).c_str(), true);
    mqttClt.publish(t_DLSw, String(bms.get.disChargeFetState).c_str(), true);
    mqttClt.publish(t_DCSw, String(bms.get.chargeFetState).c_str(), true);
    mqttClt.publish(t_DTemp, String(bms.get.tempAverage).c_str(), true);

    // INA226 and calculated data
    mqttClt.publish(t_IV, String(INA_V, 2).c_str(), true);
    mqttClt.publish(t_II, String(INA_I, 2).c_str(), true);
    mqttClt.publish(t_IP, String(INA_P, 1).c_str(), true);
    mqttClt.publish(t_C_SOC, String(INA_SOC, 1).c_str(), true);
    mqttClt.publish(t_C_MaxWh, String((INA_Max_Ws / 3600), 0).c_str(), true);
    mqttClt.publish(t_C_Wh, String((INA_Calc_Ws / 3600), 0).c_str(), true);

    LastDataUpdate = UptimeSeconds;
  }
  // Update display
  if (EnableDisplay)
  {
    if ((UptimeSeconds - LastDisplayChange) >= DISPLAY_REFRESH_INTERVAL)
    {
      // Change displayed Dataset
      switch (DataSetDisplayed)
      {
      case 0:
        oled_bms_stat();
        break;
      case 1:
        oled_INA_stat();
        break;
      case 2:
        oled_sys_stat();
        break;
      }
      LastDisplayChange = UptimeSeconds;
    }
  }
  else
  {
    oled.clear();
  }

#ifdef ONBOARD_LED
  // Toggle LED at each loop
  ToggleLed(LED, 100, 4);
#endif
}

// OLED Display functions
// small OLED lines: 21 characters per line
// large (set2X) OLED Lines: 11 characters per line
void oled_sys_stat()
{
  oled.clear();
  oled.set1X();
  if (WiFi.isConnected())
  {
    oled.println("WiFi: OK");
  }
  else
  {
    oled.println("WiFi: FAIL");
  }
  if (mqttClt.connected())
  {
    oled.println("MQTT: OK");
  }
  else
  {
    oled.println("MQTT: FAIL");
  }
  if (BMSresponding)
  {
    oled.println("BMS: OK");
  }
  else
  {
    oled.println("BMS: FAIL");
  }
  if (INA_avail)
  {
    oled.println("INA226: OK");
  }
  else
  {
    oled.println("INA226: FAIL");
  }
  DataSetDisplayed = 0;
}

void oled_bms_stat()
{
  oled.clear();
  oled.set1X();
  oled.println("Daly SoC: " + String(bms.get.packSOC, 1));
  oled.println("Daly Vbat: " + String(bms.get.packVoltage, 2));
  oled.println("Daly Ibat: " + String(bms.get.packCurrent, 2));
  oled.println("Daly Vdiff: " + String((bms.get.cellDiff / 1000), 3));
  DataSetDisplayed = 1;
}

void oled_INA_stat()
{
  oled.clear();
  oled.set1X();
  oled.println("Calc SoC: " + String(INA_SOC, 1));
  oled.println("INA Vbat: " + String(INA_V, 2));
  oled.println("INA Ibat: " + String(INA_I, 2));
  oled.println("INA PWR:  " + String(INA_P, 1));
  DataSetDisplayed = 2;
}
