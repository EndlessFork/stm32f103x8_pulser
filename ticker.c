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

#include "ticker.h"

#include "pattern.h"
#include "config.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

// for memset
#include <string.h>


/* OUTPUT WAVEPATTERN
 *
 * we use timer2 to generate basic timing
 * and a dma channels to transfer data to OUTPUT_PORT bits 0..7
 * also use half-transfer interrupt to calculate (over) next block
 */

// after how many bytes the pattern will repeat
uint32_t pattern_repeat = 0;
static uint32_t pattern_offset = 0; // offset to be used for next call
uint32_t period_us;

static void timer_setup(int _timer_ticks) {
   int presc=1;

   // obtain 'good' values for prescaler and reload....
   if (_timer_ticks > 36000) {
      presc = 9;
      _timer_ticks /= 9;
      while (_timer_ticks > 65536) {
         presc *=2;
         _timer_ticks /= 2;
      }
   }

   rcc_periph_clock_enable(TIMER_RCC);

   timer_reset(TIMER);

   timer_disable_preload(TIMER);
   timer_set_mode(TIMER, TIM_CR1_CKD_CK_INT,
                  TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

   timer_set_prescaler(TIMER, presc-1); // prescaler divides by (x+1)
   timer_continuous_mode(TIMER);
   timer_set_counter(TIMER, 0);
   timer_set_period(TIMER, _timer_ticks-1); // timer divides by (x+1)
   timer_set_dma_on_update_event(TIMER);

   // Update DMA Request enable (no timer IRQ's...)
   timer_enable_irq(TIMER, TIM_DIER_UDE);

   timer_generate_event(TIMER, 1); // set UG bit
}


static void dma_setup(void) {
   rcc_periph_clock_enable(DMA_RCC);

   // Channel 2 is triggered by timer2 overflow
   dma_channel_reset(DMA_UNIT, DMA_CHANNEL);
   dma_disable_channel(DMA_UNIT, DMA_CHANNEL);
   dma_set_priority(DMA_UNIT, DMA_CHANNEL, DMA_CCR_PL_VERY_HIGH);
   dma_set_memory_size(DMA_UNIT, DMA_CHANNEL, DMA_CCR_MSIZE_8BIT);
   dma_set_peripheral_size(DMA_UNIT, DMA_CHANNEL, DMA_CCR_PSIZE_32BIT);
   dma_enable_memory_increment_mode(DMA_UNIT, DMA_CHANNEL);
   dma_disable_peripheral_increment_mode(DMA_UNIT, DMA_CHANNEL);
   dma_enable_circular_mode(DMA_UNIT, DMA_CHANNEL);
   dma_set_read_from_memory(DMA_UNIT, DMA_CHANNEL);
   dma_set_peripheral_address(DMA_UNIT, DMA_CHANNEL, (uint32_t) & GPIOA_ODR);
   dma_set_memory_address(DMA_UNIT, DMA_CHANNEL, (uint32_t) & output_buffer);
   dma_set_number_of_data(DMA_UNIT, DMA_CHANNEL, 2*HALF_BUFFER_SIZE);

   dma_enable_half_transfer_interrupt(DMA_UNIT, DMA_CHANNEL);
   dma_enable_transfer_complete_interrupt(DMA_UNIT, DMA_CHANNEL);
   nvic_enable_irq(DMA_NVIC_IRQ);
   // no enable_channel yet!
}

void DMA_IRQ(void) {
   if (dma_get_interrupt_flag(DMA_UNIT, DMA_CHANNEL, DMA_TCIF)) {
      // transfer complete interrupt -> recalculate second block + re-enable dma
      dma_disable_channel(DMA_UNIT, DMA_CHANNEL);
      dma_enable_channel(DMA_UNIT, DMA_CHANNEL);
      dma_clear_interrupt_flags(DMA_UNIT, DMA_CHANNEL, DMA_TCIF | DMA_GIF);
      pattern_offset = fill_buffer(output_buffer[1], HALF_BUFFER_SIZE, pattern_offset);
   } else if (dma_get_interrupt_flag(DMA_UNIT, DMA_CHANNEL, DMA_HTIF)) {
      // half transfer interrupt -> re-calculate first block
      dma_clear_interrupt_flags(DMA_UNIT, DMA_CHANNEL, DMA_HTIF | DMA_GIF);
      pattern_offset = fill_buffer(output_buffer[0], HALF_BUFFER_SIZE, pattern_offset);
   };
}

void setup_ticker(void) {
   rcc_periph_clock_enable(OUTPUT_RCC);

   // PX0..7 -> Pulses via DMA
   gpio_set_mode(OUTPUT_PORT,
                 GPIO_MODE_OUTPUT_10_MHZ,
                 GPIO_CNF_OUTPUT_PUSHPULL,
                 OUTPUT_PINS);
   gpio_clear(OUTPUT_PORT, OUTPUT_PINS);
}

uint32_t timer_ticks;
void ticker_start(int requested_ticker) {
   // figure out values from currently active pattern...
   pattern_repeat = fill_buffer(NULL, 0, 0);
   pattern_offset = 0;

   if (!pattern_repeat) {
      // can stretch, use 10us as time base and one buffer for pattern_repeat
//      pattern_repeat = HALF_BUFFER_SIZE;
      pattern_repeat = MIN_REPEAT;
   }

   // find timer freq so that period in usecs is just shorter than MAX_pattern_repeat_USECS
   timer_ticks = MAX_REPEAT_USECS * (F_CPU/1000000UL) / pattern_repeat;

   // second approch: use 1-2-5 scaling of basic timer ticks (72)
   period_us = pattern_repeat; // start with 1usec per tick
   timer_ticks = (F_CPU / 1000000UL);
   while (period_us * 10UL < MAX_REPEAT_USECS) {
      period_us *= 10;
      timer_ticks *= 10;
   }
   if (period_us * 5UL < MAX_REPEAT_USECS) {
      period_us *= 5;
      timer_ticks *= 5;
   }
   if (period_us * 3UL < MAX_REPEAT_USECS) {
      period_us *= 3;
      timer_ticks *= 3;
   }
   if (period_us * 2UL < MAX_REPEAT_USECS) {
      period_us *= 2;
      timer_ticks *= 2;
   }
   if (period_us + (period_us >> 1) < MAX_REPEAT_USECS) {
      period_us += period_us >> 1;
      timer_ticks += timer_ticks >> 1;
   }

   // honor requested value, period_us follows then and may be outside wanted limits.
   if (requested_ticker)
      timer_ticks = requested_ticker;

   // calculate and store repeat frequency in usecs
   period_us = (timer_ticks*pattern_repeat) / (F_CPU/1000000UL);

   /* first round is always completely empty */
   memset(output_buffer, 0, BUFFER_SIZE);

   pattern_offset = 0;
   dma_setup();
   timer_setup(timer_ticks);
   dma_enable_channel(DMA_UNIT, DMA_CHANNEL);
   timer_enable_counter(TIMER);
}

void ticker_stop(void) {
   timer_disable_counter(TIMER);
   dma_disable_channel(DMA_UNIT, DMA_CHANNEL);
}

