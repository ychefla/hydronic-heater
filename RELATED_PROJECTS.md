# Related Projects and Feature Suggestions

## Analysis of Existing Diesel Heater Controller Projects

**Date**: December 2025  
**Purpose**: Document existing projects for design inspiration and feature enhancement  
**Status**: ✅ REVISED based on project requirements and priorities

---

## ⚠️ Important: Feature Priorities Updated

This document has been revised to reflect the **Autoterm Flow 5D hybrid architecture** (February 2026).

**🔄 ARCHITECTURE PIVOT**:
- Heater changed from HCalory W51 to **Autoterm Flow 5D**
- Approach changed from full hardware replacement to **hybrid** (Autoterm handles combustion, ESP32 handles smart layer)
- UART protocol testing no longer needed (Autoterm provides documentation)
- See [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) for details

**✅ HIGH PRIORITY (Phase 2–3)**:
- AutotermUART communication class
- PID zone temperature control
- Modular Communication Layer (MQTT / Paku-IoT)
- Configuration Management (NVS)
- Multi-zone valve control
- Scheduling and power profiles

**⏳ FUTURE (Phase 4+)**:
- OLED Display + Keypad (via Paku-Core framework)
- OTA Updates (via Paku-IoT framework)
- Web configuration interface

**❌ NOT IMPLEMENTING**:
- ~~UART Protocol Reverse Engineering~~ (Autoterm provides documentation)
- ~~Full hardware replacement~~ (hybrid approach instead)
- Hardware RTC (use NTP)
- Fuel Consumption Tracking (Autoterm provides via UART)
- Bluetooth (web interface preferred)
- Home Assistant (Paku-IoT focus)
- Cyclic Temperature (not needed)
- Multi-Language (not needed)
- Other smart home integrations (not needed for now)

---

## Executive Summary

This document analyzes existing open-source diesel heater controller projects to identify proven features, design patterns, and enhancement opportunities for our implementation.

**Key Finding**: The **Afterburner** project by Ray Jones is the most mature and feature-complete open-source diesel heater controller, with 5+ years of development and active community support.

---

## 1. Major Related Projects

### 1.1 Afterburner Diesel Heater Controller ⭐⭐⭐⭐⭐

**Developer**: Ray Jones (mrjones.id.au)  
**Platform**: ESP32  
**Status**: Mature, actively maintained  
**License**: Open Source  
**Community**: Very active

**Project URLs**:
- Website: http://www.afterburner.mrjones.id.au/
- Documentation: http://www.afterburner.mrjones.id.au/features.html
- Community builds: Multiple forks and derivatives

#### Key Features

**Hardware & Interface**:
- ✅ 1.3" OLED display (128x64) with full graphics
- ✅ 5-button keypad for menu navigation
- ✅ Custom PCB design with professional enclosure
- ✅ Battery-backed RTC (DS3231) for timers
- ✅ Low-voltage cutout protection
- ✅ Compact design with rear mounting

**Communication**:
- ✅ WiFi access point with web server
- ✅ MQTT support (full remote control)
- ✅ Bluetooth SPP (with Android app)
- ✅ WebSocket for real-time updates
- ✅ 433MHz RF keyfob support
- ✅ JSON data encapsulation

**Control Features**:
- ✅ **Temperature-based regulation** (PID control)
- ✅ **Thermostat operation** with adjustable hysteresis
- ✅ **External thermostat** support via GPIO
- ✅ **Fan control** (PID and percentage modes)
- ✅ **Fuel pump calibration**
- ✅ **Altitude compensation**
- ✅ **Cyclic temperature** operation

**Safety**:
- ✅ Emergency stop thread
- ✅ Watchdog timers
- ✅ Overtemperature protection
- ✅ Low-voltage cutout
- ✅ Automatic restart after failure

