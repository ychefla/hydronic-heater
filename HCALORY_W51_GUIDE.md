# Hcalory W51 YWH-A202 Specific Implementation Guide

**Heater Model**: Hcalory W51 (YWH-A202)  
**Type**: 5kW Hydronic Diesel Heater  
**Application**: Camper van floor/air/water heating  
**Controller Type**: Proprietary (Chinese diesel heater standard)

---

## 1. Heater Specifications

### Known Specifications
- **Model**: Hcalory W51 YWH-A202
- **Power Output**: Typically 2-5kW (adjustable)
- **Fuel Type**: Diesel
- **Voltage**: 12V DC (common for camper vans)
- **Type**: Hydronic (coolant-based)
- **Control Panel**: Wired LCD/LED control panel with temperature display
- **Optional**: Bluetooth control via smartphone app (BLE)

### Temperature Specifications (Based on Similar Models)
- **Operating Range**: 130-230°C (burning chamber sensor)
- **Ignition Temperature**: ~80°C
- **Target Operating**: ~180°C
- **Maximum Safe**: 230°C

---

## 2. UART Protocol Investigation

### Current Status
**⚠️ UART protocol for Hcalory W51 YWH-A202 is NOT officially documented.**

The Hcalory W51 uses a **proprietary control protocol** between the main controller board and the control panel. While the heater likely communicates via UART internally, the exact protocol specifications are not publicly available.

### Known Communication Methods
1. **Wired Control Panel** - Standard connection (most common)
2. **RF Remote** - 433 MHz wireless remote (optional)
3. **Bluetooth LE** - Smartphone app control (some models)

### Protocol Discovery Options

#### Option A: Reverse Engineering (Recommended First Step)
You can discover the UART protocol by monitoring communication between the existing controller and control panel:

**Equipment Needed**:
- USB-TTL adapter (3.3V logic level)
- Logic analyzer (optional but recommended)
- Multimeter

**Steps**:
1. **Identify UART Lines**
   - Open the heater control panel connection
   - Locate RX/TX/GND wires (often labeled or color-coded)
   - Common colors: Yellow/Blue for data, Red for power, Black for ground
   - Measure voltage levels (should be 3.3V or 5V logic)

2. **Tap Into Communication**
   ```
   Heater Controller <---> Control Panel
         |                      |
         |---> USB-TTL Adapter --->  Computer
         |      (RX/TX/GND)
   ```
   - Connect USB-TTL RX to Controller TX
   - Connect USB-TTL TX to Controller RX (optional, for bidirectional monitoring)
   - Connect GND to common ground

3. **Capture Serial Traffic**
   - Use serial monitor (Arduino IDE, PuTTY, or Realterm)
   - Try common baud rates: 9600, 19200, 38400, 57600, 115200
   - Try settings: 8N1, 8E1, 8O1
   - Look for repeating patterns when:
     - Turning heater on/off
     - Changing temperature setpoint
     - Changing power level
     - Reading status/temperature

4. **Analyze Protocol**
   - Identify packet structure (header, length, command, data, checksum)
   - Document command codes for:
     - Power on/off
     - Set temperature
     - Set power level
     - Read temperature
     - Read status
   - Test if you can send commands and get responses

#### Option B: Consult Community Resources
Several open-source projects have reverse-engineered Chinese diesel heater protocols:

- **DieselHeaterRF** (GitHub: jakkik/DieselHeaterRF) - ESP32 library for RF control
- **Afterburner** - Enhanced heater controller (may have protocol info)
- **Van Life Forums** - Users who have done similar work

#### Option C: Contact Manufacturer
- Hcalory official support: Request technical documentation for integrators
- May provide UART protocol specifications for development purposes
- Unlikely but worth attempting

---

## 3. Implementation Approaches

### Approach 1: Full Hardware Replacement (Current Design) ✅ READY NOW

**What This Means**:
- Replace the entire Hcalory controller with ESP32-based system
- Direct control of all components: glow plug, fuel pump, air fan, coolant pump
- Independent temperature sensors (DS18B20)
- No dependency on existing heater controller

**Advantages**:
- Complete control and customization
- All safety features under your control
- Variable power output (PWM fuel pump control)
- No protocol limitations
- All documentation already complete (current PR)

**Disadvantages**:
- More hardware work (MOSFETs, relays, wiring)
- Need to calibrate power levels
- Higher complexity

