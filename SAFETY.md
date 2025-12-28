# Critical Safety Requirements
## Hydronic Diesel Heater Controller

**Version:** 5.0 - Corrected Temperature Safety System  
**Date:** December 2025  
**Priority:** CRITICAL

> **⚠️ CRITICAL UPDATE:** Temperature limits have been corrected based on actual heater specifications.
> The burning chamber operates at **130-230°C** (NOT 600-900°C as previously documented).

---

## 1. Overview

This document specifies **mandatory safety features** that MUST be implemented to prevent equipment damage, fire hazards, and safety incidents.

### Complete Safety System Summary

| Safety System | Trigger Condition | Response Time | Action |
|---------------|-------------------|---------------|--------|
| **SAFE-1A: Chamber Overheat** | ≥230°C | <500ms | Emergency shutdown |
| **SAFE-1B: Rapid Temp Spike** | >50°C in 5s | Immediate | Emergency shutdown |
| **SAFE-1C: Low Chamber Temp** | <130°C | None | Maintain min power |
| **SAFE-2: Coolant Overheat** | >95°C (boiling prevention) | <500ms | Emergency shutdown |
| **SAFE-3: Fuel Depletion** | Temp drop pattern | <30 seconds | Safe shutdown + purge |
| **SAFE-4: Pump Failure** | Pump not running with heat | Immediate | Emergency shutdown |
| **SAFE-5: Sensor Failure** | Invalid readings | <2 seconds | Emergency shutdown |

All systems are **CRITICAL** and **MANDATORY** for safe operation.

---

## 2. Critical Temperature Specifications

### ⚠️ CORRECTED BURNING CHAMBER TEMPERATURES

**Operating Range**: 130°C - 230°C (measured at chamber sensor)

| Temperature | Condition | Action |
|------------|-----------|--------|
| <80°C | Cold/Startup | Glow plug heating |
| 80°C | Ignition detected | Start fuel delivery |
| <130°C | Below clean burn threshold | **Maintain minimum power** - DO NOT reduce |
| 130°C | **Minimum for clean burning** | Safe lower limit |
| 180°C | Target operating temperature | Optimal efficiency |
| 220°C | Approaching maximum | Reduce to minimum power |
| **230°C** | **MAXIMUM SAFE TEMPERATURE** | **EMERGENCY SHUTDOWN** |

**CRITICAL NOTE**: These are actual chamber temperatures measured by DS18B20 sensor, NOT exhaust gas temperatures which are much higher.

---

## 3. Critical Safety Requirements

### SAFE-1A: Burning Chamber Maximum Temperature Protection

**Priority**: CRITICAL - MANDATORY

**Requirements**:
- SAFE-1A.1: System SHALL continuously monitor burning chamber temperature
- SAFE-1A.2: System SHALL trigger emergency shutdown if temperature ≥ **230°C**
- SAFE-1A.3: System SHALL trigger warning if temperature ≥ 220°C (approaching limit)
- SAFE-1A.4: System SHALL check temperature at minimum 2 Hz (every 500ms)
- SAFE-1A.5: Emergency shutdown SHALL complete within 500ms of detection
- SAFE-1A.6: System SHALL NOT allow restart until temperature drops below 150°C
- SAFE-1A.7: System SHALL log overtemperature events with timestamp

**Shutdown Sequence on Maximum Temperature**:
1. Immediately cut fuel delivery (diesel pump OFF)
2. Immediately disable glow plug
3. Set air fan to MAXIMUM speed (cooling)
4. Set heat exchanger fan to MAXIMUM speed
5. Maintain coolant pump ON
6. Enter ERROR state with message "CHAMBER OVERHEAT"
7. Sound alarm if available
8. Send MQTT alert (if connected)

**Rationale**: 
- Prevents fire hazard from overheated combustion chamber
- Prevents damage to heater components
- Maximum safe operating temperature

**Testing**:
- Simulate 230°C+ condition
- Verify shutdown completes in < 500ms
- Verify all fuel cutoff mechanisms activate
- Verify fans activate at maximum

---

### SAFE-1B: Rapid Temperature Spike Detection

**Priority**: CRITICAL - MANDATORY

**NEW REQUIREMENT**: Rapid temperature increases indicate dry run or coolant circulation failure.

