/*
 * ESP32 Template
 * ==================
 *
 * Includes useful functions like
 * - DeepSleep
 * - MQTT
 * - OTA Updates (ATTN: requires MQTT!)
 *
 */
#include <setup.h>

/*
 * Main Loop
 * ========================================================================
 */
void loop()
{
  // Check connection to MQTT broker, subscribe and update topics
  MqttUpdater();

#ifdef OTA_UPDATE
  // Handle OTA updates
  if (OTAUpdateHandler())
  {
    // OTA Update in progress, restart main loop
    return;
  }
#endif

  // Run user specific loop
  user_loop();

#ifdef READVCC
  // Read VCC and publish to MQTT
  // Might not work correctly!
  VCC = VDIV * VFULL_SCALE * float(analogRead(VBAT_ADC_PIN)) / ADC_MAXVAL;
  mqttClt.publish(vcc_topic, String(VCC).c_str(), true);
  DEBUG_PRINTLN("VCC = " + String(VCC) + " V");
  DEBUG_PRINT("Raw ADC Pin readout: ");
  DEBUG_PRINTLN(analogRead(VBAT_ADC_PIN));
  WIFI_CLTNAME.flush();
#endif

#ifdef E32_DEEP_SLEEP
  // disconnect WiFi and go to sleep
  DEBUG_PRINTLN("Good night for " + String(DS_DURATION_MIN) + " minutes.");
  WIFI_CLTNAME.flush();
  WiFi.disconnect();
  ESP.deepSleep(DS_DURATION_MIN * 60000000);
  // ATTN: Sketch continues to run for a short time after initiating DeepSleep, so pause here
  delay(5000);
#else
  DEBUG_PRINTLN("Loop finished, DeepSleep disabled. Restarting..");
#endif
  // Flush WiFi TX data
  WIFI_CLTNAME.flush();
}