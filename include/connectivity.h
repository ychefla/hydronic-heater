#ifndef CONNECTIVITY_H
#define CONNECTIVITY_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>

// Forward declaration
class HydronicHeaterController;

// WiFi and MQTT Manager
class ConnectivityManager {
private:
    WiFiClient wifiClient;
    PubSubClient* mqttClient;
    WiFiManager* wifiManager;
    
    String mqttBroker;
    int mqttPort;
    String mqttUser;
    String mqttPassword;
    String topicPrefix;
    
    unsigned long lastMqttReconnect;
    unsigned long lastStatusPublish;
    unsigned long lastWifiCheck;
    
    bool mqttConnected;
    bool wifiConnected;
    
    HydronicHeaterController* controller;
    
    void reconnectMQTT();
    void checkWiFi();
    void handleMqttMessage(char* topic, byte* payload, unsigned int length);
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static ConnectivityManager* instance;

public:
    ConnectivityManager();
    void begin();
    void setController(HydronicHeaterController* ctrl);
    void setMqttBroker(String broker, int port = 1883);
    void setMqttCredentials(String user, String password);
    void setTopicPrefix(String prefix);
    
    void update();
    void publishStatus(const String& jsonStatus);
    void publishTemperature(const String& sensor, float temperature);
    void publishMode(const String& mode);
    void publishPowerLevel(int powerPercent);
    
    bool isWifiConnected();
    bool isMqttConnected();
    
    // Configuration management
    void loadConfig();
    void saveConfig();
    void factoryReset();
    
    // Time management
    void setupNTP();
    bool isTimeValid();
    String getCurrentTime();
};

// Heating Zone Controller
class HeatingZone {
private:
    String name;
    float targetTemp;
    float currentTemp;
    float maxTemp;
    int valvePin;
    bool enabled;
    bool valveOpen;
    
    uint64_t sensorAddress;  // DS18B20 unique address
    
public:
    HeatingZone(String zoneName, int pin);
    void begin();
    
    void setTarget(float temp);
    void setMaxTemp(float temp);
    void setEnabled(bool enable);
    void setSensorAddress(uint64_t address);
    
    void updateTemperature(float temp);
    void update();  // Update valve based on temperature
    
    float getTarget();
    float getCurrent();
    bool isEnabled();
    bool isValveOpen();
    String getName();
};

// Power Profile Manager
class PowerProfile {
public:
    String name;
    int powerPercent;      // 0-100%
    int fuelPumpPWM;       // Calculated PWM value
    int airFanSpeed;       // Fan speed for this power level
    
    PowerProfile(String n, int power);
    void calculate();  // Calculate PWM values from power percentage
};

// Scheduler for timed heating
class HeatingSchedule {
public:
    String name;
    bool enabled;
    uint8_t days;          // Bitmask: bit 0=Monday, bit 6=Sunday
    int startHour;
    int startMinute;
    int durationMinutes;
    String mode;           // "eco", "normal", "boost"
    
    HeatingSchedule();
    bool isActive(struct tm* timeInfo);
    bool shouldStart(struct tm* timeInfo);
    bool shouldStop(struct tm* timeInfo, unsigned long startTime);
};

// Schedule Manager
class ScheduleManager {
private:
    HeatingSchedule schedules[4];  // Support up to 4 schedules
    int scheduleCount;
    bool schedulingEnabled;
    unsigned long scheduleStartTime;
    int activeScheduleIndex;
    
public:
    ScheduleManager();
    void begin();
    
    void addSchedule(HeatingSchedule schedule);
    void removeSchedule(int index);
    void clearSchedules();
    
    void setEnabled(bool enable);
    bool isEnabled();
    
    void update(struct tm* timeInfo);
    bool isScheduleActive();
    String getActiveScheduleName();
    
    void loadFromJson(JsonDocument& doc);
    void saveToJson(JsonDocument& doc);
};

#endif