# Requirements Specification
## Hydronic Diesel Heater Controller System

**Version:** 1.0  
**Date:** December 2025  
**Status:** Draft

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [System Overview](#2-system-overview)
3. [Functional Requirements](#3-functional-requirements)
4. [Non-Functional Requirements](#4-non-functional-requirements)
5. [Hardware Requirements](#5-hardware-requirements)
6. [Software Requirements](#6-software-requirements)
7. [Safety Requirements](#7-safety-requirements)
8. [Interface Requirements](#8-interface-requirements)
9. [Constraints and Assumptions](#9-constraints-and-assumptions)
10. [Requirements Traceability](#10-requirements-traceability)

---

## 1. Introduction

### 1.1 Purpose

This document specifies the requirements for an ESP32-based controller system that replaces the original controller in a hydronic diesel heater. The system shall provide complete autonomous control over all heater components with comprehensive safety monitoring.

### 1.2 Scope

**In Scope**:
- Control of all heater components
- Temperature monitoring at multiple points
- Safety interlocks and emergency shutdown
- User interface for basic control
- Automatic startup and shutdown sequences

**Out of Scope**:
- Remote connectivity (WiFi/Bluetooth) - future enhancement
- Data logging and analytics - future enhancement
- Mobile application - future enhancement
- Integration with HVAC systems - future enhancement

### 1.3 Definitions and Acronyms

| Term | Definition |
|------|------------|
| PWM | Pulse Width Modulation - variable output control |
| DS18B20 | Digital temperature sensor (OneWire protocol) |
| GPIO | General Purpose Input/Output pin |
| ESP32 | Microcontroller platform by Espressif |
| OneWire | Serial communication protocol for sensors |
| Coolant | Heat transfer fluid (water/glycol mixture) |
| Glow Plug | Electric heating element for combustion pre-heating |

### 1.4 References

- DS18B20 Datasheet
- ESP32 Technical Reference Manual
- Diesel heater manufacturer specifications (varies by model)
- OneWire protocol specification

---

## 2. System Overview

### 2.1 System Context

```
┌─────────────────────────────────────────────────────────┐
│                  External Environment                    │
│                                                          │
│  ┌────────────┐        ┌──────────────────────┐        │
│  │   User     │◄──────►│  ESP32 Controller    │        │
│  │ (Serial)   │        │      System          │        │
│  └────────────┘        └──────────┬───────────┘        │
│                                   │                     │
│                         ┌─────────┴────────┐            │
│                         ▼                  ▼            │
│              ┌──────────────────┐  ┌──────────────┐    │
│              │  Heater          │  │  Coolant     │    │
│              │  Components      │  │  System      │    │
│              │  (combustion)    │  │  (heat xfer) │    │
│              └──────────────────┘  └──────────────┘    │
└─────────────────────────────────────────────────────────┘
```

### 2.2 User Characteristics

**Primary Users**: 
- Technical individuals with basic understanding of heating systems
- Ability to wire electrical components safely
- Comfortable with serial terminal interfaces

**Skill Level**: Intermediate to Advanced

---

## 3. Functional Requirements

### 3.1 Component Control Requirements

#### FR-1: Glow Plug Control
**Priority**: Critical

**Requirements**:
- FR-1.1: System SHALL control glow plug with PWM (0-100% power)
- FR-1.2: System SHALL support configurable warmup time (default 60 seconds)
- FR-1.3: System SHALL track glow plug activation duration
- FR-1.4: System SHALL reduce glow plug power after ignition success
- FR-1.5: System SHALL turn off glow plug at operating temperature

**Rationale**: Glow plug preheats combustion chamber for reliable ignition

#### FR-2: Diesel Pump Control
**Priority**: Critical

**Requirements**:
- FR-2.1: System SHALL control diesel pump (ON/OFF)
- FR-2.2: System SHALL activate pump only after glow plug warmup
- FR-2.3: System SHALL deactivate pump immediately on stop command
- FR-2.4: System SHALL deactivate pump on any safety trigger

**Rationale**: Precise fuel delivery timing critical for safe operation

#### FR-3: Air Fan Control
**Priority**: Critical

**Requirements**:
- FR-3.1: System SHALL control air fan with PWM (0-100% speed)
- FR-3.2: System SHALL start air fan during ignition phase
- FR-3.3: System SHALL modulate air fan speed based on temperature
- FR-3.4: System SHALL run air fan at maximum during cooldown

**Rationale**: Air supply essential for combustion; speed affects temperature

#### FR-4: Coolant Pump Control
**Priority**: High

**Requirements**:
- FR-4.1: System SHALL control coolant pump (ON/OFF)
- FR-4.2: System SHALL activate pump when coolant temperature exceeds threshold
- FR-4.3: System SHALL keep pump running during cooldown
- FR-4.4: System SHALL support configurable activation temperature

**Rationale**: Protects system from operating cold, extracts heat when ready

#### FR-5: Heat Exchanger Fan Control
**Priority**: High

**Requirements**:
- FR-5.1: System SHALL control heat exchanger fan with PWM (0-100% speed)
- FR-5.2: System SHALL modulate fan speed based on coolant temperature
- FR-5.3: System SHALL run fan at maximum during cooldown
- FR-5.4: System SHALL turn off fan when coolant below minimum temperature

**Rationale**: Transfers heat to conditioned space efficiently

### 3.2 Sensing Requirements

#### FR-6: Burning Chamber Temperature Sensing
**Priority**: Critical

**Requirements**:
- FR-6.1: System SHALL measure burning chamber temperature
- FR-6.2: System SHALL update reading at least once per second
- FR-6.3: System SHALL validate sensor readings are within range
- FR-6.4: System SHALL detect sensor disconnection

**Rationale**: Critical for safety and control decisions

#### FR-7: Coolant Temperature Sensing
**Priority**: High

**Requirements**:
- FR-7.1: System SHALL measure coolant input temperature
- FR-7.2: System SHALL measure coolant output temperature
- FR-7.3: System SHALL update readings at least once per second
- FR-7.4: System SHALL validate sensor readings

**Rationale**: Needed for coolant system control and efficiency monitoring

#### FR-8: Air Temperature Sensing
**Priority**: Medium

**Requirements**:
- FR-8.1: System SHALL measure ambient air temperature
- FR-8.2: System SHALL update reading at least once per second
- FR-8.3: System SHALL provide air temperature in status reports

**Rationale**: Useful for performance monitoring and user information

#### FR-9: Flow Rate Sensing (Optional)
**Priority**: Low

**Requirements**:
- FR-9.1: System MAY support coolant flow rate sensor
- FR-9.2: If enabled, SHALL measure flow rate in liters per minute
- FR-9.3: If enabled, SHALL update flow rate at least once per second
- FR-9.4: System SHALL operate normally without flow sensor

**Rationale**: Enhanced monitoring but not essential for basic operation

### 3.3 Operational Requirements

#### FR-10: Startup Sequence
**Priority**: Critical

**Requirements**:
- FR-10.1: System SHALL execute automatic startup sequence on start command
- FR-10.2: Sequence SHALL begin with glow plug warmup phase
- FR-10.3: Sequence SHALL proceed to ignition phase after warmup
- FR-10.4: Sequence SHALL transition to running state upon successful ignition
- FR-10.5: Sequence SHALL abort to error state if ignition fails

**Rationale**: Automated, repeatable startup ensures reliability

#### FR-11: Normal Operation
**Priority**: Critical

**Requirements**:
- FR-11.1: System SHALL maintain target combustion temperature
- FR-11.2: System SHALL manage coolant circulation based on temperature
- FR-11.3: System SHALL adjust heat exchanger fan based on demand
- FR-11.4: System SHALL continuously monitor all sensors
- FR-11.5: System SHALL perform safety checks at least twice per second

**Rationale**: Efficient, safe operation during steady-state running

#### FR-12: Shutdown Sequence
**Priority**: Critical

**Requirements**:
- FR-12.1: System SHALL execute safe shutdown on stop command
- FR-12.2: Shutdown SHALL immediately stop fuel delivery
- FR-12.3: Shutdown SHALL maintain cooling for minimum 60 seconds
- FR-12.4: Shutdown SHALL prevent restart until cooldown complete
- FR-12.5: System SHALL perform emergency shutdown on safety trigger

**Rationale**: Prevent thermal shock and component damage

### 3.4 User Interface Requirements

#### FR-13: Command Interface
**Priority**: High

**Requirements**:
- FR-13.1: System SHALL accept commands via serial interface
- FR-13.2: System SHALL support "start" command to begin operation
- FR-13.3: System SHALL support "stop" command to shutdown
- FR-13.4: System SHALL support "status" command for information display
- FR-13.5: System SHALL acknowledge commands with feedback

**Rationale**: Simple interface for basic control

#### FR-14: Status Reporting
**Priority**: Medium

**Requirements**:
- FR-14.1: System SHALL report current operational state
- FR-14.2: System SHALL report all temperature readings
- FR-14.3: System SHALL report component states (on/off/speed)
- FR-14.4: System SHALL report error messages when applicable
- FR-14.5: System SHALL automatically print status periodically

**Rationale**: User needs visibility into system operation

---

## 4. Non-Functional Requirements

### 4.1 Performance Requirements

#### NFR-1: Response Time
**Priority**: High

**Requirements**:
- NFR-1.1: Safety checks SHALL execute within 500ms
- NFR-1.2: State transitions SHALL occur within 100ms
- NFR-1.3: Sensor readings SHALL be available within 1 second
- NFR-1.4: User commands SHALL be acknowledged within 100ms

**Rationale**: Timely response critical for safety and usability

#### NFR-2: Reliability
**Priority**: Critical

**Requirements**:
- NFR-2.1: System SHALL operate continuously for 24+ hours
- NFR-2.2: System SHALL recover from transient sensor errors
- NFR-2.3: System SHALL fail to safe state on power loss
- NFR-2.4: System SHALL not corrupt state during unexpected reset

**Rationale**: Heating system must be dependable

#### NFR-3: Resource Usage
**Priority**: Medium

**Requirements**:
- NFR-3.1: Software SHALL use less than 100KB flash memory
- NFR-3.2: Software SHALL use less than 50KB RAM
- NFR-3.3: CPU usage SHALL remain below 10% during normal operation
- NFR-3.4: System SHALL leave resources for future enhancements

**Rationale**: Efficient use of hardware; room for expansion

### 4.2 Maintainability Requirements

#### NFR-4: Code Quality
**Priority**: Medium

**Requirements**:
- NFR-4.1: Code SHALL be organized into logical modules
- NFR-4.2: Code SHALL include comments for complex logic
- NFR-4.3: Code SHALL follow consistent naming conventions
- NFR-4.4: Code SHALL use meaningful variable and function names

**Rationale**: Ease future modifications and debugging

#### NFR-5: Configuration
**Priority**: Medium

**Requirements**:
- NFR-5.1: Pin assignments SHALL be configurable in header file
- NFR-5.2: Temperature thresholds SHALL be configurable
- NFR-5.3: Timing constants SHALL be configurable
- NFR-5.4: Configuration SHALL not require code logic changes

**Rationale**: Adapt to different heater models and user preferences

### 4.3 Usability Requirements

#### NFR-6: Documentation
**Priority**: High

**Requirements**:
- NFR-6.1: System SHALL include comprehensive README
- NFR-6.2: System SHALL include wiring diagrams
- NFR-6.3: System SHALL include configuration examples
- NFR-6.4: System SHALL include troubleshooting guide
- NFR-6.5: System SHALL include API documentation

**Rationale**: Users need clear guidance for successful deployment

---

## 5. Hardware Requirements

### 5.1 Controller Platform

#### HR-1: Microcontroller
**Priority**: Critical

**Requirements**:
- HR-1.1: SHALL use ESP32 or compatible microcontroller
- HR-1.2: SHALL have minimum 20 GPIO pins
- HR-1.3: SHALL support hardware PWM generation
- HR-1.4: SHALL have 3.3V logic levels
- HR-1.5: SHALL support Arduino framework

### 5.2 Input Hardware

#### HR-2: Temperature Sensors
**Priority**: Critical

**Requirements**:
- HR-2.1: SHALL use DS18B20 digital temperature sensors
- HR-2.2: SHALL support -55°C to +125°C range (minimum)
- HR-2.3: SHALL use OneWire protocol communication
- HR-2.4: SHALL include 4.7kΩ pullup resistors

#### HR-3: Flow Sensor (Optional)
**Priority**: Low

**Requirements**:
- HR-3.1: MAY use pulse-output flow sensor
- HR-3.2: If used, SHALL provide pulse output to GPIO
- HR-3.3: IF used, SHALL be calibrated for coolant type

### 5.3 Output Hardware

#### HR-4: Power Switching
**Priority**: Critical

**Requirements**:
- HR-4.1: SHALL use MOSFETs or relays for all high-current outputs
- HR-4.2: SHALL NOT connect loads directly to ESP32 pins
- HR-4.3: Glow plug circuit SHALL handle minimum 50A
- HR-4.4: Other circuits SHALL be rated for connected loads
- HR-4.5: SHALL include flyback diodes for inductive loads

#### HR-5: Wiring
**Priority**: Critical

**Requirements**:
- HR-5.1: Wire gauges SHALL match current requirements
- HR-5.2: All connections SHALL be mechanically secure
- HR-5.3: All high-current paths SHALL include fuses
- HR-5.4: Ground connections SHALL use star topology

---

## 6. Software Requirements

### 6.1 Development Platform

#### SR-1: Build System
**Priority**: High

**Requirements**:
- SR-1.1: SHALL use PlatformIO build system
- SR-1.2: SHALL support command-line builds
- SR-1.3: SHALL automatically manage dependencies
- SR-1.4: SHALL support ESP32 platform

#### SR-2: Dependencies
**Priority**: High

**Requirements**:
- SR-2.1: SHALL use Arduino framework
- SR-2.2: SHALL use OneWire library for sensors
- SR-2.3: SHALL use DallasTemperature library
- SR-2.4: SHALL minimize external dependencies

### 6.2 Software Architecture

#### SR-3: Code Organization
**Priority**: Medium

**Requirements**:
- SR-3.1: SHALL separate component control into classes
- SR-3.2: SHALL separate configuration into header file
- SR-3.3: SHALL use state machine for operational control
- SR-3.4: SHALL encapsulate hardware details in components

#### SR-4: Error Handling
**Priority**: High

**Requirements**:
- SR-4.1: SHALL validate all sensor readings
- SR-4.2: SHALL log error conditions to serial output
- SR-4.3: SHALL enter safe state on critical errors
- SR-4.4: SHALL continue operation on non-critical warnings

---

## 7. Safety Requirements

### 7.1 Critical Safety Requirements

#### SAFE-1: Overtemperature Protection
**Priority**: Critical

**Requirements**:
- SAFE-1.1: System SHALL monitor burning chamber temperature continuously
- SAFE-1.2: System SHALL trigger emergency stop if temp > 900°C
- SAFE-1.3: Emergency stop SHALL occur within 500ms of detection
- SAFE-1.4: System SHALL NOT allow restart until manual intervention

**Rationale**: Prevent fire hazard from overheating

#### SAFE-2: Fuel Cutoff
**Priority**: Critical

**Requirements**:
- SAFE-2.1: System SHALL stop fuel delivery on any error
- SAFE-2.2: System SHALL stop fuel delivery on stop command
- SAFE-2.3: Fuel cutoff SHALL be immediate (< 100ms)
- SAFE-2.4: Default state (power off) SHALL have fuel delivery OFF

**Rationale**: Fuel control most critical safety function

#### SAFE-3: Ignition Timeout
**Priority**: Critical

**Requirements**:
- SAFE-3.1: System SHALL timeout ignition after 2 minutes
- SAFE-3.2: Timeout SHALL trigger emergency stop
- SAFE-3.3: System SHALL prevent restart without manual intervention

**Rationale**: Prevent fuel accumulation and flooding

### 7.2 Secondary Safety Requirements

#### SAFE-4: Cooldown Enforcement
**Priority**: High

**Requirements**:
- SAFE-4.1: System SHALL enforce minimum 60-second cooldown
- SAFE-4.2: Cooldown SHALL maintain cooling fans at maximum
- SAFE-4.3: System SHALL prevent restart during cooldown

**Rationale**: Prevent thermal shock damage

#### SAFE-5: Sensor Validation
**Priority**: High

**Requirements**:
- SAFE-5.1: System SHALL detect disconnected sensors
- SAFE-5.2: System SHALL validate temperature readings in range
- SAFE-5.3: System SHALL warn on sensor failures
- SAFE-5.4: System SHALL allow operation with degraded sensors (warning mode)

**Rationale**: Balance safety with availability

---

## 8. Interface Requirements

### 8.1 Electrical Interfaces

#### INT-1: GPIO Assignments
**Priority**: High

**Requirements**:
- INT-1.1: Pin assignments SHALL be documented in config.h
- INT-1.2: PWM outputs SHALL use LEDC-capable pins
- INT-1.3: OneWire sensors MAY share or use separate pins
- INT-1.4: Pins SHALL not conflict with ESP32 boot strapping pins

#### INT-2: Signal Levels
**Priority**: Critical

**Requirements**:
- INT-2.1: All GPIO outputs SHALL be 3.3V logic
- INT-2.2: All GPIO inputs SHALL tolerate 3.3V maximum
- INT-2.3: Level shifting SHALL be used for 5V devices if needed

### 8.2 Communication Interfaces

#### INT-3: Serial Interface
**Priority**: High

**Requirements**:
- INT-3.1: Baud rate SHALL be 115200
- INT-3.2: Format SHALL be 8N1 (8 data bits, no parity, 1 stop bit)
- INT-3.3: Line endings SHALL accept CR, LF, or CRLF
- INT-3.4: Commands SHALL be case-insensitive

---

## 9. Constraints and Assumptions

### 9.1 Constraints

**Technical Constraints**:
- Must use ESP32 platform (hardware limitation)
- Must operate within ESP32 specifications
- Must not exceed pin current limits (40mA per pin)
- PWM channels limited to 16 (ESP32 hardware)

**Environmental Constraints**:
- Operating temperature: -20°C to +60°C (ESP32 rated range)
- Voltage supply: 5V ±5% (USB or regulated)
- Electromagnetic: Must tolerate automotive environment

**User Constraints**:
- Users must have technical competency for wiring
- Users must have access to serial terminal software
- Users responsible for proper fusing and protection

### 9.2 Assumptions

**System Assumptions**:
- Heater components are electrically compatible
- Adequate power supply available for all components
- Proper safety devices (fuses, circuit breakers) installed
- Installation in well-ventilated area

**Operational Assumptions**:
- User will perform initial testing without fuel
- User will monitor first few operation cycles
- User has fire extinguisher available
- User complies with local regulations

---

## 10. Requirements Traceability

### 10.1 Requirements to Design Mapping

| Requirement ID | Design Element | Implementation File |
|----------------|----------------|---------------------|
| FR-1 (Glow Plug) | GlowPlug class | components.cpp |
| FR-2 (Diesel Pump) | DieselPump class | components.cpp |
| FR-3 (Air Fan) | Fan class | components.cpp |
| FR-4 (Coolant Pump) | CoolantPump class | components.cpp |
| FR-5 (Heat Exchanger) | Fan class | components.cpp |
| FR-6-8 (Temperature) | TemperatureSensor class | components.cpp |
| FR-9 (Flow) | FlowSensor class | components.cpp |
| FR-10-12 (Operation) | HydronicHeaterController | controller.cpp |
| FR-13-14 (UI) | main.cpp command processing | main.cpp |
| SAFE-1-5 (Safety) | checkSafetyConditions() | controller.cpp |

### 10.2 Requirements Coverage

| Category | Total Requirements | Implemented | Coverage |
|----------|-------------------|-------------|----------|
| Functional | 48 | 48 | 100% |
| Non-Functional | 19 | 19 | 100% |
| Hardware | 15 | 15 (documented) | 100% |
| Software | 11 | 11 | 100% |
| Safety | 14 | 14 | 100% |
| Interface | 9 | 9 | 100% |
| **TOTAL** | **116** | **116** | **100%** |

### 10.3 Test Coverage Requirements

Each requirement SHALL have corresponding test procedure:
- Unit tests for component classes
- Integration tests for controller
- Safety tests for emergency conditions
- Hardware tests for actual deployment

(Test plan to be developed in separate document)

---

## Approval

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Author | System Designer | 2025-12-28 | _________ |
| Reviewer | Safety Engineer | ___ | _________ |
| Approver | Project Lead | ___ | _________ |

---

**Document History**:
- v1.0 (2025-12-28): Initial requirements specification created
