/**
 * @file main.cpp
 * @brief Hydronic heater add-on — standalone entry point.
 *
 * This file runs when the heater add-on is built as a standalone
 * firmware (not integrated into paku-core). Useful for bench testing
 * the Autoterm UART interface with just a serial console.
 *
 * Build modes:
 *   - Default: expects real Autoterm on UART + DS18B20 sensor
 *   - EMULATOR_MODE (-D EMULATOR_MODE): runs an internal Autoterm
 *     emulator on Serial1, cross-wired to Serial2 via the ESP32
 *     GPIO matrix. No external wiring or sensors needed.
 *
 * When integrated with paku-core, this file is NOT compiled.
 * Instead, paku-core calls heater_addon_setup() / heater_addon_loop()
 * from the add-on API (see heater_addon.h).
 */

#include <Arduino.h>
#include "config.h"
#include "autoterm_uart.h"
#include "heater_safety.h"
#include "coolant_temp_sensor.h"
#include "flow_sensor.h"

#ifdef EMULATOR_MODE
#include "autoterm_emulator.h"
#endif

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static AutotermUart   heater(Serial2, HEATER_UART_RX_PIN, HEATER_UART_TX_PIN);

#ifdef EMULATOR_MODE
// In emulator mode: safety runs with both sensors simulated.
// Flow sensor simulation lets us test SAFE-F1 trip/recovery too.
static HeaterSafety   safety(heater, true);  // has flow sensor (simulated)

// Emulator on Serial1 — cross-wired internally:
//   Emulator TX (GPIO 16) → Driver RX (GPIO 16)
//   Driver TX (GPIO 17) → Emulator RX (GPIO 17)
static AutotermEmulator emulator(Serial1, HEATER_UART_TX_PIN, HEATER_UART_RX_PIN);
#else
static HeaterSafety   safety(heater, FLOW_SENSOR_PIN >= 0);
#endif

// Sensor drivers (abstracted — real or simulated based on build mode)
static CoolantTempSensor coolantSensor(ONEWIRE_BUS_PIN);
static FlowSensor        flowSensor(FLOW_SENSOR_PIN);

// Serial command buffer
static String commandBuffer;

// Timing
static unsigned long lastTempRead  = 0;
static unsigned long lastStatusLog = 0;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
static void processCommand(const String& cmd);
static void printHelp();
static void printStatus();

#ifdef EMULATOR_MODE
static void processEmulatorCommand(const String& cmd);
static void printEmulatorHelp();
#endif

// ===========================================================================
// setup()
// ===========================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  Autoterm Flow 5D — UART Controller");
#ifdef EMULATOR_MODE
    Serial.println("  *** EMULATOR MODE — no wiring ***");
#else
    Serial.println("  Hydronic Heater Add-on (standalone)");
#endif
    Serial.println("========================================\n");

#ifdef EMULATOR_MODE
    // Start emulator FIRST — it must be listening before the driver
    // sends its first cycle frame. Internal GPIO loopback: both
    // UARTs share the same pins, cross-wired via the GPIO matrix.
    emulator.begin();
#endif

    // Initialize Autoterm UART driver
    heater.begin();

    // Initialize safety monitor
    safety.begin();

    // Initialize sensor drivers
    coolantSensor.begin();
    flowSensor.begin();

#ifdef EMULATOR_MODE
    Serial.println("[Sensors] Simulated — use 'coolant' / 'flow' commands");
#endif

    printHelp();
}

// ===========================================================================
// loop()
// ===========================================================================

void loop() {
    unsigned long now = millis();

#ifdef EMULATOR_MODE
    // 0. Update emulator (must run before driver to process responses)
    emulator.update();
#endif

    // 1. Update Autoterm UART communication
    heater.update();

    // 2. Read sensors and feed to safety monitor
    if (now - lastTempRead >= TEMP_READ_INTERVAL) {
        lastTempRead = now;

        float coolant = coolantSensor.read();
        safety.feedCoolantTemp(coolant);

        float flow = flowSensor.update();
        safety.feedFlowRate(flow, flowSensor.lastPulseTime());
    }

    // 3. Run safety checks
    safety.update();

    // 4. Handle serial commands
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (commandBuffer.length() > 0) {
                processCommand(commandBuffer);
                commandBuffer = "";
            }
        } else {
            commandBuffer += c;
        }
    }

    // 5. Periodic status log
    if (now - lastStatusLog >= STATUS_LOG_INTERVAL) {
        lastStatusLog = now;
        printStatus();
    }

    delay(10);
}

