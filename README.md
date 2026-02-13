# Hydronic Diesel Heater Controller

ESP32 smart controller for the **Autoterm Flow 5D** hydronic diesel heater in a campervan.

## Overview

The Autoterm Flow 5D handles combustion (certified controller). The ESP32 adds:

- **UART control**: start/stop, power setpoint, read telemetry
- **MQTT telemetry**: publish heater data to Paku-IoT
- **Safety monitoring**: coolant flow, coolant overheat, UART watchdog

Cabin temperature is measured by RuuviTag (via paku-core BLE).

## Piping Layout

```text
Autoterm Flow 5D (built-in pump + heater)
         │ hot coolant out
         ▼
    Distribution manifold
         ├── Floor heating loop ──── ball valve ──┐
         ├── Air heat exchanger ─── ball valve ──┤
         └── Copper plate (water) ── ball valve ──┤
                                                   ▼
                                            Expansion tank
                                                   │
                                                   ▼
                                          Return to Autoterm
```

- Ball valves are **manual** in v1 (motorized valves are a [future feature](FUTURE_FEATURES.md))
- Water is heated **on-demand** via copper plate heat exchanger (no tank, no storage)
- Fuel from van's diesel tank
- Autoterm's built-in pump circulates coolant through all circuits

## Hardware (v1)

| Component | Purpose | Notes |
| --------- | ------- | ----- |
| Autoterm Flow 5D (12V) | Heater + circulation pump | See [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) |
| ESP32 (LilyGo T-Display S3) | Smart controller | Runs as paku-core add-on (compile-time flag) |
| Flow sensor | Coolant flow safety | GPIO 13 |
| UART cable | ESP32 ↔ Autoterm | GPIO 16 RX, GPIO 17 TX |

### Temperature Sources

| Measurement | Source | v1? |
| ----------- | ------ | --- |
| Cabin air | RuuviTag via paku-core BLE | ✅ |
| Combustion temp | Autoterm UART telemetry | ✅ |
| Fan RPM | Autoterm UART telemetry | ✅ |
| Fuel rate | Autoterm UART telemetry | ✅ |
| Supply voltage | Autoterm UART telemetry | ✅ |
| Coolant supply/return | Autoterm UART (TBD) or DS18B20 | ❌ See [future](FUTURE_FEATURES.md) |
| Floor supply/return | DS18B20 (need suitable probes) | ❌ See [future](FUTURE_FEATURES.md) |

> **Open question**: Does Autoterm provide coolant temperature via UART? If not, at least one DS18B20 on the coolant return is needed for overheat safety. To be verified with Autoterm documentation.

### Heating Circuits

| Circuit | Purpose | v1 Control |
| ------- | ------- | ---------- |
| Floor heating | Underfloor radiant loops | Manual ball valve |
| Air heating | Cabin air via heat exchanger + fan | Manual ball valve, fan manual (off/low/high) |
| Water heating | On-demand hot water via copper plate | Manual ball valve |

## v1 Scope

**In scope:**

- Autoterm UART communication (start, stop, set power, read telemetry)
- Publish telemetry to Paku-IoT via MQTT
- Coolant flow monitoring (flow sensor) → stop heater if flow drops
- Coolant overheat detection → stop heater
- UART watchdog (no Autoterm response for 30s → stop)
- ESP32 hardware watchdog

**Not in scope (v1):** zone valves, PID control, heat exchanger fan control, scheduling, power profiles, additional DS18B20 sensors. See [FUTURE_FEATURES.md](FUTURE_FEATURES.md).

## Safety

See [SAFETY.md](SAFETY.md). Summary:

| Risk | Owner | Action |
| ---- | ----- | ------ |
| Combustion (overheat, flame, fuel) | Autoterm (certified) | Delegated |
| Coolant flow loss | ESP32 | Flow sensor → stop Autoterm |
| Coolant overheat (>95°C) | ESP32 | Stop Autoterm |
| UART communication loss | ESP32 | 30s timeout → stop Autoterm |
| ESP32 hang | Hardware watchdog | Reset to safe state |

## Architecture: paku-core Add-on

The heater controller is an **optional add-on module** for paku-core, enabled via a compile-time flag. Not every paku-core instance has a heater — some may lack BLE, some serve other purposes.

```text
paku-core (base)          ← WiFi, MQTT, display, OTA
  └── heater add-on       ← AutotermUART, flow safety (compile-time opt-in)
  └── (other add-ons)     ← future modules
```

**When enabled** (on the paku-core instance wired to the Autoterm):

- Shares WiFi, MQTT, display, and BLE stack — no extra device
- Cabin temp from RuuviTag directly available (no MQTT round-trip)
- Heater status on the existing display
- AutotermUART module is self-contained (UART + safety logic)

**When not enabled** — paku-core operates normally without heater code.

**Build flag**: `platformio.ini` build flag (e.g. `-D HEATER_ENABLED`) controls inclusion. Heater-specific code compiles out cleanly when disabled.

## Build

```bash
pio run              # Build
pio run -t upload    # Flash
pio device monitor   # Serial (115200 baud)
```

## Cost Estimate (v1, EUR)

| Item | Cost |
| ---- | ---- |
| Autoterm Flow 5D (12V) | ~€600 |
| Flow sensor | ~€15 |
| Wiring, connectors | ~€20 |
| **Total** (ESP32 shared with paku-core) | **~€635** |

> Plumbing (heat exchanger, floor loops, copper plate, ball valves, expansion tank) is separate from the controller budget.

## Documentation

| Document | Content |
| -------- | ------- |
| [README.md](README.md) | Project overview, hardware, v1 scope (this file) |
| [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) | Autoterm specs, UART protocol |
| [SAFETY.md](SAFETY.md) | Safety requirements |
| [PAKU_INTEGRATION.md](PAKU_INTEGRATION.md) | MQTT topics, Paku-IoT integration |
| [FUTURE_FEATURES.md](FUTURE_FEATURES.md) | Deferred features (valves, PID, scheduling, sensors) |
