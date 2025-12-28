# Troubleshooting Guide

This guide helps diagnose and fix common issues with the hydronic heater controller.

## Quick Diagnostics

### Step 1: Check Serial Output
Connect to serial monitor at 115200 baud and look for:
- Initialization messages
- Current state
- Temperature readings
- Error messages

### Step 2: Type "status"
Review the output for:
- All temperatures should be reasonable (not -127°C)
- Component states match expectations
- No error messages

### Step 3: Check Power
- ESP32 has stable power (LED on)
- High-power components have adequate supply
- All fuses intact

---

## Common Issues

### Issue: Temperature Shows -127°C

**Symptom**: Sensor reads -127.0°C constantly

**Possible Causes**:
1. Sensor not connected
2. Sensor damaged
3. Missing pullup resistor
4. Wrong pin number in config
5. Bad solder joint

**Solutions**:
```
1. Check physical connection of DS18B20
   - Red wire to 3.3V or 5V
   - Black wire to GND
   - Yellow/white wire to GPIO pin

2. Verify 4.7kΩ pullup resistor present between data pin and VCC

3. Test sensor with multimeter:
   - Power to GND: should be ~5V or 3.3V
   - Data pin should not be floating

4. Try different GPIO pin to rule out pin damage

5. Test with known-good sensor
```

---

### Issue: ESP32 Keeps Resetting

**Symptom**: System reboots repeatedly, "brownout detector" messages

**Possible Causes**:
1. Insufficient power supply
2. Power supply noise from high-current loads
3. Ground loops
4. Faulty USB cable

**Solutions**:
```
1. Use adequate power supply:
   - ESP32 needs stable 5V, 500mA minimum
   - Use quality USB cable with data lines
   - Consider external 5V regulator

2. Add bulk capacitor (100-1000µF) near ESP32 VIN

3. Separate high-current grounds from ESP32 ground
   - Use star ground configuration
   - Keep high-current returns away from ESP32

4. Try different USB port or power supply

5. Temporarily disconnect all high-power components
   - If stable, add components back one at a time
```

---

### Issue: Glow Plug Doesn't Heat

**Symptom**: No activity on glow plug circuit

**Possible Causes**:
1. MOSFET not switching
2. No power to load
3. Blown fuse
4. Bad connection
5. Software not activating

**Solutions**:
```
1. Measure voltage at MOSFET gate:
   - Should be 3.3V when active
   - Check with multimeter or oscilloscope

2. Check MOSFET wiring:
   - Gate to ESP32 via 1kΩ resistor
   - Source to GND
   - Drain to load negative
   - Load positive to 12V

3. Test MOSFET:
   - Measure drain-source voltage
   - Should be near 0V when ON
   - Should be 12V when OFF

4. Check fuse continuity

5. Verify software activation:
   - Add debug: Serial.println("Glow plug ON");
   - Check state is GLOW_PLUG_WARMUP or IGNITION
```

---

### Issue: Fans Don't Spin

**Symptom**: Fans don't respond to PWM control

**Possible Causes**:
1. PWM frequency too high/low for fan
2. Not enough voltage/current
3. MOSFET selection wrong
4. Fan damaged

**Solutions**:
```
1. Try different PWM frequency in config.h:
   #define PWM_FREQUENCY 25000  // Try 25kHz
   or
   #define PWM_FREQUENCY 1000   // Try 1kHz

2. Test fan directly with 12V:
   - Should spin freely
   - Check current draw

3. Use logic-level MOSFET (Vgs threshold < 3V):
   - IRLZ44N or similar
   - Check datasheet

4. Verify PWM signal with oscilloscope

5. Try higher speed value:
   fan->setSpeed(255);  // Full speed for testing
```

---

### Issue: System Goes to ERROR State

**Symptom**: State changes to ERROR, heater stops

**Possible Causes**:
1. Overtemperature condition
2. Ignition timeout
3. Safety check triggered

**Solutions**:
```
1. Check error message:
   String err = controller->getErrorMessage();
   Serial.println(err);

2. For "Overheat detected":
   - Verify burning chamber sensor accurate
   - Check if heater actually overheating
   - Adjust MAX_SAFE_TEMP if false alarm
   - Ensure adequate cooling

3. For "Ignition timeout":
   - Check fuel supply
   - Verify air fan working
   - Check glow plug temperature
   - Increase STARTUP_SEQUENCE_TIME if needed
   - Verify fuel pump operation

4. Reset by stopping and restarting:
   controller->stopHeater();
   // Wait for OFF state
   controller->startHeater();
```

---

### Issue: Won't Ignite

**Symptom**: Stuck in IGNITION state, eventually times out

**Possible Causes**:
1. No fuel delivery
2. Insufficient air flow
3. Glow plug not hot enough
4. Fuel mixture wrong
5. Ignition threshold too high

**Solutions**:
```
1. Verify fuel pump operation:
   - Listen for pump running
   - Check fuel line pressure
   - Ensure fuel tank not empty

2. Check air fan:
   - Should be running during ignition
   - Verify airflow to combustion chamber
   - Clean air intake

3. Verify glow plug:
   - Should have completed warmup (60s)
   - Should be glowing red/orange
   - Check voltage and current draw

4. Lower ignition temperature temporarily:
   #define IGNITION_TEMP 250.0  // Was 300.0

5. Increase ignition timeout:
   #define STARTUP_SEQUENCE_TIME 180000  // 3 minutes
```

