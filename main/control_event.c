#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "driver/gpio.h"

#include "vehicles.h"
#include "control_event.h"
#include "control_scheme.h"
#include "custom/gpio.h"

static const char* TAG = "control_event";

esp_event_loop_handle_t CONTROL_EVENT_LOOP;

ct_control_button_register_t CONTROL_BUTTON_REGISTER;
SemaphoreHandle_t CONTROL_BUTTON_REGISTER_LOCK;

ESP_EVENT_DEFINE_BASE(CONTROL_EVENT_BASE);

void control_event_task(void* arg) {

    uint64_t last_changed = 0;
    ct_control_event_t event = {};

    ct_control_button_register_t previous_state = 0;
    
    CONTROL_BUTTON_REGISTER = 0;

    CONTROL_BUTTON_REGISTER_LOCK = xSemaphoreCreateBinary();
    assert(CONTROL_BUTTON_REGISTER_LOCK != NULL);

    xSemaphoreGive(CONTROL_BUTTON_REGISTER_LOCK);

    while(1) {
        vTaskDelay(50 / portTICK_PERIOD_MS);

        xSemaphoreTake(CONTROL_BUTTON_REGISTER_LOCK, portMAX_DELAY);
        
        #ifdef VEHICLE_READ_GPIO_BUTTONS
            VEHICLE_READ_GPIO_BUTTONS()
        #endif

        #ifdef CUSTOM_READ_GPIO_BUTTONS
            CUSTOM_READ_GPIO_BUTTONS()
        #endif

        for(int i = sizeof(ct_control_button_register_t) * 8; i >= 0; i--) {

            if(!((SCHEME_GESTURE_MASK[SCHEME_PAGE_IDX] >> i) & 1)) {
                // This button should only emit up / down events (gesture mask bit is 0).

                if(((previous_state ^ CONTROL_BUTTON_REGISTER) >> i) & 1) {

                    if(esp_timer_get_time() - last_changed < CONTROL_GESTURE_DEBOUNCE_uS) {
                        // Debounce
                        continue;
                    }

                    last_changed = esp_timer_get_time();

                    event.button = i; 

                    event.plain.state = (CONTROL_BUTTON_REGISTER >> i) & 1;
                    event.page = SCHEME_PAGE_IDX;
                    ESP_ERROR_CHECK(esp_event_post_to(CONTROL_EVENT_LOOP, CONTROL_EVENT_BASE, CONTROL_BUTTON_EVENT, &event, sizeof(event), portMAX_DELAY));                    
                    
                    memset(&event, 0, sizeof(event));
                }

            } else {
                // This button should emit gestures.
                if(esp_timer_get_time() - last_changed > CONTROL_GESTURE_WINDOW_MS * 1000ULL) {
                
                    if(event.gesture.count > 0) {
                        // Post the message.
                        event.page = SCHEME_PAGE_IDX;
                        ESP_ERROR_CHECK(esp_event_post_to(CONTROL_EVENT_LOOP, CONTROL_EVENT_BASE, CONTROL_BUTTON_GESTURE_EVENT, &event, sizeof(event), portMAX_DELAY));                    
                    }

                    event.button = NONE;
                    event.gesture.count = 0;
                    event.gesture.long_press = false;
                    last_changed = esp_timer_get_time();
                }

                if(((previous_state ^ CONTROL_BUTTON_REGISTER) >> i) & 1) {
                    // Changed

                    if(esp_timer_get_time() - last_changed < CONTROL_GESTURE_DEBOUNCE_uS) {
                        // Debounce
                        continue;
                    }
                    
                    if(event.button != i) {
                        // This is a new button press.
                        event.gesture.count = 0;
                        event.gesture.long_press = false;
                    }
    
                    if((CONTROL_BUTTON_REGISTER >> i) & 1) {
                        // High
                        ESP_LOGI(TAG, "Control %d is now high", i);
                        event.gesture.count++;
                        event.gesture.long_press = true; // Set high count bit in anticipation of a long press
                    } else {
                        // Low
                        ESP_LOGI(TAG, "Control %d is now low", i);
                        event.gesture.long_press = false;
                    }

                    last_changed = esp_timer_get_time();

                    event.button = i; 
                }
            }            
        }
        previous_state = CONTROL_BUTTON_REGISTER;
        xSemaphoreGive(CONTROL_BUTTON_REGISTER_LOCK);
    }

    xSemaphoreTake(CONTROL_BUTTON_REGISTER_LOCK, portMAX_DELAY);
    vSemaphoreDelete(CONTROL_BUTTON_REGISTER_LOCK);

    vTaskDelete(NULL);
}

/**
 * Initialise the control event subsystem
*/

esp_err_t control_event_init(void) {

    #ifdef CUSTOM_GPIO_SETUP
        CUSTOM_GPIO_SETUP()
    #endif

    #ifdef VEHICLE_GPIO_SETUP
        VEHICLE_GPIO_SETUP()
    #endif

    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = "control_evt",
        .task_priority = 10,
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &CONTROL_EVENT_LOOP));

    return ESP_OK;
}

esp_err_t control_event_start(void) {
    xTaskCreate(control_event_task, "control_detect", 3072, NULL, 10, NULL);
    return ESP_OK;
}