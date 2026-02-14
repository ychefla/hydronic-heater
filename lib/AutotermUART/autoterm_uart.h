/**
 * @file autoterm_uart.h
 * @brief High-level Autoterm UART driver.
 *
 * Manages serial communication with the Autoterm Flow 5D heater:
 *   - Sends commands (start, stop, set mode/power, ventilation)
 *   - Receives and parses status responses
 *   - Maintains communication heartbeat (3-second cycle)
 *   - Tracks connection state (online / offline / timeout)
 *
 * @note This driver does NOT manage safety — see HeaterSafety.
 */

#ifndef AUTOTERM_UART_H
#define AUTOTERM_UART_H

#include <Arduino.h>
#include "autoterm_protocol.h"

/// How long without a valid STATUS before we consider the heater offline.
static constexpr unsigned long UART_TIMEOUT_MS = 10000;

/// Communication cycle interval (ms).
static constexpr unsigned long COMM_CYCLE_MS   = 3000;

/**
 * @brief Connection state of the UART link.
 */
enum class UartLinkState : uint8_t {
    Disconnected,   ///< No valid frames received yet
    Online,         ///< Receiving valid status responses
    Timeout         ///< Was online, but no response for > UART_TIMEOUT_MS
};

/**
 * @brief Autoterm UART driver.
 *
 * Typical usage:
 * @code
 * AutotermUart heater(Serial2);
 * heater.begin();
 * // in loop():
 * heater.update();
 * if (heater.isOnline()) {
 *     auto& st = heater.getStatus();
 *     Serial.printf("State: %s  Voltage: %.1fV\n",
 *                   autotermStateName(st.state), st.voltage);
 * }
 * // To start:
 * heater.start(MODE_BY_POWER, 20, 5);
 * // To stop:
 * heater.shutdown();
 * @endcode
 */
class AutotermUart {
public:
    /**
     * @brief Construct driver on a HardwareSerial port.
     * @param serial  Reference to HardwareSerial (e.g. Serial2).
     * @param rxPin   UART RX pin (default 16).
     * @param txPin   UART TX pin (default 17).
     */
    explicit AutotermUart(HardwareSerial& serial,
                          int rxPin = 16, int txPin = 17);

    /**
     * @brief Initialize the UART port. Call once in setup().
     */
    void begin();

    /**
     * @brief Call every loop iteration. Handles:
     *        - Reading incoming bytes and assembling frames
     *        - Sending the periodic communication cycle
     *        - Tracking link state
     */
    void update();

    // -----------------------------------------------------------------------
    // Commands
    // -----------------------------------------------------------------------

    /**
     * @brief Send START command.
     * @param mode       Operating mode (MODE_BY_HEATER, MODE_BY_PANEL, etc.).
     * @param targetTemp Target temperature in °C (0xFF if not used).
     * @param powerLevel Power level 0-9 (0xFF if not used).
     * @param ventilation true to enable ventilation.
     * @return true if frame was sent.
     */
    bool start(uint8_t mode, uint8_t targetTemp = 0xFF,
               uint8_t powerLevel = 0xFF, bool ventilation = false);

    /**
     * @brief Send SHUTDOWN command.
     * @return true if frame was sent.
     */
    bool shutdown();

    /**
     * @brief Send GET/SET command (query current settings).
     * @return true if frame was sent.
     */
    bool querySettings();

    /**
     * @brief Send PANEL_TEMP — report ambient temperature to heater.
     * @param tempC  Temperature in °C.
     * @return true if frame was sent.
     */
    bool sendPanelTemp(uint8_t tempC);

    /**
     * @brief Send VENTILATION command (fan-only mode).
     * @param powerLevel Fan power 0-9.
     * @return true if frame was sent.
     */
    bool startVentilation(uint8_t powerLevel);

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    /** @brief Latest parsed status from the heater. */
    const AutotermStatus& getStatus() const { return status_; }

    /** @brief Current UART link state. */
    UartLinkState getLinkState() const { return linkState_; }

    /** @brief true if we have a valid, recent status. */
    bool isOnline() const { return linkState_ == UartLinkState::Online; }

    /** @brief Number of valid frames received since begin(). */
    uint32_t getFrameCount() const { return frameCount_; }

    /** @brief Number of CRC/parse errors since begin(). */
    uint32_t getErrorCount() const { return errorCount_; }

private:
    HardwareSerial& serial_;
    int rxPin_;
    int txPin_;

    // Receive buffer + state machine
    uint8_t rxBuf_[FRAME_MAX_SIZE];
    size_t  rxIdx_;
    bool    rxInFrame_;

    // Communication cycle
    unsigned long lastCycleMs_;
    uint8_t       cycleStep_;        ///< 0=GET, 1=STATUS, 2=PANEL_TEMP
    uint8_t       panelTemp_;        ///< Ambient temp to send to heater

    // Status
    AutotermStatus status_;
    UartLinkState  linkState_;
    unsigned long  lastValidRxMs_;

    // Stats
    uint32_t frameCount_;
    uint32_t errorCount_;

    // Pending command (overrides next cycle if set)
    bool    pendingCmd_;
    uint8_t pendingBuf_[FRAME_MAX_SIZE];
    size_t  pendingLen_;

    // -----------------------------------------------------------------------
    // Internal methods
    // -----------------------------------------------------------------------

    /** @brief Send raw bytes to serial. */
    bool sendFrame(const uint8_t* buf, size_t len);

    /** @brief Process a fully received frame. */
    void processFrame(const uint8_t* buf, size_t len);

    /** @brief Run one step of the 3-second communication cycle. */
    void runCycleStep();

    /** @brief Read available bytes and assemble frames. */
    void readSerial();

    /** @brief Update link state based on timing. */
    void updateLinkState();
};

#endif // AUTOTERM_UART_H
