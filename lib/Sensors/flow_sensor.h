/**
 * @file flow_sensor.h
 * @brief Coolant flow sensor driver (pulse-counter).
 *
 * In real mode: attaches an ISR to the flow sensor GPIO, counts
 * pulses, and calculates the flow rate in litres per minute.
 *
 * In EMULATOR_MODE: returns a simulated flow rate set via
 * setSimulated().
 *
 * The caller reads the flow rate periodically and feeds it to
 * HeaterSafety::feedFlowRate().
 */

#ifndef FLOW_SENSOR_H
#define FLOW_SENSOR_H

#include <Arduino.h>

/// Typical YF-S201 / YF-B1 hall-effect flow sensor calibration.
static constexpr float FLOW_PULSES_PER_LITER = 7.5f;

class FlowSensor {
public:
    /**
     * @param pin  GPIO pin for the flow sensor (pulse output).
     *             Pass -1 to disable (no sensor installed).
     *             Ignored in EMULATOR_MODE.
     */
    explicit FlowSensor(int pin);

    /**
     * @brief Initialize GPIO and attach interrupt. Call once in setup().
     */
    void begin();

    /**
     * @brief Update flow rate calculation.
     *
     * In real mode: reads the ISR pulse counter, calculates L/min,
     * resets the counter. Call this at a regular interval (≥ 1 s).
     *
     * In EMULATOR_MODE: returns the simulated value unchanged.
     *
     * @return Flow rate in litres per minute.
     */
    float update();

    /** @brief Latest computed flow rate (L/min). */
    float getFlowRate() const { return flowRate_; }

    /** @brief True if a sensor pin was configured. */
    bool isInstalled() const { return pin_ >= 0; }

    /** @brief Millis timestamp of the last detected pulse (0 if none). */
    unsigned long lastPulseTime() const { return lastPulseMs_; }

#ifdef EMULATOR_MODE
    /**
     * @brief Set the simulated flow rate.
     * @param lpm  Flow rate in litres per minute.
     */
    void setSimulated(float lpm) { simFlowRate_ = lpm; }
#endif

    // ISR (public for IRAM_ATTR linkage)
#ifndef EMULATOR_MODE
    static void IRAM_ATTR pulseISR();
#endif

private:
    int           pin_;
    float         flowRate_;
    unsigned long lastCalcMs_;
    unsigned long lastPulseMs_;

#ifdef EMULATOR_MODE
    float simFlowRate_;
#else
    static volatile uint32_t pulseCount_;
#endif
};

#endif // FLOW_SENSOR_H
