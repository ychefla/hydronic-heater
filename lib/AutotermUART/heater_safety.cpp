/**
 * @file heater_safety.cpp
 * @brief Heater safety monitor implementation.
 */

#include "heater_safety.h"

// ---------------------------------------------------------------------------
// Trip name
// ---------------------------------------------------------------------------

const char* safetyTripName(SafetyTrip trip) {
    switch (trip) {
        case TRIP_COOLANT_OVERHEAT: return "Coolant Overheat (SAFE-T1)";
        case TRIP_FLOW_LOSS:        return "Flow Loss (SAFE-F1)";
        case TRIP_UART_TIMEOUT:     return "UART Timeout (SAFE-U1)";
        case TRIP_HEATER_ERROR:     return "Heater Error (SAFE-E1)";
        default:                    return "None";
    }
}

// ---------------------------------------------------------------------------
// Constructor / begin
// ---------------------------------------------------------------------------

HeaterSafety::HeaterSafety(AutotermUart& heater, bool hasFlowSensor)
    : heater_(heater)
    , hasFlowSensor_(hasFlowSensor)
    , coolantTemp_(NAN)
    , coolantValid_(false)
    , flowRate_(0.0f)
    , lastFlowPulseMs_(0)
    , tripReason_(TRIP_NONE)
    , lastCheckMs_(0) {
}

void HeaterSafety::begin() {
    if (hasFlowSensor_) {
        Serial.println("[HeaterSafety] Flow sensor enabled — expecting feedFlowRate() calls");
    } else {
        Serial.println("[HeaterSafety] No flow sensor — SAFE-F1 skipped");
    }
    Serial.println("[HeaterSafety] Initialized — all checks active");
}

// ---------------------------------------------------------------------------
// feedCoolantTemp / feedFlowRate
// ---------------------------------------------------------------------------

void HeaterSafety::feedCoolantTemp(float tempC) {
    if (isnan(tempC) || tempC < -55.0f || tempC > 125.0f) {
        coolantValid_ = false;
        return;
    }
    coolantTemp_  = tempC;
    coolantValid_ = true;
}

void HeaterSafety::feedFlowRate(float lpm, unsigned long pulseTime) {
    flowRate_ = lpm;
    if (pulseTime > 0) {
        lastFlowPulseMs_ = pulseTime;
    } else if (lpm > 0.0f) {
        // No explicit timestamp — assume flow is present now
        lastFlowPulseMs_ = millis();
    }
}

// ---------------------------------------------------------------------------
// update()
// ---------------------------------------------------------------------------

void HeaterSafety::update() {
    unsigned long now = millis();
    if (now - lastCheckMs_ < SAFETY_CHECK_INTERVAL_MS) {
        return;
    }
    lastCheckMs_ = now;

    // If already tripped, just keep sending shutdown
    if (isTripped()) {
        // Re-send shutdown periodically to ensure heater obeys
        heater_.shutdown();
        return;
    }

    // Only run safety checks when heater reports it is in an active state
    const auto& st = heater_.getStatus();
    bool heaterActive = (st.state == AutotermState::Starting
                      || st.state == AutotermState::Warming
                      || st.state == AutotermState::Running
                      || st.state == AutotermState::Ventilation);

    // SAFE-U1 and SAFE-E1 always checked
    checkUartTimeout();
    checkHeaterError();

    // SAFE-T1 and SAFE-F1 only when heater is active
    if (heaterActive) {
        checkCoolantOverheat();
        checkFlowLoss();
    }
}

// ---------------------------------------------------------------------------
// Individual checks
// ---------------------------------------------------------------------------

void HeaterSafety::checkCoolantOverheat() {
    if (!coolantValid_) return;

    if (coolantTemp_ > COOLANT_OVERHEAT_C) {
        triggerTrip(TRIP_COOLANT_OVERHEAT);
        Serial.printf("[SAFETY] TRIP: Coolant overheat! %.1f°C > %.1f°C\n",
                      coolantTemp_, COOLANT_OVERHEAT_C);
    }
}

void HeaterSafety::checkFlowLoss() {
    if (!hasFlowSensor_) return;  // No flow sensor

    unsigned long now = millis();
    if (lastFlowPulseMs_ > 0 && (now - lastFlowPulseMs_) > FLOW_TIMEOUT_MS) {
        triggerTrip(TRIP_FLOW_LOSS);
        Serial.println("[SAFETY] TRIP: Coolant flow loss!");
    }
}

void HeaterSafety::checkUartTimeout() {
    // Only trip if we were online and then lost connection
    if (heater_.getLinkState() == UartLinkState::Timeout) {
        triggerTrip(TRIP_UART_TIMEOUT);
        Serial.println("[SAFETY] TRIP: UART timeout — no response from heater");
    }
}

void HeaterSafety::checkHeaterError() {
    const auto& st = heater_.getStatus();
    if (st.valid && st.error != AutotermError::None) {
        triggerTrip(TRIP_HEATER_ERROR);
        Serial.printf("[SAFETY] TRIP: Heater reports error %s\n",
                      autotermErrorName(st.error));
    }
}

// ---------------------------------------------------------------------------
// Trip / clear
// ---------------------------------------------------------------------------

void HeaterSafety::triggerTrip(SafetyTrip reason) {
    if (tripReason_ & reason) return;  // Already tripped for this reason

    tripReason_ |= reason;
    heater_.shutdown();

    Serial.printf("[SAFETY] *** EMERGENCY STOP *** Reason: %s\n",
                  safetyTripName(reason));
}

bool HeaterSafety::clearTrip() {
    // Can only clear if conditions allow
    if (tripReason_ & TRIP_COOLANT_OVERHEAT) {
        if (!coolantValid_ || coolantTemp_ > COOLANT_RECOVERY_C) {
            Serial.printf("[SAFETY] Cannot clear: coolant %.1f°C > %.1f°C recovery threshold\n",
                          coolantTemp_, COOLANT_RECOVERY_C);
            return false;
        }
    }

    if (tripReason_ & TRIP_HEATER_ERROR) {
        const auto& st = heater_.getStatus();
        if (st.valid && st.error != AutotermError::None) {
            Serial.println("[SAFETY] Cannot clear: heater still reporting error");
            return false;
        }
    }

    if (tripReason_ & TRIP_UART_TIMEOUT) {
        if (!heater_.isOnline()) {
            Serial.println("[SAFETY] Cannot clear: UART still offline");
            return false;
        }
    }

    tripReason_ = TRIP_NONE;
    Serial.println("[SAFETY] Trip cleared — returning to normal monitoring");
    return true;
}
