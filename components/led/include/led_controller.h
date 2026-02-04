#pragma once

#include <driver/gpio.h>
#include <led_strip.h>
#include <vector>
#include <string>
#include <math.h>
#include "esp_timer.h"

// Hardware Config
#define LED_PIN GPIO_NUM_33
#define NUM_LEDS 12

// Color structure (Replaces CRGB)
struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    RgbColor() : r(0), g(0), b(0) {}
    RgbColor(uint8_t _r, uint8_t _g, uint8_t _b) : r(_r), g(_g), b(_b) {}
    
    // Helper to scale brightness
    void scale(uint8_t scale) {
        r = (r * scale) >> 8;
        g = (g * scale) >> 8;
        b = (b * scale) >> 8;
    }
};

enum AnimationType {
    OFF = 0,
    PROCESSING,
    SUCCESS,
    WAITING,
    STARTUP,
    SHUTDOWN,
    RAINBOW_PULSE,
    FIREWORK,
    POLICE,
    HEARTBEAT,
    FIRE,
    CANDY_CANE,
    STROBE,
    HEARTBEAT_FLASH,
    DEVICE_SHUTDOWN,
    BLINKING_WARNING,
    WAKE_WORD,
    SPEECH_PROCESSING,
    NOTIFICATION,
    ERROR_BLINK,
    PAIRING,
    VOICE_RESPONSE,
    ACTION_CONFIRM,
    
    // Configurable ones
    CONF_PULSE,
    CONF_CHASE,
    CONF_SPARKLE,
    CONF_GRADIENT,
    CONF_WAVE,
    CONF_AURORA,
    CONF_PLASMA,
    
    // Fallback
    SOLID_COLOR
};

class LedController {
private:
    led_strip_handle_t led_strip;
    std::vector<RgbColor> leds; // Internal buffer
    
    AnimationType currentAnim = OFF;
    AnimationType nextAnim = OFF;
    
    int64_t animStartTime = 0;
    int64_t animDuration = 0;
    int64_t lastFrameTime = 0;
    
    // Configuration storage
    std::string currentConfig;
    
    // State variables for animations
    uint16_t state_step = 0;
    uint16_t state_val1 = 0;
    uint16_t state_val2 = 0;
    float state_float = 0.0f;
    bool state_bool = false;

    // Helpers
    void show();
    void setPixel(int idx, RgbColor color);
    void setAll(RgbColor color);
    void fadeToBlack(uint8_t amount);
    RgbColor hsv2rgb(uint8_t h, uint8_t s, uint8_t v);
    uint32_t millis();
    
    // Math helpers for smooth animations (sine waves, etc)
    uint8_t beatsin8(uint8_t bpm, uint8_t lowest = 0, uint8_t highest = 255, uint32_t time_shift = 0, uint8_t phase_offset = 0);
    
    // Config Parser Helpers
    int getConfigInt(const char* key, int defaultVal);
    RgbColor getConfigColor(const char* key, RgbColor defaultVal);
    bool getConfigBool(const char* key, bool defaultVal);

    // Animation Implementations
    void anim_processing();
    void anim_success();
    void anim_waiting();
    void anim_rainbow_pulse();
    void anim_firework();
    void anim_police();
    void anim_heartbeat();
    void anim_fire();
    void anim_device_shutdown();
    void anim_wake_word();
    void anim_voice_response();
    
    // Configurable Impls
    void anim_conf_pulse();
    void anim_conf_chase();
    void anim_conf_plasma();
    void anim_conf_aurora();

public:
    LedController();
    bool init();
    void update(bool connected); // Call this in your main loop
    
    void setLed(int idx, RgbColor color);
    void setAnimation(int type, int duration_ms, const char* config = "");
    void clear();
};
