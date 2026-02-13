# Paku-IoT Integration Guide
## Hydronic Heater Controller Integration with Paku Cloud Platform

**Version**: 1.1  
**Date**: February 2026  
**Purpose**: Document integration between ESP32 Hydronic Heater Controller and Paku-IoT cloud platform

> **Note (February 2026)**: This integration guide remains valid for the Autoterm Flow 5D hybrid architecture. The MQTT topic structure and telemetry format are unchanged. The ESP32 now additionally publishes Autoterm UART telemetry (combustion temp, fan RPM, fuel rate, error codes) alongside DS18B20 zone sensor data. See [AUTOTERM_FLOW_5D_GUIDE.md](AUTOTERM_FLOW_5D_GUIDE.md) for the updated architecture.

---

## 1. Overview

This document describes the integration architecture between the ESP32 hydronic heater controller and the **Paku-IoT** cloud platform (https://github.com/ychefla/paku-iot), enabling cloud-based monitoring, control, and data analytics for camper van heating systems.

### 1.1 Integration Architecture

```
┌──────────────────────────────────────────────────────────────┐
│                    Camper Van (Edge)                         │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │     ESP32 Hydronic Heater Controller                   │ │
│  │  - Temperature monitoring (5+ sensors)                 │ │
│  │  - Intelligent power control                           │ │
│  │  - Safety systems (5 critical)                         │ │
│  │  - MQTT client                                         │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │ WiFi/4G                                  │
└───────────────────┼──────────────────────────────────────────┘
                    │ MQTT over TLS
                    │ (JSON payloads)
┌───────────────────▼──────────────────────────────────────────┐
│                  Paku-IoT Cloud Platform                     │
│                                                              │
│  ┌────────────────────────────────────────────────────────┐ │
│  │  MQTT Broker (mosquitto / AWS IoT / Azure IoT Hub)    │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│  ┌────────────────▼───────────────────────────────────────┐ │
│  │  Paku-IoT Core (Python)                               │ │
│  │  - Device management                                   │ │
│  │  - Data ingestion & validation                        │ │
│  │  - Real-time processing                               │ │
│  │  - Command routing                                     │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│  ┌────────────────▼───────────────────────────────────────┐ │
│  │  Data Storage & Analytics                             │ │
│  │  - Time-series database (InfluxDB / TimescaleDB)      │ │
│  │  - Historical data                                     │ │
│  │  - Aggregations & reports                             │ │
│  └────────────────┬───────────────────────────────────────┘ │
│                   │                                          │
│  ┌────────────────▼───────────────────────────────────────┐ │
│  │  User Interface / Dashboards                          │ │
│  │  - Web dashboard (Grafana / Custom)                   │ │
│  │  - Mobile app                                          │ │
│  │  - Alerts & notifications                             │ │
│  └───────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. MQTT Topic Structure for Paku-IoT

### 2.1 Topic Naming Convention

Following Paku-IoT standards, topics use hierarchical structure:

```
paku/{organization}/{location}/{device_type}/{device_id}/{data_type}
```

**For Hydronic Heater**:
```
paku/{org}/{location}/heater/{heater_id}/{data_type}
```

**Example**:
```
paku/mycompany/van001/heater/heater01/telemetry
paku/mycompany/van001/heater/heater01/status
paku/mycompany/van001/heater/heater01/command
paku/mycompany/van001/heater/heater01/alert
```

### 2.2 Topic Definitions

#### Telemetry Topics (Device → Cloud)

**Temperature Telemetry**:
```
Topic: paku/{org}/{location}/heater/{id}/telemetry/temperature
Frequency: Every 5 seconds
Payload:
{
  "timestamp": "2025-12-28T08:00:00Z",
  "device_id": "heater01",
  "temperatures": {
    "burning_chamber": 625.5,
    "coolant_input": 65.2,
    "coolant_output": 72.8,
    "cabin_air": 20.5,
    "floor": 24.0,
    "water_tank": 42.0
  },
  "units": "celsius"
}
```

**Status Telemetry**:
```
Topic: paku/{org}/{location}/heater/{id}/telemetry/status
Frequency: Every 10 seconds
Payload:
{
  "timestamp": "2025-12-28T08:00:00Z",
  "device_id": "heater01",
  "state": "RUNNING",
  "power_percent": 60,
  "components": {
    "glow_plug": "OFF",
    "diesel_pump": 153,
    "air_fan": 192,
    "coolant_pump": "ON",
    "heat_exchanger_fan": 255
  },
  "runtime_seconds": 3600,
  "uptime_seconds": 86400
}
```

**Safety Telemetry**:
```
Topic: paku/{org}/{location}/heater/{id}/telemetry/safety
Frequency: Every 5 seconds
Payload:
{
  "timestamp": "2025-12-28T08:00:00Z",
  "device_id": "heater01",
  "safety_status": "OK",
  "warnings": [],
  "last_check": "2025-12-28T07:59:55Z",
  "safety_shutdowns_count": 0
}
```

**Performance Telemetry**:
```
Topic: paku/{org}/{location}/heater/{id}/telemetry/performance
Frequency: Every 30 seconds
Payload:
{
  "timestamp": "2025-12-28T08:00:00Z",
  "device_id": "heater01",
  "fuel_consumption": {
    "current_lph": 0.3,
    "total_liters": 5.2,
    "estimated_remaining_hours": 25.6
  },
  "efficiency": {
    "heat_output_kw": 3.5,
    "coolant_flow_lpm": 3.2,
    "temperature_delta": 7.6
  }
}
```

#### Command Topics (Cloud → Device)

**Control Commands**:
```
Topic: paku/{org}/{location}/heater/{id}/command/control
Payload:
{
  "command_id": "cmd_12345",
  "timestamp": "2025-12-28T08:00:00Z",
  "action": "set_mode",
  "parameters": {
    "mode": "continuous",
    "target_temperature": 21.0
  }
}

Actions:
- "start" - Start heater
- "stop" - Stop heater
- "set_mode" - Set operating mode (manual/continuous/scheduled)
- "set_target" - Set target temperature
- "set_power" - Set power level (20-100%)
- "set_profile" - Set power profile (eco/normal/boost)
- "emergency_stop" - Emergency shutdown
```

**Configuration Commands**:
```
Topic: paku/{org}/{location}/heater/{id}/command/config
Payload:
{
  "command_id": "cmd_12346",
  "timestamp": "2025-12-28T08:00:00Z",
  "config_type": "temperature_limits",
  "parameters": {
    "coolant_target": 70.0,
    "room_target": 20.0,
    "chamber_max": 700.0
  }
}
```

#### Response Topics (Device → Cloud)

**Command Acknowledgment**:
```
Topic: paku/{org}/{location}/heater/{id}/response/command
Payload:
{
  "command_id": "cmd_12345",
  "timestamp": "2025-12-28T08:00:01Z",
  "status": "success",
  "message": "Mode changed to continuous",
  "execution_time_ms": 150
}
```

#### Alert Topics (Device → Cloud)

**Safety Alerts**:
```
Topic: paku/{org}/{location}/heater/{id}/alert/safety
Priority: High (QoS 2, Retained)
Payload:
{
  "alert_id": "alert_98765",
  "timestamp": "2025-12-28T08:00:00Z",
  "severity": "critical",
  "type": "overheat",
  "message": "Chamber temperature exceeded 900°C - emergency shutdown triggered",
  "data": {
    "temperature": 905.3,
    "threshold": 900.0,
    "action_taken": "emergency_shutdown"
  }
}

Severities: info, warning, critical
Types: overheat, coolant_overheat, fuel_empty, pump_failure, sensor_failure
```

---

## 3. Device Registration with Paku-IoT

### 3.1 Device Metadata

Each heater device registers with Paku-IoT platform with following metadata:

```json
{
  "device_id": "heater01",
  "device_type": "hydronic_diesel_heater",
  "manufacturer": "DIY",
  "model": "ESP32_Controller_v1.0",
  "location": {
    "organization": "mycompany",
    "site": "van001",
    "description": "Camper Van Hydronic Heater"
  },
  "capabilities": {
    "sensors": [
      "burning_chamber_temp",
      "coolant_input_temp",
      "coolant_output_temp",
      "cabin_air_temp",
      "floor_temp",
      "water_tank_temp",
      "coolant_flow_rate"
    ],
    "actuators": [
      "glow_plug",
      "diesel_pump",
      "air_fan",
      "coolant_pump",
      "heat_exchanger_fan"
    ],
    "control_modes": [
      "manual",
      "continuous",
      "scheduled"
    ],
    "power_profiles": [
      "eco",
      "normal",
      "boost"
    ],
    "safety_features": [
      "chamber_overheat_protection",
      "coolant_boiling_prevention",
      "fuel_depletion_detection",
      "pump_failure_detection",
      "sensor_validation"
    ]
  },
  "specifications": {
    "power_range": {
      "min_kw": 0.4,
      "max_kw": 5.0
    },
    "temperature_range": {
      "min_celsius": -20,
      "max_celsius": 900
    },
    "fuel_type": "diesel"
  },
  "firmware": {
    "version": "1.0.0",
    "build_date": "2025-12-28",
    "features": [
      "intelligent_power_control",
      "multi_zone_heating",
      "mqtt_connectivity"
    ]
  }
}
```

### 3.2 Device Authentication

**MQTT Authentication Methods**:

1. **Username/Password** (Basic)
   ```cpp
   mqttClient.connect("heater01", "username", "password");
   ```

2. **TLS Client Certificates** (Recommended)
   ```cpp
   wifiClientSecure.setCACert(ca_cert);
   wifiClientSecure.setCertificate(client_cert);
   wifiClientSecure.setPrivateKey(client_key);
   mqttClient.connect("heater01");
   ```

3. **Token-Based** (OAuth2 / JWT)
   ```cpp
   String token = getAuthToken();
   mqttClient.connect("heater01", token.c_str(), nullptr);
   ```

---

## 4. Implementation in ESP32 Controller

### 4.1 Configuration for Paku-IoT

Add to `include/config.h`:

```cpp
// ============================================
// PAKU-IOT INTEGRATION CONFIGURATION
// ============================================

// Organization and location
#define PAKU_ORG "mycompany"
#define PAKU_LOCATION "van001"
#define PAKU_DEVICE_ID "heater01"

// MQTT Topics (Paku-IoT format)
#define PAKU_TOPIC_BASE "paku/" PAKU_ORG "/" PAKU_LOCATION "/heater/" PAKU_DEVICE_ID

// Telemetry topics
#define PAKU_TOPIC_TELEMETRY_TEMP PAKU_TOPIC_BASE "/telemetry/temperature"
#define PAKU_TOPIC_TELEMETRY_STATUS PAKU_TOPIC_BASE "/telemetry/status"
#define PAKU_TOPIC_TELEMETRY_SAFETY PAKU_TOPIC_BASE "/telemetry/safety"
#define PAKU_TOPIC_TELEMETRY_PERF PAKU_TOPIC_BASE "/telemetry/performance"

// Command topics
#define PAKU_TOPIC_COMMAND_CTRL PAKU_TOPIC_BASE "/command/control"
#define PAKU_TOPIC_COMMAND_CONFIG PAKU_TOPIC_BASE "/command/config"

// Response topics
#define PAKU_TOPIC_RESPONSE PAKU_TOPIC_BASE "/response/command"

// Alert topics
#define PAKU_TOPIC_ALERT_SAFETY PAKU_TOPIC_BASE "/alert/safety"

// Telemetry intervals
#define PAKU_TEMP_INTERVAL 5000      // 5 seconds
#define PAKU_STATUS_INTERVAL 10000   // 10 seconds
#define PAKU_SAFETY_INTERVAL 5000    // 5 seconds
#define PAKU_PERF_INTERVAL 30000     // 30 seconds
```

### 4.2 Enhanced ConnectivityManager for Paku-IoT

Extend `include/connectivity.h`:

```cpp
class PakuIoTManager {
private:
    ConnectivityManager* connectivity;
    String organization;
    String location;
    String deviceId;
    
    unsigned long lastTempPublish;
    unsigned long lastStatusPublish;
    unsigned long lastSafetyPublish;
    unsigned long lastPerfPublish;
    
public:
    PakuIoTManager(ConnectivityManager* conn);
    void begin(String org, String loc, String devId);
    void update();
    
    // Telemetry publishing
    void publishTemperatureTelemetry(JsonDocument& tempData);
    void publishStatusTelemetry(JsonDocument& statusData);
    void publishSafetyTelemetry(JsonDocument& safetyData);
    void publishPerformanceTelemetry(JsonDocument& perfData);
    
    // Alert publishing
    void publishSafetyAlert(String severity, String type, String message, JsonDocument& data);
    
    // Command handling
    void handleCommand(JsonDocument& command);
    void sendCommandResponse(String commandId, String status, String message);
    
    // Device registration
    void registerDevice();
    void sendHeartbeat();
};
```

### 4.3 Example Implementation

Create `src/paku_integration.cpp`:

```cpp
#include "connectivity.h"
#include "controller.h"

void PakuIoTManager::update() {
    unsigned long now = millis();
    
    // Temperature telemetry
    if (now - lastTempPublish >= PAKU_TEMP_INTERVAL) {
        StaticJsonDocument<512> doc;
        doc["timestamp"] = getCurrentISO8601();
        doc["device_id"] = deviceId;
        
        JsonObject temps = doc.createNestedObject("temperatures");
        temps["burning_chamber"] = controller->getBurningChamberTemp();
        temps["coolant_input"] = controller->getCoolantInputTemp();
        temps["coolant_output"] = controller->getCoolantOutputTemp();
        temps["cabin_air"] = controller->getAirTemp();
        
        doc["units"] = "celsius";
        
        publishTemperatureTelemetry(doc);
        lastTempPublish = now;
    }
    
    // Status telemetry
    if (now - lastStatusPublish >= PAKU_STATUS_INTERVAL) {
        StaticJsonDocument<512> doc;
        doc["timestamp"] = getCurrentISO8601();
        doc["device_id"] = deviceId;
        doc["state"] = controller->getStateName();
        doc["power_percent"] = controller->getPowerPercent();
        
        JsonObject components = doc.createNestedObject("components");
        // Add component states
        
        publishStatusTelemetry(doc);
        lastStatusPublish = now;
    }
    
    // Safety telemetry
    if (now - lastSafetyPublish >= PAKU_SAFETY_INTERVAL) {
        StaticJsonDocument<256> doc;
        doc["timestamp"] = getCurrentISO8601();
        doc["device_id"] = deviceId;
        doc["safety_status"] = controller->getState() == ERROR ? "ERROR" : "OK";
        
        publishSafetyTelemetry(doc);
        lastSafetyPublish = now;
    }
}

void PakuIoTManager::publishSafetyAlert(String severity, String type, String message, JsonDocument& data) {
    StaticJsonDocument<512> alert;
    alert["alert_id"] = "alert_" + String(millis());
    alert["timestamp"] = getCurrentISO8601();
    alert["severity"] = severity;
    alert["type"] = type;
    alert["message"] = message;
    alert["data"] = data;
    
    String payload;
    serializeJson(alert, payload);
    
    connectivity->publish(PAKU_TOPIC_ALERT_SAFETY, payload, true, 2);  // QoS 2, retained
}
```

---

## 5. Paku-IoT Cloud Side Integration

### 5.1 Device Handler (Python)

Example `paku-iot` device handler:

```python
# paku_iot/handlers/heater_handler.py

from paku_iot.core import DeviceHandler, Telemetry, Command
import json
from datetime import datetime

class HeaterDeviceHandler(DeviceHandler):
    """Handler for ESP32 Hydronic Heater devices"""
    
    device_type = "hydronic_diesel_heater"
    
    def __init__(self, mqtt_client, db_client):
        super().__init__(mqtt_client, db_client)
        self.subscribe_topics = [
            "paku/+/+/heater/+/telemetry/#",
            "paku/+/+/heater/+/alert/#"
        ]
        
    def on_telemetry(self, topic, payload):
        """Handle incoming telemetry data"""
        data = json.loads(payload)
        
        # Extract device info from topic
        parts = topic.split('/')
        org = parts[1]
        location = parts[2]
        device_id = parts[4]
        telemetry_type = parts[6]
        
        # Store in time-series database
        self.store_telemetry(
            device_id=device_id,
            telemetry_type=telemetry_type,
            timestamp=data['timestamp'],
            data=data
        )
        
        # Check for anomalies
        if telemetry_type == "temperature":
            self.check_temperature_anomalies(device_id, data)
        
        # Update real-time dashboard
        self.update_dashboard(device_id, telemetry_type, data)
    
    def on_alert(self, topic, payload):
        """Handle safety alerts"""
        alert = json.loads(payload)
        
        # Log alert
        self.log_alert(alert)
        
        # Send notifications
        if alert['severity'] == 'critical':
            self.send_critical_notification(alert)
        
        # Store for analytics
        self.store_alert(alert)
    
    def send_command(self, device_id, action, parameters):
        """Send command to heater device"""
        org, location = self.get_device_location(device_id)
        
        command = {
            "command_id": self.generate_command_id(),
            "timestamp": datetime.utcnow().isoformat() + "Z",
            "action": action,
            "parameters": parameters
        }
        
        topic = f"paku/{org}/{location}/heater/{device_id}/command/control"
        self.mqtt_client.publish(topic, json.dumps(command), qos=1)
        
        return command['command_id']
    
    def check_temperature_anomalies(self, device_id, data):
        """Detect abnormal temperature patterns"""
        temps = data['temperatures']
        
        # Example: Check for rapid temperature rise
        if temps['burning_chamber'] > 850:
            self.create_warning(
                device_id,
                "High chamber temperature approaching limit",
                {"temperature": temps['burning_chamber']}
            )
```

### 5.2 Data Storage Schema

**InfluxDB Schema** (Time-series data):

```
Measurement: heater_telemetry
Tags:
  - org
  - location
  - device_id
  - telemetry_type
Fields:
  - burning_chamber_temp
  - coolant_input_temp
  - coolant_output_temp
  - cabin_air_temp
  - power_percent
  - state
  - ...

Example Query:
SELECT mean("burning_chamber_temp") 
FROM "heater_telemetry" 
WHERE "device_id" = 'heater01' 
  AND time > now() - 1h 
GROUP BY time(5m)
```

**PostgreSQL Schema** (Metadata and alerts):

```sql
-- Devices table
CREATE TABLE heater_devices (
    device_id VARCHAR(50) PRIMARY KEY,
    org VARCHAR(50),
    location VARCHAR(50),
    model VARCHAR(100),
    firmware_version VARCHAR(20),
    registered_at TIMESTAMP,
    last_seen TIMESTAMP
);

-- Alerts table
CREATE TABLE heater_alerts (
    alert_id VARCHAR(50) PRIMARY KEY,
    device_id VARCHAR(50),
    timestamp TIMESTAMP,
    severity VARCHAR(20),
    type VARCHAR(50),
    message TEXT,
    data JSONB,
    acknowledged BOOLEAN DEFAULT FALSE
);

-- Commands table
CREATE TABLE heater_commands (
    command_id VARCHAR(50) PRIMARY KEY,
    device_id VARCHAR(50),
    issued_at TIMESTAMP,
    action VARCHAR(50),
    parameters JSONB,
    status VARCHAR(20),
    response TEXT,
    completed_at TIMESTAMP
);
```

---

## 6. Dashboard and Visualization

### 6.1 Grafana Dashboard Configuration

Example Grafana dashboard panels:

**Temperature Panel**:
```json
{
  "title": "Heater Temperatures",
  "targets": [{
    "query": "SELECT burning_chamber_temp, coolant_output_temp, cabin_air_temp FROM heater_telemetry WHERE device_id = '$device' AND $timeFilter"
  }],
  "type": "graph"
}
```

**Power Control Panel**:
```json
{
  "title": "Power Level",
  "targets": [{
    "query": "SELECT power_percent FROM heater_telemetry WHERE device_id = '$device' AND $timeFilter"
  }],
  "type": "gauge",
  "thresholds": [
    { "value": 0, "color": "green" },
    { "value": 70, "color": "yellow" },
    { "value": 90, "color": "red" }
  ]
}
```

---

## 7. Mobile App Integration

### 7.1 Mobile App Features

- Real-time monitoring (temperatures, status, power)
- Remote control (start/stop, mode changes, power profiles)
- Alerts and notifications
- Historical data and trends
- Schedule management

### 7.2 API Endpoints

Paku-IoT REST API for mobile apps:

```
GET    /api/devices/{device_id}/status
GET    /api/devices/{device_id}/telemetry/latest
GET    /api/devices/{device_id}/telemetry/history?from=&to=
POST   /api/devices/{device_id}/command
GET    /api/devices/{device_id}/alerts
PATCH  /api/devices/{device_id}/config
```

---

## 8. Security Considerations

### 8.1 Communication Security

- **TLS/SSL**: All MQTT communication encrypted
- **Certificate-based auth**: Client certificates for devices
- **Token rotation**: Regular token refresh for devices
- **Network isolation**: Devices in separate VLAN/VPC

### 8.2 Access Control

- **Role-based access** (RBAC) in Paku-IoT
- **Device permissions**: Separate read/write/admin
- **Audit logging**: All commands and changes logged

---

## 9. Testing and Validation

### 9.1 Integration Testing

```python
# tests/test_heater_integration.py

def test_telemetry_publishing():
    """Test that heater publishes telemetry correctly"""
    # Start heater simulator
    # Subscribe to telemetry topics
    # Verify data format and frequency
    
def test_command_execution():
    """Test sending commands to heater"""
    # Send start command
    # Verify heater responds
    # Check status changes
    
def test_alert_handling():
    """Test safety alert propagation"""
    # Trigger overheat condition
    # Verify alert published
    # Check notification sent
```

---

## 10. Deployment Guide

### 10.1 ESP32 Configuration

1. Configure WiFi credentials
2. Set Paku-IoT broker address
3. Configure organization/location/device_id
4. Upload certificates (if using TLS)
5. Flash firmware

### 10.2 Cloud Configuration

1. Register device in Paku-IoT
2. Configure MQTT broker ACLs
3. Set up database schemas
4. Deploy device handler
5. Configure dashboards

---

## 11. Future Enhancements

### 11.1 Advanced Features for Paku-IoT

- **Predictive maintenance**: ML models for failure prediction
- **Fuel optimization**: AI-powered efficiency recommendations
- **Multi-device coordination**: Manage fleet of heaters
- **Weather integration**: Adjust heating based on forecast
- **Energy monitoring**: Track power consumption and costs

### 11.2 Edge Computing

- Local data processing on ESP32
- Offline operation with data buffering
- Edge AI for anomaly detection
- Local control even without cloud

---

## 12. References

**Project Links**:
- Paku Project: https://github.com/ychefla/paku
- Paku-IoT: https://github.com/ychefla/paku-iot
- Paku Project Board: https://github.com/users/ychefla/projects/3

**Standards**:
- MQTT 3.1.1 / 5.0
- JSON Schema
- ISO 8601 timestamps
- TLS 1.2+

---

**Last Updated**: December 28, 2025  
**Status**: Integration specification complete, ready for implementation  
**Next Steps**: Implement PakuIoTManager class, test with Paku-IoT cloud instance
