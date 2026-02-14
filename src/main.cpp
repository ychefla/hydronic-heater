/**
 * @file main.cpp
 * @brief Hydronic heater add-on — standalone entry point.
 *
 * This file runs when the heater add-on is built as a standalone
 * firmware (not integrated into paku-core). Useful for bench testing
 * the Autoterm UART interface with just a serial console.
 *
 * When integrated with paku-core, this file is NOT compiled.
 * Instead, paku-core calls heater_addon_setup() / heater_addon_loop()
 * from the add-on API (see heater_addon.h).
 */

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"
#include "autoterm_uart.h"
#include "heater_safety.h"

// ---------------------------------------------------------------------------
// Globals
// ---------------------------------------------------------------------------
static AutotermUart   heater(Serial2, HEATER_UART_RX_PIN, HEATER_UART_TX_PIN);
static HeaterSafety   safety(heater, FLOW_SENSOR_PIN);

// DS18B20 on coolant return line
static OneWire        oneWire(ONEWIRE_BUS_PIN);
static DallasTemperature ds18b20(&oneWire);

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

// ===========================================================================
// setup()
// ===========================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n========================================");
    Serial.println("  Autoterm Flow 5D — UART Controller");
    Serial.println("  Hydronic Heater Add-on (standalone)");
    Serial.println("========================================\n");

    // Initialize Autoterm UART
    heater.begin();

    // Initialize safety monitor
    safety.begin();

    // Initialize DS18B20
    ds18b20.begin();
    int sensorCount = ds18b20.getDeviceCount();
    Serial.printf("[DS18B20] Found %d sensor(s) on GPIO %d\n",
                  sensorCount, ONEWIRE_BUS_PIN);

    printHelp();
}

// ===========================================================================
// loop()
// ===========================================================================

void loop() {
    unsigned long now = millis();

    // 1. Update Autoterm UART communication
    heater.update();

    // 2. Read DS18B20 periodically and feed to safety monitor
    if (now - lastTempRead >= TEMP_READ_INTERVAL) {
        lastTempRead = now;
        ds18b20.requestTemperatures();
        float coolantTemp = ds18b20.getTempCByIndex(0);
        safety.feedCoolantTemp(coolantTemp);
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
    Serial.println("\nCommands:");
    Serial.println("  start [0-9] — Start heater (power mode, default level 5)");
    Serial.println("  stop        — Shutdown heater");
    Serial.println("  vent        — Fan-only ventilation mode");
    Serial.println("  status      — Print current status");
    Serial.println("  reset       — Clear safety trip (if conditions allow)");
    Serial.println("  help        — Show this help");
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
    Serial.println("---------------------");
}
