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

#include "config.h"
#include "ticker.h"
#include "delay.h"
#include "led.h"
#include "pattern.h"
#include "stlinky.h"
//#include "usbcdc.h"
#include "usart.h"

#include <libopencm3/cm3/cortex.h>

// for printf
#include <stdio.h>
#include <errno.h>

// playing around....
void hard_fault_handler(void);
void reset_handler(void);
void hard_fault_handler(void){
   reset_handler();
}

uint8_t output_buffer[2][HALF_BUFFER_SIZE];

// arduino style
void setup(void);
void loop(void);


void setup(void) {
   rcc_clock_setup_in_hse_8mhz_out_72mhz();

   setup_delay();
   setup_led();
   usart_init();
   select_pattern(0);
   setup_ticker();
   ticker_start(0);
   ui_init();
}

void loop(void) {
   delay(200);
   ui_check_event();
}

int main(void) {
   setup();
   while(1)
      loop();
   return 0;
}

