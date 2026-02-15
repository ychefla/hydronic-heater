/**
 * @file autoterm_emulator.cpp
 * @brief Autoterm Flow 5D heater emulator implementation.
 */

#include "autoterm_emulator.h"
#include <string.h>

// ===========================================================================
// Constructor / begin
// ===========================================================================

AutotermEmulator::AutotermEmulator(HardwareSerial& serial, int rxPin, int txPin)
    : serial_(serial)
    , rxPin_(rxPin)
    , txPin_(txPin)
    , rxIdx_(0)
    , rxInFrame_(false)
    , state_(AutotermState::Off)
    , error_(AutotermError::None)
    , stateEnteredMs_(0)
    , operatingMode_(MODE_BY_POWER)
    , targetTemp_(0xFF)
    , powerLevel_(0)
    , coreTemp_(EMU_CORE_TEMP_AMBIENT)
    , voltage_(EMU_VOLTAGE_NOMINAL)
    , voltageOverride_(NAN)
    , lastPanelTemp_(20)
    , lastSimUpdateMs_(0)
    , framesRx_(0)
    , framesTx_(0) {
}

void AutotermEmulator::begin() {
    serial_.begin(AUTOTERM_BAUD, AUTOTERM_DATA_BITS, rxPin_, txPin_);
    stateEnteredMs_ = millis();
    lastSimUpdateMs_ = millis();

    Serial.println("[Emulator] Autoterm Flow 5D emulator started");
    Serial.printf("[Emulator] UART on pins RX=%d TX=%d @ %d baud\n",
                  rxPin_, txPin_, AUTOTERM_BAUD);
    Serial.println("[Emulator] Heater state: Off");
}

// ===========================================================================
// update()
// ===========================================================================

void AutotermEmulator::update() {
    readSerial();
    updateStateMachine();
    updateSimulation();
}

// ===========================================================================
// Error injection
// ===========================================================================

void AutotermEmulator::injectError(AutotermError err) {
    error_ = err;
    if (err != AutotermError::None) {
        Serial.printf("[Emulator] ERROR INJECTED: %s\n", autotermErrorName(err));
        // Real heater would go to shutdown on error
        if (state_ == AutotermState::Running ||
            state_ == AutotermState::Warming ||
            state_ == AutotermState::Starting) {
            transitionTo(AutotermState::ShuttingDown);
        }
    } else {
        Serial.println("[Emulator] Error cleared");
    }
}

void AutotermEmulator::overrideVoltage(float volts) {
    voltageOverride_ = volts;
    if (!isnan(volts)) {
        Serial.printf("[Emulator] Voltage override: %.1fV\n", volts);
    } else {
        Serial.println("[Emulator] Voltage override cleared — resuming simulation");
    }
}

void AutotermEmulator::forceState(AutotermState state) {
    transitionTo(state);
    Serial.printf("[Emulator] Forced to state: %s\n", autotermStateName(state));
}

// ===========================================================================
// Serial reading + frame assembly (mirrors driver logic)
// ===========================================================================

void AutotermEmulator::readSerial() {
    while (serial_.available()) {
        uint8_t b = serial_.read();

        if (!rxInFrame_) {
            if (b == FRAME_PREAMBLE) {
                rxBuf_[0] = b;
                rxIdx_ = 1;
                rxInFrame_ = true;
            }
            continue;
        }

        if (rxIdx_ < FRAME_MAX_SIZE) {
            rxBuf_[rxIdx_++] = b;
        } else {
            rxInFrame_ = false;
            rxIdx_ = 0;
            continue;
        }

        if (rxIdx_ < 3) continue;

        uint8_t payloadLen = rxBuf_[2];
        size_t expectedLen = 5 + payloadLen + 2;

        if (rxIdx_ >= expectedLen) {
            processFrame(rxBuf_, expectedLen);
            rxInFrame_ = false;
            rxIdx_ = 0;
        }
    }
}

void AutotermEmulator::processFrame(const uint8_t* buf, size_t len) {
    if (!autotermValidateFrame(buf, len)) {
        return;  // Bad CRC — ignore
    }

    uint8_t sender = buf[1];
    // We only listen to the panel (driver)
    if (sender != SENDER_PANEL) {
        return;
    }

    framesRx_++;

    uint8_t command = buf[4];
    uint8_t payloadLen = buf[2];
    const uint8_t* payload = &buf[5];

    handleCommand(command, payload, payloadLen);
}

