#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define USER_LED_NODE DT_ALIAS(led0)
#define LED_PERIOD_MS 500

int main(void)
{
	LOG_INF("stm32l476-zephyr-mqtt foundation starting");

#if !DT_NODE_HAS_STATUS(USER_LED_NODE, okay)
	LOG_ERR("user LED devicetree alias 'led0' is missing or disabled");
	return 0;
#else
	static const struct gpio_dt_spec user_led =
		GPIO_DT_SPEC_GET(USER_LED_NODE, gpios);
	int ret;

	if (!gpio_is_ready_dt(&user_led)) {
		LOG_ERR("user LED GPIO device is not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&user_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("failed to configure user LED GPIO (%d)", ret);
		return 0;
	}

	LOG_INF("user LED toggling every %d ms", LED_PERIOD_MS);
	for (;;) {
		ret = gpio_pin_toggle_dt(&user_led);
		if (ret < 0) {
			LOG_ERR("failed to toggle user LED GPIO (%d)", ret);
			return 0;
		}
		k_msleep(LED_PERIOD_MS);
	}
#endif
}
