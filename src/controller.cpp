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
    float chamberTemp = burningChamberTemp->getLastReading();
    
    // Check if warmup temperature reached OR timeout
    if (chamberTemp >= GLOW_PLUG_WARMUP_TEMP || elapsedTime >= GLOW_PLUG_WARMUP_TIME) {
        Serial.println("Glow plug warmup complete. Starting ignition...");
        Serial.print("Chamber temperature: ");
        Serial.print(chamberTemp);
        Serial.println("°C");
        currentState = IGNITION;
        stateStartTime = millis();
        
        // Start air fan at low speed
        airFan->setSpeed(128);
        
        // Start diesel pump at minimum for ignition
        dieselPump->setSpeed(FUEL_PUMP_MIN_PWM);
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
    if (chamberTemp >= OPERATING_TEMP_TARGET && glowPlug->isHeating()) {
        glowPlug->turnOff();
        Serial.println("Glow plug disabled - at operating temperature");
    }
    
    // ========================================
    // INTELLIGENT POWER CONTROL
    // ========================================
    // Adjust heating power based on multiple factors:
    // 1. Burning chamber temperature (maintain 130-230°C range)
    // 2. Coolant temperature (related to circulation)
    // 3. Room temperature (intelligent thermostat)
    
    updateIntelligentPowerControl();
    
    // Adjust coolant pump and heat exchanger fan
    adjustCoolantPump();
    adjustHeatExchangerFan();
}

// ========================================
// INTELLIGENT POWER CONTROL IMPLEMENTATION
// ========================================

void HydronicHeaterController::updateIntelligentPowerControl() {
    unsigned long currentTime = millis();
    
    // Only adjust power periodically (every 2 seconds)
    if (currentTime - lastPowerAdjustment < POWER_CONTROL_INTERVAL) {
        return;
    }
    lastPowerAdjustment = currentTime;
    
    // Get current temperatures
    float chamberTemp = burningChamberTemp->getLastReading();
    float coolantTemp = coolantOutputTemp->isValidReading() ? 
                        coolantOutputTemp->getLastReading() : 
                        coolantInputTemp->getLastReading();
    float roomTemp = airTemp->getLastReading();
    
    // Calculate power requirements from each control input
    int chamberPower = calculatePowerFromChamberTemp(chamberTemp);
    int coolantPower = calculatePowerFromCoolantTemp(coolantTemp);
    int roomPower = calculatePowerFromRoomTemp(roomTemp, roomTargetTemperature);
    
    // Use the MOST RESTRICTIVE (lowest) power requirement
    // This ensures we don't exceed any limit
    targetPowerPercent = chamberPower;
    if (coolantPower < targetPowerPercent) {
        targetPowerPercent = coolantPower;
        Serial.println("Power limited by coolant temperature");
    }
    if (roomPower < targetPowerPercent) {
        targetPowerPercent = roomPower;
        Serial.println("Power limited by room temperature");
    }
    
    // Ensure power stays within safe limits
    targetPowerPercent = constrain(targetPowerPercent, POWER_MIN_STABLE, POWER_MAX_LIMIT);
    
    // Gradually adjust current power toward target (smooth transitions)
    if (currentPowerPercent < targetPowerPercent) {
        currentPowerPercent += POWER_ADJUST_STEP;
        if (currentPowerPercent > targetPowerPercent) {
            currentPowerPercent = targetPowerPercent;
        }
        Serial.print("Increasing power to: ");
        Serial.print(currentPowerPercent);
        Serial.println("%");
    } else if (currentPowerPercent > targetPowerPercent) {
        currentPowerPercent -= POWER_ADJUST_STEP;
        if (currentPowerPercent < targetPowerPercent) {
            currentPowerPercent = targetPowerPercent;
        }
        Serial.print("Decreasing power to: ");
        Serial.print(currentPowerPercent);
        Serial.println("%");
    }
    
    // Apply the power level to fuel pump and air fan
    applyPowerLevel(currentPowerPercent);
}

