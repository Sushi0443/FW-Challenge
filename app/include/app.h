/*
 * Copyright (c) 2018-2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ADC_SAMPLER_H_
#define ZEPHYR_INCLUDE_ADC_SAMPLER_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>

extern bool en_ble;
extern uint16_t voltage_mv;
extern uint16_t sample_interval_ms;

extern struct k_work_delayable led_sample_work;
extern struct k_work_delayable led_error_work;
extern struct k_work_delayable battery_voltage_work;
extern struct k_work_delayable led_idle_work;

int adc_init(void);
int led_init(void);
int button_init(void);
void advertising_update(void);
void sample_count_increment_and_save(void);
int watchdog_init(void);


#endif /* ZEPHYR_INCLUDE_ADC_SAMPLER_H_ */