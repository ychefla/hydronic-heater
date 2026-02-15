/**
 * @file autoterm_protocol.cpp
 * @brief Autoterm UART protocol — CRC, frame building, frame parsing.
 */

#include "autoterm_protocol.h"
#include <string.h>

// ---------------------------------------------------------------------------
// State / error name tables
// ---------------------------------------------------------------------------

const char* autotermStateName(AutotermState state) {
    switch (state) {
        case AutotermState::Off:          return "Off";
        case AutotermState::Starting:     return "Starting";
        case AutotermState::Standby:      return "Standby";
        case AutotermState::Warming:      return "Warming";
        case AutotermState::Running:      return "Running";
        case AutotermState::ShuttingDown: return "Shutting Down";
        case AutotermState::Cooling:      return "Cooling";
        case AutotermState::Ventilation:  return "Ventilation";
        default:                          return "Unknown";
    }
}

const char* autotermErrorName(AutotermError err) {
    switch (err) {
        case AutotermError::None:            return "None";
        case AutotermError::Overheating:     return "E01: Overheating";
        case AutotermError::Voltage:         return "E02: Voltage";
        case AutotermError::GlowPlugFailure: return "E03: Glow Plug";
        case AutotermError::FuelPumpFailure: return "E04: Fuel Pump";
        case AutotermError::FlameSensor:     return "E05: Flame Sensor";
        case AutotermError::TempSensor:      return "E06: Temp Sensor";
        case AutotermError::FanFailure:      return "E07: Fan";
        case AutotermError::ControllerFault: return "E08: Controller";
        case AutotermError::IgnitionFailure: return "E09: Ignition";
        case AutotermError::NoFlame:         return "E13: No Flame";
        default:                             return "Unknown Error";
    }
}

// ---------------------------------------------------------------------------
// CRC-16 Modbus (reflected polynomial 0xA001)
// ---------------------------------------------------------------------------

uint16_t autotermCRC16(const uint8_t* data, size_t len) {
    uint16_t crc = CRC_INIT;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ CRC_POLY;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Frame building
// ---------------------------------------------------------------------------

size_t autotermBuildFrame(uint8_t* buf, uint8_t command,
                          const uint8_t* payload, size_t payloadLen) {
    if (payloadLen + FRAME_MIN_SIZE > FRAME_MAX_SIZE) {
        return 0;  // Payload too large
    }

    size_t idx = 0;

    // Header
    buf[idx++] = FRAME_PREAMBLE;     // 0xAA
    buf[idx++] = SENDER_PANEL;       // 0x03 — we are the panel
    buf[idx++] = (uint8_t)payloadLen; // Length = payload byte count
    buf[idx++] = FRAME_RESERVED;     // 0x00
    buf[idx++] = command;

    // Payload
    if (payload != nullptr && payloadLen > 0) {
        memcpy(&buf[idx], payload, payloadLen);
        idx += payloadLen;
    }

    // CRC-16 over bytes [1..idx-1] (sender through end of payload)
    // i.e. everything after preamble, before CRC
    uint16_t crc = autotermCRC16(&buf[1], idx - 1);

    // CRC is stored big-endian in the protocol
    buf[idx++] = (uint8_t)(crc >> 8);   // CRC high
    buf[idx++] = (uint8_t)(crc & 0xFF); // CRC low

    return idx;
}

// ---------------------------------------------------------------------------
// Frame validation
// ---------------------------------------------------------------------------

bool autotermValidateFrame(const uint8_t* buf, size_t len) {
    // Minimum frame size
    if (len < FRAME_MIN_SIZE) {
        return false;
    }

    // Preamble check
    if (buf[0] != FRAME_PREAMBLE) {
        return false;
    }

    // Length field consistency: payload length = buf[2]
    // Total frame size should be: 5 (header) + payloadLen + 2 (CRC)
    uint8_t payloadLen = buf[2];
    size_t expectedLen = 5 + payloadLen + 2;
    if (len < expectedLen) {
        return false;
    }

    // CRC check: compute over bytes [1..end-2] (sender through payload)
    size_t crcDataLen = expectedLen - 3;  // Subtract preamble(1) + CRC(2)
    uint16_t computed = autotermCRC16(&buf[1], crcDataLen);

    // CRC in frame is big-endian
    uint16_t received = ((uint16_t)buf[expectedLen - 2] << 8)
                       | (uint16_t)buf[expectedLen - 1];

    return computed == received;
}

// ---------------------------------------------------------------------------
// Status parsing
// ---------------------------------------------------------------------------

bool autotermParseStatus(const uint8_t* payload, size_t payloadLen,
                         AutotermStatus& out) {
    if (payloadLen < 10) {
        out.valid = false;
        return false;
    }

    memcpy(out.rawBytes, payload, 10);

    out.state    = static_cast<AutotermState>(payload[0]);
    // payload[1] — unknown
    out.error    = static_cast<AutotermError>(payload[2]);
    // payload[3] — unknown (step?)

    // Voltage: little-endian uint16 at bytes 4-5, ×10
    uint16_t rawVolt = (uint16_t)payload[4] | ((uint16_t)payload[5] << 8);
    out.voltage  = rawVolt / 10.0f;

    // payload[6-7] — unknown
    out.coreTemp = payload[8];
    // payload[9] — unknown

    out.valid     = true;
    out.timestamp = millis();
    return true;
}
