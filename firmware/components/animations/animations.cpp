#include "animations.h"

#define NUM_LEDS 12
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define MAX_BRIGHTNESS 255
#define MIN_BRIGHTNESS 20

void LEDAnimations::setConfig(char* config) {
    configParser.setConfig(config);
}

void LEDAnimations::processing(CRGB* leds, uint8_t num_leds) {
    static uint8_t pos = 0;
    fadeToBlackBy(leds, num_leds, 64);

    for(int i = 0; i < 3; i++) {
        int idx = (pos + i) % num_leds;
        leds[idx] = CRGB::Blue;
        leds[idx].nscale8(255 - (i * 40)); 
    }

    EVERY_N_MILLISECONDS(50) {
        pos = (pos + 1) % num_leds;
    }
}

void LEDAnimations::success(CRGB* leds, uint8_t num_leds) {
    static uint8_t brightness = 255;

    fill_solid(leds, num_leds, CRGB::Green);
    for(int i = 0; i < num_leds; i++) {
        if(random8() < 30) {
            leds[i] = CRGB::White;
        }
        leds[i].nscale8(brightness);
    }

    EVERY_N_MILLISECONDS(20) {
        brightness = beatsin8(30, 100, 255);
    }
}

void LEDAnimations::waiting(CRGB* leds, uint8_t num_leds) {
    static uint8_t pos = 0;
    fadeToBlackBy(leds, num_leds, 20);

    uint8_t brightness = beatsin8(15, 50, 255);
    leds[pos] = CRGB::White;
    leds[pos].nscale8(brightness);

    EVERY_N_MILLISECONDS(100) {
        pos = (pos + 1) % num_leds;
    }
}

void LEDAnimations::startup(CRGB* leds, uint8_t num_leds) {
    static uint8_t hue = 0;
    fill_rainbow(leds, num_leds, hue, 255/num_leds);
    EVERY_N_MILLISECONDS(20) {
        hue++;
    }
}

void LEDAnimations::shutdown(CRGB* leds, uint8_t num_leds) {
    fadeToBlackBy(leds, num_leds, 10);
    delay(10);
}


void LEDAnimations::rainbow_pulse(CRGB* leds, uint8_t num_leds) {
    static uint8_t hue = 0;
    static uint8_t brightness = 255;

    fill_rainbow(leds, num_leds, hue, 255/num_leds);
    EVERY_N_MILLISECONDS(20) {
        hue++;
        brightness = beatsin8(30, 100, 255);
    }

    for(int i = 0; i < num_leds; i++) {
        leds[i].nscale8(brightness);
    }
}

void LEDAnimations::firework(CRGB* leds, uint8_t num_leds) {
    static uint8_t stage = 0;
    static uint8_t center = num_leds / 2;

    fadeToBlackBy(leds, num_leds, 64);

    if(stage == 0) {
        leds[center] = CRGB::White;
        stage = 1;
    } else {
        for(int i = 1; i < 4; i++) {
            if(center + i < num_leds) leds[center + i] = CRGB::Yellow;
            if(center - i >= 0) leds[center - i] = CRGB::Yellow;
        }
        stage = 0;
        center = random8(num_leds);
    }
    delay(100);
}

void LEDAnimations::police(CRGB* leds, uint8_t num_leds) {
    static bool red_phase = true;
    EVERY_N_MILLISECONDS(200) {
        red_phase = !red_phase;
    }

    for(int i = 0; i < num_leds; i++) {
        if(i < num_leds/2) {
            leds[i] = red_phase ? CRGB::Red : CRGB::Black;
        } else {
            leds[i] = red_phase ? CRGB::Black : CRGB::Blue;
        }
    }
}

void LEDAnimations::heartbeat(CRGB* leds, uint8_t num_leds) {
    static uint8_t beat = 0;
    EVERY_N_MILLISECONDS(20) {
        beat = beatsin8(40, 0, 255, 0, 0);
    }

    fill_solid(leds, num_leds, CRGB::Red);
    for(int i = 0; i < num_leds; i++) {
        leds[i].nscale8(beat);
    }
}

