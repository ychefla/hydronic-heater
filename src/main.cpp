#include <Arduino.h>
#include "controller.h"

// Create controller instance (set to true to enable flow sensor)
HydronicHeaterController* controller;

// Command buffer for serial control
String commandBuffer = "";

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("\n\n================================");
    Serial.println("Hydronic Diesel Heater Controller");
    Serial.println("ESP32 Based Control System");
    Serial.println("================================\n");
    
    // Initialize controller (change to true if flow sensor is installed)
    controller = new HydronicHeaterController(false);
    controller->begin();
    
    Serial.println("\nCommands:");
    Serial.println("  start  - Start the heater");
    Serial.println("  stop   - Stop the heater");
    Serial.println("  status - Print current status");
    Serial.println("================================\n");
}

void loop() {
    // Update controller
    controller->update();
    
    // Handle serial commands
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (commandBuffer.length() > 0) {
                processCommand(commandBuffer);
                commandBuffer = "";
            }
        } else {
            commandBuffer += c;
        }
    }
    
    // Print status periodically
    static unsigned long lastStatusPrint = 0;
    if (millis() - lastStatusPrint >= 10000) {  // Every 10 seconds
        controller->printStatus();
        lastStatusPrint = millis();
    }
    
    delay(100);
}

void processCommand(String command) {
    command.trim();
    command.toLowerCase();
    
    if (command == "start") {
        Serial.println("Command: Starting heater...");
        controller->startHeater();
    } 
    else if (command == "stop") {
        Serial.println("Command: Stopping heater...");
        controller->stopHeater();
    } 
    else if (command == "status") {
        controller->printStatus();
    } 
    else {
        Serial.print("Unknown command: ");
        Serial.println(command);
    }
}
