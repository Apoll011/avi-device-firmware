#ifndef AVI_PROTOCOL_H
#define AVI_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define AVI_MAX_PACKET_SIZE 1024
#define AVI_MAX_TOPIC_LEN 128
#define AVI_MAX_DATA_LEN 512
#define AVI_MAX_PEER_ID_LEN 64
#define AVI_MAX_REASON_LEN 128
#define AVI_MAX_SENSOR_NAME_LEN 64

// Press type enumeration
typedef enum {
    AVI_PRESS_SINGLE = 0,
    AVI_PRESS_DOUBLE = 1,
    AVI_PRESS_LONG = 2
} avi_press_type_t;

// Sensor value types
typedef enum {
    AVI_SENSOR_TEMPERATURE = 0,
    AVI_SENSOR_HUMIDITY = 1,
    AVI_SENSOR_BATTERY = 2,
    AVI_SENSOR_STATUS = 3,
    AVI_SENSOR_RAW = 4
} avi_sensor_type_t;

// Sensor value union
typedef struct {
    avi_sensor_type_t type;
    union {
        float temperature;
        float humidity;
        uint8_t battery;
        bool status;
        int32_t raw;
    } value;
} avi_sensor_value_t;

// Uplink message types (device -> server)
typedef enum {
    AVI_UPLINK_HELLO = 0,
    AVI_UPLINK_SUBSCRIBE = 1,
    AVI_UPLINK_UNSUBSCRIBE = 2,
    AVI_UPLINK_PUBLISH = 3,
    AVI_UPLINK_STREAM_START = 4,
    AVI_UPLINK_STREAM_DATA = 5,
    AVI_UPLINK_STREAM_CLOSE = 6,
    AVI_UPLINK_BUTTON_PRESS = 7,
    AVI_UPLINK_SENSOR_UPDATE = 8
} avi_uplink_type_t;

// Downlink message types (server -> device)
typedef enum {
    AVI_DOWNLINK_WELCOME = 0,
    AVI_DOWNLINK_ERROR = 1,
    AVI_DOWNLINK_MESSAGE = 2,
    AVI_DOWNLINK_SUBSCRIBE_ACK = 3,
    AVI_DOWNLINK_UNSUBSCRIBE_ACK = 4
} avi_downlink_type_t;

// Uplink message structures
typedef struct {
    uint64_t device_id;
} avi_uplink_hello_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
} avi_uplink_subscribe_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
} avi_uplink_unsubscribe_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
    uint8_t data[AVI_MAX_DATA_LEN];
    size_t data_len;
} avi_uplink_publish_t;

typedef struct {
    uint8_t local_stream_id;
    char target_peer_id[AVI_MAX_PEER_ID_LEN];
    char reason[AVI_MAX_REASON_LEN];
} avi_uplink_stream_start_t;

typedef struct {
    uint8_t local_stream_id;
    uint8_t data[AVI_MAX_DATA_LEN];
    size_t data_len;
} avi_uplink_stream_data_t;

typedef struct {
    uint8_t local_stream_id;
} avi_uplink_stream_close_t;

typedef struct {
    uint8_t button_id;
    avi_press_type_t press_type;
} avi_uplink_button_press_t;

typedef struct {
    char sensor_name[AVI_MAX_SENSOR_NAME_LEN];
    avi_sensor_value_t data;
} avi_uplink_sensor_update_t;

// Downlink message structures
typedef struct {
    uint8_t reason;
} avi_downlink_error_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
    uint8_t data[AVI_MAX_DATA_LEN];
    size_t data_len;
} avi_downlink_message_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
} avi_downlink_subscribe_ack_t;

typedef struct {
    char topic[AVI_MAX_TOPIC_LEN];
} avi_downlink_unsubscribe_ack_t;

// Protocol encoding/decoding functions
int avi_encode_hello(uint8_t *buf, size_t buf_len, uint64_t device_id);
int avi_encode_subscribe(uint8_t *buf, size_t buf_len, const char *topic);
int avi_encode_unsubscribe(uint8_t *buf, size_t buf_len, const char *topic);
int avi_encode_publish(uint8_t *buf, size_t buf_len, const char *topic, const uint8_t *data, size_t data_len);
int avi_encode_stream_start(uint8_t *buf, size_t buf_len, uint8_t stream_id, const char *target_peer, const char *reason);
int avi_encode_stream_data(uint8_t *buf, size_t buf_len, uint8_t stream_id, const uint8_t *data, size_t data_len);
int avi_encode_stream_close(uint8_t *buf, size_t buf_len, uint8_t stream_id);
int avi_encode_button_press(uint8_t *buf, size_t buf_len, uint8_t button_id, avi_press_type_t press_type);
int avi_encode_sensor_update(uint8_t *buf, size_t buf_len, const char *sensor_name, avi_sensor_value_t *sensor_value);

int avi_decode_downlink(const uint8_t *buf, size_t buf_len, avi_downlink_type_t *type, void *msg_out);

#ifdef __cplusplus
}
#endif

#endif // AVI_PROTOCOL_H
