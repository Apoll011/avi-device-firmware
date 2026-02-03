#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include <FastLED.h>
#include "animation_configs.h"
#include "config.h"

class LEDAnimations {
private:
    ConfigParser configParser;
public:
    CRGB leds[NUM_LEDS];

    unsigned long animationStart = 0;
    unsigned long animationDuration = 0;
    bool animationRunning = false;
    uint8_t animationType = 0;
    uint8_t nextAnimation = 0;

    void setConfig(char* config);

    void processing(CRGB* leds, uint8_t num_leds);
    void success(CRGB* leds, uint8_t num_leds);
    void waiting(CRGB* leds, uint8_t num_leds);
    void startup(CRGB* leds, uint8_t num_leds);
    void shutdown(CRGB* leds, uint8_t num_leds);
    void rainbow_pulse(CRGB* leds, uint8_t num_leds);
    void firework(CRGB* leds, uint8_t num_leds);
    void police(CRGB* leds, uint8_t num_leds);
    void heartbeat(CRGB* leds, uint8_t num_leds);
    void fire(CRGB* leds, uint8_t num_leds);
    void candy_cane(CRGB* leds, uint8_t num_leds);
    void strobe(CRGB* leds, uint8_t num_leds);
    void heartbeat_flash(CRGB* leds, uint8_t num_leds);
    void device_shutdown(CRGB* leds, uint8_t num_leds);
    void blinking_warning(CRGB* leds, uint8_t num_leds);
    void wakeWord(CRGB* leds, uint8_t num_leds);
    void speechProcessing(CRGB* leds, uint8_t num_leds);
    void notificationHighlight(CRGB* leds, uint8_t num_leds);
    void errorBlink(CRGB* leds, uint8_t num_leds);
    void pairing(CRGB* leds, uint8_t num_leds);
    void voiceResponse(CRGB* leds, uint8_t num_leds);
    void actionConfirmation(CRGB* leds, uint8_t num_leds);

    void configurable_pulse(CRGB* leds, uint8_t num_leds);
    void configurable_chase(CRGB* leds, uint8_t num_leds);
    void configurable_sparkle(CRGB* leds, uint8_t num_leds);
    void configurable_gradient(CRGB* leds, uint8_t num_leds);
    void configurable_wave(CRGB* leds, uint8_t num_leds);
    void status_indicator(CRGB* leds, uint8_t num_leds);
    void progress_indicator(CRGB* leds, uint8_t num_leds);
    void configurable_firework(CRGB* leds, uint8_t num_leds);
    void configurable_meteor(CRGB* leds, uint8_t num_leds);
    void configurable_ripple(CRGB* leds, uint8_t num_leds);
    void configurable_pixels(CRGB* leds, uint8_t num_leds);
    void configurable_scanner(CRGB* leds, uint8_t num_leds);
    void configurable_strobe_patterns(CRGB* leds, uint8_t num_leds);
    void configurable_aurora(CRGB* leds, uint8_t num_leds);
    void configurable_plasma(CRGB* leds, uint8_t num_leds);

    void runAnimation(bool conected);
    void startAnimation(int type, int duration);
    void setLed(int index, CRGB color);
    void setAnimation(int animation_id, long duration, char config[64]);
    void setup();
    void clear();
};



#endif
