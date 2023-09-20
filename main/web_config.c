#include "sdkconfig.h"

#ifdef CONFIG_CT_SCHEME_WEBCONFIG

#include <sys/param.h>
#include <math.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_timer.h"

#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"
#include "dns_server.h"
#include "web_config.h"

#include "cJSON.h"

#include "tpms.h"
#include "ble_hid.h"
#include "host/ble_hs.h"

static const char *TAG = "web_config";

extern const char index_html_start[] asm("_binary_index_html_gz_start");
extern const char index_html_end[] asm("_binary_index_html_gz_end");

/* Type of Unescape algorithms to be used */
#define NGX_UNESCAPE_URI          (1)
#define NGX_UNESCAPE_REDIRECT     (2)

static uint64_t time_start;
static ble_addr_t tpms_found_addr;
SemaphoreHandle_t wait_for_pairing;

ct_web_config_t WEB_CONFIG = {
    .system_button = SIGNAL_C,
    .page_rules[0] = {
        .actions[0] = {.button = SIGNAL_C, .count = 1, .gesture = true, .action = ACTION_VOICE_ASSIST},
        .actions[1] = {.button = SIGNAL_C, .count = 2, .gesture = true, .action = ACTION_PLAY_PAUSE},
        .actions[2] = {.button = SIGNAL_C, .count = 3, .gesture = true, .action = ACTION_SKIP_FWD},
        .actions[3] = {.button = SIGNAL_C, .count = 4, .gesture = true, .action = ACTION_SKIP_REV}
    },
    .page_rules[1] = {
        .actions[0] = {.button = JOY_U, .count = 1, .gesture = false, .action = ACTION_VOL_UP},
        .actions[1] = {.button = JOY_D, .count = 1, .gesture = false, .action = ACTION_VOL_DOWN},
    },
    .page_rules[2] = {
        .actions[0] = {.button = JOY_U, .count = 1, .gesture = false, .action = ACTION_UP},
        .actions[1] = {.button = JOY_D, .count = 1, .gesture = false, .action = ACTION_DOWN},
        .actions[2] = {.button = JOY_L, .count = 1, .gesture = false, .action = ACTION_LEFT},
        .actions[3] = {.button = JOY_R, .count = 1, .gesture = false, .action = ACTION_RIGHT},
        .actions[4] = {.button = SIGNAL_C, .count = 1, .gesture = false, .action = ACTION_ENTER},
    },
    .tpms_addr[0] = {0},
    .tpms_addr[1] = {0}
    };
    

esp_err_t web_config_read(ct_web_config_t *config)
{
    esp_err_t err;
    nvs_handle_t hnd;
    size_t required_size = 0; // value will default to 0, if not set yet in NVS

    err = nvs_open(WEB_CONFIG_STORAGE_NAMESPACE, NVS_READWRITE, &hnd);
    if (err != ESP_OK)
        return err;

    err = nvs_get_blob(hnd, "config", NULL, &required_size);
    if (err != ESP_OK)
        return err;

    if (required_size == 0)
    {
        return ESP_ERR_NOT_FOUND;
    }

    if (required_size != sizeof(ct_web_config_t))
        return ESP_FAIL;

    err = nvs_get_blob(hnd, "config", config, &required_size);
    if (err != ESP_OK)
        return err;

    nvs_close(hnd);

    return ESP_OK;
}

esp_err_t web_config_write(ct_web_config_t *config)
{

    esp_err_t err;
    nvs_handle_t hnd;
    size_t required_size = sizeof(ct_web_config_t); // value will default to 0, if not set yet in NVS

    err = nvs_open(WEB_CONFIG_STORAGE_NAMESPACE, NVS_READWRITE, &hnd);
    if (err != ESP_OK)
        return err;

    err = nvs_set_blob(hnd, "config", config, required_size);

    if (err != ESP_OK)
        return err;

    // Commit
    err = nvs_commit(hnd);
    if (err != ESP_OK)
        return err;

    // Close
    nvs_close(hnd);

    return ESP_OK;
}

