#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "app.h"
#include <zephyr/logging/log.h>
#include "app_events.h"

LOG_MODULE_REGISTER(BUTTONS, CONFIG_APP_LOG_LEVEL);

#define MIN_BUTTON_DEBOUNCE_TIME_MS CONFIG_APP_BUTTON_DEBOUNCE_DELAY_MS
#define TIME_TO_TRIGGGER_ERROR_STATE_MS 2000

struct k_work_delayable button0_reset_work;

static bool button0_is_pressed = true;

#if DT_NODE_EXISTS(DT_ALIAS(btn-user))
#define BUTTON0 DT_ALIAS(btn-user)
#elif DT_NODE_EXISTS(DT_ALIAS(button0))
#define BUTTON0 DT_ALIAS(button0)
#elif DT_NODE_EXISTS(DT_NODELABEL(btn-user))
#define BUTTON0 DT_NODELABEL(btn-user)
#elif DT_NODE_EXISTS(DT_NODELABEL(button0))
#define BUTTON0 DT_NODELABEL(button0)
#else
#define BUTTON0 DT_INVALID_NODE
#endif

#if DT_NODE_EXISTS(BUTTON0)
#define BUTTON0_DEV DT_PHANDLE(BUTTON0, gpios)
#define BUTTON0_PIN DT_PHA(BUTTON0, gpios, pin)
#define BUTTON0_FLAGS DT_PHA(BUTTON0, gpios, flags)

static const struct device *button0_dev = DEVICE_DT_GET(BUTTON0_DEV);
// static void adv_update_work_handler(struct k_work *work)
// {
// 	advertising_update();
// }
// static K_WORK_DEFINE(adv_update_work, adv_update_work_handler);

static void button0_cb(const struct device *port, struct gpio_callback *cb,
                       gpio_port_pins_t pins)
{
    static uint32_t prev_pres = 0;
	//static bool is_pressed = true;
	uint32_t elapsed_time = 0;

    // Button is configured to interrupt on both edges, so this callback will be called
    // twice for each press. We determine whether it was a press or release by tracking
	if(button0_is_pressed)
	{
	// Button was pressed down now register the time and set the flag
		button0_is_pressed = false;
		prev_pres = k_uptime_get_32();// get uptime in milliseconds

		//Just in case the either press or release event is missed or not registered
		//This will reset the button state after 4 seconds
		k_work_reschedule(&button0_reset_work, K_SECONDS(4));
	}
	else{
        // Button was released now calculate the elapsed time and if it is more than debounce time
        // consider it as a valid press
		button0_is_pressed = true;

		elapsed_time = k_uptime_get_32() - prev_pres;

		if(elapsed_time > TIME_TO_TRIGGGER_ERROR_STATE_MS) //use for demoing error state led pattern. This has no functional purpose
		{
			app_evt_raise(APP_ERR_BUTTON);
		}
		else if(elapsed_time > MIN_BUTTON_DEBOUNCE_TIME_MS) //debouncing
		{
			// toggle the ble enable/disable flag
			en_ble = !en_ble;
			LOG_INF("Advertising toggle button: %s\n", en_ble?"ENABLED":"DISABLED");

			// only if sampling work is battery sample task is not running
			// Restart sampling work immediately
			if(k_work_delayable_busy_get(&battery_voltage_work) == 0) 
			{
				k_work_reschedule(&battery_voltage_work, K_NO_WAIT);
			}
        }
		else{}
	}
}
#endif /* BUTTON0 */

static void button0_reset(struct k_work *work)
{
	button0_is_pressed = true;
	//printk("new threshold value is... %d\n",threshold_do_f16 );
}

int button_init()
{
	int err;
#if DT_NODE_EXISTS(BUTTON0)

	err = gpio_pin_configure(button0_dev, BUTTON0_PIN,
				 BUTTON0_FLAGS | GPIO_INPUT);
	if (err) {
		return err;
	}

	static struct gpio_callback gpio_cb0;

	err = gpio_pin_interrupt_configure(button0_dev, BUTTON0_PIN,
					   GPIO_INT_EDGE_BOTH);
	if (err) {
		return err;
	}

	gpio_init_callback(&gpio_cb0, button0_cb, BIT(BUTTON0_PIN));
	gpio_add_callback(button0_dev, &gpio_cb0);

#else
	LOG_ERR("WARNING: Buttons not supported on this board.\n");
#endif

    k_work_init_delayable(&button0_reset_work, button0_reset);

	return 0;
}