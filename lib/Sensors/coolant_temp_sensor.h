/**
 * @file coolant_temp_sensor.h
 * @brief DS18B20 coolant temperature sensor driver.
 *
 * Abstracts the DS18B20 OneWire interface for real hardware, and
 * provides simulated temperature in EMULATOR_MODE.
 *
 * The caller reads the sensor periodically and feeds the value to
 * HeaterSafety::feedCoolantTemp().
 */

#ifndef COOLANT_TEMP_SENSOR_H
#define COOLANT_TEMP_SENSOR_H

#include <Arduino.h>

#ifndef EMULATOR_MODE
#include <OneWire.h>
#include <DallasTemperature.h>
#endif

class CoolantTempSensor {
public:
    /**
     * @param pin  GPIO pin for OneWire bus (ignored in EMULATOR_MODE).
     */
    explicit CoolantTempSensor(int pin);

    /**
     * @brief Initialize sensor hardware. Call once in setup().
     * @return Number of DS18B20 devices found (0 in EMULATOR_MODE).
     */
    int begin();

    /**
     * @brief Request and read the temperature.
     *
     * In real mode: triggers a OneWire conversion and reads the first
     * device on the bus.  In EMULATOR_MODE: returns the simulated value.
     *
     * @return Temperature in °C, or NAN if the sensor is not present
     *         or the reading is invalid.
     */
    float read();

#ifdef EMULATOR_MODE
    /**
     * @brief Set the simulated coolant temperature.
     * @param tempC  Temperature in °C.
     */
    void setSimulated(float tempC) { simTemp_ = tempC; }
#endif

    /** @brief Latest reading (does not trigger a new conversion). */
    float lastReading() const { return lastTemp_; }

    /** @brief True if at least one DS18B20 was found at begin(). */
    bool isPresent() const { return present_; }

private:
    int   pin_;
    float lastTemp_;
    bool  present_;

#ifdef EMULATOR_MODE
    float simTemp_;
#else
    OneWire           oneWire_;
    DallasTemperature ds_;
#endif
};

#endif // COOLANT_TEMP_SENSOR_H
