/**
 * @file config.example.h
 * @brief Example configuration — copy to include/config.h and adjust pins.
 *
 * Hydronic heater add-on for Autoterm Flow 5D.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// Modify these to match your wiring.
// ============================================

// UART to Autoterm Flow 5D (via level shifter e.g. ADUM1201)
// ⚠️ Autoterm is 5V TTL — do NOT connect directly to ESP32 (3.3V)!
#define HEATER_UART_RX_PIN   16     // ESP32 RX ← Autoterm TX
#define HEATER_UART_TX_PIN   17     // ESP32 TX → Autoterm RX

// DS18B20 coolant temperature (OneWire, 4.7kΩ pull-up to 3.3V)
#define ONEWIRE_BUS_PIN      4      // Coolant return line

// Flow sensor (pulse counter) — set to -1 if not installed
#define FLOW_SENSOR_PIN      13

// ============================================
// TIMING (milliseconds)
// ============================================
#define TEMP_READ_INTERVAL    1000  // DS18B20 read interval
#define STATUS_LOG_INTERVAL   10000 // Serial status print
#define MQTT_PUBLISH_INTERVAL 5000  // MQTT telemetry (when integrated)

#endif // CONFIG_H
