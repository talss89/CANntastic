#pragma once

#include <inttypes.h>
#include "esp_event.h"
#include "freertos/semphr.h"

#include "sdkconfig.h"
#include "controls.h"

#define CONTROL_GESTURE_WINDOW_MS 900
#define CONTROL_GESTURE_DEBOUNCE_uS 10000ULL
#define CONTROL_BUTTON_REGISTER control_button_state
#define CONTROL_BUTTON_REGISTER_LOCK control_button_state_sem
#define CONTROL_BUTTON_REGISTER_GESTURE_MASK control_button_gesture_mask
#define CONTROL_EVENT_LOOP control_event_loop_hdl

ESP_EVENT_DECLARE_BASE(CONTROL_EVENT_BASE);

enum {
    CONTROL_BUTTON_EVENT,
    CONTROL_BUTTON_GESTURE_EVENT,
    CONTROL_BUTTON_SETUP_EVENT
};

#define GPIO_MAP_TO_BUTTON_REGISTER(C, P) {\
    if(gpio_get_level(P)) {\
        CONTROL_BUTTON_REGISTER |= (1 << C);\
    } else {\
        CONTROL_BUTTON_REGISTER &= ~(1 << C);\
    }\
}

#define GPIO_MAP_TO_BUTTON_REGISTER_INVERT(C, P) {\
    if(!gpio_get_level(P)) {\
        CONTROL_BUTTON_REGISTER |= (1 << C);\
    } else {\
        CONTROL_BUTTON_REGISTER &= ~(1 << C);\
    }\
}



typedef struct {
    uint8_t count : 7;
    bool long_press : 1;
} ct_control_event_gesture_t;

typedef struct {
    enum {
        DOWN = 0, UP = 1
    } state;
} ct_control_event_plain_t;

typedef struct {
    ct_control_button_t button;
    uint8_t page;
    union {
        ct_control_event_gesture_t gesture;
        ct_control_event_plain_t plain;
    };
} ct_control_event_t;

typedef uint64_t ct_control_button_register_t;

esp_err_t control_event_init(void);
esp_err_t control_event_start(void);

extern esp_event_loop_handle_t CONTROL_EVENT_LOOP;
extern ct_control_button_register_t CONTROL_BUTTON_REGISTER;
extern ct_control_button_register_t CONTROL_BUTTON_REGISTER_GESTURE_MASK;
extern SemaphoreHandle_t CONTROL_BUTTON_REGISTER_LOCK;