---

### Issue: Runs but Won't Maintain Temperature

**Symptom**: Ignites but temperature drops, cycles on/off

**Possible Causes**:
1. Insufficient fuel delivery
2. Excessive air flow
3. Control logic issues
4. Temperature sensor location

**Solutions**:
```
1. Check fuel pump:
   - Should run continuously when in RUNNING state
   - Verify adequate fuel flow rate

2. Adjust air fan speed:
   - In handleRunning(), reduce fan speed
   - May need lower airflow for your heater

3. Check temperature sensor placement:
   - Should be in hottest part of chamber
   - Not in exhaust flow
   - Good thermal contact

4. Add hysteresis to prevent oscillation
   - Already implemented, may need tuning
```

---

### Issue: Coolant Not Circulating

**Symptom**: Coolant pump doesn't activate or no flow

**Possible Causes**:
1. Pump not activating
2. Air lock in system
3. Pump damaged
4. Temperature threshold not met

**Solutions**:
```
1. Check activation logic:
   - Pump starts when coolant output > COOLANT_MIN_TEMP (40°C)
   - Lower if needed:
     #define COOLANT_MIN_TEMP 30.0

2. Force pump on for testing:
   coolantPump->turnOn();

3. Bleed air from coolant system:
   - Run pump manually
   - Open bleed valves
   - Fill system completely

4. Verify pump power:
   - 12V at pump terminals when active
   - Check MOSFET/relay switching

5. Check flow sensor (if installed):
   - Should show > 0 when pump running
```

---

### Issue: Build Errors

**Symptom**: Won't compile

**Common Errors & Fixes**:

**"OneWire.h: No such file or directory"**
```bash
# Install libraries manually
pio lib install "OneWire"
pio lib install "DallasTemperature"
```

**"undefined reference to 'GlowPlug::begin'"**
```
- Ensure all .cpp files in src/ directory
- Try clean build: pio run --target clean
```

**"'ledcSetup' was not declared"**
```
- ESP32 platform not installed
- Check platformio.ini has correct platform
- Try: pio platform install espressif32
```

---

### Issue: Upload Fails

**Symptom**: Can't upload to ESP32

**Solutions**:
```
1. Hold BOOT button during upload

2. Check correct port selected:
   pio device list
   # Update platformio.ini if needed

3. Try different baud rate:
   # In platformio.ini
   upload_speed = 115200

4. Check USB cable:
   - Must support data, not just charging
   - Try different cable

5. Install USB-to-Serial drivers:
   - CP210x or CH340 depending on board

6. Check board selection:
   # In platformio.ini should be:
   board = esp32dev
```

---

## Safety Issue Warnings

### Heater Overheating

**If heater is physically overheating:**
1. Press emergency stop immediately
2. Cut power to fuel pump
3. Keep cooling fans running
4. Do not attempt restart until cool
5. Investigate cause before next use

**Possible causes:**
- Coolant not circulating
- Heat exchanger blocked
- Excessive fuel delivery
- Insufficient cooling capacity

### Fuel Leak

**If you smell diesel or see leak:**
1. Stop heater immediately
2. Cut power to entire system
3. Ventilate area
4. Fix leak before restart
5. Check all fuel line connections

### Smoke or Strange Odors

**If unusual smoke/smell:**
1. Stop heater
2. Check exhaust is not blocked
3. Verify proper combustion (blue flame, not black smoke)
4. Check air intake not restricted
5. Verify fuel quality

---

## Diagnostic Commands

Add these to main.cpp for debugging:

```cpp
void processCommand(String command) {
    // Existing commands...
    
    if (command == "test-glow") {
        glowPlug->turnOn();
        Serial.println("Glow plug ON");
    }
    else if (command == "test-pump") {
        dieselPump->turnOn();
        Serial.println("Diesel pump ON");
    }
    else if (command == "test-fan") {
        airFan->setSpeed(255);
        Serial.println("Air fan FULL");
    }
    else if (command == "all-off") {
        glowPlug->turnOff();
        dieselPump->turnOff();
        airFan->turnOff();
        Serial.println("All OFF");
    }
}
```

---

## Getting Additional Help

If problems persist:

1. **Document the issue:**
   - Copy serial output
   - Note all error messages
   - Record sequence of events

2. **Check basics:**
   - All connections secure
   - Power supply adequate
   - Fuses intact
   - Software matches hardware

3. **Isolate the problem:**
   - Test components individually
   - Use multimeter to verify voltages
   - Check continuity of all connections

4. **Open GitHub issue:**
   - Include serial output
   - Describe setup and wiring
   - List troubleshooting steps tried
   - Include photos if helpful

---

## Maintenance Checklist

Regular maintenance prevents issues:

- [ ] Check all electrical connections monthly
- [ ] Clean temperature sensors
- [ ] Verify fuel filter clean
- [ ] Test emergency shutdown
- [ ] Check fan bearings (listen for noise)
- [ ] Inspect fuel lines for cracks
- [ ] Clean air intake filter
- [ ] Test all safety features
- [ ] Backup configuration settings
- [ ] Update software if newer version available
