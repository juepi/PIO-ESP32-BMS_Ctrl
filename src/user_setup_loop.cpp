/*
 * ESP32 Template
 * ==================
 * User specific function "user_loop" called in main loop
 * User specific funtion "user_setup" called in setup function
 * Add stuff you want to run here
 */
#include "setup.h"

// Setup Daly BMS connector instance
Daly_BMS_UART Daly(DALY_UART);

// Setup VE.Direct
SoftwareSerial VEDSer_Chrg1;
VeDirectFrameHandler VED_Chrg1;
SoftwareSerial VEDSer_Shnt;
VeDirectFrameHandler VED_Shnt;

// Setup OneWire Temperature Sensor(s)
#ifdef ENA_ONEWIRE // Optional OneWire support
OneWire oneWire(PIN_OWDATA);
DallasTemperature OWtemp(&oneWire);
#endif

// Global variable declarations
// for MQTT topics
Load_SSR_Config SSR1;
Load_SSR_Config SSR3;
Balancer_Config SSR2;
PV_Config PV;
Safety_Config Safety;

/*
 * User Setup Function
 * ========================================================================
 */
void user_setup()
{
  pinMode(PIN_SSR1, OUTPUT);
  pinMode(PIN_SSR2, OUTPUT);
  pinMode(PIN_SSR3, OUTPUT);
  pinMode(PIN_SSR4, OUTPUT);
  digitalWrite(PIN_SSR1, LOW);
  digitalWrite(PIN_SSR2, LOW);
  digitalWrite(PIN_SSR3, LOW);
  digitalWrite(PIN_SSR4, LOW);
  Daly.Init();

  VEDSer_Chrg1.begin(VED_BAUD, SWSERIAL_8N1, PIN_VED_CHRG1_RX, PIN_VED_CHRG1_TX, false, 512);
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

  VEDSer_Shnt.begin(VED_BAUD, SWSERIAL_8N1, PIN_VED_SHNT_RX, PIN_VED_SHNT_TX, false, 512);
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

#ifdef ENA_ONEWIRE // Optional OneWire support
  OWtemp.begin();
  OWtemp.setResolution(OWRES);
#endif
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
  static bool FirstLoop = true; // Firmware startup
  // Daly BMS
  static Daly_BMS_data BMS;
  static int CDiff_Unplausible_Cnt = 0;
  static float Prev_CDiff = 0;
  // Uptime calculation
  static uint32_t UptimeSeconds = 0;
  static unsigned long oldMillis = 0;
  // Other helpers
  static uint32_t MQTTLastDataPublish = 0;
  static const char *Bool_Decoder[] = {"off", "on"};
  // Connection Status decoding (for all serial connections)
  static const char *ConnStat_Decoder[] = {"Startup", "Ok", "Timeout", "Read_Fail", "Init_Fail"};
  // VE.Direct Charger
  // Int to Text conversion for Charger data
  static short UpdAvgArrIndx = 0; // 1hr average PPV calculation
  static uint32_t LastAvgCalc = 0;
  static const char *VED_MPPT_Decoder[] = {"Off", "V_I_Lim", "Active"};
  static const char *VED_CS_Decoder[] = {"Off", "na", "Fault", "Bulk", "Absorption", "Float"};
  static VED_Charger_data VCHRG1;
  // VE.Direct SmartShunt
  static char VED_SS_Labels[9][6] = {"PID", "V", "I", "P", "CE", "SOC", "TTG", "ALARM", "AR"};
  static VED_Shunt_data VSS;
#ifdef ENA_ONEWIRE // Optional OneWire support
  static OneWire_data OW;
#endif

  //
  // Start the action
  //
  // on boot, wait a second to ensure we have received data from the VE.Direct devices
  if (FirstLoop)
  {
    // Publish initial SSR active and desired states to the broker to match the firmware boot state of ALL OFF
    mqttClt.publish(t_Ctrl_Cfg_SSR1_setState, String(Bool_Decoder[0]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR1_actState, String(Bool_Decoder[0]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_setState, String(Bool_Decoder[0]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_actState, String(Bool_Decoder[0]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_setState, String(Bool_Decoder[0]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_actState, String(Bool_Decoder[0]).c_str(), true);
    // Use MqttDelay to ensure that we re-read the freshly published settings from the broker
    MqttDelay(1000);
  }

  //
  // Increase Uptime counter
  //
  if ((millis() - oldMillis) >= 1000)
  {
    oldMillis = millis();
    UptimeSeconds++;
#ifdef ONBOARD_LED
    // Toggle LED every second
    ToggleLed(LED, 1, 1);
#endif
  }

  //
  // Decode available data from VE.Direct SoftwareSerials immediately
  //
  // Handle VE.Direct SmartSolar Charger #1
  if (VEDSer_Chrg1.available())
  {
    while (VEDSer_Chrg1.available())
    {
      VED_Chrg1.rxData(VEDSer_Chrg1.read());
    }
    if (VCHRG1.lastDecodedFr < VED_Chrg1.frameCounter)
    {
      // new frame decoded from VeDirectFrameHandler
      VCHRG1.lastUpdate = UptimeSeconds;
      VCHRG1.lastDecodedFr = VED_Chrg1.frameCounter;
      // assume connection OK
      VCHRG1.ConnStat = 1;
      // Store new data in struct (only PPV needed here, everything else will just be copied out to MQTT topics)
      VCHRG1.PPV = atoi(VED_Chrg1.veValue[VCHRG1.iPPV]);
    }
    // grant time for background tasks
    delay(1);
  }

  // Verify if connection to VE.Direct device is alive
  if ((UptimeSeconds - VCHRG1.lastUpdate) > VED_TIMEOUT)
  {
    // Connection timed out (no valid data received within timeout period)
    VCHRG1.ConnStat = 2;
  }

  // Handle VE.Direct SmartShunt
  if (VEDSer_Shnt.available())
  {
    while (VEDSer_Shnt.available())
    {
      VED_Shnt.rxData(VEDSer_Shnt.read());
    }
    if (VSS.lastDecodedFr < VED_Shnt.frameCounter)
    {
      // new frame decoded from VeDirectFrameHandler
      VSS.lastDecodedFr = VED_Shnt.frameCounter;
      // assume connection OK
      VSS.ConnStat = 1;

      // Verify data and store in struct
      // Battery Voltage (in mV)
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
      // Battery Current (in mA)
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
      // Battery Power (in W)
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
      // Consumed Energy (in mAh)
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
          if (FirstLoop)
          {
            // we have to assume the first readout is correct
            VSS.SOC = atof(VED_Shnt.veValue[VSS.iSOC]) / 10;
          }
          else
          {
            // Additional plausibility check for new SOC
            if (abs(atof(VED_Shnt.veValue[VSS.iSOC]) - VSS.SOC * 10) > VSS_MAX_SOC_DIFF)
            {
              // Unplausible increase or decrease of SOC - discard data
              mqttClt.publish(t_Ctrl_StatT, String("VSS_SOC_Discard").c_str(), true);
              VSS.ConnStat = 3;
            }
            else
            {
              // new SOC seems ok
              VSS.SOC = atof(VED_Shnt.veValue[VSS.iSOC]) / 10;
            }
          }
        }
        else
        {
          // Unplausible SOC decoded
          mqttClt.publish(t_Ctrl_StatT, String("VSS_SOC_Discard").c_str(), true);
          VSS.ConnStat = 3;
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

      // If connection state is still ok (no data verification error), increase lastUpdate
      if (VSS.ConnStat == 1)
      {
        VSS.lastUpdate = UptimeSeconds;
      }
    }
    // grant time for background tasks
    delay(1);
  }

  // Verify if connection to VE.Direct device is alive
  if ((UptimeSeconds - VSS.lastUpdate) > VED_TIMEOUT)
  {
    // Connection timed out (no valid data received within timeout period)
    VSS.ConnStat = 2;
  }

  //
  // Gather data from Daly BMS
  //
  if ((UptimeSeconds - BMS.lastUpdate) >= DALY_UPDATE_INTERVAL || FirstLoop)
  {
    // Fetch data from BMS
    // ATTN: takes between 640 and 710ms!
    if (Daly.update())
    {
      // data set received
      BMS.lastValid = UptimeSeconds;
      BMS.ConnStat = 1;

      // Verify Cell difference
      if (FirstLoop)
      {
        Prev_CDiff = Daly.get.cellDiff;
      }
      else
      {
        // compare against previous readout
        if (abs(Prev_CDiff - Daly.get.cellDiff) > MAX_CDIFF_DIFF)
        {
          if (CDiff_Unplausible_Cnt >= MAX_IGNORED_CDIFF)
          {
            // although unrealistic, we have to assume the readout is correct after several retries..
            CDiff_Unplausible_Cnt = 0;
            Prev_CDiff = Daly.get.cellDiff;
          }
          else
          {
            // unrealistic change, increase error counter
            CDiff_Unplausible_Cnt++;
            // and reset to previous value
            Daly.get.cellDiff = Prev_CDiff;
            mqttClt.publish(t_Ctrl_StatT, String("Daly_Cdiff_Discard").c_str(), true);
          }
        }
        else
        {
          // new value seems ok
          CDiff_Unplausible_Cnt = 0;
          Prev_CDiff = Daly.get.cellDiff;
        }
      }
    }
    else
    {
      if ((UptimeSeconds - BMS.lastValid) > DALY_TIMEOUT)
      {
        // Timeout limit reached
        BMS.ConnStat = 2;
      }
      else
      {
        // Not yet in timeout (readout failed)
        BMS.ConnStat = 3;
      }
    }
    BMS.lastUpdate = UptimeSeconds;
  }

  //
  // Gather data from OneWire sensors
  //
#ifdef ENA_ONEWIRE // Optional OneWire support
  if ((UptimeSeconds - OW.lastUpdate) >= OW_UPDATE_INTERVAL || FirstLoop)
  {
    if (FirstLoop)
    {
      // Check if the correct amount of sensors were found
      if (OWtemp.getDeviceCount() != NUM_OWTEMP)
      {
        // incorrect amount of sensors found
        // this is a permanent failure
        OW.ConnStat = 4;
      }
    }
    // Get OW temperatures
    if (OW.ConnStat <= 3)
    {
      OWtemp.requestTemperatures(); // blocking call
      OW.ReadOk = true;
      for (int i = 0; i < NUM_OWTEMP; i++)
      {
        float ReadT = OWtemp.getTempCByIndex(i);
        if (ReadT == DEVICE_DISCONNECTED_C || ReadT == DEVICE_FAULT_OPEN_C)
        {
          // sensor read fault - end this read cycle
          // Do not update temperature readout array in case of read fault
          // to avoid false-positives on temperature error handling
          OW.ReadOk = false;
          break;
        }
        else
        {
          // Readout valid, update temperature array
          OW.Temperature[i] = ReadT;
        }
      }
      if (OW.ReadOk)
      {
        OW.ConnStat = 1;
        OW.lastValid = UptimeSeconds;
      }
      else
      {
        if ((UptimeSeconds - OW.lastValid) > OW_TIMEOUT)
        {
          // Timeout limit reached
          OW.ConnStat = 2;
        }
        else
        {
          // Not yet in timeout (readout failed)
          OW.ConnStat = 3;
        }
      }
      OW.lastUpdate = UptimeSeconds;
    }
  }
#endif // ENA_ONEWIRE

  //
  // Update 1hr power average every 6 minutes
  //
  if ((UptimeSeconds - LastAvgCalc) >= 360 || FirstLoop)
  {
    // Fill array with current power value if we've just booted
    if (FirstLoop)
      for (int i = 0; i < 10; i++)
      {
        VCHRG1.Avg_PPV_Arr[i] = VCHRG1.PPV;
      }
    LastAvgCalc = UptimeSeconds;
    // Update array element with current PV power
    VCHRG1.Avg_PPV_Arr[UpdAvgArrIndx] = VCHRG1.PPV;
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
      Pavg_Sum += VCHRG1.Avg_PPV_Arr[i];
    }
    VCHRG1.Avg_PPV = Pavg_Sum / 10;
    // Update PPV power level
    if (VCHRG1.Avg_PPV < PV.LowPPV)
    {
      PV.PwrLvl = 0;
    }
    else if (VCHRG1.Avg_PPV >= PV.HighPPV)
    {
      PV.PwrLvl = 2;
    }
    else
    {
      PV.PwrLvl = 1; // Medium PV power - currently unused
    }
  }

  //
  // Publish data to MQTT broker
  //
  if (((UptimeSeconds - MQTTLastDataPublish) >= DATA_UPDATE_INTERVAL || FirstLoop) && mqttClt.connected())
  {
    MQTTLastDataPublish = UptimeSeconds;

    // Controller
    if (FirstLoop)
    {
      mqttClt.publish(t_Ctrl_StatT, String("Startup Firmware v" + String(FIRMWARE_VERSION) + " WiFi RSSI: " + String(WiFi.RSSI())).c_str(), true);
    }
    mqttClt.publish(t_Ctrl_StatU, String(UptimeSeconds).c_str(), true);
    // Publish SSR states periodically (logging / plots for FHEM)
    mqttClt.publish(t_Ctrl_Cfg_SSR1_actState, String(Bool_Decoder[(int)SSR1.actState]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_actState, String(Bool_Decoder[(int)SSR2.actState]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_actState, String(Bool_Decoder[(int)SSR3.actState]).c_str(), true);

    // Daly BMS
    if (BMS.ConnStat <= 1)
    {
      // Send cell voltages
      for (int i = 0; i < Daly.get.numberOfCells; i++)
      {
        int j = i + 1;
        String MqttTopStr = String(t_DV_C_Templ) + String(j) + "V";
        mqttClt.publish(MqttTopStr.c_str(), String((Daly.get.cellVmV[i] / 1000), 3).c_str(), true);
      }
      // send any other more or less useful stuff
      mqttClt.publish(t_DSOC, String(Daly.get.packSOC, 0).c_str(), true);
      mqttClt.publish(t_DV, String(Daly.get.packVoltage, 2).c_str(), true);
      mqttClt.publish(t_DdV, String(Daly.get.cellDiff, 0).c_str(), true);
      mqttClt.publish(t_DI, String(Daly.get.packCurrent, 2).c_str(), true);
      mqttClt.publish(t_DLSw, String(Bool_Decoder[(int)Daly.get.disChargeFetState]).c_str(), true);
      mqttClt.publish(t_DCSw, String(Bool_Decoder[(int)Daly.get.chargeFetState]).c_str(), true);
      mqttClt.publish(t_DTemp, String(Daly.get.tempAverage, 0).c_str(), true);
      mqttClt.publish(t_D_CSTAT, String(ConnStat_Decoder[BMS.ConnStat]).c_str(), true);
      // Add some delay for WiFi processing
      delay(100);
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_D_CSTAT, String(ConnStat_Decoder[BMS.ConnStat]).c_str(), true);
    }

    // VE.Direct Charger #1
    if (VCHRG1.ConnStat <= 1)
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
      mqttClt.publish(t_VED_C1_MPPT, String(VED_MPPT_Decoder[atoi(VED_Chrg1.veValue[VCHRG1.iMPPT])]).c_str(), true);
      mqttClt.publish(t_VED_C1_H20, String((atoi(VED_Chrg1.veValue[VCHRG1.iH20]) * 10)).c_str(), true);
      mqttClt.publish(t_VED_C1_CSTAT, String(ConnStat_Decoder[VCHRG1.ConnStat]).c_str(), true);
      mqttClt.publish(t_VED_C1_AvgPPV, String(VCHRG1.Avg_PPV, 0).c_str(), true);
      delay(100);
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_VED_C1_CSTAT, String(ConnStat_Decoder[VCHRG1.ConnStat]).c_str(), true);
    }

    // VE.Direct SmartShunt
    if (VSS.ConnStat <= 1)
    {
      mqttClt.publish(t_VED_SH_V, String(VSS.V, 2).c_str(), true);
      mqttClt.publish(t_VED_SH_I, String(VSS.I, 1).c_str(), true);
      mqttClt.publish(t_VED_SH_P, String(VSS.P).c_str(), true);
      mqttClt.publish(t_VED_SH_CE, String(VSS.CE, 1).c_str(), true);
      mqttClt.publish(t_VED_SH_SOC, String(VSS.SOC, 0).c_str(), true);
      mqttClt.publish(t_VED_SH_TTG, String(VSS.TTG).c_str(), true);
      mqttClt.publish(t_VED_SH_ALARM, String(VED_Shnt.veValue[VSS.iALARM]).c_str(), true);
      mqttClt.publish(t_VED_SH_AR, String(VED_Shnt.veValue[VSS.iAR]).c_str(), true);
      mqttClt.publish(t_VED_SH_CSTAT, String(ConnStat_Decoder[VSS.ConnStat]).c_str(), true);
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_VED_SH_CSTAT, String(ConnStat_Decoder[VSS.ConnStat]).c_str(), true);
    }

#ifdef ENA_ONEWIRE // Optional OneWire support
    // OneWire Sensors
    if (OW.ConnStat <= 1)
    {
      // Send sensor data
      for (int i = 0; i < NUM_OWTEMP; i++)
      {
        int j = i + 1;
        String MqttTopStr = String(t_OW_TEMP_Templ) + String(j);
        mqttClt.publish(MqttTopStr.c_str(), String(OW.Temperature[i], 0).c_str(), true);
      }
      mqttClt.publish(t_OW_CSTAT, String(ConnStat_Decoder[OW.ConnStat]).c_str(), true);
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_OW_CSTAT, String(ConnStat_Decoder[OW.ConnStat]).c_str(), true);
    }
#endif // ENA_ONEWIRE

    // Report network outage recovery (once)
    if (NetRecoveryMillis > 0)
    {
      mqttClt.publish(t_Ctrl_StatT, String("Net_Outage_Recovered: " + String(UptimeSeconds)).c_str(), true);
      NetRecoveryMillis = 0;
    }

    // Add some more delay for WiFi processing
    delay(100);
  }

  //
  // Load Controller (SSR1)
  //
  // If SOC has reached the configured charge limit, enable load
  if (SSR1.Auto && !SSR1.actState)
  {
    // Auto mode enabled, SSR1 disabled, check PV power state
    switch (PV.PwrLvl)
    {
    case 0 ... 1: // Low and medium power
      if (VSS.SOC > SSR1.LPOnSOC)
      {
        SSR1.setState = true;
        mqttClt.publish(t_Ctrl_StatT, String("SSR1_ON_LoPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
      break;
    case 2: // high power
      if (VSS.SOC > SSR1.HPOnSOC)
      {
        SSR1.setState = true;
        mqttClt.publish(t_Ctrl_StatT, String("SSR1_ON_HiPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
    }
  }

  // Disable SSR1 when SOC according to PV power is reached
  // This must also trigger if the load was enabled manually, so use effective SSR1.actState
  if (SSR1.actState)
  {
    switch (PV.PwrLvl)
    {
    case 0 ... 1: // Low and medium power
      if (VSS.SOC <= SSR1.LPOffSOC)
      {
        SSR1.setState = false;
        mqttClt.publish(t_Ctrl_StatT, String("SSR1_OFF_LoPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
      break;
    case 2: // high power
      if (VSS.SOC <= SSR1.HPOffSOC)
      {
        SSR1.setState = false;
        mqttClt.publish(t_Ctrl_StatT, String("SSR1_OFF_HiPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
    }
  }

  //
  // Load Controller (SSR3)
  //
  // If SOC has reached the configured charge limit, enable load
  if (SSR3.Auto && !SSR3.actState)
  {
    // Auto mode enabled, SSR1 disabled, check PV power state
    switch (PV.PwrLvl)
    {
    case 0 ... 1: // Low and medium power
      if (VSS.SOC > SSR3.LPOnSOC)
      {
        SSR3.setState = true;
        mqttClt.publish(t_Ctrl_StatT, String("SSR3_ON_LoPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
      break;
    case 2: // high power
      if (VSS.SOC > SSR3.HPOnSOC)
      {
        SSR3.setState = true;
        mqttClt.publish(t_Ctrl_StatT, String("SSR3_ON_HiPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
    }
  }

  // Disable SSR3 when SOC according to PV power is reached
  // This must also trigger if the load was enabled manually, so use effective SSR3.actState
  if (SSR3.actState)
  {
    switch (PV.PwrLvl)
    {
    case 0 ... 1: // Low and medium power
      if (VSS.SOC <= SSR3.LPOffSOC)
      {
        SSR3.setState = false;
        mqttClt.publish(t_Ctrl_StatT, String("SSR3_OFF_LoPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
      break;
    case 2: // high power
      if (VSS.SOC <= SSR3.HPOffSOC)
      {
        SSR3.setState = false;
        mqttClt.publish(t_Ctrl_StatT, String("SSR3_OFF_HiPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
      }
    }
  }

  //
  // Active Balancer Controller (SSR2)
  //
  if (SSR2.Auto)
  {
    if (SSR2.actState)
    {
      // Balancer is active
      // ..check if any cell is below low cell voltage threshold
      for (int i = 0; i < Daly.get.numberOfCells; i++)
      {
        if (Daly.get.cellVmV[i] < SSR2.CVOff)
        {
          // Low cell voltage threshold reached, disable Balancer
          SSR2.setState = false;
          mqttClt.publish(t_Ctrl_StatT, String("SSR2_OFF_C" + String((i + 1)) + "_Vlow").c_str(), true);
          break;
        }
      }
    }
    else
    {
      // Balancer is inactive
      // If voltage difference is too high
      if (Daly.get.cellDiff > SSR2.CdiffOn)
      {
        // ..check if any cell is above enable voltage
        for (int i = 0; i < Daly.get.numberOfCells; i++)
        {
          if (Daly.get.cellVmV[i] > SSR2.CVOn)
          {
            // High cell voltage threshold reached, enable Balancer
            SSR2.setState = true;
            mqttClt.publish(t_Ctrl_StatT, String("SSR2_ON_C" + String((i + 1)) + "_Vhi").c_str(), true);
            // end loop, one cell above threshold is enough
            break;
          }
        }
      }
    }
  }

  //
  // SAFETY CHECKS
  //
  // Disable loads and auto-modes when communication to VSS, Daly-BMS or OW-sensors timed out
#ifdef ENA_ONEWIRE // Optional OneWire support
  if (!Safety.ConnStateCritical && (VSS.ConnStat == 2 || BMS.ConnStat == 2 || OW.ConnStat == 2))
#else
  if (!Safety.ConnStateCritical && (VSS.ConnStat == 2 || BMS.ConnStat == 2))
#endif
  {
    Safety.ConnStateCritical = true;
    SSR1.setState = false;
    SSR3.setState = false;
    SSR1.Auto = false;
    SSR3.Auto = false;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
#ifdef ENA_ONEWIRE // Optional OneWire support
    mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CONNSTAT:" + String(VSS.ConnStat) + "/" + String(BMS.ConnStat) + "/" + String(OW.ConnStat)).c_str(), true);
#else
    mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CONNSTAT:" + String(VSS.ConnStat) + "/" + String(BMS.ConnStat)).c_str(), true);
#endif
  }
  // Check if communcation has been restored to all devices
#ifdef ENA_ONEWIRE // Optional OneWire support
  if (Safety.ConnStateCritical && VSS.ConnStat == 1 && BMS.ConnStat == 1 && OW.ConnStat == 1)
#else
  if (Safety.ConnStateCritical && VSS.ConnStat == 1 || BMS.ConnStat == 1)
#endif
  {
    Safety.ConnStateCritical = false;
    SSR1.Auto = true;
    SSR3.Auto = true;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
#ifdef ENA_ONEWIRE // Optional OneWire support
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CONNSTAT").c_str(), true);
#else
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CONNSTAT").c_str(), true);
#endif
  }

  // Disable loads and auto-modes when battery pack voltage is below critical threshold
  if (!Safety.LowBatVCritical && Daly.get.packVoltage <= Safety.Crit_Bat_Low_V)
  {
    Safety.LowBatVCritical = true;
    SSR1.setState = false;
    SSR3.setState = false;
    SSR1.Auto = false;
    SSR3.Auto = false;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_StatT, String("CRIT_SAFETY_LOW_VBAT:" + String(Daly.get.packVoltage, 1) + "V").c_str(), true);
  }
  // Check if we recovered from undervoltage critical condition
  if (Safety.LowBatVCritical && Daly.get.packVoltage > Safety.Rec_Bat_Low_V)
  {
    Safety.LowBatVCritical = false;
    SSR1.Auto = true;
    SSR3.Auto = true;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_LOW_VBAT:" + String(Daly.get.packVoltage, 1) + "V").c_str(), true);
  }

  // Disable Loads and auto-modes if Celldiff is extremely high
  if (!Safety.CVdiffCritical && Daly.get.cellDiff > Safety.Crit_CVdiff)
  {
    Safety.CVdiffCritical = true;
    SSR1.Auto = false;
    SSR2.Auto = false;
    SSR3.Auto = false;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_Auto, String(Bool_Decoder[(int)SSR2.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
    SSR1.setState = false;
    SSR3.setState = false;
    mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CDIFF:" + String(Daly.get.cellDiff, 0)).c_str(), true);
  }
  // Check if we recovered from Cell diff critical condition
  if (Safety.CVdiffCritical && Daly.get.cellDiff < Safety.Rec_CVdiff)
  {
    Safety.CVdiffCritical = false;
    SSR1.Auto = true;
    SSR2.Auto = true;
    SSR3.Auto = true;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_Auto, String(Bool_Decoder[(int)SSR2.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CDIFF:" + String((Daly.get.cellDiff, 0))).c_str(), true);
  }

  //
  // Temperature based SAFETY CHECKS
  //
  // Check Cell temperatures
#ifdef ENA_ONEWIRE // Optional OneWire support
  if (!Safety.CellTempCritical && (OW.Temperature[i_C1_SENS] > Safety.Crit_CellTemp || OW.Temperature[i_C8_SENS] > Safety.Crit_CellTemp || Daly.get.tempAverage > Safety.Crit_CellTemp))
#else
  if (!Safety.CellTempCritical && Daly.get.tempAverage > Safety.Crit_CellTemp)
#endif
  {
    Safety.CellTempCritical = true;
    if (PV.PwrLvl == 2)
    {
      // Cells probably overheated while charging, disable Daly Charge FET
      BMS.setCSw = 0;
      mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CTEMP: BMS Charging disabled").c_str(), true);
    }
    else
    {
      // Cells probably overheated while discharging, disable loads
      SSR1.Auto = false;
      SSR3.Auto = false;
      mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
      mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
      SSR1.setState = false;
      SSR3.setState = false;
      mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CTEMP: Loads disabled").c_str(), true);
#ifdef ENA_ONEWIRE // Optional OneWire support
      mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CTEMP:" + String(OW.Temperature[i_C1_SENS], 0) + "/" + String(Daly.get.tempAverage, 0) + "/" + String(OW.Temperature[i_C8_SENS], 0)).c_str(), true);
#else
      mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CTEMP:" + String(Daly.get.tempAverage, 0)).c_str(), true);
#endif
    }
  }
  // Check if all cells recovered from ciritcal cell temperatures
#ifdef ENA_ONEWIRE // Optional OneWire support
  if (Safety.CellTempCritical && OW.Temperature[i_C1_SENS] < Safety.Rec_CellTemp && OW.Temperature[i_C8_SENS] < Safety.Rec_CellTemp && Daly.get.tempAverage < Safety.Rec_CellTemp)
#else
  if (Safety.CellTempCritical && Daly.get.tempAverage < Safety.Rec_CellTemp)
#endif
  {
    Safety.CellTempCritical = false;
    BMS.setCSw = 1;
    SSR1.Auto = true;
    SSR3.Auto = true;
    mqttClt.publish(t_Ctrl_Cfg_SSR1_Auto, String(Bool_Decoder[(int)SSR1.Auto]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_Auto, String(Bool_Decoder[(int)SSR3.Auto]).c_str(), true);
#ifdef ENA_ONEWIRE // Optional OneWire support
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CTEMP:" + String(OW.Temperature[i_C1_SENS], 0) + "/" + String(Daly.get.tempAverage, 0) + "/" + String(OW.Temperature[i_C8_SENS], 0)).c_str(), true);
#else
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CTEMP:" + String(Daly.get.tempAverage, 0)).c_str(), true);
#endif
  }

  // Check charger temperature (with OneWire only)
#ifdef ENA_ONEWIRE // Optional OneWire support
  if (!Safety.ChrgTempCritical && OW.Temperature[i_CHRG_SENS] > Safety.Crit_ChrgTemp)
  {
    // Charger too hot, disable charge FETs on BMS
    Safety.ChrgTempCritical = true;
    BMS.setCSw = 0;
    mqttClt.publish(t_Ctrl_StatT, String("CRITICAL_SAFETY_CHRGTEMP:" + String(OW.Temperature[i_CHRG_SENS], 0)).c_str(), true);
  }
  // Check if charger temperature recovered
  if (Safety.ChrgTempCritical && OW.Temperature[i_CHRG_SENS] < Safety.Rec_ChrgTemp)
  {
    Safety.ChrgTempCritical = false;
    BMS.setCSw = 1;
    mqttClt.publish(t_Ctrl_StatT, String("RECOVER_SAFETY_CHRGTEMP:" + String(OW.Temperature[i_CHRG_SENS], 0)).c_str(), true);
  }
#endif

  //
  // Set desired Daly-BMS MOSFET states
  //
  if (BMS.setCSw < 2)
  {
    // Set desired Charge Switch state
    switch (BMS.setCSw)
    {
    case 0:
      Daly.setChargeMOS(false);
      break;
    case 1:
      Daly.setChargeMOS(true);
      break;
    }
    mqttClt.publish(t_Ctrl_StatT, (String("Daly_BMS_CSw:") + String(Bool_Decoder[BMS.setCSw])).c_str(), true);
    // desired state set, reset to "do not change"
    BMS.setCSw = 2;
  }
  if (BMS.setLSw < 2)
  {
    // Set desired Discharge Switch state
    switch (BMS.setLSw)
    {
    case 0:
      Daly.setDischargeMOS(false);
      break;
    case 1:
      Daly.setDischargeMOS(true);
      break;
    }
    mqttClt.publish(t_Ctrl_StatT, (String("Daly_BMS_LSw:") + String(Bool_Decoder[BMS.setLSw])).c_str(), true);
    // desired state set, reset to "do not change"
    BMS.setLSw = 2;
  }

  //
  // Set desired SSR1 state
  //
  if (SSR1.setState != SSR1.actState)
  {
    digitalWrite(PIN_SSR1, SSR1.setState);
    SSR1.actState = SSR1.setState;
    // and publish states immediately for better responsiveness in UI
    mqttClt.publish(t_Ctrl_Cfg_SSR1_setState, String(Bool_Decoder[(int)SSR1.setState]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR1_actState, String(Bool_Decoder[(int)SSR1.actState]).c_str(), true);
    delay(100);
  }

  //
  // Set desired SSR2 state
  //
  if (SSR2.setState != SSR2.actState)
  {
    digitalWrite(PIN_SSR2, SSR2.setState);
    SSR2.actState = SSR2.setState;
    // and publish states immediately for better responsiveness in UI
    mqttClt.publish(t_Ctrl_Cfg_SSR2_setState, String(Bool_Decoder[(int)SSR2.setState]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR2_actState, String(Bool_Decoder[(int)SSR2.actState]).c_str(), true);
    delay(100);
  }

  //
  // Set desired SSR3 state
  //
  if (SSR3.setState != SSR3.actState)
  {
    digitalWrite(PIN_SSR3, SSR3.setState);
    SSR3.actState = SSR3.setState;
    // and publish states immediately for better responsiveness in UI
    mqttClt.publish(t_Ctrl_Cfg_SSR3_setState, String(Bool_Decoder[(int)SSR3.setState]).c_str(), true);
    mqttClt.publish(t_Ctrl_Cfg_SSR3_actState, String(Bool_Decoder[(int)SSR3.actState]).c_str(), true);
    delay(100);
  }

  // Reset FirstLoop
  if (FirstLoop)
  {
    FirstLoop = false;
  }
}
