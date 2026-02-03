#include "avi_protocol.h"
#include <string.h>

// Postcard encoding helpers
static inline void encode_varint(uint8_t **ptr, uint64_t value) {
    while (value >= 0x80) {
        **ptr = (uint8_t)(value | 0x80);
        (*ptr)++;
        value >>= 7;
    }
    **ptr = (uint8_t)value;
    (*ptr)++;
}

static inline int decode_varint(const uint8_t **ptr, const uint8_t *end, uint64_t *value) {
    *value = 0;
    int shift = 0;
    while (*ptr < end) {
        uint8_t byte = **ptr;
        (*ptr)++;
        *value |= ((uint64_t)(byte & 0x7F)) << shift;
        if ((byte & 0x80) == 0) {
            return 0;
        }
        shift += 7;
        if (shift >= 64) return -1;
    }
    return -1;
}

static inline void encode_string(uint8_t **ptr, const char *str) {
    size_t len = strlen(str);
    encode_varint(ptr, len);
    memcpy(*ptr, str, len);
    *ptr += len;
}

static inline int decode_string(const uint8_t **ptr, const uint8_t *end, char *out, size_t max_len) {
    uint64_t len;
    if (decode_varint(ptr, end, &len) != 0) return -1;
    if (len >= max_len || *ptr + len > end) return -1;
    memcpy(out, *ptr, len);
    out[len] = '\0';
    *ptr += len;
    return 0;
}

static inline void encode_bytes(uint8_t **ptr, const uint8_t *data, size_t len) {
    encode_varint(ptr, len);
    memcpy(*ptr, data, len);
    *ptr += len;
}

static inline int decode_bytes(const uint8_t **ptr, const uint8_t *end, uint8_t *out, size_t *out_len, size_t max_len) {
    uint64_t len;
    if (decode_varint(ptr, end, &len) != 0) return -1;
    if (len > max_len || *ptr + len > end) return -1;
    memcpy(out, *ptr, len);
    *out_len = len;
    *ptr += len;
    return 0;
}

// Encode Hello message
int avi_encode_hello(uint8_t *buf, size_t buf_len, uint64_t device_id) {
    if (buf_len < 20) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for Hello = 0
    encode_varint(&ptr, 0);
    
    // Struct with one field: device_id
    encode_varint(&ptr, device_id);
    
    return ptr - buf;
}

// Encode Subscribe message
int avi_encode_subscribe(uint8_t *buf, size_t buf_len, const char *topic) {
    if (buf_len < 2 + strlen(topic)) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for Subscribe = 1
    encode_varint(&ptr, 1);
    
    // Struct with one field: topic
    encode_string(&ptr, topic);
    
    return ptr - buf;
}

// Encode Unsubscribe message
int avi_encode_unsubscribe(uint8_t *buf, size_t buf_len, const char *topic) {
    if (buf_len < 2 + strlen(topic)) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for Unsubscribe = 2
    encode_varint(&ptr, 2);
    
    // Struct with one field: topic
    encode_string(&ptr, topic);
    
    return ptr - buf;
}

// Encode Publish message
int avi_encode_publish(uint8_t *buf, size_t buf_len, const char *topic, const uint8_t *data, size_t data_len) {
    if (buf_len < 10 + strlen(topic) + data_len) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for Publish = 3
    encode_varint(&ptr, 3);
    
    // Struct with two fields: topic, data
    encode_string(&ptr, topic);
    encode_bytes(&ptr, data, data_len);
    
    return ptr - buf;
}

// Encode StreamStart message
int avi_encode_stream_start(uint8_t *buf, size_t buf_len, uint8_t stream_id, const char *target_peer, const char *reason) {
    if (buf_len < 10 + strlen(target_peer) + strlen(reason)) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for StreamStart = 4
    encode_varint(&ptr, 4);
    
    // Struct with three fields
    *ptr++ = stream_id;
    encode_string(&ptr, target_peer);
    encode_string(&ptr, reason);
    
    return ptr - buf;
}

