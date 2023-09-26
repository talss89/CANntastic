#pragma once
#include "sdkconfig.h"

#ifdef CONFIG_CT_CUSTOM_GPIO

    #define CUSTOM_GPIO_INPUT_PINS ((1ULL << 0) | (1ULL << 15) | (1ULL << 18) | (1ULL << 17) | (1ULL << 21))
    
    #define CUSTOM_GPIO_SETUP() {\
        gpio_config_t custom_io_conf = {};\
        custom_io_conf.intr_type = GPIO_INTR_DISABLE;\
        custom_io_conf.mode = GPIO_MODE_INPUT;\
        custom_io_conf.pin_bit_mask = CUSTOM_GPIO_INPUT_PINS;\
        custom_io_conf.pull_down_en = 0;\
        custom_io_conf.pull_up_en = GPIO_PULLUP_ENABLE;\
        gpio_config(&custom_io_conf);\
    }
    
    
    #define CUSTOM_READ_GPIO_BUTTONS() {\
    } 

#endif