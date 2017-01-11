// vim: set fileencoding=utf-8 :
/*****************************************************************************
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 *  der GNU General Public License, wie von der Free Software Foundation,
 *  Version 2 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 *  veröffentlichten Version, weiterverbreiten und/oder modifizieren.
 *
 *  Dieses Programm wird in der Hoffnung, dass es nützlich sein wird, aber
 *  OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
 *  Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 *  Siehe die GNU General Public License für weitere Details.
 *
 *  Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 *  Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.
 *
 *  Copyright (C) 2016 Enrico Faulhaber <enrico.faulhaber@frm2.tum.de>
 *
 *****************************************************************************/

#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>


#define OUTPUT_PORT GPIOA
#define OUTPUT_PINS (GPIO0|GPIO1|GPIO2|GPIO3|GPIO4|GPIO5|GPIO6|GPIO7)
#define OUTPUT_RCC RCC_GPIOA

#define TIMER TIM2
#define TIMER_RCC RCC_TIM2

#define DMA_UNIT     DMA1
#define DMA_RCC      RCC_DMA1
#define DMA_CHANNEL  DMA_CHANNEL2
#define DMA_IRQ      dma1_channel2_isr
#define DMA_NVIC_IRQ NVIC_DMA1_CHANNEL2_IRQ

#define LED_PORT GPIOC
#define LED_PIN  GPIO13
#define LED_RCC  RCC_GPIOC

// hardcoded double buffering uses TWO of these buffers...
#define HALF_BUFFER_SIZE 8192
#define BUFFER_SIZE (2*HALF_BUFFER_SIZE)

#define F_CPU 72000000
#define MAX_REPEAT_USECS 52428
//#define MAX_REPEAT_USECS 40000
#define MIN_REPEAT 256

extern uint8_t output_buffer[2][HALF_BUFFER_SIZE];

#endif
