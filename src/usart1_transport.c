#include "usart1_transport.h"

#include <errno.h>

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <stm32l476xx.h>

#define HSI16_HZ 16000000U
#define LSE_HZ DT_PROP_OR(DT_NODELABEL(clk_lse), clock_frequency, 0U)
/* Zero means HSE is not described/enabled for this board; selecting it at
 * runtime is then rejected rather than guessed.
 */
#define HSE_HZ DT_PROP_OR(DT_NODELABEL(clk_hse), clock_frequency, 0U)
#define USART1_ERROR_FLAGS (USART_ISR_PE | USART_ISR_FE | USART_ISR_NE | USART_ISR_ORE)
#define USART1_ERROR_CLEARS (USART_ICR_PECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_ORECF)

static struct byte_ring rx_ring;
static struct usart1_error_counters errors;
static struct usart1_clock_info clock_info;

static const uint32_t msi_frequency_hz[] = {
	100000U, 200000U, 400000U, 800000U, 1000000U, 2000000U,
	4000000U, 8000000U, 16000000U, 24000000U, 32000000U, 48000000U,
};

static uint32_t msi_hz(void)
{
	uint32_t range;

	if ((RCC->CR & RCC_CR_MSIRGSEL) != 0U) {
		range = (RCC->CR & RCC_CR_MSIRANGE) >> RCC_CR_MSIRANGE_Pos;
	} else {
		range = (RCC->CSR & RCC_CSR_MSISRANGE) >> RCC_CSR_MSISRANGE_Pos;
	}
	return range < ARRAY_SIZE(msi_frequency_hz) ? msi_frequency_hz[range] : 0U;
}

static uint32_t pll_r_hz(void)
{
	uint32_t pll = RCC->PLLCFGR;
	uint32_t source;
	uint32_t m = ((pll & RCC_PLLCFGR_PLLM) >> RCC_PLLCFGR_PLLM_Pos) + 1U;
	uint32_t n = (pll & RCC_PLLCFGR_PLLN) >> RCC_PLLCFGR_PLLN_Pos;
	uint32_t r = 2U * (((pll & RCC_PLLCFGR_PLLR) >> RCC_PLLCFGR_PLLR_Pos) + 1U);

	switch (pll & RCC_PLLCFGR_PLLSRC) {
	case RCC_PLLCFGR_PLLSRC_MSI:
		source = msi_hz();
		break;
	case RCC_PLLCFGR_PLLSRC_HSI:
		source = HSI16_HZ;
		break;
	case RCC_PLLCFGR_PLLSRC_HSE:
		source = HSE_HZ;
		break;
	default:
		return 0U;
	}
	if (source == 0U || n == 0U || (pll & RCC_PLLCFGR_PLLREN) == 0U) {
		return 0U;
	}
	return (uint32_t)(((uint64_t)source * n) / ((uint64_t)m * r));
}

static uint32_t sysclk_hz(void)
{
	switch (RCC->CFGR & RCC_CFGR_SWS) {
	case RCC_CFGR_SWS_MSI:
		return msi_hz();
	case RCC_CFGR_SWS_HSI:
		return HSI16_HZ;
	case RCC_CFGR_SWS_HSE:
		return HSE_HZ;
	case RCC_CFGR_SWS_PLL:
		return pll_r_hz();
	default:
		return 0U;
	}
}

static uint32_t ahb_divisor(void)
{
	static const uint16_t divisors[] = {
		1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U,
		2U, 4U, 8U, 16U, 64U, 128U, 256U, 512U,
	};
	return divisors[(RCC->CFGR & RCC_CFGR_HPRE) >> RCC_CFGR_HPRE_Pos];
}

static uint32_t apb2_divisor(void)
{
	static const uint8_t divisors[] = {1U, 1U, 1U, 1U, 2U, 4U, 8U, 16U};
	return divisors[(RCC->CFGR & RCC_CFGR_PPRE2) >> RCC_CFGR_PPRE2_Pos];
}

static int derive_clock(struct usart1_clock_info *info)
{
	uint32_t selector = (RCC->CCIPR & RCC_CCIPR_USART1SEL) >> RCC_CCIPR_USART1SEL_Pos;
	uint32_t sysclk = sysclk_hz();
	uint32_t pclk2;

	/* Zephyr's board value describes HCLK; verify it against the live SYSCLK
	 * source and AHB prescaler before deriving PCLK2.
	 */
	if (sysclk == 0U || sysclk / ahb_divisor() != CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) {
		return -EINVAL;
	}
	pclk2 = sysclk / ahb_divisor() / apb2_divisor();

	info->sysclk_hz = sysclk;
	info->pclk2_hz = pclk2;
	info->usart1_clock_source = (uint8_t)selector;
	info->apb2_divisor = (uint8_t)apb2_divisor();
	switch (selector) {
	case 0U:
		info->peripheral_hz = pclk2;
		break;
	case 1U:
		info->peripheral_hz = sysclk;
		break;
	case 2U:
		info->peripheral_hz = HSI16_HZ;
		break;
	case 3U:
		info->peripheral_hz = LSE_HZ;
		break;
	default:
		return -EINVAL;
	}
	info->brr = (info->peripheral_hz + (USART1_BAUD_RATE / 2U)) / USART1_BAUD_RATE;
	if (info->brr < 16U || info->brr > UINT16_MAX) {
		return -ERANGE;
	}
	info->nominal_baud = info->peripheral_hz / info->brr;
	return 0;
}

