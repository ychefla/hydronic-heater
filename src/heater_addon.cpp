/**
 * @file heater_addon.cpp
 * @brief Paku-core heater add-on implementation.
 *
 * When compiled within paku-core (-D HEATER_ENABLED):
 *   - Publishes HA MQTT auto-discovery configs on connect
 *   - Publishes telemetry to paku/heater/{device_id}/data
 *   - Publishes HA state to paku/heater/{device_id}/state
 *   - Handles inbound commands from MQTT
 *
 * Not compiled in EMULATOR_MODE (standalone test mode).
 * In standalone esp32dev mode, MQTT features are excluded.
 */

#ifndef EMULATOR_MODE

#include "heater_addon.h"
#include "config.h"
#include "coolant_temp_sensor.h"
#include "flow_sensor.h"

#ifdef HEATER_ENABLED
// Only available when compiled within paku-core
#include <PubSubClient.h>
extern PubSubClient client;
extern char deviceId[];
#endif

// ---------------------------------------------------------------------------
// Module-level instances
// ---------------------------------------------------------------------------
static AutotermUart*      s_heater  = nullptr;
static HeaterSafety*      s_safety  = nullptr;
static CoolantTempSensor* s_coolant = nullptr;
static FlowSensor*        s_flow    = nullptr;

static unsigned long s_lastTempRead    = 0;

#ifdef HEATER_ENABLED
static unsigned long s_lastMqttPublish = 0;
static bool          s_discoveryPublished = false;
#endif

// ===========================================================================
// setup / loop
// ===========================================================================

void heater_addon_setup() {
    Serial.println("[HeaterAddon] Initializing...");

    // Autoterm UART
    s_heater = new AutotermUart(Serial2, HEATER_UART_RX_PIN, HEATER_UART_TX_PIN);
    s_heater->begin();

    // Safety monitor (flow sensor present if pin >= 0)
    s_safety = new HeaterSafety(*s_heater, FLOW_SENSOR_PIN >= 0);
    s_safety->begin();

    // Coolant temperature sensor (DS18B20)
    s_coolant = new CoolantTempSensor(ONEWIRE_BUS_PIN);
    s_coolant->begin();

    // Flow sensor (pulse counter)
    s_flow = new FlowSensor(FLOW_SENSOR_PIN);
    s_flow->begin();

    Serial.println("[HeaterAddon] Ready");
}

// ===========================================================================
// HA MQTT Auto-Discovery (only when compiled within paku-core)
// ===========================================================================
#ifdef HEATER_ENABLED

/**
 * @brief Publish a single HA discovery config message.
 *
 * Topic: homeassistant/{component}/{node_id}/{object_id}/config
 * Payload: JSON with device, name, state_topic, etc.
 */
static void publishDiscovery(const char* component, const char* objectId,
                              const char* name, const char* valueTpl,
                              const char* devClass = nullptr,
                              const char* unit = nullptr,
                              const char* icon = nullptr) {
    String nodeId = String("autoterm_") + deviceId;
    String topic  = String("homeassistant/") + component + "/" + nodeId
                    + "/" + objectId + "/config";
    String stateTopic = String("paku/heater/") + deviceId + "/state";

    JsonDocument doc;
    doc["name"]         = name;
    doc["unique_id"]    = nodeId + "_" + objectId;
    doc["state_topic"]  = stateTopic;
    doc["value_template"] = valueTpl;
    if (devClass) doc["device_class"]     = devClass;
    if (unit)     doc["unit_of_measurement"] = unit;
    if (icon)     doc["icon"]             = icon;

    // HA device grouping — all entities share this block
    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0]  = nodeId;
    dev["name"]            = String("Autoterm ") + deviceId;
    dev["manufacturer"]    = "Autoterm";
    dev["model"]           = "Flow 5D";
    dev["sw_version"]      = "1.0.0";

    String payload;
    serializeJson(doc, payload);
    client.publish(topic.c_str(), payload.c_str(), true);  // retained

    Serial.printf("[HA] Discovery: %s → %s\n", topic.c_str(), name);
}

