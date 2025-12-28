# Camper Van Heater System - Feature Overview

## Purpose

This document provides an overview of the extended hydronic heater controller specifically designed for camper van applications, based on cheap Chinese diesel heaters with significant feature enhancements.

---

## Base Hardware: Chinese Hydronic Diesel Heaters

### Typical Specifications
- **Power Output**: 2kW, 3kW, or 5kW models
- **Voltage**: 12V or 24V DC
- **Fuel**: Diesel
- **Heat Transfer**: Combined air and water/coolant
- **Cost**: $150-$400 USD
- **Common Brands**: Vevor, Hcalory, generic marketplace brands

### Stock Controller Limitations
- ❌ Simple on/off control only
- ❌ No temperature feedback
- ❌ No remote control
- ❌ Fixed power output
- ❌ Noisy operation
- ❌ No scheduling
- ❌ Poor integration options

### Our Enhanced Controller
- ✅ Variable power output (20-100%)
- ✅ Multi-zone temperature control
- ✅ WiFi and MQTT remote control
- ✅ Scheduled heating
- ✅ Quiet "Eco" mode
- ✅ Home Assistant integration
- ✅ Comprehensive monitoring

---

## Key Features

### 1. Variable Power Output

**Problem Solved**: Stock controllers run at full power or off, wasting fuel and creating noise.

**Our Solution**:
- PWM control of fuel pump (20-100% power)
- Proportional air fan speed
- Three power profiles:
  - **Eco Mode**: 30% power, whisper-quiet, 0.15 L/hr fuel
  - **Normal Mode**: 60% power, balanced, 0.3 L/hr fuel
  - **Boost Mode**: 100% power, maximum heat, 0.5 L/hr fuel

**Benefits**:
- 40-60% fuel savings in Eco mode
- Much quieter operation for sleeping
- Maintains comfortable temperature without cycling
- Extends component life

### 2. Multi-Zone Heating Control

**Camper Van Heating Needs**:
1. **Floor Heating**: Underfloor loops for radiant warmth
2. **Air Heating**: Heat exchanger with fans for quick cabin warming
3. **Water Heating**: Hot water for washing (hands, dishes, shower)

**Implementation**:
```
Diesel Heater → Hot Coolant → 3-Way Valve Distribution
                                    ↓
                        ┌───────────┼──────────┐
                        ↓           ↓          ↓
                    Floor       Air Heat    Water
                    Loops      Exchanger    Tank
```

**Zone Control**:
- Independent on/off for each zone
- Separate temperature targets
- Safety limits per zone
- Priority management (air first for quick warmup)

**Example Configuration**:
- Cabin air: Target 20°C
- Floor: Target 25°C (max 40°C for safety)
- Water tank: Target 45°C (max 80°C for safety)

### 3. MQTT Remote Control

**Use Cases**:
- Control from bed without getting up
- Monitor status from inside living area
- Integration with home automation
- Remote monitoring when away from van

**MQTT Topics**:
```
camper/heater/status              → Full system status (JSON)
camper/heater/temperature/cabin   → 20.5°C
camper/heater/temperature/floor   → 24.0°C
camper/heater/temperature/water   → 42.0°C
camper/heater/mode                → "continuous"
camper/heater/power               → 60 (percent)

camper/heater/cmd/set_mode        ← "continuous", "scheduled", "off"
camper/heater/cmd/set_target      ← {"zone": "cabin", "temp": 21.0}
camper/heater/cmd/set_power       ← 30 (percent)
camper/heater/cmd/set_profile     ← "eco", "normal", "boost"
```

**Home Assistant Integration**:
- Auto-discovery support
- Climate entity for easy control
- Temperature sensors
- Status sensors
- Service calls for advanced control

### 4. Scheduled Heating

**Typical Scenarios**:

**Morning Warmup**:
```
Schedule: "Wake Up"
Days: Monday-Friday
Start: 06:30
Duration: 120 minutes
Profile: Boost
Action: Pre-warm cabin before waking
```

**Night Heating**:
```
Schedule: "Sleep Mode"
Days: Every day
Start: 22:00
Duration: 480 minutes (8 hours)
Profile: Eco
Action: Maintain comfortable temperature quietly
```

**Pre-arrival**:
```
Schedule: "Return to Van"
Days: Saturday-Sunday
Start: 16:00
Duration: 60 minutes
Profile: Normal
Action: Warm van before returning from activities
```

**Features**:
- Up to 4 independent schedules
- Day-of-week selection
- Duration or end-time based
- Power profile per schedule
- Override capability
- Persistent across power loss

### 5. Continuous Heating Mode

