/**
 * @file device_features.h
 * @brief Modular Device Features System
 * 
 * This provides a plugin-like system where features can be enabled/disabled
 * based on hardware capabilities. Each feature is self-contained and can
 * be activated independently.
 */

#pragma once

#include <memory>
#include <vector>
#include "avi_embedded.h"
#include "board_korvo.h"
#include "led_controller.h"

namespace Features {

/**
 * @brief Base class for all device features
 * 
 * Features follow a lifecycle: init() -> start() -> update() -> stop()
 */
class Feature {
public:
    virtual ~Feature() = default;
    
    virtual bool init() = 0;
    virtual bool start() = 0;
    virtual void update() = 0;
    virtual void stop() = 0;
    
    virtual const char* getName() const = 0;
};

/**
 * @brief Button input feature
 * 
 * Monitors button state and reports press events via AVI
 */
class ButtonFeature : public Feature {
public:
    ButtonFeature(AVI_AviEmbedded* avi, uint8_t button_id);
    
    bool init() override;
    bool start() override;
    void update() override;
    void stop() override;
    
    const char* getName() const override { return "Button"; }
    
private:
    void handleButtonEvent(bool pressed, float voltage);
    
    AVI_AviEmbedded* m_avi;
    uint8_t m_button_id;
    std::unique_ptr<class Board::Button> m_button;
};

/**
 * @brief LED strip feature
 * 
 * Controls LED animations and responds to AVI commands
 */
class LedFeature : public Feature {
public:
    explicit LedFeature(AVI_AviEmbedded* avi);
    
    bool init() override;
    bool start() override;
    void update() override;
    void stop() override;
    
    const char* getName() const override { return "LED"; }
    
private:
    void handleLedCommand(const char* topic, size_t topic_len,
                         const uint8_t* data, size_t data_len);
    
    AVI_AviEmbedded* m_avi;
    std::unique_ptr<class LedController> m_leds;
    bool m_connected;
};

/**
 * @brief Audio output feature
 * 
 * Receives audio data via AVI and plays through I2S
 */
class AudioFeature : public Feature {
public:
    explicit AudioFeature(AVI_AviEmbedded* avi);
    
    bool init() override;
    bool start() override;
    void update() override;
    void stop() override;
    
    const char* getName() const override { return "Audio"; }
    
private:
    void handleAudioData(const char* topic, size_t topic_len,
                        const uint8_t* data, size_t data_len);
    
    AVI_AviEmbedded* m_avi;
};

/**
 * @brief Feature Manager
 * 
 * Manages the lifecycle of all enabled features
 */
class FeatureManager {
public:
    explicit FeatureManager(AVI_AviEmbedded* avi);
    
    void addFeature(std::unique_ptr<Feature> feature);
    
    bool initAll();
    bool startAll();
    void updateAll();
    void stopAll();
    
private:
    AVI_AviEmbedded* m_avi;
    std::vector<std::unique_ptr<Feature>> m_features;
};

} // namespace Features
