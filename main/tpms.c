#include "tpms.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "vehicles.h"
#include "driver/twai.h"
#include "web_config.h"

static const char* TAG = "tpms";
SemaphoreHandle_t TPMS_STATE_LOCK;
esp_event_loop_handle_t TPMS_EVENT_LOOP;
ct_tpms_state_t tpms_state = {};

ESP_EVENT_DEFINE_BASE(TPMS_EVENT_BASE);

void tpms_task(void* arg) {

    while(1) {
        ESP_LOGI(TAG, "Sending TPMS Data...");
        VEHICLE_TPMS_SEND(tpms_state);
        esp_event_loop_run(TPMS_EVENT_LOOP, pdMS_TO_TICKS(VEHICLE_TPMS_INTERVAL_MS));

        for(int i = 0; i < TPMS_WHEEL_COUNT; i ++) {
            if((esp_timer_get_time() - tpms_state.wheel[i].last_seen) > TPMS_TIMEOUT_US) {
                TPMS_ENTER_LOCK();
                tpms_state.wheel[i].no_signal = true;
                tpms_state.wheel[i].last_seen = esp_timer_get_time();
                TPMS_EXIT_LOCK();
            }
        }
    }

    vTaskDelete(NULL);
}

void tpms_consume_update(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data) {
    ct_ble_tpms_packet_t* tpms = (ct_ble_tpms_packet_t*) event_data;

    uint8_t wheel_index = 0xFF;

    ESP_LOGI(TAG, "TPMS Data: %s", addr_str(&tpms->addr));

    for(int i = 0; i < TPMS_WHEEL_COUNT; i ++) {
        if(memcmp(&tpms->addr, &WEB_CONFIG.tpms_addr[i], sizeof(ble_addr_t)) == 0) {
            wheel_index = i;
            break;
        }
    }

    if(wheel_index == 0xFF)
        return;

    ESP_LOGI(TAG, "Wheel: %d", wheel_index);

    TPMS_ENTER_LOCK();
    
    tpms_state.wheel[wheel_index].pressure = (uint8_t) (((((uint16_t) tpms->data[3] << 8) | tpms->data[4]) - 147) / 10);
    tpms_state.wheel[wheel_index].temperature = (uint8_t) tpms->data[2];
    tpms_state.wheel[wheel_index].battery = (uint8_t) tpms->data[1];
    tpms_state.wheel[wheel_index].last_seen = esp_timer_get_time();

    if(tpms_state.wheel[wheel_index].pressure < 32) { // TODO: Set values from web ui
        tpms_state.warning = true;
    }

    TPMS_EXIT_LOCK();

}

esp_err_t tpms_start(void) {
    #if defined (VEHICLE_TPMS_INTERVAL_MS) && defined (VEHICLE_TPMS_SEND)
        xTaskCreate(tpms_task, "tpms", 3072, NULL, 10, NULL);
    #endif
    return ESP_OK;
}

esp_err_t tpms_init(void) {
    TPMS_STATE_LOCK = xSemaphoreCreateBinary();
    
    esp_event_loop_args_t loop_args = {
        .queue_size = 32,
        .task_name = NULL,
        .task_priority = 0,
        .task_stack_size = 0,
        .task_core_id = tskNO_AFFINITY
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &TPMS_EVENT_LOOP));
    ESP_ERROR_CHECK(esp_event_handler_register_with(TPMS_EVENT_LOOP, TPMS_EVENT_BASE, TPMS_UPDATE_EVENT, tpms_consume_update, NULL));

    for(int i = 0; i < TPMS_WHEEL_COUNT; i ++) {
        tpms_state.wheel[i].pressure = 0;
        tpms_state.wheel[i].temperature = 0;
        tpms_state.wheel[i].last_seen = 0;
        tpms_state.wheel[i].battery = 0;
        tpms_state.wheel[i].no_signal = false;
    }

    tpms_state.warning = false;
    
    TPMS_EXIT_LOCK();
    
    return ESP_OK;
}