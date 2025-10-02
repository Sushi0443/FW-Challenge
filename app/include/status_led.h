#pragma once

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <stdbool.h>
#include <stdint.h>

/*
 * Status LED patterns:
 * - Idle/OK: slow blink (short ON, long OFF)
 * - Sampling: brief double-blink (one-shot overlay)
 * - Error: rapid blink (continuous until cleared) *
 * Thread-safe for typical single-core usage; all actions run in a dedicated workqueue and are non-blocking.
 */

#ifdef __cplusplus
extern "C" {
#endif

int led_init(void);

#ifdef __cplusplus
}
#endif