/**
 * @file device_features.cpp
 * @brief Device Features Implementation
 */

#include "device_features.h"
#include "device_config.h"
#include "esp_log.h"
#include <cstring>
#include "led_strip.h"

#ifdef FEATURE_AUDIO_OUTPUT
#include "driver/i2s.h"
#endif

static const char* TAG = "FEATURES";

namespace Features {

// ============================================================================
// Button Feature
// ============================================================================

#ifdef FEATURE_BUTTON_INPUT

ButtonFeature::ButtonFeature(AVI_AviEmbedded* avi)
    : m_avi(avi)
    , m_button_controller(nullptr) {
}

bool ButtonFeature::init() {
    ESP_LOGI(TAG, "Initializing Button feature (%d buttons)", BUTTON_COUNT);
    
    #ifdef BOARD_ESP32_KORVO_V1_1
    m_button_controller = std::make_unique<Board::ButtonController>(
        BUTTON_ADC_CHANNEL,
        BUTTON_COUNT,
        BUTTON_THRESHOLDS,
        BUTTON_TOLERANCE
    );
    #else
    #error "Button configuration not defined for this board"
    #endif
    
    m_button_controller->init();
    
    // Set up callback for button events
    m_button_controller->onButtonEvent([this](uint8_t button_id, bool pressed) {
        handleButtonEvent(button_id, pressed);
    });
    
    ESP_LOGI(TAG, "Button feature initialized");
    return true;
}

bool ButtonFeature::start() {
    ESP_LOGI(TAG, "Button feature started");
    return true;
}

void ButtonFeature::update() {
    if (m_button_controller) {
        m_button_controller->poll();
    }
}

void ButtonFeature::stop() {
    ESP_LOGI(TAG, "Button feature stopped");
}

void ButtonFeature::handleButtonEvent(uint8_t button_id, bool pressed) {
    const char* button_names[6] = {"REC", "MODE", "PLAY", "SET", "VOL-", "VOL+"};
    
    const char* state_str = pressed ? "pressed" : "released";
    ESP_LOGI(TAG, "Button %d (%s) %s", button_id, 
             button_id < 6 ? button_names[button_id] : "UNKNOWN", state_str);
    
    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"button\":%d,\"name\":\"%s\"}",
             button_id,
             button_id < 6 ? button_names[button_id] : "UNKNOWN");

    if (pressed) {
        uint8_t press_type = 0; // Single press
        int ret = avi_embedded_button_pressed(m_avi, button_id, press_type, payload, strlen(payload));
        ESP_LOGD(TAG, "AVI button event sent [ret=%d]", ret);
    }
}

void ButtonFeature::handleMessage(const char* topic, size_t topic_len,
                               const uint8_t* data, size_t data_len) {}
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
    
    if (!m_leds->init()) {
        ESP_LOGE(TAG, "Failed to initialize LED controller");
        return false;
    }
    
    // Subscribe to LED control topics
    const char* topics[] = {
        TOPIC_LED_CONTROL,
        TOPIC_LED_ANIMATION,
        TOPIC_LED_CLEAR
    };
    
    for (const char* topic : topics) {
        int ret = avi_embedded_subscribe(m_avi, topic, strlen(topic));
        if (ret == 0) {
            ESP_LOGI(TAG, "  ✓ Subscribed to: %s", topic);
        } else {
            ESP_LOGW(TAG, "  ✗ Failed to subscribe to: %s", topic);
        }
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

void LedFeature::setConnected(bool connected) {
    m_connected = connected;
}

void LedFeature::handleMessage(const char* topic, size_t topic_len,
                               const uint8_t* data, size_t data_len) {
    if (data_len == 0) return;
    
    std::string topic_str(topic, topic_len);
    std::string payload(reinterpret_cast<const char*>(data), data_len);
    
    ESP_LOGI(TAG, "LED message on '%s': %s", topic_str.c_str(), payload.c_str());
    
    // LED Control: "index,r,g,b"
    if (topic_str == TOPIC_LED_CONTROL) {
        int index, r, g, b;
        if (sscanf(payload.c_str(), "%d,%d,%d,%d", &index, &r, &g, &b) == 4) {
            if (index >= 0 && index < LED_COUNT) {
                m_leds->setLed(index, RgbColor(r, g, b));
                ESP_LOGI(TAG, "Set LED %d to RGB(%d,%d,%d)", index, r, g, b);
            }
        }
    }
    // LED Animation: "animation_id,duration[,config]"
    else if (topic_str == TOPIC_LED_ANIMATION) {
        int animation_id;
        long duration;
        char config[64] = "";
        
        int parsed = sscanf(payload.c_str(), "%d,%ld,%63s", 
                           &animation_id, &duration, config);
        
        if (parsed >= 2) {
            m_leds->setAnimation(animation_id, duration, config);
            ESP_LOGI(TAG, "Set animation %d, duration %ld", animation_id, duration);
        }
    }
    // LED Clear: "CLEAR"
    else if (topic_str == TOPIC_LED_CLEAR) {
        m_leds->clear();
        ESP_LOGI(TAG, "Cleared all LEDs");
    }
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
    int sub_ret = avi_embedded_subscribe(m_avi, TOPIC_AUDIO_DATA, strlen(TOPIC_AUDIO_DATA));
    if (sub_ret == 0) {
        ESP_LOGI(TAG, "  ✓ Subscribed to: %s", TOPIC_AUDIO_DATA);
    } else {
        ESP_LOGW(TAG, "  ✗ Failed to subscribe to: %s", TOPIC_AUDIO_DATA);
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

void AudioFeature::handleMessage(const char* topic, size_t topic_len,
                                 const uint8_t* data, size_t data_len) {
    if (data_len == 0) return;
    
    std::string topic_str(topic, topic_len);
    if (topic_str == TOPIC_AUDIO_DATA) {
	    size_t bytes_written;
		esp_err_t ret = i2s_write(I2S_NUM_0, data, data_len, &bytes_written, portMAX_DELAY);
    
	    if (ret != ESP_OK) {
		    ESP_LOGW(TAG, "I2S write failed: %s", esp_err_to_name(ret));
	    } else {
	        ESP_LOGD(TAG, "🔊 Played %zu bytes of audio", bytes_written);
	    }
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

void FeatureManager::handleMessage(const char* topic, size_t topic_len,
                                   const uint8_t* data, size_t data_len) {
    std::string topic_str(topic, topic_len);
    
	

    ESP_LOGI(TAG, "Recived msg from topi: %sc", topic);
    
	// Route messages to appropriate features
	//for (auto& feature : m_features) {
    //    feature->handleMessage(topic, topic_len, data, data_len);
    //}
}

} // namespace Features