**Timers & Automation**:
- ✅ **14 programmable timers** (start/stop/repeat)
- ✅ Weekly overview and scheduling
- ✅ One-shot and repeat modes
- ✅ Temperature profile timers
- ✅ Timer-driven GPIO actions

**Monitoring & Reporting**:
- ✅ Burn chamber temperature
- ✅ Fan speed (RPM)
- ✅ Pump rate
- ✅ Battery voltage
- ✅ Glow plug power
- ✅ Hour meters (glow plug, heater, system)
- ✅ **Estimated fuel consumption**
- ✅ **Auto-shutdown before fuel empty**

**Advanced**:
- ✅ Over-the-air (OTA) firmware updates
- ✅ Multi-language support
- ✅ Extensive configuration options
- ✅ Debug console via web/serial

#### What We Can Learn

**Adopted in Our Project**:
- ✅ Temperature-based control algorithms
- ✅ Safety watchdogs and protection
- ✅ Multiple communication protocols (we have MQTT)
- ✅ Timer-based scheduling
- ✅ Fuel consumption tracking concept

**Not Yet Implemented (Future Enhancements)**:
- ⏳ OLED display with keypad
- ⏳ RTC for accurate timers
- ⏳ Bluetooth support
- ⏳ OTA updates
- ⏳ Altitude compensation
- ⏳ Fuel level monitoring with auto-shutdown
- ⏳ Hour meters and usage statistics

---

### 1.2 esp32-diesel-heater by sgoodluck ⭐⭐⭐⭐

**Platform**: ESP32 with PlatformIO  
**Language**: C++ (modular architecture)  
**Status**: Active development  

**GitHub**: https://github.com/sgoodluck/esp32-diesel-heater

#### Key Features

**Architecture**:
- ✅ Modular C++ structure
- ✅ Separate modules for hardware, sensors, safety, networking
- ✅ PlatformIO build system (same as ours!)
- ✅ Well-documented code

**Features**:
- ✅ WiFi configuration portal
- ✅ MQTT integration
- ✅ Web server for configuration
- ✅ Multiple temperature sensors
- ✅ Safety monitoring
- ✅ Remote monitoring and control

**Documentation**:
- ✅ Step-by-step assembly instructions
- ✅ Wiring diagrams
- ✅ Configuration guide
- ✅ GitHub issues for community support

#### What We Can Learn

**Already Similar to Our Project**:
- ✅ Modular C++ architecture
- ✅ PlatformIO build system
- ✅ Safety-first design
- ✅ MQTT integration

**Different Approaches**:
- Web portal vs. our serial interface (both valid)
- Configuration portal approach (we use config.h)

---

### 1.3 esp32-universal-diesel-heater-controller by zorrobyte ⭐⭐⭐

**Platform**: ESP32  
**Language**: MicroPython  
**Status**: Alpha/untested  

**GitHub**: https://github.com/zorrobyte/esp32-universal-diesel-heater-controller

#### Key Features

- ✅ MicroPython implementation
- ✅ Standalone WiFi AP with web portal
- ✅ MQTT remote control
- ✅ Temperature-based regulation
- ✅ Safety mechanisms
- ✅ Simulator included

#### What We Can Learn

**Interesting Approach**:
- MicroPython for rapid development (vs. our C++)
- Built-in simulator (great for testing!)

**Status**:
- Marked as alpha/untested
- Good proof-of-concept
- Our C++ approach more production-ready

---

### 1.4 DieselHeaterRF by jakkik ⭐⭐⭐

**Platform**: ESP32/Arduino  
**Purpose**: 433MHz RF control library  

**GitHub**: https://github.com/jakkik/DieselHeaterRF

#### Key Features

- ✅ Control heaters via 433MHz RF
- ✅ Replicate stock remote signals
- ✅ ESPHome integration
- ✅ Home Assistant compatible

#### What We Can Learn

**Alternative Control Method**:
- Many cheap Chinese heaters come with RF remotes
- Could add RF control as alternative to direct hardware control
- Useful if replacing stock controller is too invasive

