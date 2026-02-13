# UART Protocol Testing Guide
## Hydronic Diesel Heater - Critical Decision Point

> **⚠️ ARCHIVED (February 2026)**: This guide is **no longer needed**. The project has pivoted from the HCalory W51 to the **Autoterm Flow 5D**, which provides official UART protocol documentation. See [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) for the current integration approach. This document is retained for historical reference only.

**Version:** 1.0  
**Date:** December 2025  
**Priority:** 🔬 PHASE 0 - DO THIS FIRST

---

## 🎯 Purpose

**This is the MOST IMPORTANT first step in the project.**

Before building the complete hardware replacement controller, we MUST test if your specific heater model supports UART communication. This determines whether we:
- **Option A**: Build on top of existing controller (if UART works well)
- **Option B**: Complete hardware replacement (current design)

**Time Investment**: 1-2 weeks testing  
**Potential Savings**: Months of development if UART works

---

## 📋 Prerequisites

### Hardware Needed
- ESP32 development board
- Logic level analyzer or USB-to-TTL adapter (3.3V or 5V compatible)
- Your Chinese diesel heater with control board exposed
- Multimeter
- Jumper wires
- Computer with serial terminal software

### Software Needed
- Arduino IDE or PlatformIO
- Serial terminal (Arduino Serial Monitor, PuTTY, or CoolTerm)
- Logic analyzer software (optional but helpful)

---

## 🔍 Step 1: Locate UART Connection Points

### Common Locations on Chinese Heater Controllers

1. **Controller Board Inspection**:
   - Remove heater cover to access control board
   - Look for labels: TX, RX, GND, VCC
   - Common connectors: JST-XH, pin headers, or pads

2. **Typical Chinese Heater Board Pinouts**:
   ```
   Common configurations:
   - 4-pin connector: VCC, GND, TX, RX
   - 6-pin connector: VCC, GND, TX, RX, Button+, Button-
   - Pads on PCB labeled: T, R, G, V
   ```

3. **Visual Identification**:
   - TX/RX lines often go through 1kΩ resistors
   - May connect to HC-05/HC-06 Bluetooth modules
   - Often near main microcontroller chip

### Safety Precautions
- ⚠️ **Disconnect heater from 12V power before probing**
- ⚠️ **Use multimeter to verify voltage levels (should be 3.3V or 5V)**
- ⚠️ **Do NOT connect heater main power and USB serial simultaneously** (ground loop risk)

---

## 🔌 Step 2: Physical Connection

### Connection Diagram

```
ESP32          Chinese Heater Board
GPIO 16 (RX2) ─────── TX (Heater transmits)
GPIO 17 (TX2) ─────── RX (Heater receives)
GND ──────────────── GND
                     VCC (DO NOT CONNECT unless heater is unpowered)
```

### ESP32 Code for Initial Testing

```cpp
// File: uart_test_basic.ino
// Test various baud rates to find the correct one

void setup() {
  Serial.begin(115200);  // USB serial for debugging
  
  // Try common baud rates for Chinese heaters
  // Most common: 25000, 9600, 115200
  Serial2.begin(25000, SERIAL_8N1, 16, 17);  // RX=16, TX=17
  
  Serial.println("UART Test Started");
  Serial.println("Listening on Serial2 at 25000 baud...");
}

void loop() {
  // Forward heater data to USB serial
  if (Serial2.available()) {
    char c = Serial2.read();
    Serial.print("HEX: 0x");
    Serial.print(c, HEX);
    Serial.print(" DEC: ");
    Serial.print(c, DEC);
    Serial.print(" CHAR: ");
    Serial.println(c);
  }
  
  // Forward USB commands to heater
  if (Serial.available()) {
    char c = Serial.read();
    Serial2.write(c);
    Serial.print("Sent: 0x");
    Serial.println(c, HEX);
  }
}
```

---

## 📡 Step 3: Identify Protocol

### Test Different Baud Rates

Common baud rates for Chinese diesel heaters:
- **25000** ← Most common for Eberspacher-clone protocol
- 9600
- 19200
- 38400
- 115200

### Test Procedure

1. **Power on heater** (with 12V supply, NOT running)
2. **Monitor serial output** for 30 seconds
3. **Look for repeating patterns**:
   - Regular data frames (every 1-2 seconds typical)
   - Consistent byte sequences
   - Checksum patterns at end of frames

4. **If you see garbage**: Try different baud rate
5. **If you see nothing**: Check connections, try TX/RX swap

### Expected Output Examples

**Good signal (correct baud rate)**:
```
HEX: 0x76 DEC: 118 CHAR: v
HEX: 0x0C DEC: 12  CHAR: 
HEX: 0x00 DEC: 0   CHAR: 
HEX: 0x2D DEC: 45  CHAR: -
...repeating pattern every 2 seconds...
```

**Bad signal (wrong baud rate)**:
```
HEX: 0xFF DEC: 255 CHAR: �
HEX: 0x8A DEC: 138 CHAR: �
HEX: 0x23 DEC: 35  CHAR: #
...random garbage...
```

---

## 🧪 Step 4: Protocol Analysis

### Frame Structure Analysis

Once you find the correct baud rate:

1. **Capture 100+ frames** in a file
2. **Look for patterns**:
   - Frame start bytes (common: 0x76, 0xAA, 0x55)
   - Frame length (typical: 12-24 bytes)
   - Checksum position (usually last byte or two)
   - Repeated values in same positions

### Common Protocol Types

**Type 1: Eberspacher Clone (Most Common)**
```
Frame format (24 bytes at 25000 baud):
[0] = 0x76 (Start byte)
[1] = 0x16 (Frame length)
[2-3] = State/Status
[4-5] = Temperature readings
[6-7] = Pump frequency
[8-9] = Fan RPM
[10-21] = Various parameters
[22-23] = CRC16 checksum
```

