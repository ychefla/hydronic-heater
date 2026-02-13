# Autoterm Flow 5D Integration Guide

**Heater Model**: Autoterm Flow 5D  
**Type**: 5kW Hydronic Diesel Heater  
**Application**: Camper van floor/air/water heating  
**Manufacturer**: Autoterm (Estonia, formerly Planar/Binar)  
**Controller Integration**: Hybrid — ESP32 smart layer on top of Autoterm controller

---

## 1. Why Autoterm Flow 5D

### Background

This project originally targeted cheap Chinese hydronic diesel heaters (HCalory W51 YWH-A202). That approach was abandoned because:

- ❌ **HCalory refused to provide UART protocol documentation** for ESP32 integration
- ❌ Reverse-engineering the proprietary protocol is time-consuming and unreliable
- ❌ Emulating the remote controller provides no additional functionality
- ❌ Full hardware replacement (direct glow plug/fuel pump control) is complex and carries combustion safety risks

The **Autoterm Flow 5D** was selected as the replacement because it is the best fit for DIY integration of a hydronic diesel heater in a campervan application.

### Why Autoterm Flow 5D Specifically

| Criterion | Autoterm Flow 5D | HCalory W51 | Webasto/Eberspächer |
|-----------|-----------------|-------------|---------------------|
| Protocol documentation | ✅ Available to integrators | ❌ Proprietary, refused | ❌ Locked ecosystem |
| DIY community support | ✅ Strong (van/marine) | ⚠️ Limited | ❌ Minimal |
| Granular power control via protocol | ✅ Yes | ❓ Unknown | ❌ No |
| Telemetry (temp, RPM, fuel, errors) | ✅ Full access | ❓ Unknown | ❌ Limited |
| Build quality & certifications | ✅ E-marked, CE | ⚠️ Variable | ✅ Premium |
| Price | ~€500–700 | ~€150–300 | ~€1000+ |
| Hydronic (liquid) | ✅ 5kW | ✅ 5kW | ✅ Various |
| 12V operation | ✅ Yes | ✅ Yes | ✅ Yes |

### Manufacturer

**Autoterm** (formerly Planar/Binar) is an Estonian company that manufactures vehicle heaters. They are known for:
- Proper engineering and quality control
- E-mark and CE certification
- Willingness to share technical documentation with integrators
- Active presence in the van conversion and marine communities

---

## 2. Heater Specifications

### Autoterm Flow 5D Specs

- **Model**: Autoterm Flow 5D (liquid/hydronic)
- **Heat output**: 1.4 – 5.0 kW (continuously variable)
- **Voltage**: 12V DC (also available in 24V)
- **Fuel consumption**: 0.18 – 0.62 L/hr diesel
- **Coolant flow**: Built-in circulation pump
- **Weight**: ~3.2 kg
- **Dimensions**: Compact, suitable for under-floor or compartment mounting
- **Operating temperature range**: -40°C to +40°C ambient
- **Noise level**: Low, especially at reduced power
- **Certifications**: E-mark (ECE R122), CE

### Control Interface

- **Standard control panel**: Autoterm Comfort Control / PU-27 (wired)
- **Optional**: Autoterm app via Bluetooth (Autoterm Control BT module)
- **Integration interface**: UART protocol for external controller integration

### Internal Components (managed by Autoterm controller)

The Autoterm controller handles all combustion-critical functions:
- Glow plug ignition and management
- Fuel metering pump (variable output)
- Combustion air fan (variable speed)
- Flame monitoring and safety shutdown
- Overheat protection
- Error diagnostics

---

## 3. Integration Architecture: Hybrid Approach

### Philosophy

The key insight is to **split responsibilities**:
- **Autoterm Flow 5D controller** → handles combustion, fuel metering, ignition, flame safety
- **ESP32 controller** → handles smart features, multi-zone control, scheduling, MQTT/Paku-IoT

This is **safer and more maintainable** than full hardware replacement because:
1. Combustion safety is handled by a certified, tested controller
2. No need to calibrate glow plug timing, fuel-air ratios, or flame detection
3. Reduces the ESP32 firmware to the "smart layer" — the part we actually want to build
4. Autoterm handles error recovery and safe shutdown for combustion faults

