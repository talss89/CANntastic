#pragma once

#include <inttypes.h>

#include "sdkconfig.h"

#define CONTROL_EVENT_DATA_SZ 4

typedef enum {
    BRAKE_FRONT = (1 << 0),
    BRAKE_REAR = (1 << 1),
    CLUTCH = (1 << 2),
    SIGNAL_L = (1 << 3),
    SIGNAL_R = (1 << 4),
    SIGNAL_C = (1 << 5),
    FLASH = (1 << 6)
} ct_control_t;

typedef enum {
    BTN_DOWN,
    BTN_UP
} ct_action_t;

typedef struct {
    ct_control_t control;
    ct_action_t action;
    unsigned char data[CONTROL_EVENT_DATA_SZ];
} ct_control_event_t;