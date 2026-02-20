/**
 * @file can_bus.cpp
 * @brief Generic ESP32 TWAI (CAN 2.0B) driver implementation.
 *
 * Uses the ESP-IDF TWAI driver API (documented):
 * https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html
 *
 * This is a generic transport layer — no Autoterm-specific protocol logic.
 * The CIVD protocol layer will be added when Autoterm releases the CAN
 * adapter documentation.
 */

#include "can_bus.h"
#include <string.h>

CanBus::CanBus(gpio_num_t txPin, gpio_num_t rxPin)
    : txPin_(txPin), rxPin_(rxPin), installed_(false),
      stats_{}, filterEnabled_(false), filterCode_(0),
      filterMask_(0), filterExtended_(false)
{}

bool CanBus::begin(CanBitrate bitrate, twai_mode_t mode) {
    if (installed_) {
        Serial.println("[CAN] Already running — call stop() first");
        return false;
    }

    // General configuration
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(txPin_, rxPin_, mode);
    g_config.rx_queue_len = 32;
    g_config.tx_queue_len = 16;

    // Timing configuration
    twai_timing_config_t t_config = bitrateToTiming(bitrate);

    // Filter configuration
    twai_filter_config_t f_config;
    if (filterEnabled_) {
        f_config.acceptance_code = filterCode_;
        f_config.acceptance_mask = filterMask_;
        f_config.single_filter = true;
    } else {
        f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    }

    // Install driver
    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("[CAN] Driver install failed: %s\n", esp_err_to_name(err));
        return false;
    }

    // Start driver
    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("[CAN] Start failed: %s\n", esp_err_to_name(err));
        twai_driver_uninstall();
        return false;
    }

    installed_ = true;
    memset(&stats_, 0, sizeof(stats_));

    Serial.printf("[CAN] Started on TX=GPIO%d RX=GPIO%d @ %lu kbps\n",
                  txPin_, rxPin_, static_cast<uint32_t>(bitrate));
    return true;
}

void CanBus::stop() {
    if (!installed_) return;

    twai_stop();
    twai_driver_uninstall();
    installed_ = false;
    Serial.println("[CAN] Stopped");
}

bool CanBus::send(uint32_t id, const uint8_t* data, uint8_t len,
                  uint32_t timeoutMs) {
    CanFrame frame;
    frame.id  = id;
    frame.dlc = (len > 8) ? 8 : len;
    if (data && len > 0) {
        memcpy(frame.data, data, frame.dlc);
    }
    return send(frame, timeoutMs);
}

bool CanBus::send(const CanFrame& frame, uint32_t timeoutMs) {
    if (!installed_) return false;

    twai_message_t msg = {};
    msg.identifier = frame.id;
    msg.data_length_code = frame.dlc;
    msg.extd = frame.extended ? 1 : 0;
    msg.rtr  = frame.rtr ? 1 : 0;
    memcpy(msg.data, frame.data, frame.dlc);

    esp_err_t err = twai_transmit(&msg, pdMS_TO_TICKS(timeoutMs));
    if (err == ESP_OK) {
        stats_.txCount++;
        return true;
    }

    stats_.txErrors++;
    if (err != ESP_ERR_TIMEOUT) {
        Serial.printf("[CAN] TX error: %s\n", esp_err_to_name(err));
    }
    return false;
}

bool CanBus::receive(CanFrame& frame, uint32_t timeoutMs) {
    if (!installed_) return false;

    twai_message_t msg;
    esp_err_t err = twai_receive(&msg, pdMS_TO_TICKS(timeoutMs));
    if (err != ESP_OK) return false;

    frame.id        = msg.identifier;
    frame.extended  = msg.extd;
    frame.rtr       = msg.rtr;
    frame.dlc       = msg.data_length_code;
    frame.timestamp = millis();
    memcpy(frame.data, msg.data, frame.dlc);

    stats_.rxCount++;
    return true;
}