### System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                     Camper Van (Edge)                           │
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │          ESP32 Smart Controller (Paku Edge)                │ │
│  │                                                            │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────────┐  │ │
│  │  │ Zone Control │  │  Scheduling  │  │ MQTT / Paku-IoT│  │ │
│  │  │ (floor, air, │  │  (timed,     │  │ (telemetry,    │  │ │
│  │  │  water)      │  │   profiles)  │  │  commands)     │  │ │
│  │  └──────┬───────┘  └──────────────┘  └────────────────┘  │ │
│  │         │                                                  │ │
│  │  ┌──────▼───────────────────────────────────────────────┐ │ │
│  │  │  PID Temperature Controller                          │ │ │
│  │  │  - Reads zone temperatures (DS18B20 sensors)         │ │ │
│  │  │  - Calculates heat demand                            │ │ │
│  │  │  - Sends power setpoint to Autoterm via UART         │ │ │
│  │  └──────┬───────────────────────────────────────────────┘ │ │
│  │         │ UART (commands + telemetry)                      │ │
│  └─────────┼──────────────────────────────────────────────────┘ │
│            │                                                     │
│  ┌─────────▼──────────────────────────────────────────────────┐ │
│  │       Autoterm Flow 5D (Heater Unit)                       │ │
│  │                                                            │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │  Autoterm Controller (certified, handles safety)     │ │ │
│  │  │  - Glow plug ignition & management                   │ │ │
│  │  │  - Fuel metering (variable 0.18–0.62 L/hr)          │ │ │
│  │  │  - Combustion air fan                                │ │ │
│  │  │  - Flame monitoring                                  │ │ │
│  │  │  - Overheat protection                               │ │ │
│  │  │  - Error diagnostics                                 │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  Additional Hardware (managed by ESP32)                  │   │
│  │  - DS18B20 sensors: cabin, floor, water tank, outside    │   │
│  │  - Zone valves: floor loop, water coil                   │   │
│  │  - Heat exchanger fan (cabin air)                        │   │
│  │  - Coolant distribution pump (if separate from heater)   │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### What Changes from the Original Design

| Aspect | Original (HCalory full replacement) | New (Autoterm hybrid) |
|--------|--------------------------------------|----------------------|
| Glow plug control | ESP32 direct PWM | Autoterm controller |
| Fuel pump control | ESP32 direct PWM | Autoterm controller (via UART setpoint) |
| Combustion air fan | ESP32 direct PWM | Autoterm controller |
| Combustion safety | ESP32 firmware | Autoterm controller (certified) |
| Zone valve control | ESP32 GPIO | ESP32 GPIO *(unchanged)* |
| Zone temperature sensors | ESP32 DS18B20 | ESP32 DS18B20 *(unchanged)* |
| Heat exchanger fan | ESP32 PWM | ESP32 PWM *(unchanged)* |
| PID temperature control | ESP32 | ESP32 *(unchanged)* |
| MQTT / Paku-IoT | ESP32 | ESP32 *(unchanged)* |
| Scheduling | ESP32 | ESP32 *(unchanged)* |
| Power setpoint | Direct PWM | UART command to Autoterm |
| Telemetry source | DS18B20 only | DS18B20 + Autoterm UART data |

### What We Keep from the Original Design

Most of the smart-layer design work carries over:
- ✅ Multi-zone heating control (floor, air, water)
- ✅ PID temperature feedback
- ✅ Power profiles (Eco, Normal, Boost)
- ✅ Scheduled heating
- ✅ MQTT/Paku-IoT integration
- ✅ DS18B20 sensor network for zone temperatures
- ✅ Zone valve/pump GPIO control
- ✅ Heat exchanger fan PWM control

### What We Remove

- ❌ GlowPlug class (direct PWM control)
- ❌ DieselPump class (direct PWM control)
- ❌ Combustion air fan class (direct control)
- ❌ Combustion state machine (GLOW_PLUG_WARMUP, IGNITION states)
- ❌ Combustion safety code (SAFE-1A, SAFE-1B, SAFE-3 — chamber overheat, spike detection, fuel depletion)

### What We Add

- ✅ **AutotermUART class** — serial communication with Autoterm controller
- ✅ **Extended telemetry** — combustion data from Autoterm (fan RPM, fuel rate, error codes)
- ✅ **Power setpoint interface** — send desired power level to Autoterm via UART
- ✅ **Heater status monitoring** — read Autoterm state (off, starting, running, error)

---

## 4. UART Communication Protocol

### Overview

The Autoterm Flow 5D communicates via UART at the control panel connector. The protocol allows an external controller to:
- Start and stop the heater
- Set power level / target temperature
- Read operating status, temperatures, fan RPM, fuel consumption
- Read error codes and diagnostics

### Physical Connection

