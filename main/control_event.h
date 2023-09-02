#pragma once

#include <inttypes.h>
#include "esp_event.h"
#include "freertos/semphr.h"

#include "sdkconfig.h"

#define CONTROL_EVENT_DATA_SZ 4
#define CONTROL_GESTURE_TIMEOUT_MS 1000

ESP_EVENT_DECLARE_BASE(CONTROL_EVENT_BASE);

enum {
    CONTROL_BUTTON_EVENT,
    CONTROL_CAN_BUTTON_EVENT
};

#define BUTTON_MAP_TO_GPIO_PULLUP(C, P) {\
    if(!gpio_get_level(P)) {\
        r |= C;\
    }\
}

#define BUTTON_MAP_TO_GPIO_PULLDOWN(C, P) {\
    if(!gpio_get_level(P)) {\
        r |= C;\
    }\
}

typedef enum {
    BRAKE_FRONT = (1 << 1),
    BRAKE_REAR = (1 << 2),
    CLUTCH = (1 << 3),
    SIGNAL_L = (1 << 4),
    SIGNAL_R = (1 << 5),
    SIGNAL_C = (1 << 6),
    FLASH = (1 << 7)
} ct_control_button_t;

typedef struct {
    ct_control_button_t button;
    uint8_t count : 7;
    bool long_press : 1;
} ct_control_event_t;

typedef uint64_t ct_control_button_register_t;

esp_err_t control_event_init(void);

extern esp_event_loop_handle_t control_event_loop_hdl;
extern ct_control_button_register_t control_button_state;
extern SemaphoreHandle_t control_button_state_sem;
