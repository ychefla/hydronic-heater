/**
 * @file autoterm_protocol.h
 * @brief Autoterm UART protocol constants and frame definitions.
 *
 * Protocol reverse-engineered from Autoterm Air 2D/4D (community projects).
 * Expected compatible with Flow 5D — verify with real hardware.
 *
 * Sources:
 *   - https://github.com/Boren/ha-autoterm-diesel-heater (PROTOCOL.md)
 *   - https://github.com/timokovanen/esphome-autoterm (C++ ESP32)
 */

#ifndef AUTOTERM_PROTOCOL_H
#define AUTOTERM_PROTOCOL_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Physical layer
// ---------------------------------------------------------------------------
static constexpr uint32_t AUTOTERM_BAUD        = 2400;
static constexpr uint32_t AUTOTERM_DATA_BITS   = SERIAL_8N1;

// ---------------------------------------------------------------------------
// Frame constants
// ---------------------------------------------------------------------------
static constexpr uint8_t  FRAME_PREAMBLE       = 0xAA;
static constexpr uint8_t  SENDER_PANEL          = 0x03;   ///< Panel / controller
static constexpr uint8_t  SENDER_HEATER         = 0x04;   ///< Heater ECU
static constexpr uint8_t  FRAME_RESERVED        = 0x00;
static constexpr size_t   FRAME_MIN_SIZE        = 7;      ///< Preamble+Sender+Len+Rsv+Cmd+CRC(2)
static constexpr size_t   FRAME_MAX_SIZE        = 32;     ///< Conservative upper bound

// CRC-16 Modbus
static constexpr uint16_t CRC_INIT              = 0xFFFF;
static constexpr uint16_t CRC_POLY              = 0xA001; ///< Reversed 0x8005

// ---------------------------------------------------------------------------
// Command IDs
// ---------------------------------------------------------------------------
static constexpr uint8_t CMD_START              = 0x01;
static constexpr uint8_t CMD_GET_SET            = 0x02;
static constexpr uint8_t CMD_SHUTDOWN           = 0x03;
static constexpr uint8_t CMD_STATUS             = 0x0F;
static constexpr uint8_t CMD_PANEL_TEMP         = 0x11;
static constexpr uint8_t CMD_VENTILATION        = 0x23;

// ---------------------------------------------------------------------------
// Operating modes (byte 2 of START payload)
// ---------------------------------------------------------------------------
static constexpr uint8_t MODE_BY_HEATER         = 0x01;
static constexpr uint8_t MODE_BY_PANEL          = 0x02;
static constexpr uint8_t MODE_BY_EXTERNAL       = 0x03;
static constexpr uint8_t MODE_BY_POWER          = 0x04;

// ---------------------------------------------------------------------------
// Heater states (status response byte 0)
// ---------------------------------------------------------------------------
enum class AutotermState : uint8_t {
    Off          = 0x00,
    Starting     = 0x01,
    Standby      = 0x02,
    Warming      = 0x03,
    Running      = 0x04,
    ShuttingDown = 0x05,
    Cooling      = 0x06,
    Ventilation  = 0x08,
    Unknown      = 0xFF
};

/**
 * @brief Return a human-readable name for a heater state.
 */
const char* autotermStateName(AutotermState state);

// ---------------------------------------------------------------------------
// Error codes (status response byte 2)
// ---------------------------------------------------------------------------
enum class AutotermError : uint8_t {
    None              = 0x00,
    Overheating       = 0x01,
    Voltage           = 0x02,
    GlowPlugFailure   = 0x03,
    FuelPumpFailure    = 0x04,
    FlameSensor        = 0x05,
    TempSensor         = 0x06,
    FanFailure         = 0x07,
    ControllerFault    = 0x08,
    IgnitionFailure    = 0x09,
    Unknown0A          = 0x0A,
    Unknown0B          = 0x0B,
    Unknown0C          = 0x0C,
    NoFlame            = 0x0D,
    Unknown            = 0xFF
};

/**
 * @brief Return a human-readable name for an error code.
 */
const char* autotermErrorName(AutotermError err);

// ---------------------------------------------------------------------------
// Parsed status data
// ---------------------------------------------------------------------------
struct AutotermStatus {
    AutotermState state     = AutotermState::Unknown;
    AutotermError error     = AutotermError::None;
    float         voltage   = 0.0f;   ///< Battery voltage (V)
    uint8_t       coreTemp  = 0;      ///< Heater core temperature (°C)
    uint8_t       rawBytes[10] = {};   ///< Full 10-byte payload for debugging
    bool          valid     = false;   ///< True if successfully parsed
    unsigned long timestamp = 0;       ///< millis() when received
};

// ---------------------------------------------------------------------------
// Frame building / parsing
// ---------------------------------------------------------------------------

/**
 * @brief Calculate CRC-16 Modbus over a buffer.
 * @param data  Pointer to data (starting from sender byte).
 * @param len   Number of bytes.
 * @return CRC-16 value.
 */
uint16_t autotermCRC16(const uint8_t* data, size_t len);

/**
 * @brief Build a complete frame into the output buffer.
 * @param buf       Output buffer (must be >= FRAME_MAX_SIZE).
 * @param command   Command ID.
 * @param payload   Payload bytes (may be nullptr if payloadLen == 0).
 * @param payloadLen  Number of payload bytes.
 * @return Number of bytes written to buf, or 0 on error.
 */
size_t autotermBuildFrame(uint8_t* buf, uint8_t command,
                          const uint8_t* payload, size_t payloadLen);

/**
 * @brief Validate a received frame.
 *
 * Checks preamble, length consistency and CRC.
 *
 * @param buf   Received frame bytes.
 * @param len   Number of bytes received.
 * @return true if frame is valid.
 */
bool autotermValidateFrame(const uint8_t* buf, size_t len);

/**
 * @brief Parse a validated STATUS response into AutotermStatus.
 * @param payload     Pointer to the payload bytes (after command byte).
 * @param payloadLen  Length of payload.
 * @param out         Output status struct.
 * @return true if parsed successfully.
 */
bool autotermParseStatus(const uint8_t* payload, size_t payloadLen,
                         AutotermStatus& out);

#endif // AUTOTERM_PROTOCOL_H
