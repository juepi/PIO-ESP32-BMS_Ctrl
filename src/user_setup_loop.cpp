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

// Declare global user vars
bool BMSresponding = false;

/*
 * User Setup Loop
 * ========================================================================
 */
void user_setup()
{
  Wire.begin(I2C_SDA, I2C_SCL);
  oled.begin(&Adafruit128x32, OLED_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();
  bms.Init();
}

/*
 * User Main Loop
 * ========================================================================
 */
void user_loop()
{

  // Update data from BMS
  if(bms.update())
  {
    BMSresponding = true;
  }
  else
  {
    BMSresponding = false;
  }

  // small OLED lines: 21 characters
  // large (set2X) OLED Lines: 11 Characters
  oled_sys_stat();
  oled_bms_stat();

#ifdef ONBOARD_LED
  // Toggle LED at each loop
  ToggleLed(LED, 500, 4);
#endif
}

void oled_sys_stat()
{
  oled.clear();
  oled.invertDisplay(true);
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
  oled.print("Uptime: ");
  unsigned long secs = millis() / 1000UL;
  oled.println(secs);
  MqttDelay(5000);
  oled.invertDisplay(false);
}

void oled_bms_stat()
{
  oled.clear();
  oled.set2X();
  oled.println("SoC: " + String(bms.get.packSOC,1));
  oled.set1X();
  oled.println("Vbat: " + String(bms.get.packVoltage,2));
  oled.println("Ibat: " + String(bms.get.packCurrent,2));
  MqttDelay(5000);
}