**Requirements**:
- SAFE-1B.1: System SHALL monitor temperature change rate every 5 seconds
- SAFE-1B.2: System SHALL trigger emergency shutdown if temp increases >50°C in 5 seconds
- SAFE-1B.3: Rapid spike indicates dry run or coolant problem
- SAFE-1B.4: Response time: IMMEDIATE (within current check cycle)

**Detection Logic**:
```cpp
// Check every 5 seconds during RUNNING state
if (currentTime - lastSpikeCheck >= 5000) {
    float tempChange = currentTemp - lastTemp;
    if (tempChange > 50.0) {  // Rapid spike
        emergencyShutdown("RAPID TEMP SPIKE");
    }
}
```

**Possible Causes**:
- Coolant pump failure (not circulating heat away)
- Coolant flow blockage
- Air in coolant system (dry run)
- Heat exchanger fan failure
- Sensor placement issue (direct flame contact)

**Shutdown Sequence**:
1. Immediately cut fuel delivery
2. Maximum cooling activation
3. ERROR state with "RAPID TEMP SPIKE"
4. Log event for diagnostics

**Rationale**: 
- Prevents catastrophic overheating from circulation failure
- Early detection of dry run conditions
- Protects against coolant system failures

---

### SAFE-1C: Minimum Operating Temperature Maintenance

**Priority**: CRITICAL - MANDATORY

**NEW REQUIREMENT**: Chamber temperature must stay above 130°C for clean burning.

**Requirements**:
- SAFE-1C.1: System SHALL NOT reduce power below minimum if temp < 130°C
- SAFE-1C.2: Below 130°C indicates incomplete combustion (sooting, inefficiency)
- SAFE-1C.3: Power MUST be maintained at minimum stable level (20%)
- SAFE-1C.4: System SHALL warn if temperature drops below 130°C during operation

**Power Control Logic**:
```cpp
if (chamberTemp < 130.0) {
    // DO NOT reduce power - maintain minimum for clean burn
    return POWER_MIN_STABLE;  // 20%
}
```

**Rationale**: 
- Below 130°C: Incomplete combustion produces soot, carbon buildup
- Reducing power further makes problem worse
- Must maintain minimum heat for clean burning
- Trade-off: Accept minimum power even if other factors suggest reduction

**Warning Indicators**:
- Log warning if temperature drops below 130°C
- Continue operation but monitor closely
- Check fuel quality, air supply, combustion chamber condition

---

### SAFE-2: Coolant Overtemperature Protection (Boiling Prevention)

**Priority**: CRITICAL - MANDATORY

**Requirements**:
- SAFE-2.1: System SHALL detect fuel depletion condition
- SAFE-2.2: System SHALL trigger safe shutdown if fuel depleted
- SAFE-2.3: System SHALL detect fuel depletion within 30 seconds
- SAFE-2.4: System SHALL prevent damage from running without fuel

**Detection Methods** (implement at least 2 for redundancy):