// Calculate required power based on burning chamber temperature
// CRITICAL: Chamber must stay between 130°C (clean burn) and 230°C (max safe)
int HydronicHeaterController::calculatePowerFromChamberTemp(float chamberTemp) {
    // CRITICAL: Below 130°C is incomplete combustion (sooting, inefficient)
    // DO NOT reduce power below minimum if we're at or below this threshold
    if (chamberTemp < OPERATING_TEMP_MIN) {
        Serial.println("Chamber temperature below clean burn threshold - maintaining minimum power");
        return POWER_MIN_STABLE;  // Maintain minimum, do not reduce further
    }
    
    // Below target (130-180°C): Increase power to reach optimal temperature
    if (chamberTemp >= OPERATING_TEMP_MIN && chamberTemp < OPERATING_TEMP_TARGET) {
        Serial.println("Chamber temperature below target - increasing power");
        // Linear increase from min to full power
        float range = OPERATING_TEMP_TARGET - OPERATING_TEMP_MIN;
        float position = chamberTemp - OPERATING_TEMP_MIN;
        int power = POWER_MIN_STABLE + (int)((100 - POWER_MIN_STABLE) * (position / range));
        return constrain(power, POWER_MIN_STABLE, 100);
    }
    
    // At target (180°C ±10°C): Maintain moderate power
    if (chamberTemp >= (OPERATING_TEMP_TARGET - 10) && 
        chamberTemp <= (OPERATING_TEMP_TARGET + 10)) {
        return 60;  // Normal operating power
    }
    
    // Approaching max (190-220°C): Progressively reduce power
    if (chamberTemp > (OPERATING_TEMP_TARGET + 10) && chamberTemp < (OPERATING_TEMP_MAX - 10)) {
        Serial.println("Chamber temperature elevated - reducing power");
        // Linear reduction from 60% down to 30%
        float range = (OPERATING_TEMP_MAX - 10) - (OPERATING_TEMP_TARGET + 10);
        float position = chamberTemp - (OPERATING_TEMP_TARGET + 10);
        int power = 60 - (int)(30 * (position / range));
        return constrain(power, 30, 60);
    }
    
    // Near maximum (220-230°C): Minimum power only
    if (chamberTemp >= (OPERATING_TEMP_MAX - 10) && chamberTemp < OPERATING_TEMP_MAX) {
        Serial.println("Chamber temperature near maximum - minimum power only");
        return POWER_MIN_STABLE;
    }
    
    // At or above maximum (≥230°C): Critical, will trigger shutdown in safety check
    if (chamberTemp >= OPERATING_TEMP_MAX) {
        Serial.println("!!! Chamber temperature AT MAXIMUM - CRITICAL !!!");
        return POWER_MIN_STABLE;  // This will also trigger emergency shutdown
    }
    
    return 60;  // Default moderate power
}

// Calculate required power based on coolant temperature
int HydronicHeaterController::calculatePowerFromCoolantTemp(float coolantTemp) {
    // If coolant is cold, allow more power
    if (coolantTemp < COOLANT_TARGET_TEMP - 10) {
        return 100;  // Full power to warm up coolant
    }
    
    // If coolant is near target, use moderate power
    if (coolantTemp >= (COOLANT_TARGET_TEMP - 10) && 
        coolantTemp <= (COOLANT_TARGET_TEMP + 5)) {
        return 70;  // Moderate power to maintain
    }
    
    // If coolant is above target, reduce power
    if (coolantTemp > (COOLANT_TARGET_TEMP + 5) && 
        coolantTemp < COOLANT_MAX_TEMP) {
        Serial.println("Coolant temperature above target - reducing power");
        // Progressive reduction as coolant gets hotter
        float excess = coolantTemp - COOLANT_TARGET_TEMP;
        int power = 70 - (int)(excess * 3);  // Reduce ~3% per degree above target
        return constrain(power, POWER_MIN_STABLE, 70);
    }
    
    // If coolant is approaching maximum, significantly reduce power
    if (coolantTemp >= COOLANT_MAX_TEMP) {
        Serial.println("Coolant temperature at maximum - minimum power");
        return POWER_MIN_STABLE;  // Minimum stable power only
    }
    
    return 70;
}

