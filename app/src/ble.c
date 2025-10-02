#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/settings/settings.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(BLE_MOD, CONFIG_APP_LOG_LEVEL);

#include "app.h"
#include "app_uuids.h"
#include "ble.h"
#include "app_events.h"

#define DEVICE_NAME             CONFIG_APP_BLE_DEVICE_NAME
#define DEVICE_NAME_LEN         (sizeof(DEVICE_NAME) - 1)

#define APP_NODE DT_PATH(app)
#define ENABLE_BLE DT_NODE_HAS_PROP(APP_NODE, enable_ble)



/* Index of the Voltage characteristic value attribute within custom_svc */
#define VOLTAGE_ATTR_IDX 2

bool en_ble = true;
static bool voltage_notify_enabled;

static void voltage_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
    voltage_notify_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static const struct bt_gatt_cpf voltage_cpf = {
    .format      = 0x06,            /* 16-bit unsigned int */
    .exponent    = -3,              /* milli */
    .unit        = 0x2728,
    .name_space  = 0x1,
    .description = 0,
};

static const struct bt_gatt_cpf sample_interval_cpf = {
    .format      = 0x06,
    .exponent    = -3,              /* milli */
    .unit        = 0x2703,
    .name_space  = 0x1,
    .description = 0,
};

/* Optional: Expose a read-only Service Name characteristic to help clients label the service. */
static const char service_name[] = "FW Challenge Service";

/* Locally declare a 128-bit UUID for the Service Name characteristic */
static struct bt_uuid_128 uuid_service_name = BT_UUID_INIT_128(
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x09, 0x87, 0x65, 0x43, 0x21);

static ssize_t read_service_name(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                 void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset,
                             service_name, sizeof(service_name) - 1);
}

static ssize_t read_voltage(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &voltage_mv, sizeof(voltage_mv));
}

static ssize_t read_sample_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                                    void *buf, uint16_t len, uint16_t offset)
{
    return bt_gatt_attr_read(conn, attr, buf, len, offset, &sample_interval_ms, sizeof(sample_interval_ms));
}

BT_GATT_SERVICE_DEFINE(custom_svc,
    BT_GATT_PRIMARY_SERVICE(BT_UUID_CUSTOM_SERVICE),
    BT_GATT_CHARACTERISTIC(BT_UUID_VOLTAGE_CHAR,
                           BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ,
                           read_voltage, NULL, &voltage_mv),
    BT_GATT_CUD("Voltage in mV", BT_GATT_PERM_READ),
    BT_GATT_CCC(voltage_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
    BT_GATT_CPF(&voltage_cpf),
    BT_GATT_CHARACTERISTIC(BT_UUID_SAMPLE_INTERVAL_CHAR,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_sample_interval, NULL, &sample_interval_ms),
    BT_GATT_CUD("Sample interval in ms", BT_GATT_PERM_READ),
    BT_GATT_CPF(&sample_interval_cpf),
    /* Read-only textual name to help clients identify the custom service */
    BT_GATT_CHARACTERISTIC(&uuid_service_name.uuid,
                           BT_GATT_CHRC_READ,
                           BT_GATT_PERM_READ,
                           read_service_name, NULL, NULL),
    BT_GATT_CUD("Service Name", BT_GATT_PERM_READ),
);

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Scan response: include the 128-bit service UUID and a small service-data payload with name */
static const uint8_t svc_data_payload[] = {
    /* 128-bit UUID in little-endian order must come first in service data */
    BT_UUID_CUSTOM_SERVICE_VAL,
    /* ASCII label (truncated to fit typical scan response space) */
    'F','W',' ','C','h','a','l','l','e','n','g','e'
};

static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_CUSTOM_SERVICE_VAL),
    BT_DATA(BT_DATA_SVC_DATA128, svc_data_payload, sizeof(svc_data_payload)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err) {
        LOG_ERR("Connection failed, err 0x%02x %s", err, bt_hci_err_to_str(err));
        return;
    }
    LOG_INF("Connected");
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    LOG_INF("Disconnected, reason 0x%02x %s", reason, bt_hci_err_to_str(reason));
}

static void recycled_cb(void)
{
    LOG_INF("Connection object available from previous conn. Disconnect is complete!");
    ble_advertising_start();
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected        = connected,
    .disconnected     = disconnected,
    .recycled         = recycled_cb,
};

#if defined(CONFIG_APP_BLE_SECURITY_ENABLED)
static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Passkey for %s: %06u", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing cancelled: %s", addr);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    char addr[BT_ADDR_LE_STR_LEN];
    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
    LOG_ERR("Pairing failed conn: %s, reason %d %s", addr, reason, bt_security_err_to_str(reason));
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .passkey_display = auth_passkey_display,
    .cancel = auth_cancel,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
    .pairing_complete = pairing_complete,
    .pairing_failed = pairing_failed
};
#endif

static struct k_work adv_work;

static void adv_work_handler(struct k_work *work)
{
    if (!en_ble) {
        (void)bt_le_adv_stop();
        LOG_INF("Advertising disabled by en_ble");
        return;
    }
    int err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_2, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err == -EALREADY) {
        LOG_DBG("Advertising already running");
        return;
    }
    if (err) {
        app_evt_raise(APP_ERR_BLE);
        LOG_ERR("Advertising failed to start (err %d)", err);
        return;
    }
    LOG_INF("Advertising started");
}

void ble_advertising_start(void)
{
    k_work_submit(&adv_work);
}

void notify_voltage(uint16_t mv)
{
    voltage_mv = mv;
    if (!voltage_notify_enabled) {
        return;
    }
    (void)bt_gatt_notify(NULL, &custom_svc.attrs[VOLTAGE_ATTR_IDX], &voltage_mv, sizeof(voltage_mv));
}

int ble_init(void)
{
    en_ble = DT_NODE_HAS_PROP(APP_NODE, enable_ble);// get the devicetree value
    en_ble = IS_ENABLED(CONFIG_APP_ENABLE_BLE);// get the Kconfig value. This is the final value to use
    // Initialize the Bluetooth Subsystem when enabled by DT
#if ENABLE_BLE
    int err = bt_enable(NULL);
    if (err) {
        app_evt_raise(APP_ERR_BLE);
        LOG_ERR("Bluetooth init failed (err %d)", err);
        return err;
    }
    /* Set device name from Kconfig option (non-persistent) */
    (void)bt_set_name(DEVICE_NAME);
#if defined(CONFIG_APP_BLE_SECURITY_ENABLED)
    err = bt_conn_auth_cb_register(&conn_auth_callbacks);
    if (err) {
        LOG_ERR("Failed to register authorization callbacks");
        return err;
    }
    err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
    if (err) {
        LOG_ERR("Failed to register authorization info callbacks");
        return err;
    }
#endif
    k_work_init(&adv_work, adv_work_handler);
    LOG_INF("Bluetooth initialized");
#endif
    return 0;
}
