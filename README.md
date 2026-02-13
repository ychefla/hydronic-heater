# Hydronic Diesel Heater Controller

ESP32-based smart controller for the **Autoterm Flow 5D** hydronic diesel heater. Uses a **hybrid architecture**: the Autoterm controller handles combustion safety, while the ESP32 provides multi-zone heating control, scheduling, PID temperature feedback, and MQTT/Paku-IoT cloud integration.

> **🎯 Autoterm Flow 5D**: This project targets the Autoterm Flow 5D specifically for its documented UART protocol and DIY-friendly integration. See **[AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md)** for the complete integration guide.
>
> **📝 Historical note**: This project originally targeted the HCalory W51 with a full hardware replacement approach. That was abandoned because HCalory refused to provide protocol documentation, making reliable integration impossible. See [HCALORY_W51_GUIDE.md](HCALORY_W51_GUIDE.md) for the archived original design.

## Documentation Structure

This project follows a design-first approach. Please review documents in this order:

### 🎯 START HERE
1. **[AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md)** - 🎯 **Autoterm Flow 5D Integration Guide** — heater specs, hybrid architecture, UART protocol, getting started

### Design Documentation
2. **[REQUIREMENTS.md](REQUIREMENTS.md)** - Base system requirements specification
3. **[REQUIREMENTS_V2.md](REQUIREMENTS_V2.md)** - Extended campervan requirements
4. **[DESIGN.md](DESIGN.md)** - System architecture and design decisions
5. **[ARCHITECTURE.md](ARCHITECTURE.md)** - Visual architecture diagrams
6. **[SAFETY.md](SAFETY.md)** - ⚠️ **CRITICAL** - Safety requirements (READ BEFORE DEPLOYING)

### Implementation Guides
7. **README.md** (this file) - Implementation overview and features
8. **[WIRING.md](WIRING.md)** - Hardware wiring guide (zone distribution)
9. **[QUICKSTART.md](QUICKSTART.md)** - Quick setup guide
10. **[API.md](API.md)** - API documentation for developers

### Integration & Advanced
11. **[PAKU_INTEGRATION.md](PAKU_INTEGRATION.md)** - Paku-IoT cloud platform integration guide
12. **[CAMPERVAN_FEATURES.md](CAMPERVAN_FEATURES.md)** - Camper van application guide
13. **[RELATED_PROJECTS.md](RELATED_PROJECTS.md)** - Feature priorities and related projects
14. **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem diagnosis and solutions

### Archived (Original HCalory Design)
15. **[HCALORY_W51_GUIDE.md](HCALORY_W51_GUIDE.md)** - *(archived)* Original HCalory W51 guide
16. **[UART_TESTING_GUIDE.md](UART_TESTING_GUIDE.md)** - *(archived)* Generic UART reverse-engineering guide

## Features

