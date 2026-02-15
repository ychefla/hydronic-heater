# Hydronic Heater Wiring

Pin assignments and wiring for the Autoterm Flow 5D heater add-on (ESP32).

## Pin Summary

| Pin | Function | Notes |
|-----|----------|-------|
| GPIO16 | UART RX (ESP32 ← Autoterm TX) | Via level shifter |
| GPIO17 | UART TX (ESP32 → Autoterm RX) | Via level shifter |
| GPIO4 | DS18B20 OneWire (coolant return) | 4.7kΩ pull-up to 3.3V |
| GPIO13 | Flow sensor (pulse counter) | Set to -1 if not installed |

Pins are defined in `include/config.h` (copy from `config.example.h`).

## Autoterm UART (5V TTL → 3.3V)

⚠️ **The Autoterm Flow 5D uses 5V TTL. Do NOT connect directly to ESP32 (3.3V)!**

Use a bidirectional level shifter (e.g. ADUM1201):

```
ESP32 (3.3V)          Level Shifter          Autoterm Flow 5D (5V TTL)
────────────          ─────────────          ────────────────────────
GPIO16 (RX2)  ◄────── ADUM1201 ──────◄────── TX  (White wire)
GPIO17 (TX2)  ──────► ADUM1201 ──────►────── RX  (Green wire)
GND           ──────────────────────────────  GND (Blue wire)
                                              +5V (Red wire)
```

## DS18B20 Coolant Sensor

```
ESP32                DS18B20
─────                ───────
3.3V  ──────┬─────── VDD  (pin 3, red)
            │
          [4.7kΩ]    ← pull-up (required)
            │
GPIO4 ──────┴─────── DQ   (pin 2, yellow)
GND   ────────────── GND  (pin 1, black)
```

Mount on the coolant return line for accurate temperature reading.

## Flow Sensor

Connect the pulse output to GPIO13. If not installed, set `FLOW_SENSOR_PIN` to `-1` in `config.h`.
