#include "controller.h"
#include "config.h"

HydronicHeaterController::HydronicHeaterController(bool enableFlow) 
    : currentState(OFF), stateStartTime(0), enableFlowSensor(enableFlow) {
    
    // Initialize components
    glowPlug = new GlowPlug(GLOW_PLUG_PIN, GLOW_PLUG_CHANNEL);
    dieselPump = new DieselPump(DIESEL_PUMP_PIN, DIESEL_PUMP_CHANNEL);  // UPDATED: Now with PWM channel
    airFan = new Fan(AIR_FAN_PIN, AIR_FAN_CHANNEL);
    coolantPump = new CoolantPump(COOLANT_PUMP_PIN);
    heatExchangerFan = new Fan(HEAT_EXCHANGER_FAN_PIN, HEAT_EXCHANGER_FAN_CHANNEL);
    
    burningChamberTemp = new TemperatureSensor(BURNING_CHAMBER_TEMP_PIN);
    coolantInputTemp = new TemperatureSensor(COOLANT_INPUT_TEMP_PIN);
    coolantOutputTemp = new TemperatureSensor(COOLANT_OUTPUT_TEMP_PIN);
    airTemp = new TemperatureSensor(AIR_TEMP_PIN);
    
    if (enableFlowSensor) {
        flowSensor = new FlowSensor(FLOW_SENSOR_PIN);
    } else {
        flowSensor = nullptr;
    }
}

HydronicHeaterController::~HydronicHeaterController() {
    delete glowPlug;
    delete dieselPump;
    delete airFan;
    delete coolantPump;
    delete heatExchangerFan;
    delete burningChamberTemp;
    delete coolantInputTemp;
    delete coolantOutputTemp;
    delete airTemp;
    if (flowSensor) delete flowSensor;
}

void HydronicHeaterController::begin() {
    Serial.println("Initializing Hydronic Heater Controller...");
    
    glowPlug->begin();
    dieselPump->begin();
    airFan->begin();
    coolantPump->begin();
    heatExchangerFan->begin();
    
    burningChamberTemp->begin();
    coolantInputTemp->begin();
    coolantOutputTemp->begin();
    airTemp->begin();
    
    if (flowSensor) {
        flowSensor->begin();
    }
    
    Serial.println("Controller initialized.");
}

void HydronicHeaterController::update() {
    static unsigned long lastTempUpdate = 0;
    static unsigned long lastSafetyCheck = 0;
    
    unsigned long currentTime = millis();
    
    // Update temperature readings
    if (currentTime - lastTempUpdate >= TEMP_READ_INTERVAL) {
        burningChamberTemp->update();
        coolantInputTemp->update();
        coolantOutputTemp->update();
        airTemp->update();
        lastTempUpdate = currentTime;
    }
    
    // Update flow sensor if enabled
    if (flowSensor) {
        flowSensor->update();
    }
    
    // Safety checks
    if (currentTime - lastSafetyCheck >= SAFETY_CHECK_INTERVAL) {
        checkSafetyConditions();
        lastSafetyCheck = currentTime;
    }
    
    // State machine update
    updateState();
}

void HydronicHeaterController::updateState() {
    switch (currentState) {
        case GLOW_PLUG_WARMUP:
            handleGlowPlugWarmup();
            break;
        case IGNITION:
            handleIgnition();
            break;
        case RUNNING:
            handleRunning();
            break;
        case SHUTDOWN:
            handleShutdown();
            break;
        case ERROR:
            // Remain in error state until reset
            break;
        case OFF:
        default:
            // Do nothing
            break;
    }
}

void HydronicHeaterController::handleGlowPlugWarmup() {
    unsigned long elapsedTime = millis() - stateStartTime;
    
    if (elapsedTime >= GLOW_PLUG_WARMUP_TIME) {
        Serial.println("Glow plug warmup complete. Starting ignition...");
        currentState = IGNITION;
        stateStartTime = millis();
        
        // Start air fan at low speed
        airFan->setSpeed(128);
        
        // Start diesel pump
        dieselPump->turnOn();
    }
}

