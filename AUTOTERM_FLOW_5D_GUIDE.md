# Autoterm Flow 5D — Heater Reference

Heater-specific information for the Autoterm Flow 5D. For project overview and v1 scope, see [README.md](README.md).

## Specifications

| Parameter | Value |
| --------- | ----- |
| Model | Autoterm Flow 5D (liquid/hydronic) |
| Heat output | 1.4 – 5.0 kW (continuously variable) |
| Voltage | 12V DC |
| Fuel consumption | 0.18 – 0.62 L/hr diesel |
| Coolant pump | Built-in circulation pump |
| Weight | ~3.2 kg |
| Operating temp range | −40°C to +40°C ambient |
| Certifications | E-mark (ECE R122), CE |

## What Autoterm Manages Internally

The certified Autoterm controller handles all combustion:

- Glow plug ignition and management
- Fuel metering pump (variable output)
- Combustion air fan (variable speed)
- Flame monitoring and safety shutdown
- Overheat protection
- Error diagnostics and recovery

**The ESP32 never touches combustion.** It talks to the Autoterm controller via UART.

## UART Protocol

### Physical Connection

```text
ESP32                    Autoterm Flow 5D
─────                    ────────────────
GPIO 16 (RX2) ◄──────── TX
GPIO 17 (TX2) ────────►  RX
GND ──────────────────── GND

Baud rate: 2400, 8N1 (verify with Autoterm docs)
Voltage: 3.3V or 5V (verify for your unit)
```

Connection is made at the Autoterm control panel connector.

### Frame Format

Binary frame-based protocol (community-documented, verify with official docs):

```text
┌──────┬────────┬─────────┬──────────┬──────────┐
│ SYNC │ LENGTH │ COMMAND │   DATA   │ CHECKSUM │
│ 0xAA │ 1 byte │ 1 byte  │ N bytes  │ 1 byte   │
└──────┴────────┴─────────┴──────────┴──────────┘
```

### Commands

| Command | Direction | Description |
| ------- | --------- | ----------- |
| Start heater | ESP32 → Autoterm | Begin startup sequence |
| Stop heater | ESP32 → Autoterm | Begin shutdown sequence |
| Set power level | ESP32 → Autoterm | Set heat output (%) |
| Set target temp | ESP32 → Autoterm | Set thermostat target |
| Request status | ESP32 → Autoterm | Poll current state |
| Status response | Autoterm → ESP32 | State + telemetry data |

### Telemetry from UART

| Parameter | Description |
| --------- | ----------- |
| Heater state | Off / Starting / Running / Stopping / Error |
| Combustion temp | Internal temperature |
| Fan RPM | Combustion fan speed |
| Fuel rate | Current consumption (L/hr) |
| Power level | Current output (%) |
| Error code | Diagnostic code (if error state) |
| Supply voltage | Battery voltage |

> **Open question**: Does the UART telemetry include coolant supply/return temperature? If not, a DS18B20 on the coolant return is needed for overheat safety. To be verified with Autoterm documentation.

## Obtaining Documentation

Autoterm provides integration documentation to integrators on request:

- **Email**: <support@autoterm.com>
- **Website**: [autoterm.com](https://autoterm.com)
- Dealers can also assist with technical documentation

> Request the UART protocol specification before starting Phase 2 (UART integration).

## Why Autoterm

Selected over alternatives because:

- **UART protocol available** — Autoterm shares documentation with integrators
- **Certified combustion** — E-mark, CE (no DIY combustion safety needed)
- **Community** — widely used in van conversion and marine
- **Quality** — Estonian manufacturer, proper engineering

| | Autoterm Flow 5D | Chinese heaters | Webasto/Eberspächer |
| --- | --- | --- | --- |
| Protocol access | ✅ Available | ❌ Proprietary | ❌ Locked |
| DIY integration | ✅ Supported | ⚠️ Reverse-engineer | ❌ Not intended |
| Certifications | ✅ E-mark, CE | ⚠️ Variable | ✅ Premium |
| Price | ~€600 | ~€150–300 | ~€1000+ |
