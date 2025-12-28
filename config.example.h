// Example custom configuration
// Copy this file to include/config.h and modify as needed

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PIN CONFIGURATION
// Modify these to match your wiring
// ============================================

// Control Outputs
#define GLOW_PLUG_PIN 25              // PWM controlled
#define DIESEL_PUMP_PIN 26            // Digital output
#define AIR_FAN_PIN 27                // PWM controlled
#define COOLANT_PUMP_PIN 14           // Digital output
#define HEAT_EXCHANGER_FAN_PIN 12     // PWM controlled

// Temperature Sensor Pins (DS18B20 OneWire)
// Each needs 4.7kΩ pullup resistor to VCC
#define BURNING_CHAMBER_TEMP_PIN 4
#define COOLANT_INPUT_TEMP_PIN 16
#define COOLANT_OUTPUT_TEMP_PIN 17
#define AIR_TEMP_PIN 5

// Optional Sensors
#define FLOW_SENSOR_PIN 13            // Pulse counter input

// ============================================
// PWM CONFIGURATION
// ============================================
#define PWM_FREQUENCY 5000            // 5 kHz
#define PWM_RESOLUTION 8              // 8-bit (0-255)

// PWM Channel Assignment
#define GLOW_PLUG_CHANNEL 0
#define AIR_FAN_CHANNEL 1
#define HEAT_EXCHANGER_FAN_CHANNEL 2

// ============================================
// TEMPERATURE THRESHOLDS (Celsius)
// Adjust based on your heater specifications
// ============================================

// Glow Plug & Combustion
#define GLOW_PLUG_WARMUP_TEMP 800.0   // Target glow plug temperature
#define IGNITION_TEMP 300.0            // Temperature indicating ignition success
#define OPERATING_TEMP 600.0           // Normal operating temperature
#define MAX_SAFE_TEMP 900.0            // Emergency shutdown temperature

// Coolant System
#define COOLANT_MIN_TEMP 40.0          // Minimum temp to start circulation
#define COOLANT_MAX_TEMP 80.0          // Maximum coolant temperature

// ============================================
// TIMING CONSTANTS (milliseconds)
// ============================================

// Startup Sequence
#define GLOW_PLUG_WARMUP_TIME 60000    // 60 seconds - glow plug preheating
#define STARTUP_SEQUENCE_TIME 120000   // 2 minutes - max time for ignition

// Update Intervals
#define TEMP_READ_INTERVAL 1000        // 1 second - temperature sensor reading
#define SAFETY_CHECK_INTERVAL 500      // 0.5 seconds - safety monitoring

// Shutdown
#define COOLDOWN_TIME 60000            // 60 seconds - cooldown period

// ============================================
// ADVANCED SETTINGS
// ============================================

// Fan Speed Levels (0-255)
#define FAN_SPEED_LOW 128
#define FAN_SPEED_MEDIUM 192
#define FAN_SPEED_HIGH 255

// Temperature Hysteresis
#define TEMP_HYSTERESIS 10.0           // °C - prevent oscillation

// Flow Sensor Calibration
#define FLOW_SENSOR_PULSES_PER_LITER 7.5  // Calibration factor

#endif
