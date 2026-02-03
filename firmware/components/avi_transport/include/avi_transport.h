/**
 * @file avi_transport.h
 * @brief AVI Protocol Transport Layer
 * 
 * Provides WiFi connectivity and UDP transport for AVI protocol
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include "esp_wifi.h"
#include "esp_event.h"
#include "lwip/sockets.h"

namespace AVI {

/**
 * @brief WiFi Manager - handles WiFi connection lifecycle
 */
class WiFiManager {
public:
    using ConnectionCallback = std::function<void(bool connected)>;
    
    WiFiManager(const char* ssid, const char* password);
    ~WiFiManager();
    
    bool init();
    void onConnectionChange(ConnectionCallback callback);
    bool isConnected() const { return m_connected; }
    
private:
    static void eventHandler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);
    
    const char* m_ssid;
    const char* m_password;
    bool m_connected;
    ConnectionCallback m_callback;
    
    esp_event_handler_instance_t m_wifi_handler;
    esp_event_handler_instance_t m_ip_handler;
};

/**
 * @brief UDP Transport - handles UDP socket communication
 */
class UdpTransport {
public:
    UdpTransport(const char* server_ip, uint16_t port);
    ~UdpTransport();
    
    bool connect();
    void disconnect();
    
    int32_t send(const uint8_t* data, size_t length);
    int32_t receive(uint8_t* buffer, size_t buffer_size);
    
    bool isConnected() const { return m_connected; }
    
private:
    const char* m_server_ip;
    uint16_t m_port;
    int m_socket;
    bool m_connected;
    struct sockaddr_in m_server_addr;
};

} // namespace AVI
