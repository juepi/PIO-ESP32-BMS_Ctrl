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
}

/*
 * User Main Loop
 * ========================================================================
 */
void user_loop()
{
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
    oled.println("WiFi: Connected.");
  }
  else
  {
    oled.println("WiFi: Disconnected!");
  }
  if (mqttClt.connected())
  {
    oled.println("MQTT: Connected.");
  }
  else
  {
    oled.println("MQTT: Disconnected!");
  }
  oled.print("OTA Update req.: ");
  oled.println(String(OTAupdate));
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
  oled.println("SoC: 100%");
  oled.set1X();
  oled.println("Vbat: 13.7V");
  oled.println("Ibat: -1,2A");
  MqttDelay(5000);
}