// Calculate required power based on room temperature (intelligent thermostat)
int HydronicHeaterController::calculatePowerFromRoomTemp(float roomTemp, float target) {
    // If room temperature sensor invalid, return full power (fail-safe)
    if (!airTemp->isValidReading()) {
        return 100;
    }
    
    float tempDifference = target - roomTemp;
    
    // Room is significantly colder than target (>5°C below)
    if (tempDifference > ROOM_TEMP_OFFSET_COLD) {
        Serial.print("Room significantly cold (");
        Serial.print(roomTemp);
        Serial.print("°C vs target ");
        Serial.print(target);
        Serial.println("°C) - maximum power");
        return 100;  // Maximum power for rapid heating
    }
    
    // Room is moderately cold (2-5°C below target)
    if (tempDifference > 2.0 && tempDifference <= ROOM_TEMP_OFFSET_COLD) {
        Serial.println("Room moderately cold - high power");
        return 80;  // High power to reach target
    }
    
    // Room is slightly cold (1-2°C below target)
    if (tempDifference > ROOM_TEMP_HYSTERESIS && tempDifference <= 2.0) {
        Serial.println("Room slightly cold - moderate power");
        return 50;  // Moderate power to maintain
    }
    
    // Room is at target (within hysteresis)
    if (abs(tempDifference) <= ROOM_TEMP_HYSTERESIS) {
        Serial.println("Room at target temperature - minimum power");
        return POWER_MIN_STABLE;  // Just enough to keep warm
    }
    
    // Room is above target (too warm)
    if (tempDifference < -ROOM_TEMP_HYSTERESIS) {
        Serial.print("Room above target (");
        Serial.print(roomTemp);
        Serial.print("°C vs target ");
        Serial.print(target);
        Serial.println("°C) - minimum power");
        return POWER_MIN_STABLE;  // Minimum to avoid overheating
    }
    
    return 50;  // Default moderate power
}

