#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include "components.h"

// Heater operating states
enum HeaterState {
    OFF,
    GLOW_PLUG_WARMUP,
    IGNITION,
    RUNNING,
    SHUTDOWN,
    ERROR
};

// Controller class
class HydronicHeaterController {
private:
    GlowPlug* glowPlug;
    DieselPump* dieselPump;
    Fan* airFan;
    CoolantPump* coolantPump;
    Fan* heatExchangerFan;
    
    TemperatureSensor* burningChamberTemp;
    TemperatureSensor* coolantInputTemp;
    TemperatureSensor* coolantOutputTemp;
    TemperatureSensor* airTemp;
    
    FlowSensor* flowSensor;  // Optional
    
    HeaterState currentState;
    unsigned long stateStartTime;
    String errorMessage;
    
    bool enableFlowSensor;
    
    void updateState();
    void handleGlowPlugWarmup();
    void handleIgnition();
    void handleRunning();
    void handleShutdown();
    void checkSafetyConditions();
    void adjustCoolantPump();
    void adjustHeatExchangerFan();
    
public:
    HydronicHeaterController(bool enableFlow = false);
    ~HydronicHeaterController();
    
    void begin();
    void update();
    void startHeater();
    void stopHeater();
    
    HeaterState getState();
    String getStateName();
    String getErrorMessage();
    
    // Sensor readings
    float getBurningChamberTemp();
    float getCoolantInputTemp();
    float getCoolantOutputTemp();
    float getAirTemp();
    float getCoolantFlowRate();
    
    // Status information
    void printStatus();
};

#endif
