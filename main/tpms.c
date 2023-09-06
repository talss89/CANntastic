#include "tpms.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "vehicles.h"
#include "driver/twai.h"

static const char* TAG = "tpms";

void tpms_task(void* arg) {

    ct_tpms_state_t tpms_state = {};

    while(1) {
        ESP_LOGI(TAG, "Sending TPMS Data...");
        VEHICLE_TPMS_SEND(tpms_state);
        vTaskDelay(pdMS_TO_TICKS(VEHICLE_TPMS_INTERVAL_MS));
    }

    vTaskDelete(NULL);
}

esp_err_t tpms_start(void) {
    #if defined (VEHICLE_TPMS_INTERVAL_MS) && defined (VEHICLE_TPMS_SEND)
        xTaskCreate(tpms_task, "tpms", 3072, NULL, 10, NULL);
    #endif
    return ESP_OK;
}

esp_err_t tpms_init(void) {
    
    return ESP_OK;
}