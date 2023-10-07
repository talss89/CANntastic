#pragma once

#include <stdlib.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "host/ble_hs.h"
#include "esp_central.h"

ESP_EVENT_DECLARE_BASE(TPMS_EVENT_BASE);

esp_err_t tpms_start(void);
esp_err_t tpms_init(void);

#define TPMS_WHEEL_COUNT 2
#define TPMS_STATE_LOCK tpms_state_lock
#define TPMS_EVENT_LOOP tpms_event_loop_hdl
#define TPMS_TIMEOUT_US 1000LL * 1000 * 60 * 5 // 5 Minutes


enum {
    TPMS_UPDATE_EVENT
};

#define TPMS_ENTER_LOCK() {\
    xSemaphoreTake(TPMS_STATE_LOCK, portMAX_DELAY);\
}

#define TPMS_EXIT_LOCK() {\
    xSemaphoreGive(TPMS_STATE_LOCK);\
}

typedef struct {
    uint8_t pressure;
    int8_t temperature;
    uint8_t battery;
    int64_t last_seen;
    bool no_signal;
} ct_tpms_wheel_t;

typedef struct {
    bool warning;
    ct_tpms_wheel_t wheel[TPMS_WHEEL_COUNT];
} ct_tpms_state_t;

typedef struct {
    uint8_t data[7];
    ble_addr_t addr;
} ct_ble_tpms_packet_t;


extern SemaphoreHandle_t TPMS_STATE_LOCK;
extern esp_event_loop_handle_t TPMS_EVENT_LOOP;