#include "controller.h"
#include "config.h"

HydronicHeaterController::HydronicHeaterController(bool enableFlow) 
    : currentState(OFF), stateStartTime(0), enableFlowSensor(enableFlow) {
    
    // Initialize components
    glowPlug = new GlowPlug(GLOW_PLUG_PIN, GLOW_PLUG_CHANNEL);
    dieselPump = new DieselPump(DIESEL_PUMP_PIN);
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
    
    // Check for overheat
    if (chamberTemp >= MAX_SAFE_TEMP && currentState != OFF && currentState != SHUTDOWN) {
        Serial.println("ERROR: Overheat detected!");
        errorMessage = "Chamber temperature too high";
        currentState = ERROR;
        stopHeater();
    }
    
    // Check for sensor failures
    if (!burningChamberTemp->isValidReading() && currentState != OFF) {
        Serial.println("WARNING: Burning chamber sensor invalid!");
    }
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