**Method 1: Temperature Drop Detection** (Primary)
```cpp
// CORRECTED: With actual chamber temps (130-230°C range)
// If burning chamber temperature drops rapidly while pump running
if (dieselPump->isRunning() && 
    currentState == RUNNING &&
    chamberTemp < 100 &&                    // Below 100°C (should be 130-230°C)
    chamberTemp < lastChamberTemp &&        // Temperature dropping
    timeSinceIgnition > 60000) {            // After initial startup
    // Likely fuel depletion
    fuelDepletionCounter++;
    if (fuelDepletionCounter >= 3) {        // 3 checks = 15 seconds
        triggerFuelEmptyShutdown();
    }
}
```
}
```

**Method 2: Ignition Failure Pattern** (Secondary)
```cpp
// If repeatedly fails to ignite
if (ignitionAttempts > 3 && 
    chamberTemp < IGNITION_TEMP) {
    // Possible fuel depletion
    triggerFuelEmptyShutdown();
}
```

**Method 3: Fuel Level Sensor** (Optional Hardware)
```cpp
// If fuel level sensor available
if (fuelLevelSensor->getLevel() < FUEL_LEVEL_CRITICAL) {
    // Pre-emptive warning
    sendFuelLowWarning();
}
if (fuelLevelSensor->getLevel() == 0) {
    triggerFuelEmptyShutdown();
}
```

**Method 4: Running Time with Calculated Consumption** (Tertiary)
```cpp
// Track estimated fuel consumption
float estimatedFuelUsed = calculateFuelConsumption();
if (estimatedFuelUsed > FUEL_TANK_CAPACITY * 0.95) {
    sendFuelLowWarning();
}
```

**Shutdown Sequence on Fuel Depletion**:
1. Stop diesel pump immediately
2. Disable glow plug
3. Run air fan at HIGH speed to purge chamber
4. Maintain fans for 120 seconds (extended purge)
5. Enter ERROR state with message "FUEL EMPTY"
6. Prevent restart until manual reset
7. Send MQTT alert

**Warning Sequence** (if fuel low but not empty):
1. Send MQTT warning "FUEL LOW"
2. Reduce power to Eco mode (extend remaining fuel)
3. Flash status LED (if available)
4. Continue operation with monitoring

**Rationale**:
- Running without fuel can damage fuel pump
- Can cause incomplete combustion and carbon buildup
- Prevents dangerous situations
- Early warning allows time to refuel

---

### SAFE-3: Coolant Overtemperature Protection (Boiling Prevention)

**Priority**: CRITICAL - MANDATORY

**Requirements**:
- SAFE-3.1: System SHALL monitor coolant temperature continuously
- SAFE-3.2: System SHALL trigger emergency shutdown if coolant > 95°C (approaching boiling at 100°C)
- SAFE-3.3: System SHALL trigger warning if coolant > 85°C
- SAFE-3.4: System SHALL increase cooling effort if coolant > 75°C
- SAFE-3.5: System SHALL prevent restart until coolant < 60°C
- SAFE-3.6: System SHALL check coolant temperature at minimum 2 Hz

**Temperature Thresholds**:
```cpp
#define COOLANT_NORMAL_MAX 75.0      // Normal upper limit
#define COOLANT_WARNING_TEMP 85.0    // Warning threshold
#define COOLANT_CRITICAL_TEMP 95.0   // Emergency shutdown
#define COOLANT_BOILING_POINT 100.0  // Physical limit (varies with pressure)
```

**Action Levels**:

**Level 1: Normal (< 75°C)**
- Normal operation
- No special action

**Level 2: Elevated (75-85°C)**
```cpp
if (coolantTemp > 75.0 && coolantTemp < 85.0) {
    // Increase cooling
    heatExchangerFan->setSpeed(255);      // Maximum fan
    coolantPump->turnOn();                // Ensure circulation
    // Reduce heat input
    reducePowerLevel(10);                 // Reduce by 10%
    logEvent("Coolant temperature elevated");
}
```

**Level 3: Warning (85-95°C)**
```cpp
if (coolantTemp > 85.0 && coolantTemp < 95.0) {
    // Aggressive cooling
    heatExchangerFan->setSpeed(255);      // Maximum fan
    coolantPump->turnOn();                // Force circulation
    // Significantly reduce heat
    reducePowerLevel(50);                 // Reduce by 50%
    // Alert user
    sendMqttWarning("COOLANT HIGH TEMP");
    logEvent("Coolant temperature high - reducing power");
}
```

**Level 4: Critical (≥ 95°C)**
```cpp
if (coolantTemp >= 95.0) {
    triggerCoolantOverheatShutdown();
}
```

**Shutdown Sequence on Coolant Overheat**:
1. **Immediate**: Stop diesel pump (cut fuel)
2. **Immediate**: Disable glow plug
3. **Immediate**: Set air fan to MAXIMUM
4. **Immediate**: Set heat exchanger fan to MAXIMUM
5. **Immediate**: Ensure coolant pump is ON
6. Maintain cooling for minimum 180 seconds (3 minutes)
7. Enter ERROR state with message "COOLANT OVERHEAT"
8. Monitor coolant temperature drop
9. Once coolant < 60°C, allow manual reset
10. Send MQTT critical alert

**Additional Protection**:
```cpp
// Pressure relief valve recommendation
// If system has pressure sensor
if (coolantPressure > MAX_SAFE_PRESSURE) {
    triggerCoolantOverheatShutdown();
}

