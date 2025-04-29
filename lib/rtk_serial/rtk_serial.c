#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/semphr.h" // Include FreeRTOS semaphore header

#define UART_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 1024

static const char *TAG = "RTK_SERIAL";

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

UBXNavPVT nav_pvt;
UBXNavSVIN nav_svin;
SemaphoreHandle_t nav_data_mutex; // Mutex for protecting nav_pvt and nav_svin

bool process_ubx_message(uint8_t *data, uint16_t *length) {
    UBXHeader *header = (UBXHeader *)data;
    if (header->preamble1 == 0xB5 && header->preamble2 == 0x62 && header->msg_class == 0x01){
        if (header->msg_id == 0x3B) {
            ESP_LOGI(TAG, "NAV-SVIN message received");
            // Lock the mutex before modifying nav_svin
            if (xSemaphoreTake(nav_data_mutex, portMAX_DELAY)) {
                memcpy(&nav_svin, data + sizeof(UBXHeader), sizeof(UBXNavSVIN));
                xSemaphoreGive(nav_data_mutex); // Unlock the mutex
            } else {
                ESP_LOGE(TAG, "Failed to lock mutex for nav_svin");
            }
            // Remove the UBX header and NAV-SVIN from the data buffer if it has additional data.
            int copied_bytes = sizeof(UBXHeader) + sizeof(UBXNavSVIN) +2; // 2 bytes for CRC
            if (*length > copied_bytes) {
                ESP_LOGI(TAG, "NAV-SVIN message has additional data, length: %d", header->length);
                memcpy(data, data + copied_bytes, *length - copied_bytes);
            }
            *length -= copied_bytes;
            return true;
        } else if (header->msg_id == 0x07) {
            ESP_LOGI(TAG, "NAV-PVT message received");
            // Lock the mutex before modifying nav_pvt
            if (xSemaphoreTake(nav_data_mutex, portMAX_DELAY)) {
                memcpy(&nav_pvt, data + sizeof(UBXHeader), sizeof(UBXNavPVT));
                xSemaphoreGive(nav_data_mutex); // Unlock the mutex
                
            } else {
                ESP_LOGE(TAG, "Failed to lock mutex for nav_pvt");
            }
            // Remove the UBX header and NAV-PVT from the data buffer if it has additional data.
            int copied_bytes = sizeof(UBXHeader) + sizeof(UBXNavPVT) +2; // 2 bytes for CRC
            if (*length > copied_bytes) {
                ESP_LOGI(TAG, "NAV-PVT message has additional data, length: %d", header->length);
                memcpy(data, data + copied_bytes, *length - copied_bytes);
            }
            *length -= copied_bytes;
            return true;
        }
    }
    ESP_LOGE(TAG, "Invalid UBX header: p1:0x%02X p2:0x%02X c:0x%02X id:0x%02X l:%d", 
        header->preamble1, header->preamble2, header->msg_class, header->msg_id, header->length);
    return false;
}

void setup_serial_port() {
    // Configure UART
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, 16, 17, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, UART_BUF_SIZE, 0, 0, NULL, 0);

    ESP_LOGI(TAG, "UART configured and driver installed");

    // Initialize the mutex
    nav_data_mutex = xSemaphoreCreateMutex();
    if (nav_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex for nav data");
    }
}

uint8_t *read_serial_data(uint16_t *total_length) {
    uint16_t data_length;
    uint8_t *data_buffer = NULL;
    uint8_t *rtcm_buffer = NULL;
    uint16_t rtcm_length = 0;
    uint16_t bytes_read = 0;
    int length = 0;

    ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));

    
    if (length <= 0) {
        // ESP_LOGI(TAG, "No data available in serial port.");
        return NULL;
    }

    
    // Allocate buffer for the UART data
    *total_length = length;
    data_buffer = (uint8_t *)malloc(length);
    rtcm_buffer = (uint8_t *)malloc(length);
    if (!data_buffer || !rtcm_buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for buffers");
        return NULL;
    }

    // Read the data from UART
    bytes_read = uart_read_bytes(UART_NUM, data_buffer, length, pdMS_TO_TICKS(100));

    int msg_count = 0;
    while(bytes_read > 0) {
        switch (data_buffer[0])  // Check the first byte of the data
        {
        case 0xD3:
            // Extract data length from byte 2 and byte 3
            data_length = ((data_buffer[1] & 0x03) << 8) | data_buffer[2];

            if (bytes_read < data_length + 6) {
                ESP_LOGE(TAG, "Failed to read data bytes: got %d, expected %d", bytes_read, data_length + 3);
                free(data_buffer);
                return NULL;
            }
            memcpy(rtcm_buffer + rtcm_length, data_buffer, data_length + 6);
            rtcm_length += data_length + 6;
            
            memcpy(data_buffer, data_buffer + data_length + 6, bytes_read - (data_length + 6));
            bytes_read -= data_length + 6;

            ESP_LOGI(TAG, "Successfully read %d bytes", data_length + 6);
            if (bytes_read > 0) {
                ESP_LOGI(TAG, "Remaining bytes: %d", bytes_read);
            }

            continue;

        case 0xB5:  
            if(!process_ubx_message(data_buffer, &bytes_read)) {
                free(data_buffer);
                return NULL;
            }
            continue;

        default:
            ESP_LOGE(TAG, "Unknown header byte: 0x%02X. Length: %d", data_buffer[0], bytes_read);
            free(data_buffer);
            return NULL;
            break;
        }  
        if (msg_count > 4) {
            ESP_LOGI(TAG, "Ending message reading due to excessive messages");
            free(rtcm_buffer);
            free(data_buffer);
            return NULL;
        }
        msg_count++;
    }
    free(data_buffer);
    if (rtcm_length > 0) {
        ESP_LOGI(TAG, "RTCM data length: %d", rtcm_length);
        return rtcm_buffer;
    }
    free(rtcm_buffer);
    return NULL;
}