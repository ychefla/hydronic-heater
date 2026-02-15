/**
 * @file autoterm_emulator.h
 * @brief Autoterm Flow 5D heater emulator for testing.
 *
 * Simulates the heater ECU on a HardwareSerial port. Receives
 * commands from the AutotermUart driver and responds with realistic
 * status frames, using the same binary protocol.
 *
 * Designed for use with ESP32 internal GPIO loopback:
 *   - Driver  on Serial2  (RX=16, TX=17)
 *   - Emulator on Serial1 (RX=17, TX=16)
 *
 * No external wiring is needed — the ESP32 GPIO matrix routes
 * TX of one UART to RX of the other internally.
 *
 * Features:
 *   - Full state machine: Off → Starting → Warming → Running → ...
 *   - Simulated core temperature (ramps up/down realistically)
 *   - Simulated battery voltage with small jitter
 *   - Error injection via serial commands for testing safety layer
 *   - Configurable startup/shutdown timing
 */

#ifndef AUTOTERM_EMULATOR_H
#define AUTOTERM_EMULATOR_H

#include <Arduino.h>
#include "autoterm_protocol.h"

// ---------------------------------------------------------------------------
// Simulation timing (milliseconds)
// ---------------------------------------------------------------------------
static constexpr unsigned long EMU_STARTING_DURATION_MS  = 8000;   ///< Off → Starting
static constexpr unsigned long EMU_WARMING_DURATION_MS   = 12000;  ///< Starting → Warming → Running
static constexpr unsigned long EMU_SHUTDOWN_DURATION_MS  = 6000;   ///< Shutting Down
static constexpr unsigned long EMU_COOLING_DURATION_MS   = 10000;  ///< Cooling → Off

// Temperature simulation
static constexpr float EMU_CORE_TEMP_AMBIENT   = 22.0f;  ///< Ambient / off temperature
static constexpr float EMU_CORE_TEMP_RUNNING   = 180.0f; ///< Steady-state running temp
static constexpr float EMU_CORE_TEMP_RAMP_RATE = 0.5f;   ///< °C per 100ms when heating
static constexpr float EMU_CORE_TEMP_COOL_RATE = 0.3f;   ///< °C per 100ms when cooling

// Voltage simulation
static constexpr float EMU_VOLTAGE_NOMINAL     = 12.6f;
static constexpr float EMU_VOLTAGE_RUNNING_DIP = 0.3f;   ///< Slight drop when running
static constexpr float EMU_VOLTAGE_JITTER      = 0.1f;   ///< Random ±jitter

/**
 * @brief Autoterm heater emulator.
 *
 * Usage:
 * @code
 * AutotermEmulator emu(Serial1, 17, 16);  // cross-wired with driver
 * emu.begin();
 * // in loop():
 * emu.update();
 * @endcode
 */
class AutotermEmulator {
public:
    /**
     * @brief Construct emulator on a HardwareSerial port.
     * @param serial  Reference to HardwareSerial (e.g. Serial1).
     * @param rxPin   Emulator RX pin (connects to driver TX).
     * @param txPin   Emulator TX pin (connects to driver RX).
     */
    explicit AutotermEmulator(HardwareSerial& serial,
                               int rxPin = 17, int txPin = 16);

    /**
     * @brief Initialize UART. Call once in setup().
     */
    void begin();

    /**
     * @brief Process incoming frames and update simulation.
     *        Call every loop iteration.
     */
    void update();

    // -----------------------------------------------------------------------
    // Error injection (for testing safety layer)
    // -----------------------------------------------------------------------

    /**
     * @brief Inject an error code. The emulator will report this in STATUS.
     * @param err  Error to inject. Use AutotermError::None to clear.
     */
    void injectError(AutotermError err);

    /**
     * @brief Override battery voltage (for testing voltage safety).
     * @param volts  Voltage to report, or NAN to resume simulation.
     */
    void overrideVoltage(float volts);

    /**
     * @brief Force the emulator into a specific state (for testing).
     * @param state  Target state.
     */
    void forceState(AutotermState state);

    // -----------------------------------------------------------------------
    // Getters
    // -----------------------------------------------------------------------

    /** @brief Current emulated heater state. */
    AutotermState getState() const { return state_; }

    /** @brief Current simulated core temperature. */
    float getCoreTemp() const { return coreTemp_; }

    /** @brief Current simulated voltage. */
    float getVoltage() const { return voltage_; }

    /** @brief Number of frames received from the driver. */
    uint32_t getFramesRx() const { return framesRx_; }

    /** @brief Number of responses sent back. */
    uint32_t getFramesTx() const { return framesTx_; }

    /** @brief Current power level (0-9). */
    uint8_t getPowerLevel() const { return powerLevel_; }

    /** @brief Current operating mode. */
    uint8_t getOperatingMode() const { return operatingMode_; }

    /** @brief Last panel temperature received from driver. */
    uint8_t getLastPanelTemp() const { return lastPanelTemp_; }

private:
    HardwareSerial& serial_;
    int rxPin_;
    int txPin_;

    // Receive buffer
    uint8_t rxBuf_[FRAME_MAX_SIZE];
    size_t  rxIdx_;
    bool    rxInFrame_;

    // Heater state machine
    AutotermState state_;
    AutotermError error_;
    unsigned long stateEnteredMs_;
    uint8_t       operatingMode_;
    uint8_t       targetTemp_;
    uint8_t       powerLevel_;

    // Simulation values
    float   coreTemp_;
    float   voltage_;
    float   voltageOverride_;     ///< NAN = use simulation
    uint8_t lastPanelTemp_;

    // Timing
    unsigned long lastSimUpdateMs_;

    // Stats
    uint32_t framesRx_;
    uint32_t framesTx_;

    // -----------------------------------------------------------------------
    // Internal
    // -----------------------------------------------------------------------
    void readSerial();
    void processFrame(const uint8_t* buf, size_t len);
    void handleCommand(uint8_t command, const uint8_t* payload, size_t payloadLen);

    void sendStatusResponse();
    void sendSettingsResponse();
    void sendPanelTempAck();
    bool sendFrame(uint8_t command, const uint8_t* payload, size_t payloadLen);

    void updateStateMachine();
    void updateSimulation();
    void transitionTo(AutotermState newState);
};

#endif // AUTOTERM_EMULATOR_H
