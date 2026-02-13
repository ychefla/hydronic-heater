# Extended Requirements Specification
## Camper Van Hydronic Diesel Heater Controller System

**Version:** 3.0  
**Date:** February 2026  
**Status:** Updated for Autoterm Flow 5D hybrid approach

---

## 1. Introduction

### 1.1 Purpose

This document specifies requirements for an ESP32-based smart controller for the **Autoterm Flow 5D** hydronic diesel heater, using a **hybrid architecture**: Autoterm handles combustion safety, ESP32 provides multi-zone control, scheduling, and cloud integration.

### 1.2 Target Heater

**Primary Target**:
- **Autoterm Flow 5D** (12V variant)
- 1.4–5.0 kW continuously variable output
- Documented UART protocol for external integration
- Certified combustion controller (E-mark, CE)
- Built-in circulation pump

**Why Autoterm (not Chinese heaters)**:
- HCalory and similar brands refused to provide protocol documentation
- Reverse-engineering proprietary protocols is unreliable for a safety-critical system
- Emulating the remote controller provides no additional functionality
- Autoterm provides integration documentation to developers
- Certified combustion controller eliminates the need to implement safety-critical fuel/ignition control

### 1.3 Camper Van Application

**Use Cases**:
1. **Floor Heating**: Circulate hot coolant through underfloor heating loops
2. **Air Heating**: Heat exchanger with fans for cabin air warming
3. **Water Heating**: Heat water for washing hands, dishes, showers
4. **Remote Control**: Control and monitor from inside living area
5. **Scheduled Heating**: Pre-warm before waking or returning
6. **Continuous Operation**: Maintain temperature for extended periods

---

## 2. New Functional Requirements

### 2.1 Operating Modes

#### FR-20: Continuous Heating Mode
**Priority**: Critical

**Requirements**:
- FR-20.1: System SHALL support continuous heating mode
- FR-20.2: In continuous mode, system SHALL maintain target temperature indefinitely
- FR-20.3: System SHALL modulate power output based on heat demand
- FR-20.4: System SHALL support configurable target temperatures for each zone
- FR-20.5: System SHALL optimize fuel consumption in continuous mode

**Rationale**: Essential for overnight heating and extended stays

#### FR-21: Timed Heating Mode
**Priority**: High

**Requirements**:
- FR-21.1: System SHALL support scheduled heating with start/stop times
- FR-21.2: System SHALL support multiple daily schedules (min 4 time slots)
- FR-21.3: System SHALL support day-of-week scheduling
- FR-21.4: System SHALL persist schedules across power cycles
- FR-21.5: System SHALL provide countdown/preview of next scheduled event

**Example**: Start heating 30 minutes before wake-up time

#### FR-22: Manual Mode
**Priority**: High

**Requirements**:
- FR-22.1: System SHALL support immediate on/off control
- FR-22.2: Manual mode SHALL override scheduled heating
- FR-22.3: System SHALL allow returning to scheduled mode

### 2.2 Heat Output Control

#### FR-23: Variable Power Output
**Priority**: Critical

**Requirements**:
- FR-23.1: System SHALL support variable heat output (20%-100%)
- FR-23.2: System SHALL control output by modulating fuel pump duty cycle
- FR-23.3: System SHALL adjust air fan speed proportionally to fuel rate
- FR-23.4: System SHALL maintain safe combustion at all power levels
- FR-23.5: System SHALL support user-settable power limits

**Rationale**: Match heat output to demand, save fuel, quieter operation

#### FR-24: Temperature Feedback Control
**Priority**: High

**Requirements**:
- FR-24.1: System SHALL support PID or similar control algorithm
- FR-24.2: System SHALL measure room/zone temperatures
- FR-24.3: System SHALL automatically adjust power to maintain target temp
- FR-24.4: System SHALL support separate targets for each zone
- FR-24.5: System SHALL support configurable temperature hysteresis

**Example**: Maintain cabin at 20°C, water tank at 40°C

### 2.3 Remote Control

#### FR-25: MQTT Integration
**Priority**: Critical

**Requirements**:
- FR-25.1: System SHALL support MQTT protocol for remote control
- FR-25.2: System SHALL connect to WiFi access point
- FR-25.3: System SHALL support MQTT broker connection with authentication
- FR-25.4: System SHALL publish status updates to MQTT topics
- FR-25.5: System SHALL subscribe to command topics
- FR-25.6: System SHALL support MQTT auto-discovery (Home Assistant compatible)
- FR-25.7: System SHALL reconnect automatically on WiFi/MQTT loss

