# AVI Embedded Firmware

Production-ready, modular firmware for ESP32 devices with AVI protocol integration.

## Features

- ✅ **Modular Architecture** - Features are self-contained plugins
- ✅ **Board Agnostic** - Easy porting to different ESP32 boards
- ✅ **Production Ready** - RAII, error handling, logging
- ✅ **AVI Integration** - Full protocol support (sensors, buttons, streaming, pub/sub)
- ✅ **Feature Flags** - Compile-time feature selection based on hardware

## Supported Boards

- ESP32 Korvo v1.1 (default)
- ESP32 DevKit v1
- Custom boards (easy to add)

## Supported Features

- 🔘 **Button Input** - ADC or GPIO-based button detection
- 💡 **LED Strip** - WS2812B LED control with animations
- 🔊 **Audio Output** - I2S audio playback
- 🎤 **Microphone** - Audio input streaming (coming soon)

## Quick Start

1. **Configure your device** in `main/device_config.h`:
   ```cpp
   #define BOARD_ESP32_KORVO_V1_1
   #define WIFI_SSID "YourWiFi"
   #define WIFI_PASSWORD "YourPassword"
   #define AVI_SERVER_IP "192.168.1.100"
   ```

2. **Build and flash**:
   ```bash
   idf.py build flash monitor
   ```

3. **Watch it connect** to WiFi and AVI server!

## Project Structure

```
avi_firmware/
├── main/                   # Application entry point
│   ├── device_config.h     # Board & feature configuration
│   └── main.cpp           # Main application
└── components/
    ├── board_korvo/       # ESP32 Korvo board abstraction
    ├── avi_transport/     # WiFi & UDP transport layer
    └── device_features/   # Modular feature system
```

## Adding Features

Features are simple to add:

```cpp
class MyFeature : public Features::Feature {
public:
    bool init() override { /* setup */ }
    bool start() override { /* start */ }
    void update() override { /* main loop */ }
    void stop() override { /* cleanup */ }
    const char* getName() const override { return "MyFeature"; }
};
```

Enable in `device_config.h`:
```cpp
#define FEATURE_MYFEATURE
```

Add in `main.cpp`:
```cpp
#ifdef FEATURE_MYFEATURE
    m_features->addFeature(std::make_unique<Features::MyFeature>(avi));
#endif
```

## Documentation

See [DEVELOPER_GUIDE.md](DEVELOPER_GUIDE.md) for comprehensive documentation including:
- Architecture overview
- Adding new boards
- Creating custom features
- AVI protocol usage
- Best practices
- Example projects

## Current Implementation (ESP32 Korvo v1.1)

### Hardware Mapping
- **Button**: GPIO39 (ADC1_CH3), active when voltage < 1.5V
- **LEDs**: GPIO22 (WS2812B, 12 LEDs)
- **Audio Out**: I2S (BCK=GPIO26, WS=GPIO25, DATA=GPIO22)

### Features Enabled
- ✅ Button input with debouncing
- ✅ LED strip control
- ✅ Audio output via I2S
- ✅ AVI sensor reporting
- ✅ AVI pub/sub messaging
- ✅ WiFi auto-reconnect

## AVI Protocol Support

### Sending Data
```cpp
// Sensors
avi_embedded_update_sensor_temperature(avi, "temp", 4, 25.5f);
avi_embedded_update_sensor_humidity(avi, "humid", 5, 60.0f);

// Button events
avi_embedded_button_pressed(avi, button_id, press_type);

// Publish messages
avi_embedded_publish(avi, topic, topic_len, data, data_len);
```

### Receiving Data
Features automatically subscribe to topics and handle incoming messages.

## License

[Your License Here]

## Contributing

See DEVELOPER_GUIDE.md for architecture details and contribution guidelines.