// Flow verification
if (coolantFlowRate < MIN_FLOW_RATE && coolantPump->isRunning()) {
    // Possible blockage or pump failure
    logWarning("Low coolant flow detected");
    sendMqttWarning("LOW COOLANT FLOW");
}
```

**Rationale**:
- Boiling coolant creates steam pockets (vapor lock)
- Steam pockets prevent proper heat transfer
- Can cause rapid overheating and damage
- Pressure buildup can rupture hoses/components
- Scalding hazard if coolant sprays
- Prevents catastrophic failure

---

### SAFE-4: Coolant Pump Failure Detection

**Priority**: CRITICAL - MANDATORY

**Requirements**:
- SAFE-4.1: System SHALL verify coolant pump is running when heater generates heat
- SAFE-4.2: System SHALL trigger emergency shutdown if pump fails during operation
- SAFE-4.3: System SHALL check pump status continuously when chamber temp > 80°C (ignition temp)
- SAFE-4.4: System SHALL verify actual coolant flow if flow sensor available
- SAFE-4.5: System SHALL prevent heater start if pump cannot be verified
- SAFE-4.6: System SHALL NOT allow restart until pump operation confirmed

**Detection Methods**:

**Method 1: Pump State Verification** (Primary)
```cpp
// When heater is generating heat, pump MUST be running
if ((currentState == RUNNING || currentState == IGNITION) && 
    chamberTemp > IGNITION_TEMP) {
    
    if (!coolantPump->isRunning()) {
        emergencyShutdown("COOLANT PUMP FAILURE");
    }
}
```

**Method 2: Flow Rate Verification** (if flow sensor installed)
```cpp
// Verify actual coolant movement
if (flowSensor && coolantPump->isRunning()) {
    float flowRate = flowSensor->getFlowRate();
    if (flowRate < 0.5) {  // Less than 0.5 L/min
        emergencyShutdown("NO COOLANT FLOW");
    }
}
```

**Failure Scenarios Detected**:
- Pump electrically failed (not turning)
- Pump mechanical failure (turning but not pumping)
- Blocked coolant lines (no flow despite pump running)
- Air lock in coolant system
- Broken pump impeller
- Electrical connection failure

**Shutdown Sequence on Pump Failure**:
1. **Immediate**: Stop fuel delivery (diesel pump OFF)
2. **Immediate**: Disable glow plug
3. **Immediate**: Set all cooling fans to MAXIMUM
4. Attempt to restart coolant pump (may clear air lock)
5. If pump still not running: maintain emergency state
6. Enter ERROR state with message "COOLANT PUMP FAILURE"
7. Log event with temperatures at time of failure
8. Send MQTT critical alert

**Rationale**:
- Without coolant circulation, heater will overheat rapidly
- Can cause local boiling even if overall temp seems OK
- Damage to heat exchanger, heater core
- Fire risk from overheated components
- Pump failure is a common failure mode

**Testing**:
- Disconnect pump power during operation
- Verify immediate shutdown
- Simulate blockage (if flow sensor available)
- Test with air in coolant lines

---

### SAFE-5: Temperature Sensor Failure Detection

**Priority**: CRITICAL - MANDATORY

**Requirements**:
- SAFE-5.1: System SHALL continuously validate all critical sensor readings
- SAFE-5.2: System SHALL trigger emergency shutdown if burning chamber sensor fails
- SAFE-5.3: System SHALL trigger emergency shutdown if ALL coolant sensors fail
- SAFE-5.4: System SHALL detect physically impossible temperature changes
- SAFE-5.5: System SHALL detect sensor disconnection within 2 seconds
- SAFE-5.6: System SHALL validate sensor readings are within physical limits
- SAFE-5.7: System SHALL NOT allow operation without critical sensors

**Critical Sensors** (System CANNOT operate without):
1. **Burning Chamber Temperature** - Absolutely mandatory
   - Primary safety indicator
   - No operation possible without this
   - Immediate shutdown if fails

2. **Coolant Temperature** (at least one sensor)
   - Need at least ONE of: input OR output
   - Redundancy preferred but not required
   - Cannot prevent boiling without this

**Sensor Validation Checks**:

**Check 1: Reading Within Physical Limits**
```cpp
// DS18B20 valid range: -55°C to +125°C
if (temp < -55.0 || temp > 125.0) {
    sensorFailed = true;
}

// Check for DS18B20 error codes
if (temp == -127.0 || temp == 85.0) {
    sensorFailed = true;  // Disconnected or read error
}
```

**Check 2: Rate of Change Validation**
```cpp
// CORRECTED: Physical limit with actual chamber temps (130-230°C range)
// Heater can't change >20°C per second (realistic for this temperature range)
float tempChange = abs(currentTemp - lastTemp);
float timeSeconds = timeDelta / 1000.0;
float maxChange = timeSeconds * 20.0;  // 20°C/s max