**MQTT Topics**:
```
heater/status          - JSON status updates
heater/temperature/*   - Individual temperature readings
heater/mode            - Current operating mode
heater/power           - Current power level
heater/cmd/set_mode    - Set operating mode
heater/cmd/set_target  - Set target temperature
heater/cmd/set_power   - Set power level
```

#### FR-26: Web Interface (Optional Future)
**Priority**: Low

**Requirements**:
- FR-26.1: System MAY provide web interface for configuration
- FR-26.2: If implemented, SHALL be accessible via WiFi
- FR-26.3: If implemented, SHALL support mobile-responsive design

### 2.4 Multi-Zone Heating

#### FR-27: Floor Heating Control
**Priority**: High

**Requirements**:
- FR-27.1: System SHALL support dedicated floor heating zone
- FR-27.2: System SHALL control floor heating valve/pump
- FR-27.3: System SHALL monitor floor supply/return temperatures
- FR-27.4: System SHALL support floor temperature limits (max 40°C)
- FR-27.5: System SHALL allow independent floor zone on/off

#### FR-28: Air Heating Control
**Priority**: High

**Requirements**:
- FR-28.1: System SHALL support air heating via heat exchanger
- FR-28.2: System SHALL control heat exchanger fan speed
- FR-28.3: System SHALL monitor cabin air temperature
- FR-28.4: System SHALL prioritize air heating for rapid warm-up
- FR-28.5: System SHALL support boost mode for maximum air heating

#### FR-29: Water Heating Control
**Priority**: Medium

**Requirements**:
- FR-29.1: System SHALL support water heating circuit
- FR-29.2: System SHALL control water heating valve/circuit
- FR-29.3: System SHALL monitor water tank temperature
- FR-29.4: System SHALL support water temperature target (e.g., 40-60°C)
- FR-29.5: System SHALL prevent water overheating (max 80°C)

### 2.5 Advanced Features

#### FR-30: Power Level Profiles
**Priority**: Medium

**Requirements**:
- FR-30.1: System SHALL support named power profiles (Eco, Normal, Boost)
- FR-30.2: Eco mode: 20-40% power, quiet operation
- FR-30.3: Normal mode: 40-70% power, balanced
- FR-30.4: Boost mode: 70-100% power, maximum heat
- FR-30.5: System SHALL allow switching profiles via MQTT

#### FR-31: Smart Features
**Priority**: Low

**Requirements**:
- FR-31.1: System MAY learn heating patterns and pre-heat
- FR-31.2: System MAY adjust based on outside temperature
- FR-31.3: System MAY provide fuel consumption estimates
- FR-31.4: System MAY alert on low fuel (if sensor available)

---

## 3. Updated Hardware Requirements

### 3.1 Additional Sensors

#### HR-10: Room/Zone Temperature Sensors
**Priority**: High

**Requirements**:
- HR-10.1: SHALL support minimum 3 additional DS18B20 sensors
- HR-10.2: Room temperature sensor (cabin)
- HR-10.3: Floor supply/return temperature sensors
- HR-10.4: Water tank temperature sensor

#### HR-11: Outside Temperature Sensor (Optional)
**Priority**: Low

**Requirements**:
- HR-11.1: MAY support outside temperature sensor
- HR-11.2: Used for smart pre-heating and efficiency

### 3.2 Additional Outputs

#### HR-12: Zone Control Valves/Pumps
**Priority**: High

**Requirements**:
- HR-12.1: SHALL support floor heating valve control
- HR-12.2: SHALL support water heating valve control
- HR-12.3: MAY use 3-way motorized valves or separate pumps
- HR-12.4: Control via GPIO or PWM as appropriate

### 3.3 Connectivity

#### HR-13: WiFi Connectivity
**Priority**: Critical

**Requirements**:
- HR-13.1: SHALL use ESP32 built-in WiFi
- HR-13.2: SHALL support 2.4GHz WiFi (802.11 b/g/n)
- HR-13.3: SHALL support WPA2 authentication
- HR-13.4: SHALL store WiFi credentials securely

---

## 4. Updated Software Requirements

### 4.1 Additional Libraries

#### SR-10: MQTT Library
**Priority**: Critical

**Requirements**:
- SR-10.1: SHALL use PubSubClient or similar MQTT library
- SR-10.2: SHALL support QoS levels
- SR-10.3: SHALL support retained messages
- SR-10.4: SHALL handle reconnection automatically

#### SR-11: WiFi Management
**Priority**: High

**Requirements**:
- SR-11.1: SHALL use WiFiManager or similar for configuration
- SR-11.2: SHALL provide fallback AP mode for initial setup
- SR-11.3: SHALL store credentials in EEPROM/NVS

