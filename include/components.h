#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Glow Plug Controller
class GlowPlug {
private:
    int pin;
    int pwmChannel;
    bool isActive;
    unsigned long activationTime;

public:
    GlowPlug(int pin, int pwmChannel);
    void begin();
    void setPower(int pwmValue);  // 0-255
    void turnOn();
    void turnOff();
    bool isHeating();
    unsigned long getHeatingTime();
};

// Diesel Pump Controller
class DieselPump {
private:
    int pin;
    bool isActive;

public:
    DieselPump(int pin);
    void begin();
    void turnOn();
    void turnOff();
    bool isRunning();
};

// Fan Controller (generic for air supply and heat exchanger)
class Fan {
private:
    int pin;
    int pwmChannel;
    bool isActive;
    int currentSpeed;  // 0-255

public:
    Fan(int pin, int pwmChannel);
    void begin();
    void setSpeed(int speed);  // 0-255
    void turnOff();
    bool isRunning();
    int getSpeed();
};

// Temperature Sensor Handler
class TemperatureSensor {
private:
    OneWire* oneWire;
    DallasTemperature* sensors;
    float lastReading;
    unsigned long lastReadTime;
    bool isValid;

public:
    TemperatureSensor(int pin);
    void begin();
    float readTemperature();
    float getLastReading();
    bool isValidReading();
    void update();
};

// Coolant Pump Controller
class CoolantPump {
private:
    int pin;
    bool isActive;

public:
    CoolantPump(int pin);
    void begin();
    void turnOn();
    void turnOff();
    bool isRunning();
};

// Flow Sensor Handler (optional)
class FlowSensor {
private:
    int pin;
    volatile unsigned int pulseCount;
    float flowRate;
    unsigned long lastFlowCheck;
    static FlowSensor* instance;
    
    static void IRAM_ATTR pulseCounter();

public:
    FlowSensor(int pin);
    void begin();
    float getFlowRate();  // Returns flow rate in L/min
    void update();
};

#endif
