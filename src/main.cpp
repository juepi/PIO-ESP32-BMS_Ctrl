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
  static unsigned long start_user_loop = 0;
  static unsigned long duration_user_loop = 0;
  static unsigned long netfail_reconn_millis = 0;

  // Handle network / MQTT broker connection failures (NET_OUTAGE=1)
  if (NetFailure)
  {
    // Currently no network available, try to recover
    if (millis() >= netfail_reconn_millis)
    {
      if (WiFi.isConnected())
      {
        // Try to reconnect to Broker
        MqttUpdater();
      }
      if (NetFailure)
      {
        // Still no network available, try to recover every NET_RECONNECT_INTERVAL
        netfail_reconn_millis = millis() + NET_RECONNECT_INTERVAL;
      }
      else
      {
        // Recovered from network outage
        NetRecoveryMillis = millis();
        netfail_reconn_millis = 0;
      }
    }
  }

  if (!NetFailure)
  {
    // Check connection to MQTT broker, subscribe and update topics
    MqttUpdater();

    // Handle OTA updates
    if (OTAUpdateHandler())
    {
      // OTA Update in progress, restart main loop
      return;
    }
  }

  // Run user specific loop and measure duration
  start_user_loop = millis();
  user_loop();
  duration_user_loop = millis() - start_user_loop;

  // Spare some CPU time for background tasks (if we're not in a hurry)
  if (duration_user_loop < 100)
  {
    delay(100);
  }

#ifdef READVCC
  // Read VCC and publish to MQTT
  // Might not work correctly!
  VCC = VDIV * VFULL_SCALE * float(analogRead(VBAT_ADC_PIN)) / ADC_MAXVAL;
  mqttClt.publish(vcc_topic, String(VCC).c_str(), true);
  DEBUG_PRINTLN("VCC = " + String(VCC) + " V");
  DEBUG_PRINT("Raw ADC Pin readout: ");
  DEBUG_PRINTLN(analogRead(VBAT_ADC_PIN));
  delay(100);
#endif

#ifdef E32_DEEP_SLEEP
  // disconnect WiFi and go to sleep
  DEBUG_PRINTLN("Good night for " + String(DS_DURATION_MIN) + " minutes.");
  WiFi.disconnect();
  ESP.deepSleep(DS_DURATION_MIN * 60000000);
  // ATTN: Sketch continues to run for a short time after initiating DeepSleep, so pause here
  delay(5000);
#endif
}