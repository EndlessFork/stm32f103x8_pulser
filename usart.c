#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <stdio.h>
#include <errno.h>

#include "usart.h"

/******************************************************************************
 * Simple ringbuffer implementation from open-bldc's libgovernor that
 * you can find at:
 * https://github.com/open-bldc/open-bldc/tree/master/source/libgovernor
 *****************************************************************************/

int _write(int file, char *ptr, int len);

void usart_init(void)
{
	/* Enable clocks for GPIO port A (for GPIO_USART1_TX) and USART1. */
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);

	/* Setup GPIO pin GPIO_USART1_RE_TX on GPIO port A for transmit. */
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO_USART1_TX);

	/* Setup GPIO pin GPIO_USART1_RE_RX on GPIO port A for receive. */
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT,
		      GPIO_CNF_INPUT_FLOAT, GPIO_USART1_RX);

	/* Setup UART parameters. */
	usart_set_baudrate(USART1, 115200);
	usart_set_databits(USART1, 8);
	usart_set_stopbits(USART1, USART_STOPBITS_1);
	usart_set_parity(USART1, USART_PARITY_NONE);
	usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART1, USART_MODE_TX_RX);

	/* Enable USART1 Receive interrupt. */
//	USART_CR1(USART1) |= USART_CR1_RXNEIE;

	/* Enable the USART1 interrupt. */
//	nvic_enable_irq(NVIC_USART1_IRQ);

	/* Finally enable the USART. */
	usart_enable(USART1);
}

static void usart_putchar(char c) {
    while ((USART_SR(USART1) & USART_SR_TXE) == 0) {
        asm("nop");
    }
    usart_send(USART1, c);
}

int _write(int file, char *ptr, int len)
{
	int ret=0;

	if (file >= 1) {
                while(ret < len) {
                    if (*ptr) {
                        usart_putchar(*ptr++);
                        ret++;
                    } else {
                        return ret;
                    }
                }
	}

	errno = EIO;
	return -1;
}

