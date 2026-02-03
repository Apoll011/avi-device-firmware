#include "avi_embedded.h"
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>

static const char *TAG = "avi_embedded";

struct avi_embedded_s {
    avi_embedded_config_t config;
    uint8_t *scratch_buffer;
    bool is_connected;
};

avi_embedded_t* avi_embedded_init(const avi_embedded_config_t *config) {
    if (!config || !config->udp_send || !config->udp_receive) {
        ESP_LOGE(TAG, "Invalid config");
        return NULL;
    }
    
    if (config->scratch_buffer_size < AVI_MAX_PACKET_SIZE) {
        ESP_LOGE(TAG, "Scratch buffer too small (min %d bytes)", AVI_MAX_PACKET_SIZE);
        return NULL;
    }
    
    avi_embedded_t *client = (avi_embedded_t*)calloc(1, sizeof(avi_embedded_t));
    if (!client) {
        ESP_LOGE(TAG, "Failed to allocate client");
        return NULL;
    }
    
    memcpy(&client->config, config, sizeof(avi_embedded_config_t));
    
    client->scratch_buffer = (uint8_t*)malloc(config->scratch_buffer_size);
    if (!client->scratch_buffer) {
        ESP_LOGE(TAG, "Failed to allocate scratch buffer");
        free(client);
        return NULL;
    }
    
    client->is_connected = false;
    
    ESP_LOGI(TAG, "Client initialized with device_id: %llu", config->device_id);
    
    return client;
}

void avi_embedded_destroy(avi_embedded_t *client) {
    if (!client) return;
    
    if (client->scratch_buffer) {
        free(client->scratch_buffer);
    }
    
    free(client);
    ESP_LOGI(TAG, "Client destroyed");
}

esp_err_t avi_embedded_connect(avi_embedded_t *client, uint32_t timeout_ms) {
    if (!client) return AVI_ERR_INVALID_ARG;
    
    ESP_LOGI(TAG, "Connecting to server...");
    
    // Send Hello message
    int len = avi_encode_hello(client->scratch_buffer, client->config.scratch_buffer_size, client->config.device_id);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode Hello message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send Hello message: %s", esp_err_to_name(err));
        return err;
    }
    
    // Wait for Welcome response
    uint8_t rx_buf[128];
    size_t rx_len;
    err = client->config.udp_receive(rx_buf, sizeof(rx_buf), &rx_len, timeout_ms, client->config.udp_context);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "No response from server: %s", esp_err_to_name(err));
        client->is_connected = false;
        return err;
    }
    
    // Decode response
    avi_downlink_type_t msg_type;
    if (avi_decode_downlink(rx_buf, rx_len, &msg_type, NULL) == 0 && msg_type == AVI_DOWNLINK_WELCOME) {
        client->is_connected = true;
        ESP_LOGI(TAG, "Connected successfully!");
        return ESP_OK;
    }
    
    ESP_LOGE(TAG, "Unexpected response from server");
    client->is_connected = false;
    return ESP_FAIL;
}

bool avi_embedded_is_connected(avi_embedded_t *client) {
    return client ? client->is_connected : false;
}

esp_err_t avi_embedded_subscribe(avi_embedded_t *client, const char *topic) {
    if (!client || !topic) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_subscribe(client->scratch_buffer, client->config.scratch_buffer_size, topic);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode Subscribe message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Subscribed to: %s", topic);
    }
    return err;
}

esp_err_t avi_embedded_unsubscribe(avi_embedded_t *client, const char *topic) {
    if (!client || !topic) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_unsubscribe(client->scratch_buffer, client->config.scratch_buffer_size, topic);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode Unsubscribe message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Unsubscribed from: %s", topic);
    }
    return err;
}

esp_err_t avi_embedded_publish(avi_embedded_t *client, const char *topic, const uint8_t *data, size_t data_len) {
    if (!client || !topic || !data) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_publish(client->scratch_buffer, client->config.scratch_buffer_size, topic, data, data_len);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode Publish message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Published to %s (%d bytes)", topic, data_len);
    }
    return err;
}

esp_err_t avi_embedded_start_stream(avi_embedded_t *client, uint8_t stream_id, const char *target_peer, const char *reason) {
    if (!client || !target_peer || !reason) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_stream_start(client->scratch_buffer, client->config.scratch_buffer_size, stream_id, target_peer, reason);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode StreamStart message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Stream %d started to %s", stream_id, target_peer);
    }
    return err;
}