void LEDAnimations::fire(CRGB* leds, uint8_t num_leds) {
    static uint8_t heat[NUM_LEDS];

    for(int i = 0; i < num_leds; i++) {
        heat[i] = qsub8(heat[i], random8(0, 10));
    }

    for(int i = num_leds - 1; i >= 2; i--) {
        heat[i] = (heat[i - 1] + heat[i - 2]) / 2;
    }

    if(random8() < 120) {
        heat[0] = qadd8(heat[0], random8(160, 255));
    }

    for(int i = 0; i < num_leds; i++) {
        CRGB color = HeatColor(heat[i]);
        leds[i] = color;
    }
}

void LEDAnimations::candy_cane(CRGB* leds, uint8_t num_leds) {
    static uint8_t offset = 0;
    EVERY_N_MILLISECONDS(100) {
        offset++;
    }

    for(int i = 0; i < num_leds; i++) {
        if(((i + offset) % 4) < 2) {
            leds[i] = CRGB::Red;
        } else {
            leds[i] = CRGB::White;
        }
    }
}

void LEDAnimations::strobe(CRGB* leds, uint8_t num_leds) {
    static bool on = false;
    fill_solid(leds, num_leds, on ? CRGB::White : CRGB::Black);
    on = !on;
    delay(100);
}

void LEDAnimations::heartbeat_flash(CRGB* leds, uint8_t num_leds) {
    static uint8_t beat = beatsin8(60, 0, 255);
    fill_solid(leds, num_leds, CRGB::Red);
    for (int i = 0; i < num_leds; i++) {
        leds[i].nscale8(beat);
    }
}

void LEDAnimations::device_shutdown(CRGB* leds, uint8_t num_leds) {
   static uint8_t stage = 0;
    static uint8_t brightness = 255;
    static uint16_t center = num_leds / 2;
    
    if (stage == 0) {
        uint8_t fadePoint = (255 - brightness) * (num_leds / 2) / 255;
        
        for (int i = 0; i < num_leds; i++) {
            uint8_t distFromEdge = min(i, num_leds - 1 - i);
            
            if (distFromEdge < fadePoint) {
                leds[i] = CRGB::Black;
            } else {
                leds[i] = CRGB(
                    brightness * (distFromEdge - fadePoint) / (num_leds/2 - fadePoint), // R
                    brightness * (distFromEdge - fadePoint) / (num_leds/2 - fadePoint), // G
                    brightness                                                          // B
                );
            }
        }

        EVERY_N_MILLISECONDS(20) {
            brightness = max(0, brightness - 3);
            if (brightness == 0) {
                stage = 1;
                brightness = 255;
            }
        }
    }
    
    if (stage == 1) {
        uint8_t pulseWidth = 3;
        for (int i = 0; i < num_leds; i++) {
            if (abs(i - center) <= pulseWidth) {
                leds[i] = CRGB(0, 0, brightness);
            } else {
                leds[i] = CRGB::Black;
            }
        }
        
        EVERY_N_MILLISECONDS(5) {
            brightness = max(0, brightness - 5);
            if (brightness == 0) {
                stage = 2;
            }
        }
    }
    
    if (stage == 2) {
        fill_solid(leds, num_leds, CRGB::Black);
    }
    
    delay(10);
}

void LEDAnimations::blinking_warning(CRGB* leds, uint8_t num_leds) {
    static bool on = true;
    EVERY_N_MILLISECONDS(300) {
        on = !on;
    }
    fill_solid(leds, num_leds, on ? CRGB::Orange : CRGB::Black);
}

void LEDAnimations::wakeWord(CRGB* leds, uint8_t num_leds) {
    static uint8_t center = num_leds / 2;
    static uint8_t step = 0;

    fadeToBlackBy(leds, num_leds, 50);

    for (int i = 0; i <= step; i++) {
        if (center - i >= 0) leds[center - i] = CRGB::Blue;
        if (center + i < num_leds) leds[center + i] = CRGB::Blue;
    }

    EVERY_N_MILLISECONDS(50) {
        step++;
        if (step >= num_leds / 2) step = 0;
    }
}

void LEDAnimations::speechProcessing(CRGB* leds, uint8_t num_leds) {
    static uint8_t hue = 0;

    fill_rainbow(leds, num_leds, hue, 8);
    EVERY_N_MILLISECONDS(30) {
        hue++;
    }
}

void LEDAnimations::notificationHighlight(CRGB* leds, uint8_t num_leds) {
    static uint8_t pos = 0;

    fill_solid(leds, num_leds, CRGB::Blue);
    leds[pos] = CRGB::White;

    EVERY_N_MILLISECONDS(100) {
        pos = (pos + 1) % num_leds;
    }
}

