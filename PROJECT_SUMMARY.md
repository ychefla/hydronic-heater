# Project Summary
## ESP32 Hydronic Diesel Heater Controller for Camper Vans

**Project Status**: Design and Core Implementation Complete  
**Last Updated**: December 2025

---

## Executive Summary

This project provides a complete replacement controller for cheap Chinese hydronic diesel heaters (2-5kW models), specifically designed for camper van applications. It transforms a basic on/off heater into an intelligent, multi-zone heating system with remote control, scheduled operation, and comprehensive safety features.

### Key Value Proposition

- **Cost**: $150-400 for Chinese heater + $190 in controller parts = **$340-590 total**
- **vs. Premium Heaters**: $2000-3000 for Webasto/Espar with similar capabilities
- **Savings**: $1400-2400 (70-80% cost reduction)
- **Payback**: 3-5 months if replacing campground electric hookups

---

## Documentation Structure

This project follows a **design-first approach**. Documents should be reviewed in this order:

### Phase 1: Requirements and Design
1. **[REQUIREMENTS.md](REQUIREMENTS.md)** - Original base system requirements
2. **[REQUIREMENTS_V2.md](REQUIREMENTS_V2.md)** - Extended camper van requirements
3. **[DESIGN.md](DESIGN.md)** - System architecture and design decisions
4. **[ARCHITECTURE.md](ARCHITECTURE.md)** - Visual architecture diagrams
5. **[SAFETY.md](SAFETY.md)** - ⚠️ **CRITICAL: Must read before building**

### Phase 2: Implementation
6. **[README.md](README.md)** - Project overview and features
7. **[WIRING.md](WIRING.md)** - Hardware installation guide
8. **[QUICKSTART.md](QUICKSTART.md)** - Quick setup guide
9. **[API.md](API.md)** - Developer API reference
10. **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem solving

### Phase 3: Application-Specific
11. **[CAMPERVAN_FEATURES.md](CAMPERVAN_FEATURES.md)** - Camper van specific features and use cases

---

## Implementation Status

### ✅ Completed Features

**Base Controller System**
- [x] ESP32-based hardware platform
- [x] Component control classes (glow plug, pump, fans)
- [x] DS18B20 temperature sensor integration
- [x] PWM control for variable power output
- [x] State machine for operational control
- [x] Serial command interface
- [x] PlatformIO build system
- [x] Comprehensive documentation

**Critical Safety Systems** ⚠️
- [x] Burning chamber overtemperature protection (>900°C)
- [x] Coolant overtemperature protection (>95°C, prevents boiling)
- [x] Fuel depletion detection (temperature pattern monitoring)
- [x] Critical sensor failure detection
- [x] Multi-level warning system (elevated → warning → critical)
- [x] Emergency shutdown procedure
- [x] Extended cooling on emergency stop

**Design Documentation**
- [x] Complete requirements specification
- [x] System architecture design
- [x] Component interaction diagrams
- [x] State machine diagrams
- [x] Safety analysis and procedures
- [x] 🆕 Paku-IoT cloud integration specification

### 🚧 In Progress (Headers/Framework Ready)

**Extended Features for Camper Van**
- [ ] WiFi connectivity with WiFiManager
- [ ] MQTT remote control integration
- [ ] Multi-zone heating control (floor, air, water)
- [ ] Power profiles (Eco, Normal, Boost)
- [ ] Scheduled heating with day-of-week support
- [ ] PID temperature feedback control
- [ ] Home Assistant integration
- [ ] Configuration storage in NVS

**Status**: All header files created, dependencies added, framework in place. Implementation of connectivity and zone control classes needed.

### 📋 Future Enhancements

**Phase 4: Advanced Features**
- [ ] Web configuration interface
- [ ] Fuel level monitoring
- [ ] Usage statistics and logging
- [ ] Maintenance reminders

**Phase 5: Smart Features**
- [ ] Machine learning for predictive heating
- [ ] Weather forecast integration
- [ ] Geofencing auto-start
- [ ] Mobile app (iOS/Android)

---

## Technical Highlights

### Hardware Platform
- **Microcontroller**: ESP32 (dual-core, WiFi, Bluetooth)
- **Voltage**: 3.3V logic, 12V/24V for components
- **Sensors**: DS18B20 digital temperature sensors (OneWire)
- **Outputs**: PWM for variable control, digital GPIO for on/off
- **Connectivity**: Built-in WiFi for MQTT

### Software Architecture
```
Application Layer (main.cpp)
    ↓
Controller Layer (state machine, safety, control)
    ↓
Component Layer (hardware abstraction)
    ↓
Hardware Abstraction Layer (Arduino/ESP32)
```

### Key Design Decisions

1. **ESP32 Platform**: Selected for built-in WiFi, multiple PWM channels, and strong Arduino support
2. **Component-Based Architecture**: Modular design for easy extension and testing
3. **State Machine Pattern**: Clear operational phases with predictable transitions
4. **Safety-First Design**: Multiple redundant safety checks, fail-safe defaults
5. **Design-First Approach**: Complete requirements and architecture before implementation

