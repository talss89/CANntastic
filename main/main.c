#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_app_desc.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "sdkconfig.h"


#include "canbus.h"
#include "control_event.h"

#include "hid_codes.h"
#include "hid_func.h"

#include "ble_hid.h"

static const char* TAG = "main_task";

void run_on_event(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
{
    ct_control_event_t* event = (ct_control_event_t*) event_data;
    ESP_LOGI(TAG, "Button %d, pressed %d, %s", event->button, event->count, event->long_press ? "long press" : "normal press");

    switch(event->button) {
        case SIGNAL_C:
            
            if(event->long_press) {
                hid_cc_change_key(HID_CONSUMER_VOICE_ASSIST, true);
                hid_cc_change_key(HID_CONSUMER_VOICE_ASSIST, false);
                return;
            }

            switch(event->count) {
                case 2:
                    hid_cc_change_key(HID_CONSUMER_PLAY_PAUSE, true);
                    hid_cc_change_key(HID_CONSUMER_PLAY_PAUSE, false);
                break;
                case 3:
                    hid_cc_change_key(HID_CONSUMER_SCAN_NEXT_TRK, true);
                    hid_cc_change_key(HID_CONSUMER_SCAN_NEXT_TRK, false);
                break;
                case 4:
                    hid_cc_change_key(HID_CONSUMER_SCAN_PREV_TRK, true);
                    hid_cc_change_key(HID_CONSUMER_SCAN_PREV_TRK, false);
                break;
                default:
                break;
            }
            
            break;
        default:
        break;
    }

    
}

void test_task_1(void* arg) {

    ESP_ERROR_CHECK(esp_event_handler_register_with(control_event_loop_hdl, CONTROL_EVENT_BASE, CONTROL_BUTTON_EVENT, run_on_event, NULL));
    
    while(1) {
        // ESP_LOGI(TAG, "Test Task 1");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    vTaskDelete(NULL);
}

void app_main(void) {
    ESP_LOGI(TAG, "%s %s built on %s at %s (%s)", esp_app_get_description()->project_name, esp_app_get_description()->version, esp_app_get_description()->date, esp_app_get_description()->time, esp_app_get_description()->idf_ver);

    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret);

    ble_hid_init();
    canbus_init();
    control_event_init();

    xTaskCreate(test_task_1, "test_task_1", 3072, NULL, 10, NULL);

    while(1) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}