/**
 * @brief Publish a switch discovery for heater on/off control.
 */
static void publishSwitchDiscovery() {
    String nodeId = String("autoterm_") + deviceId;
    String topic  = String("homeassistant/switch/") + nodeId + "/power/config";

    JsonDocument doc;
    doc["name"]           = "Heater";
    doc["unique_id"]      = nodeId + "_power";
    doc["state_topic"]    = String("paku/heater/") + deviceId + "/state";
    doc["command_topic"]  = String("paku/heater/") + deviceId + "/cmd";
    doc["value_template"] = "{{ value_json.running }}";
    doc["state_on"]       = "true";
    doc["state_off"]      = "false";
    doc["payload_on"]     = "{\"cmd\":\"start\",\"power\":5}";
    doc["payload_off"]    = "{\"cmd\":\"stop\"}";
    doc["icon"]           = "mdi:radiator";

    JsonObject dev = doc["device"].to<JsonObject>();
    dev["identifiers"][0]  = nodeId;
    dev["name"]            = String("Autoterm ") + deviceId;
    dev["manufacturer"]    = "Autoterm";
    dev["model"]           = "Flow 5D";
    dev["sw_version"]      = "1.0.0";

    String payload;
    serializeJson(doc, payload);
    client.publish(topic.c_str(), payload.c_str(), true);

    Serial.printf("[HA] Discovery: switch → Heater\n");
}

/**
 * @brief Publish all HA discovery configs. Call once after MQTT connect.
 */
static void publishHaDiscovery() {
    // Sensors
    publishDiscovery("sensor", "coolant_temp", "Coolant Temperature",
                     "{{ value_json.coolant_temp_c }}", "temperature", "°C");
    publishDiscovery("sensor", "flow_rate", "Flow Rate",
                     "{{ value_json.flow_lpm }}", nullptr, "L/min",
                     "mdi:water-pump");
    publishDiscovery("sensor", "battery", "Battery Voltage",
                     "{{ value_json.battery_v }}", "voltage", "V");
    publishDiscovery("sensor", "core_temp", "Core Temperature",
                     "{{ value_json.core_temp_c }}", "temperature", "°C");
    publishDiscovery("sensor", "heater_state", "Heater State",
                     "{{ value_json.state }}", nullptr, nullptr,
                     "mdi:radiator");

    // Binary sensors
    publishDiscovery("binary_sensor", "safety", "Safety OK",
                     "{{ 'ON' if value_json.safety_ok else 'OFF' }}",
                     "safety");
    publishDiscovery("binary_sensor", "uart_link", "UART Link",
                     "{{ 'ON' if value_json.uart_online else 'OFF' }}",
                     "connectivity");

    // Switch for on/off control
    publishSwitchDiscovery();

    s_discoveryPublished = true;
    Serial.println("[HA] All discovery configs published");
}

// ===========================================================================
// MQTT state publishing
// ===========================================================================

/**
 * @brief Publish heater state to HA state topic + paku telemetry topic.
 */
