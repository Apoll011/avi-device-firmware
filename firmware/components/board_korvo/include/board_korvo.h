/**
 * @file board_korvo.h
 * @brief ESP32 Korvo v1.1 Board Abstraction Layer
 */

#pragma once

#include <cstdint>
#include <functional>
#include "driver/gpio.h"
#include "driver/adc.h"

namespace Board {

/**
 * @brief Button interface for ADC-based button detection
 */
class Button {
public:
    using ButtonCallback = std::function<void(bool pressed, float voltage)>;
    
    explicit Button(adc1_channel_t channel, float threshold = 1.5f);
    
    void init();
    void poll();
    void onStateChange(ButtonCallback callback);
    
    bool isPressed() const { return m_state; }
    float getVoltage() const { return m_last_voltage; }
    
private:
    adc1_channel_t m_channel;
    float m_threshold;
    bool m_state;
    bool m_last_raw_state;
    unsigned long m_last_debounce_time;
    float m_last_voltage;
    ButtonCallback m_callback;
    
    static constexpr unsigned long DEBOUNCE_DELAY_MS = 50;
};

/**
 * @brief Initialize the Korvo board peripherals
 */
void init();

} // namespace Board