**Limitation**:
- Only controls via RF, doesn't read heater state
- Our direct hardware control is more comprehensive

---

### 1.5 UART Protocol Integration Projects ⭐⭐⭐⭐

**Multiple implementations** (cdh-esphome, Afterburner)

#### Key Features

**UART Communication**:
- ✅ 25,000 baud (non-standard)
- ✅ 48-byte frame protocol
- ✅ Bidirectional communication with heater
- ✅ Read real-time status (temp, fan, pump, errors)
- ✅ Send commands to heater

**cdh-esphome Project**:
- GitHub: https://github.com/daoudeddy/cdh-esphome
- ESPHome external component
- Full frame parsing
- Sensors for all heater parameters

#### What We Can Learn

**Major Enhancement Opportunity**:
- 🎯 **UART protocol support would be HUGE**
- Read actual heater parameters instead of inferring
- Send commands to stock heater controller
- Monitor errors and faults
- Less invasive than replacing entire controller

**Implementation Approach**:
```cpp
// Potential UART integration
class HeaterUART {
    void begin(int rxPin, int txPin, int baud = 25000);
    void update();  // Parse incoming frames
    
    // Read from heater
    float getActualChamberTemp();
    uint16_t getFanRPM();
    float getPumpFrequency();
    uint8_t getErrorCode();
    HeaterState getHeaterState();
    
    // Send to heater
    void setTargetTemp(float temp);
    void setPowerLevel(uint8_t level);
    void startHeater();
    void stopHeater();
};
```

**Benefits**:
- Can work WITH stock controller (less invasive)
- Get real sensor data from heater
- Detect actual faults and errors
- Safer integration

---

## 2. Feature Comparison Matrix

| Feature | Our Project | Afterburner | esp32-diesel-heater | UART Projects |
|---------|-------------|-------------|---------------------|---------------|
| **Platform** | ESP32 | ESP32 | ESP32 | ESP32 |
| **Language** | C++ | C++ | C++ | Various |
| **Build System** | PlatformIO | Arduino | PlatformIO | Various |
| **Display** | Serial | OLED+Keypad | Web | Web/App |
| **WiFi** | Framework | ✅ Yes | ✅ Yes | ✅ Yes |
| **MQTT** | Framework | ✅ Yes | ✅ Yes | ✅ Yes |
| **Bluetooth** | ❌ No | ✅ Yes | ❌ No | Some |
| **Safety Systems** | ✅ 5 Critical | ✅ Multiple | ✅ Yes | Limited |
| **Temperature Control** | ✅ Intelligent | ✅ PID | ✅ Basic | ✅ Read-only |
| **Timers** | Framework | ✅ 14 timers | ⏳ Planned | ❌ No |
| **Multi-Zone** | Framework | ❌ No | ❌ No | ❌ No |
| **Power Control** | ✅ Intelligent | ✅ PID | ✅ Basic | ✅ Via commands |
| **Fuel Tracking** | ⏳ Planned | ✅ Yes | ❌ No | ✅ Yes |
| **Documentation** | ✅ Extensive | ✅ Good | ✅ Good | ✅ Varies |
| **Production Ready** | ✅ Yes | ✅ Yes | ✅ Yes | ⏳ Alpha |
| **Community** | New | ✅ Large | ✅ Active | ✅ Growing |

---

## 3. Feature Suggestions Based on Research

### 3.1 CRITICAL FIRST STEP: UART Protocol Testing

#### A. UART Protocol Support 🔬🔬🔬 **PHASE 0 - DO THIS FIRST**
**Status**: **HIGHEST PRIORITY - Test immediately before further development**

**Why This Matters**:
- **If heater supports UART**: We can build on top of existing controller (hybrid approach)
- **If heater doesn't support UART**: We continue with full hardware replacement (current approach)
- **This decision determines the entire project direction**

