#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_CT_VEHICLE_TIGER1200_GEN3

#include <string.h>
#include "tpms.h"

enum ct_tiger1200_g3_wheel_enum {
    V_FRONT_WHEEL = 0x00,
    V_REAR_WHEEL = 0x10
};

enum ct_tiger1200_g3_pressure_state_enum {
    V_PRESSURE_STABLE = 0x0,
    V_PRESSURE_RISING = 0x20,
    V_PRESSURE_FALLING = 0x30
};

enum ct_tiger1200_g3_battery_diff_enum {
    V_BATTERY_OK = 0x0,
    V_BATTERY_NO_SIGNAL = 0x7,
    V_BATTERY_LOW = 0x8
};

enum ct_tiger1200_g3_fault_state_enum {
    V_FAULT_OK = 0x0,
    V_FAULT_NO_COMMS = 0x40,
    V_FAULT_LEAK = 0x80,
};

enum ct_tiger1200_g3_warning_light_enum {
    V_TPMS_WARNING_OFF = 0xA,
    V_TPMS_WARNING_ON = 0xC,
};

typedef struct __attribute__((packed)) {
    uint8_t wheel : 8;
    uint8_t pressure : 8;
    uint8_t temp : 8;
    uint8_t status : 8;
    uint8_t unused : 8;
    uint16_t counter : 16;
    uint8_t fault : 8;
} ct_tiger_1200_g3_tpms_packet_t;

void tiger1200_g3_tpms_send(ct_tpms_state_t* tpms_state);

#define VEHICLE_READ_CANBUS_BUTTONS_0x567(MSG) {\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, JOY_SEL, 1, 4)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, SIGNAL_L, 1, 6)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, FOG, 1, 8)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, JOY_L, 2, 2)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, JOY_R, 2, 4)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, HOME, 2, 8)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, JOY_U, 3, 2)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, SIGNAL_C, 3, 4)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, FLASH, 3, 6)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, JOY_D, 3, 8)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, SIGNAL_R, 4, 2)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, RIDER_SEAT_HEAT, 4, 4)\
    CANBUS_MAP_TO_BUTTON_REGISTER(MSG, MODE, 4, 6)\
}

#define VEHICLE_TPMS_INTERVAL_MS 500

#define VEHICLE_TPMS_SEND(TPMS_STATE) tiger1200_g3_tpms_send(&TPMS_STATE)

/*
#define VEHICLE_FILTER_CANBUS(FILTER) {\
FILTER.acceptance_code = (0x567 << 21);\
FILTER.acceptance_mask = ~(TWAI_STD_ID_MASK << 21);\
FILTER.single_filter = true;\
}*/

#define VEHICLE_FILTER_CANBUS(FILTER) {\
    canbus_generate_filter(0x500, 0x5FF, &FILTER);\
}

#define VEHICLE_PROCESS_CANBUS(MSG) {\
    CANBUS_PROCESS_BUTTON_PACKET(0x567, MSG)\
}

#endif