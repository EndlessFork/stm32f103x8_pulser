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
#include "switches.h"
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
static int switches = -1;

// arduino style
void setup(void);
void loop(void);

const int ticker_us[] = {
   0, // auto
   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, // 1..A
   20, // B
   30, // C
   50, // D
   100, // E
   300  // F
};



void setup(void) {
   rcc_clock_setup_in_hse_8mhz_out_72mhz();

   setup_delay();
   setup_led();
   setup_switches();
   usart_init();
//   setup_usbcdc();
   select_pattern(0x0f&(switches = read_switches()));
   setup_ticker();
//   ticker_start(ticker_us[switches >> 4]);
//   delay(60000); // 1 minute startup delay!
   ticker_start(2*8800); // pattern 7
//   ticker_start(1500*72); // pattern 1
}

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

void setup_1ms_pulsing(void);
void setup_1ms_pulsing(void) {
   // output pulses on TIMER3_CHANNEL1 (PA6)

   rcc_periph_clock_enable(RCC_TIM3);

   timer_reset(TIM3);

   timer_disable_preload(TIM3);
   timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT,
                  TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);

   timer_set_prescaler(TIM3, 71); // prescaler divides by (x+1) -> 1MHz
   timer_continuous_mode(TIM3);
   timer_set_counter(TIM3, 0);
   timer_set_period(TIM3, 999); // timer divides by (x+1) -> 1KHz

   timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
   //timer_set_oc_fast_mode(TIM3, TIM_OC1);
   timer_enable_oc_output(TIM3, TIM_OC1);
   timer_set_oc_idle_state_unset(TIM3, TIM_OC1);
   timer_set_oc_value(TIM3, TIM_OC1, 1);


   timer_generate_event(TIM3, 1); // set UG bit for initial update

   timer_enable_counter(TIM3);

   // adjust output channel
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6);
}

/*
void print_menu(void);
void print_menu(void) {
   float f;
   int u;
   printf("Pattern generator V0.1\n");
   printf("======================\n");
   printf("p: select pattern (0..%d)\n", get_max_pattern_number());
   printf("f: select frame-frequency (%.1fHz)\n", (float)F_CPU/(pattern_repeat*timer_ticks));
   printf("F: select timer-frequency (%lu)\n", F_CPU/timer_ticks);
   printf("P: select frame-period (%.1fms)\n", 1000.0*pattern_repeat*timer_ticks/F_CPU);
   printf("\n");
   printf("selected curve: %d\n", get_pattern_number());
   printf("\n");
   printf("selection:?\n");
   switch (usbcdc_getchar()) {
      case 'p' : printf("\nWhich pattern? ");
                 if (scanf("%d\n", &u)) {
                    ticker_stop();
                    select_pattern(u);
                    ticker_start(timer_ticks);
                 }
                 break;
      case 'f' : printf("\nDesired Frame frequency? ");
                 if (scanf("%f\n", &f)) {
                    ticker_stop();
                    ticker_start((unsigned long int)(F_CPU/(f * pattern_repeat)));
                 }
                 break;
      case 'F' : printf("\nDesired Timer Freq? ");
                 if (scanf("%f\n", &f)) {
                    ticker_stop();
                    ticker_start((unsigned long int)(F_CPU/f));
                 }
                 break;
      case 'P' : printf("\nselect frame-period: ");
                 if (scanf("%f\n", &f)) {
                    ticker_stop();
                    ticker_start((unsigned long int)((f*F_CPU)/(1000.0*pattern_repeat)));
                 }
                 break;
   }
   printf("\n\n");
}

*/
void loop(void) {
   int t;
   char c;

   delay(456);
  // idle();
   t = read_switches();
   if (t != switches) {
      ticker_stop();
      select_pattern(0x0f & (switches = t));
      printf("Selected pattern %d\n", switches);
      ticker_start(ticker_us[switches >> 4]);
   } else printf("time is now %lu.%03d\n", systick_millis / 1000, systick_millis % 1000);
   gpio_toggle(GPIOC, GPIO13);
/*   if (usbcdc_can_getchar()) {
      c = usbcdc_getchar();
      printf("got >%d(%c)<\n", c, c);
   }*/
}

int main(void) {
   setup();
//   setup_1ms_pulsing();
   while(1)
      loop();
   return 0;
}

