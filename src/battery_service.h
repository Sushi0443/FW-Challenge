/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef BATTERY_SERVICE_H_
#define BATTERY_SERVICE_H_

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the custom battery service
 *
 * @return 0 on success, negative errno code on failure
 */
int battery_service_init(void);

/**
 * @brief Update battery voltage characteristic
 *
 * @param voltage_mv Battery voltage in millivolts
 * @return 0 on success, negative errno code on failure
 */
int battery_service_update_voltage(uint16_t voltage_mv);

/**
 * @brief Update sample interval characteristic
 *
 * @param interval_ms Sample interval in milliseconds
 * @return 0 on success, negative errno code on failure
 */
int battery_service_update_interval(uint32_t interval_ms);

/**
 * @brief Get current sample interval
 *
 * @return Current sample interval in milliseconds
 */
uint32_t battery_service_get_interval(void);

#ifdef __cplusplus
}
#endif

#endif /* BATTERY_SERVICE_H_ */