esp_err_t avi_embedded_send_audio(avi_embedded_t *client, uint8_t stream_id, const uint8_t *pcm_data, size_t data_len) {
    if (!client || !pcm_data) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_stream_data(client->scratch_buffer, client->config.scratch_buffer_size, stream_id, pcm_data, data_len);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode StreamData message");
        return AVI_ERR_INVALID_ARG;
    }
    
    return client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
}

esp_err_t avi_embedded_close_stream(avi_embedded_t *client, uint8_t stream_id) {
    if (!client) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_stream_close(client->scratch_buffer, client->config.scratch_buffer_size, stream_id);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode StreamClose message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Stream %d closed", stream_id);
    }
    return err;
}

esp_err_t avi_embedded_button_pressed(avi_embedded_t *client, uint8_t button_id, avi_press_type_t press_type) {
    if (!client) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_button_press(client->scratch_buffer, client->config.scratch_buffer_size, button_id, press_type);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode ButtonPress message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Button %d pressed (type: %d)", button_id, press_type);
    }
    return err;
}

esp_err_t avi_embedded_update_sensor(avi_embedded_t *client, const char *sensor_name, avi_sensor_value_t *sensor_value) {
    if (!client || !sensor_name || !sensor_value) return AVI_ERR_INVALID_ARG;
    if (!client->is_connected) return AVI_ERR_NOT_CONNECTED;
    
    int len = avi_encode_sensor_update(client->scratch_buffer, client->config.scratch_buffer_size, sensor_name, sensor_value);
    if (len < 0) {
        ESP_LOGE(TAG, "Failed to encode SensorUpdate message");
        return AVI_ERR_INVALID_ARG;
    }
    
    esp_err_t err = client->config.udp_send(client->scratch_buffer, len, client->config.udp_context);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Sensor %s updated", sensor_name);
    }
    return err;
}

esp_err_t avi_embedded_poll(avi_embedded_t *client, uint32_t timeout_ms) {
    if (!client) return AVI_ERR_INVALID_ARG;
    
    uint8_t rx_buf[AVI_MAX_PACKET_SIZE];
    size_t rx_len;
    
    esp_err_t err = client->config.udp_receive(rx_buf, sizeof(rx_buf), &rx_len, timeout_ms, client->config.udp_context);
    if (err == ESP_ERR_TIMEOUT) {
        return ESP_OK;  // Timeout is not an error
    }
    if (err != ESP_OK) {
        return err;
    }
    
    // Decode message
    avi_downlink_type_t msg_type;
    union {
        avi_downlink_error_t error;
        avi_downlink_message_t message;
        avi_downlink_subscribe_ack_t subscribe_ack;
        avi_downlink_unsubscribe_ack_t unsubscribe_ack;
    } msg;
    
    if (avi_decode_downlink(rx_buf, rx_len, &msg_type, &msg) != 0) {
        ESP_LOGW(TAG, "Failed to decode downlink message");
        return ESP_FAIL;
    }
    
    // Handle message
    switch (msg_type) {
        case AVI_DOWNLINK_WELCOME:
            client->is_connected = true;
            ESP_LOGI(TAG, "Received Welcome");
            break;
            
        case AVI_DOWNLINK_ERROR:
            ESP_LOGW(TAG, "Received Error: %d", msg.error.reason);
            break;
            
        case AVI_DOWNLINK_MESSAGE:
            ESP_LOGI(TAG, "Received message on topic: %s", msg.message.topic);
            if (client->config.message_handler) {
                client->config.message_handler(msg.message.topic, msg.message.data, 
                                             msg.message.data_len, client->config.handler_context);
            }
            break;
            
        case AVI_DOWNLINK_SUBSCRIBE_ACK:
            ESP_LOGI(TAG, "Subscribe ACK: %s", msg.subscribe_ack.topic);
            break;
            
        case AVI_DOWNLINK_UNSUBSCRIBE_ACK:
            ESP_LOGI(TAG, "Unsubscribe ACK: %s", msg.unsubscribe_ack.topic);
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg_type);
            break;
    }
    
    return ESP_OK;
}