**Status**: 
- ✅ Complete design documentation (18 files)
- ✅ All code ready (controller, safety, power modulation)
- ✅ Wiring guide complete
- ✅ Safety systems implemented
- ✅ Paku-IoT integration ready

**Next Steps**:
1. Follow WIRING.md for hardware connections
2. Install sensors (burning chamber, coolant in/out, room)
3. Configure pin mappings in config.h
4. Upload firmware and test
5. Calibrate power levels (QUICKSTART.md)

---

### Approach 2: Hybrid (UART Control) ⏳ REQUIRES TESTING

**What This Means**:
- Keep existing Hcalory controller in place
- ESP32 communicates with heater via UART protocol
- Send commands to set temperature, power level, on/off
- Read status and temperature from heater
- Add additional sensors for enhanced safety

**Advantages**:
- Less hardware modification
- Leverage existing heater controller
- Potentially faster to implement (if protocol is simple)
- Lower risk of component damage

**Disadvantages**:
- Dependent on undocumented protocol
- Limited by protocol capabilities
- May not support variable power control
- Protocol may not be reliable or complete
- Still need to reverse engineer protocol

**Requirements**:
1. **MUST** successfully reverse engineer UART protocol
2. **MUST** verify protocol supports:
   - Remote on/off control
   - Temperature setpoint adjustment
   - Power level control (most critical for our use case)
   - Status/temperature reading
3. **MUST** test reliability and error handling

**Testing Procedure**:
1. Follow "Option A: Reverse Engineering" above
2. Document complete protocol in `UART_TEST_RESULTS.md`
3. Create test branch: `test/uart-protocol-hcalory-w51`
4. Implement basic ESP32 UART bridge
5. Test all required functions
6. **Decision point**: If protocol is adequate → proceed with hybrid
7. If protocol is inadequate or unreliable → use Approach 1

---

## 4. Recommended Path Forward

### Phase 0: UART Protocol Testing (1-2 weeks)

**Objective**: Determine if hybrid approach is viable

**Tasks**:
1. ✅ Identify heater model (Hcalory W51 YWH-A202)
2. 🔄 Acquire USB-TTL adapter (3.3V)
3. 🔄 Open control panel connector
4. 🔄 Identify UART lines (RX/TX/GND)
5. 🔄 Capture serial traffic while operating heater
6. 🔄 Document protocol (baud rate, format, commands)
7. 🔄 Test bidirectional communication
8. 🔄 Verify power control capabilities

**Success Criteria**:
- ✅ Protocol is decipherable
- ✅ Can send commands and receive responses
- ✅ **Can control power output** (20-100% range)
- ✅ Can read temperature and status
- ✅ Protocol is reliable and well-behaved

**Failure Criteria**:
- ❌ Protocol is encrypted or obfuscated
- ❌ **Cannot adjust power output granularly**
- ❌ Protocol is unreliable or incomplete
- ❌ Cannot read critical parameters

### Decision Point

**IF UART TESTING SUCCEEDS** → Pivot to Approach 2 (Hybrid)
- Create new design documents
- Implement UART communication layer
- Integrate with Paku-IoT
- Add supplementary sensors for safety

**IF UART TESTING FAILS** → Continue with Approach 1 (Full Replacement)
- All design work already complete
- Follow existing documentation
- Implement hardware according to WIRING.md
- Deploy production system

---

## 5. Hcalory W51 Specific Notes

### Control Panel Connector
- **Typical Pinout** (verify with multimeter):
  - Pin 1: 12V Power (Red)
  - Pin 2: Ground (Black)
  - Pin 3: TX Data (Yellow/Blue)
  - Pin 4: RX Data (Yellow/Blue)
  - Pin 5-6: Temperature sensor (if wired to panel)

**⚠️ WARNING**: Pinout may vary by production batch. Always verify with multimeter before connecting!

### Motherboard Access
- Main controller is typically inside the heater unit
- May require disassembly to access
- Look for labels: "TX", "RX", "GND", "VCC"
- Controller often uses STM32 or similar microcontroller

### Bluetooth Consideration
Some Hcalory W51 models include Bluetooth LE:
- If your model has Bluetooth, you could potentially:
  - Sniff BLE protocol (easier than UART in some cases)
  - Use existing Home Assistant integration as reference
  - Connect ESP32 directly via BLE
- BLE may offer similar or better control than UART

