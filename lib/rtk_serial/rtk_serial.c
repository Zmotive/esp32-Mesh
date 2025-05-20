#include "rtk_serial.h"


#define UART_NUM UART_NUM_1
#define UART_BAUD_RATE 115200
#define UART_BUF_SIZE 1024

static const char *TAG = "rtk_serial";

UBXNavPVT g_nav_pvt;
UBXNavSVIN g_nav_svin;
SemaphoreHandle_t g_nav_data_mutex; // Mutex for protecting nav_pvt and nav_svin

// Python struct format strings for UBXNavPVT and UBXNavSVIN (compact form)
const char *UBXNavPVT_FORMAT = "<IH6BIi4B4i2I5i2I2HIihH";
const char *UBXNavSVIN_FORMAT = "<B3x2I3i3bx2I2B2x";


/**
 * @brief Process a UBX message from the provided data buffer.
 *
 * This function checks the UBX header and, if valid, processes supported UBX messages (NAV-SVIN and NAV-PVT).
 * It verifies the message type, checks the CRC (Fletcher-8), and only copies the payload into the appropriate global structure (nav_svin or nav_pvt) if the CRC is valid.
 * The function also manages a mutex to protect shared data, and removes the processed message from the buffer. If the CRC fails, the message is rejected and not copied.
 *
 * @param data    Pointer to the buffer containing the UBX message. The buffer may contain additional data after the message.
 * @param length  Pointer to the length of the data buffer. This will be updated to reflect the remaining unprocessed data.
 * @return true if a supported UBX message was processed and copied (CRC must pass); false otherwise.
 */
bool process_ubx_message(uint8_t *data, uint16_t *length) {
    UBXHeader *header = (UBXHeader *)data;
    if (header->preamble1 == 0xB5 && header->preamble2 == 0x62 && header->msg_class == 0x01){
        // Calculate UBX message length: header (6) + payload + CRC (2)
        int payload_len = header->length;
        int ubx_msg_len = sizeof(UBXHeader) + payload_len + 2;
        if (*length < ubx_msg_len) {
            ESP_LOGE(TAG, "UBX message incomplete: got %d, expected %d", *length, ubx_msg_len);
            return false;
        }
        // Calculate CRC (Fletcher-8)
        uint8_t ck_a = 0, ck_b = 0;
        for (int i = 2; i < 6 + payload_len; i++) { // class, id, length, payload
            ck_a += data[i];
            ck_b += ck_a;
        }
        uint8_t msg_ck_a = data[6 + payload_len];
        uint8_t msg_ck_b = data[6 + payload_len + 1];
        if (ck_a != msg_ck_a || ck_b != msg_ck_b) {
            ESP_DRAM_LOGW(TAG, "UBX CRC mismatch: calc=0x%02X%02X, msg=0x%02X%02X", ck_a, ck_b, msg_ck_a, msg_ck_b);
            return false;
        }
        if (header->msg_id == 0x3B) {
            ESP_LOGI(TAG, "NAV-SVIN message received");
            // Lock the mutex before modifying nav_svin
            if (xSemaphoreTake(g_nav_data_mutex, portMAX_DELAY)) {
                memcpy(&g_nav_svin, data + sizeof(UBXHeader), sizeof(UBXNavSVIN));
                PROTOCOL_LOG_STRUCT_BASE64(ESP_LOG_INFO, DATATAG, &g_nav_svin, UBXNavSVIN, UBXNavSVIN_FORMAT);
                xSemaphoreGive(g_nav_data_mutex); // Unlock the mutex
            } else {
                ESP_LOGE(TAG, "Failed to lock mutex for nav_svin");
            }
            // Remove the UBX header and NAV-SVIN from the data buffer if it has additional data.
            int copied_bytes = sizeof(UBXHeader) + sizeof(UBXNavSVIN) + 2; // 2 bytes for CRC
            if (*length > copied_bytes) {
                ESP_LOGI(TAG, "NAV-SVIN message has additional data, length: %d", header->length);
                memcpy(data, data + copied_bytes, *length - copied_bytes);
            }
            *length -= copied_bytes;
            return true;
        } else if (header->msg_id == 0x07) {
            ESP_LOGI(TAG, "NAV-PVT message received");
            // Lock the mutex before modifying nav_pvt
            if (xSemaphoreTake(g_nav_data_mutex, portMAX_DELAY)) {
                memcpy(&g_nav_pvt, data + sizeof(UBXHeader), sizeof(UBXNavPVT));
                PROTOCOL_LOG_STRUCT_BASE64(ESP_LOG_INFO, DATATAG, &g_nav_pvt, UBXNavPVT, UBXNavPVT_FORMAT);
                xSemaphoreGive(g_nav_data_mutex); // Unlock the mutex
            } else {
                ESP_LOGE(TAG, "Failed to lock mutex for nav_pvt");
            }
            // Update global GPS PTP time structure (in microseconds)
            // Valid bit 1 is valid Date, 2 is valid Time, 4 is fully resolved:
            // 0x07 = 1 + 2 + 4. See https://content.u-blox.com/sites/default/files/ZED-F9P_IntegrationManual_UBX-18010802.pdf pg 61 for details.
            if ( ((g_nav_pvt.valid & 0x07) == 0x07) && xSemaphoreTake(g_gps_ptp_time_mutex, portMAX_DELAY)) {
                g_gps_ptp_time.gps_iTOW_us = g_nav_pvt.iTOW * 1000ULL + (int64_t)((g_nav_pvt.nano + 500) / 1000); // Combine iTOW and nano, both in us
                g_gps_ptp_time.updated_us = (uint64_t)esp_timer_get_time(); // Already in us
                g_gps_ptp_time.tAcc_us = (g_nav_pvt.tAcc + 500) / 1000; // Convert ns to us, rounding
                xSemaphoreGive(g_gps_ptp_time_mutex);
                ESP_LOGI(TAG, "NAV-PVT iTOW: %llu, tAcc: %lu, updated_us: %llu", g_gps_ptp_time.gps_iTOW_us, g_gps_ptp_time.tAcc_us, g_gps_ptp_time.updated_us);
            }
            else {
                ESP_LOGW(TAG, "NAV-PVT time not valid: 0x%02X", g_nav_pvt.valid);
            }
            // Remove the UBX header and NAV-PVT from the data buffer if it has additional data.
            int copied_bytes = sizeof(UBXHeader) + sizeof(UBXNavPVT) + 2; // 2 bytes for CRC
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
    g_nav_data_mutex = xSemaphoreCreateMutex();
    if (g_nav_data_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex for nav data");
    }
    g_gps_ptp_time_mutex = xSemaphoreCreateMutex();
    if (g_gps_ptp_time_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex for time data");
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