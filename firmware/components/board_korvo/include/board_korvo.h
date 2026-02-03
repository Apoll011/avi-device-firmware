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
 * @brief Multi-button controller for resistor ladder ADC input
 * 
 * The Korvo v1.1 has 6 buttons connected via a resistor ladder to GPIO36 (ADC1_CH0).
 * Each button produces a different voltage level when pressed.
 */
class ButtonController {
public:
    using ButtonCallback = std::function<void(uint8_t button_id, bool pressed)>;
    
    ButtonController(adc1_channel_t channel, uint8_t num_buttons, 
                     const float* thresholds, float tolerance);
    
    void init();
    void poll();
    void onButtonEvent(ButtonCallback callback);
    
    int8_t getPressedButton() const { return m_current_button; }
    float getVoltage() const { return m_last_voltage; }
    
private:
    int8_t detectButton(float voltage);
    
    adc1_channel_t m_channel;
    uint8_t m_num_buttons;
    const float* m_thresholds;
    float m_tolerance;
    
    int8_t m_current_button;     // -1 = no button, 0-5 = button ID
    int8_t m_last_button;
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