**Type 2: Webasto Clone**
```
Frame format (varies):
Different protocol, less common in cheap Chinese heaters
```

**Type 3: No Protocol**
```
No UART communication at all
Controller is isolated, no serial interface
```

### Analysis Tools

**Use logic analyzer** (if available):
- Capture both TX and RX simultaneously
- Look for request/response patterns
- Measure timing between frames

**Manual analysis**:
- Log data for 5 minutes during heater operation
- Note changes when heater state changes
- Correlate with actual heater behavior

---

## 🎮 Step 5: Command Testing

### Attempt to Send Commands

**WARNING**: Only test with heater NOT running (ignition off)

### Common Command Sequences to Try

```cpp
// Example commands to test (based on Eberspacher protocol)

// Try to start heater
byte cmd_start[] = {0x76, 0x16, 0xA0, 0x05, 0x00, 0x32, /* ... */, 0xCRC};

// Try to stop heater
byte cmd_stop[] = {0x76, 0x16, 0xA0, 0x00, 0x00, 0x00, /* ... */, 0xCRC};

// Try to change power level
byte cmd_power[] = {0x76, 0x16, 0xA0, 0x01, 0x00, 0x1E, /* ... */, 0xCRC};
```

**Testing procedure**:
1. Send command
2. Wait for response (timeout: 1 second)
3. Observe heater behavior
4. Check if status frames change

---

## 📊 Step 6: Document Results

### Create Test Report

Document in `UART_TEST_RESULTS.md`:

```markdown
# UART Test Results

**Heater Model**: [Your heater model/brand]
**Test Date**: [Date]
**Tester**: [Your name]

## Connection Details
- TX Pin: [Location]
- RX Pin: [Location]
- Voltage Level: [3.3V / 5V]

## Protocol Detection
- Baud Rate: [25000 / other]
- Data Format: [8N1 / other]
- Frame Length: [bytes]
- Frame Rate: [Hz]

## Read Capability
- [X] Can read status frames
- [ ] Cannot detect any UART signal
- Frame structure: [Describe or paste hex dump]

## Write Capability
- [ ] Successfully sent commands
- [ ] Heater responded to commands
- [ ] No response to any commands
- Commands tested: [List]

## Control Granularity
If write worked:
- [ ] Can turn on/off
- [ ] Can adjust power (percentage range: ___ to ___)
- [ ] Can adjust fan speed
- [ ] Can read temperatures
- [ ] Can read error codes

## Conclusion
[Summary and recommendation]
```

---

## 🎯 Step 7: Make Decision

### Decision Matrix

| Test Result | Recommended Approach | Rationale |
|-------------|---------------------|-----------|
| **Full read/write, good control** | Hybrid (UART + Safety) | Leverage existing controller, add safety on top |
| **Read-only** | Validation approach | Use our hardware control, UART for validation |
| **No UART or very limited** | Full replacement (current design) | Complete hardware control needed |

### Next Steps Based on Results

#### ✅ If UART Works Well:
1. Document complete protocol specification
2. Create `HeaterUART` class
3. Implement hybrid control system:
   - Use UART for normal operation
   - Override with hardware control for safety
   - Monitor both our sensors and heater's sensors
4. Update architecture documentation

#### ⚠️ If UART Read-Only:
1. Use UART for monitoring/validation
2. Continue with hardware control (current design)
3. Compare our sensors vs. heater sensors
4. Use UART data for diagnostics

#### ❌ If No UART:
1. Continue with current full replacement design
2. No changes needed to architecture
3. Proceed with Phase 1 implementation

---

## 📚 Resources

### Protocol References
- Eberspacher protocol: Search GitHub for "eberspacher protocol"
- Webasto protocol: Similar heaters use variations
- Chinese heater communities: Reddit r/Diesel_Heaters

### Similar Projects with UART Support
- afterburner-mrjones (has UART option)
- HeaterControlV3 (UART-focused)
- Various ESP32 diesel heater projects on GitHub

### Tools
- Logic analyzer: Saleae Logic, DSLogic
- Serial terminal: CoolTerm, PuTTY, Arduino Serial Monitor
- Protocol analyzer: Wireshark with serial plugin

---

## 🚨 Safety Reminders

1. **Never connect when heater is actively burning**
2. **Verify voltage levels before connecting ESP32**
3. **Use isolation if possible** (optocouplers for final design)
4. **Keep fire extinguisher nearby during testing**
5. **Test in well-ventilated area**
6. **Have emergency shutdown method ready**

---

## ✅ Success Criteria

**Test is successful if you can answer**:
1. ✅ Does my heater have UART? (Yes/No)
2. ✅ What is the baud rate? (______)
3. ✅ Can I read data? (Yes/No)
4. ✅ Can I send commands? (Yes/No)
5. ✅ Can I control power granularly? (Yes/No/Partially)
6. ✅ What is my recommended approach? (Hybrid/Validation/Replacement)

**After answering these**, you'll know exactly how to proceed with the project.

---

## 🎓 Educational Value

**Even if UART doesn't work**, this testing provides:
- Understanding of your specific heater's internals
- Knowledge of control board layout
- Experience with serial protocols
- Validation that hardware replacement is necessary

**Time well spent regardless of outcome!**

---

## 📞 Support

If you encounter issues during testing:
1. Document what you tried
2. Capture serial dumps
3. Take photos of connections
4. Share in project discussions

**Remember**: The goal is to find the BEST approach for YOUR specific heater, not to force a particular solution.

Good luck with your testing! 🔬🔥
