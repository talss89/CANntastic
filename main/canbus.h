#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"

#include "control_event.h"

typedef struct {
    ct_control_button_t button;
    bool state;
} ct_can_button_state_t;


esp_err_t canbus_init(void);