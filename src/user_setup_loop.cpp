/*
 * ESP32 Template
 * ==================
 * User specific function "user_loop" called in main loop
 * User specific funtion "user_setup" called in setup function
 * Add stuff you want to run here
 */
#include "setup.h"

// Setup Daly BMS connector instance
Daly_BMS_UART bms(DALY_UART);

// Setup VE.Direct
SoftwareSerial VEDSer_Chrg1;
VeDirectFrameHandler VED_Chrg1;
SoftwareSerial VEDSer_Shnt;
VeDirectFrameHandler VED_Shnt;

// Setup OneWire Temperature Sensor(s)
#ifdef ENA_ONEWIRE // Optional OneWire support
OneWire oneWire(OWDATA);
DallasTemperature OWtemp(&oneWire);
#endif

// Global variable declarations
// for MQTT topics
int Ctrl_DalyChSw = 2;
int Ctrl_DalyLoadSw = 2;
int Ctrl_SSR1 = 2;
int Ctrl_SSR2 = 2;

/*
 * User Setup Function
 * ========================================================================
 */
void user_setup()
{
  pinMode(SSR1, OUTPUT);
  pinMode(SSR2, OUTPUT);
  digitalWrite(SSR1, LOW);
  digitalWrite(SSR2, LOW);
  bms.Init();

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
  static bool BMSresponding = false;
  static uint32_t BMSLastDataUpdate = 0;
  static uint32_t BMSLastValidData = 0;
  static int BMSConnStat = 0;
  // Uptime calculation
  static uint32_t UptimeSeconds = 0;
  static unsigned long oldMillis = 0;
  // Load
  static bool Ctrl_SSR1_autoSetState = false; // desired state of automatic mode
  static bool Ctrl_SSR1_actState = false;     // currently active state set on GPIO
  // Balancer
  static bool Ctrl_SSR2_actState = false; // currently active state set on GPIO
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
  // Array for OneWire Temperature Sensor(s) addresses
  static float OW_SensorData[NUM_OWTEMP] = {0};
  static int OWConnStat = 0;
  static uint32_t OWLastDataUpdate = 0;
#endif

  //
  // Start the action
  //
  // on boot, wait a second to ensure we have received data from the VE.Direct devices
  if (FirstLoop)
  {
    delay(1000);
  }

  //
  // Increase Uptime counter
  //
  if ((millis() - oldMillis) >= 1000)
  {
    oldMillis = millis();
    UptimeSeconds++;
  }

  //
  // Decode available data from VE.Direct SoftwareSerials immediately
  //
  if (VEDSer_Chrg1.available())
  {
    while (VEDSer_Chrg1.available())
    {
      VED_Chrg1.rxData(VEDSer_Chrg1.read());
    }
    if (VCHRG1.lastValidFr < VED_Chrg1.frameCounter)
    {
      // new valid frame decoded from VeDirectFrameHandler
      VCHRG1.lastUpdate = UptimeSeconds;
      VCHRG1.lastValidFr = VED_Chrg1.frameCounter;
      // Connection OK
      VCHRG1.ConnStat = 1;
      // Store new data in struct (only PPV needed here, anything else will just be copied out to MQTT topics)
      VCHRG1.PPV = atoi(VED_Chrg1.veValue[VCHRG1.iPPV]);
    }
    else
    {
      if ((UptimeSeconds - VCHRG1.lastUpdate) > VED_TIMEOUT)
      {
        // Connection timed out (due to no valid data within VED_TIMEOUT)
        VCHRG1.ConnStat = 2;
      }
      else
      {
        // invalid or incomplete frame received
        // this happens during regular (non-blocking) operation of VeDirectFrameHandler, so set ConnStat to OK
        VCHRG1.ConnStat = 1;
      }
    }
    // grant time for background tasks
    delay(1);
  }
  else
  {
    if ((UptimeSeconds - VCHRG1.lastUpdate) > VED_TIMEOUT)
    {
      // Connection timed out (no data received at all within VED_TIMEOUT)
      VCHRG1.ConnStat = 2;
    }
  }

  if (VEDSer_Shnt.available())
  {
    while (VEDSer_Shnt.available())
    {
      VED_Shnt.rxData(VEDSer_Shnt.read());
    }
    if (VSS.lastValidFr < VED_Shnt.frameCounter)
    {
      // new valid frame decoded from VeDirectFrameHandler
      VSS.lastUpdate = UptimeSeconds;
      VSS.lastValidFr = VED_Shnt.frameCounter;
      // Connection OK
      VSS.ConnStat = 1;

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
              mqttClt.publish(t_Ctrl_StatT, String("VSS_SOC_Discarded:" + String(VED_Shnt.veValue[VSS.iSOC])).c_str(), true);
            }
            else
            {
              // new SOC seems ok
              VSS.SOC = atof(VED_Shnt.veValue[VSS.iSOC]) / 10;
            }
          }
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
    }
    else
    {
      if ((UptimeSeconds - VSS.lastUpdate) > VED_TIMEOUT)
      {
        // Connection timed out (due to no valid data within VED_TIMEOUT)
        VSS.ConnStat = 2;
      }
      else
      {
        // invalid or incomplete frame received
        // this happens during regular (non-blocking) operation of VeDirectFrameHandler, so set ConnStat to OK
        VSS.ConnStat = 1;
      }
      // grant time for background tasks
      delay(1);
    }
  }
  else
  {
    if ((UptimeSeconds - VSS.lastUpdate) > VED_TIMEOUT)
    {
      // Connection timed out (no data received at all within VED_TIMEOUT)
      VSS.ConnStat = 2;
    }
  }

  //
  // Gather data from Daly BMS
  //
  if ((UptimeSeconds - BMSLastDataUpdate) >= DALY_UPDATE_INTERVAL || FirstLoop)
  {
    // Fetch data from BMS
    // ATTN: takes between 640 and 710ms!
    BMSresponding = bms.update();
    if (BMSresponding)
    {
      // data set received
      BMSLastValidData = UptimeSeconds;
      BMSConnStat = 1;
    }
    else
    {
      if ((UptimeSeconds - BMSLastValidData) > DALY_TIMEOUT)
      {
        // Timeout limit reached
        BMSConnStat = 2;
      }
      else
      {
        // Not yet in timeout (readout failed)
        BMSConnStat = 3;
      }
    }
    BMSLastDataUpdate = UptimeSeconds;
  }

  //
  // Gather data from OneWire sensors
