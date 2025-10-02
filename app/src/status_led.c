#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(STATUS_LED, CONFIG_APP_LOG_LEVEL);
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include "app.h"
#include "app_events.h"

struct k_work_delayable led_idle_work;
struct k_work_delayable led_sample_work;
struct k_work_delayable led_error_work;

/* Locate led0 as alias or label by that name for paired status*/
#if DT_NODE_EXISTS(DT_ALIAS(led0))
#define LED0 DT_ALIAS(led0)
#elif DT_NODE_EXISTS(DT_ALIAS(led-status))
#define LED0 DT_ALIAS(led-status)
#elif DT_NODE_EXISTS(DT_NODELABEL(led0))
#define LED0 DT_NODELABEL(led0)
#elif DT_NODE_EXISTS(DT_NODELABEL(led-status))
#define LED0 DT_NODELABEL(led-status)
#else
#define LED0 DT_INVALID_NODE
#endif

K_MUTEX_DEFINE(led_mutex);

#if DT_NODE_EXISTS(LED0)
#define LED0_DEV DT_PHANDLE(LED0, gpios)
#define LED0_PIN DT_PHA(LED0, gpios, pin)
#define LED0_FLAGS DT_PHA(LED0, gpios, flags)

static const struct device *led0_dev = DEVICE_DT_GET(LED0_DEV);
#endif /* LED0 */

//work handlers for different LED blink patterns 
//Blink patterns indicate different operation modes
//Idle mode - single short blink every sample_interval_ms/2
//Sample mode - double short blink every sample_interval_ms/2
//Error mode - fast continuous blink

//This work is independent of sample and error work, so it can be scheduled anytime
static void idle_work(struct k_work *work)
{
    //Lock the mutex to prevent simultaneous access to the LED from different work items
	k_mutex_lock(&led_mutex, K_FOREVER);
    //blink the LED once
    gpio_pin_set(led0_dev, LED0_PIN, 1);
    k_sleep(K_MSEC(50));
    gpio_pin_set(led0_dev, LED0_PIN, 0);
    k_mutex_unlock(&led_mutex);// release the mutex
    
    //reschedule the idle work to run again after sample_interval_ms/2
    k_work_reschedule(&led_idle_work, K_MSEC(sample_interval_ms/2));
}

//This work is scheduled when a sample is taken
//It blinks the LED twice quickly to indicate a sample event. It does not reschedule itself
static void sample_work(struct k_work *work)
{
    //Lock the mutex to prevent simultaneous access to the LED from different work items
    k_mutex_lock(&led_mutex, K_FOREVER);
	gpio_pin_set(led0_dev, LED0_PIN, 1);
    k_sleep(K_MSEC(50));
    gpio_pin_set(led0_dev, LED0_PIN, 0);
    k_sleep(K_MSEC(50));
    gpio_pin_set(led0_dev, LED0_PIN, 1);
    k_sleep(K_MSEC(50));
    gpio_pin_set(led0_dev, LED0_PIN, 0);
    k_mutex_unlock(&led_mutex);// release the mutex
	//printk("new threshold value is... %d\n",threshold_do_f16 );
    
}

//This work is scheduled when an error condition is detected
//It blinks the LED continuously at a fast rate to indicate an error condition. It reschedules itself
//It also cancels the other two work items to prevent interference as error indication is critical unless reset
static void error_work(struct k_work *work)
{
	gpio_pin_toggle(led0_dev, LED0_PIN);
	//printk("new threshold value is... %d\n",threshold_do_f16 );
    k_work_reschedule(&led_error_work, K_MSEC(100));
}

int led_init(void)
{
	int err = 0;
    k_work_init_delayable(&led_idle_work, idle_work);
    k_work_init_delayable(&led_sample_work, sample_work);
    k_work_init_delayable(&led_error_work, error_work);

#if DT_NODE_EXISTS(LED0)
	if(!device_is_ready(led0_dev)) 
	{
		return -ENODEV;
	}
    
    // Set the LED pin as output, initially off
    // derive the flags based on active low or active high configuration
    gpio_flags_t flags = LED0_FLAGS | GPIO_OUTPUT_INACTIVE;
    if (IS_ENABLED(CONFIG_APP_LED_ACTIVE_LOW)) 
    { 
        flags |= GPIO_ACTIVE_LOW; 
    }
    else
    {
        flags |= GPIO_ACTIVE_HIGH;
    }

    err = gpio_pin_configure(led0_dev, LED0_PIN, flags);
    if (err) {
        return err;
    }

    k_work_reschedule(&led_idle_work, K_NO_WAIT); //start the idle blink work
#endif
return err;
}