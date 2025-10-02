/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>

#include "battery_service.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Advertising data */
static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0xf0, 0xde, 0xbc, 0x9a, 0x78, 0x56, 0x34, 0x12,
		      0x78, 0x56, 0x34, 0x12, 0x78, 0x56, 0x34, 0x12),
};

/* Scan response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

/* Connection callback handlers */
static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	
	if (err) {
		LOG_ERR("Connection failed (err %u)", err);
		return;
	}
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Connected: %s", addr);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	LOG_INF("Disconnected: %s (reason %u)", addr, reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Simulated battery voltage measurement */
static uint16_t simulate_battery_voltage(void)
{
	/* Simulate battery voltage between 3.0V and 4.2V */
	static uint16_t voltage = 4200;
	static int8_t direction = -1;
	
	voltage += (direction * 10);
	
	if (voltage <= 3000) {
		direction = 1;
		voltage = 3000;
	} else if (voltage >= 4200) {
		direction = -1;
		voltage = 4200;
	}
	
	return voltage;
}

/* Battery monitoring thread */
void battery_monitor_thread(void)
{
	LOG_INF("Battery monitoring thread started");
	
	while (1) {
		uint32_t interval_ms = battery_service_get_interval();
		uint16_t voltage_mv;
		
		/* Simulate battery voltage measurement */
		voltage_mv = simulate_battery_voltage();
		
		/* Update the characteristic */
		battery_service_update_voltage(voltage_mv);
		
		LOG_INF("Battery voltage: %u mV (interval: %u ms)", voltage_mv, interval_ms);
		
		/* Sleep for the configured interval */
		k_msleep(interval_ms);
	}
}

K_THREAD_DEFINE(battery_monitor_tid, 1024,
		battery_monitor_thread, NULL, NULL, NULL,
		7, 0, 0);

int main(void)
{
	int err;
	
	LOG_INF("Starting Bluetooth Low Energy Battery Monitor");
	LOG_INF("NCS version: 3.1.1");
	
	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}
	
	LOG_INF("Bluetooth initialized");
	
	/* Initialize the battery service */
	err = battery_service_init();
	if (err) {
		LOG_ERR("Battery service init failed (err %d)", err);
		return err;
	}
	
	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}
	
	LOG_INF("Advertising successfully started");
	LOG_INF("Device name: %s", CONFIG_BT_DEVICE_NAME);
	
	return 0;
}