#ifdef ENA_ONEWIRE // Optional OneWire support
  if ((UptimeSeconds - OWLastDataUpdate) >= OW_UPDATE_INTERVAL || FirstLoop)
  {
    OWtemp.requestTemperatures(); // blocking call
    if (FirstLoop)
    {
      // Check if the correct amount of sensors were found
      if (!OWtemp.getDeviceCount() == NUM_OWTEMP)
      {
        // incorrect amount of sensors found
        mqttClt.publish(t_Ctrl_StatT, String("OW_Init_numSens_Fail:" + String(OWtemp.getDeviceCount())).c_str(), true);
        OWConnStat = 4;
      }
    }
    // Get OW temperatures
    if (!OWConnStat == 4)
    {
      for (int i = 0; i < NUM_OWTEMP; i++)
      {
        OW_SensorData[i] = OWtemp.getTempCByIndex(i);
        if (OW_SensorData[i] == DEVICE_DISCONNECTED_C || OW_SensorData[i] == DEVICE_FAULT_OPEN_C)
        {
          // sensor fault - end this read cycle
          OWConnStat = 3;
          break;
        }
        else
        {
          OWConnStat = 1;
        }
      }
      if (OWConnStat <= 1)
      {
        OWLastDataUpdate = UptimeSeconds;
      }
      else
      {
        if ((UptimeSeconds - OWLastDataUpdate) > OW_TIMEOUT)
          // Connection timed out
          OWConnStat = 2;
      }
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
      mqttClt.publish(t_Ctrl_StatT, String("Startup Firmware v" + String(FIRMWARE_VERSION)).c_str(), true);
    }
    mqttClt.publish(t_Ctrl_StatU, String(UptimeSeconds).c_str(), true);
    mqttClt.publish(t_Ctrl_actSSR1, String(Bool_Decoder[(int)Ctrl_SSR1_actState]).c_str(), true);
    mqttClt.publish(t_Ctrl_actSSR2, String(Bool_Decoder[(int)Ctrl_SSR2_actState]).c_str(), true);

    // Daly BMS
    if (BMSConnStat <= 1)
    {
      // Send cell voltages
      for (int i = 0; i < bms.get.numberOfCells; i++)
      {
        int j = i + 1;
        String MqttTopStr = String(t_DV_C_Templ) + String(j) + "V";
        mqttClt.publish(MqttTopStr.c_str(), String((bms.get.cellVmV[i] / 1000), 3).c_str(), true);
      }
      // send any other more or less useful stuff
      mqttClt.publish(t_DSOC, String(bms.get.packSOC, 0).c_str(), true);
      mqttClt.publish(t_DV, String(bms.get.packVoltage, 2).c_str(), true);
      mqttClt.publish(t_DdV, String(bms.get.cellDiff, 0).c_str(), true);
      mqttClt.publish(t_DI, String(bms.get.packCurrent, 2).c_str(), true);
      mqttClt.publish(t_DLSw, String(Bool_Decoder[(int)bms.get.disChargeFetState]).c_str(), true);
      mqttClt.publish(t_DCSw, String(Bool_Decoder[(int)bms.get.chargeFetState]).c_str(), true);
      mqttClt.publish(t_DTemp, String(bms.get.tempAverage, 0).c_str(), true);
      mqttClt.publish(t_D_CSTAT, String(ConnStat_Decoder[BMSConnStat]).c_str(), true);
      // Add some delay for WiFi processing
      delay(100);
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_D_CSTAT, String(ConnStat_Decoder[BMSConnStat]).c_str(), true);
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
    if (OWConnStat <= 1)
    {
      // Send sensor data
      for (int i = 0; i < NUM_OWTEMP; i++)
      {
        int j = i + 1;
        String MqttTopStr = String(t_OW_TEMP_Templ) + String(j);
        mqttClt.publish(MqttTopStr.c_str(), String(OW_SensorData[i], 0).c_str(), true);
      }
    }
    else
    {
      // report (broken) connection state
      mqttClt.publish(t_OW_CSTAT, String(ConnStat_Decoder[OWConnStat]).c_str(), true);
    }
#endif // ENA_ONEWIRE

    // Add some more delay for WiFi processing
    delay(100);
  }

  //
  // Load Controller (SSR1)
  //
  // If SOC has reached the configured charge limit, enable load
  if (!Ctrl_SSR1_autoSetState && VSS.SOC >= ENABLE_LOAD_SOC)
  {
    Ctrl_SSR1 = 1;
    Ctrl_SSR1_autoSetState = true;
    mqttClt.publish(t_Ctrl_StatT, String("SSR1_ON_SoC:" + String(VSS.SOC, 0)).c_str(), true);
  }
  // if we have a really sunny day, enable load at the configured HIGH_PV limits
  else if (!Ctrl_SSR1_autoSetState && VCHRG1.Avg_PPV >= HIGH_PV_AVG_PWR && VSS.SOC >= HIGH_PV_EN_LOAD_SOC)
  {
    Ctrl_SSR1 = 1;
    Ctrl_SSR1_autoSetState = true;
    mqttClt.publish(t_Ctrl_StatT, String("SSR1_ON_HiPV_SoC:" + String(VSS.SOC, 0)).c_str(), true);
  }
  // if firmware has just started, enable load when SOC is at least BOOT_EN_LOAD_SOC
  else if (!Ctrl_SSR1_autoSetState && FirstLoop && VSS.SOC >= BOOT_EN_LOAD_SOC)
  {
    Ctrl_SSR1 = 1;
    Ctrl_SSR1_autoSetState = true;
    mqttClt.publish(t_Ctrl_StatT, String("SSR1_ON_Boot_SoC:" + String(VSS.SOC, 0)).c_str(), true);
  }

  // Disable SSR1 when DISABLE_LOAD_SOC is reached
  // This must also trigger if the load was enabled manually, so use effective SSR1_actState
  if (Ctrl_SSR1_actState && VSS.SOC <= DISABLE_LOAD_SOC)
  {
    Ctrl_SSR1 = 0;
    Ctrl_SSR1_autoSetState = false;
    mqttClt.publish(t_Ctrl_StatT, String("SSR1_OFF_SoC:" + String(VSS.SOC, 0)).c_str(), true);
  }

  //
  // Active Balancer Controller (SSR2)
  //
  if (Ctrl_SSR2_actState)
  {
    // Balancer is active
    // ..check if any cell is below BAL_OFF_CELLV
    for (int i = 0; i < bms.get.numberOfCells; i++)
    {
      if (bms.get.cellVmV[i] < BAL_OFF_CELLV)
      {
        // Low cell voltage threshold reached, disable Balancer
        Ctrl_SSR2 = 0;
        mqttClt.publish(t_Ctrl_StatT, String("SSR2_OFF_C" + String((i + 1)) + "_Vlow").c_str(), true);
        break;
      }
    }
  }
  else
  {
    // Balancer is inactive
    // If voltage difference is too high
    if (bms.get.cellDiff > BAL_ON_CELLDIFF)
    {
      // ..check if any cell is above BAL_ON_CELLV
      for (int i = 0; i < bms.get.numberOfCells; i++)
      {
        if (bms.get.cellVmV[i] > BAL_ON_CELLV)
        {
          // High cell voltage threshold reached, enable Balancer
          Ctrl_SSR2 = 1;
          mqttClt.publish(t_Ctrl_StatT, String("SSR2_ON_C" + String((i + 1)) + "_Vhi").c_str(), true);
          // end loop, one cell above threshold is enough
          break;
        }
      }
    }
  }

  //
  // Set desired switch states
  //
  if (Ctrl_DalyChSw < 2)
  {
    // Set desired Charge Switch state
    switch (Ctrl_DalyChSw)
    {
    case 0:
      bms.setChargeMOS(false);
      break;
    case 1:
      bms.setChargeMOS(true);
      break;
    }
    // desired state set, reset to "do not change"
    Ctrl_DalyChSw = 2;
    mqttClt.publish(t_Ctrl_CSw, String("dnc").c_str(), true);
  }
  if (Ctrl_DalyLoadSw < 2)
  {
    // Set desired Discharge Switch state
    switch (Ctrl_DalyLoadSw)
    {
    case 0:
      bms.setDischargeMOS(false);
      break;
    case 1:
      bms.setDischargeMOS(true);
      break;
    }
    // desired state set, reset to "do not change"
    Ctrl_DalyLoadSw = 2;
    mqttClt.publish(t_Ctrl_LSw, String("dnc").c_str(), true);
  }

  //
  // Set desired SSR1 state
  //
  if (Ctrl_SSR1 < 2)
  {
    // Set desired SSR1 state
    switch (Ctrl_SSR1)
    {
    case 0:
      digitalWrite(SSR1, LOW);
      Ctrl_SSR1_actState = false;
      break;
    case 1:
      digitalWrite(SSR1, HIGH);
      Ctrl_SSR1_actState = true;
      break;
    }
    // desired state set, reset to "do not change"
    Ctrl_SSR1 = 2;
    mqttClt.publish(t_Ctrl_SSR1, String("dnc").c_str(), true);
    // and publish active state immediately for better responsiveness in UI
    mqttClt.publish(t_Ctrl_actSSR1, String(Bool_Decoder[(int)Ctrl_SSR1_actState]).c_str(), true);
  }

  //
  // Set desired SSR2 state
  //
  if (Ctrl_SSR2 < 2)
  {
    // Set desired SSR2 state
    switch (Ctrl_SSR2)
    {
    case 0:
      digitalWrite(SSR2, LOW);
      Ctrl_SSR2_actState = false;
      break;
    case 1:
      digitalWrite(SSR2, HIGH);
      Ctrl_SSR2_actState = true;
      break;
    }
    // desired state set, reset to "do not change"
    Ctrl_SSR2 = 2;
    mqttClt.publish(t_Ctrl_SSR2, String("dnc").c_str(), true);
    // and publish active state immediately for better responsiveness in UI
    mqttClt.publish(t_Ctrl_actSSR2, String(Bool_Decoder[(int)Ctrl_SSR2_actState]).c_str(), true);
  }

  // Reset FirstLoop
  if (FirstLoop)
  {
    FirstLoop = false;
  }

  // Add some more delay for WiFi processing
  delay(100);

#ifdef ONBOARD_LED
  // Toggle LED at each loop
  ToggleLed(LED, 100, 4);
#endif
}
