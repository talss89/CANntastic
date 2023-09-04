#include "esp_log.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "hid_codes.h"
#include "hid_func.h"

#include "control_event.h"
#include "control_scheme.h"

static const char* TAG = "control_scheme";

uint8_t SCHEME_PAGE_IDX = 0;
uint8_t SCHEME_PAGE_COUNT = SCHEME_MAX_PAGES;

ct_control_button_register_t SCHEME_GESTURE_MASK[SCHEME_MAX_PAGES];

esp_err_t control_scheme_init(void) {
    SCHEME_INIT_FN();
    ESP_ERROR_CHECK(esp_event_handler_register_with(CONTROL_EVENT_LOOP, CONTROL_EVENT_BASE, CONTROL_BUTTON_GESTURE_EVENT, SCHEME_ON_GESTURE_FN, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register_with(CONTROL_EVENT_LOOP, CONTROL_EVENT_BASE, CONTROL_BUTTON_EVENT, SCHEME_ON_PLAIN_FN, NULL));
    return ESP_OK;
}