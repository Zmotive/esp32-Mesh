#include "mesh_ota.h"

#define OTA_TAG "mesh_ota"

// Handle a received OTA packet (to be called by central RX dispatcher)
void mesh_ota_handle_packet(const mesh_ota_packet_t *pkt) {
    static esp_ota_handle_t ota_handle = 0;
    static const esp_partition_t *ota_partition = NULL;
    static bool ota_in_progress = false;
    static uint32_t expected_offset = 0;
    esp_err_t err;

    switch (pkt->cmd) {
        case MESH_OTA_CMD_START:
            ESP_LOGI(OTA_TAG, "OTA START received, size=%lu", pkt->size);
            ota_partition = esp_ota_get_next_update_partition(NULL);
            if (!ota_partition) {
                ESP_LOGE(OTA_TAG, "No OTA partition found!");
                break;
            }
            err = esp_ota_begin(ota_partition, pkt->size, &ota_handle);
            if (err != ESP_OK) {
                ESP_LOGE(OTA_TAG, "esp_ota_begin failed: 0x%x", err);
                break;
            }
            ota_in_progress = true;
            expected_offset = 0;
            break;
        case MESH_OTA_CMD_DATA:
            if (!ota_in_progress) break;
            if (pkt->offset != expected_offset) {
                ESP_LOGW(OTA_TAG, "Unexpected offset: got %lu, expected %lu", pkt->offset, expected_offset);
                break;
            }
            err = esp_ota_write(ota_handle, pkt->data, pkt->size);
            if (err != ESP_OK) {
                ESP_LOGE(OTA_TAG, "esp_ota_write failed: 0x%x", err);
                break;
            }
            expected_offset += pkt->size;
            break;
        case MESH_OTA_CMD_END:
            if (!ota_in_progress) break;
            err = esp_ota_end(ota_handle);
            if (err == ESP_OK) {
                ESP_LOGI(OTA_TAG, "OTA update complete, rebooting...");
                esp_ota_set_boot_partition(ota_partition);
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                esp_restart();
            } else {
                ESP_LOGE(OTA_TAG, "esp_ota_end failed: 0x%x", err);
            }
            ota_in_progress = false;
            break;
        default:
            ESP_LOGW(OTA_TAG, "Unknown OTA command: %d", pkt->cmd);
            break;
    }
}

// Root node: send firmware to all nodes (stub)
esp_err_t mesh_ota_send_firmware(const char *fw_path) {
    // TODO: Open firmware file, send in chunks using esp_mesh_send
    ESP_LOGI(OTA_TAG, "Send firmware from: %s (stub)", fw_path);
    return ESP_OK;
}
