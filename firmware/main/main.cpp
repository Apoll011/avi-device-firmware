/**
 * @file main.cpp
 * @brief AVI Firmware Main Application
 * 
 * This is the entry point for the AVI-enabled firmware. The application
 * automatically detects and enables features based on the board configuration.
 */

#include <memory>
#include <cstring>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "device_config.h"
#include "avi_transport.h"
#include "device_features.h"
#include "avi_embedded.h"

static const char* TAG = "MAIN";

// ============================================================================
// AVI Client Wrapper
// ============================================================================

/**
 * @brief High-level AVI client that integrates transport and protocol
 */
class AviClient {
public:
    AviClient(AVI::UdpTransport& transport)
        : m_transport(transport)
        , m_avi(nullptr) {
    }
    
    ~AviClient() {
        if (m_avi) {
            avi_embedded_free(m_avi);
        }
    }
    
    bool init() {
        AVI_AviEmbeddedConfig config = {
            .device_id = DEVICE_ID
        };
        
        ESP_LOGI(TAG, "Initializing AVI (device: 0x%llx)", DEVICE_ID);
        
        m_avi = avi_embedded_new(
            config,
            m_scratch_buffer,
            sizeof(m_scratch_buffer),
            &m_transport,
            &AviClient::sendCallback,
            &AviClient::receiveCallback,
            nullptr,
            nullptr  // Message callback handled by features
        );
        
        if (!m_avi) {
            ESP_LOGE(TAG, "Failed to create AVI instance");
            return false;
        }
        
        ESP_LOGI(TAG, "AVI initialized (heap: %lu)", esp_get_free_heap_size());
        return true;
    }
    
    bool connect() {
        int ret = avi_embedded_connect(m_avi);
        if (ret == 0) {
            ESP_LOGI(TAG, "AVI connect queued");
            return true;
        }
        ESP_LOGW(TAG, "AVI connect failed: %d", ret);
        return false;
    }
    
    void poll() {
        if (m_avi) {
            avi_embedded_poll(m_avi);
        }
    }
    
    bool isConnected() const {
        return m_avi && avi_embedded_is_connected(m_avi);
    }
    
    AVI_AviEmbedded* getHandle() { return m_avi; }
    
private:
    static int32_t sendCallback(void* user_data, const uint8_t* buf, size_t len) {
        auto* transport = static_cast<AVI::UdpTransport*>(user_data);
        return transport->send(buf, len);
    }
    
    static int32_t receiveCallback(void* user_data, uint8_t* buf, size_t len) {
        auto* transport = static_cast<AVI::UdpTransport*>(user_data);
        return transport->receive(buf, len);
    }
    
    AVI::UdpTransport& m_transport;
    AVI_AviEmbedded* m_avi;
    uint8_t m_scratch_buffer[SCRATCH_BUFFER_SIZE];
};

// ============================================================================
// Application
// ============================================================================

class Application {
public:
    Application()
        : m_wifi(WIFI_SSID, WIFI_PASSWORD)
        , m_transport(AVI_SERVER_IP, AVI_SERVER_PORT)
        , m_client(m_transport)
        , m_features(nullptr)
        , m_wifi_connected(false) {
    }
    
    bool init() {
        ESP_LOGI(TAG, "Initializing application");
        
        // Initialize WiFi
        if (!m_wifi.init()) {
            ESP_LOGE(TAG, "WiFi initialization failed");
            return false;
        }
        
        // Set up WiFi connection callback
        m_wifi.onConnectionChange([this](bool connected) {
            m_wifi_connected = connected;
            if (connected) {
                onWiFiConnected();
            } else {
                onWiFiDisconnected();
            }
        });
        
        return true;
    }
    
    void run() {
        ESP_LOGI(TAG, "Application running");
        
        uint32_t loop_count = 0;
        
        while (true) {
            loop_count++;
            
            // Poll AVI protocol
            m_client.poll();
            
            // Update all features
            if (m_features) {
                m_features->updateAll();
            }
            
            vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_INTERVAL_MS));
        }
    }
    
