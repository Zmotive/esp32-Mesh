#ifndef CFG_HELPER_H
#define CFG_HELPER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
//https://github.com/platformio/platform-espressif32/issues/957
#define MBEDTLS_CONFIG_FILE "mbedtls/esp_config.h"
#include "mbedtls/md5.h"
#include "esp_ota_ops.h"


static const char *CFG_H_TAG = "cfg_helper";
//static const int ADC_ATTEN_DB_12_MV_MAX = 2450; // Max voltage for 12dB attenuation

/*
The RF Switch feature allows you to toggle between the built-in 
ceramic antenna and an external antenna by configuring GPIO14. 
To enable this function, you must first set GPIO3 to a low level,
 as this activates the RF switch control.

    * GPIO14 Low Level (Default Setting): The device uses the 
        built-in ceramic antenna.
    * GPIO14 High Level: The device switches to the external 
        antenna.

By default, GPIO14 is set to a low level, enabling the built-in
antenna. To use an external antenna, set GPIO14 to a high level.
*/
static inline void rf_switch_ext(void) {   
    gpio_set_level(GPIO_NUM_3, 0);  // Set GPIO 3 low
    vTaskDelay(pdMS_TO_TICKS(300)); // Delay for 300ms
    gpio_set_level(GPIO_NUM_14, 1); // Set GPIO 14 high
    gpio_set_level(GPIO_NUM_3, 1);  // Set GPIO 3 back high
    ESP_LOGI(CFG_H_TAG, "External Antenna Enabled");
}

/**
 * @brief Configure the specified pin as an analog input using the Oneshot ADC driver.
 * 
 * @param channel The ADC channel corresponding to the pin (ADC_CHANNEL_0 for A0, etc.).
 * @param handle Pointer to the ADC oneshot handle to be initialized.
 */
static inline void setup_analog_input(adc_channel_t channel, adc_oneshot_unit_handle_t *handle) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1, // Use ADC Unit 1
    };

    // Initialize the ADC oneshot driver
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, handle));

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12, // Set attenuation for full-scale voltage
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    // Configure the specified ADC channel
    ESP_ERROR_CHECK(adc_oneshot_config_channel(*handle, channel, &channel_config));
    ESP_LOGI(CFG_H_TAG, "Analog input configured on channel %d", channel);
}


/**
 * @brief Read the battery mV.
 * 
 * @param channel The ADC channel corresponding to the pin (ADC_CHANNEL_0 for A0, etc.).
 * @param handle Pointer to the ADC oneshot handle to be initialized.
 * 
 * See https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/peripherals/adc.html
 */
static inline int read_battery_mv(adc_channel_t channel, adc_oneshot_unit_handle_t *handle) {
    int adc_reading = 0;
    ESP_ERROR_CHECK(adc_oneshot_read(*handle, channel, &adc_reading));

    // Convert the ADC reading to a voltage (in mV)
    return adc_reading * 2;
}

// Device configuration structure for persistent storage in NVS
// this is for deltas in the HW between the 3 different node types.
typedef struct {
    uint32_t version; // Version of the configuration stored (used for init)
    uint8_t node_type; // node type identifier
    int8_t battery_analog_pin; // Analog pin for battery voltage reading
    uint8_t ext_antenna; // Flag to indicate if external antenna is used
    char fw_md5[33]; // 32 hex chars + null terminator for firmware signature
    // Add other config items as needed
} device_config_t;

// Save and load functions for device config
esp_err_t save_device_config(const device_config_t *config);
esp_err_t load_device_config(device_config_t *config);


void compute_running_firmware_md5(char *out_md5_hex);

#endif // CFG_HELPER_H