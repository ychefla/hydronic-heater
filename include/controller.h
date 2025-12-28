#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "components.h"
#include "connectivity.h"

// Forward declarations
class ConnectivityManager;
class HeatingZone;
class PowerProfile;
class ScheduleManager;

// Heater operating states
enum HeaterState {
    OFF,
    GLOW_PLUG_WARMUP,
    IGNITION,
    RUNNING,
    SHUTDOWN,
    ERROR
};

// Operating modes (NEW)
enum OperatingMode {
    MODE_MANUAL,        // Manual on/off
    MODE_CONTINUOUS,    // Maintain temperature continuously
    MODE_SCHEDULED,     // Follow schedule
    MODE_OFF_MODE       // Explicitly off
};

// Controller class
class HydronicHeaterController {
private:
    // Existing components
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
    
    // NEW: Multi-zone support
    HeatingZone* floorZone;
    HeatingZone* waterZone;
    HeatingZone* cabinZone;  // Uses airTemp sensor
    
    // NEW: Connectivity
    ConnectivityManager* connectivity;
    
    // NEW: Power management
    PowerProfile* currentProfile;
    PowerProfile* ecoProfile;
    PowerProfile* normalProfile;
    PowerProfile* boostProfile;
    int currentPowerPercent;  // 0-100%
    
    // NEW: Scheduling
    ScheduleManager* scheduler;
    
    // State management
    HeaterState currentState;
    OperatingMode operatingMode;
    unsigned long stateStartTime;
    String errorMessage;
    
    // NEW: Temperature control
    float targetTemperature;
    float roomTargetTemperature;         // NEW: Room temperature target
    float coolantTargetTemperature;      // NEW: Coolant temperature target
    float pidIntegral;
    float pidLastError;
    unsigned long lastPidUpdate;
    unsigned long lastPowerAdjustment;   // NEW: For intelligent power control
    
    // NEW: Intelligent power control state
    int currentPowerPercent;             // Current power level (20-100%)
    int targetPowerPercent;              // Target power based on control logic
    
    bool enableFlowSensor;
    bool enableZones;
    
    // Existing methods
    void updateState();
    void handleGlowPlugWarmup();
    void handleIgnition();
    void handleRunning();
    void handleShutdown();
    void checkSafetyConditions();
    void adjustCoolantPump();
    void adjustHeatExchangerFan();
    
    // CRITICAL: Emergency shutdown for safety
    void emergencyShutdown(String reason);
    
    // NEW: Intelligent power control methods
    void updateIntelligentPowerControl();
    int calculatePowerFromChamberTemp(float chamberTemp);
    int calculatePowerFromCoolantTemp(float coolantTemp);
    int calculatePowerFromRoomTemp(float roomTemp, float target);
    void applyPowerLevel(int powerPercent);
    
    // NEW: Extended control methods
    void updatePowerControl();
    void updateZones();
    void updateTemperatureControl();
    float calculatePID(float current, float target);
    void setPowerLevel(int percent);
    void applyPowerProfile(PowerProfile* profile);
    
public:
    HydronicHeaterController(bool enableFlow = false, bool enableMultiZone = false);
    ~HydronicHeaterController();
    
    void begin();
    void update();
    void startHeater();
    void stopHeater();
    
    // NEW: Mode control
    void setOperatingMode(OperatingMode mode);
    OperatingMode getOperatingMode();
    String getOperatingModeName();
    
    // NEW: Power control
    void setPowerPercent(int percent);  // 0-100%
    int getPowerPercent();
    void setProfile(String profileName);  // "eco", "normal", "boost"
    String getCurrentProfileName();
    
    // NEW: Temperature control
    void setTargetTemperature(float temp);
    float getTargetTemperature();
    
    // NEW: Zone control
    void setZoneTarget(String zoneName, float temp);
    void setZoneEnabled(String zoneName, bool enabled);
    float getZoneTemperature(String zoneName);
    bool isZoneEnabled(String zoneName);
    
    // NEW: Connectivity
    void setConnectivity(ConnectivityManager* conn);
    void publishStatusMQTT();
    void handleMQTTCommand(String command, String value);
    
    // NEW: Scheduling
    void setScheduler(ScheduleManager* sched);
    void updateSchedule();
    
    // Existing interface
    HeaterState getState();
    String getStateName();
    String getErrorMessage();
    
    // Sensor readings
    float getBurningChamberTemp();
    float getCoolantInputTemp();
    float getCoolantOutputTemp();
    float getAirTemp();
    float getCoolantFlowRate();
    
    // NEW: JSON status
    String getStatusJSON();
    
    // Status information
    void printStatus();
};

#endif
