#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

extern const char *DATATAG;

// Enumeration for protocol types (16-bit unsigned int)
typedef uint16_t ProtocolType;

enum {
    NETWORK_DATA = 0,
    RTK_DATA = 1,
    ROBOT_DATA = 2,
    PTP_DATA = 3,
    MASTER_CLOCK_DATA = 4,
    // Add more types as needed
};

// Structure for protocol header
typedef struct ProtocolHeader {
    ProtocolType type;  // 16-bit unsigned int as an enumeration
    uint16_t length;    // 16-bit unsigned int for length
} ProtocolHeader_t;

typedef struct NetworkData {
    uint32_t send_count;
    uint16_t battery_voltage;
} NetworkData_t;

typedef struct RTKData {
    uint8_t data[1000];  // Example size, adjust as needed
} RTKData_t;

typedef struct RobotData {
    uint8_t data[1000];  // Example size, adjust as needed
} RobotData_t;

typedef struct PTPData {
    uint8_t data[1000];  // Example size, adjust as needed
} PTPData_t;

typedef struct MasterClockData {
    uint8_t data[1000];  // Example size, adjust as needed
} MasterClockData_t;

#ifdef __cplusplus
extern "C" {
#endif

// --- Base64 encoding (header-only, static inline) ---
static const char protocol_base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static inline size_t protocol_base64_encode(const uint8_t *src, size_t len, char *out, size_t out_size) {
    size_t olen = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t octet_a = i < len ? src[i] : 0;
        uint32_t octet_b = (i + 1) < len ? src[i + 1] : 0;
        uint32_t octet_c = (i + 2) < len ? src[i + 2] : 0;
        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;
        if (olen + 4 > out_size) break;
        out[olen++] = protocol_base64_table[(triple >> 18) & 0x3F];
        out[olen++] = protocol_base64_table[(triple >> 12) & 0x3F];
        out[olen++] = (i + 1) < len ? protocol_base64_table[(triple >> 6) & 0x3F] : '=';
        out[olen++] = (i + 2) < len ? protocol_base64_table[triple & 0x3F] : '=';
    }
    if (olen < out_size) out[olen] = '\0';
    return olen;
}

// --- Logging macro for struct as base64 ---
#include "esp_log.h"

#define PROTOCOL_LOG_STRUCT_BASE64(level, tag, struct_ptr, struct_type, format_str) do { \
    const struct_type *plog_struct = (const struct_type *)(struct_ptr); \
    size_t plog_struct_size = sizeof(struct_type); \
    /* Calculate base64 output size: 4 * ((n+2)/3) + 1 for null */ \
    size_t plog_b64_size = 4 * ((plog_struct_size + 2) / 3) + 1; \
    char plog_b64[plog_b64_size]; \
    protocol_base64_encode((const uint8_t*)plog_struct, plog_struct_size, plog_b64, plog_b64_size); \
    ESP_LOG_LEVEL(level, tag, "NAME:%s FORMAT:%s LEN:%u BASE64:%s", #struct_type, format_str, (unsigned)plog_struct_size, plog_b64); \
} while(0)

#ifdef __cplusplus
}
#endif

#endif // PROTOCOL_H