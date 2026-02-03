/**
 * @file board_korvo.cpp
 * @brief ESP32 Korvo v1.1 Board Implementation
 */

#include "board_korvo.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cmath>

static const char* TAG = "BOARD_KORVO";

namespace Board {

// Constructor
ButtonController::ButtonController(adc1_channel_t channel, uint8_t num_buttons,
                                   const float* thresholds, float tolerance)
    : m_channel(channel)
    , m_num_buttons(num_buttons)
    , m_thresholds(thresholds)
    , m_tolerance(tolerance)
    , m_current_button(-1)
    , m_last_button(-1)
    , m_last_debounce_time(0)
    , m_last_voltage(0.0f)
    , m_callback(nullptr) {
}

// Initialization
void ButtonController::init() {
    // Note: In newer ESP-IDF versions, consider using esp_adc/adc_oneshot.h
    // For now, using legacy driver/adc.h as defined in header
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(m_channel, ADC_ATTEN_DB_11);
    
    ESP_LOGI(TAG, "Button controller initialized on ADC channel %d (%d buttons)", 
             m_channel, m_num_buttons);
}

// Internal helper to map voltage to button index
int8_t ButtonController::detectButton(float voltage) {
    // Check if no button is pressed (voltage pulled up high, usually > 3.0V on 3.3V logic)
    if (voltage > 3.0f) {
        return -1;
    }
    
    // Find which button matches the voltage within tolerance
    for (uint8_t i = 0; i < m_num_buttons; i++) {
        float lower = m_thresholds[i] - m_tolerance;
        float upper = m_thresholds[i] + m_tolerance;
        
        if (voltage >= lower && voltage <= upper) {
            return i;
        }
    }
    
    return -1;  // No match found
}

// Main polling loop
void ButtonController::poll() {
    // Read ADC value and convert to voltage
    int raw_value = adc1_get_raw(m_channel);
    m_last_voltage = raw_value * (3.3f / 4095.0f);
    
	// Detect which button (if any) is pressed
    int8_t detected_button = detectButton(m_last_voltage);
    
    // Debouncing logic
    unsigned long current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    if (detected_button != m_last_button) {
        m_last_debounce_time = current_time;
    }
    
    if ((current_time - m_last_debounce_time) > DEBOUNCE_DELAY_MS) {
        // State has been stable for > DEBOUNCE_DELAY_MS
        
        if (detected_button != m_current_button) {
            // Button state actually changed
            
            // 1. If a button was previously pressed, report release
            if (m_current_button >= 0 && m_callback) {
                m_callback(m_current_button, false);
            }
            
            // 2. Update current button
            m_current_button = detected_button;
            
            // 3. If a new button is pressed, report press
            if (m_current_button >= 0 && m_callback) {
                m_callback(m_current_button, true);
            }
        }
    }
    
    m_last_button = detected_button;
}

// Register callback
void ButtonController::onButtonEvent(ButtonCallback callback) {
    m_callback = callback;
}

// Board Global Init
void init() {
    ESP_LOGI(TAG, "Initializing ESP32 Korvo v1.1 board");
    // Add any specific board power-on sequences here if needed
}

} // namespace Board
