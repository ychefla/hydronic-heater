#ifndef CONFIG_H
#define CONFIG_H

// Pin Configuration
#define GLOW_PLUG_PIN 25
#define DIESEL_PUMP_PIN 26
#define AIR_FAN_PIN 27
#define COOLANT_PUMP_PIN 14
#define HEAT_EXCHANGER_FAN_PIN 12

// Temperature Sensor Pins (DS18B20 OneWire)
#define BURNING_CHAMBER_TEMP_PIN 4
#define COOLANT_INPUT_TEMP_PIN 16
#define COOLANT_OUTPUT_TEMP_PIN 17
#define AIR_TEMP_PIN 5

// Optional Flow Sensor Pin
#define FLOW_SENSOR_PIN 13

// PWM Configuration
#define PWM_FREQUENCY 5000
#define PWM_RESOLUTION 8

// PWM Channels
#define GLOW_PLUG_CHANNEL 0
#define AIR_FAN_CHANNEL 1
#define HEAT_EXCHANGER_FAN_CHANNEL 2

// Temperature Thresholds (in Celsius)
#define GLOW_PLUG_WARMUP_TEMP 800.0
#define IGNITION_TEMP 300.0
#define OPERATING_TEMP 600.0
#define MAX_SAFE_TEMP 900.0
#define COOLANT_MIN_TEMP 40.0
#define COOLANT_MAX_TEMP 80.0

// Timing Constants (in milliseconds)
#define GLOW_PLUG_WARMUP_TIME 60000  // 60 seconds
#define STARTUP_SEQUENCE_TIME 120000  // 2 minutes
#define TEMP_READ_INTERVAL 1000       // 1 second
#define SAFETY_CHECK_INTERVAL 500     // 0.5 seconds

#endif
