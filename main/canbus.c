#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "canbus.h"

#include "control_event.h"

#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c,"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16             PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32             PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)

static const char* TAG = "canbus_listen";

static const twai_filter_config_t f_config =  // TWAI_FILTER_CONFIG_ACCEPT_ALL();
                                            {.acceptance_code = (0x567 << 21),
                                             .acceptance_mask = ~(0x7FF << 21),
                                             .single_filter = true};
static const twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
static const twai_general_config_t g_config = {.mode = TWAI_MODE_LISTEN_ONLY,
                                              .tx_io = CONFIG_CT_CANBUS_TX_PIN, .rx_io = CONFIG_CT_CANBUS_RX_PIN,
                                              .clkout_io = TWAI_IO_UNUSED, .bus_off_io = TWAI_IO_UNUSED,
                                              .tx_queue_len = 0, .rx_queue_len = 5,
                                              .alerts_enabled = TWAI_ALERT_NONE,
                                              .clkout_divider = 0};


void canbus_listen_task(void* arg) {

    twai_message_t rx_msg;

    ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
    ESP_LOGI(TAG, "Driver installed");
    ESP_ERROR_CHECK(twai_start());
    ESP_LOGI(TAG, "Driver started");
    uint64_t prev = 0;

    while(1) {
        
        twai_receive(&rx_msg, pdMS_TO_TICKS(10));

        if(rx_msg.rtr) {
            continue;
        }

        bool state = false;
        ct_can_button_state_t event = {};

        switch(rx_msg.identifier) {
            case 0x567:
                //ESP_LOGI(TAG, "567 Rx");
                xSemaphoreTake(control_button_state_sem, portMAX_DELAY);
                event.button = SIGNAL_C;
                
                if(rx_msg.data[2] & (1<<3)) {
                    // ESP_LOGI(TAG, "567 Rx SIGNAL_C");
                    control_button_state |= SIGNAL_C;
                } else {
                    control_button_state &= !SIGNAL_C;
                }

                xSemaphoreGive(control_button_state_sem);
            break;
            
            default:
            break;
        }
        
        // uint64_t data = 0;
        // for (int i = 0; i < rx_msg.data_length_code; i++) {
        //     data |= (rx_msg.data[i] << (i * 8));
        // }

        // if(prev != data) {
        //     ESP_LOGI(TAG, "Received 0x%04lX Packet: " PRINTF_BINARY_PATTERN_INT64, rx_msg.identifier, PRINTF_BYTE_TO_BINARY_INT64(data));
        // }

        // prev = data;
        

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_ERROR_CHECK(twai_stop());
    ESP_LOGI(TAG, "Driver stopped");
    ESP_ERROR_CHECK(twai_driver_uninstall());
    ESP_LOGI(TAG, "Driver uninstalled");

    vTaskDelete(NULL);
}

esp_err_t canbus_init(void) {

    xTaskCreate(canbus_listen_task, "canbus_listen", 3072, NULL, uxTaskPriorityGet(NULL), NULL);

    return ESP_OK;
}