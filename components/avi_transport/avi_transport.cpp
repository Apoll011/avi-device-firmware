/**
 * @file avi_transport.cpp
 * @brief AVI Protocol Transport Implementation
 */

#include "avi_transport.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char* TAG = "AVI_TRANSPORT";

namespace AVI {

// ============================================================================
// WiFi Manager
// ============================================================================

WiFiManager::WiFiManager(const char* ssid, const char* password)
    : m_ssid(ssid)
    , m_password(password)
    , m_connected(false)
    , m_callback(nullptr)
    , m_wifi_handler(nullptr)
    , m_ip_handler(nullptr) {
}

WiFiManager::~WiFiManager() {
    if (m_wifi_handler) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, m_wifi_handler);
    }
    if (m_ip_handler) {
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, m_ip_handler);
    }
}

bool WiFiManager::init() {
    esp_netif_create_default_wifi_sta();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &eventHandler, this, &m_wifi_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Event handler register failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &eventHandler, this, &m_ip_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Event handler register failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), 
                 m_ssid, sizeof(wifi_config.sta.ssid) - 1);
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), 
                 m_password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set mode failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Set config failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "WiFi initialized (SSID: %s)", m_ssid);
    return true;
}

void WiFiManager::onConnectionChange(ConnectionCallback callback) {
    m_callback = callback;
}

void WiFiManager::eventHandler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    auto* manager = static_cast<WiFiManager*>(arg);
    
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "WiFi started, connecting...");
            esp_wifi_connect();
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "WiFi disconnected, reconnecting...");
            manager->m_connected = false;
            if (manager->m_callback) {
                manager->m_callback(false);
            }
            esp_wifi_connect();
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(event_data);
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        manager->m_connected = true;
        if (manager->m_callback) {
            manager->m_callback(true);
        }
    }
}

// ============================================================================
// UDP Transport
// ============================================================================

UdpTransport::UdpTransport(const char* server_ip, uint16_t port)
    : m_server_ip(server_ip)
    , m_port(port)
    , m_socket(-1)
    , m_connected(false) {
    std::memset(&m_server_addr, 0, sizeof(m_server_addr));
}

UdpTransport::~UdpTransport() {
    disconnect();
}

bool UdpTransport::connect() {
    if (m_connected) {
        return true;
    }
    
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket < 0) {
        ESP_LOGE(TAG, "Failed to create socket: errno %d", errno);
        return false;
    }
    
    m_server_addr.sin_family = AF_INET;
    m_server_addr.sin_port = htons(m_port);
    
    if (inet_pton(AF_INET, m_server_ip, &m_server_addr.sin_addr) != 1) {
        ESP_LOGE(TAG, "Invalid server IP: %s", m_server_ip);
        close(m_socket);
        m_socket = -1;
        return false;
    }
    
    m_connected = true;
    ESP_LOGI(TAG, "UDP connected to %s:%d", m_server_ip, m_port);
    return true;
}

void UdpTransport::disconnect() {
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
    m_connected = false;
}

int32_t UdpTransport::send(const uint8_t* data, size_t length) {
    if (!m_connected) {
        ESP_LOGW(TAG, "UDP not connected");
        return -1;
    }
    
    int sent = sendto(m_socket, data, length, 0,
                     reinterpret_cast<struct sockaddr*>(&m_server_addr),
                     sizeof(m_server_addr));
    
    if (sent < 0) {
        ESP_LOGE(TAG, "UDP send failed: errno %d", errno);
        return -1;
    }
    
    ESP_LOGV(TAG, "Sent %d bytes", sent);
    return 0;
}

int32_t UdpTransport::receive(uint8_t* buffer, size_t buffer_size) {
    if (!m_connected) {
        return 0;
    }
    
    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000; // 1ms
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in source_addr;
    socklen_t socklen = sizeof(source_addr);
    
    int len = recvfrom(m_socket, buffer, buffer_size, 0,
                      reinterpret_cast<struct sockaddr*>(&source_addr), &socklen);
    
    if (len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        ESP_LOGE(TAG, "UDP receive failed: errno %d", errno);
        return -1;
    }
    
    ESP_LOGV(TAG, "Received %d bytes", len);
    return len;
}

} // namespace AVI