// Apply power level to fuel pump and air fan
void HydronicHeaterController::applyPowerLevel(int powerPercent) {
    // Convert percentage to PWM values
    int fuelPumpPWM = map(powerPercent, 0, 100, FUEL_PUMP_MIN_PWM, FUEL_PUMP_MAX_PWM);
    int airFanSpeed = map(powerPercent, 0, 100, FAN_MIN_SPEED, FAN_MAX_SPEED);
    
    // Apply to components
    dieselPump->setSpeed(fuelPumpPWM);
    airFan->setSpeed(airFanSpeed);
    
    // Log power application
    static int lastLoggedPower = -1;
    if (powerPercent != lastLoggedPower) {
        Serial.print("Applied power level: ");
        Serial.print(powerPercent);
        Serial.print("% (Fuel PWM: ");
        Serial.print(fuelPumpPWM);
        Serial.print(", Fan: ");
        Serial.print(airFanSpeed);
        Serial.println(")");
        lastLoggedPower = powerPercent;
    }
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
    // CRITICAL SAFETY 1: Burning Chamber Temperature Limits
    // ========================================
    // Maximum safe temperature: 230°C
    if (chamberTemp >= OPERATING_TEMP_MAX && currentState != OFF && currentState != SHUTDOWN && currentState != ERROR) {
        Serial.println("!!! CRITICAL: BURNING CHAMBER MAXIMUM TEMPERATURE EXCEEDED !!!");
        Serial.print("Temperature: ");
        Serial.print(chamberTemp);
        Serial.println("°C (MAX: 230°C)");
        emergencyShutdown("CHAMBER OVERHEAT");
        return;
    }
    
    // Warning level for approaching maximum (220°C)
    if (chamberTemp >= (OPERATING_TEMP_MAX - 10) && currentState == RUNNING) {
        Serial.println("WARNING: Burning chamber temperature approaching maximum, reducing power");
        // Reduce to minimum stable power
        int minPWM = FUEL_PUMP_MIN_PWM;
        dieselPump->setSpeed(minPWM);
        airFan->setSpeed(FAN_MIN_SPEED);
    }
    
    // ========================================
    // CRITICAL SAFETY 1B: Rapid Temperature Spike Detection
    // ========================================
    // Rapid temperature increase indicates dry run or coolant circulation problem
    static float lastChamberTempForSpike = 0;
    static unsigned long lastSpikeCheck = 0;
    
    if (millis() - lastSpikeCheck >= TEMP_SPIKE_CHECK_INTERVAL && currentState == RUNNING) {
        if (lastChamberTempForSpike > 0) {
            float tempChange = chamberTemp - lastChamberTempForSpike;
            // Check for rapid increase (>50°C in 5 seconds)
            if (tempChange > TEMP_SPIKE_THRESHOLD) {
                Serial.println("!!! CRITICAL: RAPID TEMPERATURE SPIKE DETECTED !!!");
                Serial.print("Temperature increased by ");
                Serial.print(tempChange);
                Serial.println("°C in 5 seconds");
                Serial.println("Possible dry run or coolant circulation failure");
                emergencyShutdown("RAPID TEMP SPIKE");
                return;
            }
        }
        lastChamberTempForSpike = chamberTemp;
        lastSpikeCheck = millis();
    }
    
    // ========================================
    // CRITICAL SAFETY 1C: Minimum Operating Temperature
    // ========================================
    // Below 130°C indicates incomplete combustion (but don't reduce power)
    // This is handled in calculatePowerFromChamberTemp() - power is NOT reduced below minimum
    static unsigned long lastLowTempWarning = 0;
    if (chamberTemp < OPERATING_TEMP_MIN && currentState == RUNNING && 
        millis() - lastLowTempWarning > 10000) {
        Serial.println("WARNING: Chamber temperature below clean burn threshold (130°C)");
        Serial.println("Incomplete combustion may occur - maintaining minimum power");
        lastLowTempWarning = millis();
    }
    
    // ========================================
    // CRITICAL SAFETY 2: Coolant Overtemperature (Boiling Prevention)
    // ========================================
    
    // Critical level: 95°C (approaching boiling at 100°C)
    if (coolantTemp >= COOLANT_CRITICAL_TEMP && currentState != OFF && currentState != SHUTDOWN && currentState != ERROR) {
        Serial.println("!!! CRITICAL: COOLANT OVERTEMPERATURE !!!");
        Serial.print("Coolant temperature: ");
        Serial.print(coolantTemp);
        Serial.println("°C - APPROACHING BOILING!");
        emergencyShutdown("COOLANT OVERHEAT");
        return;
    }
    
    // Warning level: 85°C
    if (coolantTemp >= COOLANT_WARNING_TEMP && coolantTemp < COOLANT_CRITICAL_TEMP && currentState == RUNNING) {
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
    if (coolantTemp >= 75.0 && coolantTemp < COOLANT_WARNING_TEMP && currentState == RUNNING) {
        Serial.println("NOTICE: Coolant temperature elevated - increasing cooling");
        heatExchangerFan->setSpeed(255);  // Maximum cooling
        coolantPump->turnOn();
        // Reduce power by 10%
        int currentSpeed = dieselPump->getSpeed();
        int reducedSpeed = currentSpeed * 0.9;
        if (reducedSpeed > FUEL_PUMP_MIN_PWM) {  // Maintain minimum for stable combustion
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
        // With corrected temps: drop below 100°C when should be 130-230°C
        if (chamberTemp < (OPERATING_TEMP_MIN - 30) &&   // Below 100°C (should be above 130°C)
            chamberTemp < lastChamberTempForFuel &&       // Temperature is dropping
            dieselPump->isRunning()) {                    // Pump supposedly running
            
            lowTempCounter++;
            Serial.println("WARNING: Burning chamber temperature dropping - possible fuel depletion");
            Serial.print("Current: ");
            Serial.print(chamberTemp);
            Serial.print("°C, Last: ");
            Serial.print(lastChamberTempForFuel);
            Serial.print("°C, Counter: ");
            Serial.println(lowTempCounter);
            
            // If temperature stays low for 3 consecutive checks (15 seconds), assume fuel empty
            if (lowTempCounter >= 3) {
                Serial.println("!!! CRITICAL: FUEL DEPLETION DETECTED !!!");
                emergencyShutdown("FUEL EMPTY");
                return;
            }
        } else if (chamberTemp >= OPERATING_TEMP_MIN) {
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
            // Physical limit: With corrected temps (130-230°C), heater can't change more than 20°C per second
            if (tempChange > MAX_TEMP_CHANGE_PER_SEC) {
                Serial.println("!!! CRITICAL: SENSOR READING ANOMALY DETECTED !!!");
                Serial.print("Impossible temperature change: ");
                Serial.print(tempChange);
                Serial.println("°C in 1 second");
                Serial.print("(Physical limit: ");
                Serial.print(MAX_TEMP_CHANGE_PER_SEC);
                Serial.println("°C/s)");
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