**Current Situation**:
- ❌ **Unknown**: Whether your specific heater model supports UART protocol
- ❌ **Unknown**: If supported, can we adjust fuel injection & fan control adequately
- ⚠️ **Risk**: Building complete system only to discover UART would have worked better
- ⚠️ **Risk**: Or vice versa - wasting time on UART that doesn't work

**Testing Approach (Create Test Branch NOW)**:
1. **Create `test/uart-protocol` branch**
2. Connect ESP32 to heater's control board (find UART pins)
3. Test various baud rates (25000 is common for Chinese heaters)
4. Attempt to:
   - Read heater status/parameters
   - Send commands (power on/off, adjust power)
   - Monitor heater's internal sensors
   - Control fan speed
   - Control fuel pump frequency
5. Document what works and what doesn't
6. Evaluate control granularity (can we do 20-100% power?)

**Decision Tree**:
```
UART Test Results
├─ Full UART Support (read + write + adequate control)
│  └─ DECISION: Hybrid approach
│     - Use heater's controller for basic operation
│     - Add our safety monitoring on top
│     - Override commands when needed
│     - Simpler hardware (no pump/fan control needed)
│
├─ Partial UART Support (read only or limited control)
│  └─ DECISION: Validate-only approach
│     - Use our hardware control (current design)
│     - Read UART for validation
│     - Compare our sensors vs. heater's sensors
│     - Best of both worlds
│
└─ No UART Support or inadequate control
   └─ DECISION: Continue current approach (full hardware replacement)
      - Complete control via our hardware
      - Already designed and documented
      - Ready to implement
```

**Implementation Path (if UART works)**:
1. Add UART protocol parser (25000 baud typical)
2. Create `HeaterUART` class for communication
3. Support hybrid mode: our safety + heater's controller
4. Use heater's sensors as validation/backup
5. Override heater commands when safety limits exceeded

**Timeline**:
- **Phase 0 (NOW)**: 1-2 weeks UART testing on test branch
- **Decision Point**: Based on test results, choose approach
- **Phase 1+**: Continue with chosen approach

**Priority**: **🔬 ABSOLUTE HIGHEST - DO THIS BEFORE ANYTHING ELSE**  
**Effort**: 1-2 weeks testing + analysis  
**Value**: **CRITICAL - Determines entire project direction**

---

#### B. OLED Display + Keypad 🎯 FUTURE with Paku Integration
**Status**: Future development, must integrate with Paku ecosystem
- **Requirement**: Full integration with Paku-IoT and Paku-Core
- User interface should control everything via Paku platform
- Display should show Paku-managed data
- Not standalone - part of Paku ecosystem

**Integration Approach**:
- Use Paku-Core UI framework
- Display data from Paku-IoT cloud
- Consistent with other Paku devices
- Unified user experience

**Priority**: Low (future enhancement via Paku framework)
**Effort**: Medium (2-3 weeks with Paku integration)  
**Value**: High (when Paku-Core UI framework ready)

---

#### C. ~~Real-Time Clock (RTC)~~ ❌ NOT NEEDED
**Status**: WiFi/NTP sufficient
- System gets time from internet via WiFi
- RTC not needed for this application
- Simplifies hardware requirements
- Reduces cost

**Alternative**: NTP time sync over WiFi (already in Paku-IoT integration)

---

#### D. ~~Fuel Consumption Tracking~~ ❌ NOT NEEDED
**Status**: Not needed for main tank integration
- System draws from van's main diesel tank (large capacity)
- Cannot fully empty main tank
- Only need fuel flow detection (already implemented as SAFE-3)
- No tracking/estimation required

**Current Implementation**: ✅ Fuel depletion detection sufficient

---

### 3.2 Medium Priority Enhancements

#### E. OTA (Over-The-Air) Firmware Updates 🎯 via Paku Framework
**Status**: Use Paku-IoT/Paku-Core OTA framework
- **Requirement**: Implement using Paku-IoT OTA system
- Must integrate with Paku-Core update management
- Consistent with other Paku devices
- Centralized update management via cloud

