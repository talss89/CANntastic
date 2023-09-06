#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "vehicles.h"
#include "canbus.h"

#include "control_event.h"

static const char* TAG = "canbus_listen";

static twai_filter_config_t f_config =  TWAI_FILTER_CONFIG_ACCEPT_ALL();
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_general_config_t g_config = {.mode = TWAI_MODE_NORMAL,
                                              .tx_io = CONFIG_CT_CANBUS_TX_PIN, .rx_io = CONFIG_CT_CANBUS_RX_PIN,
                                              .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
                                              .tx_queue_len = 32, .rx_queue_len = 32,
                                              .alerts_enabled = TWAI_ALERT_NONE,
                                              .clkout_divider = 0};

void canbus_generate_filter(uint16_t min, uint16_t max, twai_filter_config_t* filter) {
    
    uint32_t mask[2] = {~0, ~0};

    for(uint16_t i = min; i < max; i ++) {
        mask[0] &= i;
        mask[1] &= ~i;
    }

    filter->acceptance_mask = ~((mask[0] | mask[1]) << 21);
    filter->acceptance_code = (min & (mask[0] | mask[1])) << 21;
}

void canbus_listen_task(void* arg) {

    twai_message_t rx_msg;

    VEHICLE_FILTER_CANBUS(f_config);

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_ERROR_CHECK(twai_start());

    while(1) {

        twai_receive(&rx_msg, pdMS_TO_TICKS(10));
        if(rx_msg.rtr) {
            continue;
        }

        switch(rx_msg.identifier) {
            VEHICLE_PROCESS_CANBUS(rx_msg)      
            default:
            break;
        }

        taskYIELD();
        // vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_ERROR_CHECK(twai_stop());
    ESP_ERROR_CHECK(twai_driver_uninstall());

    vTaskDelete(NULL);
}

esp_err_t canbus_init(void) {

    xTaskCreate(canbus_listen_task, "canbus_listen", 3072, NULL, uxTaskPriorityGet(NULL), NULL);

    return ESP_OK;
}