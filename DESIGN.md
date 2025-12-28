# System Design Document
## Hydronic Diesel Heater Controller

**Version:** 1.0  
**Date:** December 2025  
**Author:** ESP32 Controller Project

---

## Table of Contents

1. [Overview](#1-overview)
2. [System Architecture](#2-system-architecture)
3. [Component Design](#3-component-design)
4. [State Machine Design](#4-state-machine-design)
5. [Safety Systems](#5-safety-systems)
6. [Control Algorithms](#6-control-algorithms)
7. [Data Flow](#7-data-flow)
8. [Interface Design](#8-interface-design)
9. [Performance Requirements](#9-performance-requirements)
10. [Design Decisions and Rationale](#10-design-decisions-and-rationale)

---

## 1. Overview

### 1.1 Purpose

This document describes the system design for an ESP32-based controller that replaces the original controller in a hydronic diesel heater. The system provides complete autonomous control over all heater components with safety monitoring and efficient operation.

### 1.2 Scope

The controller manages:
- **Combustion system**: Glow plug, diesel pump, air supply
- **Heat transfer**: Coolant circulation, heat exchanger
- **Monitoring**: Multiple temperature sensors, optional flow sensor
- **Safety**: Overtemperature protection, ignition monitoring, fault detection

### 1.3 Design Goals

1. **Safety First**: Fail-safe operation with multiple safety interlocks
2. **Autonomous Operation**: Automatic startup, operation, and shutdown sequences
3. **Modularity**: Component-based architecture for easy modification
4. **Reliability**: Robust sensor validation and error recovery
5. **Efficiency**: Optimized control algorithms for fuel and power consumption
6. **Maintainability**: Clear code structure and comprehensive documentation

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     ESP32 Controller                         │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │           Main Controller (State Machine)            │  │
│  │  - Startup/Shutdown sequences                       │  │
│  │  - Safety monitoring                                │  │
│  │  - Control coordination                             │  │
│  └────┬─────────────────────────────────────────┬──────┘  │
│       │                                          │          │
│  ┌────▼─────────────┐                  ┌────────▼──────┐  │
│  │  Output Control   │                  │  Sensor Input │  │
│  │  - PWM generation │                  │  - Temperature│  │
│  │  - Digital output │                  │  - Flow rate  │  │
│  └────┬─────────────┘                  └────────┬──────┘  │
└───────┼──────────────────────────────────────────┼─────────┘
        │                                          │
   ┌────▼────────────────────────────────────┐    │
   │         Physical Components              │    │
   ├──────────────────────────────────────────┤    │
   │ • Glow Plug (PWM)                       │    │
   │ • Diesel Pump (Digital)                 │    │
   │ • Air Fan (PWM)                         │    │
   │ • Coolant Pump (Digital)                │    │
   │ • Heat Exchanger Fan (PWM)              │    │
   └─────────────────────────────────────────┘    │
                                                   │
   ┌───────────────────────────────────────────────▼──┐
   │            Sensor Network                        │
   ├──────────────────────────────────────────────────┤
   │ • Burning Chamber Temperature (DS18B20)         │
   │ • Coolant Input Temperature (DS18B20)           │
   │ • Coolant Output Temperature (DS18B20)          │
   │ • Air Temperature (DS18B20)                     │
   │ • Coolant Flow Rate (Optional Pulse Counter)    │
   └─────────────────────────────────────────────────┘
```

### 2.2 Software Architecture Layers

```
┌─────────────────────────────────────────────────┐
│         Application Layer (main.cpp)            │
│  - User interface (serial commands)             │
│  - Status reporting                             │
│  - Command processing                           │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│      Controller Layer (controller.cpp)          │
│  - State machine logic                          │
│  - Safety monitoring                            │
│  - Control algorithms                           │
│  - Sensor data aggregation                      │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│     Component Layer (components.cpp)            │
│  - Hardware abstraction                         │
│  - Individual component control                 │
│  - Sensor reading                               │
└────────────────┬────────────────────────────────┘
                 │
┌────────────────▼────────────────────────────────┐
│      Hardware Abstraction Layer (HAL)           │
│  - Arduino framework                            │
│  - ESP32 drivers (PWM, GPIO, OneWire)           │
└─────────────────────────────────────────────────┘
```

### 2.3 Design Patterns Used

1. **State Pattern**: Main controller uses state machine for operational modes
2. **Singleton**: FlowSensor uses singleton for interrupt handling
3. **Encapsulation**: Each component class encapsulates hardware details
4. **Composition**: Controller composed of component objects
5. **Template Method**: Update pattern consistent across components

---

## 3. Component Design

### 3.1 Component Class Hierarchy

```
┌─────────────────────────────────────────────────┐
│          Base Component Concept                 │
│  (No explicit base class - implicit interface)  │
│                                                 │
│  Common interface pattern:                      │
│    - begin()      : Initialize hardware         │
│    - update()     : Update state (if needed)    │
│    - getState()   : Query current state         │
└───────────┬─────────────────────────────────────┘
            │
     ┌──────┴────────┬────────────┬─────────────┐
     │               │            │             │
┌────▼────┐  ┌──────▼──────┐  ┌─▼────┐  ┌─────▼──────┐
│ Output  │  │   Sensor    │  │ Pump │  │ FlowSensor │
│ Control │  │   Input     │  │      │  │            │
└─────────┘  └─────────────┘  └──────┘  └────────────┘
     │               │            │             │
     ├──GlowPlug     └──Temp...   ├──Diesel    └──(pulse)
     └──Fan                        └──Coolant
```

### 3.2 Output Control Components

**GlowPlug & Fan Classes (PWM Control)**
- Use ESP32 LEDC peripheral for hardware PWM
- 8-bit resolution (0-255 values)
- 5 kHz frequency (configurable)
- Advantages:
  - CPU-independent operation
  - Precise duty cycle control
  - No software timing requirements

**Pump Classes (Digital Control)**
- Simple on/off control via GPIO
- Direct MOSFET/relay activation
- Binary state tracking

### 3.3 Input Components

**TemperatureSensor (DS18B20)**
- OneWire protocol communication
- Individual addressing capability
- Built-in ADC (0.0625°C resolution)
- Operating range: -55°C to +125°C
- Update strategy: Poll-based (1 second interval)

**FlowSensor (Pulse Counter)**
- Interrupt-driven pulse counting
- Calculated flow rate: pulses/time * calibration
- Update interval: 1 second
- Uses ESP32 hardware interrupt

### 3.4 Component Responsibilities

| Component | Responsibilities | Dependencies |
|-----------|-----------------|--------------|
| GlowPlug | PWM control, heating time tracking | LEDC driver |
| DieselPump | Fuel delivery on/off | GPIO |
| Fan | Variable speed control | LEDC driver |
| CoolantPump | Coolant circulation on/off | GPIO |
| TemperatureSensor | Temperature reading, validation | OneWire, DallasTemperature |
| FlowSensor | Flow rate calculation | GPIO interrupt |
| Controller | Orchestration, state management, safety | All components |

---

## 4. State Machine Design

### 4.1 State Diagram

```
                    ┌───────┐
                    │  OFF  │◄──────────────────┐
                    └───┬───┘                   │
                        │ start()               │
                        ▼                       │
              ┌─────────────────────┐          │
              │ GLOW_PLUG_WARMUP    │          │
              │ (60 seconds)         │          │
              │ - Glow plug ON       │          │
              └──────────┬───────────┘          │
                         │ Timer expires        │
                         ▼                      │
              ┌─────────────────────┐          │
              │    IGNITION          │          │
              │ (up to 2 minutes)    │          │
              │ - Pump ON            │──timeout─┤
              │ - Air fan LOW        │    ▼     │
              │ - Wait for temp      │ ┌─────┐ │
              └──────────┬───────────┘ │ERROR│ │
                         │ Temp > 300°C └──┬──┘ │
                         ▼                 │    │
              ┌─────────────────────┐      │    │
              │     RUNNING          │      │    │
              │ - Active control     │      │    │
              │ - Temperature mgmt   │◄overtemp─┘
              │ - Coolant circulation│      
              └──────────┬───────────┘      
                         │ stop()           
                         ▼                  
              ┌─────────────────────┐      
              │    SHUTDOWN          │      
              │ (60 seconds)         │      
              │ - Fuel OFF           │      
              │ - Fans HIGH (cooling)│      
              └──────────┬───────────┘      
                         │ Timer expires    
                         └──────────────────┘
```

### 4.2 State Descriptions

| State | Entry Actions | Active Behaviors | Exit Conditions |
|-------|--------------|------------------|-----------------|
| **OFF** | Turn off all components | Monitor for start command | User start command |
| **GLOW_PLUG_WARMUP** | Activate glow plug at full power | Monitor timer | 60 seconds elapsed |
| **IGNITION** | Start pump, air fan low speed | Monitor temperature rise | Temp > 300°C OR timeout |
| **RUNNING** | Adjust all systems | Active control algorithms, safety monitoring | User stop OR safety trigger |
| **SHUTDOWN** | Stop fuel delivery | Run cooling fans, monitor cooldown | 60 seconds elapsed |
| **ERROR** | Emergency stop fuel | Maintain cooling, log error | Manual reset (stop command) |

### 4.3 State Transition Rules

**Priority Order:**
1. Safety overrides (highest priority)
2. Timer-based transitions
3. Temperature-based transitions
4. User commands (lowest priority)

**Transition Guards:**
- WARMUP → IGNITION: Only if timer >= 60s
- IGNITION → RUNNING: Only if temperature valid and > 300°C
- Any → ERROR: Immediate on safety violation
- ERROR → OFF: Only via explicit stop command

---

## 5. Safety Systems

### 5.1 Safety Architecture

```
┌─────────────────────────────────────────────────┐
│           Safety Monitoring Layer               │
├─────────────────────────────────────────────────┤
│                                                 │
│  ┌─────────────┐  ┌──────────────┐  ┌────────┐│
│  │Overtemper-  │  │  Ignition    │  │ Sensor ││
│  │ature        │  │  Timeout     │  │ Health ││
│  │Protection   │  │  Detection   │  │ Check  ││
│  └──────┬──────┘  └──────┬───────┘  └────┬───┘│
│         │                │                │    │
│         └────────┬───────┴────────────────┘    │
│                  ▼                              │
│         ┌────────────────┐                      │
│         │ Safety Handler │                      │
│         │ - Emergency    │                      │
│         │   fuel cutoff  │                      │
│         │ - Error logging│                      │
│         └────────────────┘                      │
└─────────────────────────────────────────────────┘
```

### 5.2 Safety Features

**1. Overtemperature Protection**
- **Trigger**: Burning chamber > 900°C
- **Action**: 
  - Immediate fuel cutoff (diesel pump OFF)
  - Glow plug OFF
  - Maximum cooling (fans HIGH)
  - Enter ERROR state
- **Recovery**: Manual intervention required

**2. Ignition Timeout**
- **Trigger**: No ignition within 2 minutes
- **Rationale**: Prevents fuel flooding
- **Action**: Stop fuel delivery, enter ERROR state
- **Recovery**: Manual restart after investigation

**3. Sensor Validation**
- **Check**: Temperature readings in valid range (-55°C to +125°C)
- **Action**: Warning logged, continue operation (degraded mode)
- **Rationale**: Allow operation with partial sensor failure

**4. Cooldown Enforcement**
- **Trigger**: Any shutdown (normal or emergency)
- **Duration**: Minimum 60 seconds
- **Purpose**: Prevent thermal shock, component damage
- **Action**: Maintain fan operation, prevent premature restart

### 5.3 Safety Timing

| Check | Frequency | Priority |
|-------|-----------|----------|
| Overtemperature | 500 ms | Critical |
| Sensor validity | 1000 ms | High |
| Ignition progress | 1000 ms | High |
| State consistency | Every loop | Medium |

### 5.4 Fail-Safe Behaviors

1. **Power Loss**: All components default to OFF state
2. **Sensor Failure**: Continue with warning (allow manual monitoring)
3. **Communication Loss**: Maintain last safe state
4. **Software Crash**: Watchdog timer resets ESP32 (all outputs OFF)

---

## 6. Control Algorithms

### 6.1 Temperature Control Strategy

**Combustion Temperature Control**
```
Target: 600°C (OPERATING_TEMP)

Control Logic:
├── If temp < 550°C (below target)
│   ├── Air fan: LOW (128/255)
│   └── Glow plug: ON (if < 600°C)
│
├── If temp: 550-650°C (in range)
│   ├── Air fan: MEDIUM (192/255)
│   └── Glow plug: OFF
│
└── If temp > 650°C (above target)
    ├── Air fan: HIGH (255/255)
    └── Glow plug: OFF

Safety Override:
└── If temp > 900°C: EMERGENCY STOP
```

**Coolant Temperature Control**
```
Operating Range: 40-80°C

Control Logic:
├── If output temp < 40°C
│   └── Coolant pump: OFF (minimize heat loss)
│
├── If output temp: 40-70°C
│   ├── Coolant pump: ON
│   └── Heat exchanger fan: LOW (128/255)
│
└── If output temp > 70°C
    ├── Coolant pump: ON
    └── Heat exchanger fan: HIGH (255/255)
```

### 6.2 Startup Sequence Algorithm

```
Phase 1: GLOW_PLUG_WARMUP (60 seconds)
├── Glow plug: 100% power
├── All other components: OFF
└── Purpose: Preheat combustion chamber

Phase 2: IGNITION (up to 120 seconds)
├── Glow plug: 100% power
├── Diesel pump: ON
├── Air fan: 50% power (128/255)
├── Monitor: Burning chamber temperature
├── Success: temp > 300°C within 120s
└── Failure: Timeout → ERROR state

Phase 3: RUNNING
├── Glow plug: Reduce to 25% power, then OFF when temp > 600°C
├── Diesel pump: ON continuously
├── Air fan: Variable (temperature-dependent)
├── Coolant pump: ON when output > 40°C
└── Heat exchanger fan: Variable (coolant temp-dependent)
```

### 6.3 Shutdown Sequence Algorithm

```
Phase 1: Fuel Cutoff (immediate)
├── Diesel pump: OFF
└── Glow plug: OFF

Phase 2: Cooldown (60 seconds)
├── Air fan: 100% power (maximum cooling)
├── Heat exchanger fan: 100% power
├── Coolant pump: ON
└── Monitor: Temperature decrease

Phase 3: Complete Shutdown
├── All components: OFF
└── Ready for next cycle
```

### 6.4 Control Update Rates

| Function | Update Rate | Reason |
|----------|-------------|--------|
| Temperature reading | 1 Hz | DS18B20 conversion time ~750ms |
| Safety checks | 2 Hz | Fast response to hazards |
| Control adjustments | 1 Hz | Sufficient for thermal processes |
| Flow sensor update | 1 Hz | Accurate flow calculation |
| Status reporting | 0.1 Hz | User information |

---

## 7. Data Flow

### 7.1 Sensor Data Flow

```
Sensors → TemperatureSensor → Controller → Control Logic → Actuators
                             ↓
                        Validation
                             ↓
                     Safety Checks
                             ↓
                      User Display
```

### 7.2 Command Flow

```
User Input → Serial Parser → Command Handler → Controller Methods
                                                     ↓
                                            State Transitions
                                                     ↓
                                            Component Actions
```

### 7.3 Main Loop Data Flow

```
loop()
  │
  ├─► controller.update()
  │     │
  │     ├─► Read all sensors (1 Hz)
  │     │     ├─► burningChamberTemp.update()
  │     │     ├─► coolantInputTemp.update()
  │     │     ├─► coolantOutputTemp.update()
  │     │     └─► airTemp.update()
  │     │
  │     ├─► Safety checks (2 Hz)
  │     │     ├─► Overtemperature check
  │     │     └─► Sensor validity check
  │     │
  │     └─► State machine update
  │           ├─► Execute current state logic
  │           ├─► Check transition conditions
  │           └─► Update component outputs
  │
  ├─► Process serial commands (as available)
  │     └─► start/stop/status
  │
  └─► Periodic status output (0.1 Hz)
```

---

## 8. Interface Design

### 8.1 Serial Communication Protocol

**Format**: ASCII text commands with newline termination

**Commands**:
```
start\n    - Initiate heater startup sequence
stop\n     - Initiate safe shutdown sequence
status\n   - Print current system status
```

**Status Output Format**:
```
===== Heater Status =====
State: RUNNING
Temperatures:
  Burning Chamber: 625.5 °C
  Coolant Input: 65.2 °C
  Coolant Output: 72.8 °C
  Air: 22.3 °C
Components:
  Glow Plug: OFF
  Diesel Pump: ON
  Air Fan: 192/255
  Coolant Pump: ON
  Heat Exchanger Fan: 255/255
  Coolant Flow: 3.2 L/min
========================
```

### 8.2 Configuration Interface

**File-based**: `include/config.h`
- Compile-time configuration
- Type-safe definitions
- Well-documented constants

**Benefits**:
- No runtime overhead
- Compiler optimization
- Easy version control

### 8.3 Future Extensions

**Potential Additions**:
1. **WiFi/Bluetooth**: Remote monitoring and control
2. **MQTT**: Integration with home automation
3. **Web Interface**: Graphical status and control
4. **Data Logging**: SD card or cloud storage
5. **PID Control**: More sophisticated temperature control

---

## 9. Performance Requirements

### 9.1 Timing Requirements

| Requirement | Target | Critical? |
|-------------|--------|-----------|
| Safety check response | < 500 ms | Yes |
| Temperature reading | < 1 second | No |
| State transition | < 100 ms | Yes |
| Command response | < 100 ms | No |
| Startup to ignition | 60-180 s | No |
| Cooldown duration | 60 s | Yes |

### 9.2 Resource Requirements

**Memory**:
- Flash: ~50 KB (code + constants)
- RAM: ~10 KB (objects + stack)
- Available headroom: Large (ESP32 has 320KB RAM, 4MB flash)

**CPU**:
- Main loop: ~1% (mostly idle)
- Interrupt handlers: < 0.1%
- Available for extensions: 98%+

**Power**:
- ESP32: ~80 mA active
- Total system: Dominated by actuators (10-100A)

### 9.3 Scalability

**Current Design Supports**:
- Up to 8 temperature sensors (limited by OneWire)
- 16 PWM channels (ESP32 limitation)
- Multiple fan zones
- Additional safety sensors

**Expansion Considerations**:
- Modular component design allows easy addition
- State machine extensible
- Configuration system scales well

---

## 10. Design Decisions and Rationale

### 10.1 Key Design Decisions

**1. ESP32 Platform Choice**
- **Decision**: Use ESP32 microcontroller
- **Rationale**:
  - Sufficient I/O pins (30+ GPIO)
  - Hardware PWM (16 channels)
  - Built-in WiFi/Bluetooth for future expansion
  - Low cost, wide availability
  - Strong Arduino framework support
  - Dual-core for potential RTOS use

**2. Component-Based Architecture**
- **Decision**: Separate class for each component type
- **Rationale**:
  - Encapsulation of hardware details
  - Reusability (multiple fans, pumps)
  - Testability (mock components)
  - Clear responsibility boundaries
  - Easy to modify or replace components

**3. State Machine Pattern**
- **Decision**: Explicit state machine for controller
- **Rationale**:
  - Clear operational phases
  - Predictable behavior
  - Safety-critical transitions
  - Easy to visualize and verify
  - Testable state logic

**4. Poll-Based Sensor Reading**
- **Decision**: Periodic polling vs. continuous reading
- **Rationale**:
  - DS18B20 has inherent conversion delay
  - 1 Hz sufficient for thermal processes
  - Predictable timing
  - Lower CPU usage
  - Simpler code

**5. PWM vs. On-Off Control**
- **Decision**: PWM for fans and glow plug, digital for pumps
- **Rationale**:
  - Fans benefit from variable speed (efficiency, noise)
  - Glow plug benefits from power modulation
  - Pumps typically binary devices
  - PWM more flexible for tuning

**6. Interrupt-Based Flow Sensor**
- **Decision**: Hardware interrupt for pulse counting
- **Rationale**:
  - No missed pulses
  - Accurate at high flow rates
  - Minimal CPU usage
  - Real-time counting

**7. Fail-Safe Design**
- **Decision**: Default-off, explicit enable
- **Rationale**:
  - Safety-critical application
  - Power loss = safe state
  - Software crash = safe state
  - Prevents runaway heating

**8. Arduino Framework**
- **Decision**: Use Arduino instead of ESP-IDF
- **Rationale**:
  - Lower barrier to entry
  - Extensive library ecosystem
  - Simpler code for this application
  - Community support
  - Can migrate to ESP-IDF if needed

### 10.2 Trade-offs

| Decision | Pros | Cons | Justification |
|----------|------|------|---------------|
| Poll vs. Interrupt (temp) | Simple, predictable | Slightly slower response | Thermal inertia makes polling adequate |
| State machine vs. PID | Clear logic, safe | Less optimal control | Safety and predictability prioritized |
| Hardcoded vs. Dynamic config | Fast, type-safe | Requires recompile | Performance and reliability over flexibility |
| Single controller class | Centralized logic | Large class | Complexity manageable, cohesion high |

### 10.3 Alternative Designs Considered

**Alternative 1: PID-Based Control**
- Could use PID loops for temperature control
- Rejected: Overkill for binary actuators, complex tuning
- Future: Could add for more sophisticated systems

**Alternative 2: RTOS-Based**
- Could use FreeRTOS with tasks for each component
- Rejected: Unnecessary complexity for current requirements
- Future: Consider if real-time guarantees needed

**Alternative 3: Event-Driven Architecture**
- Could use callback/observer pattern
- Rejected: State machine clearer for this application
- Trade-off: Less flexible but more understandable

**Alternative 4: External Configuration**
- Could use JSON/EEPROM for runtime config
- Rejected: Added complexity, potential for corruption
- Future: Add for user-facing products

### 10.4 Design Validation

**Validation Methods**:
1. ✓ State diagram review (complete coverage)
2. ✓ Safety analysis (FMEA-style thinking)
3. ✓ Code walkthrough (logic verification)
4. ⚠ Hardware testing (to be done by end user)
5. ⚠ Load testing (to be done in actual use)

**Design Confidence**: High for software, medium for hardware integration (depends on specific heater model)

---

## Conclusion

This design provides a solid foundation for a safe, reliable hydronic heater controller. The modular architecture allows for easy customization to specific heater models, while the safety-first approach ensures robust operation in this critical application.

**Next Steps**:
1. Review and approve design document
2. Validate with specific heater model requirements
3. Create test plan for hardware integration
4. Consider adding design diagrams (UML, timing diagrams)
5. User testing and feedback collection

---

**Document History**:
- v1.0 (2025-12-28): Initial design document created
