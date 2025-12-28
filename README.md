# Hydronic Diesel Heater Controller

ESP32-based controller system for hydronic diesel heaters that replaces the original controller. This system provides complete control over all heater components with safety monitoring and automatic operation sequences.

## Documentation Structure

This project follows a design-first approach. Please review documents in this order:

1. **[REQUIREMENTS.md](REQUIREMENTS.md)** - System requirements specification
2. **[DESIGN.md](DESIGN.md)** - System architecture and design decisions
3. **README.md** (this file) - Implementation overview and features
4. **[WIRING.md](WIRING.md)** - Hardware wiring guide
5. **[QUICKSTART.md](QUICKSTART.md)** - Quick setup guide
6. **[API.md](API.md)** - API documentation for developers
7. **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem diagnosis and solutions
8. **[PAKU_INTEGRATION.md](PAKU_INTEGRATION.md)** - 🆕 Paku-IoT cloud platform integration guide

## Features

- **Complete heater control** - Manages all aspects of heater operation
- **Automatic startup sequence** - Glow plug warmup, ignition, and running modes
- **Safety monitoring** - Overtemperature protection and sensor validation
- **Temperature management** - Multiple temperature sensors for different zones
- **Coolant circulation control** - Automatic pump and heat exchanger fan control
- **Optional flow monitoring** - Support for coolant flow sensor
- **Serial interface** - Simple command interface for control and monitoring
- **🆕 Cloud connectivity** - Integrated with [Paku-IoT platform](https://github.com/ychefla/paku-iot) for remote monitoring, control, and data analytics

## Hardware Components

### Required Components

1. **ESP32 Development Board** - Main controller
2. **Glow Plug** - Pre-heats combustion chamber (GPIO 25, PWM control)
3. **Diesel Pump** - Fuel delivery (GPIO 26)
4. **Air Fan** - Provides air to burning chamber (GPIO 27, PWM control)
5. **Coolant Pump** - Circulates coolant (GPIO 14)
6. **Heat Exchanger Fan** - Transfers heat to air (GPIO 12, PWM control)

### Temperature Sensors (DS18B20)

- **Burning Chamber** - GPIO 4
- **Coolant Input** - GPIO 16
- **Coolant Output** - GPIO 17
- **Air Temperature** - GPIO 5

### Optional Components

- **Flow Sensor** - Coolant flow monitoring (GPIO 13)

## Wiring Diagram

```
ESP32                    Component
-----                    ---------
GPIO 25 (PWM) ---------> Glow Plug (via MOSFET/Relay)
GPIO 26       ---------> Diesel Pump (via MOSFET/Relay)
GPIO 27 (PWM) ---------> Air Fan (via MOSFET)
GPIO 14       ---------> Coolant Pump (via MOSFET/Relay)
GPIO 12 (PWM) ---------> Heat Exchanger Fan (via MOSFET)

GPIO 4        ---------> DS18B20 (Burning Chamber) + 4.7kΩ pullup
GPIO 16       ---------> DS18B20 (Coolant Input) + 4.7kΩ pullup
GPIO 17       ---------> DS18B20 (Coolant Output) + 4.7kΩ pullup
GPIO 5        ---------> DS18B20 (Air) + 4.7kΩ pullup

GPIO 13       ---------> Flow Sensor (optional)

GND           ---------> Common Ground
3.3V/5V       ---------> Sensor Power
```

### Power Requirements

- **ESP32**: 3.3V (powered via USB or dedicated regulator)
- **Sensors**: 3.3V or 5V (DS18B20 can operate on both)
- **High-power components**: Use appropriate MOSFETs or relays rated for the component's voltage and current
  - Glow Plug: Typically 12V, high current (50-100A)
  - Diesel Pump: 12V, moderate current (2-5A)
  - Fans: 12V, varies by fan (1-5A each)

**WARNING**: Never connect high-power components directly to ESP32 pins! Always use appropriate drivers (MOSFETs, relays, motor controllers) with proper ratings.

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

1. **OFF** - System idle
2. **GLOW_PLUG_WARMUP** (60 seconds) - Glow plug heats combustion chamber
3. **IGNITION** (up to 2 minutes) - Fuel and air introduced, ignition attempted
4. **RUNNING** - Normal operation with automatic temperature control
5. **SHUTDOWN** - Safe cooldown with fans running for 1 minute
6. **ERROR** - Safety condition triggered, system stopped

### Safety Features

- **Overheat Protection**: Automatically shuts down if burning chamber exceeds 900°C
- **Ignition Timeout**: Stops if ignition doesn't occur within 2 minutes
- **Sensor Validation**: Monitors sensor health and reports issues
- **Cooldown Cycle**: Ensures safe shutdown by running fans during cooldown

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

- This system controls high-temperature combustion equipment
- Always include proper safety interlocks and monitoring
- Never leave operating heater unattended
- Ensure adequate ventilation for combustion products
- Use appropriately rated components for all high-power connections
- Test thoroughly before actual use
- Consider adding redundant safety systems (external temperature cutoffs, fuel shutoff valves)
- Comply with local regulations for heating equipment

## License

This project is provided as-is for educational and development purposes. Users are responsible for ensuring safe and compliant operation of any heating equipment.

## Contributing

Contributions welcome! Please test thoroughly and document any changes.