void LEDAnimations::errorBlink(CRGB* leds, uint8_t num_leds) {
    static bool state = true;

    if (state) {
        fill_solid(leds, num_leds, CRGB::Red);
    } else {
        fill_solid(leds, num_leds, CRGB::Black);
    }

    EVERY_N_MILLISECONDS(500) {
        state = !state;
    }
}

void LEDAnimations::pairing(CRGB* leds, uint8_t num_leds) {
    fadeToBlackBy(leds, num_leds, 20);

    for (int i = 0; i < num_leds; i += 3) {
        leds[i] = CRGB::Cyan;
    }
    delay(50);
}

void LEDAnimations::voiceResponse(CRGB* leds, uint8_t num_leds) {
    fadeToBlackBy(leds, num_leds, 20);

    for (int i = 0; i < num_leds; i++) {
        leds[i] = CRGB::White;
    }
}

void LEDAnimations::actionConfirmation(CRGB* leds, uint8_t num_leds) {
    fadeToBlackBy(leds, num_leds, 50);
    for (int i = 0; i < num_leds; i++) {
        leds[i] = CRGB::Green;
    }
}

void LEDAnimations::configurable_pulse(CRGB* leds, uint8_t num_leds) {
    static uint8_t brightness = 0;
    EVERY_N_MILLISECONDS(20) {
        brightness = beatsin8(
            configParser.getValue("SPEED", 15),
            configParser.getValue("MIN_BRIGHT", 50),
            configParser.getValue("MAX_BRIGHT", 255)
        );
    }
    
    CRGB color = configParser.getColor("COLOR", CRGB::Blue);
    fill_solid(leds, num_leds, color);
    for(int i = 0; i < num_leds; i++) {
        leds[i].nscale8(brightness);
    }
}

void LEDAnimations::configurable_chase(CRGB* leds, uint8_t num_leds) {
    static uint8_t pos = 0;
    uint8_t fadeAmount = configParser.getValue("FADE", 20);
    uint8_t tailLength = configParser.getValue("TAIL", 3);
    CRGB color = configParser.getColor("COLOR", CRGB::Blue);
    bool reverse = configParser.getBool("REVERSE", false);
    
    fadeToBlackBy(leds, num_leds, fadeAmount);
    
    for(int i = 0; i < tailLength; i++) {
        int idx = reverse ? 
            ((num_leds - 1) - ((pos + i) % num_leds)) : 
            ((pos + i) % num_leds);
        leds[idx] = color;
        leds[idx].nscale8(255 - (i * 40));
    }
    
    EVERY_N_MILLISECONDS(configParser.getValue("SPEED", 50)) {
        pos = (pos + 1) % num_leds;
    }
}

void LEDAnimations::configurable_sparkle(CRGB* leds, uint8_t num_leds) {
    uint8_t fadeAmount = configParser.getValue("FADE", 80);
    uint8_t sparkleChance = configParser.getValue("CHANCE", 200);
    CRGB color = configParser.getColor("COLOR", CRGB::White);
    
    fadeToBlackBy(leds, num_leds, fadeAmount);
    if(random8() < sparkleChance) {
        leds[random8(num_leds)] = color;
    }
}

void LEDAnimations::configurable_gradient(CRGB* leds, uint8_t num_leds) {
    static uint8_t offset = 0;
    uint8_t speed = configParser.getValue("SPEED", 50);
    uint8_t hueSpread = configParser.getValue("SPREAD", 255);
    uint8_t startHue = configParser.getValue("START_HUE", 0);
    bool reverse = configParser.getBool("REVERSE", false);
    
    EVERY_N_MILLISECONDS(speed) {
        offset++;
    }
    
    for(int i = 0; i < num_leds; i++) {
        uint8_t pos = reverse ? (num_leds - 1 - i) : i;
        uint8_t hue = startHue + map(pos, 0, num_leds, 0, hueSpread) + offset;
        leds[i] = CHSV(hue, 255, 255);
    }
}

void LEDAnimations::configurable_wave(CRGB* leds, uint8_t num_leds) {
    static uint8_t offset = 0;
    uint8_t speed = configParser.getValue("SPEED", 30);
    uint8_t waveCount = configParser.getValue("WAVES", 1);
    CRGB color = configParser.getColor("COLOR", CRGB::Blue);
    
    for(int i = 0; i < num_leds; i++) {
        uint8_t brightness = sin8((i * 16 * waveCount) + offset);
        leds[i] = color;
        leds[i].nscale8(brightness);
    }
    
    offset += speed / 10;
}

