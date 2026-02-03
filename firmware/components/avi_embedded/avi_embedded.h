#ifndef AVI_EMBEDDED_H
#define AVI_EMBEDDED_H

#include "avi_protocol.h"
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define AVI_OK ESP_OK
#define AVI_ERR_INVALID_ARG ESP_ERR_INVALID_ARG
#define AVI_ERR_NO_MEM ESP_ERR_NO_MEM
#define AVI_ERR_TIMEOUT ESP_ERR_TIMEOUT
#define AVI_ERR_NOT_CONNECTED ESP_FAIL

// Forward declarations
typedef struct avi_embedded_s avi_embedded_t;

// Message handler callback
typedef void (*avi_message_handler_t)(const char *topic, const uint8_t *data, size_t data_len, void *user_ctx);

// UDP send/receive callbacks (to be implemented by user)
typedef esp_err_t (*avi_udp_send_fn_t)(const uint8_t *buf, size_t len, void *udp_ctx);
typedef esp_err_t (*avi_udp_receive_fn_t)(uint8_t *buf, size_t buf_len, size_t *received_len, uint32_t timeout_ms, void *udp_ctx);

// Configuration structure
typedef struct {
    uint64_t device_id;
    avi_udp_send_fn_t udp_send;
    avi_udp_receive_fn_t udp_receive;
    void *udp_context;
    avi_message_handler_t message_handler;
    void *handler_context;
    size_t scratch_buffer_size;  // Recommended: 1024
} avi_embedded_config_t;

// Initialize the AVI embedded client
avi_embedded_t* avi_embedded_init(const avi_embedded_config_t *config);

// Destroy the client and free resources
void avi_embedded_destroy(avi_embedded_t *client);

// Connect to the server
esp_err_t avi_embedded_connect(avi_embedded_t *client, uint32_t timeout_ms);

// Check if connected
bool avi_embedded_is_connected(avi_embedded_t *client);

// Pub/Sub methods
esp_err_t avi_embedded_subscribe(avi_embedded_t *client, const char *topic);
esp_err_t avi_embedded_unsubscribe(avi_embedded_t *client, const char *topic);
esp_err_t avi_embedded_publish(avi_embedded_t *client, const char *topic, const uint8_t *data, size_t data_len);

// Stream methods
esp_err_t avi_embedded_start_stream(avi_embedded_t *client, uint8_t stream_id, const char *target_peer, const char *reason);
esp_err_t avi_embedded_send_audio(avi_embedded_t *client, uint8_t stream_id, const uint8_t *pcm_data, size_t data_len);
esp_err_t avi_embedded_close_stream(avi_embedded_t *client, uint8_t stream_id);

// Event methods
esp_err_t avi_embedded_button_pressed(avi_embedded_t *client, uint8_t button_id, avi_press_type_t press_type);
esp_err_t avi_embedded_update_sensor(avi_embedded_t *client, const char *sensor_name, avi_sensor_value_t *sensor_value);

// Poll for incoming messages (call this in your main loop)
esp_err_t avi_embedded_poll(avi_embedded_t *client, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // AVI_EMBEDDED_H