// ===========================================================================
// Command handler — emulates heater ECU behavior
// ===========================================================================

void AutotermEmulator::handleCommand(uint8_t command, const uint8_t* payload,
                                      size_t payloadLen) {
    switch (command) {
        case CMD_START:
            if (payloadLen >= 6) {
                operatingMode_ = payload[2];
                targetTemp_    = payload[3];
                powerLevel_    = payload[5];

                if (state_ == AutotermState::Off ||
                    state_ == AutotermState::Standby) {
                    transitionTo(AutotermState::Starting);
                    Serial.printf("[Emulator] START received: mode=%02X temp=%d power=%d\n",
                                  operatingMode_, targetTemp_, powerLevel_);
                }
            }
            // Respond with current status
            sendStatusResponse();
            break;

        case CMD_SHUTDOWN:
            if (state_ != AutotermState::Off &&
                state_ != AutotermState::Cooling) {
                transitionTo(AutotermState::ShuttingDown);
                Serial.println("[Emulator] SHUTDOWN received");
            }
            sendStatusResponse();
            break;

        case CMD_GET_SET:
            if (payloadLen == 0) {
                // Query — respond with current settings
                sendSettingsResponse();
            } else if (payloadLen >= 6) {
                // Set — update settings
                operatingMode_ = payload[2];
                targetTemp_    = payload[3];
                powerLevel_    = payload[5];
                sendSettingsResponse();
            }
            break;

        case CMD_STATUS:
            sendStatusResponse();
            break;

        case CMD_PANEL_TEMP:
            if (payloadLen >= 1) {
                lastPanelTemp_ = payload[0];
            }
            sendPanelTempAck();
            break;

        case CMD_VENTILATION:
            if (payloadLen >= 1) {
                powerLevel_ = payload[0];
                if (state_ == AutotermState::Off ||
                    state_ == AutotermState::Standby) {
                    transitionTo(AutotermState::Ventilation);
                    Serial.printf("[Emulator] VENTILATION received: power=%d\n",
                                  powerLevel_);
                }
            }
            sendStatusResponse();
            break;

        default:
            // Unknown command — ignore
            break;
    }
}

// ===========================================================================
// Response builders
// ===========================================================================

void AutotermEmulator::sendStatusResponse() {
    // Build 10-byte status payload
    uint8_t payload[10] = {};

    payload[0] = static_cast<uint8_t>(state_);
    payload[1] = 0x00;  // Unknown byte 1
    payload[2] = static_cast<uint8_t>(error_);
    payload[3] = 0x00;  // Unknown byte 3 (step?)

    // Voltage as little-endian uint16, ×10
    uint16_t voltRaw = (uint16_t)(voltage_ * 10.0f);
    payload[4] = (uint8_t)(voltRaw & 0xFF);         // Low byte
    payload[5] = (uint8_t)((voltRaw >> 8) & 0xFF);   // High byte

    payload[6] = 0x00;  // Unknown byte 6
    payload[7] = 0x00;  // Unknown byte 7
    payload[8] = (uint8_t)constrain((int)coreTemp_, 0, 255);
    payload[9] = 0x00;  // Unknown byte 9

    sendFrame(CMD_STATUS, payload, sizeof(payload));
}

void AutotermEmulator::sendSettingsResponse() {
    // Echo back current settings in GET/SET format
    uint8_t payload[6] = {
        0xFF, 0xFF,
        operatingMode_,
        targetTemp_,
        0x02,  // Ventilation off
        powerLevel_
    };
    sendFrame(CMD_GET_SET, payload, sizeof(payload));
}

void AutotermEmulator::sendPanelTempAck() {
    // Echo back the panel temp as acknowledgement
    uint8_t payload[1] = { lastPanelTemp_ };
    sendFrame(CMD_PANEL_TEMP, payload, 1);
}

bool AutotermEmulator::sendFrame(uint8_t command, const uint8_t* payload,
                                  size_t payloadLen) {
    uint8_t buf[FRAME_MAX_SIZE];

    // Build frame with SENDER_HEATER (we are the heater)
    size_t idx = 0;
    buf[idx++] = FRAME_PREAMBLE;
    buf[idx++] = SENDER_HEATER;          // 0x04 — we are the heater
    buf[idx++] = (uint8_t)payloadLen;
    buf[idx++] = FRAME_RESERVED;
    buf[idx++] = command;

    if (payload && payloadLen > 0) {
        memcpy(&buf[idx], payload, payloadLen);
        idx += payloadLen;
    }

    // CRC over bytes [1..idx-1]
    uint16_t crc = autotermCRC16(&buf[1], idx - 1);
    buf[idx++] = (uint8_t)(crc >> 8);    // Big-endian
    buf[idx++] = (uint8_t)(crc & 0xFF);

    serial_.write(buf, idx);
    serial_.flush();
    framesTx_++;
    return true;
}