**Resources**:
- [Hcalory BLE Home Assistant Integration](https://community.home-assistant.io/t/hcalory-ble-control-a-hcalory-diesel-heater-from-ha/799994)

---

## 6. Safety Considerations for Hcalory W51

When implementing either approach, ensure these safety features are active:

### Critical Safety Features (All Implementations)
1. **Burning Chamber Temperature Monitoring**
   - Install DS18B20 sensor in burning chamber outlet
   - Max safe temperature: 230°C
   - Emergency shutdown at ≥230°C

2. **Coolant Temperature Monitoring**
   - Install sensors at coolant inlet and outlet
   - Critical shutdown at >95°C (boiling prevention)

3. **Rapid Temperature Spike Detection**
   - Monitor temperature rate of change
   - Shutdown if >50°C increase in 5 seconds

4. **Minimum Operating Temperature**
   - Do not reduce power below minimum when <130°C
   - Prevents incomplete combustion and sooting

5. **Coolant Pump Verification**
   - Ensure coolant pump is running when heating
   - Optional: flow sensor for direct verification

6. **Fuel Depletion Detection**
   - Monitor for temperature drop patterns
   - Safe shutdown if fuel supply fails

**See SAFETY.md v5.1 for complete safety system documentation.**

---

## 7. Testing Checklist

### Before Starting UART Testing
- [ ] Read UART_TESTING_GUIDE.md completely
- [ ] Acquire necessary equipment (USB-TTL adapter, multimeter)
- [ ] Backup existing control panel connector pinout
- [ ] Take photos of original wiring
- [ ] Prepare serial monitoring software

### During UART Testing
- [ ] Identify correct UART pins
- [ ] Verify voltage levels (3.3V or 5V)
- [ ] Test multiple baud rates
- [ ] Capture startup sequence
- [ ] Capture on/off commands
- [ ] Capture temperature setpoint changes
- [ ] **Capture power level adjustments** (CRITICAL)
- [ ] Document complete protocol

### After UART Testing
- [ ] Document results in `UART_TEST_RESULTS.md`
- [ ] Make implementation decision
- [ ] Update project documentation
- [ ] Proceed with chosen approach

---

## 8. Support Resources

### Official Hcalory Resources
- [Hcalory Official Website](https://hcalory.com)
- [Product Manuals and Videos](https://hcalory.com/pages/how-to-use)
- W51 Product Manual (download from website)

### Community Resources
- [Chinese Diesel Heater Controller Guide](https://www.vanlifeuksurvivorsguide.co.uk/post/chinese-diesel-heater-controller-motherboard-remote-control-guide)
- [DieselHeaterRF GitHub Project](https://github.com/jakkik/DieselHeaterRF)
- Van Life forums and communities

### This Project Documentation
- **UART_TESTING_GUIDE.md** - General UART testing procedures
- **WIRING.md** - Hardware connections for full replacement
- **SAFETY.md v5.1** - All safety systems (mandatory reading)
- **QUICKSTART.md** - Getting started guide
- **PAKU_INTEGRATION.md** - Cloud platform integration

---

## 9. Quick Decision Guide

### Should I test UART first?

**YES, test UART if**:
- ✅ You're comfortable with electronics reverse engineering
- ✅ You want to minimize hardware modifications
- ✅ You have time for 1-2 weeks of testing
- ✅ You're willing to pivot to full replacement if testing fails

**NO, skip UART testing if**:
- ✅ You want to start implementation immediately
- ✅ You're confident in hardware work (MOSFETs, relays)
- ✅ You want complete control and maximum flexibility
- ✅ You value comprehensive safety features over simplicity

**Current recommendation**: Test UART first (1-2 weeks investment) to potentially save months of hardware work. If UART proves inadequate, all documentation for full replacement is already complete.

---

## 10. Next Steps

1. **Read this entire guide**
2. **Decide on testing approach**:
   - Option A: Test UART protocol first (recommended)
   - Option B: Proceed with full hardware replacement (current design)
3. **If testing UART**:
   - Follow Section 2 (UART Protocol Investigation)
   - Create test branch: `test/uart-protocol-hcalory-w51`
   - Document results
   - Make decision based on findings
4. **If skipping UART**:
   - Proceed to WIRING.md
   - Order components
   - Begin hardware implementation

---

**Status**: Ready for Phase 0 UART testing  
**Timeline**: 1-2 weeks testing → decision → implementation  
**Risk**: Low (worst case: fall back to full replacement design)  

**Good luck with your Hcalory W51 YWH-A202 integration! 🚐🔥**