#### SR-12: Time Synchronization
**Priority**: High

**Requirements**:
- SR-12.1: SHALL use NTP for time synchronization
- SR-12.2: SHALL maintain time for scheduling
- SR-12.3: SHALL handle timezone configuration

#### SR-13: JSON Handling
**Priority**: Medium

**Requirements**:
- SR-13.1: SHALL use ArduinoJson for MQTT payloads
- SR-13.2: SHALL serialize status to JSON
- SR-13.3: SHALL parse commands from JSON

### 4.2 Data Persistence

#### SR-14: Configuration Storage
**Priority**: High

**Requirements**:
- SR-14.1: SHALL store settings in ESP32 NVS (non-volatile storage)
- SR-14.2: SHALL persist: WiFi, MQTT, schedules, targets, zones
- SR-14.3: SHALL survive power cycles and resets
- SR-14.4: SHALL provide factory reset capability

---

## 5. Updated Pin Assignments

### 5.1 Expanded GPIO Map

```cpp
// === Autoterm UART Communication ===
#define AUTOTERM_RX_PIN  16         // ESP32 RX <- Autoterm TX
#define AUTOTERM_TX_PIN  17         // ESP32 TX -> Autoterm RX
#define AUTOTERM_BAUD    2400       // Verify with Autoterm documentation

// === Zone Control Outputs ===
#define HEAT_EXCHANGER_FAN_PIN 12   // PWM - cabin air heating
#define FLOOR_VALVE_PIN 18          // Floor heating valve/pump
#define WATER_VALVE_PIN 19          // Water heating valve/pump

// === Temperature Sensors (OneWire bus) ===
#define ONEWIRE_BUS_PIN 4           // Single bus for all DS18B20
// Sensors identified by unique 64-bit addresses:
// - Cabin air, floor supply, floor return, water tank, outside (optional)

// === Optional ===
#define FLOW_SENSOR_PIN 13

// REMOVED (Autoterm handles combustion):
// #define GLOW_PLUG_PIN 25        -- Autoterm controller
// #define DIESEL_PUMP_PIN 26      -- Autoterm controller
// #define AIR_FAN_PIN 27          -- Autoterm controller
// #define COOLANT_PUMP_PIN 14     -- Autoterm built-in pump
```

---

## 6. Updated Configuration

### 6.1 Configuration Structure

**config.h** remains for hardware pin assignments

**NEW: config.json** stored in NVS:
```json
{
  "wifi": {
    "ssid": "CamperVan_WiFi",
    "password": "encrypted"
  },
  "mqtt": {
    "broker": "192.168.1.100",
    "port": 1883,
    "user": "heater",
    "password": "encrypted",
    "topic_prefix": "camper/heater"
  },
  "zones": {
    "cabin": {
      "enabled": true,
      "target": 20.0,
      "sensor": "28-ff-64-1e-0c-00-00-3c"
    },
    "floor": {
      "enabled": true,
      "target": 25.0,
      "max_temp": 40.0
    },
    "water": {
      "enabled": false,
      "target": 45.0,
      "max_temp": 80.0
    }
  },
  "schedules": [
    {
      "name": "Morning",
      "days": [1,2,3,4,5],
      "start": "06:30",
      "duration": 120,
      "mode": "boost"
    }
  ],
  "power": {
    "eco": 30,
    "normal": 60,
    "boost": 100
  }
}
```

---

## 7. Home Assistant Integration Example

```yaml
# configuration.yaml
mqtt:
  sensor:
    - name: "Heater Cabin Temp"
      state_topic: "camper/heater/temperature/cabin"
      unit_of_measurement: "°C"
      device_class: temperature
    
    - name: "Heater Status"
      state_topic: "camper/heater/status"
      value_template: "{{ value_json.state }}"
  
  climate:
    - name: "Camper Heater"
      mode_command_topic: "camper/heater/cmd/set_mode"
      mode_state_topic: "camper/heater/mode"
      temperature_command_topic: "camper/heater/cmd/set_target"
      temperature_state_topic: "camper/heater/target"
      current_temperature_topic: "camper/heater/temperature/cabin"
      modes:
        - "off"
        - "heat"
        - "auto"
      min_temp: 5
      max_temp: 30
```

---

## 8. Autoterm Flow 5D Integration

### 8.1 Hybrid Architecture Benefits

**Autoterm Controller Handles (combustion-critical)**:
1. Glow plug ignition and management
2. Fuel metering (variable 0.18–0.62 L/hr)
3. Combustion air fan speed control
4. Flame monitoring and safety shutdown
5. Supply voltage protection
6. Error diagnostics and codes