void LEDAnimations::status_indicator(CRGB* leds, uint8_t num_leds) {
    static uint8_t brightness = 255;
    CRGB color = configParser.getColor("COLOR", CRGB::Blue);
    Pattern pattern = configParser.getPattern("PATTERN", Pattern::SOLID);
    uint8_t speed = configParser.getValue("SPEED", 20);
    bool blink = configParser.getBool("BLINK", false);
    uint8_t blink_speed = configParser.getValue("BSPEED", 500);
    
    if(blink) {
        static bool state = true;
        EVERY_N_MILLISECONDS(blink_speed) {
            state = !state;
        }
        if(!state) {
            fill_solid(leds, num_leds, CRGB::Black);
            return;
        }
    }
    
    switch(pattern) {
        case Pattern::SOLID:
            fill_solid(leds, num_leds, color);
            break;
            
        case Pattern::GRADIENT:
            for(int i = 0; i < num_leds; i++) {
                uint8_t brightness = beatsin8(speed, 50, 255, 0, i * (255 / num_leds));
                leds[i] = color;
                leds[i].nscale8(brightness);
            }
            break;
            
        case Pattern::SPARKLE:
            fadeToBlackBy(leds, num_leds, 80);
            if(random8() < 100) {
                leds[random8(num_leds)] = color;
            }
            break;
    }
    
    EVERY_N_MILLISECONDS(speed) {
        brightness = beatsin8(30, 100, 255);
    }
    
    for(int i = 0; i < num_leds; i++) {
        leds[i].nscale8(brightness);
    }
}

void LEDAnimations::progress_indicator(CRGB* leds, uint8_t num_leds) {
    uint8_t progress = configParser.getValue("PROGRESS", 0);  // 0-100
    CRGB activeColor = configParser.getColor("ACTIVE_COLOR", CRGB::Green);
    CRGB inactiveColor = configParser.getColor("INACTIVE_COLOR", CRGB::Black);
    bool reverse = configParser.getBool("REVERSE", false);
    
    uint8_t lit_leds = (progress * num_leds) / 100;
    
    for(int i = 0; i < num_leds; i++) {
        int idx = reverse ? (num_leds - 1 - i) : i;
        leds[idx] = (i < lit_leds) ? activeColor : inactiveColor;
    }
}

void LEDAnimations::configurable_firework(CRGB* leds, uint8_t num_leds) {
    static uint8_t stage = 0;
    static uint8_t center = num_leds / 2;
    static uint8_t brightness = 255;
    
    CRGB rocketColor = configParser.getColor("ROCKET_COLOR", CRGB::White);
    CRGB burstColor = configParser.getColor("BURST_COLOR", CRGB::Yellow);
    uint8_t trailLength = configParser.getValue("TRAIL", 3);
    uint8_t burstSize = configParser.getValue("BURST_SIZE", 5);
    uint8_t fadeRate = configParser.getValue("FADE", 64);
    uint8_t speed = configParser.getValue("SPEED", 50);
    
    fadeToBlackBy(leds, num_leds, fadeRate);
    
    if(stage == 0) {
        for(int i = 0; i < trailLength; i++) {
            if(center + i < num_leds) {
                leds[center + i] = rocketColor;
                leds[center + i].nscale8(255 - (i * 50));
            }
        }
        center--;
        if(center <= burstSize) stage = 1;
    } else { 
        for(int i = 0; i < burstSize; i++) {
            if(center + i < num_leds) leds[center + i] = burstColor;
            if(center - i >= 0) leds[center - i] = burstColor;
        }
        brightness = max(0, brightness - 10);
        if(brightness == 0) {
            stage = 0;
            brightness = 255;
            center = random8(num_leds/2, num_leds-1);
        }
    }
    delay(speed);
}

