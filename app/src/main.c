/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/types.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/kernel.h>

#include <zephyr/settings/settings.h>

#include "app.h"
#include "app_events.h"
#include "app_config.h"
#include "ble.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(APP, CONFIG_APP_LOG_LEVEL);

/* static bool app_button_state; */
static uint32_t sample_count;// count of successful battery voltage samples taken
/* settings key for persistent sample count */
static const char *const SAMPLE_COUNT_KEY = "app/sample_count"; /* settings key for persistent sample count */

/* BLE GATT and advertising are handled in ble.c */

#if IS_ENABLED(CONFIG_SETTINGS)
/* Settings load handler: called for keys under our subtree */
static int settings_set_handler(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	if (strcmp(key, "sample_count") == 0) {
		if (len != sizeof(sample_count)) {
			LOG_WRN("settings: unexpected size %zu for %s", len, key);
			return -EINVAL;
		}
		int rc = read_cb(cb_arg, &sample_count, sizeof(sample_count));
		if (rc >= 0) {
			LOG_INF("settings: loaded %s=%u", SAMPLE_COUNT_KEY, sample_count);
			return 0;
		}
		LOG_ERR("settings: read_cb failed (%d) for %s", rc, SAMPLE_COUNT_KEY);
		return rc;
	}
	return -ENOENT; /* key not handled */
}

/* Order is: get, set, commit, export */
SETTINGS_STATIC_HANDLER_DEFINE(app, "app", NULL, settings_set_handler, NULL, NULL);
#endif /* CONFIG_SETTINGS */

void sample_count_increment_and_save(void)
{
	sample_count++;
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		int rc = settings_save_one(SAMPLE_COUNT_KEY, &sample_count, sizeof(sample_count));
		if (rc) {
			LOG_ERR("settings: save %s failed (%d)", SAMPLE_COUNT_KEY, rc);
		} else {
			LOG_INF("settings: saved %s=%u", SAMPLE_COUNT_KEY, sample_count);
		}
	}
}


int main(void)
{
	int err;

	LOG_INF("Starting FW-CHALLENGE\n");

	// Initialize the LED first so that we can use it to indicate failures
	// in other subsystems
	err = led_init();
	if (err) {
		LOG_ERR("LEDs init failed (err %d)\n", err);
		app_evt_raise(APP_ERR_LED);
		return 0;
	}

	// Initialize the button toggle BLE advertising
	err = button_init();
	if (err) {
		LOG_ERR("Button init failed (err %d)\n", err);
		app_evt_raise(APP_ERR_BUTTON);
		return 0;
	}

	/* Start watchdog (feeds periodically in background) */
	err = watchdog_init();
	if (err) {
		LOG_WRN("Watchdog init failed (%d)", err);
	}

	/* Initialize BLE stack and register GATT/service in ble.c */
	err = ble_init();
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		app_evt_raise(APP_ERR_BLE);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		err = settings_load();
		if (err) {
			LOG_ERR("settings: load failed (%d)", err);
		}
	}

	// Initialize the ADC for battery voltage measurement
	err = adc_init();
	if (err) {
		LOG_ERR("ADC init failed (err %d)\n", err);
		app_evt_raise(APP_ERR_ADC);
		return 0;
	}
	// Start BLE advertising if enabled and not already started
	ble_advertising_start();

}
