#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "sdkconfig.h"
#include "vehicles.h"
#include "custom/gpio.h"

#include "canbus.h"
#include "control_event.h"
#include "control_scheme.h"

#include "hid_codes.h"
#include "hid_func.h"

#include "ble_hid.h"

static const char* TAG = "main_task";

void app_main(void) {
    uint8_t mac[6];

    ESP_LOGI(TAG, "%s %s built on %s at %s (%s)", esp_app_get_description()->project_name, esp_app_get_description()->version, esp_app_get_description()->date, esp_app_get_description()->time, esp_app_get_description()->idf_ver);

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_efuse_mac_get_default(mac));

    #ifndef CONFIG_CT_DMD2_EVAL
    snprintf(CT_DEVICE_NAME, sizeof(CT_DEVICE_NAME), "CT-%02x%02x%02x", mac[5], mac[4], mac[3]);
    #endif

    ble_hid_init();
    canbus_init();
    control_event_init();
    control_scheme_init();
    control_event_start();

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}