// Encode StreamData message
int avi_encode_stream_data(uint8_t *buf, size_t buf_len, uint8_t stream_id, const uint8_t *data, size_t data_len) {
    if (buf_len < 10 + data_len) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for StreamData = 5
    encode_varint(&ptr, 5);
    
    // Struct with two fields
    *ptr++ = stream_id;
    encode_bytes(&ptr, data, data_len);
    
    return ptr - buf;
}

// Encode StreamClose message
int avi_encode_stream_close(uint8_t *buf, size_t buf_len, uint8_t stream_id) {
    if (buf_len < 5) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for StreamClose = 6
    encode_varint(&ptr, 6);
    
    // Struct with one field
    *ptr++ = stream_id;
    
    return ptr - buf;
}

// Encode ButtonPress message
int avi_encode_button_press(uint8_t *buf, size_t buf_len, uint8_t button_id, avi_press_type_t press_type) {
    if (buf_len < 5) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for ButtonPress = 7
    encode_varint(&ptr, 7);
    
    // Struct with two fields
    *ptr++ = button_id;
    encode_varint(&ptr, (uint64_t)press_type);
    
    return ptr - buf;
}

// Encode SensorUpdate message
int avi_encode_sensor_update(uint8_t *buf, size_t buf_len, const char *sensor_name, avi_sensor_value_t *sensor_value) {
    if (buf_len < 20 + strlen(sensor_name)) return -1;
    uint8_t *ptr = buf;
    
    // Enum variant index for SensorUpdate = 8
    encode_varint(&ptr, 8);
    
    // Struct with two fields: sensor_name, data (SensorValue enum)
    encode_string(&ptr, sensor_name);
    
    // Encode SensorValue enum
    encode_varint(&ptr, (uint64_t)sensor_value->type);
    switch (sensor_value->type) {
        case AVI_SENSOR_TEMPERATURE:
        case AVI_SENSOR_HUMIDITY:
            memcpy(ptr, &sensor_value->value.temperature, sizeof(float));
            ptr += sizeof(float);
            break;
        case AVI_SENSOR_BATTERY:
            *ptr++ = sensor_value->value.battery;
            break;
        case AVI_SENSOR_STATUS:
            *ptr++ = sensor_value->value.status ? 1 : 0;
            break;
        case AVI_SENSOR_RAW:
            memcpy(ptr, &sensor_value->value.raw, sizeof(int32_t));
            ptr += sizeof(int32_t);
            break;
    }
    
    return ptr - buf;
}

// Decode downlink messages
int avi_decode_downlink(const uint8_t *buf, size_t buf_len, avi_downlink_type_t *type, void *msg_out) {
    const uint8_t *ptr = buf;
    const uint8_t *end = buf + buf_len;
    uint64_t variant;
    
    if (decode_varint(&ptr, end, &variant) != 0) return -1;
    *type = (avi_downlink_type_t)variant;
    
    switch (*type) {
        case AVI_DOWNLINK_WELCOME:
            // No fields
            break;
            
        case AVI_DOWNLINK_ERROR: {
            avi_downlink_error_t *msg = (avi_downlink_error_t*)msg_out;
            if (ptr >= end) return -1;
            msg->reason = *ptr++;
            break;
        }
        
        case AVI_DOWNLINK_MESSAGE: {
            avi_downlink_message_t *msg = (avi_downlink_message_t*)msg_out;
            if (decode_string(&ptr, end, msg->topic, AVI_MAX_TOPIC_LEN) != 0) return -1;
            if (decode_bytes(&ptr, end, msg->data, &msg->data_len, AVI_MAX_DATA_LEN) != 0) return -1;
            break;
        }
        
        case AVI_DOWNLINK_SUBSCRIBE_ACK: {
            avi_downlink_subscribe_ack_t *msg = (avi_downlink_subscribe_ack_t*)msg_out;
            if (decode_string(&ptr, end, msg->topic, AVI_MAX_TOPIC_LEN) != 0) return -1;
            break;
        }
        
        case AVI_DOWNLINK_UNSUBSCRIBE_ACK: {
            avi_downlink_unsubscribe_ack_t *msg = (avi_downlink_unsubscribe_ack_t*)msg_out;
            if (decode_string(&ptr, end, msg->topic, AVI_MAX_TOPIC_LEN) != 0) return -1;
            break;
        }
        
        default:
            return -1;
    }
    
    return 0;
}
