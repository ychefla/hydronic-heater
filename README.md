# Hydronic Diesel Heater Controller

ESP32 smart controller for the **Autoterm Flow 5D** hydronic diesel heater in a campervan.

> ## ⚠️ Status: Waiting for Autoterm CAN Adapter
>
> **The UART protocol code in this repo is UNVERIFIED for the Flow 5D.**
> It was inferred from community reverse-engineering of Autoterm **Air 2D/4D**
> (different product line). Autoterm has confirmed the Flow 5D protocol is
> proprietary and not publicly documented.
>
> **Autoterm is developing a CAN adapter** that will decode heater signals to
> the **CIVD standard** (standardized CAN protocol for vehicle heaters).
> Estimated availability: 2026. When released, this project will integrate
> via CAN instead of the unverified UART protocol.
>
> **Current code status:**
> - `lib/CanBus/` — Generic ESP32 TWAI/CAN driver (ready, hardware-agnostic)
> - `lib/AutotermUART/` — ⚠️ UNVERIFIED, based on Air 2D/4D inference
> - `lib/AutotermEmulator/` — Emulator for the unverified UART protocol
> - `lib/HeaterAddon/` — paku-core integration (works with emulator)
> - `lib/Sensors/` — DS18B20 + flow sensor drivers (verified, hardware-independent)

## Overview

The Autoterm Flow 5D handles combustion (certified controller). The ESP32 adds:

- **CAN bus integration** (pending): CIVD-standard communication via Autoterm's upcoming CAN adapter
- **MQTT telemetry**: publish heater data to Paku-IoT
- **Safety monitoring**: coolant flow, coolant overheat, communication watchdog

> **Note:** The original plan was UART control (start/stop, power setpoint, read telemetry).
> However, Autoterm confirmed the UART protocol is proprietary. The project is transitioning
> to CAN/CIVD when the adapter becomes available. UART code remains for reference/emulation only.

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
| Autoterm CAN adapter | CIVD bridge | ⏳ Not yet available — ETA 2026 |
| ESP32 (LilyGo T-Display S3) | Smart controller | Runs as paku-core add-on (compile-time flag) |
| CAN transceiver (SN65HVD230) | 3.3V CAN bus interface | Between ESP32 TWAI and CAN adapter |
| Level shifter (ADUM1201) | 5V ↔ 3.3V for UART | ⚠️ UART path — unverified protocol |
| Flow sensor | Coolant flow safety | GPIO 13 |
| DS18B20 | Coolant return temperature | GPIO 4 (OneWire) — for SAFE-T1 |
| UART cable | ESP32 ↔ Autoterm | GPIO 16 RX, GPIO 17 TX — ⚠️ unverified |

### Temperature Sources

| Measurement | Source | v1? |
| ----------- | ------ | --- |
| Cabin air | RuuviTag via paku-core BLE | ✅ |
| Combustion temp | Autoterm UART telemetry | ✅ |
| Fan RPM | Autoterm UART telemetry | ✅ |
| Fuel rate | Autoterm UART telemetry | ✅ |
| Supply voltage | Autoterm UART telemetry | ✅ |
| Coolant return temp | DS18B20 on coolant return line | ✅ Required for SAFE-T1 |
| Floor supply/return | DS18B20 (need suitable probes) | ❌ See [future](FUTURE_FEATURES.md) |

> **Resolved**: Community protocol analysis (Air 2D/4D) shows no coolant temperature
> in the UART status payload. A DS18B20 on the coolant return is required for overheat
> safety (SAFE-T1). To be re-verified when Flow 5D UART is sniffed — the Flow may
> expose coolant temp in currently-unknown status bytes.

### Heating Circuits

| Circuit | Purpose | v1 Control |
| ------- | ------- | ---------- |
| Floor heating | Underfloor radiant loops | Manual ball valve |
| Air heating | Cabin air via heat exchanger + fan | Manual ball valve, fan manual (off/low/high) |
| Water heating | On-demand hot water via copper plate | Manual ball valve |

## v1 Scope

**In scope:**

- Generic ESP32 CAN/TWAI driver (ready — `lib/CanBus/`)
- Coolant flow monitoring (flow sensor) → stop heater if flow drops
- Coolant overheat detection → stop heater
- Communication watchdog (UART or CAN — no response → stop)
- ESP32 hardware watchdog
- Publish telemetry to Paku-IoT via MQTT
- Autoterm emulator for development/testing (no hardware needed)

**Blocked (waiting for Autoterm CAN adapter):**

- CIVD protocol integration (start, stop, set power, read telemetry via CAN)
- Verified heater communication — UART protocol is unverified for Flow 5D

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
# Production (requires real Autoterm + DS18B20 + level shifter)
pio run -e esp32dev -t upload

# Emulator (no wiring needed — internal UART loopback)
pio run -e emulator -t upload
pio device monitor   # Serial console (115200 baud)
```

The emulator build (`-D EMULATOR_MODE`) runs a simulated Autoterm ECU
on Serial1, cross-wired to the driver on Serial2 via the ESP32 GPIO
matrix. Supports error injection and simulated coolant temperature for
testing the safety layer. Type `help` in the serial console for commands.

## Cost Estimate (v1, EUR)

| Item | Cost |
| ---- | ---- |
| Autoterm Flow 5D (12V) | ~€600 |
| Flow sensor | ~€15 |
| DS18B20 (coolant return) | ~€3 |
| Level shifter (ADUM1201) | ~€5 |
| Wiring, connectors | ~€20 |
| **Total** (ESP32 shared with paku-core) | **~€643** |

> Plumbing (heat exchanger, floor loops, copper plate, ball valves, expansion tank) is separate from the controller budget.

## Documentation

| Document | Content |
| -------- | ------- |
| [README.md](README.md) | Project overview, hardware, v1 scope (this file) |
| [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) | Autoterm specs, UART protocol (⚠️ unverified for Flow 5D) |
| [SAFETY.md](SAFETY.md) | Safety requirements |
| [PAKU_INTEGRATION.md](PAKU_INTEGRATION.md) | MQTT topics, Paku-IoT integration |
| [FUTURE_FEATURES.md](FUTURE_FEATURES.md) | Deferred features (valves, PID, scheduling, sensors) |
