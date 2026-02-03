#include <stdio.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"

#include "avi_embedded.h"

static const char *TAG = "AVI_DEMO";

// ============================================================================
// Configuration 
// ============================================================================

#define WIFI_SSID      "MEO-1012B0"
#define WIFI_PASSWORD  "2173c715c6"
#define AVI_SERVER_IP  "192.168.1.111"  // Your AVI server IP
#define AVI_SERVER_PORT 8888

#define DEVICE_ID      0x0123456789ABCDEFULL

// ============================================================================
// UDP Context
// ============================================================================

typedef struct {
    int sock;
    struct sockaddr_in server_addr;
    bool connected;
} udp_context_t;

// ============================================================================
// Dummy Sensor Data Generation
// ============================================================================

typedef struct {
    float temperature;
    float humidity;
    uint8_t battery;
    bool led_status;
    int32_t raw_value;
    uint32_t tick_count;
} sensor_state_t;

static sensor_state_t g_sensors = {
    .temperature = 22.5f,
    .humidity = 45.0f,
    .battery = 100,
    .led_status = false,
    .raw_value = 0,
    .tick_count = 0
};

// Update sensors with realistic variations
static void update_dummy_sensors(sensor_state_t* sensors) {
    sensors->tick_count++;
    
    // Temperature: oscillate between 20-25°C
    sensors->temperature = 22.5f + 2.5f * sinf(sensors->tick_count * 0.01f);
    
    // Humidity: oscillate between 40-60%
    sensors->humidity = 50.0f + 10.0f * cosf(sensors->tick_count * 0.015f);
    
    // Battery: slowly drain from 100 to 90, then reset
    sensors->battery = 100 - (sensors->tick_count / 100) % 11;
    
    // LED status: toggle every 10 seconds
    sensors->led_status = (sensors->tick_count / 100) % 2;
    
    // Raw value: counter
    sensors->raw_value = sensors->tick_count;
}

// ============================================================================
// WiFi Event Handler
// ============================================================================

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Initialize WiFi
static esp_err_t wifi_init_sta(void)
{
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");
    return ESP_OK;
}

// ============================================================================
// AVI Callbacks
// ============================================================================

// Message callback - called when pub/sub message received
void on_message_received(void* user_data, const char* topic, size_t topic_len, 
                         const uint8_t* data, size_t data_len) 
{
    char topic_buf[128] = {0};
    size_t copy_len = topic_len < 127 ? topic_len : 127;
    memcpy(topic_buf, topic, copy_len);
    
    ESP_LOGI(TAG, "📨 Message on '%s': %d bytes", topic_buf, data_len);
    
    // Example: Control LED based on topic
    if (strncmp(topic_buf, "device/led/control", 18) == 0 && data_len > 0) {
        bool led_on = data[0] != 0;
        g_sensors.led_status = led_on;
        ESP_LOGI(TAG, "💡 LED control: %s", led_on ? "ON" : "OFF");
    }
    
    // Example: Handle commands
    if (strncmp(topic_buf, "device/command", 14) == 0 && data_len > 0) {
        ESP_LOGI(TAG, "🎮 Command received: %.*s", data_len, data);
    }
}

// UDP send callback
int32_t udp_send_c(void* user_data, const uint8_t* buf, size_t len) 
{
    udp_context_t* ctx = (udp_context_t*)user_data;
    
    if (!ctx->connected) {
        ESP_LOGW(TAG, "UDP not connected, can't send");
        return -1;
    }
    
    int sent = sendto(ctx->sock, buf, len, 0, 
                     (struct sockaddr*)&ctx->server_addr, 
                     sizeof(ctx->server_addr));
    
    if (sent < 0) {
        ESP_LOGE(TAG, "❌ UDP send failed: errno %d", errno);
        return -1;
    }
    
    ESP_LOGD(TAG, "📤 Sent %d bytes via UDP", sent);
    return 0;
}

// UDP receive callback (non-blocking)
int32_t udp_receive(void* user_data, uint8_t* buf, size_t buf_len) 
{
    udp_context_t* ctx = (udp_context_t*)user_data;
    
    if (!ctx->connected) {
        return 0;
    }
    
    // Set non-blocking with short timeout
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000; // 1ms timeout
    setsockopt(ctx->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);
    
    int len = recvfrom(ctx->sock, buf, buf_len, 0, 
                      (struct sockaddr*)&source_addr, &socklen);
    
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // No data available
        }
        ESP_LOGE(TAG, "❌ UDP recv failed: errno %d", errno);
        return -1;
    }
    
    ESP_LOGD(TAG, "📥 Received %d bytes via UDP", len);
    return len;
}

// ============================================================================
// Main Application Task
// ============================================================================