```
ESP32                    Autoterm Flow 5D
─────                    ────────────────
GPIO 16 (RX2) ◄──────── TX (Autoterm transmits)
GPIO 17 (TX2) ────────►  RX (Autoterm receives)
GND ──────────────────── GND

Voltage levels: 3.3V or 5V (verify for your specific unit)
Baud rate: Typically 2400 baud, 8N1 (verify with Autoterm documentation)
```

### Protocol Details

> **Note**: Request the official Autoterm integration documentation from the manufacturer.
> Autoterm is known to provide this to integrators upon request.
> Contact: support@autoterm.com or through your dealer.

The protocol is a binary frame-based protocol. Community-documented structure:

```
Frame format (typical):
┌──────┬────────┬─────────┬──────────┬──────────┐
│ SYNC │ LENGTH │ COMMAND │   DATA   │ CHECKSUM │
│ 0xAA │  1 byte│  1 byte │ N bytes  │  1 byte  │
└──────┴────────┴─────────┴──────────┴──────────┘
```

### Key Commands (to be verified with official documentation)

| Command | Description | Direction |
|---------|-------------|-----------|
| Start heater | Begin startup sequence | ESP32 → Autoterm |
| Stop heater | Begin shutdown sequence | ESP32 → Autoterm |
| Set power level | Set heat output (%) | ESP32 → Autoterm |
| Set target temp | Set thermostat target | ESP32 → Autoterm |
| Request status | Poll current state | ESP32 → Autoterm |
| Status response | Current state + telemetry | Autoterm → ESP32 |

### Telemetry Available via UART

| Parameter | Description | Use in our system |
|-----------|-------------|-------------------|
| Heater state | Off / Starting / Running / Stopping / Error | Display, Paku-IoT telemetry |
| Combustion temp | Internal temperature | Monitoring, safety |
| Fan RPM | Combustion fan speed | Diagnostics |
| Fuel rate | Current fuel consumption | Efficiency tracking, Paku-IoT |
| Power level | Current output percentage | PID feedback |
| Error code | Diagnostic code if in error | Alerting, troubleshooting |
| Supply voltage | Battery voltage | Low-voltage protection |

---

## 5. ESP32 Software Architecture (Hybrid)

### Updated State Machine

With the hybrid approach, the ESP32 state machine simplifies significantly:

```
                    ┌───────┐
                    │  OFF  │◄──────────────────────┐
                    └───┬───┘                       │
                        │ start()                   │
                        ▼                           │
              ┌─────────────────────┐              │
              │    STARTING         │              │
              │  - Send start cmd   │──timeout────►│
              │    to Autoterm      │              │
              │  - Wait for         │      ┌─────┐│
              │    "running" status │      │ERROR││
              └──────────┬──────────┘      └──┬──┘│
                         │ Autoterm reports     │   │
                         │ "running"            │   │
                         ▼                      │   │
              ┌─────────────────────┐          │   │
              │     RUNNING          │          │   │
              │  - PID zone control  │◄─error───┘   │
              │  - Power setpoint    │              │
              │    via UART          │              │
              │  - Zone valve mgmt   │              │
              │  - Telemetry publish │              │
              └──────────┬──────────┘              │
                         │ stop()                   │
                         ▼                          │
              ┌─────────────────────┐              │
              │    STOPPING          │              │
              │  - Send stop cmd     │              │
              │  - Wait for          │              │
              │    "off" status      │              │
              └──────────┬──────────┘              │
                         │ Autoterm reports          │
                         │ "off"                     │
                         └──────────────────────────┘
```

**Key simplification**: No GLOW_PLUG_WARMUP or IGNITION states — Autoterm handles the entire combustion startup sequence internally.

### Updated Software Layers

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                         │
│  - MQTT command handling                                    │
│  - Schedule management                                      │
│  - Status reporting / Paku-IoT telemetry                   │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼────────────────────────────────────┐
│                   Controller Layer                           │
│  - Simplified state machine (OFF → STARTING → RUNNING →    │
│    STOPPING → OFF)                                          │
│  - PID temperature control per zone                        │
│  - Zone valve management                                    │
│  - Power profile selection (Eco / Normal / Boost)          │
└───────────┬────────────────────────────┬────────────────────┘
            │                            │