// ===========================================================================
// State machine
// ===========================================================================

void AutotermEmulator::transitionTo(AutotermState newState) {
    if (state_ == newState) return;
    state_ = newState;
    stateEnteredMs_ = millis();
}

void AutotermEmulator::updateStateMachine() {
    unsigned long now = millis();
    unsigned long elapsed = now - stateEnteredMs_;

    switch (state_) {
        case AutotermState::Starting:
            // After startup duration, move to Warming
            if (elapsed >= EMU_STARTING_DURATION_MS) {
                transitionTo(AutotermState::Warming);
                Serial.println("[Emulator] State: Warming");
            }
            break;

        case AutotermState::Warming:
            // After warming duration, move to Running
            if (elapsed >= EMU_WARMING_DURATION_MS) {
                transitionTo(AutotermState::Running);
                Serial.println("[Emulator] State: Running");
            }
            break;

        case AutotermState::Running:
            // Stay running until shutdown or error
            break;

        case AutotermState::ShuttingDown:
            if (elapsed >= EMU_SHUTDOWN_DURATION_MS) {
                transitionTo(AutotermState::Cooling);
                Serial.println("[Emulator] State: Cooling");
            }
            break;

        case AutotermState::Cooling:
            if (elapsed >= EMU_COOLING_DURATION_MS &&
                coreTemp_ <= EMU_CORE_TEMP_AMBIENT + 5.0f) {
                transitionTo(AutotermState::Off);
                error_ = AutotermError::None;  // Clear error on full stop
                Serial.println("[Emulator] State: Off");
            }
            break;

        case AutotermState::Ventilation:
            // Fan-only — stays until shutdown
            break;

        case AutotermState::Off:
        case AutotermState::Standby:
        default:
            break;
    }
}

// ===========================================================================
// Thermal + voltage simulation
// ===========================================================================

void AutotermEmulator::updateSimulation() {
    unsigned long now = millis();
    if (now - lastSimUpdateMs_ < 100) return;  // 10 Hz update
    lastSimUpdateMs_ = now;

    // --- Core temperature ---
    float targetCoreTemp;
    switch (state_) {
        case AutotermState::Starting:
            targetCoreTemp = EMU_CORE_TEMP_AMBIENT + 30.0f;
            break;
        case AutotermState::Warming:
            targetCoreTemp = EMU_CORE_TEMP_RUNNING * 0.7f;
            break;
        case AutotermState::Running:
            // Scale by power level (0-9): at level 0 = 40% of max, level 9 = 100%
            targetCoreTemp = EMU_CORE_TEMP_RUNNING *
                             (0.4f + 0.6f * (powerLevel_ / 9.0f));
            break;
        case AutotermState::Ventilation:
            targetCoreTemp = EMU_CORE_TEMP_AMBIENT + 5.0f;
            break;
        default:
            targetCoreTemp = EMU_CORE_TEMP_AMBIENT;
            break;
    }

    // Ramp toward target
    if (coreTemp_ < targetCoreTemp) {
        coreTemp_ += EMU_CORE_TEMP_RAMP_RATE;
        if (coreTemp_ > targetCoreTemp) coreTemp_ = targetCoreTemp;
    } else if (coreTemp_ > targetCoreTemp) {
        coreTemp_ -= EMU_CORE_TEMP_COOL_RATE;
        if (coreTemp_ < targetCoreTemp) coreTemp_ = targetCoreTemp;
    }

    // --- Voltage ---
    if (!isnan(voltageOverride_)) {
        voltage_ = voltageOverride_;
    } else {
        float baseVoltage = EMU_VOLTAGE_NOMINAL;
        // Slight voltage dip when heater is actively burning
        if (state_ == AutotermState::Running ||
            state_ == AutotermState::Warming ||
            state_ == AutotermState::Starting) {
            baseVoltage -= EMU_VOLTAGE_RUNNING_DIP;
        }
        // Add small random jitter
        float jitter = ((random(0, 200) - 100) / 1000.0f) * EMU_VOLTAGE_JITTER;
        voltage_ = baseVoltage + jitter;
    }
}
