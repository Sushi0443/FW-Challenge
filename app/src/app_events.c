/* Centralized application event handling */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include "app.h"
#include "app_events.h"

LOG_MODULE_REGISTER(APP_EVENTS, CONFIG_APP_LOG_LEVEL);

atomic_t app_evt_bits;

/* Work item that reacts to events (runs on system workqueue, non-blocking) */
static void app_evt_work_handler(struct k_work *work)
{
    /* Check for any error bits and react */
    uint32_t err = atomic_get(&app_evt_bits) & APP_ERR_ANY;
    if (err) {
        LOG_ERR("App error bits: 0x%08x", err);

        /* Stop periodic/status works promptly */
        k_work_cancel_delayable(&led_sample_work);
        k_work_cancel_delayable(&led_idle_work);
        k_work_cancel_delayable(&battery_voltage_work);

        if (!app_evt_has(APP_ERR_LED))//make sure there are no errors from initializing LEDs before trying to blink the LED
        {
            k_work_reschedule(&led_error_work, K_NO_WAIT); //trigger a quick blink of status LED to indicate error  
        }
    } else {
        /* No errors; ensure periodic works are running */
    }
}

static K_WORK_DEFINE(app_evt_work, app_evt_work_handler);

void app_evt_raise(uint32_t bits)
{
    /* Set/post the bits, then schedule handler work */
    atomic_or(&app_evt_bits, bits);

    /* Run the reaction asynchronously */
    k_work_submit(&app_evt_work);
}

/* Initialize event object early */
static int app_events_init(void)
{
    atomic_clear(&app_evt_bits);
    return 0;
}

SYS_INIT(app_events_init, POST_KERNEL, 0);
