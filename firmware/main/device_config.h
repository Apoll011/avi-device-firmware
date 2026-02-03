/**
 * @file device_config.h
 * @brief Device-specific configuration
 * 
 * Define the board type here to enable the appropriate features.
 * Each board configuration automatically includes its required features.
 */

#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"

// ============================================================================
// Board Selection - Uncomment ONE
// ============================================================================

#define BOARD_ESP32_KORVO_V1_1
// #define BOARD_ESP32_DEVKIT_V1
// #define BOARD_ESP32_S3_DEVKIT
// #define BOARD_CUSTOM

// ============================================================================
// Network Configuration
// ============================================================================

#define WIFI_SSID      "MEO-1012B0"
#define WIFI_PASSWORD  "2173c715c6"
#define AVI_SERVER_IP  "192.168.1.111"
#define AVI_SERVER_PORT 8888

// ============================================================================
// Device Identity
// ============================================================================

#define DEVICE_ID      0x0123456789ABCDEFULL
#define DEVICE_NAME    "Alex-Box"

// ============================================================================
// AVI Topics Configuration
// ============================================================================

// Subscriptions (device listens to these)
#define TOPIC_LED_CONTROL       "device/led/control"
#define TOPIC_LED_ANIMATION     "device/led/animation"
#define TOPIC_LED_CLEAR         "device/led/clear"
#define TOPIC_AUDIO_DATA        "device/audio/data"
#define TOPIC_COMMAND           "device/command"

// Publications (device sends to these)
#define TOPIC_BUTTON_EVENT      "device/button/event"
#define TOPIC_STATUS            "device/status"
#define TOPIC_HEARTBEAT         "device/heartbeat"

// ============================================================================
// Board-Specific Feature Flags
// ============================================================================

#ifdef BOARD_ESP32_KORVO_V1_1
    #define FEATURE_AUDIO_OUTPUT
    #define FEATURE_BUTTON_INPUT
    #define FEATURE_LED_STRIP
    
    // Korvo v1.1 has 6 buttons on a resistor ladder connected to GPIO36 (ADC1_CH0)
    #define BUTTON_COUNT 6
    #define BUTTON_ADC_CHANNEL ADC1_CHANNEL_0  // GPIO36
    
    // Button voltage thresholds (in volts) for each button
    // These are approximate values - tune based on your actual hardware
    static const float BUTTON_THRESHOLDS[BUTTON_COUNT] = {
        0.0f,   // Button 0: REC   - ~0V
        0.5f,   // Button 1: MODE  - ~0.5V
        1.0f,   // Button 2: PLAY  - ~1.0V
        1.5f,   // Button 3: SET   - ~1.5V
        2.0f,   // Button 4: VOL-  - ~2.0V
        2.5f    // Button 5: VOL+  - ~2.5V
    };
    static const float BUTTON_TOLERANCE = 0.2f;  // ±0.2V tolerance
    
    #define PIN_LED_DATA        GPIO_NUM_22
    #define PIN_I2S_BCK         GPIO_NUM_27
    #define PIN_I2S_WS          GPIO_NUM_25
    #define PIN_I2S_DATA_OUT    GPIO_NUM_26
    
    #define LED_COUNT           12
    
#elif defined(BOARD_ESP32_DEVKIT_V1)
    #define FEATURE_BUTTON_INPUT
    #define FEATURE_LED_STRIP
    
    #define BUTTON_COUNT        1
    #define PIN_BUTTON          GPIO_NUM_0
    #define PIN_LED_DATA        GPIO_NUM_5
    #define LED_COUNT           8
    
#elif defined(BOARD_CUSTOM)
    // Define your custom board features here
    #error "Please configure BOARD_CUSTOM features"
    
#else
    #error "No board selected! Please define a board type in device_config.h"
#endif

// ============================================================================
// Application Configuration
// ============================================================================

#define MAIN_TASK_STACK_SIZE    8192
#define MAIN_TASK_PRIORITY      5
#define SCRATCH_BUFFER_SIZE     2048

#define WIFI_CONNECT_TIMEOUT_MS 10000
#define AVI_CONNECT_DELAY_MS    2000
#define MAIN_LOOP_INTERVAL_MS   50
