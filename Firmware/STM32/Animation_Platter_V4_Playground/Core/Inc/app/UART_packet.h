#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#define PACKET_SOF 0xAA

// Message types
typedef enum {
    MSG_SYSTEM_DATA = 0x01,
    MSG_TOUCH_EVENT = 0x02,
    MSG_HEARTBEAT   = 0x03,
    MSG_DEBUG_TEXT  = 0x04,
    MSG_TESTING_DATA = 0x10
} packet_type_t;

// System data payload example
typedef struct __attribute__((packed)) {
    uint32_t rpm;
    uint16_t voltage;
    int16_t temperature;
    uint8_t state;
} system_data_t;

// Heartbeat payload example
typedef struct __attribute__((packed)) {
    uint32_t uptime_ms;
} heartbeat_t;

// Generic packet max payload
#define PACKET_MAX_PAYLOAD 64

// Compute 8-bit XOR checksum
static inline uint8_t packet_crc(const uint8_t *data, uint16_t len) {
    uint8_t crc = 0;
    for(uint16_t i=0; i<len; i++) {
        crc ^= data[i];
    }
    return crc;
}

#endif // PACKET_H