static void usart1_isr(const void *unused)
{
	uint32_t status = USART1->ISR;
	uint32_t clear = 0U;
	uint8_t byte;

	ARG_UNUSED(unused);
	if ((status & USART_ISR_PE) != 0U) {
		saturating_increment_u32(&errors.parity);
		clear |= USART_ICR_PECF;
	}
	if ((status & USART_ISR_FE) != 0U) {
		saturating_increment_u32(&errors.framing);
		clear |= USART_ICR_FECF;
	}
	if ((status & USART_ISR_NE) != 0U) {
		saturating_increment_u32(&errors.noise);
		clear |= USART_ICR_NCF;
	}
	if ((status & USART_ISR_ORE) != 0U) {
		saturating_increment_u32(&errors.overrun);
		clear |= USART_ICR_ORECF;
	}
	if ((status & USART_ISR_RXNE) != 0U) {
		byte = (uint8_t)USART1->RDR;
		if (!byte_ring_push(&rx_ring, byte)) {
			saturating_increment_u32(&errors.ring_overflow);
		}
	}
	if (clear != 0U) {
		USART1->ICR = clear;
	}
}

int usart1_transport_init(void)
{
	int result;
	uint32_t ignored;

	byte_ring_init(&rx_ring);
	errors = (struct usart1_error_counters){0};

	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
	(void)RCC->APB2ENR;

	/* PA9/PA10: alternate-function mode, AF7; PA10 has a weak pull-up. */
	GPIOA->MODER = (GPIOA->MODER & ~((3UL << (9U * 2U)) | (3UL << (10U * 2U)))) |
		       (2UL << (9U * 2U)) | (2UL << (10U * 2U));
	GPIOA->OTYPER &= ~((1UL << 9U) | (1UL << 10U));
	GPIOA->OSPEEDR |= (3UL << (9U * 2U)) | (3UL << (10U * 2U));
	GPIOA->PUPDR = (GPIOA->PUPDR & ~((3UL << (9U * 2U)) | (3UL << (10U * 2U)))) |
		       (1UL << (10U * 2U));
	GPIOA->AFR[1] = (GPIOA->AFR[1] & ~((0xFUL << 4U) | (0xFUL << 8U))) |
			(7UL << 4U) | (7UL << 8U);

	USART1->CR1 = 0U;
	USART1->CR2 = 0U;
	USART1->CR3 = 0U;
	result = derive_clock(&clock_info);
	if (result < 0) {
		return result;
	}
	USART1->BRR = clock_info.brr;

	/* Drain stale data and clear all receive/error state before IRQ enable. */
	ignored = USART1->RDR;
	ARG_UNUSED(ignored);
	USART1->ICR = USART1_ERROR_CLEARS;
	IRQ_CONNECT(USART1_IRQn, 1, usart1_isr, NULL, 0);
	irq_enable(USART1_IRQn);
	USART1->CR3 = USART_CR3_EIE;
	USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE |
		       USART_CR1_PEIE | USART_CR1_UE;
	return 0;
}

int usart1_transport_write(const uint8_t *data, size_t length)
{
	if (data == NULL && length != 0U) {
		return -EINVAL;
	}
	for (size_t i = 0U; i < length; ++i) {
		while ((USART1->ISR & USART_ISR_TXE) == 0U) {
		}
		USART1->TDR = data[i];
	}
	if (length != 0U) {
		while ((USART1->ISR & USART_ISR_TC) == 0U) {
		}
	}
	return 0;
}

bool usart1_transport_read(uint8_t *byte)
{
	return byte_ring_pop(&rx_ring, byte);
}

size_t usart1_transport_rx_count(void)
{
	return byte_ring_count(&rx_ring);
}

void usart1_transport_get_errors(struct usart1_error_counters *snapshot)
{
	unsigned int key;

	if (snapshot == NULL) {
		return;
	}
	key = irq_lock();
	*snapshot = errors;
	irq_unlock(key);
}

void usart1_transport_get_clock(struct usart1_clock_info *snapshot)
{
	if (snapshot != NULL) {
		*snapshot = clock_info;
	}
}