void avi_app_task(void* pvParameters) 
{
    ESP_LOGI(TAG, "🚀 AVI application task started");
    
    // Wait for WiFi connection
    ESP_LOGI(TAG, "⏳ Waiting for WiFi connection...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Create UDP socket
    udp_context_t udp_ctx = {
        .sock = -1,
        .connected = false
    };
    
    udp_ctx.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_ctx.sock < 0) {
        ESP_LOGE(TAG, "❌ Failed to create socket");
        vTaskDelete(NULL);
        return;
    }
    
    // Configure server address
    memset(&udp_ctx.server_addr, 0, sizeof(udp_ctx.server_addr));
    udp_ctx.server_addr.sin_family = AF_INET;
    udp_ctx.server_addr.sin_port = htons(AVI_SERVER_PORT);
    
    if (inet_pton(AF_INET, AVI_SERVER_IP, &udp_ctx.server_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "❌ Invalid server IP address");
        close(udp_ctx.sock);
        vTaskDelete(NULL);
        return;
    }
    
    udp_ctx.connected = true;
    ESP_LOGI(TAG, "✅ UDP socket ready (server: %s:%d)", 
             AVI_SERVER_IP, AVI_SERVER_PORT);
    
    // Allocate scratch buffer for serialization
    static uint8_t scratch_buffer[2048];
    
    // Configure AVI
    AVI_AviEmbeddedConfig config = {
        .device_id = DEVICE_ID
    };
    
    ESP_LOGI(TAG, "🔧 Creating AVI instance (device_id: 0x%llx)...", DEVICE_ID);
    
    // Create AVI instance
	AVI_AviEmbedded* avi = avi_embedded_new(
        config,
        scratch_buffer,
        sizeof(scratch_buffer),
        &udp_ctx,           // UDP user data
        udp_send_c,           // Send callback
        udp_receive,        // Receive callback
        NULL,               // Message callback user data
        on_message_received // Message callback
    );
    
    if (avi == NULL) {
        ESP_LOGE(TAG, "❌ Failed to create AVI instance");
        close(udp_ctx.sock);
        vTaskDelete(NULL);
        return;
    }
    
    ESP_LOGI(TAG, "✅ AVI instance created successfully");
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    
    // Connect to server
    ESP_LOGI(TAG, "🔌 Connecting to AVI server...");
    int ret = avi_embedded_connect(avi);
    if (ret == 0) {
        ESP_LOGI(TAG, "✅ Connect command queued");
    } else {
        ESP_LOGW(TAG, "⚠️  Connect failed: %d", ret);
    }
    
    // Wait for connection to establish
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Subscribe to topics
    ESP_LOGI(TAG, "📢 Subscribing to topics...");
    
    const char* topics[] = {
        "device/led/control",
        "device/command",
        "sensors/requests",
        "broadcast/announcements"
    };
    
    for (int i = 0; i < sizeof(topics) / sizeof(topics[0]); i++) {
        ret = avi_embedded_subscribe(avi, topics[i], strlen(topics[i]));
        if (ret == 0) {
            ESP_LOGI(TAG, "  ✓ Subscribed to: %s", topics[i]);
        } else {
            ESP_LOGW(TAG, "  ✗ Failed to subscribe to: %s", topics[i]);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Main loop counters
    uint32_t loop_count = 0;
    uint32_t last_status_log = 0;
    
    ESP_LOGI(TAG, "🔄 Entering main loop...");
    ESP_LOGI(TAG, "");
    
    // Main loop
    while (1) {
        loop_count++;
        
        // Poll for incoming messages (every loop)
        avi_embedded_poll(avi);
        
        // Log connection status (every 10 seconds)
        if (loop_count - last_status_log >= 100) {
            last_status_log = loop_count;
            if (avi_embedded_is_connected(avi)) {
                ESP_LOGI(TAG, "📊 Status: Connected | Loop: %lu | Heap: %lu", 
                         loop_count, esp_get_free_heap_size());
            } else {
                ESP_LOGW(TAG, "📊 Status: Disconnected | Loop: %lu", loop_count);
            }
        }
        
        // Update dummy sensors
        update_dummy_sensors(&g_sensors);
        
        // === Temperature Sensor (every 2 seconds) ===
        if (loop_count % 20 == 0) {
            const char* sensor_name = "temp_main";
            ret = avi_embedded_update_sensor_temperature(
                avi, sensor_name, strlen(sensor_name), g_sensors.temperature
            );
            ESP_LOGI(TAG, "🌡️  Temperature: %.2f°C [ret=%d]", 
                     g_sensors.temperature, ret);
        }
        
        // === Humidity Sensor (every 3 seconds) ===
        if (loop_count % 30 == 0) {
            const char* sensor_name = "humidity_main";
            ret = avi_embedded_update_sensor_humidity(
                avi, sensor_name, strlen(sensor_name), g_sensors.humidity
            );
            ESP_LOGI(TAG, "💧 Humidity: %.2f%% [ret=%d]", 
                     g_sensors.humidity, ret);
        }
        
        // === Battery Sensor (every 5 seconds) ===
        if (loop_count % 50 == 0) {
            const char* sensor_name = "battery";
            ret = avi_embedded_update_sensor_battery(
                avi, sensor_name, strlen(sensor_name), g_sensors.battery
            );
            ESP_LOGI(TAG, "🔋 Battery: %d%% [ret=%d]", 
                     g_sensors.battery, ret);
        }
        
        // === LED Status (every 10 seconds) ===
        if (loop_count % 100 == 0) {
            const char* sensor_name = "led_status";
            ret = avi_embedded_update_sensor_status(
                avi, sensor_name, strlen(sensor_name), g_sensors.led_status
            );
            ESP_LOGI(TAG, "💡 LED Status: %s [ret=%d]", 
                     g_sensors.led_status ? "ON" : "OFF", ret);
        }
        
        // === Raw Sensor (every 4 seconds) ===
        if (loop_count % 40 == 0) {
            const char* sensor_name = "counter";
            ret = avi_embedded_update_sensor_raw(
                avi, sensor_name, strlen(sensor_name), g_sensors.raw_value
            );
            ESP_LOGI(TAG, "🔢 Counter: %ld [ret=%d]", 
                     g_sensors.raw_value, ret);
        }
        
        // === Button Press Event (every 7 seconds) ===
        if (loop_count % 70 == 0) {
            uint8_t button_id = 1;
            uint8_t press_type = (loop_count / 70) % 3; // Cycle through press types
            const char* press_names[] = {"SINGLE", "DOUBLE", "LONG"};
            
            ret = avi_embedded_button_pressed(avi, button_id, press_type);
            ESP_LOGI(TAG, "🔘 Button %d: %s [ret=%d]", 
                     button_id, press_names[press_type], ret);
        }
        
        // === Publish Status (every 8 seconds) ===
        if (loop_count % 80 == 0) {
            const char* topic = "device/heartbeat";
            char status_msg[64];
            snprintf(status_msg, sizeof(status_msg), 
                     "{\"uptime\":%lu,\"heap\":%lu}", 
                     loop_count, esp_get_free_heap_size());
            
            ret = avi_embedded_publish(
                avi, topic, strlen(topic), 
                (uint8_t*)status_msg, strlen(status_msg)
            );
            ESP_LOGI(TAG, "💬 Published: %s [ret=%d]", status_msg, ret);
        }
        
        // === Stream Demo (every 15 seconds) ===
        if (loop_count % 150 == 0) {
            const char* target = "server";
            const char* reason = "demo_stream";
            uint8_t stream_id = 1;
            
            // Start stream
            ret = avi_embedded_start_stream(
                avi, stream_id, target, strlen(target), reason, strlen(reason)
            );
            ESP_LOGI(TAG, "🎙️  Stream START (id=%d, target=%s) [ret=%d]", 
                     stream_id, target, ret);
            
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Send dummy audio data
            uint8_t dummy_audio[128];
            for (int i = 0; i < sizeof(dummy_audio); i++) {
                dummy_audio[i] = (uint8_t)(rand() % 256);
            }
            
            ret = avi_embedded_send_stream_data(
                avi, stream_id, dummy_audio, sizeof(dummy_audio)
            );
            ESP_LOGI(TAG, "🎙️  Stream DATA (id=%d, %d bytes) [ret=%d]", 
                     stream_id, sizeof(dummy_audio), ret);
            
            vTaskDelay(pdMS_TO_TICKS(100));
            
            // Close stream
            ret = avi_embedded_close_stream(avi, stream_id);
            ESP_LOGI(TAG, "🎙️  Stream CLOSE (id=%d) [ret=%d]", stream_id, ret);
        }
        
        // Loop delay (100ms)
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Cleanup (never reached)
    avi_embedded_free(avi);
    close(udp_ctx.sock);
    vTaskDelete(NULL);
}

// ============================================================================
// Main Entry Point
// ============================================================================

void app_main(void) 
{
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "╔═══════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   AVI Embedded - Complete Demo       ║");
    ESP_LOGI(TAG, "╚═══════════════════════════════════════╝");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Device ID: 0x%llx", DEVICE_ID);
    ESP_LOGI(TAG, "Server:    %s:%d", AVI_SERVER_IP, AVI_SERVER_PORT);
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Initialize WiFi
    ESP_LOGI(TAG, "🌐 Initializing WiFi...");
    wifi_init_sta();
    
    // Initialize AVI system
    ESP_LOGI(TAG, "🔧 Initializing AVI embedded system...");
    avi_embedded_init();
    
    // Create application task
    ESP_LOGI(TAG, "🚀 Creating application task...");
    xTaskCreate(avi_app_task, "avi_app", 8192, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "✅ System initialized, running...");
    ESP_LOGI(TAG, "");
}
