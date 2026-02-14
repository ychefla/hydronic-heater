/**
 * @file autoterm_uart.cpp
 * @brief Autoterm UART driver implementation.
 */

#include "autoterm_uart.h"
#include <string.h>

// ===========================================================================
// Constructor / begin
// ===========================================================================

AutotermUart::AutotermUart(HardwareSerial& serial, int rxPin, int txPin)
    : serial_(serial)
    , rxPin_(rxPin)
    , txPin_(txPin)
    , rxIdx_(0)
    , rxInFrame_(false)
    , lastCycleMs_(0)
    , cycleStep_(0)
    , panelTemp_(20)
    , status_{}
    , linkState_(UartLinkState::Disconnected)
    , lastValidRxMs_(0)
    , frameCount_(0)
    , errorCount_(0)
    , pendingCmd_(false)
    , pendingLen_(0) {
}

void AutotermUart::begin() {
    serial_.begin(AUTOTERM_BAUD, AUTOTERM_DATA_BITS, rxPin_, txPin_);
    lastCycleMs_ = millis();
    linkState_   = UartLinkState::Disconnected;
    rxIdx_       = 0;
    rxInFrame_   = false;

    Serial.println("[AutotermUART] Initialized on pins RX="
                   + String(rxPin_) + " TX=" + String(txPin_)
                   + " @ " + String(AUTOTERM_BAUD) + " baud");
}

// ===========================================================================
// update() — call every loop()
// ===========================================================================

void AutotermUart::update() {
    readSerial();
    updateLinkState();

    unsigned long now = millis();
    if (now - lastCycleMs_ >= COMM_CYCLE_MS) {
        lastCycleMs_ = now;
        runCycleStep();
    }
}

// ===========================================================================
// Commands
// ===========================================================================

bool AutotermUart::start(uint8_t mode, uint8_t targetTemp,
                          uint8_t powerLevel, bool ventilation) {
    uint8_t payload[6] = {
        0xFF, 0xFF,                          // Marker bytes
        mode,                                // Operating mode
        targetTemp,                          // Target temp or 0xFF
        ventilation ? (uint8_t)0x01 : (uint8_t)0x02, // Vent on/off
        powerLevel                           // Power 0-9 or 0xFF
    };

    pendingLen_ = autotermBuildFrame(pendingBuf_, CMD_START, payload, sizeof(payload));
    if (pendingLen_ == 0) return false;
    pendingCmd_ = true;

    Serial.printf("[AutotermUART] START queued: mode=%02X temp=%d power=%d\n",
                  mode, targetTemp, powerLevel);
    return true;
}

bool AutotermUart::shutdown() {
    pendingLen_ = autotermBuildFrame(pendingBuf_, CMD_SHUTDOWN, nullptr, 0);
    if (pendingLen_ == 0) return false;
    pendingCmd_ = true;

    Serial.println("[AutotermUART] SHUTDOWN queued");
    return true;
}

bool AutotermUart::querySettings() {
    uint8_t buf[FRAME_MAX_SIZE];
    size_t len = autotermBuildFrame(buf, CMD_GET_SET, nullptr, 0);
    return sendFrame(buf, len);
}

bool AutotermUart::sendPanelTemp(uint8_t tempC) {
    panelTemp_ = tempC;
    uint8_t payload[1] = { tempC };
    uint8_t buf[FRAME_MAX_SIZE];
    size_t len = autotermBuildFrame(buf, CMD_PANEL_TEMP, payload, 1);
    return sendFrame(buf, len);
}

bool AutotermUart::startVentilation(uint8_t powerLevel) {
    // Ventilation payload: [power, 0x00, 0x00]
    uint8_t payload[3] = { powerLevel, 0x00, 0x00 };

    pendingLen_ = autotermBuildFrame(pendingBuf_, CMD_VENTILATION, payload, sizeof(payload));
    if (pendingLen_ == 0) return false;
    pendingCmd_ = true;

    Serial.printf("[AutotermUART] VENTILATION queued: power=%d\n", powerLevel);
    return true;
}

