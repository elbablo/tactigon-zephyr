#include "ble/ble.h"

LOG_MODULE_REGISTER(ble);

/* UUID caratteristica */
#define BT_UUID_CHAR_IMU BT_UUID_DECLARE_128( \
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08, \
    0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10)

#define BT_UUID_CHAR_AUDIO BT_UUID_DECLARE_128( \
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18, \
    0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20)

/* ---- Connessione attiva ---- */
static struct bt_conn *active_conn;
const struct bt_gatt_attr *ble_audio_char;
const struct bt_gatt_attr *ble_imu_char;

/* ---- Connection callbacks ---- */
static void bt_connected(struct bt_conn *conn, uint8_t err)
{
    if (!err) {
        active_conn = bt_conn_ref(conn);
        k_event_set(&app_events, EVT_BT_CONNECTED);
    }
}

static void bt_disconnected(struct bt_conn *conn, uint8_t reason)
{
    if (active_conn) {
        bt_conn_unref(active_conn);
        active_conn = NULL;
    }
    k_event_set(&app_events, EVT_BT_DISCONNECTED);
}

static void bt_recycled(void) {}

/* Callback struttura */
static struct bt_conn_cb conn_callbacks = {
    .connected = bt_connected,
    .disconnected = bt_disconnected,
    .recycled = bt_recycled,
};

/* ---- CCC callbacks ---- */
static void ccc_char_imu_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    if (value == BT_GATT_CCC_NOTIFY) {
        k_event_set(&app_events, EVT_BT_SUB_CHAR_IMU);
    } else {
        k_event_set(&app_events, EVT_BT_UNSUB_CHAR_IMU);
    }
}

static void ccc_char_audio_changed(const struct bt_gatt_attr *attr, uint16_t value) {
    if (value == BT_GATT_CCC_NOTIFY) {
        k_event_set(&app_events, EVT_BT_SUB_CHAR_AUDIO);
    } else {
        k_event_set(&app_events, EVT_BT_UNSUB_CHAR_AUDIO);
    }
}

/* ---- GATT service ---- */
BT_GATT_SERVICE_DEFINE(tactigon_service,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_DECLARE_16(0x180A)),

    BT_GATT_CHARACTERISTIC(
        BT_UUID_CHAR_IMU,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL
    ),
    BT_GATT_CCC(ccc_char_imu_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE
    ),

    BT_GATT_CHARACTERISTIC(
        BT_UUID_CHAR_AUDIO,
        BT_GATT_CHRC_NOTIFY,
        BT_GATT_PERM_NONE,
        NULL, NULL, NULL
    ),
    BT_GATT_CCC(ccc_char_audio_changed,
        BT_GATT_PERM_READ | BT_GATT_PERM_WRITE
    )
);



/* ---- Notify audio ---- */
int ble_audio_notify(const adpcm_packet_t *pkt)
{
    for (size_t offset = 0; offset < pkt->len; offset += BLE_BUFFER_LEN) {
        size_t chunk = MIN(BLE_BUFFER_LEN, pkt->len - offset);
        
        bt_gatt_notify(active_conn, ble_audio_char, pkt->buf + offset, chunk);
    }

    return 0;
}

/* ---- Notify imu ---- */
int ble_imu_notify(const imu_packet_t *pkt)
{
    return bt_gatt_notify(active_conn, ble_imu_char, pkt->buff, pkt->len);
}

/* ---- Advertising ---- */
static const struct bt_le_adv_param adv_param =
    BT_LE_ADV_PARAM_INIT(
        BT_LE_ADV_OPT_CONN | BT_LE_ADV_OPT_USE_IDENTITY,
        BT_GAP_ADV_FAST_INT_MIN_2,
        BT_GAP_ADV_FAST_INT_MAX_2,
        NULL
    );

static const struct bt_data ad[] = {
    BT_DATA(BT_DATA_NAME_COMPLETE, BLE_NAME, sizeof(BLE_NAME) - 1),
};

/* ---- Init ---- */
int ble_init(void)
{
    int err = bt_enable(NULL);
    if (err) return err;

    err = bt_conn_cb_register(&conn_callbacks);
    if (err) return err;

    ble_audio_char = bt_gatt_find_by_uuid(tactigon_service.attrs, tactigon_service.attr_count, BT_UUID_CHAR_AUDIO);
    ble_imu_char = bt_gatt_find_by_uuid(tactigon_service.attrs, tactigon_service.attr_count, BT_UUID_CHAR_IMU);

    err = ble_adv_start();
    return err;
}

int ble_adv_start(void)
{
    return bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), NULL, 0);
}