**ESP32 Smart Layer Handles (application-level)**:
1. Multi-zone temperature control via PID
2. Zone valve management (floor, water, air)
3. Power setpoint commands to Autoterm via UART
4. Scheduling and power profiles
5. MQTT/Paku-IoT cloud integration
6. Telemetry aggregation (sensors + Autoterm data)

### 8.2 UART Communication Requirements

**Protocol**:
- Binary frame-based protocol via UART
- Baud rate: typically 2400 (verify with Autoterm documentation)
- Request integration docs from Autoterm: support@autoterm.com

**Required Commands**:
- Start/stop heater
- Set power level (percentage)
- Set target temperature
- Request status and telemetry
- Read error codes

---

## 9. Safety Updates

### 9.1 Additional Safety Requirements

#### SAFE-10: Zone Overtemperature
**Priority**: Critical

**Requirements**:
- SAFE-10.1: Monitor all zone temperatures
- SAFE-10.2: Shut down if floor > 50°C
- SAFE-10.3: Shut down if water > 85°C
- SAFE-10.4: Alert via MQTT on approach to limits

#### SAFE-11: WiFi/MQTT Loss Handling
**Priority**: High

**Requirements**:
- SAFE-11.1: Continue operation if WiFi lost
- SAFE-11.2: Use last known settings
- SAFE-11.3: Revert to safe defaults after extended loss
- SAFE-11.4: Indicate connection status

#### SAFE-12: Watchdog Timer
**Priority**: High

**Requirements**:
- SAFE-12.1: Enable ESP32 hardware watchdog
- SAFE-12.2: Reset if system hangs
- SAFE-12.3: Log watchdog resets

---

## 10. Typical Camper Van Installation

```
┌─────────────────────────────────────────────────────────┐
│                    Camper Van Layout                    │
│                                                         │
│  ┌──────────┐              ┌────────────┐              │
│  │  Diesel  │              │   Water    │              │
│  │  Heater  │◄────────────►│   Tank     │              │
│  │  Unit    │   Coolant    │  (heated)  │              │
│  └────┬─────┘              └────────────┘              │
│       │                                                 │
│       │ Coolant Circuit                                │
│       │                                                 │
│  ┌────▼─────────────────────────────────────────────┐  │
│  │          3-Way Valve Distribution               │  │
│  │          (Floor / Air / Water)                  │  │
│  └────┬───────────────┬──────────────────┬─────────┘  │
│       │               │                  │            │
│  ┌────▼──────┐  ┌─────▼─────┐   ┌───────▼────────┐   │
│  │   Floor   │  │   Heat    │   │  Water Tank    │   │
│  │  Heating  │  │ Exchanger │   │    Coil        │   │
│  │   Loops   │  │  + Fans   │   │                │   │
│  └───────────┘  └───────────┘   └────────────────┘   │
│                                                        │
│  ┌──────────────────────────────────────────────────┐ │
│  │           ESP32 Controller                       │ │
│  │  - Heater control                               │ │
│  │  - Zone valves                                  │ │
│  │  - Temperature monitoring                       │ │
│  │  - WiFi/MQTT                                    │ │
│  └──────────────────────────────────────────────────┘ │
│                                                        │
│  WiFi → Phone/Tablet for Control                      │
└────────────────────────────────────────────────────────┘
```

---

## 11. Requirements Priority Summary

| Priority | Count | Examples |
|----------|-------|----------|
| Critical | 8 | MQTT, Continuous mode, Variable power, Safety |
| High | 12 | Timed heating, Zone control, Temperature feedback |
| Medium | 6 | Water heating, Power profiles, JSON handling |
| Low | 4 | Web interface, Smart features, Outside temp |

---

## 12. Implementation Phases

### Phase 1: Core Extensions (Immediate)
- Variable power output control
- Continuous heating mode
- WiFi connectivity
- Basic MQTT integration
- Single zone temperature control

### Phase 2: Multi-Zone (Next)
- Floor heating control
- Water heating control
- Zone-specific temperature management
- 3-way valve control

### Phase 3: Advanced Features
- Timed/scheduled heating
- Power profiles (Eco/Normal/Boost)
- Home Assistant auto-discovery
- Configuration web interface

### Phase 4: Smart Features (Future)
- Learning algorithms
- Fuel consumption tracking
- Predictive heating
- Mobile app integration

---

**Document History**:
- v3.0 (2026-02-12): Pivoted to Autoterm Flow 5D hybrid architecture
- v2.0 (2025-12-28): Updated for camper van application with MQTT and multi-zone support
- v1.0 (2025-12-28): Initial requirements specification created