void LEDAnimations::configurable_meteor(CRGB* leds, uint8_t num_leds) {
    static uint8_t pos = 0;
    CRGB meteorColor = configParser.getColor("METEOR_COLOR", CRGB::Red);
    CRGB trailColor = configParser.getColor("TRAIL_COLOR", CRGB::Orange);
    uint8_t meteorSize = configParser.getValue("SIZE", 3);
    uint8_t trailDecay = configParser.getValue("DECAY", 64);
    uint8_t speed = configParser.getValue("SPEED", 30);
    bool randomDecay = configParser.getBool("RANDOM_DECAY", true);
    bool reverse = configParser.getBool("REVERSE", false);
    
    for(int i = 0; i < num_leds; i++) {
        if(randomDecay) {
            if(random8(10) > 5) {
                leds[i] = leds[i].fadeToBlackBy(trailDecay);
            }
        } else {
            leds[i] = leds[i].fadeToBlackBy(trailDecay);
        }
    }
    
    for(int i = 0; i < meteorSize; i++) {
        int idx = reverse ? 
            ((num_leds - 1) - ((pos + i) % num_leds)) : 
            ((pos + i) % num_leds);
            
        if(i == 0) {
            leds[idx] = meteorColor;
        } else {
            leds[idx] = trailColor;
            leds[idx].nscale8(255 - (i * 50));
        }
    }
    
    EVERY_N_MILLISECONDS(speed) {
        pos = (pos + 1) % num_leds;
    }
}

void LEDAnimations::configurable_ripple(CRGB* leds, uint8_t num_leds) {
    static uint8_t center = num_leds / 2;
    static uint8_t wave = 0;
    static bool rippleActive = false;
    
    CRGB rippleColor = configParser.getColor("RIPPLE_COLOR", CRGB::Blue);
    CRGB bgColor = configParser.getColor("BG_COLOR", CRGB::Black);
    uint8_t rippleSpeed = configParser.getValue("SPEED", 50);
    uint8_t fadeRate = configParser.getValue("FADE", 20);
    uint8_t maxWaves = configParser.getValue("MAX_WAVES", 3);
    bool autoTrigger = configParser.getBool("AUTO_TRIGGER", true);
    
    fadeToBlackBy(leds, num_leds, fadeRate);
    
    if(!rippleActive && autoTrigger && random8() < 20) {
        rippleActive = true;
        center = random8(num_leds);
        wave = 0;
    }
    
    if(rippleActive) {
        for(int i = 0; i < num_leds; i++) {
            uint8_t distance = abs(i - center);
            if(distance == wave) {
                uint8_t brightness = 255 - (wave * 255 / num_leds);
                leds[i] = rippleColor;
                leds[i].nscale8(brightness);
            }
        }
        
        EVERY_N_MILLISECONDS(rippleSpeed) {
            wave++;
            if(wave >= num_leds / maxWaves) {
                rippleActive = false;
            }
        }
    } else {
        fill_solid(leds, num_leds, bgColor);
    }
}

CRGB blend_colors(const CRGB& color1, const CRGB& color2, uint8_t blend) {
    uint8_t r = (color1.r * (255 - blend) + color2.r * blend) / 255;
    uint8_t g = (color1.g * (255 - blend) + color2.g * blend) / 255;
    uint8_t b = (color1.b * (255 - blend) + color2.b * blend) / 255;
    return CRGB(r, g, b);
}

void LEDAnimations::configurable_pixels(CRGB* leds, uint8_t num_leds) {
    CRGB color1 = configParser.getColor("COLOR1", CRGB::Blue);
    CRGB color2 = configParser.getColor("COLOR2", CRGB::Red);
    uint8_t density = configParser.getValue("DENSITY", 50);  // 0-100
    uint8_t changeRate = configParser.getValue("CHANGE_RATE", 20);
    uint8_t fadeRate = configParser.getValue("FADE", 20);
    bool smoothTransition = configParser.getBool("SMOOTH", true);
    
    fadeToBlackBy(leds, num_leds, fadeRate);
    
    EVERY_N_MILLISECONDS(changeRate) {
        for(int i = 0; i < num_leds; i++) {
            if(random8(100) < density) {
                if(smoothTransition) {
                    uint8_t blend = random8();
                    leds[i] = blend_colors(color1, color2, blend);
                } else {
                    leds[i] = random8(2) ? color1 : color2;
                }
            }
        }
    }
}

