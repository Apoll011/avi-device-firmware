# AVI Embedded Firmware - Developer Guide

## Overview

This is a **modular, production-ready firmware architecture** for ESP32 devices that integrates with the AVI protocol. The system is designed to be **board-agnostic** and **feature-driven**, allowing you to easily port to different hardware platforms and enable/disable features based on hardware capabilities.

## Architecture Philosophy

### Domain-Driven Design
The codebase is organized around **domains** rather than technical layers:
- **Board Layer**: Hardware abstraction (buttons, I2S, ADC, etc.)
- **Transport Layer**: Network communication (WiFi, UDP)
- **Feature Layer**: Application capabilities (LED, Audio, Button)
- **Application Layer**: Business logic orchestration

### Modular Features System
Features are **self-contained plugins** that can be independently:
- Enabled/disabled at compile time
- Initialized and started
- Updated in the main loop
- Stopped and cleaned up

This follows the **Open/Closed Principle** - you can add new features without modifying existing code.

---

## Project Structure

```
avi_firmware/
├── CMakeLists.txt                          # Root build configuration
├── main/
│   ├── CMakeLists.txt                      # Main component config
│   ├── device_config.h                     # Board selection & configuration
│   └── main.cpp                            # Application entry point
└── components/
    ├── board_korvo/                        # ESP32 Korvo board abstraction
    │   ├── CMakeLists.txt
    │   ├── include/board_korvo.h
    │   └── board_korvo.cpp
    ├── avi_transport/                      # WiFi & UDP transport
    │   ├── CMakeLists.txt
    │   ├── include/avi_transport.h
    │   └── avi_transport.cpp
    └── device_features/                    # Modular feature system
        ├── CMakeLists.txt
        ├── include/device_features.h
        └── device_features.cpp
```

---

## Quick Start

### 1. Configure Your Device

Edit `main/device_config.h`:

```cpp
// Choose your board
#define BOARD_ESP32_KORVO_V1_1

// Set your network credentials
#define WIFI_SSID      "YourWiFiSSID"
#define WIFI_PASSWORD  "YourPassword"
#define AVI_SERVER_IP  "192.168.1.100"
#define AVI_SERVER_PORT 8888

// Set device identity
#define DEVICE_ID      0x0123456789ABCDEFULL
#define DEVICE_NAME    "My-Device"
```

### 2. Build & Flash

```bash
cd avi_firmware
idf.py build
idf.py flash monitor
```

### 3. Watch It Connect

You'll see output like:
```
I (1234) MAIN: Device: My-Device
I (1235) MAIN: ID: 0x0123456789ABCDEF
I (1236) MAIN: Enabled features:
I (1237) MAIN:   • Button Input
I (1238) MAIN:   • LED Strip
I (1239) MAIN:   • Audio Output
I (2500) AVI_TRANSPORT: Got IP: 192.168.1.105
I (2501) AVI_TRANSPORT: UDP connected to 192.168.1.100:8888
I (2502) MAIN: AVI initialized
I (2600) FEATURES: Button feature started
I (2601) FEATURES: LED feature started
I (2602) FEATURES: Audio feature started
```

---

## Adding a New Board

### Step 1: Create Board Component

Create `components/board_myboard/`:

```cpp
// include/board_myboard.h
#pragma once
#include <functional>
#include "driver/gpio.h"

namespace Board {
    class Button {
    public:
        using ButtonCallback = std::function<void(bool pressed)>;
        
        explicit Button(gpio_num_t pin);
        void init();
        void poll();
        void onStateChange(ButtonCallback callback);
        
    private:
        gpio_num_t m_pin;
        bool m_state;
        ButtonCallback m_callback;
    };
    
    void init();
}
```

```cpp
// board_myboard.cpp
#include "board_myboard.h"
#include "esp_log.h"

static const char* TAG = "BOARD_MYBOARD";

namespace Board {
    Button::Button(gpio_num_t pin) : m_pin(pin), m_state(false) {}
    
    void Button::init() {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << m_pin),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        gpio_config(&io_conf);
        ESP_LOGI(TAG, "Button initialized on GPIO %d", m_pin);
    }
    
    void Button::poll() {
        bool current = gpio_get_level(m_pin) == 0; // Active low
        if (current != m_state) {
            m_state = current;
            if (m_callback) {
                m_callback(m_state);
            }
        }
    }
    
    void Button::onStateChange(ButtonCallback callback) {
        m_callback = callback;
    }
    
    void init() {
        ESP_LOGI(TAG, "Initializing MyBoard");
    }
}
```

### Step 2: Add Board Configuration

In `main/device_config.h`:

```cpp
#elif defined(BOARD_MYBOARD)
    #define FEATURE_BUTTON_INPUT
    #define FEATURE_LED_STRIP
    
    #define PIN_BUTTON          GPIO_NUM_0
    #define PIN_LED_DATA        GPIO_NUM_5
    #define LED_COUNT           8
```

### Step 3: Update Feature Component

In `components/device_features/device_features.cpp`, add your board's includes:

```cpp
#ifdef BOARD_MYBOARD
#include "board_myboard.h"
#endif
```

### Step 4: Build for Your Board

```cpp
// device_config.h
#define BOARD_MYBOARD  // Uncomment this, comment others
```

Done! Your board is now integrated.

---

## Creating a New Feature

Features follow a **lifecycle pattern**: init → start → update → stop

### Example: Temperature Sensor Feature

```cpp
// In device_features.h
class TemperatureFeature : public Feature {
public:
    explicit TemperatureFeature(AVI_AviEmbedded* avi);
    
    bool init() override;
    bool start() override;
    void update() override;
    void stop() override;
    const char* getName() const override { return "Temperature"; }
    
private:
    AVI_AviEmbedded* m_avi;
    float m_last_temp;
    uint32_t m_update_counter;
};
```

```cpp
// In device_features.cpp
TemperatureFeature::TemperatureFeature(AVI_AviEmbedded* avi)
    : m_avi(avi), m_last_temp(0.0f), m_update_counter(0) {}

bool TemperatureFeature::init() {
    ESP_LOGI(TAG, "Initializing Temperature sensor");
    // Initialize your sensor here
    return true;
}

bool TemperatureFeature::start() {
    ESP_LOGI(TAG, "Temperature sensor started");
    return true;
}

void TemperatureFeature::update() {
    m_update_counter++;
    
    // Read temperature every 2 seconds (assuming 50ms loop interval)
    if (m_update_counter % 40 == 0) {
        // Read from your sensor
        float temp = read_temperature_sensor();
        
        if (temp != m_last_temp) {
            // Send to AVI server
            const char* sensor_name = "temp_main";
            avi_embedded_update_sensor_temperature(
                m_avi, sensor_name, strlen(sensor_name), temp
            );
            
            ESP_LOGI(TAG, "Temperature: %.2f°C", temp);
            m_last_temp = temp;
        }
    }
}

void TemperatureFeature::stop() {
    ESP_LOGI(TAG, "Temperature sensor stopped");
}
```

### Enable the Feature

```cpp
// In device_config.h
#ifdef BOARD_ESP32_KORVO_V1_1
    #define FEATURE_TEMPERATURE  // Add this
    // ... other features
#endif
```

```cpp
// In main.cpp setupFeatures()
#ifdef FEATURE_TEMPERATURE
    m_features->addFeature(
        std::make_unique<Features::TemperatureFeature>(m_client.getHandle())
    );
#endif
```

---

## Feature Communication Patterns

### 1. Sending Sensor Data

```cpp
// Temperature
avi_embedded_update_sensor_temperature(avi, "sensor_name", name_len, 25.5f);

// Humidity
avi_embedded_update_sensor_humidity(avi, "humidity", name_len, 60.0f);

// Battery
avi_embedded_update_sensor_battery(avi, "battery", name_len, 85);

// Status (boolean)
avi_embedded_update_sensor_status(avi, "door_open", name_len, true);

// Raw (int32)
avi_embedded_update_sensor_raw(avi, "counter", name_len, 12345);
```

### 2. Button Events

```cpp
uint8_t button_id = 0;
uint8_t press_type = 0; // 0=single, 1=double, 2=long
avi_embedded_button_pressed(avi, button_id, press_type);
```

### 3. Subscribing to Topics

```cpp
const char* topic = "device/command";
avi_embedded_subscribe(avi, topic, strlen(topic));
```

### 4. Publishing Messages

```cpp
const char* topic = "device/status";
const char* message = "{\"online\":true}";
avi_embedded_publish(avi, topic, strlen(topic),
                     (const uint8_t*)message, strlen(message));
```

### 5. Streaming Audio

```cpp
uint8_t stream_id = 1;
const char* target = "server";
const char* reason = "voice_command";

// Start stream
avi_embedded_start_stream(avi, stream_id, target, strlen(target),
                          reason, strlen(reason));

// Send audio data
uint8_t audio_buffer[512];
avi_embedded_send_stream_data(avi, stream_id, audio_buffer, sizeof(audio_buffer));

// Close stream
avi_embedded_close_stream(avi, stream_id);
```

---

## Best Practices

