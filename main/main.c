#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_app_desc.h"

#include "sdkconfig.h"

#include "cantastic.h"
#include "control_event.h"

static const char* TAG = "Main";

void app_main(void) {
    ESP_LOGI(TAG, "Cantastic Start v%s", esp_app_get_description()->version);
}