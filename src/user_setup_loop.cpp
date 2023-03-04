/*
 * ESP32 Template
 * ==================
 * User specific function "user_loop" called in main loop
 * User specific funtion "user_setup" called in setup function
 * Add stuff you want to run here
 */
#include "setup.h"

// Setup OLED instance
SSD1306AsciiWire oled;

// Setup Daly BMS connector instance
Daly_BMS_UART bms(DALY_UART);

// Setup VE.Direct
#ifdef VEDIR_CHRG
SoftwareSerial VEDSer_Chrg1;
VeDirectFrameHandler VED_Chrg1;
#endif
#ifdef VEDIR_SHUNT
SoftwareSerial VEDSer_Shnt;
VeDirectFrameHandler VED_Shnt;
#endif

// Setup INA226 wattmeter instance (highly recommended! Daly far too imprecise!)
INA226 ina(Wire);

// Global variable declarations
bool INA_avail = false;
// for MQTT topics
int Ctrl_CSw = 2;
int Ctrl_LSw = 2;
bool Ctrl_SSR1 = false;
bool Ctrl_SSR2 = false;

/*
 * User Setup Function
 * ========================================================================
 */
void user_setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(BUT1, INPUT_PULLDOWN);
  pinMode(SSR1, OUTPUT);
  pinMode(SSR2, OUTPUT);
  digitalWrite(SSR1, LOW);
  digitalWrite(SSR2, LOW);
  oled.begin(&Adafruit128x32, OLED_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  bms.Init();

#ifdef VEDIR_CHRG
  VEDSer_Chrg1.begin(VED_BAUD, SWSERIAL_8N1, VED_CHRG1_RX, VED_CHRG1_TX, false, 512);
  if (!VEDSer_Chrg1)
  {
    DEBUG_PRINTLN("Failed to setup VEDSer_Chrg1! Make sure RX/TX pins are free to use for SoftwareSerial!");
    while (1)
    {
      delay(500);
    }
  }
  else
  {
    DEBUG_PRINTLN("Initialized VEDSer_Chrg1.");
  }
  // We don't need TX
  VEDSer_Chrg1.enableIntTx(false);
  VEDSer_Chrg1.flush();
#endif

#ifdef VEDIR_SHUNT
  VEDSer_Shnt.begin(VED_BAUD, SWSERIAL_8N1, VED_SHNT_RX, VED_SHNT_TX, false, 512);
  if (!VEDSer_Shnt)
  {
    DEBUG_PRINTLN("Failed to setup VEDSer_Shnt! Make sure RX/TX pins are free to use for SoftwareSerial!");
    while (1)
    {
      delay(500);
    }
  }
  else
  {
    DEBUG_PRINTLN("Initialized VEDSer_Shnt.");
  }
  // We don't need TX
  VEDSer_Shnt.enableIntTx(false);
  VEDSer_Shnt.flush();
#endif

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
  static bool FirstLoop = true;
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
  static uint32_t LastBalancerEnable = 0;
  static short UpdAvgArrIndx = 0;
  static uint32_t LastAvgCalc = 0;
#ifdef VEDIR_CHRG
  static unsigned long VED_Chrg1_FrameSent = 0;
  // Int to Text conversion for Charger data
  static const char *VED_MPPT_Decoder[] = {"Off", "V_I_Lim", "Active"};
  static const char *VED_CS_Decoder[] = {"Off", "na", "Fault", "Bulk", "Absorption", "Float"};
  static const char *VED_ConnStat_Decoder[] = {"Startup", "Ok", "Timeout"};
  static VED_Charger_data VCHRG1;
#endif
#ifdef VEDIR_SHUNT
  static unsigned long VED_Shnt_FrameSent = 0;
  static char VED_SS_Labels[9][6] = {"PID", "V", "I", "P", "CE", "SOC", "TTG", "ALARM", "AR"};
  static int VED_SS_ConnStat = 0;
  static VED_Shunt_data VSS;
#endif

  // Decode data from VE.Direct SoftwareSerial as often as possible
#ifdef VEDIR_CHRG
  if (VEDSer_Chrg1.available())
  {
    while (VEDSer_Chrg1.available())
    {
      VED_Chrg1.rxData(VEDSer_Chrg1.read());
    }
    VCHRG1.lastUpdate = UptimeSeconds;
    // grant time for background tasks
    delay(1);
  }
#endif
#ifdef VEDIR_SHUNT
  if (VEDSer_Shnt.available())
  {
    while (VEDSer_Shnt.available())
    {
      VED_Shnt.rxData(VEDSer_Shnt.read());
    }
    VSS.lastUpdate = UptimeSeconds;
    // grant time for background tasks
    delay(1);
  }

  // Verify data and store in struct
  // Battery Voltage
  if (VSS.iV == 255)
  {
    VSS.iV = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_V]);
  }
  if (VSS.iV != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iV]) > 10000) && (atoi(VED_Shnt.veValue[VSS.iV]) < 30000))
    {
      VSS.V = atof(VED_Shnt.veValue[VSS.iV]) / 1000;
    }
  }
  // Battery Current
  if (VSS.iI == 255)
  {
    VSS.iI = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_I]);
  }
  if (VSS.iI != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iI]) > -150000) && (atoi(VED_Shnt.veValue[VSS.iI]) < 150000))
    {
      VSS.I = atof(VED_Shnt.veValue[VSS.iI]) / 1000;
    }
  }
  // Battery Power
  if (VSS.iP == 255)
  {
    VSS.iP = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_P]);
  }
  if (VSS.iP != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iP]) > -2500) && (atoi(VED_Shnt.veValue[VSS.iP]) < 2500))
    {
      VSS.P = atoi(VED_Shnt.veValue[VSS.iP]);
    }
  }
  // Consumed Energy
  if (VSS.iCE == 255)
  {
    VSS.iCE = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_CE]);
  }
  if (VSS.iCE != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iCE]) >= -200000) && (atoi(VED_Shnt.veValue[VSS.iCE]) <= 0))
    {
      VSS.CE = atof(VED_Shnt.veValue[VSS.iCE]) / 1000;
    }
  }
  // Battery SoC
  if (VSS.iSOC == 255)
  {
    VSS.iSOC = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_SOC]);
  }
  if (VSS.iSOC != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iSOC]) >= 0) && (atoi(VED_Shnt.veValue[VSS.iSOC]) <= 1000))
    {
      VSS.SOC = atof(VED_Shnt.veValue[VSS.iSOC]) / 10;
    }
  }
  // Battery TTG (time-to-empty)
  if (VSS.iTTG == 255)
  {
    VSS.iTTG = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_TTG]);
  }
  if (VSS.iTTG != 255)
  {
    if ((atoi(VED_Shnt.veValue[VSS.iTTG]) >= -1) && (atoi(VED_Shnt.veValue[VSS.iTTG]) < 10000))
    {
      VSS.TTG = atoi(VED_Shnt.veValue[VSS.iTTG]);
    }
  }

  // Get .veValue indexes for informational data (no data verification)
  if (VSS.iALARM == 255)
  {
    VSS.iALARM = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_ALARM]);
  }
  if (VSS.iAR == 255)
  {
    VSS.iAR = VED_Shnt.getIndexByName(VED_SS_Labels[i_SS_LBL_AR]);
  }