private:
    void onWiFiConnected() {
        ESP_LOGI(TAG, "WiFi connected, setting up AVI");
        
        // Connect UDP transport
        if (!m_transport.connect()) {
            ESP_LOGE(TAG, "UDP transport connection failed");
            return;
        }
        
        // Initialize AVI client
        if (!m_client.init()) {
            ESP_LOGE(TAG, "AVI client initialization failed");
            return;
        }
        
        // Connect to AVI server
        if (!m_client.connect()) {
            ESP_LOGW(TAG, "AVI server connection failed");
            return;
        }
        
        vTaskDelay(pdMS_TO_TICKS(AVI_CONNECT_DELAY_MS));
        
        // Initialize features
        setupFeatures();
    }
    
    void onWiFiDisconnected() {
        ESP_LOGW(TAG, "WiFi disconnected");
        
        if (m_features) {
            m_features->stopAll();
        }
    }
    
    void setupFeatures() {
        ESP_LOGI(TAG, "Setting up device features");
        
        m_features = std::make_unique<Features::FeatureManager>(m_client.getHandle());
        
        // Add features based on board configuration
        
#ifdef FEATURE_BUTTON_INPUT
        m_features->addFeature(
            std::make_unique<Features::ButtonFeature>(m_client.getHandle())
        );
#endif

#ifdef FEATURE_LED_STRIP
        m_features->addFeature(
            std::make_unique<Features::LedFeature>(m_client.getHandle())
        );
#endif

#ifdef FEATURE_AUDIO_OUTPUT
        m_features->addFeature(
            std::make_unique<Features::AudioFeature>(m_client.getHandle())
        );
#endif
        
        // Initialize all features
        if (!m_features->initAll()) {
            ESP_LOGE(TAG, "Feature initialization failed");
            return;
        }
        
        // Start all features
        if (!m_features->startAll()) {
            ESP_LOGE(TAG, "Feature start failed");
            return;
        }
        
        ESP_LOGI(TAG, "All features initialized and started");
    }
    
    AVI::WiFiManager m_wifi;
    AVI::UdpTransport m_transport;
    AviClient m_client;
    std::unique_ptr<Features::FeatureManager> m_features;
    bool m_wifi_connected;
};

// ============================================================================
// FreeRTOS Task
// ============================================================================

static void app_task(void* pvParameters) {
    ESP_LOGI(TAG, "Application task started");
    
    auto* app = static_cast<Application*>(pvParameters);
    app->run();
    
    vTaskDelete(nullptr);
}

// ============================================================================
// Main Entry Point
// ============================================================================

extern "C" void app_main() {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║         AVI Embedded Firmware         ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Device:    %s", DEVICE_NAME);
    ESP_LOGI(TAG, "ID:        0x%llx", DEVICE_ID);
    ESP_LOGI(TAG, "Board:     ESP32 Korvo v1.1");
    ESP_LOGI(TAG, "Server:    %s:%d", AVI_SERVER_IP, AVI_SERVER_PORT);
    ESP_LOGI(TAG, "");
    
    // Log enabled features
    ESP_LOGI(TAG, "Enabled features:");
#ifdef FEATURE_BUTTON_INPUT
    ESP_LOGI(TAG, "  • Button Input");
#endif
#ifdef FEATURE_LED_STRIP
    ESP_LOGI(TAG, "  • LED Strip");
#endif
#ifdef FEATURE_AUDIO_OUTPUT
    ESP_LOGI(TAG, "  • Audio Output");
#endif
#ifdef FEATURE_MICROPHONE
    ESP_LOGI(TAG, "  • Microphone");
#endif
    ESP_LOGI(TAG, "");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize network stack
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Initialize AVI system
    ESP_LOGI(TAG, "Initializing AVI embedded system");
    avi_embedded_init();
    
    // Create application
    static Application app;
    
    if (!app.init()) {
        ESP_LOGE(TAG, "Application initialization failed");
        return;
    }
    
    // Create application task
    xTaskCreate(app_task, "app_main", 
                MAIN_TASK_STACK_SIZE, 
                &app, 
                MAIN_TASK_PRIORITY, 
                nullptr);
    
    ESP_LOGI(TAG, "System started");
    ESP_LOGI(TAG, "");
}
