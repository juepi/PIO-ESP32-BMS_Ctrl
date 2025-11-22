/*
 *   ESP32 Template
 *   Common Function declarations
 */
#ifndef COMMON_FUNCTIONS_H
#define COMMON_FUNCTIONS_H

#include <Arduino.h>
#include "mqtt-ota-config.h"

//
// Declare common functions
//
extern void ToggleLed(int PIN, int WaitTime, int Count);
extern void MqttCallback(char *topic, byte *payload, unsigned int length);
extern bool MqttSubscribe(const char *Topic);
extern bool MqttConnectToBroker();
extern void MqttUpdater();
extern void MqttDelay(uint32_t delayms);
extern bool OTAUpdateHandler();
extern void wifi_up();
extern void wifi_down();

//
// Declare common global vars
//
extern bool JustBooted;
extern bool DelayDeepSleep;
extern uint32_t UptimeSeconds;


//
// C++ Classes
//

// Calculate moving average (FIFO) of a given number of float measurements
class MovingAverage {
public:
    // Constructor: Initializes the moving average with a specific capacity (window size).
    MovingAverage(size_t capacity);

    // Explicitly delete copy constructor and assignment operator to prevent issues.
    MovingAverage(const MovingAverage&) = delete;
    MovingAverage& operator=(const MovingAverage&) = delete;

    /**
     * Adds a new measurement value and updates the average.
     * @param new_value The new measurement value (float).
     */
    void addValue(float new_value);

    /**
     * Calculates and returns the current moving average.
     * @return The calculated average (float).
     */
    float getAverage() const;

private:
    std::vector<float> values_; // Stores the measurement values as floats.
    size_t capacity_;            // Maximum number of values to store (the "window size").
    size_t count_;               // Current number of stored values.
    size_t next_index_;          // Index where the next value will be stored (circular buffer logic).
    float sum_;                  // Current sum of all stored values (float).
};

#endif // COMMON_FUNCTIONS_H