void HydronicHeaterController::handleIgnition() {
    unsigned long elapsedTime = millis() - stateStartTime;
    float chamberTemp = burningChamberTemp->getLastReading();
    
    // Check if ignition successful
    if (chamberTemp >= IGNITION_TEMP) {
        Serial.println("Ignition successful. Entering running mode...");
        currentState = RUNNING;
        stateStartTime = millis();
        
        // Start coolant circulation
        coolantPump->turnOn();
        
        // Reduce glow plug power
        glowPlug->setPower(64);
    } 
    else if (elapsedTime >= STARTUP_SEQUENCE_TIME) {
        // Ignition failed
        Serial.println("ERROR: Ignition failed!");
        errorMessage = "Ignition timeout";
        currentState = ERROR;
        stopHeater();
    }
}

void HydronicHeaterController::handleRunning() {
    float chamberTemp = burningChamberTemp->getLastReading();
    
    // Turn off glow plug once at operating temperature
    if (chamberTemp >= OPERATING_TEMP && glowPlug->isHeating()) {
        glowPlug->turnOff();
    }
    
    // Adjust air fan speed based on temperature
    if (chamberTemp < OPERATING_TEMP - 50) {
        airFan->setSpeed(128);  // Low speed
    } else if (chamberTemp > OPERATING_TEMP + 50) {
        airFan->setSpeed(255);  // High speed
    } else {
        airFan->setSpeed(192);  // Medium speed
    }
    
    // Adjust coolant pump and heat exchanger fan
    adjustCoolantPump();
    adjustHeatExchangerFan();
}

void HydronicHeaterController::handleShutdown() {
    unsigned long elapsedTime = millis() - stateStartTime;
    
    // Stop fuel supply immediately
    dieselPump->turnOff();
    glowPlug->turnOff();
    
    // Keep fans running for cooldown
    if (elapsedTime < 60000) {  // 1 minute cooldown
        airFan->setSpeed(255);
        heatExchangerFan->setSpeed(255);
        coolantPump->turnOn();
    } else {
        // Shutdown complete
        airFan->turnOff();
        heatExchangerFan->turnOff();
        coolantPump->turnOff();
        currentState = OFF;
        Serial.println("Shutdown complete.");
    }
}

