#include "components.h"
#include "config.h"

// GlowPlug Implementation
GlowPlug::GlowPlug(int pin, int pwmChannel) : pin(pin), pwmChannel(pwmChannel), isActive(false), activationTime(0) {}

void GlowPlug::begin() {
    ledcSetup(pwmChannel, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(pin, pwmChannel);
    turnOff();
}

void GlowPlug::setPower(int pwmValue) {
    pwmValue = constrain(pwmValue, 0, 255);
    ledcWrite(pwmChannel, pwmValue);
    if (pwmValue > 0 && !isActive) {
        isActive = true;
        activationTime = millis();
    } else if (pwmValue == 0) {
        isActive = false;
    }
}

void GlowPlug::turnOn() {
    setPower(255);
}

void GlowPlug::turnOff() {
    setPower(0);
}

bool GlowPlug::isHeating() {
    return isActive;
}

unsigned long GlowPlug::getHeatingTime() {
    if (!isActive) return 0;
    return millis() - activationTime;
}

// DieselPump Implementation
DieselPump::DieselPump(int pin) : pin(pin), isActive(false) {}

void DieselPump::begin() {
    pinMode(pin, OUTPUT);
    turnOff();
}

void DieselPump::turnOn() {
    digitalWrite(pin, HIGH);
    isActive = true;
}

void DieselPump::turnOff() {
    digitalWrite(pin, LOW);
    isActive = false;
}

bool DieselPump::isRunning() {
    return isActive;
}

// Fan Implementation
Fan::Fan(int pin, int pwmChannel) : pin(pin), pwmChannel(pwmChannel), isActive(false), currentSpeed(0) {}

void Fan::begin() {
    ledcSetup(pwmChannel, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(pin, pwmChannel);
    turnOff();
}

void Fan::setSpeed(int speed) {
    speed = constrain(speed, 0, 255);
    currentSpeed = speed;
    ledcWrite(pwmChannel, speed);
    isActive = (speed > 0);
}

void Fan::turnOff() {
    setSpeed(0);
}

bool Fan::isRunning() {
    return isActive;
}

int Fan::getSpeed() {
    return currentSpeed;
}

// TemperatureSensor Implementation
TemperatureSensor::TemperatureSensor(int pin) : lastReading(-127.0), lastReadTime(0), isValid(false) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
}

void TemperatureSensor::begin() {
    sensors->begin();
}

float TemperatureSensor::readTemperature() {
    sensors->requestTemperatures();
    float temp = sensors->getTempCByIndex(0);
    
    if (temp != DEVICE_DISCONNECTED_C && temp > -55.0 && temp < 125.0) {
        lastReading = temp;
        lastReadTime = millis();
        isValid = true;
    } else {
        isValid = false;
    }
    
    return lastReading;
}

float TemperatureSensor::getLastReading() {
    return lastReading;
}

bool TemperatureSensor::isValidReading() {
    return isValid;
}

void TemperatureSensor::update() {
    readTemperature();
}

// CoolantPump Implementation
CoolantPump::CoolantPump(int pin) : pin(pin), isActive(false) {}

void CoolantPump::begin() {
    pinMode(pin, OUTPUT);
    turnOff();
}

void CoolantPump::turnOn() {
    digitalWrite(pin, HIGH);
    isActive = true;
}

void CoolantPump::turnOff() {
    digitalWrite(pin, LOW);
    isActive = false;
}

bool CoolantPump::isRunning() {
    return isActive;
}

// FlowSensor Implementation
FlowSensor* FlowSensor::instance = nullptr;

FlowSensor::FlowSensor(int pin) : pin(pin), pulseCount(0), flowRate(0.0), lastFlowCheck(0) {
    instance = this;
}

void IRAM_ATTR FlowSensor::pulseCounter() {
    if (instance != nullptr) {
        instance->pulseCount++;
    }
}

void FlowSensor::begin() {
    pinMode(pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pin), pulseCounter, FALLING);
    lastFlowCheck = millis();
}

float FlowSensor::getFlowRate() {
    return flowRate;
}

void FlowSensor::update() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - lastFlowCheck;
    
    if (elapsedTime >= 1000) {  // Update every second
        // Flow rate calculation (pulses per second * calibration factor)
        // Typical calibration: ~7.5 pulses per liter for common flow sensors
        flowRate = (pulseCount / 7.5) * (60000.0 / elapsedTime);  // L/min
        
        pulseCount = 0;
        lastFlowCheck = currentTime;
    }
}
