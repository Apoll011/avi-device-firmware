#include "led_controller.h"
#include <esp_log.h>
#include <esp_random.h>
#include <string.h>
#include <algorithm>
#include "led_strip.h"

#define TAG "LED_CTRL"

LedController::LedController() {
    leds.resize(NUM_LEDS);
}

bool LedController::init() {
    ESP_LOGI(TAG, "Initializing LED Strip on GPIO %d", LED_PIN);

    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_PIN,
        .max_leds = NUM_LEDS,
        .led_model = LED_MODEL_WS2812,
        .flags = { .invert_out = false } 
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags = { .with_dma = false }
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    led_strip_clear(led_strip);

	return true;
}

// ---------------------------------------------------------
// Core Loop
// ---------------------------------------------------------

void LedController::update(bool connected) {
    int64_t now = millis();
    
    // Handle Duration / Auto-switch
    if (currentAnim != OFF && currentAnim != SOLID_COLOR) {
        if (animDuration > 0 && (now - animStartTime > animDuration)) {
            // Animation finished
            currentConfig = "";
            
            if (nextAnim != OFF) {
                setAnimation(nextAnim, 5000);
                nextAnim = OFF;
            } else if (connected) {
                setAnimation(OFF, 0); // Or idle animation
                clear();
            } else {
                setAnimation(WAITING, 0); // Waiting for connection
            }
        }
    }

    // Limit framerate to ~60FPS (16ms)
    if (now - lastFrameTime < 6) return;
    lastFrameTime = now;

    // Run Animation Logic
    switch (currentAnim) {
        case PROCESSING: anim_processing(); break;
        case SUCCESS: anim_success(); break;
        case WAITING: anim_waiting(); break;
        case STARTUP: anim_rainbow_pulse(); break; // Reusing rainbow for startup
        case SHUTDOWN: fadeToBlack(20); break;
        case RAINBOW_PULSE: anim_rainbow_pulse(); break;
        case FIREWORK: anim_firework(); break;
        case POLICE: anim_police(); break;
        case HEARTBEAT: anim_heartbeat(); break;
        case FIRE: anim_fire(); break;
        case DEVICE_SHUTDOWN: anim_device_shutdown(); break;
        case WAKE_WORD: anim_wake_word(); break;
        case VOICE_RESPONSE: anim_voice_response(); break;
        
        // Configurable
        case CONF_PULSE: anim_conf_pulse(); break;
        case CONF_CHASE: anim_conf_chase(); break;
        case CONF_PLASMA: anim_conf_plasma(); break;
        case CONF_AURORA: anim_conf_aurora(); break;

        default: break;
    }
    
    show();
}

void LedController::setAnimation(int type, int duration_ms, const char* config) {
    currentAnim = (AnimationType)type;
    animDuration = duration_ms;
    animStartTime = millis();
    if (config) currentConfig = config;
    
    // Reset states
    state_step = 0;
    state_val1 = 0;
    state_float = 0;
    state_bool = false;
    
    ESP_LOGI(TAG, "Animation Set: %d, Dur: %d", type, duration_ms);
}

void LedController::clear() {
    setAll(RgbColor(0,0,0));
    show();
}

// ---------------------------------------------------------
// Helpers
// ---------------------------------------------------------

uint32_t LedController::millis() {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void LedController::show() {
    for (int i = 0; i < NUM_LEDS; i++) {
        led_strip_set_pixel(led_strip, i, leds[i].r, leds[i].g, leds[i].b);
    }
    led_strip_refresh(led_strip);
}

void LedController::setPixel(int idx, RgbColor color) {
    if(idx >= 0 && idx < NUM_LEDS) leds[idx] = color;
}

void LedController::setLed(int idx, RgbColor color) {
	if(idx >= 0 && idx < NUM_LEDS) leds[idx] = color;
}

void LedController::setAll(RgbColor color) {
    for(auto& led : leds) led = color;
}

void LedController::fadeToBlack(uint8_t amount) {
    for(auto& led : leds) {
        led.scale(255 - amount);
    }
}

// Fast integer math HSV to RGB
RgbColor LedController::hsv2rgb(uint8_t h, uint8_t s, uint8_t v) {
    RgbColor rgb;
    unsigned char region, remainder, p, q, t;

    if (s == 0) {
        rgb.r = v; rgb.g = v; rgb.b = v;
        return rgb;
    }

    region = h / 43;
    remainder = (h - (region * 43)) * 6; 

    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * remainder) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

    switch (region) {
        case 0: rgb.r = v; rgb.g = t; rgb.b = p; break;
        case 1: rgb.r = q; rgb.g = v; rgb.b = p; break;
        case 2: rgb.r = p; rgb.g = v; rgb.b = t; break;
        case 3: rgb.r = p; rgb.g = q; rgb.b = v; break;
        case 4: rgb.r = t; rgb.g = p; rgb.b = v; break;
        default: rgb.r = v; rgb.g = p; rgb.b = q; break;
    }
    return rgb;
}