void HydronicHeaterController::checkSafetyConditions() {
    float chamberTemp = burningChamberTemp->getLastReading();
    float coolantTemp = coolantOutputTemp->getLastReading();
    
    // ========================================
    // CRITICAL SAFETY 1: Burning Chamber Overtemperature
    // ========================================
    if (chamberTemp >= MAX_SAFE_TEMP && currentState != OFF && currentState != SHUTDOWN && currentState != ERROR) {
        Serial.println("!!! CRITICAL: BURNING CHAMBER OVERTEMPERATURE !!!");
        Serial.print("Temperature: ");
        Serial.print(chamberTemp);
        Serial.println("°C");
        emergencyShutdown("CHAMBER OVERHEAT");
        return;
    }
    
    // Warning level for approaching overheat
    if (chamberTemp >= (MAX_SAFE_TEMP - 50) && currentState == RUNNING) {
        Serial.println("WARNING: Burning chamber temperature high, reducing power");
        // Reduce power by 50% as safety measure
        if (dieselPump->getSpeed() > 128) {
            dieselPump->setSpeed(128);
            airFan->setSpeed(128);
        }
    }
    
    // ========================================
    // CRITICAL SAFETY 2: Coolant Overtemperature (Boiling Prevention)
    // ========================================
    
    // Critical level: 95°C (approaching boiling at 100°C)
    if (coolantTemp >= 95.0 && currentState != OFF && currentState != SHUTDOWN && currentState != ERROR) {
        Serial.println("!!! CRITICAL: COOLANT OVERTEMPERATURE !!!");
        Serial.print("Coolant temperature: ");
        Serial.print(coolantTemp);
        Serial.println("°C - APPROACHING BOILING!");
        emergencyShutdown("COOLANT OVERHEAT");
        return;
    }
    
    // Warning level: 85°C
    if (coolantTemp >= 85.0 && coolantTemp < 95.0 && currentState == RUNNING) {
        Serial.println("WARNING: Coolant temperature high - reducing power 50%");
        // Aggressive cooling
        heatExchangerFan->setSpeed(255);  // Maximum cooling
        coolantPump->turnOn();             // Ensure circulation
        // Reduce heat input significantly
        int currentSpeed = dieselPump->getSpeed();
        dieselPump->setSpeed(currentSpeed / 2);  // 50% reduction
        airFan->setSpeed(128);
    }
    
    // Elevated level: 75°C
    if (coolantTemp >= 75.0 && coolantTemp < 85.0 && currentState == RUNNING) {
        Serial.println("NOTICE: Coolant temperature elevated - increasing cooling");
        heatExchangerFan->setSpeed(255);  // Maximum cooling
        coolantPump->turnOn();
        // Reduce power by 10%
        int currentSpeed = dieselPump->getSpeed();
        int reducedSpeed = currentSpeed * 0.9;
        if (reducedSpeed > 64) {  // Maintain minimum for stable combustion
            dieselPump->setSpeed(reducedSpeed);
        }
    }
    
    // ========================================
    // CRITICAL SAFETY 3: Fuel Depletion Detection
    // ========================================
    
    // Method 1: Temperature drop detection (primary indicator)
    static unsigned long lastFuelCheck = 0;
    static float lastChamberTempForFuel = 0;
    static int lowTempCounter = 0;
    
    if (currentState == RUNNING && millis() - lastFuelCheck >= 5000) {  // Check every 5 seconds
        // Check for sudden temperature drop (fuel depletion signature)
        if (chamberTemp < (OPERATING_TEMP - 200) &&   // 400°C drop from normal
            chamberTemp < lastChamberTempForFuel &&    // Temperature is dropping
            dieselPump->isRunning()) {                 // Pump supposedly running
            
            lowTempCounter++;
            Serial.println("WARNING: Burning chamber temperature dropping - possible fuel depletion");
            Serial.print("Counter: ");
            Serial.println(lowTempCounter);
            
            // If temperature stays low for 3 consecutive checks (15 seconds), assume fuel empty
            if (lowTempCounter >= 3) {
                Serial.println("!!! CRITICAL: FUEL DEPLETION DETECTED !!!");
                emergencyShutdown("FUEL EMPTY");
                return;
            }
        } else if (chamberTemp >= (OPERATING_TEMP - 100)) {
            // Temperature normal, reset counter
            lowTempCounter = 0;
        }
        
        lastChamberTempForFuel = chamberTemp;
        lastFuelCheck = millis();
    }
    
    // Method 2: Ignition failure pattern (backup detection)
    // This is handled in handleIgnition() with timeout
    
    // ========================================
    // CRITICAL SAFETY 4: Coolant Pump Failure Detection
    // ========================================
    
    // If heater is running and generating heat, coolant pump MUST be running
    if ((currentState == RUNNING || currentState == IGNITION) && 
        chamberTemp > IGNITION_TEMP) {  // Only check when actually generating heat
        
        if (!coolantPump->isRunning()) {
            Serial.println("!!! CRITICAL: COOLANT PUMP NOT RUNNING !!!");
            Serial.println("Heat is being generated but coolant is not circulating");
            Serial.println("Risk of overheating and component damage");
            emergencyShutdown("COOLANT PUMP FAILURE");
            return;
        }
        
        // Additional check: If flow sensor available, verify actual flow
        if (flowSensor && coolantPump->isRunning()) {
            float flowRate = flowSensor->getFlowRate();
            if (flowRate < 0.5) {  // Less than 0.5 L/min is abnormal
                Serial.println("!!! CRITICAL: LOW/NO COOLANT FLOW DETECTED !!!");
                Serial.print("Flow rate: ");
                Serial.print(flowRate);
                Serial.println(" L/min");
                Serial.println("Pump may be running but coolant not flowing");
                Serial.println("Possible blockage, air lock, or pump failure");
                emergencyShutdown("NO COOLANT FLOW");
                return;
            }
        }
    }
    
    // ========================================
    // CRITICAL SAFETY 5: Temperature Sensor Validation
    // ========================================
    
    // Enhanced sensor validation with state-specific requirements
    
    // ALWAYS CRITICAL: Burning chamber sensor (any state except OFF)
    if (!burningChamberTemp->isValidReading() && currentState != OFF && currentState != ERROR) {
        Serial.println("!!! CRITICAL: BURNING CHAMBER SENSOR FAILURE !!!");
        Serial.println("Cannot operate without combustion temperature monitoring");
        emergencyShutdown("CHAMBER SENSOR FAIL");
        return;
    }
    
    // CRITICAL during operation: Coolant sensor (when heater running)
    if ((currentState == RUNNING || currentState == IGNITION || currentState == SHUTDOWN) &&
        !coolantOutputTemp->isValidReading() && 
        !coolantInputTemp->isValidReading()) {
        Serial.println("!!! CRITICAL: ALL COOLANT SENSORS FAILED !!!");
        Serial.println("Cannot monitor coolant temperature - risk of boiling");
        emergencyShutdown("COOLANT SENSOR FAIL");
        return;
    }
    
    // Individual coolant sensor failures (warning if we have redundancy)
    if (!coolantOutputTemp->isValidReading() && coolantInputTemp->isValidReading()) {
        static unsigned long lastCoolantWarning = 0;
        if (millis() - lastCoolantWarning > 10000) {  // Warn every 10 seconds
            Serial.println("WARNING: Coolant output sensor failed - using input sensor only");
            lastCoolantWarning = millis();
        }
    }
    
    if (!coolantInputTemp->isValidReading() && coolantOutputTemp->isValidReading()) {
        static unsigned long lastCoolantWarning2 = 0;
        if (millis() - lastCoolantWarning2 > 10000) {
            Serial.println("WARNING: Coolant input sensor failed - using output sensor only");
            lastCoolantWarning2 = millis();
        }
    }
    
    // Detect if sensor readings are physically impossible (sensor malfunction)
    static float lastChamberTemp = 0;
    static unsigned long lastSensorCheck = 0;
    
    if (millis() - lastSensorCheck >= 1000) {  // Check every second
        // Check for impossible temperature jumps (sensor glitch/failure)
        if (lastChamberTemp > 0) {
            float tempChange = abs(chamberTemp - lastChamberTemp);
            // Physical limit: Heater can't change more than 100°C per second
            if (tempChange > 100.0) {
                Serial.println("!!! CRITICAL: SENSOR READING ANOMALY DETECTED !!!");
                Serial.print("Impossible temperature change: ");
                Serial.print(tempChange);
                Serial.println("°C in 1 second");
                Serial.println("Sensor malfunction or wiring issue");
                emergencyShutdown("SENSOR MALFUNCTION");
                return;
            }
        }
        lastChamberTemp = chamberTemp;
        lastSensorCheck = millis();
    }
    
    // Warning: Non-critical sensor failures
    if (!airTemp->isValidReading() && currentState != OFF) {
        static unsigned long lastAirTempWarning = 0;
        if (millis() - lastAirTempWarning > 30000) {  // Warn every 30 seconds
            Serial.println("WARNING: Air temperature sensor invalid - continuing in degraded mode");
            lastAirTempWarning = millis();
        }
    }
}