bool CanBus::recoverBusOff() {
    if (!installed_) return false;
    esp_err_t err = twai_initiate_recovery();
    if (err == ESP_OK) {
        Serial.println("[CAN] Bus-off recovery initiated");
        return true;
    }
    Serial.printf("[CAN] Recovery failed: %s\n", esp_err_to_name(err));
    return false;
}

void CanBus::setFilter(uint32_t id, uint32_t mask, bool extended) {
    filterEnabled_  = true;
    filterCode_     = id;
    filterMask_     = mask;
    filterExtended_ = extended;
}

void CanBus::clearFilter() {
    filterEnabled_ = false;
    filterCode_    = 0;
    filterMask_    = 0;
}

CanBusState CanBus::getState() const {
    if (!installed_) return CanBusState::Stopped;

    twai_status_info_t status;
    if (twai_get_status_info(&status) != ESP_OK) {
        return CanBusState::Stopped;
    }

    switch (status.state) {
        case TWAI_STATE_RUNNING:    return CanBusState::Running;
        case TWAI_STATE_BUS_OFF:    return CanBusState::BusOff;
        case TWAI_STATE_RECOVERING: return CanBusState::Recovering;
        case TWAI_STATE_STOPPED:    return CanBusState::Stopped;
        default:                    return CanBusState::Stopped;
    }
}

void CanBus::getErrorCounters(uint32_t& txErrorCount,
                               uint32_t& rxErrorCount) const {
    if (!installed_) {
        txErrorCount = 0;
        rxErrorCount = 0;
        return;
    }

    twai_status_info_t status;
    if (twai_get_status_info(&status) == ESP_OK) {
        txErrorCount = status.tx_error_counter;
        rxErrorCount = status.rx_error_counter;
    } else {
        txErrorCount = 0;
        rxErrorCount = 0;
    }
}

void CanBus::printStatus() const {
    Serial.println("--- CAN Bus Status ---");

    if (!installed_) {
        Serial.println("  State: NOT INSTALLED");
        Serial.println("----------------------");
        return;
    }

    twai_status_info_t status;
    if (twai_get_status_info(&status) != ESP_OK) {
        Serial.println("  State: ERROR (cannot read)");
        Serial.println("----------------------");
        return;
    }

    const char* stateStr;
    switch (status.state) {
        case TWAI_STATE_RUNNING:    stateStr = "RUNNING";    break;
        case TWAI_STATE_BUS_OFF:    stateStr = "BUS-OFF";    break;
        case TWAI_STATE_RECOVERING: stateStr = "RECOVERING"; break;
        case TWAI_STATE_STOPPED:    stateStr = "STOPPED";    break;
        default:                    stateStr = "UNKNOWN";    break;
    }

    Serial.printf("  State:       %s\n", stateStr);
    Serial.printf("  TX queued:   %lu\n", (unsigned long)status.msgs_to_tx);
    Serial.printf("  RX queued:   %lu\n", (unsigned long)status.msgs_to_rx);
    Serial.printf("  TX errors:   %lu\n", (unsigned long)status.tx_error_counter);
    Serial.printf("  RX errors:   %lu\n", (unsigned long)status.rx_error_counter);
    Serial.printf("  TX count:    %lu\n", stats_.txCount);
    Serial.printf("  RX count:    %lu\n", stats_.rxCount);
    Serial.printf("  TX failures: %lu\n", stats_.txErrors);
    Serial.printf("  RX missed:   %lu\n", stats_.rxMissed);
    Serial.println("----------------------");
}

twai_timing_config_t CanBus::bitrateToTiming(CanBitrate bitrate) const {
    switch (bitrate) {
        case CanBitrate::CAN_125KBPS:  return TWAI_TIMING_CONFIG_125KBITS();
        case CanBitrate::CAN_250KBPS:  return TWAI_TIMING_CONFIG_250KBITS();
        case CanBitrate::CAN_500KBPS:  return TWAI_TIMING_CONFIG_500KBITS();
        case CanBitrate::CAN_1MBPS:    return TWAI_TIMING_CONFIG_1MBITS();
        default:                       return TWAI_TIMING_CONFIG_250KBITS();
    }
}
