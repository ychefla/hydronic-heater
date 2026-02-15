# Future Features

Features deferred from v1. See [README.md](README.md) for current scope.

---

## Motorized Zone Valves

Replace manual ball valves with ESP32-controlled motorized valves.

- Floor heating valve (GPIO TBD)
- Air heating valve (GPIO TBD)
- Water heating valve (GPIO TBD)
- Independent on/off per zone
- **Prerequisite**: identify suitable motorized ball valves

## PID Temperature Control

Per-zone PID feedback for automatic temperature regulation.

- Cabin target temp → adjust Autoterm power setpoint
- Floor target temp → modulate floor valve + power
- **Prerequisite**: zone valves + additional temperature sensors

## Additional DS18B20 Sensors

Wired sensors for zone monitoring (not needed in v1 — Autoterm provides internal telemetry, cabin temp from RuuviTag).

- Coolant supply/return — if not available via Autoterm UART
- Floor supply/return — for floor PID control
- **Prerequisite**: identify suitable high-temperature probe housings

## Heat Exchanger Fan Control

ESP32-controlled fan for cabin air heating (currently manual: off/low/high).

- PWM speed control via MOSFET driver
- Integrate with cabin temperature PID
- **Note**: may require separate ESP32 + hardware — TBD

## Power Profiles

Named presets for Autoterm power setpoint:

| Profile | Power | Use Case |
| ------- | ----- | -------- |
| Eco | ~30% (~0.18 L/hr) | Overnight, quiet |
| Normal | ~60% (~0.35 L/hr) | General use |
| Boost | 100% (~0.62 L/hr) | Quick warmup |

## Scheduled Heating

Timed heating with day-of-week support:

- Up to 4 schedule slots
- Power profile per slot
- Persistent across power loss (NVS storage)

## Outside Temperature

Source TBD (RuuviTag outside, DS18B20, or weather API). For smart pre-heating logic.

## Grafana Dashboards

Pre-built dashboards via Paku-IoT for historical temperature, fuel consumption, runtime tracking.

## Per-Zone Safety Limits

Once zone valves and DS18B20 sensors are installed:

- Floor zone max: 50°C → close floor valve
- Water zone max: 85°C → close water valve
- WiFi/MQTT loss: continue on last settings, revert to safe defaults after timeout