void LEDAnimations::configurable_scanner(CRGB* leds, uint8_t num_leds) {
    static int16_t pos = 0;
    static bool direction = true; 

    CRGB color = configParser.getColor("COLOR", CRGB::Red);
    uint8_t size = configParser.getValue("SIZE", 3);
    uint8_t fadeRate = configParser.getValue("FADE", 128);
    uint8_t speed = configParser.getValue("SPEED", 20);
    bool bounce = configParser.getBool("BOUNCE", true);
    bool trail = configParser.getBool("TRAIL", true);
    
    if(trail) {
        fadeToBlackBy(leds, num_leds, fadeRate);
    } else {
        fill_solid(leds, num_leds, CRGB::Black);
    }
    
    for(int i = 0; i < size; i++) {
        int currentPos = pos - (direction ? i : -i);
        if(currentPos >= 0 && currentPos < num_leds) {
            leds[currentPos] = color;
            leds[currentPos].nscale8(255 - (i * (255 / size)));
        }
    }
    
    EVERY_N_MILLISECONDS(speed) {
        if(direction) {
            pos++;
            if(pos >= num_leds) {
                if(bounce) {
                    direction = false;
                    pos = num_leds - 1;
                } else {
                    pos = 0;
                }
            }
        } else {
            pos--;
            if(pos < 0) {
                if(bounce) {
                    direction = true;
                    pos = 0;
                } else {
                    pos = num_leds - 1;
                }
            }
        }
    }
}

void LEDAnimations::configurable_strobe_patterns(CRGB* leds, uint8_t num_leds) {
    static uint8_t phase = 0;
    static uint8_t count = 0;
    
    CRGB onColor = configParser.getColor("ON_COLOR", CRGB::White);
    CRGB offColor = configParser.getColor("OFF_COLOR", CRGB::Black);
    uint8_t onTime = configParser.getValue("ON_TIME", 50);
    uint8_t offTime = configParser.getValue("OFF_TIME", 50);
    uint8_t pulseCount = configParser.getValue("PULSES", 3);
    uint8_t gapTime = configParser.getValue("GAP_TIME", 500);
    bool alternating = configParser.getBool("ALTERNATING", false);
    
    if(phase == 0) {
        if(alternating) {
            for(int i = 0; i < num_leds; i++) {
                leds[i] = (i % 2 == count % 2) ? onColor : offColor;
            }
        } else {
            fill_solid(leds, num_leds, onColor);
        }
        EVERY_N_MILLISECONDS(onTime) {
            phase = 1;
        }
    } else if(phase == 1) {
        fill_solid(leds, num_leds, offColor);
        EVERY_N_MILLISECONDS(offTime) {
            phase = 0;
            count++;
            if(count >= pulseCount) {
                phase = 2;
                count = 0;
            }
        }
    } else if(phase == 2) { 
        fill_solid(leds, num_leds, offColor);
        EVERY_N_MILLISECONDS(gapTime) {
            phase = 0;
        }
    }
}

void LEDAnimations::configurable_aurora(CRGB* leds, uint8_t num_leds) {
    static uint16_t noise[NUM_LEDS];
    static uint16_t time = 0;
    
    uint8_t hue1 = configParser.getValue("HUE1", 160);
    uint8_t hue2 = configParser.getValue("HUE2", 140);
    uint8_t saturation = configParser.getValue("SAT", 200);
    uint8_t speed = configParser.getValue("SPEED", 20);
    uint8_t scale = configParser.getValue("SCALE", 30);
    uint8_t intensity = configParser.getValue("INTENSITY", 200);
    bool blended = configParser.getBool("BLEND", true);
    
    for(int i = 0; i < num_leds; i++) {
        uint16_t ioffset = scale * i;
        noise[i] = inoise16(time + ioffset, time + ioffset);
        uint8_t noise_val = noise[i] >> 8;
        
        if(blended) {
            uint8_t hue = map(noise_val, 0, 255, hue1, hue2);
            uint8_t bright = map(noise_val, 0, 255, intensity/2, intensity);
            leds[i] = CHSV(hue, saturation, bright);
        } else {
            if(noise_val > 127) {
                leds[i] = CHSV(hue1, saturation, intensity);
            } else {
                leds[i] = CHSV(hue2, saturation, intensity);
            }
        }
    }
    
    EVERY_N_MILLISECONDS(speed) {
        time += 1000;
    }
}

