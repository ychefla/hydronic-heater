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

> **Source**: Reverse-engineered from Autoterm **Air 2D/4D** (community projects below).
> The Flow 5D (hydronic) likely uses the same protocol framework but **status payload
> fields may differ** — the Air models have no coolant. Verify with real hardware.

### Community References

- [Boren/ha-autoterm-diesel-heater](https://github.com/Boren/ha-autoterm-diesel-heater) — Python, full PROTOCOL.md (Air model)
- [timokovanen/esphome-autoterm](https://github.com/timokovanen/esphome-autoterm) — C++ ESP32/ESPHome (Air 2D & 4D)
- [grimoire314 blog](https://grimoire314.wordpress.com/2018/08/22/planar-diesel-heater-controller-reverse-engineering/) — original reverse-engineering

### Physical Connection

```text
ESP32 (3.3V)     Level Shifter     Autoterm Flow 5D (5V TTL)
────────────     ─────────────     ────────────────────────
GPIO 16 (RX2) ◄── ADUM1201 ───◄── TX (White wire)
GPIO 17 (TX2) ──► ADUM1201 ─────► RX (Green wire)
GND ─────────────────────────────── GND (Blue wire)
                                    +5V (Red wire)
```

| Parameter | Value |
| --------- | ----- |
| Baud rate | 2400 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Logic level | **5V TTL** — level shifter required (e.g. ADUM1201) |

⚠️ **ESP32 is 3.3V. Autoterm is 5V. Direct connection risks damaging the ESP32.**

Connection at the Autoterm control panel connector. Wire colors may vary at connectors.

### Frame Format (confirmed)

```text
┌──────────┬────────┬────────┬──────────┬─────────┬─────────────┬───────────┐
│ Preamble │ Sender │ Length │ Reserved │ Command │ Payload     │ CRC-16    │
│ 0xAA     │ 1 byte │ 1 byte │ 0x00     │ 1 byte  │ N bytes     │ 2 bytes   │
└──────────┴────────┴────────┴──────────┴─────────┴─────────────┴───────────┘
```

- **Sender**: `0x03` = Panel (controller), `0x04` = Heater
- **Length**: payload byte count (after command, before CRC)
- **CRC**: CRC-16 Modbus, polynomial 0x8005 (reversed), init 0xFFFF, **big-endian**
- **Minimum message**: 7 bytes (no payload)

### Commands (confirmed for Air, expected same for Flow)

| ID | Name | Direction | Payload | Description |
| --- | --- | --- | --- | --- |
| `0x01` | START | Panel→Heater | 6 bytes | Start with mode/temp/power |
| `0x02` | GET/SET | Both | 0 or 6 bytes | Query or change settings |
| `0x03` | SHUTDOWN | Panel→Heater | 0 bytes | Stop heater |
| `0x0F` | STATUS | Both | 0 / 10 bytes | Request / report status |
| `0x11` | PANEL_TEMP | Both | 1 byte | Exchange panel temperature |
| `0x23` | VENTILATION | Both | 3 bytes | Fan-only mode |

### START Payload (6 bytes)

```text
Bytes 0-1: 0xFF 0xFF (marker)
Byte 2:    Mode — 0x01=By Heater, 0x02=By Panel, 0x03=By External, 0x04=By Power
Byte 3:    Target temp °C (0xFF if unused)
Byte 4:    Ventilation — 0x01=On, 0x02=Off
Byte 5:    Power level 0-9 (0xFF if unused)
```

### STATUS Response (10 bytes, from Air — verify for Flow)

| Byte | Field | Notes |
| --- | --- | --- |
| 0 | State | 0x00=Off, 0x01=Starting, 0x04=Running, 0x05=Shutting Down, 0x08=Ventilation |
| 1 | Unknown | |
| 2 | Error code | 0x00=None, 0x01=Overheat, 0x02=Voltage, ... (19 codes) |
| 3 | Unknown | |
| 4-5 | Battery voltage | Little-endian, ×10 (e.g. 0x7F 0x00 = 12.7V) |
| 6-7 | Unknown | |
| 8 | Core temp °C | Heater core temperature |
| 9 | Unknown | |

> ⚠️ **Flow 5D difference**: The Air models have no coolant. The Flow 5D may use
> unknown bytes (1, 3, 6-7, 9) for coolant temperature or pump status. Must verify
> by sniffing UART traffic on real Flow 5D hardware.

### Communication Pattern (confirmed)

Repeating 3-second cycle:

1. **Second 1**: GET settings (`0x02`)
2. **Second 2**: STATUS request (`0x0F`)
3. **Second 3**: PANEL_TEMP exchange (`0x11`)

### Error Codes (confirmed for Air)

| Code | Name | Description |
| --- | --- | --- |
| 0x00 | None | No error |
| 0x01 | E01 | Overheating |
| 0x02 | E02 | Voltage too high/low |
| 0x03 | E03 | Glow plug failure |
| 0x04 | E04 | Fuel pump failure |
| 0x05 | E05 | Flame sensor failure |
| 0x06 | E06 | Temperature sensor failure |
| 0x07 | E07 | Combustion air fan failure |
| 0x09 | E09 | Failed to start |
| 0x0D | E13 | No flame / ignition failure |

### What Needs Verification on Flow 5D

- [ ] Do unknown status bytes carry coolant temp or pump status?
- [ ] Are command IDs and payloads identical to Air models?
- [ ] Any additional Flow-specific error codes?
- [ ] Baud rate confirmed 2400 on Flow 5D?

Verification method: sniff UART between stock controller and Flow 5D using
[Boren's monitor tool](https://github.com/Boren/ha-autoterm-diesel-heater/blob/main/monitor/heater_monitor.py).

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