cJSON *web_config_to_json(ct_web_config_t *config)
{
    cJSON *root;
    cJSON *rules;

    root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "system_button", config->system_button);
    rules = cJSON_AddArrayToObject(root, "page_rules");

    for (uint8_t i = 0; i < SCHEME_MAX_PAGES; i++)
    {

        if (config->page_rules[i].actions[0].button == 0)
            continue;

        cJSON *rule = cJSON_CreateObject();
        cJSON *actions = cJSON_AddArrayToObject(rule, "actions");

        for (uint8_t j = 0; j < WEB_CONFIG_MAX_RULES; j++)
        {

            if (config->page_rules[i].actions[j].button == 0)
                continue;

            cJSON *action = cJSON_CreateObject();
            cJSON_AddNumberToObject(action, "button", config->page_rules[i].actions[j].button);
            cJSON_AddNumberToObject(action, "count", config->page_rules[i].actions[j].count);
            cJSON_AddBoolToObject(action, "gesture", config->page_rules[i].actions[j].gesture);
            cJSON_AddNumberToObject(action, "action", config->page_rules[i].actions[j].action);
            cJSON_AddItemToArray(actions, action);
        }
        cJSON_AddItemToArray(rules, rule);
    }

    return root;
}

esp_err_t web_config_from_json(ct_web_config_t *config, char *json)
{
    ct_web_config_t tmp = {};
    double numtmp = 0;

    memset(&tmp, 0, sizeof(ct_web_config_t));

    cJSON *root = cJSON_Parse(json);

    cJSON *rules = cJSON_GetObjectItem(root, "page_rules");
    size_t rule_page_size = cJSON_GetArraySize(rules);

    if (rule_page_size == 0 || rule_page_size > SCHEME_MAX_PAGES)
        return ESP_FAIL;

    numtmp = cJSON_GetNumberValue(cJSON_GetObjectItem(root, "system_button"));
    if (numtmp == NAN)
        return ESP_FAIL;
    tmp.system_button = (ct_control_button_t)numtmp;

    for (uint8_t i = 0; i < rule_page_size; i++)
    {
        cJSON *rule = cJSON_GetArrayItem(rules, i);
        cJSON *actions = cJSON_GetObjectItem(rule, "actions");
        size_t actions_size = cJSON_GetArraySize(actions);

        if (actions_size == 0 || actions_size > WEB_CONFIG_MAX_RULES)
            return ESP_FAIL;

        for (uint8_t j = 0; j < actions_size; j++)
        {
            cJSON *action = cJSON_GetArrayItem(actions, j);

            numtmp = cJSON_GetNumberValue(cJSON_GetObjectItem(action, "button"));
            if (numtmp == NAN)
                return ESP_FAIL;
            tmp.page_rules[i].actions[j].button = (ct_control_button_t)numtmp;

            numtmp = cJSON_GetNumberValue(cJSON_GetObjectItem(action, "count"));
            if (numtmp == NAN)
                return ESP_FAIL;
            tmp.page_rules[i].actions[j].count = (unsigned int)numtmp;

            tmp.page_rules[i].actions[j].gesture = cJSON_IsTrue(cJSON_GetObjectItem(action, "gesture"));

            numtmp = cJSON_GetNumberValue(cJSON_GetObjectItem(action, "action"));
            if (numtmp == NAN)
                return ESP_FAIL;
            tmp.page_rules[i].actions[j].action = (ct_web_config_action_type_t)numtmp;
        }
    }

    cJSON_Delete(root);
    memcpy(config, &tmp, sizeof(ct_web_config_t));
    return ESP_OK;
}

void ngx_unescape_uri(u_char **dst, u_char **src, size_t size, unsigned int type)
{
    u_char  *d, *s, ch, c, decoded;
    enum {
        sw_usual = 0,
        sw_quoted,
        sw_quoted_second
    } state;

    d = *dst;
    s = *src;

    state = 0;
    decoded = 0;

    while (size--) {

        ch = *s++;

        switch (state) {
        case sw_usual:
            if (ch == '?'
                    && (type & (NGX_UNESCAPE_URI | NGX_UNESCAPE_REDIRECT))) {
                *d++ = ch;
                goto done;
            }

            if (ch == '%') {
                state = sw_quoted;
                break;
            }

            *d++ = ch;
            break;

        case sw_quoted:

            if (ch >= '0' && ch <= '9') {
                decoded = (u_char) (ch - '0');
                state = sw_quoted_second;
                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                decoded = (u_char) (c - 'a' + 10);
                state = sw_quoted_second;
                break;
            }

            /* the invalid quoted character */

            state = sw_usual;

            *d++ = ch;

            break;

        case sw_quoted_second:

            state = sw_usual;

            if (ch >= '0' && ch <= '9') {
                ch = (u_char) ((decoded << 4) + (ch - '0'));

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);

                    break;
                }

                *d++ = ch;

                break;
            }

            c = (u_char) (ch | 0x20);
            if (c >= 'a' && c <= 'f') {
                ch = (u_char) ((decoded << 4) + (c - 'a') + 10);

                if (type & NGX_UNESCAPE_URI) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    *d++ = ch;
                    break;
                }

                if (type & NGX_UNESCAPE_REDIRECT) {
                    if (ch == '?') {
                        *d++ = ch;
                        goto done;
                    }

                    if (ch > '%' && ch < 0x7f) {
                        *d++ = ch;
                        break;
                    }

                    *d++ = '%'; *d++ = *(s - 2); *d++ = *(s - 1);
                    break;
                }

                *d++ = ch;

                break;
            }

            /* the invalid quoted character */

            break;
        }
    }