// ===========================================================================
// Internal: serial reading + frame assembly
// ===========================================================================

void AutotermUart::readSerial() {
    while (serial_.available()) {
        uint8_t b = serial_.read();

        // Look for preamble to start a new frame
        if (!rxInFrame_) {
            if (b == FRAME_PREAMBLE) {
                rxBuf_[0] = b;
                rxIdx_     = 1;
                rxInFrame_ = true;
            }
            continue;
        }

        // Accumulate bytes
        if (rxIdx_ < FRAME_MAX_SIZE) {
            rxBuf_[rxIdx_++] = b;
        } else {
            // Buffer overflow — discard
            rxInFrame_ = false;
            rxIdx_     = 0;
            errorCount_++;
            continue;
        }

        // We need at least 3 bytes to know the length field
        if (rxIdx_ < 3) continue;

        // Check if we have a complete frame
        uint8_t payloadLen  = rxBuf_[2];
        size_t  expectedLen = 5 + payloadLen + 2;  // header(5) + payload + CRC(2)

        if (rxIdx_ >= expectedLen) {
            processFrame(rxBuf_, expectedLen);
            rxInFrame_ = false;
            rxIdx_     = 0;
        }
    }
}

void AutotermUart::processFrame(const uint8_t* buf, size_t len) {
    if (!autotermValidateFrame(buf, len)) {
        errorCount_++;
        return;
    }

    frameCount_++;
    lastValidRxMs_ = millis();

    uint8_t sender  = buf[1];
    uint8_t command = buf[4];
    uint8_t payloadLen = buf[2];
    const uint8_t* payload = &buf[5];

    // We only process responses from the heater
    if (sender != SENDER_HEATER) {
        return;
    }

    switch (command) {
        case CMD_STATUS:
            if (payloadLen >= 10) {
                autotermParseStatus(payload, payloadLen, status_);
            }
            break;

        case CMD_GET_SET:
            // Settings response — could parse if needed in future
            break;

        case CMD_PANEL_TEMP:
            // Heater acknowledges our panel temp — no action
            break;

        default:
            break;
    }
}

// ===========================================================================
// Internal: communication cycle
// ===========================================================================

void AutotermUart::runCycleStep() {
    // If there is a pending user command, send it instead of the cycle step
    if (pendingCmd_ && pendingLen_ > 0) {
        sendFrame(pendingBuf_, pendingLen_);
        pendingCmd_ = false;
        pendingLen_ = 0;
        return;
    }

    // Normal 3-second cycle: step 0→GET, 1→STATUS, 2→PANEL_TEMP
    switch (cycleStep_) {
        case 0: {
            // Query settings
            uint8_t buf[FRAME_MAX_SIZE];
            size_t len = autotermBuildFrame(buf, CMD_GET_SET, nullptr, 0);
            sendFrame(buf, len);
            break;
        }
        case 1: {
            // Request status
            uint8_t buf[FRAME_MAX_SIZE];
            size_t len = autotermBuildFrame(buf, CMD_STATUS, nullptr, 0);
            sendFrame(buf, len);
            break;
        }
        case 2: {
            // Send panel temperature
            uint8_t payload[1] = { panelTemp_ };
            uint8_t buf[FRAME_MAX_SIZE];
            size_t len = autotermBuildFrame(buf, CMD_PANEL_TEMP, payload, 1);
            sendFrame(buf, len);
            break;
        }
    }

    cycleStep_ = (cycleStep_ + 1) % 3;
}

// ===========================================================================
// Internal: send + link state
// ===========================================================================

bool AutotermUart::sendFrame(const uint8_t* buf, size_t len) {
    if (len == 0) return false;
    serial_.write(buf, len);
    serial_.flush();
    return true;
}

void AutotermUart::updateLinkState() {
    unsigned long now = millis();

    if (lastValidRxMs_ == 0) {
        linkState_ = UartLinkState::Disconnected;
    } else if (now - lastValidRxMs_ > UART_TIMEOUT_MS) {
        linkState_ = UartLinkState::Timeout;
    } else {
        linkState_ = UartLinkState::Online;
    }
}
