# Quick Start Guide

This guide will help you get your hydronic heater controller up and running quickly.

## Prerequisites

- ESP32 development board
- USB cable for programming
- Computer with PlatformIO installed
- Basic electronic components (MOSFETs, resistors, etc.)
- Hydronic diesel heater hardware

## Step 1: Install Software

### Install PlatformIO

Choose one method:

**Option A: PlatformIO IDE (Recommended for beginners)**
1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Install PlatformIO IDE extension from VS Code marketplace
3. Restart VS Code

**Option B: PlatformIO Core CLI**
```bash
pip install platformio
```

## Step 2: Download and Open Project

```bash
# Clone the repository
git clone https://github.com/ychefla/hydronic-heater.git
cd hydronic-heater

# Open in VS Code (if using PlatformIO IDE)
code .
```

## Step 3: Configure for Your Setup

1. Review `include/config.h` for default pin assignments
2. If you need different pins, create your own configuration
3. Adjust temperature thresholds if needed

**Optional**: Disable flow sensor if not using:
```cpp
// In src/main.cpp, line 8:
controller = new HydronicHeaterController(false); // false = no flow sensor
```

## Step 4: Connect ESP32

1. Connect ESP32 to computer via USB
2. Note the COM port (usually auto-detected)

## Step 5: Build and Upload

### Using PlatformIO IDE:
1. Click the checkmark icon (✓) to build
2. Click the arrow icon (→) to upload
3. Click the plug icon to open serial monitor

### Using PlatformIO CLI:
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Open serial monitor
pio device monitor
```

## Step 6: Initial Test (No Hardware Connected)

1. Upload the code to ESP32
2. Open serial monitor (115200 baud)
3. You should see startup messages
4. Type `status` and press Enter - you'll see sensor readings (will show errors without sensors)

This confirms the software is working!

## Step 7: Connect Temperature Sensors

Start with just the temperature sensors:

1. Wire one DS18B20 sensor to GPIO 4 (burning chamber)
   - Red → 3.3V
   - Black → GND
   - Yellow → GPIO 4 (with 4.7kΩ pullup to 3.3V)

2. Upload code and check serial monitor
3. Type `status` - you should now see a valid temperature reading

4. Repeat for other sensors (GPIO 16, 17, 5)

**Tip**: Test each sensor individually before connecting all of them.

## Step 8: Wire Control Outputs

⚠️ **CRITICAL**: Never connect high-power loads directly to ESP32!

Use MOSFETs or relays for all outputs. See `WIRING.md` for detailed diagrams.

Start with LOW-POWER components first:
1. Connect fans (with MOSFETs)
2. Test by typing `start` in serial monitor
3. Observe fan behavior through startup sequence

## Step 9: Test Without Fuel

**IMPORTANT**: Do initial testing WITHOUT connecting fuel!

1. Wire all components EXCEPT diesel pump
2. Upload code and type `start`
3. Observe the startup sequence:
   - Glow plug should activate
   - After warmup, air fan should start
   - System should eventually timeout (no ignition without fuel)
   - Type `stop` to shutdown safely

## Step 10: Full System Test

⚠️ **WARNING**: Only perform full test in safe environment!

1. Connect diesel pump circuit
2. Ensure proper ventilation
3. Have fire extinguisher ready
4. Type `start` to begin sequence
5. Monitor temperatures and system state

## Commands Reference

Connect via serial monitor at 115200 baud:

- `start` - Start the heater
- `stop` - Stop the heater (safe shutdown)
- `status` - Print current system status

Status is also printed automatically every 10 seconds.

## Troubleshooting

### ESP32 won't connect
- Check USB cable (must support data)
- Try different USB port
- Press BOOT button while uploading

### Temperature reads -127°C
- Sensor not connected or faulty
- Check wiring
- Verify 4.7kΩ pullup resistor

### Component doesn't activate
- Check MOSFET wiring
- Verify power supply
- Check fuses
- Test component with multimeter

### System goes to ERROR state
- Check serial monitor for error message
- Verify all temperature sensors working
- Check for overtemperature condition

### Build fails
- Verify PlatformIO is installed correctly
- Check internet connection (downloads libraries)
- Try: `pio lib install` to manually install libraries

## Safety Reminders

- ✓ Test incrementally
- ✓ Use proper wire gauges
- ✓ Install all fuses
- ✓ Never leave unattended
- ✓ Ensure adequate ventilation
- ✓ Have fire extinguisher ready
- ✓ Follow local regulations

## Next Steps

Once basic operation is confirmed:

1. Fine-tune temperature thresholds in `config.h`
2. Adjust timing constants for your specific heater
3. Calibrate flow sensor if installed
4. Add additional safety interlocks as needed
5. Consider adding remote monitoring (WiFi/Bluetooth)

## Getting Help

If you encounter issues:

1. Check `README.md` for detailed documentation
2. Review `WIRING.md` for wiring details
3. Examine serial output for error messages
4. Verify all connections with multimeter
5. Open an issue on GitHub with details

## Success Indicators

You know it's working when:
- ✓ All temperature sensors show reasonable readings
- ✓ Startup sequence progresses correctly
- ✓ Glow plug heats up during warmup
- ✓ Ignition occurs and temperature rises
- ✓ System maintains operating temperature
- ✓ Coolant circulation activates
- ✓ Safe shutdown works properly

Happy heating! 🔥
