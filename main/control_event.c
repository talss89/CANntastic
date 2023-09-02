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

#include "control_event.h"

#define GPIO_INPUT_PINS ((1ULL << 2) | (1ULL << 15))

static const char* TAG = "control_event";

esp_event_loop_handle_t control_event_loop_hdl;
ct_control_button_register_t control_button_state;
SemaphoreHandle_t control_button_state_sem;

ESP_EVENT_DEFINE_BASE(CONTROL_EVENT_BASE);

/**
 * Return a contiguous index for a control button bit flag
*/

uint8_t control_button_compact(ct_control_button_t control_flag) {
    ct_control_button_t r;
    uint8_t bit_pos = 0;

    r = control_flag;

    while(r > 0) {
        r = r >> 1;
        bit_pos ++;
    }

    return bit_pos;
}

ct_control_button_register_t control_process_gpio() {

    ct_control_button_register_t r = 0;

    BUTTON_MAP_TO_GPIO_PULLUP(SIGNAL_C, 2);
    BUTTON_MAP_TO_GPIO_PULLUP(SIGNAL_L, 15);

    return r;
}

void control_event_task(void* arg) {

    uint64_t last_changed = 0;
    ct_control_event_t event = {};

    ct_control_button_register_t previous_state = 0;
    
    control_button_state = 0;

    control_button_state_sem = xSemaphoreCreateBinary();
    assert(control_button_state_sem != NULL);

    xSemaphoreGive(control_button_state_sem);

    while(1) {
        vTaskDelay(50 / portTICK_PERIOD_MS);

        xSemaphoreTake(control_button_state_sem, portMAX_DELAY);
        // control_button_state |= control_process_gpio();

        for(int i = sizeof(ct_control_button_register_t); i >= 0; i--) {
            if(esp_timer_get_time() - last_changed > 1000000ULL) {
               
                if(event.count > 0) {
                     // Post the message.
                    ESP_ERROR_CHECK(esp_event_post_to(control_event_loop_hdl, CONTROL_EVENT_BASE, CONTROL_BUTTON_EVENT, &event, sizeof(event), portMAX_DELAY));                    
                }

                event.button = 0;
                event.count = 0;
                event.long_press = false;
                last_changed = esp_timer_get_time();
            }

            if(((previous_state ^ control_button_state) >> i) & 1) {
                // Changed

                if(esp_timer_get_time() - last_changed < 10000ULL) {
                    // Debounce
                    continue;
                }
                
                if(event.button != (1 << i)) {
                    // This is a new button press.
                    event.count = 0;
                    event.long_press = false;
                }
 
                if((control_button_state >> i) & 1) {
                    // High
                    ESP_LOGI(TAG, "Control %d is now high", i);
                    event.count++;
                    event.long_press = true; // Set high count bit in anticipation of a long press
                } else {
                    // Low
                    ESP_LOGI(TAG, "Control %d is now low", i);
                    event.long_press = false;
                }

                last_changed = esp_timer_get_time();
                event.button = (1 << i);
            }
            
        }
        previous_state = control_button_state;
        xSemaphoreGive(control_button_state_sem);
    }

    xSemaphoreTake(control_button_state_sem, portMAX_DELAY);
    vSemaphoreDelete(control_button_state_sem);

    vTaskDelete(NULL);
}

/**
 * Initialise the control event subsystem
*/

esp_err_t control_event_init(void) {

    gpio_config_t io_conf = {};

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_INPUT_PINS;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = "control_evt",
        .task_priority = 10,
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &control_event_loop_hdl));

    xTaskCreate(control_event_task, "control_detect", 3072, NULL, 10, NULL);

    return ESP_OK;
}