// ===========================================================================
// Command processor
// ===========================================================================

static void processCommand(const String& cmd) {
    String c = cmd;
    c.trim();
    c.toLowerCase();

#ifdef EMULATOR_MODE
    // Emulator-specific commands (prefixed with 'emu' or fault injection)
    if (c.startsWith("emu ") || c.startsWith("coolant ") ||
        c.startsWith("error ") || c.startsWith("voltage ") ||
        c.startsWith("flow ")) {
        processEmulatorCommand(c);
        return;
    }
#endif

    if (c == "start") {
        if (safety.isTripped()) {
            Serial.println("Cannot start: safety tripped. Use 'reset' first.");
            return;
        }
        Serial.println("Starting heater (Power mode, level 5)...");
        heater.start(MODE_BY_POWER, 0xFF, 5);
    }
    else if (c.startsWith("start ")) {
        if (safety.isTripped()) {
            Serial.println("Cannot start: safety tripped. Use 'reset' first.");
            return;
        }
        int level = c.substring(6).toInt();
        level = constrain(level, 0, 9);
        Serial.printf("Starting heater (Power mode, level %d)...\n", level);
        heater.start(MODE_BY_POWER, 0xFF, (uint8_t)level);
    }
    else if (c == "stop") {
        Serial.println("Stopping heater...");
        heater.shutdown();
    }
    else if (c == "vent") {
        Serial.println("Starting ventilation (power 5)...");
        heater.startVentilation(5);
    }
    else if (c == "status") {
        printStatus();
    }
    else if (c == "reset") {
        if (safety.clearTrip()) {
            Serial.println("Safety trip cleared.");
        }
    }
    else if (c == "help") {
        printHelp();
    }
    else {
        Serial.printf("Unknown command: '%s'. Type 'help'.\n", c.c_str());
    }
}

static void printHelp() {
    Serial.println("\n--- Driver Commands ---");
    Serial.println("  start [0-9] — Start heater (power mode, default level 5)");
    Serial.println("  stop        — Shutdown heater");
    Serial.println("  vent        — Fan-only ventilation mode");
    Serial.println("  status      — Print current status");
    Serial.println("  reset       — Clear safety trip (if conditions allow)");
    Serial.println("  help        — Show this help");
#ifdef EMULATOR_MODE
    printEmulatorHelp();
#endif
    Serial.println();
}

static void printStatus() {
    const auto& st = heater.getStatus();

    Serial.println("--- Heater Status ---");
    Serial.printf("  UART link:     %s\n",
                  heater.isOnline() ? "ONLINE" :
                  (heater.getLinkState() == UartLinkState::Timeout ? "TIMEOUT" : "DISCONNECTED"));
    Serial.printf("  Frames RX:     %lu  Errors: %lu\n",
                  heater.getFrameCount(), heater.getErrorCount());

    if (st.valid) {
        Serial.printf("  Heater state:  %s\n", autotermStateName(st.state));
        Serial.printf("  Error:         %s\n", autotermErrorName(st.error));
        Serial.printf("  Battery:       %.1f V\n", st.voltage);
        Serial.printf("  Core temp:     %d °C\n", st.coreTemp);
    } else {
        Serial.println("  (no valid status received yet)");
    }

    Serial.printf("  Coolant temp:  %.1f °C\n", safety.getCoolantTemp());
    Serial.printf("  Flow rate:     %.1f L/min\n", safety.getFlowRate());

    if (safety.isTripped()) {
        Serial.printf("  *** SAFETY TRIPPED: 0x%02X ***\n", safety.getTripReason());
    } else {
        Serial.println("  Safety: OK");
    }

#ifdef EMULATOR_MODE
    Serial.println("--- Emulator ---");
    Serial.printf("  Emu state:     %s\n", autotermStateName(emulator.getState()));
    Serial.printf("  Emu core temp: %.0f °C\n", emulator.getCoreTemp());
    Serial.printf("  Emu voltage:   %.1f V\n", emulator.getVoltage());
    Serial.printf("  Emu power:     %d\n", emulator.getPowerLevel());
    Serial.printf("  Emu RX/TX:     %lu / %lu frames\n",
                  emulator.getFramesRx(), emulator.getFramesTx());
    Serial.printf("  Sim coolant:   %.1f °C\n", coolantSensor.lastReading());
    Serial.printf("  Sim flow:      %.1f L/min\n", flowSensor.getFlowRate());
#endif

    Serial.println("---------------------");
}

