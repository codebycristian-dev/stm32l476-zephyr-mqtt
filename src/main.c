#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <string.h>

#include "usart1_transport.h"
#include "espat_response.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

#define USER_LED_NODE DT_ALIAS(led0)
#define LED_PERIOD_MS 500
#define LOOPBACK_TIMEOUT_MS 1000
#define ESPAT_AT_TIMEOUT_MS 1000

#if CONFIG_APP_ESPAT_AT_DIAGNOSTIC
static void run_espat_at_diagnostic(void)
{
	static const uint8_t command[] = {'A', 'T', '\r', '\n'};
	struct espat_response response;
	int64_t deadline;
	uint8_t byte;

	espat_response_init(&response);
	LOG_INF("ESP-AT diagnostic: sending AT\\r\\n on PA9; bounded response on PA10");
	if (usart1_transport_write(command, sizeof(command)) < 0) {
		LOG_ERR("ESP-AT diagnostic: command transmit rejected");
		return;
	}
	deadline = k_uptime_get() + ESPAT_AT_TIMEOUT_MS;
	while (!espat_response_complete(&response) && !response.overflow &&
	       k_uptime_get() < deadline) {
		if (usart1_transport_read(&byte)) {
			espat_response_feed(&response, byte);
		} else {
			k_msleep(1);
		}
	}
	espat_response_finish(&response);
	LOG_INF("ESP-AT diagnostic: result=%s bytes=%u/%u echo=%u urc=%u prompts=%u overflow=%u timeout=%u",
		response.final == ESPAT_FINAL_OK ? "OK" :
		response.final == ESPAT_FINAL_ERROR ? "ERROR" : "NONE",
		(unsigned int)response.length, ESPAT_RESPONSE_CAPACITY,
		response.echo_count, response.unsolicited_count, response.prompt_count,
		response.overflow, response.final == ESPAT_FINAL_NONE);
}
#endif

#if CONFIG_APP_USART1_LOOPBACK
static int run_loopback_case(const char *name, const uint8_t *payload, size_t length)
{
	uint8_t received[USART1_RX_CAPACITY];
	size_t count = 0U;
	int64_t deadline = k_uptime_get() + LOOPBACK_TIMEOUT_MS;

	if (length > sizeof(received) || usart1_transport_write(payload, length) < 0) {
		LOG_ERR("USART1 loopback %s: transmit rejected", name);
		return -1;
	}
	while (count < length && k_uptime_get() < deadline) {
		if (usart1_transport_read(&received[count])) {
			count++;
		} else {
			k_msleep(1);
		}
	}
	if (count != length || memcmp(payload, received, length) != 0) {
		LOG_ERR("USART1 loopback %s: mismatch (%u/%u bytes)", name,
			(unsigned int)count, (unsigned int)length);
		return -1;
	}
	LOG_INF("USART1 loopback %s: PASS (%u bytes)", name, (unsigned int)length);
	return 0;
}

static int run_physical_loopback(void)
{
	static const uint8_t short_payload[] = {'L', '4', '7', '6', '!'};
	static const uint8_t binary_payload[] = {0x00, 0x01, 0x7f, 0x80, 0xfe, 0xff, '\r', '\n'};
	static const uint8_t wrap_payload[56] = {
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
		0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
		0x58, 0x59, 0x5a, 0xa0, 0xa1, 0xa2, 0xa3, 0xa4,
		0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac,
	};
	struct usart1_error_counters errors;

	LOG_INF("USART1 physical loopback mode: PA9 must be connected to PA10");
	if (run_loopback_case("short", short_payload, sizeof(short_payload)) < 0 ||
	    run_loopback_case("binary", binary_payload, sizeof(binary_payload)) < 0 ||
	    run_loopback_case("ring-wrap", wrap_payload, sizeof(wrap_payload)) < 0) {
		return -1;
	}
	usart1_transport_get_errors(&errors);
	if (errors.parity != 0U || errors.framing != 0U || errors.noise != 0U ||
	    errors.overrun != 0U || errors.ring_overflow != 0U) {
		LOG_ERR("USART1 loopback errors: PE=%u FE=%u NE=%u ORE=%u ring=%u",
			(unsigned int)errors.parity, (unsigned int)errors.framing,
			(unsigned int)errors.noise, (unsigned int)errors.overrun,
			(unsigned int)errors.ring_overflow);
		return -1;
	}
	LOG_INF("USART1 physical loopback suite: PASS; all error counters zero");
	return 0;
}
#endif

int main(void)
{
	struct usart1_clock_info usart_clock;
	int usart_result;

	LOG_INF("stm32l476-zephyr-mqtt foundation starting");
	usart_result = usart1_transport_init();
	if (usart_result < 0) {
		LOG_ERR("USART1 direct-CMSIS initialization failed (%d)", usart_result);
		return 0;
	}
	usart1_transport_get_clock(&usart_clock);
	LOG_INF("USART1 ready: source=%u sysclk=%u pclk2=%u APB2div=%u peripheral=%u BRR=%u baud=%u",
		usart_clock.usart1_clock_source, usart_clock.sysclk_hz,
		usart_clock.pclk2_hz, usart_clock.apb2_divisor,
		usart_clock.peripheral_hz, usart_clock.brr, usart_clock.nominal_baud);

#if CONFIG_APP_USART1_LOOPBACK
	if (run_physical_loopback() < 0) {
		return 0;
	}
#endif

#if CONFIG_APP_ESPAT_AT_DIAGNOSTIC
	run_espat_at_diagnostic();
#endif

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
