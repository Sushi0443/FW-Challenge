/*
 * Application event bits and API
 */

#ifndef APP_EVENTS_H_
#define APP_EVENTS_H_

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>

/* Event bits set in app_evt to indicate events and errors */
enum {
    APP_EVT_OK    = BIT(0),
    APP_ERR_LED   = BIT(1),
    APP_ERR_BUTTON= BIT(2),
    APP_ERR_ADC   = BIT(3),
    APP_ERR_BLE   = BIT(4),
};

#define APP_ERR_ANY (APP_ERR_LED | APP_ERR_BUTTON | APP_ERR_ADC | APP_ERR_BLE)

/* Global event bitmask (defined in a .c file) */
extern atomic_t app_evt_bits;

/* Helper to raise events and trigger app-level error handling */
void app_evt_raise(uint32_t bits);

/* Query if any bits in mask are set */
static inline bool app_evt_has(uint32_t mask)
{
    return (atomic_get(&app_evt_bits) & mask) != 0;
}

/* Clear specific event bits */
static inline void app_evt_clear(uint32_t bits)
{
    atomic_and(&app_evt_bits, ~bits);
}

#endif /* APP_EVENTS_H_ */