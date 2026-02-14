/**
 * @file heater_addon.cpp
 * @brief Paku-core heater add-on implementation.
 */

#include "heater_addon.h"
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------------------------------------------------------------------
// Module-level instances
// ---------------------------------------------------------------------------
static AutotermUart*      s_heater  = nullptr;
static HeaterSafety*      s_safety  = nullptr;
static OneWire*           s_oneWire = nullptr;
static DallasTemperature* s_ds18b20 = nullptr;

static unsigned long s_lastTempRead = 0;

// ===========================================================================
// setup / loop
// ===========================================================================

void heater_addon_setup() {
    Serial.println("[HeaterAddon] Initializing...");

    // Autoterm UART
    s_heater = new AutotermUart(Serial2, HEATER_UART_RX_PIN, HEATER_UART_TX_PIN);
    s_heater->begin();

    // Safety monitor
    s_safety = new HeaterSafety(*s_heater, FLOW_SENSOR_PIN);
    s_safety->begin();

    // DS18B20 coolant sensor
    s_oneWire = new OneWire(ONEWIRE_BUS_PIN);
    s_ds18b20 = new DallasTemperature(s_oneWire);
    s_ds18b20->begin();

    int count = s_ds18b20->getDeviceCount();
    Serial.printf("[HeaterAddon] DS18B20: %d sensor(s) on GPIO %d\n",
                  count, ONEWIRE_BUS_PIN);
    Serial.println("[HeaterAddon] Ready");
}

void heater_addon_loop() {
    if (!s_heater || !s_safety) return;

    unsigned long now = millis();

    // Update Autoterm communication
    s_heater->update();

    // Read coolant temperature
    if (now - s_lastTempRead >= TEMP_READ_INTERVAL) {
        s_lastTempRead = now;
        s_ds18b20->requestTemperatures();
        float coolantTemp = s_ds18b20->getTempCByIndex(0);
        s_safety->feedCoolantTemp(coolantTemp);
    }

    // Run safety checks
    s_safety->update();
}

// ===========================================================================
// Telemetry
// ===========================================================================

void heater_addon_telemetry(JsonDocument& doc) {
    if (!s_heater || !s_safety) return;

    const auto& st = s_heater->getStatus();

    doc["heater"]["link"] = s_heater->isOnline() ? "online" : "offline";

    if (st.valid) {
        doc["heater"]["state"]    = autotermStateName(st.state);
        doc["heater"]["error"]    = autotermErrorName(st.error);
        doc["heater"]["voltage"]  = st.voltage;
        doc["heater"]["coreTemp"] = st.coreTemp;
    }

    doc["heater"]["coolantTemp"] = s_safety->getCoolantTemp();
    doc["heater"]["flowRate"]    = s_safety->getFlowRate();
    doc["heater"]["safetyOk"]    = !s_safety->isTripped();

    if (s_safety->isTripped()) {
        doc["heater"]["tripReason"] = s_safety->getTripReason();
    }
}

// ===========================================================================
// MQTT command handler
// ===========================================================================

void heater_addon_command(const char* payload, unsigned int length) {
    if (!s_heater || !s_safety) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload, length);
    if (err) {
        Serial.printf("[HeaterAddon] JSON parse error: %s\n", err.c_str());
        return;
    }

    const char* cmd = doc["cmd"] | "";

    if (strcmp(cmd, "start") == 0) {
        if (s_safety->isTripped()) {
            Serial.println("[HeaterAddon] Cannot start: safety tripped");
            return;
        }
        uint8_t power = doc["power"] | 5;
        power = constrain(power, 0, 9);
        s_heater->start(MODE_BY_POWER, 0xFF, power);
        Serial.printf("[HeaterAddon] START power=%d\n", power);
    }
    else if (strcmp(cmd, "stop") == 0) {
        s_heater->shutdown();
        Serial.println("[HeaterAddon] STOP");
    }
    else if (strcmp(cmd, "vent") == 0) {
        uint8_t power = doc["power"] | 5;
        power = constrain(power, 0, 9);
        s_heater->startVentilation(power);
        Serial.printf("[HeaterAddon] VENTILATION power=%d\n", power);
    }
    else if (strcmp(cmd, "reset") == 0) {
        if (s_safety->clearTrip()) {
            Serial.println("[HeaterAddon] Safety trip cleared");
        } else {
            Serial.println("[HeaterAddon] Cannot clear trip — conditions not met");
        }
    }
    else {
        Serial.printf("[HeaterAddon] Unknown command: '%s'\n", cmd);
    }
}

// ===========================================================================
// Accessors
// ===========================================================================

AutotermUart& heater_addon_getUart() {
    return *s_heater;
}

HeaterSafety& heater_addon_getSafety() {
    return *s_safety;
}
