/**
 * @file coolant_temp_sensor.cpp
 * @brief DS18B20 coolant temperature sensor implementation.
 */

#include "coolant_temp_sensor.h"

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

CoolantTempSensor::CoolantTempSensor(int pin)
    : pin_(pin)
    , lastTemp_(NAN)
    , present_(false)
#ifdef EMULATOR_MODE
    , simTemp_(45.0f)    // Default simulated coolant temp
#else
    , oneWire_(pin)
    , ds_(&oneWire_)
#endif
{
}

// ---------------------------------------------------------------------------
// begin
// ---------------------------------------------------------------------------

int CoolantTempSensor::begin() {
#ifdef EMULATOR_MODE
    present_ = true;
    Serial.println("[CoolantTemp] EMULATOR — use setSimulated() to change");
    return 0;
#else
    ds_.begin();
    int count = ds_.getDeviceCount();
    present_ = (count > 0);
    Serial.printf("[CoolantTemp] Found %d DS18B20 sensor(s) on GPIO %d\n",
                  count, pin_);
    return count;
#endif
}

// ---------------------------------------------------------------------------
// read
// ---------------------------------------------------------------------------

float CoolantTempSensor::read() {
#ifdef EMULATOR_MODE
    lastTemp_ = simTemp_;
#else
    if (!present_) {
        lastTemp_ = NAN;
        return lastTemp_;
    }
    ds_.requestTemperatures();
    float t = ds_.getTempCByIndex(0);
    // DallasTemperature returns DEVICE_DISCONNECTED_C (-127) on failure
    if (t <= -55.0f || t > 125.0f) {
        lastTemp_ = NAN;
    } else {
        lastTemp_ = t;
    }
#endif
    return lastTemp_;
}
