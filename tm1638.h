/*

 support for tm1638 based display/keypad

*/

#ifndef _TM1638_H_
#define _TM1638_H_

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#ifndef TM1638_PORT
#define TM1638_PORT GPIOB
#endif

#ifndef TM1638_CLK_PIN
#define TM1638_CLK_PIN GPIO13
#endif

#ifndef TM1638_DIO_PIN
#define TM1638_DIO_PIN GPIO14
#endif

#ifndef TM1638_STB_PIN
#define TM1638_STB_PIN GPIO12
#endif


void tm1638_init(void);
void tm1638_set_brightness(uint8_t level_0_to_8);
void tm1638_set_digit(uint8_t pos_0_to_7, char c);
void tm1638_set_led(uint8_t pos_0_to_7, uint8_t color_0_to_3);
void tm1638_put_string(uint8_t pos_0_to_7, char*);
void tm1638_clear(void);
uint32_t tm1638_read_keys(void);

#endif
