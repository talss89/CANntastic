#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_mac.h"

/* BLE */
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "gatt_svr.h"
#include "hid_func.h"

#include "ble_hid.h"

#define MAC2STR_REV(a) (a)[5], (a)[4], (a)[3], (a)[2], (a)[1], (a)[0]

static const char *TAG = "ble_hid";

static int bleprph_gap_event(struct ble_gap_event *event, void *arg);
static uint8_t own_addr_type;

/**
 * Logs information about a connection to the console.
 */
static void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc) {
    ESP_LOGI(TAG, "handle=%d our_ota_addr_type=%d our_ota_addr=" MACSTR " our_id_addr_type=%d our_id_addr=" MACSTR " peer_ota_addr_type=%d peer_ota_addr=" MACSTR " peer_id_addr_type=%d peer_id_addr=" MACSTR " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                  "encrypted=%d authenticated=%d bonded=%d\n",
             desc->conn_handle,
             desc->our_ota_addr.type,
             MAC2STR_REV(desc->our_ota_addr.val),
             desc->our_id_addr.type,
             MAC2STR_REV(desc->our_id_addr.val),
             desc->peer_ota_addr.type,
             MAC2STR_REV(desc->peer_ota_addr.val),
             desc->peer_id_addr.type,
             MAC2STR_REV(desc->peer_id_addr.val),
             desc->conn_itvl, desc->conn_latency,
             desc->supervision_timeout,
             desc->sec_state.encrypted,
             desc->sec_state.authenticated,
             desc->sec_state.bonded);
}

int user_parse(const struct ble_hs_adv_field *data, void *arg)
{
    ESP_LOGI(TAG, "Parse data: field len %d, type %x, total %d bytes",
             data->length, data->type, data->length + 2); /* first byte type and second byte length */
    return 1;
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void bleprph_advertise(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
     *     o Discoverability in forthcoming advertisement (general)
     *     o BLE-only (BR/EDR unsupported).
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.adv_itvl_is_present = 1;
    fields.adv_itvl = 40;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.appearance = HID_KEYBOARD_APPEARENCE;
    fields.appearance_is_present = 1;

    fields.uuids16 = (ble_uuid16_t[]){
        BLE_UUID16_INIT(GATT_UUID_HID_SERVICE)};
    fields.num_uuids16 = 1;
    fields.uuids16_is_complete = 1;

    uint8_t buf[50], buf_sz;
    rc = ble_hs_adv_set_fields(&fields, buf, &buf_sz, 50);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error setting advertisement data to buf; rc=%d", rc);
        return;
    }
    if (buf_sz > BLE_HS_ADV_MAX_SZ)
    {
        ESP_LOGE(TAG, "Too long advertising data: name %s, appearance %x, uuid16 %x, advsize = %d",
                 name, fields.appearance, GATT_UUID_HID_SERVICE, buf_sz);
        ble_hs_adv_parse(buf, buf_sz, user_parse, NULL);
        return;
    }

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error setting advertisement data; rc=%d", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, bleprph_gap_event, NULL);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error enabling advertisement; rc=%d", rc);
        return;
    }
}