void LEDAnimations::configurable_plasma(CRGB* leds, uint8_t num_leds) {
    static uint16_t time = 0;
    
    uint8_t speed = configParser.getValue("SPEED", 30);
    uint8_t scale = configParser.getValue("SCALE", 20);
    uint8_t hueShift = configParser.getValue("HUE_SHIFT", 0);
    uint8_t saturation = configParser.getValue("SAT", 255);
    bool multiColor = configParser.getBool("MULTI_COLOR", true);
    
    for(int i = 0; i < num_leds; i++) {
        uint8_t value1 = sin8(i * scale + time);
        uint8_t value2 = sin8((i * scale) / 2 + time);
        uint8_t value3 = sin8((i * scale) / 3 + time);
        
        uint8_t combined = (value1 + value2 + value3) / 3;
        
        if(multiColor) {
            uint8_t hue = combined + hueShift;
            leds[i] = CHSV(hue, saturation, combined);
        } else {
            CRGB color = configParser.getColor("COLOR", CRGB::Blue);
            leds[i] = color;
            leds[i].nscale8(combined);
        }
    }
    
    EVERY_N_MILLISECONDS(speed) {
        time += 5;
    }
}

void LEDAnimations::setLed(int index, CRGB color) {
    if (index >= 0 && index < NUM_LEDS) {
        leds[index] = color;
        FastLED.show();
    }
}

void LEDAnimations::setAnimation(int animation_id, long duration, char config[64]) {
    nextAnimation = animation_id;
    animationDuration = duration;
    setConfig(config);
}


void LEDAnimations::startAnimation(int type, int duration) {
    animationType = type;
    animationDuration = duration;
    animationStart = millis();
    animationRunning = true;
}

void LEDAnimations::clear() {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    FastLED.show();
}

void LEDAnimations::runAnimation(bool conected) {
    if (nextAnimation > 0) {
        startAnimation(nextAnimation, animationDuration);
        nextAnimation = 0;
    }

    if (animationRunning && millis() - animationStart >= animationDuration) {
        animationRunning = false;
        setConfig("");
        if (animationType != 5 && conected) {
            startAnimation(5, 5000);
        }
    }

    if (!animationRunning && !conected && animationType != 3) {
        startAnimation(3, 5000);
    }

    switch (animationType) {
        case 1: processing(leds, NUM_LEDS); break;
        case 2: success(leds, NUM_LEDS); break;
        case 3: waiting(leds, NUM_LEDS); break;
        case 4: startup(leds, NUM_LEDS); break;
        case 5: shutdown(leds, NUM_LEDS); break;
        case 6: rainbow_pulse(leds, NUM_LEDS); break;
        case 7: firework(leds, NUM_LEDS); break;
        case 8: police(leds, NUM_LEDS); break;
        case 9: heartbeat(leds, NUM_LEDS); break;
        case 10: fire(leds, NUM_LEDS); break;
        case 11: candy_cane(leds, NUM_LEDS); break;
        case 12: strobe(leds, NUM_LEDS); break;
        case 13: heartbeat_flash(leds, NUM_LEDS); break;
        case 14: device_shutdown(leds, NUM_LEDS); break;
        case 15: blinking_warning(leds, NUM_LEDS); break;
        case 16: wakeWord(leds, NUM_LEDS); break;
        case 17: speechProcessing(leds, NUM_LEDS); break;
        case 18: notificationHighlight(leds, NUM_LEDS); break;
        case 19: errorBlink(leds, NUM_LEDS); break;
        case 20: pairing(leds, NUM_LEDS); break;
        case 21: voiceResponse(leds, NUM_LEDS); break;
        case 22: actionConfirmation(leds, NUM_LEDS); break;
        case 23: configurable_pulse(leds, NUM_LEDS); break;
        case 24: configurable_chase(leds, NUM_LEDS); break;
        case 25: configurable_sparkle(leds, NUM_LEDS); break;
        case 26: configurable_gradient(leds, NUM_LEDS); break;
        case 27: configurable_wave(leds, NUM_LEDS); break;
        case 28: status_indicator(leds, NUM_LEDS); break;
        case 29: progress_indicator(leds, NUM_LEDS); break;
        case 30: configurable_firework(leds, NUM_LEDS); break;
        case 31: configurable_meteor(leds, NUM_LEDS); break;
        case 32: configurable_ripple(leds, NUM_LEDS); break;
        case 33: configurable_pixels(leds, NUM_LEDS); break;
        case 34: configurable_scanner(leds, NUM_LEDS); break;
        case 35: configurable_strobe_patterns(leds, NUM_LEDS); break;
        case 36: configurable_aurora(leds, NUM_LEDS); break;
        case 37: configurable_plasma(leds, NUM_LEDS); break;

        default:
            fill_solid(leds, NUM_LEDS, CRGB::Black);
            break;
    }

    FastLED.show();
}

void LEDAnimations::setup() {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(64);
}
