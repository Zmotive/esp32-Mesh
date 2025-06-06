#include "esp_log.h"
#include "esp_mesh.h"
#include "protocol.h"
#include "cfg_helper.h"

const char *DATATAG = "DATA_TAG";

void handle_echo_packet(const mesh_addr_t *from, const void *payload, size_t payload_len, int mesh_layer, int is_root, int send_count, uint8_t *tx_buf) {
    if (payload_len < sizeof(int)) {
        ESP_LOGW("ECHO", "Echo payload too small");
        return;
    }
    int rx_send_count = 0;
    memcpy(&rx_send_count, payload, sizeof(int));
    if (is_root) {
        ESP_LOGI("ECHO", "[ECHO RX] from: "MACSTR", my_send_count: %d, rx_send_count: %d", MAC2STR(from->addr), send_count, rx_send_count);
    } else {
        // Non-root: attach own send_count and send back to root
        ProtocolHeader_t *header = (ProtocolHeader_t *)tx_buf;
        header->type = ECHO_DATA;
        header->length = sizeof(int);
        memcpy(tx_buf + sizeof(ProtocolHeader_t), &send_count, sizeof(int));
        mesh_data_t data;
        data.data = tx_buf;
        data.size = sizeof(ProtocolHeader_t) + sizeof(int);
        data.proto = MESH_PROTO_BIN;
        data.tos = MESH_TOS_P2P;
        esp_err_t err = esp_mesh_send(from, &data, MESH_DATA_P2P, NULL, 0);
        if (err) {
            ESP_LOGE("ECHO", "Failed to echo back to root: 0x%x", err);
        }
    }
}

void handle_fw_query_packet(const mesh_addr_t *from, uint8_t *tx_buf, device_config_t *config) {
    // Prepare FW_REPORT packet with current MD5
    ProtocolHeader_t *header = (ProtocolHeader_t *)tx_buf;
    header->type = FW_REPORT;
    header->length = 32; // MD5 hex string length
    memcpy(tx_buf + sizeof(ProtocolHeader_t), config->fw_md5, 32);
    mesh_data_t data;
    data.data = tx_buf;
    data.size = sizeof(ProtocolHeader_t) + 32;
    data.proto = MESH_PROTO_BIN;
    data.tos = MESH_TOS_P2P;
    esp_err_t err = esp_mesh_send(from, &data, MESH_DATA_P2P, NULL, 0);
    if (err) {
        ESP_LOGE("FW_QUERY", "Failed to send FW_REPORT: 0x%x", err);
    }
}

void handle_fw_report_packet(const mesh_addr_t *from, const void *payload, size_t payload_len) {
    char md5[33] = {0};
    if (payload_len >= 32) {
        memcpy(md5, payload, 32);
        ESP_LOGI("FW_QUERY", "Node "MACSTR" reports firmware MD5: %s", MAC2STR(from->addr), md5);
    } else {
        ESP_LOGW("FW_QUERY", "FW_REPORT payload too small");
    }
}