if (tempChange > maxChange) {
    // Impossible change - sensor malfunction
    emergencyShutdown("SENSOR MALFUNCTION");
}
```

**Check 3: Sensor Communication**
```cpp
// Verify sensor responds to read requests
if (!sensor->isValidReading()) {
    // Sensor not responding or disconnected
    emergencyShutdown("SENSOR FAIL");
}
```

**Failure Response by Sensor**:

```cpp
// BURNING CHAMBER: Immediate shutdown (any operating state)
if (!burningChamberTemp->isValidReading() && currentState != OFF) {
    emergencyShutdown("CHAMBER SENSOR FAIL");
}

// COOLANT: Shutdown if ALL sensors failed (during operation)
if ((currentState == RUNNING || currentState == IGNITION) &&
    !coolantOutputTemp->isValidReading() && 
    !coolantInputTemp->isValidReading()) {
    emergencyShutdown("COOLANT SENSOR FAIL");
}

// COOLANT: Warning if only ONE sensor failed (redundancy)
if (!coolantOutputTemp->isValidReading() && 
    coolantInputTemp->isValidReading()) {
    logWarning("Coolant output sensor failed - using input sensor");
    // Continue operation with remaining sensor
}

// AIR TEMP: Warning only (non-critical)
if (!airTemp->isValidReading()) {
    logWarning("Air sensor failed - degraded mode");
    // Continue operation
}
```

**Shutdown Sequence on Sensor Failure**:
1. Log which sensor(s) failed
2. Log last valid readings from all sensors
3. Immediate fuel cutoff
4. Maximum cooling
5. Enter ERROR state with descriptive message
6. Display sensor failure information
7. Prevent restart until sensor replaced/fixed
8. User must verify sensor function before reset

**Rationale**:
- Cannot safely operate "blind" without critical sensors
- Sensor failure could mask dangerous conditions
- False readings more dangerous than no readings
- Early detection prevents operating with bad data
- Better to shut down safely than risk catastrophic failure

**Common Sensor Failure Modes**:
- Wire disconnection (most common)
- Corroded connections
- Water ingress in sensor
- Physical sensor damage from heat/vibration
- Poor quality sensors failing prematurely
- Short circuit in wiring

**Testing**:
- Disconnect each critical sensor during operation
- Verify immediate shutdown
- Test with intermittent connection (loose wire)
- Verify error messages are accurate
- Test redundancy (one coolant sensor failing)

---

## 3. Multiple Sensor Redundancy

### Temperature Sensor Validation

**Requirements**:
```cpp
// Validate all critical temperature readings
bool validateTemperature(float temp, float lastTemp, unsigned long timeDelta) {
    // Check if reading is within physical limits
    if (temp < -55.0 || temp > 125.0) {
        return false;  // DS18B20 cannot read outside this range
    }
    
    // Check for sudden unrealistic jumps
    float maxChange = (timeDelta / 1000.0) * 50.0;  // Max 50°C per second
    if (abs(temp - lastTemp) > maxChange) {
        return false;  // Physically impossible change rate
    }
    
    // Check for sensor disconnection signature
    if (temp == -127.0 || temp == 85.0) {
        return false;  // DS18B20 error codes
    }
    
    return true;
}
```

### Sensor Failure Handling

**Critical Sensors** (MUST work for operation):
- Burning chamber temperature
- Coolant temperature (at least one point)

**Non-Critical Sensors** (warnings only):
- Air temperature
- Floor temperature
- Water temperature

**Failure Response**:
```cpp
if (!burningChamberTemp->isValidReading()) {
    // CRITICAL: Cannot operate without this sensor
    triggerSensorFailureShutdown("BURNING CHAMBER SENSOR FAIL");
}

if (!coolantOutputTemp->isValidReading() && 
    !coolantInputTemp->isValidReading()) {
    // CRITICAL: Cannot operate without coolant monitoring
    triggerSensorFailureShutdown("COOLANT SENSOR FAIL");
}

