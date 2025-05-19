#ifndef RTK_SERIAL_H
#define RTK_SERIAL_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "../gps_ptp_time/gps_ptp_time.h"
#include "esp_timer.h" // For esp_timer_get_time()

/*
U-center MSG configuration for base station:
0xB5 0x62 preable
NAV-SVIN (Survey In, if RTK is ready) 5s
    found here: https://content.u-blox.com/sites/default/files/products/documents/u-blox8-M8_ReceiverDescrProtSpec_UBX-13003221.pdf?utm_content=UBX-13003221
NAV-PVT (Position Velocity Time, Master time and fix information) 5s
    found here: https://content.u-blox.com/sites/default/files/LAP120_Interfacedescription_UBX-20046191.pdf

0xD3 Messages
RTCM3.3 1005 1s
RTCM3.3 1074 1s
RTCM3.3 1084 1s
RTCM3.3 1094 1s
RTCM3.3 1124 1s
RTCM3.3 1230 x 5s
*/
typedef struct UBXHeader {
    uint8_t preamble1;
    uint8_t preamble2;
    uint8_t msg_class;
    uint8_t msg_id;
    uint16_t length;
} UBXHeader;

typedef struct UBXNavPVT {
    uint32_t iTOW;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t valid;
    uint32_t tAcc;
    int32_t nano;
    uint8_t fixType;
    uint8_t flags;
    uint8_t flags2;
    uint8_t numSV;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint32_t hAcc;
    uint32_t vAcc;
    int32_t velN;
    int32_t velE;
    int32_t velD;
    int32_t gSpeed;
    int32_t headMot;
    uint32_t sAcc;
    uint32_t headAcc;
    uint16_t pDOP;
    uint8_t flags3;
    uint32_t reserved0;
    int32_t headVeh;
    int16_t magDec;
    uint16_t magAcc;
} UBXNavPVT;

typedef struct UBXNavSVIN {
    uint8_t version;
    uint32_t iTOW;
    uint32_t dur;
    int32_t meanX;
    int32_t meanY;
    int32_t meanZ;
    int8_t meanXHP;
    int8_t meanYHP;
    int8_t meanZHP;
    uint32_t meanAcc;
    uint32_t obs;
    uint8_t valid;
    uint8_t active;
} UBXNavSVIN;

extern UBXNavPVT g_nav_pvt;
extern UBXNavSVIN g_nav_svin;
extern SemaphoreHandle_t g_nav_data_mutex; // Mutex for protecting nav_pvt and nav_svin


/**
 * @brief Sets up the UART serial port with the specified configuration.
 */
void setup_serial_port(void);

/**
 * @brief Reads a data packet from the UART serial interface.
 * 
 * @param total_length Pointer to a variable where the total length of the received data will be stored.
 *                     This includes the header, payload, and CRC.
 * @return A pointer to the dynamically allocated buffer containing the received data, or NULL if an error occurs.
 *         The caller is responsible for freeing the allocated memory using `free`.
 */
uint8_t *read_serial_data(uint16_t *total_length);

#endif // RTK_SERIAL_H