// default password for bonding, can be changed from sdkconfig var CONFIG_EXAMPLE_DISP_PASSWD
int Disp_password = 123456;

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int bleprph_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        ESP_LOGI(TAG, "connection %s; status=%d ",
                 event->connect.status == 0 ? "established" : "failed",
                 (int) event->connect.status);
        if (event->connect.status == 0)
        {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);

            hid_clean_vars(&desc);
        }
        else
        {
            /* Connection failed; resume advertising. */
            bleprph_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnect; reason=%d ", (int) event->disconnect.reason);
        hid_set_disconnected();

        /* Connection terminated; resume advertising. */
        bleprph_advertise();
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE_REQ:
        /* The central requested the connection parameters update. */
        ESP_LOGI(TAG, "connection update request");
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        ESP_LOGI(TAG, "connection updated; status=%d ",
                 (int) event->conn_update.status);
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "advertise complete; reason=%d",
                 (int) event->adv_complete.reason);
        bleprph_advertise();
        return 0;

    case BLE_GAP_EVENT_ENC_CHANGE:
        /* Encryption has been enabled or disabled for this connection. */
        ESP_LOGI(TAG, "encryption change event; status=%d ",
                 (int) event->enc_change.status);
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%04X "
                      "reason=%d prev_notify=%d cur_notify=%d prev_indicate=%d cur_indicate=%d",
                 (int) event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 (int) event->subscribe.reason,
                 (int) event->subscribe.prev_notify,
                 (int) event->subscribe.cur_notify,
                 (int) event->subscribe.prev_indicate,
                 (int) event->subscribe.cur_indicate);

        hid_set_notify(event->subscribe.attr_handle,
                       event->subscribe.cur_notify,
                       event->subscribe.cur_indicate);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        ESP_LOGI(TAG, "notify event; status=%d conn_handle=%d attr_handle=%04X type=%s",
                 (int) event->notify_tx.status,
                 (int) event->notify_tx.conn_handle,
                 event->notify_tx.attr_handle,
                 event->notify_tx.indication ? "indicate" : "notify");
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d",
                 (int) event->mtu.conn_handle,
                 (int) event->mtu.channel_id,
                 (int) event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_REPEAT_PAIRING:
        /* We already have a bond with the peer, but it is attempting to
         * establish a new secure link.  This app sacrifices security for
         * convenience: just throw away the old bond and accept the new link.
         */

        /* Delete the old bond. */
        rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
        assert(rc == 0);
        ble_store_util_delete_peer(&desc.peer_id_addr);

        /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
         * continue with the pairing operation.
         */
        return BLE_GAP_REPEAT_PAIRING_RETRY;

    case BLE_GAP_EVENT_PASSKEY_ACTION:
        ESP_LOGI(TAG, "PASSKEY_ACTION_EVENT started \n");
        struct ble_sm_io pkey = {0};

        if (event->passkey.params.action == BLE_SM_IOACT_DISP)
        {
            pkey.action = event->passkey.params.action;

            /* This is the passkey to be entered on peer */
            pkey.passkey = Disp_password;

            ESP_LOGI(TAG, "Enter passkey %d on the peer side", (int) pkey.passkey);
            rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
            ESP_LOGI(TAG, "ble_sm_inject_io result: %d", (int) rc);
        }
        else if (event->passkey.params.action == BLE_SM_IOACT_INPUT ||
                 event->passkey.params.action == BLE_SM_IOACT_NUMCMP ||
                 event->passkey.params.action == BLE_SM_IOACT_OOB)
        {
            ESP_LOGE(TAG, "BLE_SM_IOACT_INPUT, BLE_SM_IOACT_NUMCMP, BLE_SM_IOACT_OOB"
                          " bonding not supported!");
        }
        return 0;

    default:
        ESP_LOGI(TAG, "Unknown GAP event: %d", (int) event->type);
    }

    return 0;
}

static void bleprph_on_reset(int reason) {
    ESP_LOGE(TAG, "Resetting state; reason=%d", (int) reason);
}

static void bleprph_on_sync(void) {
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0)
    {
        ESP_LOGE(TAG, "error determining address type; rc=%d", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    ESP_LOGI(TAG, "Device Address: " MACSTR, MAC2STR_REV(addr_val));

    /* Begin advertising. */
    bleprph_advertise();
}

void bleprph_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

/* this declaration was taken from examples */
void ble_store_config_init(void);

esp_err_t ble_hid_init(void)
{

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = bleprph_on_reset;
    ble_hs_cfg.sync_cb = bleprph_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_bonding = 1;
    /* Enable the appropriate bit masks to make sure the keys
     * that are needed are exchanged
     */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ENC;
#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#endif
#ifdef CONFIG_EXAMPLE_RESOLVE_PEER_ADDR
    /* Stores the IRK */
    ble_hs_cfg.sm_our_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist |= BLE_SM_PAIR_KEY_DIST_ID;
#endif

    int rc = gatt_svr_init();
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set("Cantastic2");
    assert(rc == 0);

    rc = ble_svc_gap_device_appearance_set(HID_KEYBOARD_APPEARENCE); /* HID Keyboard*/
    assert(rc == 0);

    ble_store_config_init();

    nimble_port_freertos_init(bleprph_host_task);

    return ESP_OK;
}