- **Hybrid architecture** - Autoterm handles combustion safety; ESP32 handles smart features
- **Heater control via UART** - Start/stop, power setpoint, telemetry from Autoterm Flow 5D
- **Multi-zone heating** - Independent floor, cabin air, and water heating zones
- **PID temperature control** - Smooth feedback control per zone
- **Power profiles** - Eco (quiet), Normal, and Boost modes
- **Scheduled heating** - Timed operation with day-of-week support
- **Safety monitoring** - Zone overtemperature, coolant monitoring, UART watchdog
- **Temperature management** - Multiple DS18B20 sensors for zone control
- **Optional flow monitoring** - Support for coolant flow sensor
- **Cloud connectivity** - Integrated with [Paku-IoT platform](https://github.com/ychefla/paku-iot) for remote monitoring, control, and data analytics
- **Rich telemetry** - Combines ESP32 sensor data with Autoterm internal data (fan RPM, fuel rate, error codes)

## Hardware Components

### Heater Unit

1. **Autoterm Flow 5D** (12V) - Hydronic diesel heater with integrated controller

### ESP32 Smart Controller

1. **ESP32 Development Board** - Smart layer controller (or LilyGo T-Display S3)
2. **UART connection** - ESP32 ↔ Autoterm control panel connector (GPIO 16 RX, GPIO 17 TX)

### Zone Distribution Hardware

1. **Heat Exchanger Fan** - Cabin air heating (GPIO 12, PWM control)
2. **Floor Heating Valve** - Floor loop control (GPIO 18)
3. **Water Heating Valve** - Water coil control (GPIO 19)

### Temperature Sensors (DS18B20, OneWire bus on GPIO 4)

- **Cabin Air** - Room temperature for PID control
- **Floor Supply / Return** - Floor heating zone monitoring
- **Water Tank** - Hot water zone monitoring
- **Outside** (optional) - Ambient temperature for smart control

### Optional Components

- **Flow Sensor** - Coolant flow monitoring (GPIO 13)

## Wiring Diagram

```
ESP32                    Component
-----                    ---------

=== Autoterm UART Communication ===
GPIO 16 (RX2) <--------- Autoterm TX (heater status/telemetry)
GPIO 17 (TX2) ---------> Autoterm RX (commands/setpoints)
GND           ---------> Autoterm GND

=== Zone Control Outputs ===
GPIO 12 (PWM) ---------> Heat Exchanger Fan (via MOSFET)
GPIO 18       ---------> Floor Heating Valve (via MOSFET/Relay)
GPIO 19       ---------> Water Heating Valve (via MOSFET/Relay)

=== Temperature Sensors (DS18B20, shared OneWire bus) ===
GPIO 4        ---------> DS18B20 bus + 4.7kΩ pullup
                          (cabin, floor supply, floor return,
                           water tank, outside)

=== Optional ===
GPIO 13       ---------> Flow Sensor (coolant monitoring)

GND           ---------> Common Ground
3.3V/5V       ---------> Sensor Power
```

### Power Requirements

- **ESP32**: 3.3V (powered via USB or dedicated regulator from 12V vehicle system)
- **Sensors**: 3.3V or 5V (DS18B20 can operate on both)
- **Heat Exchanger Fan**: 12V, via MOSFET (1-5A depending on fan)
- **Zone Valves**: 12V, via MOSFET/relay (low current)
- **Autoterm Flow 5D**: 12V direct from vehicle battery (separate circuit with fuse)

**NOTE**: With the hybrid approach, high-current combustion components (glow plug, fuel pump, combustion fan) are powered and controlled entirely by the Autoterm unit — no MOSFET wiring needed for those.

## Software Setup

### Prerequisites

- [PlatformIO](https://platformio.org/) installed
- ESP32 board support
- Required libraries (automatically installed via platformio.ini):
  - OneWire
  - DallasTemperature

### Building and Uploading

```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Usage

### Serial Commands

Connect to the ESP32 via serial at 115200 baud. Available commands:

- `start` - Start the heater (begins glow plug warmup sequence)
- `stop` - Stop the heater (initiates safe shutdown with cooldown)
- `status` - Print current system status

### Operation Sequence

1. **OFF** - System idle, Autoterm off
2. **STARTING** - ESP32 sends start command via UART, Autoterm handles glow plug warmup and ignition internally
3. **RUNNING** - Autoterm reports running; ESP32 manages zones via PID control, sends power setpoints via UART
4. **STOPPING** - ESP32 sends stop command; Autoterm handles safe shutdown and cooldown internally
5. **ERROR** - Autoterm or ESP32 safety condition triggered

### Safety Features

- **Combustion safety** (Autoterm): Overheat protection, flame monitoring, ignition timeout, fuel metering safety
- **Zone safety** (ESP32): Floor overheat (>50°C), water overheat (>85°C), coolant overheat (>95°C)
- **Communication watchdog** (ESP32): UART timeout detection → safe shutdown
- **Sensor validation** (ESP32): DS18B20 health monitoring for all zones

## Configuration

Edit `include/config.h` to customize:

- **Pin assignments** - Change GPIO pins for components
- **Temperature thresholds** - Adjust operating temperatures
- **Timing constants** - Modify warmup and startup times
- **PWM settings** - Configure PWM frequency and resolution

### Example Customization

```cpp
// In config.h
#define GLOW_PLUG_WARMUP_TIME 90000  // Change to 90 seconds
#define OPERATING_TEMP 650.0          // Change target temp to 650°C
#define COOLANT_MAX_TEMP 85.0         // Increase max coolant temp
```

## Project Structure

```
hydronic-heater/
├── platformio.ini          # PlatformIO configuration
├── include/
│   ├── config.h           # Pin and parameter configuration
│   ├── components.h       # Component class declarations
│   └── controller.h       # Main controller class
├── src/
│   ├── main.cpp          # Arduino main program
│   ├── components.cpp    # Component implementations
│   └── controller.cpp    # Controller logic
└── README.md             # This file
```

## Development

### Adding New Features

The modular design makes it easy to extend:

1. **New sensors**: Add sensor classes in `components.h/cpp`
2. **New control logic**: Modify state machine in `controller.cpp`
3. **New commands**: Add handlers in `main.cpp`

### Testing

Always test on a bench setup before installing in a vehicle or heating system:

1. Test each component individually
2. Verify temperature readings with known references
3. Test safety shutdowns (simulate high temperature)
4. Verify complete startup and shutdown sequences

## Safety Warnings

⚠️ **IMPORTANT SAFETY INFORMATION** ⚠️

- This system controls a diesel heating appliance — combustion safety is critical
- Combustion safety is handled by the Autoterm certified controller — do NOT bypass it
- The ESP32 smart layer manages zone distribution only
- Always include proper safety interlocks and monitoring
- Never leave a newly installed system operating unattended
- Ensure adequate ventilation and proper exhaust routing
- Install a CO detector in the vehicle
- Use appropriately rated components for all electrical connections
- Test thoroughly before regular use
- Comply with local regulations for vehicle heating equipment
- Follow Autoterm installation manual for heater unit placement and fuel/exhaust routing

## License

This project is provided as-is for educational and development purposes. Users are responsible for ensuring safe and compliant operation of any heating equipment.

## Contributing

Contributions welcome! Please test thoroughly and document any changes.