#pragma once

#include <inttypes.h>
#include "control_event.h"
#include "controls.h"

#define SCHEME_MAX_PAGES 5
#define SCHEME_PAGE_COUNT scheme_page_max
#define SCHEME_PAGE_IDX scheme_page_idx

#define SCHEME_ON_GESTURE_FN control_scheme_on_gesture
#define SCHEME_ON_PLAIN_FN control_scheme_on_plain
#define SCHEME_INIT_FN control_scheme_driver_init

#define SCHEME_GESTURE_MASK control_scheme_gesture_mask

#define SCHEME_ON_GESTURE void SCHEME_ON_GESTURE_FN(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
#define SCHEME_ON_PLAIN void SCHEME_ON_PLAIN_FN(void* handler_arg, esp_event_base_t base, int32_t id, void* event_data)
#define SCHEME_INIT void SCHEME_INIT_FN(void)

#define SCHEME_BEGIN_EVENT(EVENT) ct_control_event_t* EVENT = (ct_control_event_t*) event_data;

#define SCHEME_NEXT_PAGE() {\
    xSemaphoreTake(CONTROL_BUTTON_REGISTER_LOCK, portMAX_DELAY);\
    SCHEME_PAGE_IDX ++; if(SCHEME_PAGE_IDX >= SCHEME_PAGE_COUNT) { SCHEME_PAGE_IDX = 0; }\
    ESP_LOGI(TAG, "Control scheme page is now %d", SCHEME_PAGE_IDX);\
    xSemaphoreGive(CONTROL_BUTTON_REGISTER_LOCK);\
}

#define SCHEME_SELECT_PAGE(IDX) {\
    xSemaphoreTake(CONTROL_BUTTON_REGISTER_LOCK, portMAX_DELAY);\
    SCHEME_PAGE_IDX = IDX-1; if(SCHEME_PAGE_IDX >= SCHEME_PAGE_COUNT) { SCHEME_PAGE_IDX = 0; }\
    ESP_LOGI(TAG, "Control scheme page is now %d", SCHEME_PAGE_IDX);\
    xSemaphoreGive(CONTROL_BUTTON_REGISTER_LOCK);\
}

#define SCHEME_SET_PAGE_COUNT(COUNT) {\
    if(COUNT > SCHEME_MAX_PAGES) {\
        ESP_LOGE(TAG, "SCHEME_SET_PAGE_COUNT(%d) is greater than SCHEME_MAX_PAGES = %d", COUNT, SCHEME_MAX_PAGES);\
        abort();\
    } else {\
        SCHEME_PAGE_COUNT = COUNT;\
    }\
}

#define SCHEME_SET_GESTURE_MASK(ID,MASK) {\
    SCHEME_GESTURE_MASK[ID] = MASK;\
}

extern uint8_t SCHEME_PAGE_IDX;
extern uint8_t SCHEME_PAGE_COUNT;
extern SCHEME_ON_GESTURE;
extern SCHEME_ON_PLAIN;
extern SCHEME_INIT;
extern ct_control_button_register_t SCHEME_GESTURE_MASK[SCHEME_MAX_PAGES];
esp_err_t control_scheme_init(void);