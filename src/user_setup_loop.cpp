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

// Setup INA226 wattmeter instance (highly recommended! Daly far too imprecise!)
INA226 ina(Wire);

// Global variable declarations
bool INA_avail = false;
// for MQTT topics
int Ctrl_CSw = 2;
int Ctrl_LSw = 2;
bool Ctrl_SSR1 = false;

/*
 * User Setup Loop
 * ========================================================================
 */
void user_setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(BUT1, INPUT_PULLDOWN);
  pinMode(SSR1, OUTPUT);
  digitalWrite(SSR1, LOW);
  oled.begin(&Adafruit128x32, OLED_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  bms.Init();
  INA_avail = ina.begin(INA_ADDRESS);
  if (INA_avail)
  {
    // Continuously measure and average measurements for a timespan of ~ 0.5sec
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
  //
  // Declare Vars
  //
  static bool BMSresponding = false;
  static bool EnableDisplay = false;
  // Power Calculations / Readings
  static INA226_Raw INADAT;
  static Calculations Calc;
  // Other helpers
  static uint32_t LastDisplayChange = 0;
  static uint32_t LastDataUpdate = 0;
  static int NextDataSet = 0;
  static uint32_t UptimeSeconds = 0;
  static unsigned long oldMillis = 0;
  static const char *Stat_Decoder[] = {"FAIL", "OK"};
  static const char *Bool_Decoder[] = {"off", "on"};
  static bool OledCleared = false;

  //
  // Gather data every second
  //
  if ((millis() - oldMillis) >= 1000)
  {
    // unsigned long error = millis() - oldMillis; // timing inaccuracy evaluation
    oldMillis = millis();
    UptimeSeconds++;

    // Get data from INA226
    INADAT.V = ina.readBusVoltage();
    INADAT.I = ina.readShuntCurrent();
    // ATTN: ina.readBusPower() always reports positive values - not usable for our purpose!
    if (abs(INADAT.I) >= INA_MIN_I)
    {
      // Calculations
      Calc.P = INADAT.V * INADAT.I;
      Calc.Ws = Calc.Ws + Calc.P;
      if (Calc.Ws > Calc.max_Ws)
      {
        // We've got a new max. Ws value for the battery, remember it
        Calc.max_Ws = Calc.Ws;
      }
      else if (Calc.Ws < 0)
      {
        // Energy stored in the battery can never be less than 0
        // this might happen when ESP resets while discharging
        Calc.Ws = 0;
      }

      Calc.SOC = (Calc.Ws / Calc.max_Ws) * 100;
    }
    else
    {
      INADAT.I = 0;
      Calc.P = 0;
    }

    // Update data from BMS
    BMSresponding = bms.update();

    // Display enabled?
    EnableDisplay = (bool)digitalRead(BUT1);
    // DEBUG_PRINTLN("Uptime " + String(UptimeSeconds));
    // DEBUG_PRINTLN("Millis-Diff: " + String(error));
  }

  //
  // Publish data to MQTT broker
  //
  if ((UptimeSeconds - LastDataUpdate) >= DATA_UPDATE_INTERVAL && mqttClt.connected())
  {
    // Daly BMS and Controller Data
    if (BMSresponding)
    {
      mqttClt.publish(t_DSOC, String(bms.get.packSOC, 1).c_str(), true);
      mqttClt.publish(t_DV, String(bms.get.packVoltage, 2).c_str(), true);
      mqttClt.publish(t_DdV, String((bms.get.cellDiff / 1000), 3).c_str(), true);
      mqttClt.publish(t_DI, String(bms.get.packCurrent, 2).c_str(), true);
      mqttClt.publish(t_DV_C1, String((bms.get.cellVmV[0] / 1000), 3).c_str(), true);
      mqttClt.publish(t_DV_C2, String((bms.get.cellVmV[1] / 1000), 3).c_str(), true);
      mqttClt.publish(t_DV_C3, String((bms.get.cellVmV[2] / 1000), 3).c_str(), true);
      mqttClt.publish(t_DV_C4, String((bms.get.cellVmV[3] / 1000), 3).c_str(), true);
      mqttClt.publish(t_DLSw, String(Bool_Decoder[(int)bms.get.disChargeFetState]).c_str(), true);
      mqttClt.publish(t_DCSw, String(Bool_Decoder[(int)bms.get.chargeFetState]).c_str(), true);
      mqttClt.publish(t_DTemp, String(bms.get.tempAverage).c_str(), true);
      mqttClt.publish(t_Ctrl_StatT, String("ok").c_str(), true);
      // Add some delay for WiFi processing
      delay(100);
    }
    else
    {
      mqttClt.publish(t_Ctrl_StatT, String("BMS_Fail").c_str(), true);
    }
    mqttClt.publish(t_Ctrl_StatU, String(UptimeSeconds).c_str(), true);

    // INA226 and calculated data
    mqttClt.publish(t_IV, String(INADAT.V, 2).c_str(), true);
    mqttClt.publish(t_II, String(INADAT.I, 2).c_str(), true);
    mqttClt.publish(t_IP, String(Calc.P, 1).c_str(), true);
    mqttClt.publish(t_C_SOC, String(Calc.SOC, 1).c_str(), true);
    mqttClt.publish(t_C_MaxWh, String((Calc.max_Ws / 3600), 3).c_str(), true);
    mqttClt.publish(t_C_Wh, String((Calc.Ws / 3600), 1).c_str(), true);

    LastDataUpdate = UptimeSeconds;
    // Add some more delay for WiFi processing
    delay(200);
  }

  //
  // Load handling (SSR1)
  //
  // If a Cell has reached its charge limit, enable SSR1 (load)
  if (bms.alarm.levelOneCellVoltageTooHigh || bms.alarm.levelTwoCellVoltageTooHigh)
  {
    Ctrl_SSR1 = true;
    mqttClt.publish(t_Ctrl_SSR1, String("on").c_str(), true);
  }

  // Disable SSR1 when battery voltage is low and no more current running out of the battery
  // Note: "Battery discharged" voltage quite high due to my special setup (AC powered 12VDC supply in parallel)
  if (Ctrl_SSR1 && INADAT.V <= 12.3f && INADAT.I >= -0.1f && INADAT.I <= 0.0f)
  {
    Ctrl_SSR1 = false;
    mqttClt.publish(t_Ctrl_SSR1, String("off").c_str(), true);
    // battery is "empty" now
    Calc.Ws = 0;
  }

  //
  // Handle desired Daly MOSFET charge / discharge switch states
  //
  if (Ctrl_CSw < 2)
  {
    // Set desired Charge Switch state
    switch (Ctrl_CSw)
    {
    case 0:
      bms.setChargeMOS(false);
      break;
    case 1:
      bms.setChargeMOS(true);
      break;
    }
    // state changed, reset to "do not change"
    Ctrl_CSw = 2;
    mqttClt.publish(t_Ctrl_CSw, String("dnc").c_str(), true);
  }
  if (Ctrl_LSw < 2)
  {
    // Set desired Discharge Switch state
    switch (Ctrl_LSw)
    {
    case 0:
      bms.setDischargeMOS(false);
      break;
    case 1:
      bms.setDischargeMOS(true);
      break;
    }
    // state changed, reset to "do not change"
    Ctrl_LSw = 2;
    mqttClt.publish(t_Ctrl_LSw, String("dnc").c_str(), true);
  }

  //
  // Handle SSR1 state
  //
  if (Ctrl_SSR1 != (bool)digitalRead(SSR1))
  {
    if (Ctrl_SSR1)
    {
      digitalWrite(SSR1, HIGH);
    }
    else
    {
      digitalWrite(SSR1, LOW);
    }
  }

  //
  // Update display
  //
  // INFO: 21 characters per line, 4 lines on small font (set1X)
  // 11 characters per line on large font (set2X)
  if (EnableDisplay)
  {
    if ((UptimeSeconds - LastDisplayChange) >= DISPLAY_REFRESH_INTERVAL)
    {
      // Change displayed Dataset
      switch (NextDataSet)
      {
      case 0:
        oled.clear();
        oled.set1X();
        oled.println("WiFi: " + String(Stat_Decoder[(int)WiFi.isConnected()]));
        oled.println("MQTT: " + String(Stat_Decoder[(int)mqttClt.connected()]));
        oled.println("BMS:  " + String(Stat_Decoder[(int)BMSresponding]));
        oled.println("INA:  " + String(Stat_Decoder[(int)INA_avail]));
        NextDataSet++;
        break;
      case 1:
        oled.clear();
        oled.set1X();
        oled.println("Daly SoC: " + String(bms.get.packSOC, 1));
        oled.println("Daly Vbat: " + String(bms.get.packVoltage, 2));
        oled.println("Daly Ibat: " + String(bms.get.packCurrent, 2));
        oled.println("Daly Vdiff: " + String((bms.get.cellDiff / 1000), 3));
        NextDataSet++;
        break;
      case 2:
        oled.clear();
        oled.set1X();
        oled.println("Calc SoC: " + String(Calc.SOC, 1));
        oled.println("INA Vbat: " + String(INADAT.V, 2));
        oled.println("INA Ibat: " + String(INADAT.I, 2));
        oled.println("INA PWR:  " + String(Calc.P, 1));
        NextDataSet = 3;
        break;
      case 3:
        oled.clear();
        oled.set1X();
        oled.println("Charge FET: " + String(Bool_Decoder[(int)bms.get.chargeFetState]));
        oled.println("Disch FET:  " + String(Bool_Decoder[(int)bms.get.disChargeFetState]));
        oled.println("SSR1:       " + String(Bool_Decoder[(int)Ctrl_SSR1]));
        oled.println("Uptime:     " + String(UptimeSeconds));
        NextDataSet = 0;
        break;
      }
      LastDisplayChange = UptimeSeconds;
      OledCleared = false;
    }
  }
  else
  {
    if (!OledCleared)
    {
      oled.clear();
      OledCleared = true;
    }
  }

#ifdef ONBOARD_LED
  // Toggle LED at each loop
  ToggleLed(LED, 100, 4);
#endif
}
