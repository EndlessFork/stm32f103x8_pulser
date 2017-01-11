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

#include "switches.h"
#include "config.h"

#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>


void setup_switches(void) {
   rcc_periph_clock_enable(SWITCHES_RCC);
   gpio_set_mode(SWITCHES_PORT, GPIO_MODE_INPUT, 
                 GPIO_CNF_INPUT_PULL_UPDOWN, SWITCHES_PINS);
   gpio_set(SWITCHES_PORT, SWITCHES_PINS); // 'output' a 1 to enable pull-up
}

int read_switches(void) {
   uint16_t mask = SWITCHES_PINS;
   uint16_t res = gpio_get(SWITCHES_PORT, SWITCHES_PINS);
   if (!mask)
      return 0;
   while (!(mask & 1)) {
      mask >>= 1;
      res >>= 1;
   }
   // fix wiring
   res = (res &0xAA) | ((res &0x44) >> 2) | ((res &0x11) <<2);

   res = 7; //
//   res = 15; /*********************************************/
//   res = 1; /*********************************************/
   return (int) res;
}

