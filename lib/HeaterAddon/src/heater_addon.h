/**
 * @file heater_addon.h
 * @brief Paku-core heater add-on API.
 *
 * When paku-core is compiled with -D HEATER_ENABLED, it calls these
 * functions to integrate the hydronic heater. The add-on manages:
 *   - AutotermUart communication
 *   - HeaterSafety monitoring
 *   - DS18B20 coolant sensor
 *
 * paku-core provides:
 *   - WiFi / MQTT connectivity
 *   - BLE (RuuviTag cabin temp)
 *   - Display
 *   - OTA
 *
 * Usage in paku-core's main.cpp:
 * @code
 * #ifdef HEATER_ENABLED
 * #include "heater_addon.h"
 * #endif
 *
 * void setup() {
 *     // ... paku-core init ...
 *     #ifdef HEATER_ENABLED
 *     heater_addon_setup();
 *     #endif
 * }
 *
 * void loop() {
 *     // ... paku-core loop ...
 *     #ifdef HEATER_ENABLED
 *     heater_addon_loop();
 *     #endif
 * }
 * @endcode
 */

#ifndef HEATER_ADDON_H
#define HEATER_ADDON_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "autoterm_uart.h"
#include "heater_safety.h"

/**
 * @brief Initialize the heater add-on. Call once in setup().
 */
void heater_addon_setup();

/**
 * @brief Run heater add-on tasks. Call every loop iteration.
 */
void heater_addon_loop();

/**
 * @brief Build a JSON telemetry object for MQTT publishing.
 *
 * The caller (paku-core) handles the actual MQTT publish.
 *
 * @param doc  ArduinoJson document to populate.
 */
void heater_addon_telemetry(JsonDocument& doc);

/**
 * @brief Handle an inbound MQTT command for the heater.
 *
 * Called by paku-core when a message arrives on the heater command topic.
 *
 * Expected JSON commands:
 *   { "cmd": "start", "power": 5 }
 *   { "cmd": "stop" }
 *   { "cmd": "vent", "power": 3 }
 *   { "cmd": "reset" }
 *
 * @param payload  JSON string from MQTT.
 * @param length   Length of payload.
 */
void heater_addon_command(const char* payload, unsigned int length);

/**
 * @brief Get a reference to the Autoterm UART driver.
 *
 * For advanced use by paku-core (e.g. display integration).
 */
AutotermUart& heater_addon_getUart();

/**
 * @brief Get a reference to the safety monitor.
 */
HeaterSafety& heater_addon_getSafety();

#endif // HEATER_ADDON_H