done:

    *dst = d;
    *src = s;
}

void web_config_uri_decode(char *dest, const char *src, size_t len)
{
    if (!src || !dest) {
        return;
    }

    unsigned char *src_ptr = (unsigned char *)src;
    unsigned char *dst_ptr = (unsigned char *)dest;
    ngx_unescape_uri(&dst_ptr, &src_ptr, len, NGX_UNESCAPE_URI);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

static void wifi_init_softap(void)
{
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WEB_CONFIG_WIFI_SSID,
            .ssid_len = strlen(WEB_CONFIG_WIFI_SSID),
            .password = WEB_CONFIG_WIFI_PASS,
            .max_connection = WEB_CONFIG_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK},
    };
    if (strlen(WEB_CONFIG_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);

    char ip_addr[16];
    inet_ntoa_r(ip_info.ip.addr, ip_addr, 16);
    ESP_LOGI(TAG, "Set up softAP with IP: %s", ip_addr);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:'%s' password:'%s'",
             WEB_CONFIG_WIFI_SSID, WEB_CONFIG_WIFI_PASS);
}

// HTTP GET Handler
static esp_err_t index_get_handler(httpd_req_t *req)
{
    const uint32_t index_html_len = index_html_end - index_html_start;

    ESP_LOGI(TAG, "Serve root");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_send(req, index_html_start, index_html_len);

    return ESP_OK;
}

static esp_err_t config_get_handler(httpd_req_t *req)
{
    cJSON *json_config;

    web_config_read(&WEB_CONFIG);
    json_config = web_config_to_json(&WEB_CONFIG);

    char *json_config_str = cJSON_Print(json_config);

    ESP_LOGI(TAG, "Serve GET config");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json_config_str, strlen(json_config_str));

    cJSON_Delete(json_config);
    cJSON_free(json_config_str);

    return ESP_OK;
}

void pair_tpms_update(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    ct_ble_tpms_packet_t *tpms = (ct_ble_tpms_packet_t *)event_data;

    ESP_LOGI(TAG, "TPMS Pair: %s", addr_str(&tpms->addr));
    tpms_found_addr = tpms->addr;
    xSemaphoreGive(wait_for_pairing);
}

void web_config_tpms_save(uint8_t wheel, ble_addr_t* addr) {

}

static esp_err_t pair_tpms_post_handler(httpd_req_t *req)
{
    esp_event_handler_instance_t handler;
    char *ret;
    char *buf;
    size_t buf_len;
    uint8_t wheel_index = 0;

    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1) {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            char param[WEB_CONFIG_HTTP_QUERY_KEY_MAX_LEN], dec_param[WEB_CONFIG_HTTP_QUERY_KEY_MAX_LEN] = {0};
            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "wheel", param, sizeof(param)) == ESP_OK) {
                ESP_LOGI(TAG, "Found URL query parameter => wheel=%s", param);
                web_config_uri_decode(dec_param, param, strnlen(param, WEB_CONFIG_HTTP_QUERY_KEY_MAX_LEN));
                ESP_LOGI(TAG, "Decoded query parameter => %s", dec_param);

                wheel_index = atoi(dec_param);

                if(wheel_index >= TPMS_WHEEL_COUNT) {
                    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid wheel index");
                    free(buf);
                    return ESP_FAIL;
                }

            } else {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to parse request");
                free(buf);
                return ESP_FAIL;
            }
        }
        free(buf);
    }

    wait_for_pairing = xSemaphoreCreateBinary();
    tpms_found_addr.type = 0xFF;
    ble_scan();
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(TPMS_EVENT_LOOP, TPMS_EVENT_BASE, TPMS_UPDATE_EVENT, pair_tpms_update, NULL, &handler));
    xSemaphoreTake(wait_for_pairing, pdMS_TO_TICKS(60000));
    ble_gap_disc_cancel();
    esp_event_handler_instance_unregister_with(TPMS_EVENT_LOOP, TPMS_EVENT_BASE, TPMS_UPDATE_EVENT, handler);
    httpd_resp_set_type(req, "application/json");

    if (tpms_found_addr.type == 0xFF)
    {
        ESP_LOGI(TAG, "No sensor found");
        httpd_resp_sendstr(req, "{\"found\": false}");
    }
    else
    {
        ESP_LOGI(TAG, "Found: %s", addr_str(&tpms_found_addr));

        WEB_CONFIG.tpms_addr[wheel_index] = tpms_found_addr;

        web_config_write(&WEB_CONFIG);

        int err = asprintf(&ret, "{\"found\": true, \"address\": \"%s\"}", addr_str(&tpms_found_addr));

        if (err == -1)
        {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate response buffer");
            return ESP_FAIL;
        }
        

        httpd_resp_sendstr(req, ret);
        free(ret);
    }

    return ESP_OK;
}

