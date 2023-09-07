#pragma once

#include "sdkconfig.h"

#ifdef CONFIG_CT_SCHEME_WEBCONFIG

#include "esp_err.h"
#include "control_scheme.h"
#include "controls.h"

#define WEB_CONFIG_WIFI_SSID "CANtastic"
#define WEB_CONFIG_WIFI_PASS "12345678"
#define WEB_CONFIG_MAX_STA_CONN 1
#define WEB_CONFIG_STORAGE_NAMESPACE "ctwcstore"
#define WEB_CONFIG web_config
#define WEB_CONFIG_MAX_RULES CONFIG_CT_WEB_CONFIG_MAX_RULES
#define WEB_CONFIG_REQUEST_BUFFER_SIZE 8192

typedef struct {
    ct_control_button_t button;
    unsigned int count : 7;
    bool gesture : 1;
    ct_web_config_action_type_t action;
} ct_web_config_action_t;

typedef struct {
    ct_web_config_action_t actions[WEB_CONFIG_MAX_RULES];
} ct_web_config_page_rules_t;

typedef struct {
    ct_control_button_t system_button;
    ct_web_config_page_rules_t page_rules[SCHEME_MAX_PAGES];
} ct_web_config_t;

extern ct_web_config_t WEB_CONFIG;

esp_err_t web_config_init(void);

#endif