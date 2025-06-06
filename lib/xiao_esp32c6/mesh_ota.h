#ifndef MESH_OTA_H
#define MESH_OTA_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_mesh.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// OTA message types
typedef enum {
    MESH_OTA_CMD_START = 1,
    MESH_OTA_CMD_DATA = 2,
    MESH_OTA_CMD_END = 3,
    MESH_OTA_CMD_ACK = 4
} mesh_ota_cmd_t;

// OTA message structure
typedef struct {
    mesh_ota_cmd_t cmd;
    uint32_t offset;
    uint32_t size;
    uint8_t data[1024]; // Adjust as needed
} mesh_ota_packet_t;

// Root node: start OTA update
esp_err_t mesh_ota_send_firmware(const char *fw_path);

// Handle incoming OTA packets (dispatcher model)
void mesh_ota_handle_packet(const mesh_ota_packet_t *pkt);

#endif // MESH_OTA_H
