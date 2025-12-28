# Wiring Guide for Hydronic Heater Controller

## Safety First!
- Always disconnect power before wiring
- Use appropriate wire gauges for current levels
- Ensure all connections are secure and insulated
- Use fuses on all high-current circuits
- Never bypass safety devices

## Component Wiring Details

### 1. ESP32 Board
- Power: 5V via USB or VIN pin (with voltage regulator)
- Ground: Connect to common ground rail

### 2. DS18B20 Temperature Sensors (x4)

Each sensor requires:
- Red wire → 3.3V or 5V
- Black wire → GND
- Yellow/White wire → GPIO pin (with 4.7kΩ pullup resistor to VCC)

Connections:
```
Sensor 1 (Burning Chamber):
  GPIO 4 ----[4.7kΩ]---- 3.3V
  GPIO 4 ---------------- DS18B20 Data Pin
  
Sensor 2 (Coolant Input):
  GPIO 16 ----[4.7kΩ]---- 3.3V
  GPIO 16 ---------------- DS18B20 Data Pin
  
Sensor 3 (Coolant Output):
  GPIO 17 ----[4.7kΩ]---- 3.3V
  GPIO 17 ---------------- DS18B20 Data Pin
  
Sensor 4 (Air Temperature):
  GPIO 5 ----[4.7kΩ]---- 3.3V
  GPIO 5 ---------------- DS18B20 Data Pin
```

### 3. Glow Plug Control (GPIO 25) - HIGH CURRENT!

**CRITICAL**: Glow plugs draw 50-100A!

```
ESP32 GPIO 25 → MOSFET Gate (with 1kΩ resistor)
MOSFET Source → GND
MOSFET Drain → Glow Plug (-) 
12V Battery (+) → Glow Plug (+)

Recommended MOSFET: IRLB8721 or similar (60V, 100A+)
Add flyback diode across glow plug
Use 8-10 AWG wire for glow plug circuit
```

### 4. Diesel Pump Control (GPIO 26)

```
ESP32 GPIO 26 → MOSFET Gate (with 1kΩ resistor) or Relay coil
MOSFET/Relay → 12V Pump
Add flyback diode for protection
Wire gauge: 14-16 AWG
```

### 5. Air Fan Control (GPIO 27) - PWM

```
ESP32 GPIO 27 → MOSFET Gate (with 1kΩ resistor)
MOSFET Source → GND
MOSFET Drain → Fan (-)
12V Battery (+) → Fan (+)

For PWM control, use logic-level MOSFET
Wire gauge: 16-18 AWG
```

### 6. Coolant Pump Control (GPIO 14)

```
ESP32 GPIO 14 → MOSFET Gate (with 1kΩ resistor) or Relay
MOSFET/Relay → 12V Pump
Add flyback diode
Wire gauge: 14-16 AWG
```

### 7. Heat Exchanger Fan Control (GPIO 12) - PWM

```
ESP32 GPIO 12 → MOSFET Gate (with 1kΩ resistor)
MOSFET Source → GND
MOSFET Drain → Fan (-)
12V Battery (+) → Fan (+)

Wire gauge: 16-18 AWG
```

### 8. Flow Sensor (GPIO 13) - Optional

```
Flow Sensor VCC → 5V
Flow Sensor GND → GND
Flow Sensor Signal → GPIO 13

Add 10kΩ pullup resistor on signal line if needed
```

## MOSFET Driver Circuit Example

For each high-current component:

```
                     +12V
                      |
                   [Load]
                      |
ESP32 GPIO ---|1kΩ|--Gate
                      |
                    Drain
                   [MOSFET]
                    Source
                      |
                [Flyback Diode]
                      |
                     GND
```

## Power Distribution

```
12V Battery (+)
   |
   +--[Fuse 100A]--→ Glow Plug Circuit
   |
   +--[Fuse 10A]---→ Diesel Pump
   |
   +--[Fuse 10A]---→ Fans and Pumps
   |
   +--[Fuse 5A]----→ ESP32 Power Supply (via voltage regulator)

GND Battery (-)
   |
   +---------------→ Common Ground for all circuits
```

## Recommended Components

### MOSFETs
- Glow Plug: IRLB8721 (62V, 62A continuous)
- Pumps/Fans: IRLZ44N (55V, 47A) or similar logic-level MOSFETs

### Protection
- Flyback diodes: 1N4007 or equivalent (1A minimum)
- Fuses: Automotive blade fuses at appropriate ratings

### Wire Gauges (Automotive)
- Glow plug: 8 AWG minimum
- Diesel pump: 14 AWG
- Fans: 16 AWG
- Sensors: 22-24 AWG

## Testing Procedure

1. **Visual Inspection**
   - Check all connections
   - Verify polarity
   - Ensure no shorts

2. **Low-Power Test**
   - Power ESP32 only
   - Upload and test code
   - Verify serial communication

3. **Sensor Test**
   - Connect temperature sensors
   - Verify readings

4. **Component Test (NO FUEL)**
   - Test each component individually
   - Start with low PWM values
   - Verify proper operation

5. **System Integration**
   - Test complete system
   - Verify safety shutdowns
   - Test with fuel (in controlled environment)

## Troubleshooting

### Sensor reads -127°C
- Check wiring
- Verify pullup resistor
- Check sensor power

### Component doesn't activate
- Check MOSFET wiring
- Verify gate voltage
- Test component directly with power

### ESP32 resets
- Check power supply capacity
- Add decoupling capacitors
- Separate high-current grounds

## Safety Checklist

- [ ] All fuses installed
- [ ] All connections secure
- [ ] No bare wire exposed
- [ ] Proper wire gauges used
- [ ] Flyback diodes installed
- [ ] Power supply adequate
- [ ] Tested without fuel first
- [ ] Fire extinguisher nearby
- [ ] Ventilation adequate
- [ ] Emergency shutoff accessible
