/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include "app.h"
#include "app_events.h"
#include "ble.h"
// #include <nrfx_saadc.h>
/* #include <helpers/nrfx_gppi.h> */

LOG_MODULE_REGISTER(ADC_SAMPLER, CONFIG_ADC_LOG_LEVEL);

/* Global variables */
uint16_t voltage_mv = 0;
uint16_t sample_interval_ms = 0;

/* Prefer DT defaults first, then override with Kconfig values at init */
#if DT_NODE_EXISTS(DT_PATH(app)) && DT_NODE_HAS_PROP(DT_PATH(app), sample_interval_ms)
#define DT_SAMPLE_INTERVAL_MS DT_PROP(DT_PATH(app), sample_interval_ms)
#else
#define DT_SAMPLE_INTERVAL_MS 1000
#endif

#if DT_NODE_EXISTS(DT_PATH(app)) && DT_NODE_HAS_PROP(DT_PATH(app), voltage_threshold_mv)
#define DT_VOLTAGE_THRESHOLD_MV DT_PROP(DT_PATH(app), voltage_threshold_mv)
#else
#define DT_VOLTAGE_THRESHOLD_MV 3000
#endif

/* Effective threshold used at runtime */
static uint16_t voltage_threshold_mv = DT_VOLTAGE_THRESHOLD_MV;

#if !DT_NODE_EXISTS(DT_PATH(vbatt)) || \
	!DT_NODE_HAS_PROP(DT_PATH(vbatt), io_channels)
#error "No suitable devicetree overlay specified"
#endif

/* Local variables */
static uint16_t buf;

//declare a work item for sampling battery voltage 
struct k_work_delayable battery_voltage_work;
/* using adc_channel_setup_dt() to configure the ADC channel */
static const struct adc_dt_spec adc_ch = ADC_DT_SPEC_GET_BY_IDX(DT_NODELABEL(vbatt), 0);
/* Single ADC io-channel specified in devicetree (first entry). */

static struct adc_sequence sequence = {
	.buffer = &buf,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(buf),
};

// Work function to measure battery voltage periodically with sample_interval_ms
void measure_battery_voltage(struct k_work *work)
{
	int err;
	int32_t val_mv;

	// Ensure the ADC device resumes before sampling when PM is enabled
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		err = pm_device_action_run(adc_ch.dev, PM_DEVICE_ACTION_RESUME);
		if (err < 0) {
			LOG_ERR("Failed to resume ADC device (%d)", err);
		}
	}

	err = adc_read_dt(&adc_ch, &sequence);
	if (err < 0) {
		printk("Could not read (%d)", err);
		app_evt_raise(APP_ERR_ADC);
		return;
	}

	/* Convert raw sample to signed/unsigned as needed */
	if (adc_ch.channel_cfg.differential) {
		val_mv = (int32_t)((int16_t)buf);
	} else {
		val_mv = (int32_t)buf;
	}
	LOG_INF("raw=%"PRId32, val_mv);

	/* Convert raw value to millivolts using ADC instance config */
	err = adc_raw_to_millivolts_dt(&adc_ch, &val_mv);
	if (err < 0) {
		printk("Failed to convert to mV (%d)", err);
		app_evt_raise(APP_ERR_ADC);
		LOG_INF(" (mV N/A)\n");
	} else {
		voltage_mv = (uint16_t)val_mv;
		LOG_INF(", %"PRId32" mV\n", val_mv);
			notify_voltage((uint16_t)val_mv);
			/* Increment and persist the sample counter on successful measurements */
			sample_count_increment_and_save();
	}

	k_work_reschedule(&led_sample_work, K_NO_WAIT);// indicate a sample event by blinking the LED twice quickly

	/* Hook battery algorithm here if desired, e.g.:
		*   unsigned int pct = battery_level_pptt(val_mv, levels);
		*/

	if (val_mv < voltage_threshold_mv) {
		LOG_WRN("battery voltage: %d is below threshold\n",
				val_mv);
	}

	// Suspend the ADC device to save power until next sampling when PM is enabled
	if (IS_ENABLED(CONFIG_PM_DEVICE)) {
		err = pm_device_action_run(adc_ch.dev, PM_DEVICE_ACTION_SUSPEND);
		if (err < 0) {
			LOG_ERR("Failed to suspend ADC device (%d)", err);
		}
	}

	// Reschedule the work again after sample_interval_ms
	// Only if no error condition is present and notifications is enabled
	if (!app_evt_has(APP_ERR_ANY) && en_ble) 
	{
		k_work_reschedule(&battery_voltage_work, K_MSEC(sample_interval_ms));
	}
}

int adc_init(void)
{
	int err;

	/* Initialize from Devicetree first */
	sample_interval_ms = DT_SAMPLE_INTERVAL_MS;
	voltage_threshold_mv = DT_VOLTAGE_THRESHOLD_MV;

	/* Then apply Kconfig overrides (take precedence) */
#ifdef CONFIG_APP_SAMPLE_INTERVAL_MS
	sample_interval_ms = CONFIG_APP_SAMPLE_INTERVAL_MS;
#endif
#ifdef CONFIG_APP_VOLTAGE_THRESHOLD_MV
	voltage_threshold_mv = CONFIG_APP_VOLTAGE_THRESHOLD_MV;
#endif

	k_work_init_delayable(&battery_voltage_work, measure_battery_voltage);

	/* Configure the single ADC channel prior to sampling. */
	if (adc_is_ready_dt(&adc_ch) == false) {
		LOG_ERR("ADC device is not ready %s", adc_ch.dev->name);
		return 1;
	}

	err = adc_channel_setup_dt(&adc_ch);
	if (err < 0) {
		LOG_ERR("%s: device not ready (%d)", adc_ch.dev->name, err);
		return err;
	}

	// sequence.channels = BIT(adc_ch.channel_id);
	// sequence.resolution = adc_ch.resolution;
	// sequence.oversampling = adc_ch.oversampling;
	sequence.calibrate = true;	

	err = adc_sequence_init_dt(&adc_ch, &sequence);
	if (err < 0) {
		LOG_ERR("Failed to initialize ADC sequence (%d)", err);
		return err;
	}

	//take the first sample immediately
	k_work_reschedule(&battery_voltage_work, K_NO_WAIT);

	return 0;
}
