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
#include "delay.h"

#include <libopencm3/cm3/systick.h>


/*
 * delay via systick (1KHz)
 */
volatile uint32_t systick_millis = 0;
volatile uint32_t systick_delay_ctr = 0;
volatile uint8_t systick_usbcdc_timeout_ctr = 0;

void sys_tick_handler(void);
void sys_tick_handler(void) {
   systick_millis++;
   if (systick_delay_ctr)
      systick_delay_ctr--;
   if (systick_usbcdc_timeout_ctr)
      systick_usbcdc_timeout_ctr--;
}

void delay(uint32_t ms) {
   uint32_t target = systick_millis + ms;
   while (systick_millis != target) {
      asm("nop");
   }
}

void setup_delay(void) {
   systick_set_reload(72000);
   systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
   systick_counter_enable();
   systick_interrupt_enable();
}



