/**
 * @file board_korvo.cpp
 * @brief ESP32 Korvo v1.1 Board Implementation
 */

#include "board_korvo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "BOARD_KORVO";

namespace Board {

Button::Button(adc1_channel_t channel, float threshold)
    : m_channel(channel)
    , m_threshold(threshold)
    , m_state(false)
    , m_last_raw_state(false)
    , m_last_debounce_time(0)
    , m_last_voltage(0.0f)
    , m_callback(nullptr) {
}

void Button::init() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(m_channel, ADC_ATTEN_DB_11);
    ESP_LOGI(TAG, "Button initialized on ADC channel %d", m_channel);
}

void Button::poll() {
    int raw_value = adc1_get_raw(m_channel);
    m_last_voltage = raw_value * (3.3f / 4095.0f);
    bool current_raw_state = (m_last_voltage <= m_threshold);
    
    if (current_raw_state != m_last_raw_state) {
        m_last_debounce_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    }
    
    unsigned long current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if ((current_time - m_last_debounce_time) > DEBOUNCE_DELAY_MS) {
        if (current_raw_state != m_state) {
            m_state = current_raw_state;
            
            if (m_callback) {
                m_callback(m_state, m_last_voltage);
            }
        }
    }
    
    m_last_raw_state = current_raw_state;
}

void Button::onStateChange(ButtonCallback callback) {
    m_callback = callback;
}

void init() {
    ESP_LOGI(TAG, "Initializing ESP32 Korvo v1.1 board");
    // Add any board-specific initialization here
}

} // namespace Board
