/*
 * Minimal BLE API for this app
 */

#pragma once

#include <zephyr/types.h>

int ble_init(void);
void ble_advertising_start(void);
void notify_voltage(uint16_t mv);