**Purpose**: Maintain target temperature indefinitely

**How it Works**:
1. Set target temperature (e.g., 20°C for cabin)
2. Enable continuous mode
3. Controller automatically:
   - Monitors room temperature
   - Adjusts power output (20-100%)
   - Maintains target ±1°C
   - Optimizes fuel consumption

**PID Control**:
- Proportional-Integral-Derivative algorithm
- Smooth temperature control
- No oscillation or overshoot
- Self-adjusting to conditions

**Fuel Efficiency**:
- Only uses power needed to maintain temperature
- Typical: 60-80% power during warmup
- Then: 20-40% power to maintain
- vs. Stock: Always 100% with on/off cycling

### 6. Temperature Feedback Control

**Sensors Used**:
- Burning chamber (combustion control)
- Coolant input/output (system monitoring)
- Cabin air (comfort control)
- Floor surface (radiant heating)
- Water tank (hot water)
- Outside air (optional, for smart control)

**Smart Algorithms**:
- Learns thermal characteristics of your van
- Adjusts pre-heating time based on outside temp
- Prioritizes zones based on use patterns
- Predicts fuel consumption

---

## Installation in Camper Van

### Typical Layout

```
┌──────────────────────────────────────────────┐
│           Camper Van Interior                │
│                                              │
│  ┌─────────┐         ┌──────────┐           │
│  │ Heater  │         │  Water   │           │
│  │  Unit   │◄───────►│  Tank    │           │
│  │(exterior│ Coolant │ (heated) │           │
│  │ mount)  │         │          │           │
│  └────┬────┘         └──────────┘           │
│       │                                      │
│       │  ┌────────────────────────┐          │
│       └─►│ 3-Way Distribution     │          │
│          │ Valve System          │          │
│          └─┬──────────┬──────────┬┘          │
│            │          │          │           │
│      ┌─────▼────┐ ┌───▼─────┐ ┌─▼────────┐  │
│      │  Floor   │ │  Heat   │ │  Water   │  │
│      │  Heating │ │Exchanger│ │   Coil   │  │
│      │  (under) │ │ + Fans  │ │          │  │
│      └──────────┘ └─────────┘ └──────────┘  │
│                                              │
│  ┌─────────────────────────────────────┐    │
│  │      ESP32 Controller               │    │
│  │  - Mounted inside                   │    │
│  │  - WiFi access from living area     │    │
│  │  - Connected to van 12V system      │    │
│  └─────────────────────────────────────┘    │
│                                              │
│  Control via:                                │
│  • Phone/tablet (MQTT app)                   │
│  • Home Assistant dashboard                  │
│  • Voice assistant (if integrated)           │
└──────────────────────────────────────────────┘
```

### Physical Heater Placement
- **Exterior mounted**: Under van, in basement compartment, or external box
- **Exhaust**: Vented safely away from van
- **Air intake**: From exterior (not cabin air)
- **Fuel**: Connected to diesel tank or separate heater tank
- **Electrical**: 12V from van battery system

### Controller Placement
- **Inside van**: Comfortable location with WiFi coverage
- **Protected**: Away from water, extreme temperatures
- **Accessible**: For occasional configuration changes
- **Wired**: Runs to heater unit (typically 5-10m cable run)

---

## User Experience Examples

### Example 1: Winter Overnight Stay

**Scenario**: Parking overnight, -5°C outside

**Configuration**:
1. Set cabin target: 18°C (comfortable sleeping temp)
2. Enable floor heating: 22°C (warm feet)
3. Water heating: OFF (not needed overnight)
4. Select "Eco" profile (quiet for sleeping)
5. Enable continuous mode

**Result**:
- Heater starts in Boost mode to warm up (15 minutes)
- Transitions to Eco mode (30% power, very quiet)
- Maintains temperature all night
- Fuel consumption: ~1.2 liters for 8 hours
- Cost: ~$1.50 for comfortable night

### Example 2: Morning Routine

**Pre-configured Schedule**: "Morning Warmup"
- Starts: 6:30 AM
- Profile: Boost
- Zones: All enabled
- Duration: 90 minutes

**What Happens**:
- 6:30 AM: Heater auto-starts
- 6:35 AM: Cabin reaches 20°C
- 6:45 AM: Floor warm, water at 40°C
- 7:00 AM: Comfortable for shower
- 8:00 AM: Heater reduces to Eco mode for background heat

**User Experience**:
- Wake up to warm van
- Hot water ready for shower and coffee
- No need to interact with heater
- Automatic transition to efficient mode

### Example 3: Day Trip Return