// ===========================================================================
// Emulator-specific commands (fault injection, simulation control)
// ===========================================================================

#ifdef EMULATOR_MODE
static void processEmulatorCommand(const String& cmd) {
    if (cmd.startsWith("coolant ")) {
        float temp = cmd.substring(8).toFloat();
        coolantSensor.setSimulated(temp);
        safety.feedCoolantTemp(temp);
        Serial.printf("[Sim] Coolant temperature set to %.1f °C\n", temp);
    }
    else if (cmd.startsWith("flow ")) {
        float lpm = cmd.substring(5).toFloat();
        flowSensor.setSimulated(lpm);
        safety.feedFlowRate(lpm);
        Serial.printf("[Sim] Flow rate set to %.1f L/min\n", lpm);
    }
    else if (cmd == "error clear" || cmd == "error none") {
        emulator.injectError(AutotermError::None);
    }
    else if (cmd == "error overheat") {
        emulator.injectError(AutotermError::Overheating);
    }
    else if (cmd == "error voltage") {
        emulator.injectError(AutotermError::Voltage);
    }
    else if (cmd == "error glowplug") {
        emulator.injectError(AutotermError::GlowPlugFailure);
    }
    else if (cmd == "error flame") {
        emulator.injectError(AutotermError::NoFlame);
    }
    else if (cmd == "error ignition") {
        emulator.injectError(AutotermError::IgnitionFailure);
    }
    else if (cmd == "error fan") {
        emulator.injectError(AutotermError::FanFailure);
    }
    else if (cmd.startsWith("voltage ")) {
        String val = cmd.substring(8);
        if (val == "normal") {
            emulator.overrideVoltage(NAN);
        } else {
            float volts = val.toFloat();
            if (volts > 0.0f) {
                emulator.overrideVoltage(volts);
            }
        }
    }
    else if (cmd == "emu status") {
        Serial.println("--- Emulator Details ---");
        Serial.printf("  State:      %s\n", autotermStateName(emulator.getState()));
        Serial.printf("  Core temp:  %.0f °C\n", emulator.getCoreTemp());
        Serial.printf("  Voltage:    %.1f V\n", emulator.getVoltage());
        Serial.printf("  Power:      %d\n", emulator.getPowerLevel());
        Serial.printf("  Mode:       0x%02X\n", emulator.getOperatingMode());
        Serial.printf("  Panel temp: %d °C\n", emulator.getLastPanelTemp());
        Serial.printf("  RX/TX:      %lu / %lu frames\n",
                      emulator.getFramesRx(), emulator.getFramesTx());
        Serial.println("------------------------");
    }
    else {
        Serial.printf("Unknown emulator command: '%s'\n", cmd.c_str());
    }
}

static void printEmulatorHelp() {
    Serial.println("\n--- Emulator Commands ---");
    Serial.println("  coolant <°C>      — Set simulated coolant temperature");
    Serial.println("  flow <L/min>      — Set simulated coolant flow rate");
    Serial.println("  flow 0            — Simulate flow loss (triggers SAFE-F1)");
    Serial.println("  error overheat    — Inject E01 overheat error");
    Serial.println("  error voltage     — Inject E02 voltage error");
    Serial.println("  error glowplug    — Inject E03 glow plug error");
    Serial.println("  error flame       — Inject E13 no-flame error");
    Serial.println("  error ignition    — Inject E09 ignition failure");
    Serial.println("  error fan         — Inject E07 fan failure");
    Serial.println("  error clear       — Clear injected error");
    Serial.println("  voltage <V>       — Override battery voltage");
    Serial.println("  voltage normal    — Resume voltage simulation");
    Serial.println("  emu status        — Show emulator internals");
}
#endif // EMULATOR_MODE
