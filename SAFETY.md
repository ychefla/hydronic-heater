# Safety Requirements

Safety requirements for the ESP32 heater controller. Combustion safety is delegated to the certified Autoterm controller.

## Responsibility Split

| Domain | Owner | Notes |
| ------ | ----- | ----- |
| Combustion (flame, ignition, fuel metering) | **Autoterm** (certified) | E-mark, CE — not our concern |
| Combustion overheat | **Autoterm** | Internal sensors + shutdown logic |
| Supply voltage protection | **Autoterm** | Internal |
| Coolant flow monitoring | **ESP32** | Flow sensor on coolant circuit |
| Coolant overheat | **ESP32** | Stop Autoterm if coolant > 95°C |
| UART communication watchdog | **ESP32** | No response → stop Autoterm |
| ESP32 hardware watchdog | **ESP32** | Hang → hardware reset |

## ESP32 Safety Requirements (v1)

### SAFE-F1: Coolant Flow Monitoring

**Priority**: CRITICAL

If coolant stops flowing while the heater runs, coolant can boil and cause damage or injury.

| Requirement | Detail |
| ----------- | ------ |
| Sensor | Flow sensor on coolant return (GPIO 13) |
| Trigger | Flow drops below minimum threshold while Autoterm is running |
| Action | Send stop command to Autoterm, enter ERROR state |
| Response time | < 5 seconds from flow loss detection |
| Recovery | Manual reset required (restart ESP32 or clear error via MQTT) |

### SAFE-T1: Coolant Overheat Protection

**Priority**: CRITICAL

Prevent coolant from reaching boiling point. Coolant temperature source depends on what Autoterm provides via UART.

| Requirement | Detail |
| ----------- | ------ |
| Source | Autoterm UART telemetry (preferred) or dedicated DS18B20 (if needed) |
| Trigger | Coolant temperature > 95°C |
| Action | Send stop command to Autoterm, enter ERROR state |
| Response time | < 2 seconds |
| Recovery | Manual reset after coolant drops below 70°C |

> **Open question**: Verify whether Autoterm provides coolant temperature via UART. If not, add a DS18B20 on the coolant return line for this safety function.

### SAFE-C1: UART Communication Watchdog

**Priority**: CRITICAL

If communication with Autoterm is lost, the ESP32 cannot monitor or control the heater.

| Requirement | Detail |
| ----------- | ------ |
| Trigger | No valid UART response from Autoterm for 30 seconds |
| Action | Send stop command (best-effort), enter ERROR state, publish MQTT alert |
| Recovery | Automatic retry after 60 seconds; manual reset if persistent |

### SAFE-W1: ESP32 Hardware Watchdog

**Priority**: CRITICAL

Protect against ESP32 firmware hang.

| Requirement | Detail |
| ----------- | ------ |
| Implementation | Hardware watchdog timer (WDT), 10-second timeout |
| Trigger | Main loop fails to feed watchdog |
| Action | Hardware reset → ESP32 reboots into safe state (heater left in last state; Autoterm has its own timeout) |

### SAFE-S1: Sensor Validation

**Priority**: HIGH

Detect faulty sensor readings before acting on them.

| Requirement | Detail |
| ----------- | ------ |
| Flow sensor | Validate signal is within expected range; ignore single glitches |
| Temperature | Reject readings outside physically possible range (e.g. −50°C to +150°C) |
| Action on failure | Log warning; if persistent (> 10s), treat as sensor failure → stop heater |

## General Safety Notes

- **CO detector**: Install a standalone carbon monoxide detector in the cabin. This is independent of the ESP32 — it's a life-safety device.
- **Ventilation**: Ensure cabin has adequate ventilation per Autoterm installation manual.
- **Fusing**: Protect all 12V wiring with appropriate fuses per Autoterm specs.
- **Fuel**: Use proper diesel-rated fuel lines and fittings. Follow Autoterm installation guide.

## Not in v1

The following safety features are deferred to future releases (see [FUTURE_FEATURES.md](FUTURE_FEATURES.md)):

- Per-zone temperature limits (floor max 50°C, water max 85°C) — requires zone valves + DS18B20 sensors
- WiFi/MQTT loss handling (continue on last settings, revert after timeout)
- Low-voltage protection at ESP32 level (Autoterm handles its own)
