#pragma once
#include <FastLED.h>

enum ConfigKey {
    COLOR,
    SPEED,
    BRIGHTNESS,
    PATTERN,
    DIRECTION,
    INTENSITY,
    UNKNOWN
};

enum Speed {
    VERY_SLOW = 5,
    SLOW = 10,
    MEDIUM = 20,
    FAST = 40,
    VERY_FAST = 80
};

enum Direction {
    FORWARD,
    BACKWARD,
    BOUNCE,
    RANDOM
};

enum Pattern {
    SOLID,
    GRADIENT,
    RAINBOW,
    SPARKLE
};

class ConfigParser {
private:
    char currentConfig[64];
    
    bool findKey(const char* key, char* value, size_t maxLen) {
        char searchKey[32];
        snprintf(searchKey, sizeof(searchKey), "%s:", key);
        
        char* found = strstr(currentConfig, searchKey);
        if (!found) return false;
        
        found += strlen(searchKey);
        char* end = strchr(found, ';');
        size_t valueLen = end ? (end - found) : strlen(found);
        
        if (valueLen >= maxLen) valueLen = maxLen - 1;
        strncpy(value, found, valueLen);
        value[valueLen] = '\0';
        
        return true;
    }

public:
    ConfigParser() {
        currentConfig[0] = '\0';
    }
    
    void setConfig(const char* config) {
        strncpy(currentConfig, config, sizeof(currentConfig) - 1);
        currentConfig[sizeof(currentConfig) - 1] = '\0';
    }
    
    int getValue(const char* key, int defaultVal = 0) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultVal;
        
