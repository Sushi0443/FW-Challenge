/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "battery_service.h"

LOG_MODULE_REGISTER(battery_service, LOG_LEVEL_DBG);

/* Custom 128-bit UUID for Battery Monitor Service */
#define BT_UUID_BATTERY_MONITOR_SERVICE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef0)

/* Battery Voltage Characteristic UUID */
#define BT_UUID_BATTERY_VOLTAGE_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef1)

/* Sample Interval Characteristic UUID */
#define BT_UUID_SAMPLE_INTERVAL_VAL \
	BT_UUID_128_ENCODE(0x12345678, 0x1234, 0x5678, 0x1234, 0x56789abcdef2)

#define BT_UUID_BATTERY_MONITOR_SERVICE \
	BT_UUID_DECLARE_128(BT_UUID_BATTERY_MONITOR_SERVICE_VAL)
#define BT_UUID_BATTERY_VOLTAGE \
	BT_UUID_DECLARE_128(BT_UUID_BATTERY_VOLTAGE_VAL)
#define BT_UUID_SAMPLE_INTERVAL \
	BT_UUID_DECLARE_128(BT_UUID_SAMPLE_INTERVAL_VAL)

/* Service characteristics data */
static uint16_t battery_voltage_mv = 3300; /* Default 3.3V */
static uint32_t sample_interval_ms = 1000; /* Default 1 second */

/* GATT attribute indices */
enum {
	BATTERY_MONITOR_GATT_IDX_SVC,
	BATTERY_MONITOR_GATT_IDX_VOLTAGE_CHAR,
	BATTERY_MONITOR_GATT_IDX_VOLTAGE_VAL,
	BATTERY_MONITOR_GATT_IDX_VOLTAGE_CCC,
	BATTERY_MONITOR_GATT_IDX_INTERVAL_CHAR,
	BATTERY_MONITOR_GATT_IDX_INTERVAL_VAL,
	BATTERY_MONITOR_GATT_IDX_INTERVAL_CCC,
	BATTERY_MONITOR_GATT_IDX_COUNT,
};

/* Forward declarations */
static ssize_t read_voltage(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset);
static ssize_t read_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset);
static ssize_t write_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags);
static void voltage_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);
static void interval_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value);

/* Battery Monitor Service Declaration */
BT_GATT_SERVICE_DEFINE(battery_monitor_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_BATTERY_MONITOR_SERVICE),
	
	/* Battery Voltage Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_BATTERY_VOLTAGE,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ,
			       read_voltage, NULL, &battery_voltage_mv),
	BT_GATT_CCC(voltage_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	
	/* Sample Interval Characteristic */
	BT_GATT_CHARACTERISTIC(BT_UUID_SAMPLE_INTERVAL,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE | BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_interval, write_interval, &sample_interval_ms),
	BT_GATT_CCC(interval_ccc_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
);

/* Read battery voltage characteristic */
static ssize_t read_voltage(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    void *buf, uint16_t len, uint16_t offset)
{
	uint16_t voltage = battery_voltage_mv;
	
	LOG_DBG("Battery voltage read: %u mV", voltage);
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &voltage, sizeof(voltage));
}

/* Read sample interval characteristic */
static ssize_t read_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			     void *buf, uint16_t len, uint16_t offset)
{
	uint32_t interval = sample_interval_ms;
	
	LOG_DBG("Sample interval read: %u ms", interval);
	
	return bt_gatt_attr_read(conn, attr, buf, len, offset, &interval, sizeof(interval));
}

/* Write sample interval characteristic */
static ssize_t write_interval(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			      const void *buf, uint16_t len, uint16_t offset, uint8_t flags)
{
	uint32_t new_interval;
	
	if (offset != 0) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}
	
	if (len != sizeof(new_interval)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}
	
	memcpy(&new_interval, buf, sizeof(new_interval));
	
	/* Validate interval (min 100ms, max 60s) */
	if (new_interval < 100 || new_interval > 60000) {
		LOG_WRN("Invalid sample interval: %u ms", new_interval);
		return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
	}
	
	sample_interval_ms = new_interval;
	LOG_INF("Sample interval updated to: %u ms", sample_interval_ms);
	
	return len;
}

/* CCC changed callback for voltage characteristic */
static void voltage_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	
	LOG_INF("Battery voltage notifications %s", notif_enabled ? "enabled" : "disabled");
}

/* CCC changed callback for interval characteristic */
static void interval_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value == BT_GATT_CCC_NOTIFY);
	
	LOG_INF("Sample interval notifications %s", notif_enabled ? "enabled" : "disabled");
}

int battery_service_init(void)
{
	LOG_INF("Battery Monitor Service initialized");
	LOG_INF("Service UUID: 12345678-1234-5678-1234-56789abcdef0");
	LOG_INF("Voltage Char UUID: 12345678-1234-5678-1234-56789abcdef1");
	LOG_INF("Interval Char UUID: 12345678-1234-5678-1234-56789abcdef2");
	
	return 0;
}

int battery_service_update_voltage(uint16_t voltage_mv)
{
	int err;
	
	battery_voltage_mv = voltage_mv;
	
	LOG_DBG("Updating battery voltage to: %u mV", voltage_mv);
	
	err = bt_gatt_notify(NULL, &battery_monitor_svc.attrs[BATTERY_MONITOR_GATT_IDX_VOLTAGE_VAL],
			     &battery_voltage_mv, sizeof(battery_voltage_mv));
	if (err && err != -ENOTCONN) {
		LOG_ERR("Failed to send voltage notification: %d", err);
		return err;
	}
	
	return 0;
}

int battery_service_update_interval(uint32_t interval_ms)
{
	int err;
	
	sample_interval_ms = interval_ms;
	
	LOG_DBG("Updating sample interval to: %u ms", interval_ms);
	
	err = bt_gatt_notify(NULL, &battery_monitor_svc.attrs[BATTERY_MONITOR_GATT_IDX_INTERVAL_VAL],
			     &sample_interval_ms, sizeof(sample_interval_ms));
	if (err && err != -ENOTCONN) {
		LOG_ERR("Failed to send interval notification: %d", err);
		return err;
	}
	
	return 0;
}

uint32_t battery_service_get_interval(void)
{
	return sample_interval_ms;
}
