#include "cfg_helper.h"

#define DEVICE_CONFIG_NAMESPACE "devcfg"

esp_err_t save_device_config(const device_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(DEVICE_CONFIG_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(nvs, "config", config, sizeof(device_config_t));
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

esp_err_t load_device_config(device_config_t *config) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(DEVICE_CONFIG_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;
    size_t size = sizeof(device_config_t);
    err = nvs_get_blob(nvs, "config", config, &size);
    nvs_close(nvs);
    return err;
}

void compute_running_firmware_md5(char *out_md5_hex) {
    const esp_partition_t *part = esp_ota_get_running_partition();
    if (!part) {
        strcpy(out_md5_hex, "ERROR");
        return;
    }
    mbedtls_md5_context ctx;
    mbedtls_md5_init(&ctx);
    mbedtls_md5_starts(&ctx);
    uint8_t buf[1024];
    size_t offset = 0;
    while (offset < part->size) {
        size_t to_read = sizeof(buf);
        if (part->size - offset < to_read) to_read = part->size - offset;
        if (esp_partition_read(part, offset, buf, to_read) != ESP_OK) {
            strcpy(out_md5_hex, "ERROR");
            mbedtls_md5_free(&ctx);
            return;
        }
        mbedtls_md5_update(&ctx, buf, to_read);
        offset += to_read;
    }
    uint8_t md5[16];
    mbedtls_md5_finish(&ctx, md5);
    mbedtls_md5_free(&ctx);
    for (int i = 0; i < 16; ++i) sprintf(out_md5_hex + i*2, "%02x", md5[i]);
    out_md5_hex[32] = '\0';
}