if (!airTemp->isValidReading()) {
    // WARNING: Can continue but log warning
    logWarning("Air temperature sensor failure - continuing in degraded mode");
}
```

---

## 4. Fail-Safe Design Principles

### Hardware Fail-Safe

1. **Power Loss**
   - All outputs default to OFF state
   - No fuel delivery possible
   - Gravity/spring returns for valves

2. **ESP32 Crash/Reset**
   - Hardware watchdog timer
   - All PWM stops → components OFF
   - System reboots to safe OFF state

3. **Wiring Fault**
   - Broken wire to fuel pump → pump OFF (fail-safe)
   - Broken wire to sensor → detected as invalid reading
   - Short circuit → fuse protection

### Software Fail-Safe

1. **State Machine Safety**
   ```cpp
   // Always check state validity
   void updateState() {
       // Reset if in invalid state
       if (currentState < OFF || currentState > ERROR) {
           currentState = ERROR;
           stopAllOutputs();
       }
       
       // Timeout protection
       if (millis() - stateStartTime > MAX_STATE_DURATION) {
           logError("State timeout - forcing shutdown");
           emergencyShutdown("STATE TIMEOUT");
       }
   }
   ```

2. **Watchdog Timer**
   ```cpp
   void setup() {
       // Enable hardware watchdog (8 seconds)
       esp_task_wdt_init(8, true);
       esp_task_wdt_add(NULL);
   }
   
   void loop() {
       // Reset watchdog every loop
       esp_task_wdt_reset();
       
       // If loop() hangs, ESP32 will reset
   }
   ```

3. **Sanity Checks**
   ```cpp
   void checkSystemSanity() {
       // Diesel pump should never run with glow plug off in startup
       if (currentState == GLOW_PLUG_WARMUP && dieselPump->isRunning()) {
           emergencyShutdown("INVALID STATE: Pump on during warmup");
       }
       
       // Heater should never run with both temps invalid
       if (currentState == RUNNING && 
           !burningChamberTemp->isValidReading() &&
           !coolantOutputTemp->isValidReading()) {
           emergencyShutdown("SENSOR FAILURE");
       }
   }
   ```

---

## 5. Emergency Shutdown Procedure

### Unified Emergency Shutdown Function

```cpp
void emergencyShutdown(String reason) {
    // Log immediately
    logCritical("EMERGENCY SHUTDOWN: " + reason);
    
    // 1. IMMEDIATE FUEL CUTOFF
    dieselPump->turnOff();
    glowPlug->turnOff();
    
    // 2. MAXIMUM COOLING
    airFan->setSpeed(255);
    heatExchangerFan->setSpeed(255);
    coolantPump->turnOn();
    
    // 3. CLOSE ZONE VALVES (prevent heat distribution)
    if (floorZone) floorZone->closeValve();
    if (waterZone) waterZone->closeValve();
    
    // 4. UPDATE STATE
    currentState = ERROR;
    errorMessage = reason;
    errorTimestamp = millis();
    
    // 5. ALERT USER
    sendMqttAlert("EMERGENCY_SHUTDOWN", reason);
    soundAlarm();  // If available
    
    // 6. LOG TO NON-VOLATILE STORAGE
    saveErrorToNVS(reason, millis());
    
    // 7. INCREMENT SAFETY COUNTER
    safetyShutdownCount++;
    if (safetyShutdownCount > 3) {
        // Repeated safety shutdowns - serious problem
        permanentLockout = true;
        sendMqttAlert("REPEATED_SAFETY_SHUTDOWNS", 
                      String(safetyShutdownCount));
    }
}
```

---

## 6. Safety Status Monitoring

### Real-Time Safety Dashboard

**MQTT Topics for Safety**:
```
camper/heater/safety/status              → "OK" / "WARNING" / "CRITICAL"
camper/heater/safety/chamber_temp        → 625.5
camper/heater/safety/coolant_temp        → 72.3
camper/heater/safety/fuel_status         → "OK" / "LOW" / "EMPTY"
camper/heater/safety/last_error          → "COOLANT OVERHEAT"
camper/heater/safety/error_count         → 2
camper/heater/safety/uptime              → 86400 (seconds)
```

**Home Assistant Safety Automation Example**:
```yaml
automation:
  - alias: "Heater Critical Alert"
    trigger:
      - platform: mqtt
        topic: "camper/heater/safety/status"
        payload: "CRITICAL"
    action:
      - service: notify.mobile_app
        data:
          title: "🔥 HEATER EMERGENCY"
          message: "Critical safety shutdown triggered!"
          data:
            priority: high
            ttl: 0
      - service: light.turn_on
        target:
          entity_id: light.van_warning_light
        data:
          effect: "strobe"
