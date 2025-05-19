#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

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

#endif // PROTOCOL_H