---

## Critical Safety Features ⚠️

### Three Mandatory Safety Systems

**1. Burning Chamber Overtemperature Protection**
- Monitors combustion temperature continuously
- Emergency shutdown at 900°C
- Warning at 850°C with power reduction
- Response time: <500ms

**2. Coolant Overtemperature Protection (Boiling Prevention)**
- Critical shutdown at 95°C (approaching 100°C boiling point)
- Warning at 85°C with 50% power reduction
- Elevated cooling at 75°C
- Prevents dangerous steam pressure buildup

**3. Fuel Depletion Detection**
- Monitors temperature drop patterns
- Detects fuel exhaustion within 30 seconds
- Extended purge cycle on shutdown
- Prevents pump damage and carbon buildup

### Emergency Shutdown Procedure

When any critical condition detected:
1. **Immediate** fuel cutoff (pump OFF, glow plug OFF)
2. **Maximum** cooling (all fans 100%, coolant pump ON)
3. Enter ERROR state with descriptive message
4. Extended cooling period (3+ minutes)
5. Prevent restart until manual intervention
6. Log event for analysis

**See [SAFETY.md](SAFETY.md) for complete safety documentation.**

---

## Target Application: Camper Van Heating

### Use Cases

**1. Floor Heating**
- Radiant warmth through underfloor loops
- Target: 25°C surface, max 40°C safety limit
- Comfortable for bare feet

**2. Air Heating**
- Heat exchanger with variable speed fans
- Quick cabin warming capability
- Target: 20°C cabin temperature

**3. Water Heating**
- Hot water for washing (hands, dishes, shower)
- Target: 45°C tank temperature, max 80°C safety
- On-demand or continuous

### Operating Modes

**Manual Mode**
- Direct on/off control
- Immediate response
- Overrides scheduling

**Continuous Mode**
- Maintains target temperature indefinitely
- PID feedback control
- Auto power adjustment (20-100%)
- Fuel-efficient operation

**Scheduled Mode**
- Up to 4 independent schedules
- Day-of-week selection
- Power profile per schedule
- Examples: morning warmup, night heating, pre-arrival

### Power Profiles

| Profile | Power | Fuel | Noise | Use Case |
|---------|-------|------|-------|----------|
| Eco | 30% | 0.15 L/hr | Very quiet | Overnight, maintenance |
| Normal | 60% | 0.30 L/hr | Moderate | General use, balanced |
| Boost | 100% | 0.50 L/hr | Louder | Quick warmup, extreme cold |

---

## Installation Overview

### Physical Components

**Heater Unit** (exterior mounting)
- Chinese 2-5kW hydronic diesel heater
- Exhaust vented safely outside
- Fuel line from diesel tank
- 12V power from van battery

**Controller** (interior mounting)
- ESP32 board in weatherproof enclosure
- 6x DS18B20 temperature sensors
- MOSFETs/relays for component control
- WiFi access from living area

**Distribution System**
- 3-way motorized valves for zone control
- Coolant circulation pumps
- Heat exchanger with fans
- Floor heating loops
- Water tank heating coil

### Typical Costs

| Component | Cost (USD) |
|-----------|------------|
| Chinese diesel heater (5kW) | $250 |
| ESP32 + components | $80 |
| Temperature sensors (6x) | $15 |
| Valves, pumps, hardware | $135 |
| Installation materials | $50 |
| **Total System** | **$530** |

**Compare to**: Webasto Air Top 2000 ($2000+) with basic control

---

## Control Options

### 1. Serial Interface (Built-in)
```
Commands:
  start   - Start heater
  stop    - Stop heater  
  status  - Print full status
```

### 2. MQTT/Home Assistant (Planned)
```yaml
# Example Home Assistant control
service: climate.set_temperature
target:
  entity_id: climate.camper_heater
data:
  temperature: 21
  hvac_mode: heat
```

### 3. Physical Controls (Optional)
- Emergency stop button (hardwired)
- Status indicators (LEDs)
- Temperature display (optional)

---

## Safety Compliance

### User Responsibilities

✓ Read and understand all safety documentation  
✓ Install per wiring diagrams and guidelines  
✓ Use proper wire gauges and fusing  
✓ Ensure adequate ventilation  
✓ Install CO detector (separate device)  
✓ Regular maintenance and inspection  
✓ Never disable safety features  
✓ Keep fire extinguisher accessible  
✓ Follow local codes and regulations  

### System Safety Features

✓ Multiple temperature monitoring points  
✓ Redundant safety checks (2 Hz)  
✓ Fail-safe design (power loss = OFF)  
✓ Hardware watchdog timer  
✓ Critical sensor validation  
✓ Emergency shutdown capability  
✓ Extended cooldown enforcement  
✓ Event logging for analysis  

