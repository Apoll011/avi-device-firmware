/**
 * @file device_features.cpp
 * @brief Device Features Implementation
 */

#include "device_features.h"
#include "device_config.h"
#include "esp_log.h"
#include <cstring>
#ifdef FEATURE_BUTTON_INPUT
#include "board_korvo.h"
#endif

#ifdef FEATURE_LED_STRIP
#include "led_controller.h"
#endif

#ifdef FEATURE_AUDIO_OUTPUT
#include "driver/i2s.h"
#endif

static const char* TAG = "FEATURES";

namespace Features {

// ============================================================================
// Button Feature
// ============================================================================

#ifdef FEATURE_BUTTON_INPUT

ButtonFeature::ButtonFeature(AVI_AviEmbedded* avi, uint8_t button_id)
    : m_avi(avi)
    , m_button_id(button_id)
    , m_button(nullptr) {
}

bool ButtonFeature::init() {
    ESP_LOGI(TAG, "Initializing Button feature (ID: %d)", m_button_id);
    
    m_button = std::make_unique<Board::Button>(
        PIN_BUTTON_ADC, 
        BUTTON_VOLTAGE_THRESHOLD
    );
    
    m_button->init();
    
    // Set up callback for button state changes
    m_button->onStateChange([this](bool pressed, float voltage) {
        handleButtonEvent(pressed, voltage);
    });
    
    return true;
}

bool ButtonFeature::start() {
    ESP_LOGI(TAG, "Button feature started");
    return true;
}

void ButtonFeature::update() {
    if (m_button) {
        m_button->poll();
    }
}

void ButtonFeature::stop() {
    ESP_LOGI(TAG, "Button feature stopped");
}

void ButtonFeature::handleButtonEvent(bool pressed, float voltage) {
    // Determine press type (for now, just single press)
    // You can extend this to detect double-press, long-press, etc.
    uint8_t press_type = 0; // Single press
    
    if (pressed) {
        int ret = avi_embedded_button_pressed(m_avi, m_button_id, press_type);
        ESP_LOGI(TAG, "🔘 Button %d pressed (%.2fV) [ret=%d]", 
                 m_button_id, voltage, ret);
    } else {
        ESP_LOGI(TAG, "🔘 Button %d released (%.2fV)", m_button_id, voltage);
    }
}

#endif // FEATURE_BUTTON_INPUT

// ============================================================================
// LED Feature
// ============================================================================

#ifdef FEATURE_LED_STRIP

LedFeature::LedFeature(AVI_AviEmbedded* avi)
    : m_avi(avi)
    , m_leds(nullptr)
    , m_connected(false) {
}

bool LedFeature::init() {
    ESP_LOGI(TAG, "Initializing LED feature");
    
    m_leds = std::make_unique<LedController>();
    
	m_leds->init();
    
    // Subscribe to LED control topic
    const char* topic = "device/led/control";
    int ret = avi_embedded_subscribe(m_avi, topic, strlen(topic));
    if (ret != 0) {
        ESP_LOGW(TAG, "Failed to subscribe to %s", topic);
    }
    
    // Subscribe to LED animation topic
    topic = "device/led/animation";
    ret = avi_embedded_subscribe(m_avi, topic, strlen(topic));
    if (ret != 0) {
        ESP_LOGW(TAG, "Failed to subscribe to %s", topic);
    }
    
    return true;
}

bool LedFeature::start() {
    // Start with boot animation
    m_leds->setAnimation(RAINBOW_PULSE, 5000);
    ESP_LOGI(TAG, "LED feature started");
    return true;
}

void LedFeature::update() {
    if (m_leds) {
        m_leds->update(m_connected);
    }
}

void LedFeature::stop() {
    if (m_leds) {
        m_leds->clear();
    }
    ESP_LOGI(TAG, "LED feature stopped");
}

void LedFeature::handleLedCommand(const char* topic, size_t topic_len,
                                  const uint8_t* data, size_t data_len) {
    std::string topic_str(topic, topic_len);
    
    // Parse LED commands
    // Format: "index,r,g,b" or "CLEAR" or "animation_id,duration"
    
    if (data_len == 0) return;
    
    std::string command(reinterpret_cast<const char*>(data), data_len);
    
    ESP_LOGI(TAG, "LED command on '%s': %s", topic_str.c_str(), command.c_str());
    
    // TODO: Implement command parsing
    // This is a placeholder - you'll want to implement proper parsing
}

#endif // FEATURE_LED_STRIP

// ============================================================================
// Audio Feature
// ============================================================================

#ifdef FEATURE_AUDIO_OUTPUT

AudioFeature::AudioFeature(AVI_AviEmbedded* avi)
    : m_avi(avi) {
}

bool AudioFeature::init() {
    ESP_LOGI(TAG, "Initializing Audio feature");
    
    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 64,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    
    i2s_pin_config_t pin_config = {
        .bck_io_num = PIN_I2S_BCK,
        .ws_io_num = PIN_I2S_WS,
        .data_out_num = PIN_I2S_DATA_OUT,
        .data_in_num = -1
    };
    
    esp_err_t ret = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S driver install failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S set pin failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    // Subscribe to audio data topic
    const char* topic = "device/audio/data";
    int sub_ret = avi_embedded_subscribe(m_avi, topic, strlen(topic));
    if (sub_ret != 0) {
        ESP_LOGW(TAG, "Failed to subscribe to %s", topic);
    }
    
    return true;
}

bool AudioFeature::start() {
    ESP_LOGI(TAG, "Audio feature started");
    return true;
}

void AudioFeature::update() {
    // Audio is event-driven via AVI messages
}

void AudioFeature::stop() {
    i2s_driver_uninstall(I2S_NUM_0);
    ESP_LOGI(TAG, "Audio feature stopped");
}

void AudioFeature::handleAudioData(const char* topic, size_t topic_len,
                                   const uint8_t* data, size_t data_len) {
    if (data_len == 0) return;
    
    size_t bytes_written;
    esp_err_t ret = i2s_write(I2S_NUM_0, data, data_len, &bytes_written, portMAX_DELAY);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "I2S write failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Played %zu bytes of audio", bytes_written);
    }
}

#endif // FEATURE_AUDIO_OUTPUT

// ============================================================================
// Feature Manager
// ============================================================================

FeatureManager::FeatureManager(AVI_AviEmbedded* avi)
    : m_avi(avi) {
}

void FeatureManager::addFeature(std::unique_ptr<Feature> feature) {
    if (feature) {
        ESP_LOGI(TAG, "Adding feature: %s", feature->getName());
        m_features.push_back(std::move(feature));
    }
}

bool FeatureManager::initAll() {
    ESP_LOGI(TAG, "Initializing %zu features", m_features.size());
    
    for (auto& feature : m_features) {
        if (!feature->init()) {
            ESP_LOGE(TAG, "Failed to initialize feature: %s", feature->getName());
            return false;
        }
    }
    
    return true;
}

bool FeatureManager::startAll() {
    ESP_LOGI(TAG, "Starting all features");
    
    for (auto& feature : m_features) {
        if (!feature->start()) {
            ESP_LOGE(TAG, "Failed to start feature: %s", feature->getName());
            return false;
        }
    }
    
    return true;
}

void FeatureManager::updateAll() {
    for (auto& feature : m_features) {
        feature->update();
    }
}

void FeatureManager::stopAll() {
    ESP_LOGI(TAG, "Stopping all features");
    
    for (auto& feature : m_features) {
        feature->stop();
    }
}

} // namespace Features