#endif

  //
  // Gather data every second
  //
  if ((millis() - oldMillis) >= 1000 || FirstLoop)
  {
    oldMillis = millis();
    UptimeSeconds++;

    // Update data from BMS
    // ATTN: takes between 640 and 710ms!
    BMSresponding = bms.update();

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
        // this might happen when ESP is flashed while discharging
        Calc.Ws = 0;
      }
      Calc.SOC = (Calc.Ws / Calc.max_Ws) * 100;
    }
    else
    {
      INADAT.I = 0;
      Calc.P = 0;
    }

    // Check if battery is full..
    if (bms.alarm.levelOnePackVoltageTooHigh || bms.alarm.levelOneCellVoltageTooHigh || INADAT.V > BAT_FULL_V)
    {
      Calc.SOC = 100;
      Calc.max_Ws = Calc.Ws;
    }
    // ..or empty
    else if ((INADAT.V <= BAT_EMPTY_V) || (FirstLoop && INADAT.V < BAT_NEARLY_EMPTY_V))
    {
      Calc.SOC = 0;
      Calc.Ws = 0;
    }

    // Display enabled?
    EnableDisplay = (bool)digitalRead(BUT1);
  }

  //
  // Publish data to MQTT broker
  //
  if (((UptimeSeconds - LastDataUpdate) >= DATA_UPDATE_INTERVAL || FirstLoop) && mqttClt.connected())
  {
    LastDataUpdate = UptimeSeconds;
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

#ifdef VEDIR_CHRG
    if ((UptimeSeconds - VCHRG1.lastUpdate) < VED_TIMEOUT)
    {
      if (VCHRG1.lastUpdate >= VCHRG1.lastPublish)
      {
        // Decode most important CS values to text
        switch (atoi(VED_Chrg1.veValue[VCHRG1.iCS]))
        {
        case 0 ... 5:
          mqttClt.publish(t_VED_C1_CS, String(VED_CS_Decoder[atoi(VED_Chrg1.veValue[VCHRG1.iCS])]).c_str(), true);
          break;
        default:
          mqttClt.publish(t_VED_C1_CS, String(VED_Chrg1.veValue[VCHRG1.iCS]).c_str(), true);
          break;
        }

        // Decode most important ERR values to text
        switch (atoi(VED_Chrg1.veValue[VCHRG1.iERR]))
        {
        case 0:
          mqttClt.publish(t_VED_C1_ERR, String("Ok").c_str(), true);
          break;
        case 2:
          mqttClt.publish(t_VED_C1_ERR, String("Vbat_HI").c_str(), true);
          break;
        case 17:
          mqttClt.publish(t_VED_C1_ERR, String("Ch_Temp_HI").c_str(), true);
          break;
        case 18:
          mqttClt.publish(t_VED_C1_ERR, String("Ch_I_HI").c_str(), true);
          break;
        default:
          mqttClt.publish(t_VED_C1_CS, String(VED_Chrg1.veValue[VCHRG1.iERR]).c_str(), true);
          break;
        }

        mqttClt.publish(t_VED_C1_PPV, String(VED_Chrg1.veValue[VCHRG1.iPPV]).c_str(), true);
        mqttClt.publish(t_VED_C1_IB, String((atof(VED_Chrg1.veValue[VCHRG1.iIB]) / 1000), 2).c_str(), true);
        mqttClt.publish(t_VED_C1_VB, String((atof(VED_Chrg1.veValue[VCHRG1.iVB]) / 1000), 2).c_str(), true);
        mqttClt.publish(t_VED_C1_MPPT, String(VED_MPPT_Decoder[atoi(VED_Chrg1.veValue[VCHRG1.iMPPT])]).c_str(), true);
        mqttClt.publish(t_VED_C1_H20, String((atoi(VED_Chrg1.veValue[VCHRG1.iH20]) * 10)).c_str(), true);
        mqttClt.publish(t_VED_C1_CSTAT, String(VED_ConnStat_Decoder[VCHRG1.ConnStat]).c_str(), true);
        // Data published, connection OK
        VCHRG1.ConnStat = 1;
        VCHRG1.lastPublish = UptimeSeconds;
        delay(100);
      }
    }
    else
    {
      // No data received within timeout period!
      VCHRG1.ConnStat = 2;
      mqttClt.publish(t_VED_C1_CSTAT, String(VED_ConnStat_Decoder[VCHRG1.ConnStat]).c_str(), true);
    }

#endif // VEDIR_CHRG

#ifdef VEDIR_SHUNT
    if ((UptimeSeconds - VSS.lastUpdate) < VED_TIMEOUT)
    {
      if (VSS.lastUpdate >= VSS.lastPublish)
      {
        mqttClt.publish(t_VED_SH_V, String(VSS.V, 2).c_str(), true);
        mqttClt.publish(t_VED_SH_I, String(VSS.I, 2).c_str(), true);
        mqttClt.publish(t_VED_SH_P, String(VSS.P).c_str(), true);
        mqttClt.publish(t_VED_SH_CE, String(VSS.CE, 2).c_str(), true);
        mqttClt.publish(t_VED_SH_SOC, String(VSS.SOC, 1).c_str(), true);
        mqttClt.publish(t_VED_SH_TTG, String(VSS.TTG).c_str(), true);
        mqttClt.publish(t_VED_SH_ALARM, String(VED_Shnt.veValue[VSS.iALARM]).c_str(), true);
        mqttClt.publish(t_VED_SH_AR, String(VED_Shnt.veValue[VSS.iAR]).c_str(), true);
        mqttClt.publish(t_VED_SH_CSTAT, String(VED_ConnStat_Decoder[VSS.ConnStat]).c_str(), true);
        // Data published, connection OK
        VSS.ConnStat = 1;
        VSS.lastPublish = UptimeSeconds;
      }
    }
    else
    {
      // No data received within timeout period!
      VSS.ConnStat = 2;
      mqttClt.publish(t_VED_SH_CSTAT, String(VED_ConnStat_Decoder[VSS.ConnStat]).c_str(), true);
    }
#endif // VEDIR_SHUNT

    // Add some more delay for WiFi processing
    delay(100);
  }

  //
  // Update 1hr power average every 6 minutes
  //
  if ((UptimeSeconds - LastAvgCalc) >= 360 || FirstLoop)
  {
    LastAvgCalc = UptimeSeconds;
    // get power avg of the prev 6 minutes and add it to the averaging data
    Calc.P_Avg_Arr[UpdAvgArrIndx] = (Calc.Ws - Calc.P_Avg_Prev_Ws) / 360;
    Calc.P_Avg_Prev_Ws = Calc.Ws;
    // rotate to next array element
    UpdAvgArrIndx++;
    if (UpdAvgArrIndx > 9)
    {
      UpdAvgArrIndx = 0;
    }
    // Recalculate 1hr power average
    float Pavg_Sum = 0;
    for (int i = 0; i < 10; i++)
    {
      Pavg_Sum += Calc.P_Avg_Arr[i];
    }
    Calc.P_Avg_1h = Pavg_Sum / 10;
    mqttClt.publish(t_C_AvgP, String(Calc.P_Avg_1h, 1).c_str(), true);
    // Add some delay for WiFi processing
    delay(100);
  }

  //
  // Load Controller (SSR1)
  //
  // If CSOC has reached the configured charge limit, enable load
  if (!Ctrl_SSR1 && Calc.SOC >= ENABLE_LOAD_CSOC)
  {
    Ctrl_SSR1 = true;
    mqttClt.publish(t_Ctrl_SSR1, String("on").c_str(), true);
    // Add some delay for WiFi processing
    delay(100);
  }
  // if we have a really sunny day, enable load at the configured HIGH_PV limits
  else if (!Ctrl_SSR1 && Calc.P_Avg_1h >= HIGH_PV_AVG_PWR && Calc.SOC >= HIGH_PV_EN_LOAD_CSOC)
  {
    Ctrl_SSR1 = true;
    mqttClt.publish(t_Ctrl_SSR1, String("on").c_str(), true);
    // Add some delay for WiFi processing
    delay(100);
  }

  // Disable SSR1 when CSOC_DISABLE_LOAD is reached
  if (Ctrl_SSR1 && Calc.SOC <= DISABLE_LOAD_CSOC)
  {
    Ctrl_SSR1 = false;
    mqttClt.publish(t_Ctrl_SSR1, String("off").c_str(), true);
    // Add some delay for WiFi processing
    delay(100);
  }

  //
  // Active Balancer Controller (SSR2)
  //
  if (Ctrl_SSR2)
  {
    // Balancer is active
    // If balancer is running for at least BAL_MIN_ON_DUR..
    // Note: this check is always true if the Balancer
    // has been manually enabled through MQTT, so we don't bother about this special case.
    if ((UptimeSeconds - LastBalancerEnable) > BAL_MIN_ON_DUR)
    {
      // ..check if any cell is below BAL_OFF_CELLV
      for (int i = 0; i < bms.get.numberOfCells; i++)
      {
        if (bms.get.cellVmV[i] < BAL_OFF_CELLV)
        {
          // Low cell voltage threshold reached, disable Balancer
          Ctrl_SSR2 = false;
          mqttClt.publish(t_Ctrl_SSR2, String("off").c_str(), true);
          // Add some delay for WiFi processing
          delay(100);
          // end loop, one cell below threshold is enough
          break;
        }
      }
    }
  }
  else
  {
    // Balancer is inactive
    // If 1hr Power average is above threshold..
    if (Calc.P_Avg_1h > BAL_ON_MIN_PWRAVG)
    {
      // ..check if any cell is above BAL_ON_CELLV
      for (int i = 0; i < bms.get.numberOfCells; i++)
      {
        if (bms.get.cellVmV[i] > BAL_ON_CELLV)
        {
          // High cell voltage threshold reached, enable Balancer
          Ctrl_SSR2 = true;
          mqttClt.publish(t_Ctrl_SSR2, String("on").c_str(), true);
          // Add some delay for WiFi processing
          delay(100);
          // remember when we've enabled the Balancer
          LastBalancerEnable = UptimeSeconds;
          // end loop, one cell above threshold is enough
          break;
        }
      }
    }
  }

  //
  // Set desired Daly MOSFET charge / discharge switch states
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
  // Set desired SSR1 state
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
  // Set desired SSR2 state
  //
  if (Ctrl_SSR2 != (bool)digitalRead(SSR2))
  {
    if (Ctrl_SSR2)
    {
      digitalWrite(SSR2, HIGH);
    }
    else
    {
      digitalWrite(SSR2, LOW);
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
#ifdef VEDIR_CHRG
        oled.println("VE.D_C1 FrCnt: " + String(VED_Chrg1.frameCounter));
        oled.println("VE.D_C1 PPV: " + String(VED_Chrg1.veValue[VCHRG1.iPPV]));
#else
        oled.println("Charge FET: " + String(Bool_Decoder[(int)bms.get.chargeFetState]));
        oled.println("Disch FET:  " + String(Bool_Decoder[(int)bms.get.disChargeFetState]));
#endif
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

  // Reset FirstLoop
  if (FirstLoop)
  {
    FirstLoop = false;
  }

#ifdef ONBOARD_LED
  // Toggle LED at each loop
  ToggleLed(LED, 100, 4);
#endif
}
