#include <zephyr/kernel.h>
#include <zephyr/device.h>
#if IS_ENABLED(CONFIG_WATCHDOG)
#include <zephyr/drivers/watchdog.h>
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(APP_WDT, CONFIG_APP_LOG_LEVEL);

#if IS_ENABLED(CONFIG_APP_WDT_ENABLE) && IS_ENABLED(CONFIG_WATCHDOG)

/* Pick the first watchdog by chosen or alias; fallback to DT_NODELABEL(wdt0) */
#if DT_HAS_CHOSEN(zephyr_watchdog)
#define WDT_NODE DT_CHOSEN(zephyr_watchdog)
#elif DT_NODE_EXISTS(DT_ALIAS(watchdog0))
#define WDT_NODE DT_ALIAS(watchdog0)
#elif DT_NODE_EXISTS(DT_NODELABEL(wdt0))
#define WDT_NODE DT_NODELABEL(wdt0)
#else
#define WDT_NODE DT_INVALID_NODE
#endif

#if DT_NODE_EXISTS(WDT_NODE)
static const struct device *const wdt_dev = DEVICE_DT_GET(WDT_NODE);
#else
static const struct device *const wdt_dev = NULL;
#endif

static int wdt_channel_id = -1;
static struct k_work_delayable wdt_feed_work;

static void wdt_feed_handler(struct k_work *work)
{
    if (wdt_channel_id >= 0 && wdt_dev) {
        (void)wdt_feed(wdt_dev, wdt_channel_id);
    }
    /* Feed roughly every half-timeout for margin */
    k_work_reschedule(&wdt_feed_work, K_MSEC(CONFIG_APP_WDT_TIMEOUT_MS / 2));
}

int watchdog_init(void)
{
    if (!wdt_dev) {
        LOG_WRN("No watchdog device node found");
        return -ENODEV;
    }
    if (!device_is_ready(wdt_dev)) {
        LOG_ERR("Watchdog device not ready");
        return -ENODEV;
    }

    static struct wdt_timeout_cfg cfg = {
        .window = {
            .min = 0,
            .max = CONFIG_APP_WDT_TIMEOUT_MS,
        },
        .flags = WDT_FLAG_RESET_SOC, /* ensure SoC reset on timeout */
        .callback = NULL,           /* no interrupt callback, let it reset */
    };

    wdt_channel_id = wdt_install_timeout(wdt_dev, &cfg);
    if (wdt_channel_id < 0) {
        LOG_ERR("wdt_install_timeout failed (%d)", wdt_channel_id);
        return wdt_channel_id;
    }

    int err = wdt_setup(wdt_dev, WDT_OPT_PAUSE_HALTED_BY_DBG);
    if (err) {
        LOG_ERR("wdt_setup failed (%d)", err);
        return err;
    }

    k_work_init_delayable(&wdt_feed_work, wdt_feed_handler);
    /* Start feeding right away */
    k_work_reschedule(&wdt_feed_work, K_NO_WAIT);
    LOG_INF("Watchdog started: %d ms", CONFIG_APP_WDT_TIMEOUT_MS);
    return 0;
}

#else /* watchdog disabled or not supported */

int watchdog_init(void)
{
    return 0;
}

#endif