static esp_err_t config_post_handler(httpd_req_t *req)
{
    esp_err_t err;

    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = malloc(WEB_CONFIG_REQUEST_BUFFER_SIZE);
    int received = 0;

    if (buf == NULL)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to allocate request buffer");
        return ESP_FAIL;
    }

    if (total_len >= WEB_CONFIG_REQUEST_BUFFER_SIZE)
    {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request too large");
        free(buf);
        return ESP_FAIL;
    }
    while (cur_len < total_len)
    {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0)
        {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            free(buf);
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    err = web_config_from_json(&WEB_CONFIG, buf);

    if (err)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Failed to parse request");
        free(buf);
        return ESP_FAIL;
    }

    err = web_config_write(&WEB_CONFIG);

    if (err)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to commit config");
        free(buf);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\": true}");

    free(buf);
    return ESP_OK;
}

static const httpd_uri_t post_pair_tpms = {
    .uri = "/pair-tpms",
    .method = HTTP_POST,
    .handler = pair_tpms_post_handler};

static const httpd_uri_t post_config = {
    .uri = "/config",
    .method = HTTP_POST,
    .handler = config_post_handler};

static const httpd_uri_t get_config = {
    .uri = "/config",
    .method = HTTP_GET,
    .handler = config_get_handler};

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler};

// HTTP Error (404) Handler - Redirects all requests to the root page
esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    // Set status
    httpd_resp_set_status(req, "302 Temporary Redirect");
    // Redirect to the "/" root directory
    httpd_resp_set_hdr(req, "Location", "/");
    // iOS requires content in the response to detect a captive portal, simply redirecting is not sufficient.
    httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

    ESP_LOGI(TAG, "Redirecting to root");
    return ESP_OK;
}

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &get_config);
        httpd_register_uri_handler(server, &post_config);
        httpd_register_uri_handler(server, &post_pair_tpms);
        httpd_register_err_handler(server, HTTPD_404_NOT_FOUND, http_404_error_handler);
    }
    return server;
}

void web_config_should_enter_setup(void *handler_arg, esp_event_base_t base, int32_t id, void *event_data)
{
    ESP_LOGI(TAG, "CONFIG MODE!");
}

esp_err_t web_config_init(void)
{
    time_start = esp_timer_get_time();

    ESP_ERROR_CHECK(esp_event_handler_register_with(CONTROL_EVENT_LOOP, CONTROL_EVENT_BASE, CONTROL_BUTTON_SETUP_EVENT, web_config_should_enter_setup, NULL));

    /*
        Turn of warnings from HTTP server as redirecting traffic will yield
        lots of invalid requests
    */
    esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
    esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
    esp_log_level_set("httpd_parse", ESP_LOG_ERROR);

    // Initialize networking stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop needed by the  main app
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ble_gap_disc_cancel();

    // Initialize Wi-Fi including netif with default config
    esp_netif_create_default_wifi_ap();

    // Initialise ESP32 in SoftAP mode
    wifi_init_softap();

    // Start the server for the first time
    start_webserver();

    // Start the DNS server that will redirect all queries to the softAP IP
    start_dns_server();
    return ESP_OK;
}

#endif