┌───────────▼──────────┐  ┌─────────────▼────────────────────┐
│  Autoterm UART Layer │  │    Zone Hardware Layer            │
│  - Send commands     │  │  - DS18B20 temperature sensors   │
│  - Read telemetry    │  │  - Zone valves (GPIO)            │
│  - Parse status      │  │  - Heat exchanger fan (PWM)      │
│  - Error handling    │  │  - Coolant distribution pump     │
└──────────────────────┘  └──────────────────────────────────┘
```

### Updated Pin Assignments

```cpp
// === Autoterm UART Communication ===
#define AUTOTERM_RX_PIN  16    // ESP32 RX ← Autoterm TX
#define AUTOTERM_TX_PIN  17    // ESP32 TX → Autoterm RX
#define AUTOTERM_BAUD    2400  // Verify with Autoterm documentation

// === Zone Temperature Sensors (DS18B20, OneWire bus) ===
#define ONEWIRE_BUS_PIN  4     // Single bus, sensors identified by address
// Sensors: cabin, floor supply, floor return, water tank, outside (optional)

// === Zone Control Outputs ===
#define HEAT_EXCHANGER_FAN_PIN  12  // PWM — cabin air heating
#define FLOOR_VALVE_PIN         18  // GPIO — floor heating loop valve
#define WATER_VALVE_PIN         19  // GPIO — water heating coil valve

