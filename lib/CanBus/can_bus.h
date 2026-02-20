/**
 * @file can_bus.h
 * @brief Generic ESP32 TWAI (CAN 2.0B) driver.
 *
 * Hardware-agnostic CAN bus interface using the ESP32 TWAI peripheral.
 * No protocol-specific logic — this is a transport layer only.
 *
 * Based on: ESP-IDF TWAI Driver API (documented)
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html
 *
 * Usage:
 *   CanBus can(GPIO_NUM_4, GPIO_NUM_5);   // TX, RX pins
 *   can.begin(CanBitrate::CAN_250KBPS);   // or CAN_500KBPS
 *   can.send(0x123, data, 8);             // send standard frame
 *   CanFrame frame;
 *   if (can.receive(frame, 10)) { ... }   // receive with 10ms timeout
 *
 * @note ESP32 TWAI supports CAN 2.0B (standard + extended frames).
 *       Requires an external CAN transceiver (e.g., SN65HVD230, MCP2551)
 *       between the ESP32 TWAI pins and the CAN bus.
 *
 * @note The CAN bitrate for Autoterm's upcoming CAN adapter is NOT YET KNOWN.
 *       Common automotive rates are 250 kbps and 500 kbps. The correct rate
 *       will be determined when Autoterm releases the CAN adapter documentation.
 */

#ifndef CAN_BUS_H
#define CAN_BUS_H

#include <Arduino.h>
#include <driver/twai.h>

// ---- Bitrate presets ----

enum class CanBitrate : uint32_t {
    CAN_125KBPS = 125,
    CAN_250KBPS = 250,
    CAN_500KBPS = 500,
    CAN_1MBPS   = 1000,
};

// ---- CAN frame (transport-level, protocol-agnostic) ----

struct CanFrame {
    uint32_t id          = 0;        ///< CAN ID (11-bit standard or 29-bit extended)
    bool     extended    = false;    ///< True if 29-bit extended ID
    bool     rtr         = false;    ///< Remote transmission request
    uint8_t  dlc         = 0;       ///< Data length code (0–8)
    uint8_t  data[8]     = {};      ///< Payload bytes
    uint32_t timestamp   = 0;       ///< millis() when received
};

// ---- Bus state ----

enum class CanBusState : uint8_t {
    Stopped,        ///< Driver not installed or stopped
    Running,        ///< Normal operation
    ErrorWarning,   ///< Error count threshold exceeded (recoverable)
    BusOff,         ///< Bus-off state (too many errors, needs recovery)
    Recovering,     ///< Bus-off recovery in progress
};

// ---- Bus statistics ----

struct CanBusStats {
    uint32_t txCount     = 0;   ///< Frames transmitted successfully
    uint32_t rxCount     = 0;   ///< Frames received successfully
    uint32_t txErrors    = 0;   ///< Transmit errors
    uint32_t rxErrors    = 0;   ///< Receive errors
    uint32_t rxMissed    = 0;   ///< Receive queue overflows
    uint32_t busErrors   = 0;   ///< Bus-off events
    uint32_t arbLost     = 0;   ///< Arbitration lost events
};

// ---- Driver class ----

class CanBus {
public:
    /**
     * @brief Construct a CAN bus driver.
     * @param txPin  GPIO for CAN TX (to transceiver)
     * @param rxPin  GPIO for CAN RX (from transceiver)
     *
     * @note Requires an external CAN transceiver chip (e.g., SN65HVD230).
     *       The ESP32 TWAI pins output/receive logic-level signals, not
     *       differential CAN bus signals.
     */
    CanBus(gpio_num_t txPin, gpio_num_t rxPin);

    /**
     * @brief Initialize and start the TWAI driver.
     * @param bitrate  CAN bus speed (must match all devices on the bus)
     * @param mode     TWAI operating mode (default: normal)
     * @return true if driver installed and started successfully
     *
     * @note The correct bitrate for Autoterm's CAN adapter is not yet known.
     */
    bool begin(CanBitrate bitrate,
               twai_mode_t mode = TWAI_MODE_NORMAL);

    /**
     * @brief Stop and uninstall the TWAI driver.
     */
    void stop();

    /**
     * @brief Send a standard (11-bit ID) CAN frame.
     * @param id       CAN message ID (0x000–0x7FF)
     * @param data     Payload bytes
     * @param len      Payload length (0–8)
     * @param timeoutMs  Transmit timeout in milliseconds (0 = non-blocking)
     * @return true if frame was queued/sent successfully
     */
    bool send(uint32_t id, const uint8_t* data, uint8_t len,
              uint32_t timeoutMs = 100);

    /**
     * @brief Send a pre-built CAN frame (standard or extended).
     * @param frame    Frame to send
     * @param timeoutMs  Transmit timeout in milliseconds
     * @return true if frame was queued/sent successfully
     */
    bool send(const CanFrame& frame, uint32_t timeoutMs = 100);

    /**
     * @brief Receive a CAN frame.
     * @param[out] frame    Received frame (populated on success)
     * @param timeoutMs     Receive timeout in milliseconds (0 = non-blocking)
     * @return true if a frame was received within the timeout
     */
    bool receive(CanFrame& frame, uint32_t timeoutMs = 0);

    /**
     * @brief Attempt to recover from bus-off state.
     * @return true if recovery initiated successfully
     *
     * @note Recovery may take time. Check getState() until it returns Running.
     */
    bool recoverBusOff();

    /**
     * @brief Install a hardware acceptance filter.
     * @param id    Acceptance code (CAN ID to accept)
     * @param mask  Acceptance mask (1 = must match, 0 = don't care)
     * @param extended  True for 29-bit filter, false for 11-bit
     *
     * @note Must be called BEFORE begin(). Calling after begin() requires
     *       stop() → setFilter() → begin() cycle.
     */
    void setFilter(uint32_t id, uint32_t mask, bool extended = false);

    /**
     * @brief Accept all CAN IDs (clear filter). Default behavior.
     * @note Must be called BEFORE begin().
     */
    void clearFilter();

    // ---- Status ----

    CanBusState getState() const;
    bool isRunning() const { return getState() == CanBusState::Running; }
    const CanBusStats& getStats() const { return stats_; }

    /**
     * @brief Get TWAI driver error counters (TEC/REC).
     * @param[out] txErrorCount  Transmit error counter
     * @param[out] rxErrorCount  Receive error counter
     */
    void getErrorCounters(uint32_t& txErrorCount, uint32_t& rxErrorCount) const;

    /**
     * @brief Print bus status to Serial (for debugging).
     */
    void printStatus() const;

private:
    gpio_num_t      txPin_;
    gpio_num_t      rxPin_;
    bool            installed_;
    CanBusStats     stats_;

    // Filter config (applied at begin())
    bool            filterEnabled_;
    uint32_t        filterCode_;
    uint32_t        filterMask_;
    bool            filterExtended_;

    twai_timing_config_t bitrateToTiming(CanBitrate bitrate) const;
};

#endif // CAN_BUS_H