---

## Development Workflow

### Setup Environment
```bash
# Install PlatformIO
pip install platformio

# Clone repository
git clone https://github.com/ychefla/hydronic-heater.git
cd hydronic-heater

# Build
pio run

# Upload to ESP32
pio run --target upload

# Monitor
pio device monitor
```

### File Structure
```
hydronic-heater/
├── platformio.ini              # Build configuration
├── include/
│   ├── config.h               # Pin assignments, thresholds
│   ├── components.h           # Component classes
│   ├── controller.h           # Main controller
│   └── connectivity.h         # WiFi/MQTT/zones
├── src/
│   ├── main.cpp              # Application entry point
│   ├── components.cpp        # Component implementations
│   └── controller.cpp        # Controller logic
└── [documentation files]
```

---

## Testing Strategy

### Phase 1: Component Testing (bench)
- [ ] Test each sensor individually
- [ ] Verify PWM output with oscilloscope
- [ ] Test MOSFETs with dummy loads
- [ ] Validate temperature readings

### Phase 2: Safety Testing (no fuel)
- [ ] Simulate overtemperature conditions
- [ ] Test emergency shutdown response times
- [ ] Verify sensor failure detection
- [ ] Test all warning thresholds

### Phase 3: Integrated Testing (with fuel, controlled)
- [ ] Full startup sequence
- [ ] Normal operation monitoring
- [ ] Scheduled heating
- [ ] Zone control
- [ ] Power profile switching

### Phase 4: Field Testing (actual use)
- [ ] Overnight operation
- [ ] Extended continuous use
- [ ] Various weather conditions
- [ ] Fuel consumption measurement
- [ ] MQTT connectivity reliability

---

## Known Limitations

### Current Implementation
- Connectivity features framework only (not yet implemented)
- Single OneWire bus (all sensors share one pin)
- No web interface (planned future)
- Limited to 4 schedules (extendable)
- No fuel level sensor integration (planned)

### Chinese Heater Limitations
- Inconsistent build quality across models
- May require calibration for specific heater
- Some noise/vibration at high power
- Limited manufacturer documentation
- Warranty implications of controller replacement

---

## Contributing

This is an open-source project. Contributions welcome!

**Ways to Contribute**:
- Report issues and bugs
- Share installation photos and experiences
- Improve documentation
- Add features (connectivity, zones, etc.)
- Test with different heater models
- Translate documentation

**Before Contributing**:
- Read existing documentation
- Follow the design-first approach
- Maintain safety-critical features
- Test thoroughly
- Document your changes

---

## License and Disclaimer

**License**: Open source (license to be determined)

**Disclaimer**:
- This is a DIY project - use at your own risk
- No warranty expressed or implied
- User assumes all liability
- Safety features provided as-is
- Professional installation recommended
- Follow all local codes and regulations
- Regular maintenance is mandatory

**Safety Warning**:
This system controls combustion equipment capable of causing fire, carbon monoxide poisoning, or burns if improperly installed or maintained. Only undertake this project if you have appropriate skills and accept all risks.

---

## Support and Community

**Documentation**: This repository  
**Issues**: GitHub issue tracker  
**Discussions**: GitHub discussions  
**Updates**: Watch repository for releases  

---

## Acknowledgments

- ESP32 community for excellent platform
- Arduino framework contributors
- PlatformIO team
- OneWire and DallasTemperature library authors
- Home Assistant community
- Camper van and tiny home communities

---

## Quick Reference

### Essential Commands
```bash
# Build and upload
pio run --target upload && pio device monitor

# Check status
# In serial monitor type: status

# Start heating
# In serial monitor type: start

# Emergency stop
# In serial monitor type: stop
```

### Critical Temperatures
- Burning chamber: Normal 600°C, Max 900°C
- Coolant: Normal 40-75°C, Critical 95°C
- Floor: Target 25°C, Max 45°C
- Water: Target 45°C, Max 80°C

### Emergency Procedures
1. Type `stop` in serial monitor OR
2. Cut 12V power to controller OR
3. Use physical emergency stop switch (if installed)

---

## Project Roadmap

**✅ Phase 1: Core System** (COMPLETE)
- Base controller with safety features
- Design documentation
- Component architecture

**🚧 Phase 2: Extended Features** (IN PROGRESS)
- Connectivity implementation
- Multi-zone control
- Scheduling

**📋 Phase 3: Advanced Features** (PLANNED)
- Web interface
- Analytics and logging
- Maintenance tracking

**💡 Phase 4: Smart Features** (FUTURE)
- Machine learning
- Mobile app
- Voice control

---

**Last Updated**: December 28, 2025  
**Version**: 1.0  
**Status**: Core implementation complete, extended features in development

**For questions, issues, or contributions, please use the GitHub repository.**

🚐 Stay warm, stay safe, happy camping! 🔥