// === Optional ===
#define FLOW_SENSOR_PIN  13         // Coolant flow monitoring
```

**Removed pins** (no longer needed):
- ~~GPIO 25 — Glow plug PWM~~ (Autoterm handles)
- ~~GPIO 26 — Diesel pump PWM~~ (Autoterm handles)
- ~~GPIO 27 — Combustion air fan PWM~~ (Autoterm handles)
- ~~GPIO 14 — Coolant pump~~ (Autoterm has built-in pump; may still be needed if using a separate distribution pump)

---

## 6. Safety Architecture (Hybrid)

### Safety Split

| Safety Domain | Responsible | Implementation |
|---------------|-------------|----------------|
| Combustion chamber overheat | **Autoterm controller** | Internal sensors + certified logic |
| Flame monitoring | **Autoterm controller** | Internal |
| Ignition failure / timeout | **Autoterm controller** | Internal |
| Fuel metering safety | **Autoterm controller** | Internal |
| Glow plug management | **Autoterm controller** | Internal |
| Supply voltage protection | **Autoterm controller** | Internal |
| **Zone overheat (floor, water)** | **ESP32** | DS18B20 sensors + GPIO valve shutoff |
| **Coolant distribution** | **ESP32** | Flow sensor + pump monitoring |
| **Communication watchdog** | **ESP32** | UART timeout → safe state |
| **WiFi/MQTT loss handling** | **ESP32** | Continue on last settings, revert to safe defaults |

### ESP32 Safety Requirements (Retained)

These safety features from the original design still apply:

| ID | Requirement | Trigger | Action |
|----|-------------|---------|--------|
| SAFE-Z1 | Floor zone overheat | Floor temp > 50°C | Close floor valve |
| SAFE-Z2 | Water zone overheat | Water temp > 85°C | Close water valve |
| SAFE-Z3 | Coolant overheat | Coolant > 95°C | Send stop to Autoterm, open all valves for dissipation |
| SAFE-C1 | UART communication loss | No response for 30s | Send stop, enter error state |
| SAFE-C2 | ESP32 watchdog | System hang | Hardware reset, safe defaults |
| SAFE-C3 | WiFi/MQTT loss | Extended disconnection | Continue on last settings, revert after timeout |

### Combustion Safety (Delegated to Autoterm)

These are **no longer implemented in ESP32 firmware** — Autoterm handles them:

- ~~SAFE-1A: Burning chamber overheat~~
- ~~SAFE-1B: Rapid temperature spike~~
- ~~SAFE-1C: Low chamber temp / incomplete combustion~~
- ~~SAFE-3: Fuel depletion detection~~

The ESP32 monitors Autoterm error codes via UART and reports them to Paku-IoT.

---

## 7. Hardware Requirements

### Heater Unit
- **Autoterm Flow 5D** (12V variant for campervan)
- Mounting hardware (under-floor or compartment)
- Fuel line from diesel tank
- Exhaust pipe with silencer
- Coolant hoses (supply + return)

### ESP32 Controller
- **ESP32 development board** (or LilyGo T-Display S3 for built-in display)
- UART connection cable to Autoterm control panel connector
- Power supply from 12V vehicle system (with regulator)

### Zone Distribution
- **3-way motorized valve** or **individual zone valves** (floor, water)
- **Heat exchanger** with fan for cabin air heating
- **DS18B20 sensors** (5–6 units): cabin, floor supply, floor return, water tank, (outside)
- **Flow sensor** (optional) for coolant monitoring

### Estimated Cost

| Item | Cost (EUR) |
|------|------------|
| Autoterm Flow 5D (12V) | €550–700 |
| ESP32 board | €10 |
| DS18B20 sensors (6x) | €15 |
| Zone valves (2x) | €60 |
| Heat exchanger + fan | €50 |
| Wiring, connectors, fuses | €30 |
| Installation materials | €50 |
| **Total** | **~€770–920** |

### Cost Comparison

| Option | Total Cost | DIY Control | Safety | Effort |
|--------|-----------|-------------|--------|--------|
| HCalory W51 + full replacement | €350–550 | ✅ Full | ⚠️ Self-implemented | 🔴 Very high |
| **Autoterm Flow 5D + hybrid** | **€770–920** | **✅ Full smart layer** | **✅ Certified combustion** | **🟢 Moderate** |
| Webasto Thermo Top Evo + locked | €1200–1500 | ❌ None | ✅ Certified | 🟢 Low (but no DIY) |

---

## 8. Getting Started

### Phase 0: Procurement & Documentation
1. Purchase Autoterm Flow 5D (12V variant)
2. Request integration documentation from Autoterm (support@autoterm.com)
3. Review Autoterm installation manual
4. Order ESP32 board, sensors, zone valves

### Phase 1: Heater Installation
1. Install Autoterm Flow 5D per manufacturer instructions
2. Verify heater operates correctly with standard control panel
3. Test all power levels and verify shutdown behavior

### Phase 2: UART Integration
1. Identify UART pins on Autoterm control panel connector
2. Connect ESP32 to Autoterm via UART
3. Implement AutotermUART class (send/receive frames)
4. Test: start, stop, set power, read status
5. Verify telemetry data (temp, RPM, fuel rate, errors)

### Phase 3: Smart Layer
1. Implement PID zone control
2. Wire DS18B20 sensors and zone valves
3. Implement scheduling and power profiles
4. Integrate MQTT / Paku-IoT

### Phase 4: Refinement
1. Tune PID parameters for your van's thermal characteristics
2. Calibrate zone temperature targets
3. Set up Grafana dashboards via Paku-IoT
4. Test all safety scenarios

---

## 9. Community Resources

### Autoterm-Specific
- [Autoterm official website](https://autoterm.com)
- [Autoterm Flow 5D product page](https://autoterm.com/flow-5d)
- Autoterm dealer network for technical support
- Van conversion forums (Autoterm is widely used)

### DIY Integration
- Marine and campervan forums with Autoterm UART integration examples
- Home Assistant community (Autoterm integrations exist)
- Afterburner project (protocol analysis techniques applicable)

### This Project
- [README.md](README.md) — Project overview
- [DESIGN.md](DESIGN.md) — System architecture
- [SAFETY.md](SAFETY.md) — Safety requirements
- [PAKU_INTEGRATION.md](PAKU_INTEGRATION.md) — Cloud platform integration
- [CAMPERVAN_FEATURES.md](CAMPERVAN_FEATURES.md) — Campervan application guide

---

## 10. Migration Notes

### From HCalory W51 Design

If you have been following the original HCalory W51 design documents:

- **HCALORY_W51_GUIDE.md** → Superseded by this document
- **UART_TESTING_GUIDE.md** → No longer needed (Autoterm provides documentation)
- **WIRING.md** → Simplified (no direct component wiring for combustion)
- **SAFETY.md** → Combustion safety delegated to Autoterm; zone safety retained
- **State machine** → Simplified (no GLOW_PLUG_WARMUP, IGNITION states)
- **Component classes** → Remove GlowPlug, DieselPump, combustion Fan; add AutotermUART

### Code Impact

| File | Change |
|------|--------|
| `include/components.h` | Remove GlowPlug, DieselPump, combustion Fan classes. Add AutotermUART class. |
| `src/components.cpp` | Same removals/additions |
| `include/controller.h` | Simplify state machine. Replace direct component control with UART commands. |
| `src/controller.cpp` | Simplify state transitions. PID sends setpoint via UART. |
| `include/config.h` | Update pin assignments. Add UART config. Remove combustion pins. |
| `src/main.cpp` | Update initialization. Remove combustion component setup. |

---

**Status**: Ready for Phase 0 (procurement + documentation request)  
**Timeline**: Autoterm documentation → UART integration → smart layer  
**Risk**: Low — Autoterm is known to support integrators  

**Let's build a proper smart heater controller! 🚐🔥**