// ========================================
// EMERGENCY SHUTDOWN FUNCTION
// ========================================
void HydronicHeaterController::emergencyShutdown(String reason) {
    Serial.println("\n========================================");
    Serial.println("!!!  EMERGENCY SHUTDOWN TRIGGERED  !!!");
    Serial.println("========================================");
    Serial.print("REASON: ");
    Serial.println(reason);
    Serial.println("========================================\n");
    
    // 1. IMMEDIATE FUEL CUTOFF
    dieselPump->turnOff();
    glowPlug->turnOff();
    Serial.println("✓ Fuel delivery stopped");
    Serial.println("✓ Glow plug disabled");
    
    // 2. MAXIMUM COOLING
    airFan->setSpeed(255);
    heatExchangerFan->setSpeed(255);
    coolantPump->turnOn();
    Serial.println("✓ Cooling fans at maximum");
    Serial.println("✓ Coolant circulation active");
    
    // 3. UPDATE STATE
    currentState = ERROR;
    errorMessage = reason;
    stateStartTime = millis();
    
    // 4. Extended cooling period for emergency shutdown
    Serial.println("\nMaintaining emergency cooling...");
    Serial.println("System will remain in ERROR state");
    Serial.println("Manual intervention required to restart\n");
    
    // Note: In full implementation, this would also:
    // - Close zone valves
    // - Send MQTT critical alert
    // - Sound alarm
    // - Log to NVS
    // - Increment safety counter
}

