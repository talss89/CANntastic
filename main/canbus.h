#pragma once

#include <stdio.h>
#include <stdlib.h>
#include "esp_err.h"
#include "esp_log.h"
#include "driver/twai.h"
#include "sdkconfig.h"
#include "vehicles.h"

#include "control_event.h"


typedef struct {
    ct_control_button_t button;
    bool state;
} ct_can_button_state_t;

#define CANBUS_BUTTON_PRESSED(MSG, BYTE, BIT) MSG.data[BYTE-1] & (1 << (BIT - 1))
#define CANBUS_MAP_TO_BUTTON_REGISTER(MSG, BUTTON, BYTE, BIT) {\
    if(CANBUS_BUTTON_PRESSED(MSG, BYTE, BIT)) {\
        CONTROL_BUTTON_REGISTER |= (1 << BUTTON);\
    } else {\
        CONTROL_BUTTON_REGISTER &= ~(1 << BUTTON);\
    }\
}

#define CANBUS_PROCESS_BUTTON_PACKET(ID, MSG) {\
case ID:\
    xSemaphoreTake(CONTROL_BUTTON_REGISTER_LOCK, portMAX_DELAY);\
    VEHICLE_READ_CANBUS_BUTTONS_##ID(MSG)\
    xSemaphoreGive(CONTROL_BUTTON_REGISTER_LOCK);\
break;\
}

esp_err_t canbus_init(void);