        return atoi(value);
    }
    
    CRGB getColor(const char* key, CRGB defaultColor = CRGB::Black) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultColor;
        
        if (strcmp(value, "RED") == 0) return CRGB::Red;
        if (strcmp(value, "GREEN") == 0) return CRGB::Green;
        if (strcmp(value, "BLUE") == 0) return CRGB::Blue;
        if (strcmp(value, "WHITE") == 0) return CRGB::White;
        if (strcmp(value, "BLACK") == 0) return CRGB::Black;
        if (strcmp(value, "YELLOW") == 0) return CRGB::Yellow;
        if (strcmp(value, "PURPLE") == 0) return CRGB::Purple;
        if (strcmp(value, "ORANGE") == 0) return CRGB::Orange;

        if (strcmp(value, "CRIMSON") == 0) return CRGB::Crimson;
        if (strcmp(value, "DARKRED") == 0) return CRGB::DarkRed;
        if (strcmp(value, "MAROON") == 0) return CRGB::Maroon;
        if (strcmp(value, "PINK") == 0) return CRGB::Pink;
        if (strcmp(value, "DEEPPINK") == 0) return CRGB::DeepPink;
        if (strcmp(value, "HOTPINK") == 0) return CRGB::HotPink;
        if (strcmp(value, "SALMON") == 0) return CRGB::Salmon;
        if (strcmp(value, "CORAL") == 0) return CRGB::Coral;
        if (strcmp(value, "DARKORANGE") == 0) return CRGB::DarkOrange;
        if (strcmp(value, "GOLDENROD") == 0) return CRGB::Goldenrod;
        if (strcmp(value, "GOLD") == 0) return CRGB::Gold;
        if (strcmp(value, "CHOCOLATE") == 0) return CRGB::Chocolate;
        if (strcmp(value, "BROWN") == 0) return CRGB::Brown;
        if (strcmp(value, "SIENNA") == 0) return CRGB::Sienna;
        if (strcmp(value, "SANDYBROWN") == 0) return CRGB::SandyBrown;
        if (strcmp(value, "PEACHPUFF") == 0) return CRGB::PeachPuff;
        if (strcmp(value, "LIGHTYELLOW") == 0) return CRGB::LightYellow;
        if (strcmp(value, "KHAKI") == 0) return CRGB::Khaki;
        if (strcmp(value, "DARKKHAKI") == 0) return CRGB::DarkKhaki;
        if (strcmp(value, "OLIVE") == 0) return CRGB::Olive;
        if (strcmp(value, "LIME") == 0) return CRGB::Lime;
        if (strcmp(value, "LIMEGREEN") == 0) return CRGB::LimeGreen;
        if (strcmp(value, "FORESTGREEN") == 0) return CRGB::ForestGreen;
        if (strcmp(value, "SEAGREEN") == 0) return CRGB::SeaGreen;
        if (strcmp(value, "SPRINGGREEN") == 0) return CRGB::SpringGreen;
        if (strcmp(value, "DARKGREEN") == 0) return CRGB::DarkGreen;
        if (strcmp(value, "MEDIUMSEAGREEN") == 0) return CRGB::MediumSeaGreen;
        if (strcmp(value, "PALEGREEN") == 0) return CRGB::PaleGreen;
        if (strcmp(value, "CYAN") == 0) return CRGB::Cyan;
        if (strcmp(value, "DARKCYAN") == 0) return CRGB::DarkCyan;
        if (strcmp(value, "LIGHTBLUE") == 0) return CRGB::LightBlue;
        if (strcmp(value, "DEEPSKYBLUE") == 0) return CRGB::DeepSkyBlue;
        if (strcmp(value, "NAVY") == 0) return CRGB::Navy;
        if (strcmp(value, "ROYALBLUE") == 0) return CRGB::RoyalBlue;
        if (strcmp(value, "MEDIUMBLUE") == 0) return CRGB::MediumBlue;
        if (strcmp(value, "AQUAMARINE") == 0) return CRGB::Aquamarine;
        if (strcmp(value, "MAGENTA") == 0) return CRGB::Magenta;
        if (strcmp(value, "DARKMAGENTA") == 0) return CRGB::DarkMagenta;
        if (strcmp(value, "VIOLET") == 0) return CRGB::Violet;
        if (strcmp(value, "PLUM") == 0) return CRGB::Plum;
        if (strcmp(value, "DARKVIOLET") == 0) return CRGB::DarkViolet;
        if (strcmp(value, "INDIGO") == 0) return CRGB::Indigo;
        if (strcmp(value, "MEDIUMORCHID") == 0) return CRGB::MediumOrchid;
        if (strcmp(value, "BLUEVIOLET") == 0) return CRGB::BlueViolet;
        if (strcmp(value, "SNOW") == 0) return CRGB::Snow;
        if (strcmp(value, "GHOSTWHITE") == 0) return CRGB::GhostWhite;
        if (strcmp(value, "WHITESMOKE") == 0) return CRGB::WhiteSmoke;
        if (strcmp(value, "SILVER") == 0) return CRGB::Silver;
        if (strcmp(value, "GRAY") == 0) return CRGB::Gray;
        if (strcmp(value, "DARKGRAY") == 0) return CRGB::DarkGray;
        if (strcmp(value, "DIMGRAY") == 0) return CRGB::DimGray;
        if (strcmp(value, "GAINSBORO") == 0) return CRGB::Gainsboro;
        
        if (value[0] == '#' && strlen(value) == 7) {
            uint32_t colorVal;
            sscanf(value + 1, "%x", &colorVal);
            return CRGB(
                (colorVal >> 16) & 0xFF,
                (colorVal >> 8) & 0xFF,
                colorVal & 0xFF
            );
        }
        
        return defaultColor;
    }
    
    Speed getSpeed(const char* key, Speed defaultSpeed = Speed::MEDIUM) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultSpeed;
        
        if (strcmp(value, "VERY_SLOW") == 0) return Speed::VERY_SLOW;
        if (strcmp(value, "SLOW") == 0) return Speed::SLOW;
        if (strcmp(value, "MEDIUM") == 0) return Speed::MEDIUM;
        if (strcmp(value, "FAST") == 0) return Speed::FAST;
        if (strcmp(value, "VERY_FAST") == 0) return Speed::VERY_FAST;
        
        return defaultSpeed;
    }
    
    Direction getDirection(const char* key, Direction defaultDir = Direction::FORWARD) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultDir;
        
        if (strcmp(value, "FORWARD") == 0) return Direction::FORWARD;
        if (strcmp(value, "BACKWARD") == 0) return Direction::BACKWARD;
        if (strcmp(value, "BOUNCE") == 0) return Direction::BOUNCE;
        if (strcmp(value, "RANDOM") == 0) return Direction::RANDOM;
        
        return defaultDir;
    }
    
    Pattern getPattern(const char* key, Pattern defaultPattern = Pattern::SOLID) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultPattern;
        
        if (strcmp(value, "SOLID") == 0) return Pattern::SOLID;
        if (strcmp(value, "GRADIENT") == 0) return Pattern::GRADIENT;
        if (strcmp(value, "RAINBOW") == 0) return Pattern::RAINBOW;
        if (strcmp(value, "SPARKLE") == 0) return Pattern::SPARKLE;
        
        return defaultPattern;
    }
    
    float getFloat(const char* key, float defaultVal = 0.0f) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultVal;
        
        return atof(value);
    }
    
    bool getBool(const char* key, bool defaultVal = false) {
        char value[16];
        if (!findKey(key, value, sizeof(value))) return defaultVal;
        
        return (strcmp(value, "TRUE") == 0 || strcmp(value, "1") == 0);
    }
};