void HydronicHeaterController::adjustCoolantPump() {
    float coolantTemp = coolantOutputTemp->getLastReading();
    
    if (coolantTemp >= COOLANT_MIN_TEMP) {
        coolantPump->turnOn();
    }
}

void HydronicHeaterController::adjustHeatExchangerFan() {
    float coolantTemp = coolantOutputTemp->getLastReading();
    
    if (coolantTemp < COOLANT_MIN_TEMP) {
        heatExchangerFan->setSpeed(0);
    } else if (coolantTemp < COOLANT_MAX_TEMP - 10) {
        heatExchangerFan->setSpeed(128);  // Low speed
    } else if (coolantTemp >= COOLANT_MAX_TEMP - 10) {
        heatExchangerFan->setSpeed(255);  // High speed
    }
}

void HydronicHeaterController::startHeater() {
    if (currentState == OFF) {
        Serial.println("Starting heater...");
        currentState = GLOW_PLUG_WARMUP;
        stateStartTime = millis();
        errorMessage = "";
        
        // Start glow plug
        glowPlug->turnOn();
    }
}

void HydronicHeaterController::stopHeater() {
    if (currentState != OFF && currentState != SHUTDOWN) {
        Serial.println("Stopping heater...");
        currentState = SHUTDOWN;
        stateStartTime = millis();
    }
}

HeaterState HydronicHeaterController::getState() {
    return currentState;
}

String HydronicHeaterController::getStateName() {
    switch (currentState) {
        case OFF: return "OFF";
        case GLOW_PLUG_WARMUP: return "GLOW_PLUG_WARMUP";
        case IGNITION: return "IGNITION";
        case RUNNING: return "RUNNING";
        case SHUTDOWN: return "SHUTDOWN";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

String HydronicHeaterController::getErrorMessage() {
    return errorMessage;
}

float HydronicHeaterController::getBurningChamberTemp() {
    return burningChamberTemp->getLastReading();
}

float HydronicHeaterController::getCoolantInputTemp() {
    return coolantInputTemp->getLastReading();
}

float HydronicHeaterController::getCoolantOutputTemp() {
    return coolantOutputTemp->getLastReading();
}

float HydronicHeaterController::getAirTemp() {
    return airTemp->getLastReading();
}

float HydronicHeaterController::getCoolantFlowRate() {
    if (flowSensor) {
        return flowSensor->getFlowRate();
    }
    return 0.0;
}

void HydronicHeaterController::printStatus() {
    Serial.println("\n===== Heater Status =====");
    Serial.print("State: ");
    Serial.println(getStateName());
    
    if (currentState == ERROR) {
        Serial.print("Error: ");
        Serial.println(errorMessage);
    }
    
    Serial.println("\nTemperatures:");
    Serial.print("  Burning Chamber: ");
    Serial.print(getBurningChamberTemp());
    Serial.println(" °C");
    
    Serial.print("  Coolant Input: ");
    Serial.print(getCoolantInputTemp());
    Serial.println(" °C");
    
    Serial.print("  Coolant Output: ");
    Serial.print(getCoolantOutputTemp());
    Serial.println(" °C");
    
    Serial.print("  Air: ");
    Serial.print(getAirTemp());
    Serial.println(" °C");
    
    Serial.println("\nComponents:");
    Serial.print("  Glow Plug: ");
    Serial.println(glowPlug->isHeating() ? "ON" : "OFF");
    
    Serial.print("  Diesel Pump: ");
    Serial.println(dieselPump->isRunning() ? "ON" : "OFF");
    
    Serial.print("  Air Fan: ");
    Serial.print(airFan->getSpeed());
    Serial.println("/255");
    
    Serial.print("  Coolant Pump: ");
    Serial.println(coolantPump->isRunning() ? "ON" : "OFF");
    
    Serial.print("  Heat Exchanger Fan: ");
    Serial.print(heatExchangerFan->getSpeed());
    Serial.println("/255");
    
    if (flowSensor) {
        Serial.print("  Coolant Flow: ");
        Serial.print(getCoolantFlowRate());
        Serial.println(" L/min");
    }
    
    Serial.println("========================\n");
}