```

---

## 7. Testing Requirements

### Mandatory Safety Tests

**Test 1: Overtemperature Shutdown**
```
1. Run heater normally
2. Simulate high chamber temp (modify code temporarily)
3. Verify shutdown occurs < 500ms
4. Verify fuel pump stops
5. Verify fans go to maximum
6. Verify cannot restart
```

**Test 2: Coolant Overheat Shutdown**
```
1. Run heater normally
2. Gradually increase reported coolant temp
3. Verify warning at 85°C
4. Verify power reduction
5. Verify shutdown at 95°C
6. Verify extended cooling period
```

**Test 3: Fuel Depletion Detection**
```
1. Run heater normally
2. Simulate temperature drop pattern
3. Verify detection within 30 seconds
4. Verify safe shutdown
5. Verify extended purge cycle
```

**Test 4: Sensor Failure**
```
1. Disconnect burning chamber sensor
2. Verify detection < 2 seconds
3. Verify immediate shutdown
4. Verify error message accurate
```

**Test 5: Multiple Failure Scenario**
```
1. Simulate overheat + sensor failure
2. Verify safe shutdown anyway
3. Verify all safety mechanisms activate
4. Verify proper error logging
```

---

## 8. Safety Checklist

Before first operation:

- [ ] All temperature sensors installed and tested
- [ ] Burning chamber sensor verified accurate
- [ ] Coolant sensors verified accurate
- [ ] Fuel level monitoring configured
- [ ] Emergency shutdown tested
- [ ] Cooling fans verified functional
- [ ] Coolant pump verified functional
- [ ] MQTT alerts configured and tested
- [ ] Physical emergency cutoff switch installed
- [ ] Fire extinguisher accessible
- [ ] CO detector installed (separate device)
- [ ] Exhaust system verified leak-free
- [ ] All wiring secure and fused
- [ ] Documentation read and understood

---

## 9. Maintenance Requirements

### Regular Safety Checks

**Daily** (when in use):
- Visual inspection of heater and connections
- Check for fuel leaks
- Verify normal operating temperatures

**Weekly**:
- Test emergency shutdown button
- Verify all temperature sensors reading correctly
- Check exhaust system integrity

**Monthly**:
- Clean air intake filter
- Inspect coolant level and condition
- Check all electrical connections
- Verify MQTT alerts working

**Seasonally**:
- Professional inspection of combustion chamber
- Clean/replace fuel filter
- Pressure test coolant system
- Update software if available

---

## 10. Safety Incident Response

### If Emergency Shutdown Occurs

1. **DO NOT ATTEMPT IMMEDIATE RESTART**
2. Check error message via serial or MQTT
3. Investigate cause:
   - Overtemperature: Check for blockages, fan failure
   - Fuel empty: Refuel and purge system
   - Coolant overheat: Check for leaks, flow restrictions
4. Allow complete cooldown (30+ minutes)
5. Fix underlying issue
6. Test affected components
7. Reset error via command
8. Monitor closely on restart

### If Repeated Safety Shutdowns Occur

**After 3 safety shutdowns:**
- System enters permanent lockout
- Requires manual inspection and reset
- Do NOT bypass safety features
- Professional service recommended

---

## 11. Legal and Regulatory

### User Responsibilities

- Installation must comply with local codes
- User assumes all liability for modifications
- Regular maintenance is mandatory
- Professional inspection recommended annually
- Keep fire extinguisher accessible
- Never disable safety features

### Warranty Disclaimer

This is DIY open-source project:
- No warranty expressed or implied
- Use at own risk
- Safety features provided as-is
- User responsible for safe operation
- Follow all local regulations

---

**CRITICAL REMINDER**: 

These safety features are **MANDATORY** and **MUST NOT BE DISABLED OR BYPASSED**. They protect against fire hazards, equipment damage, and personal injury. If you feel these safety features are too restrictive, **DO NOT BUILD THIS PROJECT**.

Safety first, always.

---

**Document History**:
- v3.0 (2025-12-28): Critical safety requirements added per user request
- v2.0 (2025-12-28): Updated for camper van application
- v1.0 (2025-12-28): Initial requirements specification