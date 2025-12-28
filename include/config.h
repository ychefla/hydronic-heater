#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// ============================================

// Control Outputs
#define GLOW_PLUG_PIN 25
#define DIESEL_PUMP_PIN 26          // Now supports PWM for variable output
#define AIR_FAN_PIN 27
#define COOLANT_PUMP_PIN 14
#define HEAT_EXCHANGER_FAN_PIN 12

// Zone Control Outputs (NEW for multi-zone)
#define FLOOR_VALVE_PIN 18          // Floor heating valve/pump control
#define WATER_VALVE_PIN 19          // Water heating valve/pump control

// Temperature Sensors - All DS18B20 on single OneWire bus
#define ONEWIRE_BUS_PIN 4           // All DS18B20 sensors share this pin with 4.7kΩ pullup

// Individual sensor addresses (set after discovery)
// Use TemperatureSensor::discoverSensors() to find addresses
// Then update these with actual 64-bit addresses from your sensors

// Optional Flow Sensor
#define FLOW_SENSOR_PIN 13

// ============================================
// PWM CONFIGURATION
// ============================================
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8

// PWM Channels
#define GLOW_PLUG_CHANNEL 0
#define DIESEL_PUMP_CHANNEL 1        // NEW: Diesel pump now uses PWM
#define AIR_FAN_CHANNEL 2
#define HEAT_EXCHANGER_FAN_CHANNEL 3

// ============================================
// TEMPERATURE THRESHOLDS (Celsius)
// ============================================

// Combustion Control
#define GLOW_PLUG_WARMUP_TEMP 800.0
#define IGNITION_TEMP 300.0
#define OPERATING_TEMP 600.0
#define MAX_SAFE_TEMP 900.0

// Coolant System
#define COOLANT_MIN_TEMP 40.0
#define COOLANT_MAX_TEMP 80.0

// Zone Temperature Limits (NEW)
#define FLOOR_MAX_TEMP 45.0         // Safety limit for floor
#define WATER_MAX_TEMP 80.0         // Safety limit for water tank
#define CABIN_TARGET_TEMP 20.0      // Default cabin target

// ============================================
// TIMING CONSTANTS (milliseconds)
// ============================================

// Startup Sequence
#define GLOW_PLUG_WARMUP_TIME 60000
#define STARTUP_SEQUENCE_TIME 120000

// Update Intervals
#define TEMP_READ_INTERVAL 1000
#define SAFETY_CHECK_INTERVAL 500
#define MQTT_PUBLISH_INTERVAL 5000   // NEW: Publish status every 5 seconds
#define WIFI_CHECK_INTERVAL 30000    // NEW: Check WiFi connection every 30 seconds

// Shutdown
#define COOLDOWN_TIME 60000

// ============================================
// POWER LEVEL PROFILES (NEW)
// Percentage of maximum power (0-100)
// ============================================
#define POWER_ECO 30                // Quiet, fuel-efficient
#define POWER_NORMAL 60             // Balanced performance
#define POWER_BOOST 100             // Maximum heat output

// ============================================
// MQTT CONFIGURATION (NEW)
// ============================================
#define MQTT_TOPIC_PREFIX "camper/heater"
#define MQTT_CLIENT_ID "hydronic_heater"
#define MQTT_RECONNECT_INTERVAL 5000

// MQTT Topics
#define MQTT_TOPIC_STATUS MQTT_TOPIC_PREFIX "/status"
#define MQTT_TOPIC_MODE MQTT_TOPIC_PREFIX "/mode"
#define MQTT_TOPIC_POWER MQTT_TOPIC_PREFIX "/power"
#define MQTT_TOPIC_CMD_MODE MQTT_TOPIC_PREFIX "/cmd/set_mode"
#define MQTT_TOPIC_CMD_TARGET MQTT_TOPIC_PREFIX "/cmd/set_target"
#define MQTT_TOPIC_CMD_POWER MQTT_TOPIC_PREFIX "/cmd/set_power"

// Temperature topics (published individually)
#define MQTT_TOPIC_TEMP MQTT_TOPIC_PREFIX "/temperature"

// ============================================
// WIFI CONFIGURATION (NEW)
// ============================================
#define WIFI_CONNECT_TIMEOUT 30000   // 30 seconds to connect
#define WIFI_AP_NAME "Heater_Setup"  // Fallback AP for configuration
#define WIFI_AP_PASSWORD "heater123" // AP password

// ============================================
// CONTROL PARAMETERS (NEW)
// ============================================

// PID Constants for temperature control (tune these)
#define PID_KP 2.0
#define PID_KI 0.5
#define PID_KD 1.0

// Temperature control hysteresis
#define TEMP_HYSTERESIS 2.0         // ±2°C to prevent oscillation

// Fuel pump calibration (PWM to fuel flow)
// These need to be calibrated for specific heater model
#define FUEL_PUMP_MIN_PWM 64        // Minimum for stable combustion
#define FUEL_PUMP_MAX_PWM 255       // Maximum safe fuel delivery

// Fan speed mapping to power level
#define FAN_MIN_SPEED 100           // Minimum fan speed for combustion
#define FAN_MAX_SPEED 255           // Maximum fan speed

#endif
