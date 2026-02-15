/**
 * @file heater_safety.h
 * @brief Heater safety monitor — independent watchdog layer.
 *
 * Implements the safety requirements from SAFETY.md:
 *   - SAFE-T1: Coolant overheat (DS18B20 > 95°C → stop)
 *   - SAFE-F1: Coolant flow loss (no pulses → stop)
 *   - SAFE-U1: UART watchdog (no valid frames → stop)
 *   - SAFE-E1: Heater error code (non-zero → stop)
 *
 * All safety actions are STOP commands — the heater manages its own
 * safe shutdown sequence (cooling fan etc.).
 *
 * @note This module does NOT own sensors. External sensor drivers
 *       (CoolantTempSensor, FlowSensor) feed values in via
 *       feedCoolantTemp() and feedFlowRate(). This keeps the safety
 *       layer independent from hardware/emulator details.
 */

#ifndef HEATER_SAFETY_H
#define HEATER_SAFETY_H

#include <Arduino.h>
#include "autoterm_uart.h"

// ---------------------------------------------------------------------------
// Thresholds (matching SAFETY.md)
// ---------------------------------------------------------------------------
static constexpr float    COOLANT_OVERHEAT_C      = 95.0f;  ///< SAFE-T1 trigger
static constexpr float    COOLANT_RECOVERY_C       = 70.0f;  ///< Manual reset below this
static constexpr uint32_t FLOW_TIMEOUT_MS          = 5000;   ///< SAFE-F1: no pulses
static constexpr uint32_t UART_SAFETY_TIMEOUT_MS   = 15000;  ///< SAFE-U1: no valid frames
static constexpr uint32_t SAFETY_CHECK_INTERVAL_MS = 500;    ///< How often to run checks

// ---------------------------------------------------------------------------
// Safety trip reasons (bitmask)
// ---------------------------------------------------------------------------
enum SafetyTrip : uint8_t {
    TRIP_NONE           = 0x00,
    TRIP_COOLANT_OVERHEAT = 0x01,  ///< SAFE-T1
    TRIP_FLOW_LOSS        = 0x02,  ///< SAFE-F1
    TRIP_UART_TIMEOUT     = 0x04,  ///< SAFE-U1
    TRIP_HEATER_ERROR     = 0x08   ///< SAFE-E1
};

/**
 * @brief Return a human-readable name for a safety trip.
 */
const char* safetyTripName(SafetyTrip trip);

/**
 * @brief Heater safety monitor.
 *
 * Call update() in the main loop. If a safety condition trips,
 * the monitor sends a SHUTDOWN to the heater and enters TRIPPED state.
 * Recovery requires manual reset (clearTrip()) after conditions normalize.
 */
class HeaterSafety {
public:
    /**
     * @param heater    Reference to the AutotermUart driver.
     * @param hasFlowSensor  True if a flow sensor is installed.
     *                       When false, SAFE-F1 checks are skipped.
     */
    explicit HeaterSafety(AutotermUart& heater, bool hasFlowSensor = false);

    /**
     * @brief Initialize safety monitor. Call once in setup().
     */
    void begin();

    /**
     * @brief Run safety checks. Call every loop iteration.
     *
     * Internally rate-limited to SAFETY_CHECK_INTERVAL_MS.
     */
    void update();

    /**
     * @brief Feed the coolant temperature from a DS18B20 reading.
     *
     * Call this whenever you get a fresh DS18B20 reading on the
     * coolant return line. The safety monitor does not own the
     * OneWire bus — the caller does.
     *
     * @param tempC  Temperature in °C. Pass NAN if sensor invalid.
     */
    void feedCoolantTemp(float tempC);

    /**
     * @brief Feed the coolant flow rate from an external FlowSensor.
     *
     * Call this whenever the FlowSensor updates. The safety monitor
     * uses the flow rate and the timestamp to detect flow loss.
     *
     * @param lpm        Flow rate in litres per minute.
     * @param pulseTime  millis() timestamp of the last detected pulse.
     *                   Pass 0 if unknown.
     */
    void feedFlowRate(float lpm, unsigned long pulseTime = 0);

    /**
     * @brief Clear a trip and return to normal monitoring.
     *
     * Only succeeds if:
     *   - Coolant is below COOLANT_RECOVERY_C (for overheat trips)
     *   - Heater error code is 0
     *   - Flow is present (if flow sensor installed)
     *
     * @return true if trip was cleared.
     */
    bool clearTrip();

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    /** @brief True if any safety condition is currently tripped. */
    bool isTripped() const { return tripReason_ != TRIP_NONE; }

    /** @brief Bitmask of active trip reasons. */
    uint8_t getTripReason() const { return tripReason_; }

    /** @brief Latest coolant temperature fed in. */
    float getCoolantTemp() const { return coolantTemp_; }

    /** @brief Current flow rate (L/min), 0 if no sensor. */
    float getFlowRate() const { return flowRate_; }

private:
    AutotermUart& heater_;
    bool          hasFlowSensor_;

    // Coolant temperature (fed externally from CoolantTempSensor)
    float         coolantTemp_;
    bool          coolantValid_;

    // Flow sensor (fed externally from FlowSensor)
    float         flowRate_;         ///< L/min
    unsigned long lastFlowPulseMs_;  ///< Last time a pulse was seen

    // Safety state
    uint8_t       tripReason_;
    unsigned long lastCheckMs_;

    // Internal checks
    void checkCoolantOverheat();
    void checkFlowLoss();
    void checkUartTimeout();
    void checkHeaterError();
    void triggerTrip(SafetyTrip reason);
};

#endif // HEATER_SAFETY_H
