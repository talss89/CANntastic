#pragma once

#include "esp_err.h"

#define CT_DEVICE_NAME_LEN 32

extern char CT_DEVICE_NAME[CT_DEVICE_NAME_LEN];

esp_err_t ble_hid_init(void);