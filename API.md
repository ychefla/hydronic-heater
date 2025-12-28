# API Documentation

This document describes the classes and methods available in the hydronic heater controller.

## Table of Contents

1. [Component Classes](#component-classes)
2. [Controller Class](#controller-class)
3. [Configuration](#configuration)
4. [State Machine](#state-machine)

---

## Component Classes

### GlowPlug

Controls the glow plug with PWM for heating control.

#### Constructor
```cpp
GlowPlug(int pin, int pwmChannel)
```

#### Methods

**`void begin()`**
- Initializes PWM and pin configuration
- Call once in setup

**`void setPower(int pwmValue)`**
- Sets glow plug power level
- Parameters: `pwmValue` - 0 to 255 (0=off, 255=full power)

**`void turnOn()`**
- Turns glow plug to full power (255)

**`void turnOff()`**
- Turns glow plug off (0)

**`bool isHeating()`**
- Returns: `true` if glow plug is active

**`unsigned long getHeatingTime()`**
- Returns: milliseconds glow plug has been active

---

### DieselPump

Controls the diesel fuel pump.

#### Constructor
```cpp
DieselPump(int pin)
```

#### Methods

**`void begin()`**
- Initializes pin as output
- Call once in setup

**`void turnOn()`**
- Activates fuel pump

**`void turnOff()`**
- Deactivates fuel pump

**`bool isRunning()`**
- Returns: `true` if pump is active

---

### Fan

Controls fans with PWM for variable speed (air supply fan, heat exchanger fan).

#### Constructor
```cpp
Fan(int pin, int pwmChannel)
```

#### Methods

**`void begin()`**
- Initializes PWM and pin configuration
- Call once in setup

**`void setSpeed(int speed)`**
- Sets fan speed
- Parameters: `speed` - 0 to 255 (0=off, 255=max speed)

**`void turnOff()`**
- Stops the fan (speed 0)

**`bool isRunning()`**
- Returns: `true` if fan is active

**`int getSpeed()`**
- Returns: current fan speed (0-255)

---

### TemperatureSensor

Handles DS18B20 temperature sensors via OneWire protocol.

#### Constructor
```cpp
TemperatureSensor(int pin)
```

#### Methods

**`void begin()`**
- Initializes OneWire and DallasTemperature libraries
- Call once in setup

**`float readTemperature()`**
- Reads current temperature from sensor
- Returns: temperature in Celsius
- Returns: -127.0 if sensor error

**`float getLastReading()`**
- Returns: last valid temperature reading
- Does not trigger new reading

**`bool isValidReading()`**
- Returns: `true` if last reading was valid

**`void update()`**
- Convenience method to read and update temperature
- Equivalent to `readTemperature()`

---

### CoolantPump

Controls the coolant circulation pump.

#### Constructor
```cpp
CoolantPump(int pin)
```

#### Methods

**`void begin()`**
- Initializes pin as output
- Call once in setup

**`void turnOn()`**
- Activates coolant pump

**`void turnOff()`**
- Deactivates coolant pump

**`bool isRunning()`**
- Returns: `true` if pump is active

---

### FlowSensor

Monitors coolant flow rate using pulse counter (optional).

#### Constructor
```cpp
FlowSensor(int pin)
```

#### Methods

**`void begin()`**
- Initializes pin and interrupt
- Call once in setup

**`float getFlowRate()`**
- Returns: current flow rate in liters per minute

**`void update()`**
- Updates flow rate calculation
- Call regularly in loop (at least once per second)

---

## Controller Class

### HydronicHeaterController

Main controller class that manages all components and state machine.

#### Constructor
```cpp
HydronicHeaterController(bool enableFlowSensor = false)
```
- Parameters: `enableFlowSensor` - set to `true` if flow sensor is installed

#### Initialization

**`void begin()`**
- Initializes all components
- Call once in setup after Serial.begin()

#### Main Loop

**`void update()`**
- Main state machine update
- Updates all sensors
- Performs safety checks
- Call repeatedly in loop()

#### Control Methods

**`void startHeater()`**
- Initiates heater startup sequence
- Only works when state is OFF

**`void stopHeater()`**
- Initiates safe shutdown sequence
- Stops fuel supply and begins cooldown

#### State Information

**`HeaterState getState()`**
- Returns: current state enum value

**`String getStateName()`**
- Returns: human-readable state name

**`String getErrorMessage()`**
- Returns: error description if in ERROR state

#### Sensor Readings

**`float getBurningChamberTemp()`**
- Returns: burning chamber temperature in °C

**`float getCoolantInputTemp()`**
- Returns: coolant input temperature in °C

**`float getCoolantOutputTemp()`**
- Returns: coolant output temperature in °C

**`float getAirTemp()`**
- Returns: air temperature in °C

**`float getCoolantFlowRate()`**
- Returns: coolant flow rate in L/min (0.0 if sensor not enabled)

#### Status Output

**`void printStatus()`**
- Prints comprehensive status to Serial
- Includes all temperatures, component states, and flow rate

---

## Configuration

Configuration constants are defined in `include/config.h`.

### Pin Definitions

| Constant | Default | Description |
|----------|---------|-------------|
| `GLOW_PLUG_PIN` | 25 | Glow plug PWM output |
| `DIESEL_PUMP_PIN` | 26 | Diesel pump control |
| `AIR_FAN_PIN` | 27 | Air fan PWM output |
| `COOLANT_PUMP_PIN` | 14 | Coolant pump control |
| `HEAT_EXCHANGER_FAN_PIN` | 12 | Heat exchanger fan PWM |
| `BURNING_CHAMBER_TEMP_PIN` | 4 | DS18B20 sensor |
| `COOLANT_INPUT_TEMP_PIN` | 16 | DS18B20 sensor |
| `COOLANT_OUTPUT_TEMP_PIN` | 17 | DS18B20 sensor |
| `AIR_TEMP_PIN` | 5 | DS18B20 sensor |
| `FLOW_SENSOR_PIN` | 13 | Flow sensor input |

### Temperature Thresholds

| Constant | Default | Description |
|----------|---------|-------------|
| `GLOW_PLUG_WARMUP_TEMP` | 800.0°C | Target glow plug temp |
| `IGNITION_TEMP` | 300.0°C | Ignition success threshold |
| `OPERATING_TEMP` | 600.0°C | Normal operating temp |
| `MAX_SAFE_TEMP` | 900.0°C | Emergency shutdown temp |
| `COOLANT_MIN_TEMP` | 40.0°C | Min temp for circulation |
| `COOLANT_MAX_TEMP` | 80.0°C | Max coolant temp |

### Timing Constants

| Constant | Default | Description |
|----------|---------|-------------|
| `GLOW_PLUG_WARMUP_TIME` | 60000ms | Warmup duration |
| `STARTUP_SEQUENCE_TIME` | 120000ms | Max ignition time |
| `TEMP_READ_INTERVAL` | 1000ms | Sensor read frequency |
| `SAFETY_CHECK_INTERVAL` | 500ms | Safety check frequency |

---

## State Machine

### States

**`OFF`**
- Initial state
- All components off
- Waiting for start command

**`GLOW_PLUG_WARMUP`**
- Glow plug heating at full power
- Duration: 60 seconds (configurable)
- Preparing for ignition

**`IGNITION`**
- Glow plug still active
- Diesel pump active
- Air fan at low speed
- Waiting for temperature rise
- Timeout: 2 minutes

**`RUNNING`**
- Normal operation
- Glow plug reduced or off (temp dependent)
- Diesel pump active
- Air fan speed modulated by temperature
- Coolant pump active
- Heat exchanger fan active (temp dependent)

**`SHUTDOWN`**
- Diesel pump off
- Glow plug off
- Fans at high speed for cooldown
- Duration: 60 seconds
- Prevents damage from residual heat

**`ERROR`**
- Emergency state
- All fuel delivery stopped
- Cooling systems active
- Requires reset to recover
- Error message available via `getErrorMessage()`

### State Transitions

```
OFF → start() → GLOW_PLUG_WARMUP
GLOW_PLUG_WARMUP → (timer) → IGNITION
IGNITION → (temp threshold) → RUNNING
IGNITION → (timeout) → ERROR
RUNNING → stop() → SHUTDOWN
ANY → (overtemp) → ERROR
SHUTDOWN → (timer) → OFF
```

### Safety Features

1. **Overtemperature Protection**
   - Triggers at `MAX_SAFE_TEMP` (900°C)
   - Enters ERROR state
   - Stops fuel delivery immediately

2. **Ignition Timeout**
   - If temperature doesn't reach `IGNITION_TEMP` within 2 minutes
   - Enters ERROR state
   - Prevents fuel waste and flooding

3. **Sensor Validation**
   - Checks for disconnected sensors
   - Warns but doesn't stop (allows degraded operation)

4. **Cooldown Cycle**
   - Mandatory 60-second cooldown
   - Prevents thermal shock
   - Protects glow plug and chamber

---

## Usage Example

```cpp
#include <Arduino.h>
#include "controller.h"

HydronicHeaterController* controller;

void setup() {
    Serial.begin(115200);
    
    // Initialize with flow sensor disabled
    controller = new HydronicHeaterController(false);
    controller->begin();
}

void loop() {
    // Update controller (required)
    controller->update();
    
    // Example: Start heater if cold
    if (controller->getState() == OFF) {
        if (controller->getAirTemp() < 10.0) {
            controller->startHeater();
        }
    }
    
    // Example: Stop if too hot
    if (controller->getCoolantOutputTemp() > 85.0) {
        controller->stopHeater();
    }
    
    // Print status every 10 seconds
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 10000) {
        controller->printStatus();
        lastPrint = millis();
    }
}
```

---

## Advanced Customization

### Modifying Control Logic

Edit `src/controller.cpp`:

- `handleGlowPlugWarmup()` - Warmup phase logic
- `handleIgnition()` - Ignition phase logic  
- `handleRunning()` - Normal operation logic
- `handleShutdown()` - Shutdown logic
- `checkSafetyConditions()` - Safety monitoring
- `adjustCoolantPump()` - Coolant pump control
- `adjustHeatExchangerFan()` - Heat exchanger control

### Adding Custom States

1. Add to `HeaterState` enum in `controller.h`
2. Add case in `updateState()` switch statement
3. Implement handler method
4. Add state name in `getStateName()`

### Custom Safety Checks

Add checks in `checkSafetyConditions()`:

```cpp
void HydronicHeaterController::checkSafetyConditions() {
    // Existing checks...
    
    // Custom check example
    if (coolantFlowRate < 1.0 && coolantPump->isRunning()) {
        errorMessage = "Low coolant flow";
        currentState = ERROR;
        stopHeater();
    }
}
```