**Remote Control via Phone**:
- 3:00 PM: Check van temperature (4°C inside)
- 3:05 PM: Send MQTT command to start heating
- 3:10 PM: Set target 22°C, Boost mode
- 4:00 PM: Arrive at van, already warm (20°C)

**Cost**:
- 1 hour pre-heating: ~0.5 liters diesel
- Total cost: ~$0.60
- Value: Step into warm van vs. waiting 30 min

---

## Cost Analysis

### Initial Investment

| Item | Cost (USD) |
|------|------------|
| Chinese diesel heater (5kW) | $250 |
| ESP32 board | $10 |
| Temperature sensors (6x DS18B20) | $15 |
| MOSFETs, relays, components | $30 |
| Wiring, connectors | $25 |
| 3-way valves (2x) | $60 |
| Installation materials | $50 |
| **Total** | **$440** |

### Ongoing Costs

**Fuel Consumption**:
- Eco mode (30%): 0.15 L/hr
- Normal mode (60%): 0.3 L/hr
- Boost mode (100%): 0.5 L/hr

**Typical Usage** (winter night + morning):
- 8 hours overnight (Eco): 1.2 L
- 2 hours morning (Normal): 0.6 L
- Total: 1.8 L/day
- Monthly (winter): ~54 liters
- Cost (@$1.20/L): ~$65/month

**vs. Alternatives**:
- Campground electric hookup: $30-50/night
- Propane heating: Similar cost, less convenient
- Idling vehicle: More fuel, engine wear, illegal in many areas

### Return on Investment

If replacing campground stays:
- 3 nights per month: Save $90-150
- System pays for itself: 3-5 months
- Plus: Freedom to camp anywhere

---

## Safety Features

### Multi-Layer Protection

1. **Overtemperature Shutdown**
   - Burning chamber > 900°C → Emergency stop
   - Floor surface > 50°C → Zone shutdown
   - Water tank > 85°C → Zone shutdown

2. **Fuel Safety**
   - Ignition timeout (2 minutes)
   - Automatic fuel cutoff on any error
   - Power-loss defaults to OFF

3. **Cooldown Enforcement**
   - Mandatory 60-second cooldown
   - Fans run at maximum
   - Prevents thermal shock

4. **Monitoring**
   - Continuous sensor validation
   - MQTT alerts on problems
   - Watchdog timer for controller health

5. **Manual Override**
   - Emergency stop always available
   - Manual mode overrides automation
   - Physical cutoff switches recommended

### Best Practices

- ✓ Install CO detector in van
- ✓ Ensure proper exhaust venting
- ✓ Regular maintenance (clean burner annually)
- ✓ Monitor fuel quality
- ✓ Keep fire extinguisher accessible
- ✓ Never leave unattended in first few uses
- ✓ Follow manufacturer installation guidelines

---

## Advantages Over Stock Controller

| Feature | Stock Controller | Our Controller |
|---------|------------------|----------------|
| Power Control | On/Off only | 20-100% variable |
| Noise Level | Always loud | Quiet Eco mode |
| Temperature Control | None | PID feedback control |
| Multi-Zone | No | Yes, 3 zones |
| Remote Control | No | WiFi/MQTT |
| Scheduling | No | 4 schedules, day-of-week |
| Fuel Efficiency | Poor (cycling) | Excellent (modulation) |
| Integration | None | Home Assistant |
| Monitoring | LEDs only | Full telemetry |
| Cost | Included | +$190 in parts |

---

## Future Enhancements (Roadmap)

### Phase 4: Advanced Features
- [ ] Web interface for configuration
- [ ] Fuel level monitoring
- [ ] Maintenance reminders
- [ ] Usage statistics and reporting
- [ ] Altitude compensation
- [ ] Battery voltage monitoring

### Phase 5: Smart Features
- [ ] Machine learning for predictive heating
- [ ] Weather forecast integration
- [ ] Geofencing (auto-start when approaching van)
- [ ] Voice control (Alexa/Google Home)
- [ ] Mobile app (iOS/Android)
- [ ] Multi-language support

---

## Getting Started

See the comprehensive documentation:

1. **[REQUIREMENTS_V2.md](REQUIREMENTS_V2.md)** - Full feature requirements
2. **[DESIGN.md](DESIGN.md)** - System architecture
3. **[WIRING.md](WIRING.md)** - Hardware installation
4. **[QUICKSTART.md](QUICKSTART.md)** - Setup guide
5. **[API.md](API.md)** - Developer reference

---

## Community and Support

This is an open-source project. Contributions welcome!

- Share your installation photos
- Report issues and improvements
- Contribute code enhancements
- Help others in discussions

**Happy camping in comfort!** 🚐🔥