static void publishMqttState() {
    if (!s_heater || !s_safety || !client.connected()) return;

    const auto& st = s_heater->getStatus();

    // -- HA state topic (flat JSON, matches value_templates) --
    {
        JsonDocument doc;
        doc["state"]          = st.valid ? autotermStateName(st.state) : "unknown";
        doc["coolant_temp_c"] = s_safety->getCoolantTemp();
        doc["flow_lpm"]       = s_safety->getFlowRate();
        doc["battery_v"]      = st.valid ? st.voltage : 0.0f;
        doc["core_temp_c"]    = st.valid ? st.coreTemp : 0;
        doc["safety_ok"]      = !s_safety->isTripped();
        doc["uart_online"]    = s_heater->isOnline();
        doc["error"]          = st.valid ? autotermErrorName(st.error) : "none";

        bool running = st.valid &&
            (st.state == AutotermState::Starting ||
             st.state == AutotermState::Warming  ||
             st.state == AutotermState::Running);
        doc["running"] = running;

        if (s_safety->isTripped()) {
            doc["trip_reason"] = s_safety->getTripReason();
        }

        String payload;
        serializeJson(doc, payload);
        String topic = String("paku/heater/") + deviceId + "/state";
        client.publish(topic.c_str(), payload.c_str());
    }

    // -- paku-iot collector topic (matches +/+/+/data pattern) --
    {
        JsonDocument doc;
        doc["device_id"]  = String("heater_") + deviceId;
        doc["location"]   = "van";

        // Timestamp — use millis-based uptime if no NTP
        char ts[32];
        unsigned long sec = millis() / 1000;
        snprintf(ts, sizeof(ts), "%lu", sec);
        doc["timestamp"] = ts;

        JsonObject m = doc["metrics"].to<JsonObject>();
        m["coolant_temp_c"] = s_safety->getCoolantTemp();
        m["flow_lpm"]       = s_safety->getFlowRate();
        m["safety_ok"]      = !s_safety->isTripped() ? 1 : 0;
        m["uart_online"]    = s_heater->isOnline() ? 1 : 0;
        if (st.valid) {
            m["battery_v"]    = st.voltage;
            m["core_temp_c"]  = st.coreTemp;
            m["heater_state"] = autotermStateName(st.state);
            m["error"]        = autotermErrorName(st.error);
        }

        String payload;
        serializeJson(doc, payload);
        String topic = String("paku/heater/") + deviceId + "/data";
        client.publish(topic.c_str(), payload.c_str());
    }
}

#endif // HEATER_ENABLED (MQTT functions)

// ===========================================================================
// loop
// ===========================================================================

void heater_addon_loop() {
    if (!s_heater || !s_safety) return;

    unsigned long now = millis();

    // Update Autoterm communication
    s_heater->update();

    // Read sensors and feed to safety monitor
    if (now - s_lastTempRead >= TEMP_READ_INTERVAL) {
        s_lastTempRead = now;

        float coolant = s_coolant->read();
        s_safety->feedCoolantTemp(coolant);

        float flow = s_flow->update();
        s_safety->feedFlowRate(flow, s_flow->lastPulseTime());
    }

    // Run safety checks
    s_safety->update();

#ifdef HEATER_ENABLED
    // Publish HA discovery on first MQTT connection
    if (client.connected() && !s_discoveryPublished) {
        publishHaDiscovery();

        // Subscribe to heater command topic
        String cmdTopic = String("paku/heater/") + deviceId + "/cmd";
        client.subscribe(cmdTopic.c_str());
        Serial.printf("[HeaterAddon] Subscribed to %s\n", cmdTopic.c_str());
    }

    // Reset discovery flag if disconnected (re-publish on reconnect)
    if (!client.connected()) {
        s_discoveryPublished = false;
    }

    // Periodic MQTT publish
    if (client.connected() && (now - s_lastMqttPublish >= MQTT_PUBLISH_INTERVAL)) {
        s_lastMqttPublish = now;
        publishMqttState();
    }
#endif // HEATER_ENABLED
}

// ===========================================================================
// Telemetry (for paku-core snapshot buffer, if called externally)
// ===========================================================================

void heater_addon_telemetry(JsonDocument& doc) {
    if (!s_heater || !s_safety) return;

    const auto& st = s_heater->getStatus();

    doc["heater"]["link"] = s_heater->isOnline() ? "online" : "offline";

    if (st.valid) {
        doc["heater"]["state"]      = autotermStateName(st.state);
        doc["heater"]["error"]      = autotermErrorName(st.error);
        doc["heater"]["voltage"]    = st.voltage;
        doc["heater"]["coreTemp"]   = st.coreTemp;
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

#endif // !EMULATOR_MODE