**Implementation**:
- Use Paku-IoT OTA API
- Follow Paku firmware update protocols
- Version management via Paku-Core
- Rollback through Paku platform

**Priority**: Medium (implement after core Paku integration)
**Effort**: Low (3-5 days using Paku framework)  
**Value**: High (essential for deployed systems)

---

#### F. ~~Bluetooth Support (SPP or BLE)~~ ❌ NOT NEEDED
**Status**: Web server approach preferred
- Web interface works with any device (iOS, Android, laptops)
- No app development needed (especially important for iOS)
- Simpler to maintain
- Universal compatibility

**Alternative**: WiFi + web server (more practical for cross-platform)
**Priority**: None (not implementing)
**Note**: Main user doesn't have Android; iOS app development is complex

---

#### G. ~~Home Assistant Auto-Discovery~~ ❌ NOT NEEDED
**Status**: Paku-IoT integration sufficient
- Focus on Paku-IoT platform integration
- Home Assistant not primary use case
- Can be added later if needed
- Not blocking any core functionality

**Priority**: None (not needed for now)

---

### 3.3 Low Priority / Nice-to-Have

#### H. Altitude Compensation ✅ SHOULD BE INCLUDED
**Status**: Important for variable altitude use
- Adjust fuel/air mixture based on altitude
- Important for mountain/elevation changes
- Improves combustion efficiency at altitude
- Prevents incomplete combustion

**Implementation**:
```cpp
class AltitudeCompensation {
    float currentAltitude;  // meters
    float getCompensationFactor();  // 0.0-1.0
    void adjustPowerForAltitude(int& targetPower);
};
```

**Priority**: Medium-High (should be included)
**Effort**: Low-Medium (3-5 days with calibration)
**Value**: High (for camper van traveling to mountains)

#### I. ~~Cyclic Temperature Operation~~ ❌ NOT NEEDED
**Status**: Not needed for now
- Interesting concept but not required
- Can be implemented via Paku-IoT scheduling if needed later
- Simple timer extension (low complexity)

**Priority**: None (future consideration)

#### J. External GPIO Automation ✅ DIRECTLY RELATES TO HEAT EXCHANGER
**Status**: Important for heat exchanger fan control
- Control heat exchanger fan based on heater state/temperature
- GPIO output to trigger external fan relays
- Temperature-based fan speed control
- Essential for camper van air heating

**Implementation**:
```cpp
class ExternalGPIOController {
    void updateHeatExchangerFan(float coolantTemp, HeaterState state);
    void setFanSpeed(int speedPercent);  // PWM or stepped control
    bool isHeatExchangerReady();
};
```

**Priority**: High (directly needed for heat exchanger operation)
**Effort**: Low (2-3 days)
**Value**: High (core functionality for air heating)

#### K. ~~Multi-Language Support~~ ❌ NOT NEEDED
**Status**: Not needed
- English sufficient for this application
- Adds complexity without benefit
- Can be added later via Paku-Core if required

**Priority**: None

---

## 4. ~~Integration Opportunities~~ ❌ NOT NEEDED FOR NOW

**Status**: Focus on Paku-IoT integration only

The project will focus exclusively on Paku-IoT cloud platform integration. Other smart home integrations (Home Assistant, ESPHome, Node-RED) are not needed at this time and can be considered for future development if demand arises.

**Primary Integration**: Paku-IoT (see PAKU_INTEGRATION.md)

---

## 5. Architectural Improvements from Research

### 5.1 State Persistence ✅ UTILIZE
**Learning from Afterburner**:
- Save state to EEPROM/NVS (ESP32 non-volatile storage)
- Resume after power loss (safety state, operating mode)
- Remember user preferences (target temps, power profiles)
- Store runtime statistics (operating hours, error history)

**Implementation Priority**: **HIGH - Phase 1**

