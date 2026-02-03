#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "avi_embedded.h"

// WiFi Configuration
#define WIFI_SSID      "YourWiFiSSID"
#define WIFI_PASS      "YourWiFiPassword"

// Server Configuration
#define SERVER_IP      "192.168.1.100"
#define SERVER_PORT    8080

// Device Configuration
#define DEVICE_ID      0x1234567890ABCDEF

static const char *TAG = "avi_example";

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// UDP socket
static int udp_sock = -1;
static struct sockaddr_in server_addr;

// AVI client
static avi_embedded_t *avi_client = NULL;

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from AP, retrying...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initialize WiFi
static void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi initialization finished. Waiting for connection...");
    xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
}

// Initialize UDP socket
static esp_err_t udp_init(void) {
    udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_sock < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    
    // Set socket to non-blocking mode
    int flags = fcntl(udp_sock, F_GETFL, 0);
    fcntl(udp_sock, F_SETFL, flags | O_NONBLOCK);
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    
    ESP_LOGI(TAG, "UDP socket initialized");
    return ESP_OK;
}

// UDP send callback for AVI client
static esp_err_t udp_send_callback(const uint8_t *buf, size_t len, void *udp_ctx) {
    (void)udp_ctx;  // Unused
    
    int sent = sendto(udp_sock, buf, len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent < 0) {
        ESP_LOGE(TAG, "UDP send failed: errno %d", errno);
        return ESP_FAIL;
    }
    
    ESP_LOGD(TAG, "Sent %d bytes via UDP", sent);
    return ESP_OK;
}

// UDP receive callback for AVI client
static esp_err_t udp_receive_callback(uint8_t *buf, size_t buf_len, size_t *received_len, 
                                      uint32_t timeout_ms, void *udp_ctx) {
    (void)udp_ctx;  // Unused
    
    // Set socket timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);
    
    int len = recvfrom(udp_sock, buf, buf_len, 0, (struct sockaddr *)&source_addr, &socklen);
    
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return ESP_ERR_TIMEOUT;
        }
        ESP_LOGE(TAG, "UDP receive failed: errno %d", errno);
        return ESP_FAIL;
    }
    
    *received_len = len;
    ESP_LOGD(TAG, "Received %d bytes via UDP", len);
    return ESP_OK;
}

// Message handler callback
static void message_handler(const char *topic, const uint8_t *data, size_t data_len, void *user_ctx) {
    (void)user_ctx;  // Unused
    
    ESP_LOGI(TAG, "=== Message Received ===");
    ESP_LOGI(TAG, "Topic: %s", topic);
    ESP_LOGI(TAG, "Data length: %d", data_len);
    
    // Print data as hex
    ESP_LOG_BUFFER_HEX("Message data", data, data_len);
    
    // Try to print as string if it looks like text
    if (data_len > 0 && data_len < 256) {
        char text[257];
        memcpy(text, data, data_len);
        text[data_len] = '\0';
        ESP_LOGI(TAG, "Data as text: %s", text);
    }
}

// Example: Button press simulation task
static void button_task(void *pvParameters) {
    uint8_t button_count = 0;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));  // Every 5 seconds
        
        if (avi_embedded_is_connected(avi_client)) {
            // Simulate button press
            avi_press_type_t press_type = (button_count % 3 == 0) ? AVI_PRESS_SINGLE :
                                          (button_count % 3 == 1) ? AVI_PRESS_DOUBLE :
                                          AVI_PRESS_LONG;
            
            esp_err_t err = avi_embedded_button_pressed(avi_client, 1, press_type);
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Button press sent (type: %d)", press_type);
            }
            
            button_count++;
        }
    }
}

// Example: Sensor update simulation task
static void sensor_task(void *pvParameters) {
    float temperature = 20.0f;
    uint8_t battery = 100;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));  // Every 10 seconds
        
        if (avi_embedded_is_connected(avi_client)) {
            // Update temperature
            avi_sensor_value_t temp_sensor = {
                .type = AVI_SENSOR_TEMPERATURE,
                .value.temperature = temperature
            };
            avi_embedded_update_sensor(avi_client, "temp_room", &temp_sensor);
            temperature += 0.5f;  // Simulate temperature change
            
            // Update battery
            avi_sensor_value_t battery_sensor = {
                .type = AVI_SENSOR_BATTERY,
                .value.battery = battery
            };
            avi_embedded_update_sensor(avi_client, "battery", &battery_sensor);
            if (battery > 0) battery--;
            
            ESP_LOGI(TAG, "Sensor data sent (temp: %.1f°C, battery: %d%%)", 
                     temp_sensor.value.temperature, battery);
        }
    }
}

// Main AVI client task
static void avi_client_task(void *pvParameters) {
    // Initialize UDP
    ESP_ERROR_CHECK(udp_init());
    
    // Configure AVI client
    avi_embedded_config_t config = {
        .device_id = DEVICE_ID,
        .udp_send = udp_send_callback,
        .udp_receive = udp_receive_callback,
        .udp_context = NULL,
        .message_handler = message_handler,
        .handler_context = NULL,
        .scratch_buffer_size = 1024
    };
    
    // Initialize client
    avi_client = avi_embedded_init(&config);
    if (!avi_client) {
        ESP_LOGE(TAG, "Failed to initialize AVI client");
        vTaskDelete(NULL);
        return;
    }
    
    // Connect to server
    ESP_LOGI(TAG, "Connecting to server %s:%d...", SERVER_IP, SERVER_PORT);
    esp_err_t err = avi_embedded_connect(avi_client, 5000);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to server");
        avi_embedded_destroy(avi_client);
        vTaskDelete(NULL);
        return;
    }
    
    // Subscribe to topics
    avi_embedded_subscribe(avi_client, "commands");
    avi_embedded_subscribe(avi_client, "notifications");
    
    // Publish a test message
    const char *test_msg = "Hello from ESP32!";
    avi_embedded_publish(avi_client, "status", (const uint8_t *)test_msg, strlen(test_msg));
    
    // Start example tasks
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
    
    // Main poll loop
    while (1) {
        // Poll for incoming messages
        avi_embedded_poll(avi_client, 100);  // 100ms timeout
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "AVI Embedded Client Example Starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize WiFi
    wifi_init();
    
    ESP_LOGI(TAG, "WiFi connected, starting AVI client...");
    
    // Start AVI client task
    xTaskCreate(avi_client_task, "avi_client", 8192, NULL, 5, NULL);
}
