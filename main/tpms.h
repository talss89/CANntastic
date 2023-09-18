#pragma once

#include <stdlib.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

esp_err_t tpms_start(void);
esp_err_t tpms_init(void);

#define TPMS_WHEEL_COUNT 2

typedef struct {
    uint8_t pressure;
    int8_t temperature;
} ct_tpms_wheel_t;

typedef struct {
    bool warning;
    ct_tpms_wheel_t wheel[TPMS_WHEEL_COUNT];
} ct_tpms_state_t;