**Benefits**:
- Seamless recovery from power interruptions
- User preferences preserved
- Historical data for maintenance
- Safety state tracking

---

### 5.2 Modular Communication Layer ✅ UTILIZE
**Learning from multiple projects**:
- Abstract communication from control logic
- Support multiple interfaces simultaneously:
  - Serial (debugging, local control)
  - MQTT (Paku-IoT cloud)
  - Web server (local configuration)
  - Future: additional protocols as needed
- Clean separation of concerns
- Easy to add new interfaces

**Implementation Priority**: **HIGH - Phase 1**

**Already Good in Our Design**: ✅ Foundation exists, needs implementation

**Architecture**:
```cpp
class CommunicationInterface {
    virtual void sendStatus(HeaterStatus status) = 0;
    virtual void sendTemperature(TempData data) = 0;
    virtual void sendAlert(AlertData alert) = 0;
};

class SerialInterface : public CommunicationInterface { };
class MQTTInterface : public CommunicationInterface { };
class WebInterface : public CommunicationInterface { };
```

---

### 5.3 Configuration Management ✅ UTILIZE
**Best Practices**:
- Runtime configuration via web/MQTT (no recompile needed)
- Factory reset capability
- Configuration backup/restore via Paku-IoT
- Validation before applying (prevent invalid configs)
- Configuration versioning for OTA compatibility

**Implementation Priority**: **HIGH - Phase 1**

**Key Features**:
- Store config in NVS (persists across reboots)
- MQTT commands to update configuration
- Web interface for local configuration
- Validate all changes before applying
- Rollback on validation failure

**Example Configuration Items**:
- Temperature thresholds
- Safety limits
- Power profiles
- Sensor calibration
- Network settings
- Paku-IoT credentials

---

## 6. Recommended Next Steps (REVISED based on requirements)

### Phase 1: Core Architecture & Paku Integration (1-2 months) ✅ HIGH PRIORITY
1. **State Persistence** (NVS/EEPROM) - Save state, resume after power loss
2. **Modular Communication Layer** - Abstract interfaces (Serial, MQTT, Web)
3. **Configuration Management** - Runtime config via MQTT, validation, backup/restore
4. **Paku-IoT connectivity** (WiFi, MQTT with TLS, device registration)
5. **Web server** for local configuration (WiFi captive portal)

### Phase 2: Essential Features (1 month) ✅ HIGH PRIORITY
6. **Altitude compensation** - Adjust for elevation changes
7. **External GPIO automation** - Heat exchanger fan control
8. **Scheduler implementation** - Timed heating with NTP time sync
9. **Paku-IoT OTA updates** - Using Paku framework

### Phase 3: Experimental Testing (2-3 weeks) 🧪 TEST BRANCH
10. **UART protocol support** - Create test branch to verify if heater supports it
    - Test if heater responds to UART
    - Verify power/fan control granularity
    - Decision: hybrid approach or continue direct control

### Phase 4: Future Enhancements (TBD)
11. **OLED display + keypad** - Via Paku-Core UI framework (when ready)
12. **Extended Paku-IoT features** - Advanced analytics, predictive maintenance
13. **Multiple heater model support**
14. **Community contributions**

### ❌ Not Implementing:
- RTC hardware (use NTP over WiFi)
- Fuel consumption tracking (fuel depletion detection sufficient)
- Bluetooth support (web interface preferred)
- Home Assistant auto-discovery (Paku-IoT focus)
- Cyclic temperature operation (not needed)
- Multi-language support (not needed)

---

## 7. Conclusions (REVISED)

### Our Project's Strengths
- ✅ **Safety-first design** (5 critical systems - most comprehensive)
- ✅ **Intelligent power control** (unique multi-factor approach)
- ✅ **Excellent documentation** (17 files, design-first)
- ✅ **Production-ready core** (tested control logic)
- ✅ **Modular architecture** (easy to extend)
- ✅ **Modern build system** (PlatformIO)
- ✅ **Paku-IoT integration** (complete specification)

