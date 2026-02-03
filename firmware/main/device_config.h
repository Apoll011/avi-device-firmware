/**
 * @file device_config.h
 * @brief Device-specific configuration
 * 
 * Define the board type here to enable the appropriate features.
 * Each board configuration automatically includes its required features.
 */

#pragma once

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
// Board-Specific Feature Flags
// ============================================================================

#ifdef BOARD_ESP32_KORVO_V1_1
    #define FEATURE_AUDIO_OUTPUT
    #define FEATURE_BUTTON_INPUT
    #define FEATURE_LED_STRIP
    #define FEATURE_MICROPHONE
    
    // Korvo-specific pins
    #define PIN_BUTTON_ADC      ADC1_CHANNEL_3  // GPIO39
    #define PIN_LED_DATA        GPIO_NUM_22
    #define PIN_I2S_BCK         GPIO_NUM_26
    #define PIN_I2S_WS          GPIO_NUM_25
    #define PIN_I2S_DATA_OUT    GPIO_NUM_22
    
    #define LED_COUNT           12
    #define BUTTON_VOLTAGE_THRESHOLD 1.5f
    
#elif defined(BOARD_ESP32_DEVKIT_V1)
    #define FEATURE_BUTTON_INPUT
    #define FEATURE_LED_STRIP
    
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
