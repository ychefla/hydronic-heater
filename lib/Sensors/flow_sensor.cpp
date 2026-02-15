/**
 * @file flow_sensor.cpp
 * @brief Coolant flow sensor implementation.
 */

#include "flow_sensor.h"

#ifndef EMULATOR_MODE
// Static ISR counter
volatile uint32_t FlowSensor::pulseCount_ = 0;
#endif

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

FlowSensor::FlowSensor(int pin)
    : pin_(pin)
    , flowRate_(0.0f)
    , lastCalcMs_(0)
    , lastPulseMs_(0)
#ifdef EMULATOR_MODE
    , simFlowRate_(3.0f)   // Default: healthy 3 L/min flow
#endif
{
}

// ---------------------------------------------------------------------------
// begin
// ---------------------------------------------------------------------------

void FlowSensor::begin() {
#ifdef EMULATOR_MODE
    Serial.println("[FlowSensor] EMULATOR — use setSimulated() to change");
#else
    if (pin_ >= 0) {
        pinMode(pin_, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(pin_), pulseISR, FALLING);
        lastPulseMs_ = millis();
        Serial.printf("[FlowSensor] Pulse counter on GPIO %d\n", pin_);
    } else {
        Serial.println("[FlowSensor] No flow sensor configured (pin = -1)");
    }
#endif
}

// ---------------------------------------------------------------------------
// ISR
// ---------------------------------------------------------------------------

#ifndef EMULATOR_MODE
void IRAM_ATTR FlowSensor::pulseISR() {
    pulseCount_++;
}
#endif

// ---------------------------------------------------------------------------
// update
// ---------------------------------------------------------------------------

float FlowSensor::update() {
#ifdef EMULATOR_MODE
    flowRate_    = simFlowRate_;
    lastPulseMs_ = (simFlowRate_ > 0.0f) ? millis() : lastPulseMs_;
    return flowRate_;
#else
    if (pin_ < 0) {
        flowRate_ = 0.0f;
        return flowRate_;
    }

    unsigned long now     = millis();
    unsigned long elapsed = now - lastCalcMs_;

    if (elapsed < 1000) {
        return flowRate_;  // Too soon — keep previous value
    }

    noInterrupts();
    uint32_t pulses = pulseCount_;
    pulseCount_ = 0;
    interrupts();

    if (pulses > 0) {
        lastPulseMs_ = now;
    }

    flowRate_    = (pulses / FLOW_PULSES_PER_LITER) * (60000.0f / elapsed);
    lastCalcMs_  = now;
    return flowRate_;
#endif
}