### Priority Enhancements (Based on Requirements)
- 🎯 **State persistence** - NVS storage (HIGH - Phase 1)
- 🎯 **Modular communication** - Multiple interfaces (HIGH - Phase 1)
- 🎯 **Configuration management** - Runtime config (HIGH - Phase 1)
- 🎯 **Paku-IoT connectivity** - Full implementation (HIGH - Phase 1)
- 🎯 **Altitude compensation** - Essential for camper van (HIGH - Phase 2)
- 🎯 **Heat exchanger control** - GPIO automation (HIGH - Phase 2)
- 🧪 **UART protocol testing** - Experimental test branch (Phase 3)
- ⏳ **OLED display** - Future via Paku-Core framework (Phase 4)
- ⏳ **OTA updates** - Via Paku-IoT framework (Phase 2-3)

### Features Explicitly Not Needed
- ❌ Hardware RTC (use NTP over WiFi)
- ❌ Fuel consumption tracking (depletion detection sufficient)
- ❌ Bluetooth (web interface preferred, iOS app complex)
- ❌ Home Assistant integration (Paku-IoT focus)
- ❌ Cyclic temperature (not needed)
- ❌ Multi-language (not needed)

### Competitive Positioning

**vs. Afterburner**:
- ➕ Better safety systems (5 vs 3-4)
- ➕ More intelligent power control
- ➕ Better documentation
- ➕ Multi-zone framework
- ➕ Modern cloud integration (Paku-IoT)
- ➖ No display/keypad yet (future via Paku-Core)
- ➖ No timers implemented yet (Phase 1-2)
- ➖ Smaller community (new project)

**vs. Others**:
- ➕ Most comprehensive safety
- ➕ Best documentation
- ➕ Most modern architecture
- ➕ Intelligent thermostat
- ➕ Production-ready core
- ➕ Cloud-native design (Paku-IoT)
- ➕ Camper van multi-zone focus
- ➖ Fewer connectivity options currently (intentional - focused approach)

### Unique Innovations in Our Project
1. **Five-layer safety system** (most comprehensive)
2. **Multi-factor intelligent power control** (chamber + coolant + room)
3. **Design-first approach** (requirements → design → implementation)
4. **Extensive documentation** (17 files, ~170KB)
5. **Camper van multi-zone focus** (floor, air, water)
6. **Paku-IoT cloud integration** (modern IoT platform)
7. **Focused feature set** (no feature bloat, intentional choices)

### Development Philosophy
- ✅ Safety first, always
- ✅ Design before implementation
- ✅ Comprehensive documentation
- ✅ Modular, extensible architecture
- ✅ Production-ready core before extras
- ✅ Cloud-native with Paku-IoT
- ✅ Feature decisions based on actual use case
- ✅ No unnecessary complexity

---

## 8. References

**Projects**:
1. Afterburner: http://www.afterburner.mrjones.id.au/
2. esp32-diesel-heater: https://github.com/sgoodluck/esp32-diesel-heater
3. esp32-universal-diesel-heater: https://github.com/zorrobyte/esp32-universal-diesel-heater-controller
4. cdh-esphome: https://github.com/daoudeddy/cdh-esphome
5. DieselHeaterRF: https://github.com/jakkik/DieselHeaterRF

**Communities**:
- Home Assistant Community: Diesel heater discussions
- ESP32 Forum: Various diesel heater projects
- YouTube: Multiple build tutorials
- Hackaday: Project features and reviews

**Technical**:
- ESP32 Documentation: https://docs.espressif.com/
- MQTT Protocol: https://mqtt.org/
- Home Assistant: https://www.home-assistant.io/

---

**Last Updated**: December 28, 2025 (Revised based on project requirements)  
**Next Review**: After Phase 1 implementation (State Persistence, Communication, Config Management)  
**Status**: Requirements-Driven Feature Prioritization Complete