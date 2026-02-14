/**
 * @file config.h
 * @brief Hydronic heater add-on configuration.
 *
 * Pin assignments for the Autoterm Flow 5D UART interface,
 * DS18B20 coolant sensor, and optional flow sensor.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// ============================================

// UART to Autoterm Flow 5D (via ADUM1201 level shifter)
#define HEATER_UART_RX_PIN   16     ///< ESP32 RX ← Autoterm TX (via level shifter)
#define HEATER_UART_TX_PIN   17     ///< ESP32 TX → Autoterm RX (via level shifter)

// DS18B20 coolant temperature sensor (OneWire)
#define ONEWIRE_BUS_PIN      4      ///< Coolant return line DS18B20

// Flow sensor (optional — pulse counter)
#define FLOW_SENSOR_PIN      13     ///< Coolant flow sensor, -1 to disable
// Set to -1 if no flow sensor is installed:
// #define FLOW_SENSOR_PIN   -1

// ============================================
// TIMING (milliseconds)
// ============================================
#define TEMP_READ_INTERVAL   1000   ///< DS18B20 polling interval
#define STATUS_LOG_INTERVAL  10000  ///< Serial status print interval
#define MQTT_PUBLISH_INTERVAL 5000  ///< Telemetry publish interval

// ============================================
// TEMPERATURE THRESHOLDS (see SAFETY.md)
// ============================================
// These are informational — the actual thresholds are in heater_safety.h
// COOLANT_OVERHEAT_C = 95.0  → emergency stop
// COOLANT_RECOVERY_C = 70.0  → manual reset allowed

#endif // CONFIG_H