uint8_t LedController::beatsin8(uint8_t bpm, uint8_t lowest, uint8_t highest, uint32_t time_shift, uint8_t phase_offset) {
    uint8_t beat = 0;
    uint32_t m = millis();
    uint16_t beat16 = (m * bpm * 280) / 60000; // Approximate math
    uint8_t wave = (sin((beat16 + phase_offset) * 3.14159 / 128.0) + 1.0) * 127.5;
    
    return lowest + (((highest - lowest) * wave) >> 8);
}

// ---------------------------------------------------------
// Config Parser (Simplified)
// ---------------------------------------------------------

int LedController::getConfigInt(const char* key, int defaultVal) {
    std::string k = key; k += ":";
    size_t pos = currentConfig.find(k);
    if (pos == std::string::npos) return defaultVal;
    return atoi(currentConfig.c_str() + pos + k.length());
}

RgbColor LedController::getConfigColor(const char* key, RgbColor defaultVal) {
    std::string k = key; k += ":";
    size_t pos = currentConfig.find(k);
    if (pos == std::string::npos) return defaultVal;
    
    // Very basic HEX parser (assumes #RRGGBB format)
    const char* val = currentConfig.c_str() + pos + k.length();
    if (val[0] == '#') {
        uint32_t c = strtoul(val + 1, NULL, 16);
        return RgbColor((c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF);
    }
    // Named colors could be added here
    if (strncmp(val, "RED", 3) == 0) return RgbColor(255, 0, 0);
    if (strncmp(val, "BLUE", 4) == 0) return RgbColor(0, 0, 255);
    if (strncmp(val, "GREEN", 5) == 0) return RgbColor(0, 255, 0);
    
    return defaultVal;
}

// ---------------------------------------------------------
// Animations
// ---------------------------------------------------------

void LedController::anim_processing() {
    fadeToBlack(64);
    uint32_t m = millis();
    int pos = (m / 100) % NUM_LEDS; // Rotate every 100ms
    
    for(int i = 0; i < 3; i++) {
        int idx = (pos + i) % NUM_LEDS;
        RgbColor c(0, 0, 255); // Blue
        c.scale(255 - (i * 80));
        setPixel(idx, c);
    }
}

void LedController::anim_success() {
    setAll(RgbColor(0, 255, 0)); // Green base
    uint8_t bright = beatsin8(30, 100, 255);
    
    for(int i=0; i<NUM_LEDS; i++) {
        if((esp_random() % 100) < 10) leds[i] = RgbColor(255, 255, 255); // Sparkle
        leds[i].scale(bright);
    }
}

void LedController::anim_waiting() {
    fadeToBlack(20);
    int pos = (millis() / 100) % NUM_LEDS;
    uint8_t b = beatsin8(30, 50, 255);
    RgbColor c(255, 255, 255);
    c.scale(b);
    setPixel(pos, c);
}

void LedController::anim_rainbow_pulse() {
    static uint8_t hue = 0;
    hue++; // Cycles automatically per frame
    uint8_t bri = beatsin8(30, 100, 255);
    
    for(int i=0; i<NUM_LEDS; i++) {
        RgbColor c = hsv2rgb(hue + (i * 255 / NUM_LEDS), 255, bri);
        setPixel(i, c);
    }
}

void LedController::anim_firework() {
    fadeToBlack(64);
    
    // State machine using member variables
    if(state_step == 0) {
        // Launch
        setPixel(NUM_LEDS/2, RgbColor(255, 255, 255));
        state_step = 1;
        state_val1 = millis(); // Timestamp
    } else if (state_step == 1) {
        // Explode
        if (millis() - state_val1 > 100) {
            int center = NUM_LEDS/2;
            for(int i=1; i<=2; i++) {
                setPixel(center+i, RgbColor(255, 200, 0));
                setPixel(center-i, RgbColor(255, 200, 0));
            }
            state_step = 2;
            state_val1 = millis();
        }
    } else {
        if (millis() - state_val1 > 200) state_step = 0; // Reset
    }
}

void LedController::anim_police() {
    bool redPhase = (millis() / 200) % 2 == 0;
    for(int i=0; i<NUM_LEDS; i++) {
        if(i < NUM_LEDS/2) setPixel(i, redPhase ? RgbColor(255,0,0) : RgbColor(0,0,0));
        else setPixel(i, redPhase ? RgbColor(0,0,0) : RgbColor(0,0,255));
    }
}

void LedController::anim_heartbeat() {
    uint8_t beat = beatsin8(40, 0, 255); // Simulating heartbeat curve roughly
    // Sharpen the curve
    if(beat < 200) beat = beat / 3;
    
    setAll(RgbColor(255, 0, 0));
    for(int i=0; i<NUM_LEDS; i++) leds[i].scale(beat);
}

void LedController::anim_fire() {
    // Simple fire simulation
    for(int i=0; i<NUM_LEDS; i++) {
        // Random heat
        uint8_t heat = beatsin8(20 + i*5, 0, 255, 0, i*20);
        // Map heat to color (Black -> Red -> Yellow -> White)
        RgbColor c;
        if(heat < 85) c = RgbColor(heat*3, 0, 0);
        else if(heat < 170) c = RgbColor(255, (heat-85)*3, 0);
        else c = RgbColor(255, 255, (heat-170)*3);
        setPixel(i, c);
    }
}

void LedController::anim_device_shutdown() {
    // Collapse to center
    if(state_step == 0) {
        state_val1 = 255; // Brightness
        state_step = 1;
    }
    
    if(state_val1 > 5) state_val1 -= 5;
    else state_val1 = 0;
    
    int center = NUM_LEDS / 2;
    setAll(RgbColor(0,0,0));
    
    if(state_val1 > 0) {
        // Only light up center pixels fading out
        setPixel(center, RgbColor(state_val1, state_val1, state_val1));
        setPixel(center-1, RgbColor(state_val1, state_val1, state_val1));
    }
}

void LedController::anim_wake_word() {
    fadeToBlack(60);
    int center = NUM_LEDS / 2;
    int width = (millis() / 50) % (NUM_LEDS/2 + 1);
    
    setPixel(center, RgbColor(0, 100, 255));
    for(int i=1; i<=width; i++) {
        RgbColor c(0, 100, 255);
        c.scale(255 - (i * 40));
        setPixel(center+i, c);
        setPixel(center-i, c);
    }
}

void LedController::anim_voice_response() {
    fadeToBlack(40);
    // Voice waveform simulation using multiple sine waves
    uint32_t t = millis();
    for(int i=0; i<NUM_LEDS; i++) {
        uint8_t wave = beatsin8(60, 10, 255, 0, i*30);
        leds[i] = RgbColor(255, 255, 255);
        leds[i].scale(wave);
    }
}

// ---------------------------------------------------------
// Configurable Animations
// ---------------------------------------------------------

void LedController::anim_conf_pulse() {
    int speed = getConfigInt("SPEED", 15);
    RgbColor col = getConfigColor("COLOR", RgbColor(0,0,255));
    
    uint8_t b = beatsin8(speed, 50, 255);
    setAll(col);
    for(int i=0; i<NUM_LEDS; i++) leds[i].scale(b);
}

void LedController::anim_conf_chase() {
    fadeToBlack(getConfigInt("FADE", 40));
    int speed = getConfigInt("SPEED", 100);
    RgbColor col = getConfigColor("COLOR", RgbColor(255,0,0));
    
    int pos = (millis() / speed) % NUM_LEDS;
    setPixel(pos, col);
}

void LedController::anim_conf_plasma() {
    int speed = getConfigInt("SPEED", 20);
    uint32_t t = millis() * speed / 10;
    
    for(int i=0; i<NUM_LEDS; i++) {
        uint8_t v1 = sin(i * 10 + t) * 127 + 128;
        uint8_t v2 = sin((i*15) + (t/2)) * 127 + 128;
        setPixel(i, hsv2rgb((v1+v2)/2, 255, 255));
    }
}

void LedController::anim_conf_aurora() {
    int speed = getConfigInt("SPEED", 20);
    static uint16_t time = 0;
    time += speed;
    
    // Simple Perlin-ish noise approximation
    for(int i=0; i<NUM_LEDS; i++) {
        uint8_t noise = (sin((i * 50 + time) * 0.01) + 1.0) * 127.5;
        uint8_t hue = 120 + (noise / 3); // Greenish-Purple range
        setPixel(i, hsv2rgb(hue, 200, noise));
    }
}