### 1. RAII for Resources
Always use RAII (Resource Acquisition Is Initialization):

```cpp
class MyFeature : public Feature {
public:
    bool init() override {
        m_buffer = std::make_unique<uint8_t[]>(1024);
        return true;
    }
    // Automatically cleaned up in destructor
private:
    std::unique_ptr<uint8_t[]> m_buffer;
};
```

### 2. Const Correctness
Use `const` wherever possible:

```cpp
const char* getName() const override { return "MyFeature"; }
bool isEnabled() const { return m_enabled; }
```

### 3. Error Handling
Check return values and log errors:

```cpp
int ret = avi_embedded_subscribe(avi, topic, len);
if (ret != 0) {
    ESP_LOGW(TAG, "Failed to subscribe: %d", ret);
    return false;
}
```

### 4. Logging Levels
Use appropriate log levels:
- `ESP_LOGV`: Verbose debug info
- `ESP_LOGD`: Debug information
- `ESP_LOGI`: Informational messages
- `ESP_LOGW`: Warnings
- `ESP_LOGE`: Errors

### 5. Non-Blocking Code
Never block in `update()`:

```cpp
// BAD
void update() override {
    vTaskDelay(pdMS_TO_TICKS(1000)); // Blocks everything!
}

// GOOD
void update() override {
    m_counter++;
    if (m_counter % 20 == 0) {  // Every 1 second at 50ms loop
        do_something();
    }
}
```

---

## Example Projects

### Minimal Button Device

```cpp
// device_config.h
#define BOARD_ESP32_DEVKIT_V1
#define FEATURE_BUTTON_INPUT

// That's it! Buttons automatically work
```

### Smart Doorbell

```cpp
// device_config.h
#define BOARD_ESP32_KORVO_V1_1
#define FEATURE_BUTTON_INPUT    // Doorbell button
#define FEATURE_AUDIO_OUTPUT    // Chime sound
#define FEATURE_MICROPHONE      // Talk to visitor

// Features auto-initialize based on flags
```

### LED Strip Controller

```cpp
// device_config.h
#define BOARD_CUSTOM
#define FEATURE_LED_STRIP

// Configure pins
#define PIN_LED_DATA GPIO_NUM_5
#define LED_COUNT 60
```

---

## Troubleshooting

### Partition Too Small
If you get partition overflow errors:

```bash
# Increase factory partition in partitions.csv
factory,  app,  factory, 0x10000, 0x180000
```

### Features Not Starting
Check logs for initialization errors:
```
E (1234) FEATURES: Failed to initialize feature: LED
```

Enable verbose logging:
```cpp
esp_log_level_set("FEATURES", ESP_LOG_VERBOSE);
```

### WiFi Won't Connect
Verify credentials and check serial monitor:
```
I (2345) AVI_TRANSPORT: WiFi started, connecting...
I (5678) AVI_TRANSPORT: Got IP: 192.168.1.105
```

### AVI Not Connecting
Ensure server is reachable:
```bash
ping 192.168.1.111
```

Check firewall allows UDP on port 8888.

---

## Advanced Topics

### Custom Message Handlers

Features can handle incoming AVI messages:

```cpp
class MyFeature : public Feature {
public:
    bool init() override {
        // Subscribe to custom topic
        const char* topic = "device/my_command";
        avi_embedded_subscribe(m_avi, topic, strlen(topic));
        
        // You'll need to add message callback support
        return true;
    }
};
```

### Multi-Device Coordination

Devices can communicate via pub/sub:

```cpp
// Device A publishes
const char* topic = "home/motion/detected";
const char* msg = "{\"room\":\"kitchen\"}";
avi_embedded_publish(avi, topic, strlen(topic), 
                     (const uint8_t*)msg, strlen(msg));

// Device B subscribes
avi_embedded_subscribe(avi, "home/motion/#", strlen("home/motion/#"));
```

### Power Management

Add sleep modes for battery-powered devices:

```cpp
class PowerFeature : public Feature {
    void update() override {
        if (should_sleep()) {
            ESP_LOGI(TAG, "Entering deep sleep");
            esp_deep_sleep_start();
        }
    }
};
```

---

## Summary

This architecture provides:

✅ **Board Independence** - Easy porting to new hardware  
✅ **Feature Modularity** - Enable/disable features per device  
✅ **Production Quality** - Error handling, logging, RAII  
✅ **Type Safety** - Modern C++ with compile-time checks  
✅ **Maintainability** - Clear separation of concerns  
✅ **Extensibility** - Add features without changing core code  

The system is designed to **scale from simple prototypes to production devices** while maintaining clean, maintainable code.
