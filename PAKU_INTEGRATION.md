# Paku-IoT Integration

MQTT integration between the ESP32 heater controller and Paku-IoT. For project overview, see [README.md](README.md).

## MQTT Topics

Base prefix: `paku/devices/{device_id}/`

| Topic suffix | Direction | QoS | Retained | Description |
|-------------|-----------|-----|----------|-------------|
| `state` | Device → Broker | 1 | Yes | Online/offline (LWT sets `offline`) |
| `telemetry/heater` | Device → Broker | 0 | No | Heater telemetry (periodic) |
| `cmd` | Broker → Device | 1 | No | Commands (start, stop, set power) |
| `alert` | Device → Broker | 1 | No | Safety alerts |

## Telemetry Payload

Published every 10 seconds while heater is running:

```json
{
  "timestamp": "2025-12-28T08:00:00Z",
  "device_id": "heater-AABBCC",
  "heater_state": "running",
  "power_level": 60,
  "combustion_temp": 180,
  "fan_rpm": 3200,
  "fuel_rate": 0.35,
  "supply_voltage": 12.8,
  "coolant_flow": true,
  "error_code": 0
}
```

> Fields available depend on what Autoterm provides via UART. Payload will be finalized during UART integration.

## Command Payload

```json
{
  "command": "start",
  "power_level": 60
}
```

| Command | Parameters | Description |
|---------|------------|-------------|
| `start` | `power_level` (optional, %) | Start heater at given power (default: 50%) |
| `stop` | — | Stop heater |
| `set_power` | `power_level` (%) | Adjust power while running |

## Alert Payload

```json
{
  "timestamp": "2025-12-28T08:05:00Z",
  "alert": "COOLANT_FLOW_LOSS",
  "action": "heater_stopped",
  "message": "Flow sensor detected no coolant flow. Heater stopped."
}
```

| Alert | Safety Req | Trigger |
|-------|-----------|---------|
| `COOLANT_FLOW_LOSS` | SAFE-F1 | Flow sensor below threshold |
| `COOLANT_OVERHEAT` | SAFE-T1 | Coolant > 95°C |
| `UART_TIMEOUT` | SAFE-C1 | No Autoterm response for 30s |
| `SENSOR_FAILURE` | SAFE-S1 | Persistent invalid readings |

## LWT (Last Will and Testament)

| Setting | Value |
|---------|-------|
| Topic | `paku/devices/{device_id}/state` |
| Payload | `offline` |
| QoS | 1 |
| Retain | Yes |

On connect, device publishes `online` to the same topic (retained).

## Connection

| Parameter | Value |
|-----------|-------|
| Broker | Configured in `secrets.h` |
| Port | 1883 (or 8883 for TLS) |
| Client ID | `heater-{mac_suffix}` |
| Clean session | Yes |
| Keep-alive | 60s |
