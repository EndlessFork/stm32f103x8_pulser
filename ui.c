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

#include <stdint.h>
#include <inttypes.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "delay.h"
#include "tm1638.h"
#include "usart.h"
#include "pattern.h"
#include "ticker.h"

#include "ui.h"

#define VERSION1 " Pulser "
#define VERSION2 "0.1-rc-1"

#define KEY_PREV 1
#define KEY_PAR1 2
#define KEY_PAR2 4
#define KEY_PAR3 8
#define KEY_PAR4 16
#define KEY_PAR5 32
#define KEY_PAR6 64
#define KEY_NEXT 128
#define LED_PAR1 1
#define LED_PAR2 2
#define LED_PAR3 3
#define LED_PAR4 4
#define LED_PAR5 5
#define LED_PAR6 6

void ui_init(void) {
   tm1638_init();
}

static uint8_t selected_pattern;
static int8_t selected_param=-1; // which param to display
static unsigned int selected_ticker_ticks; // one tick is 1/72MHz
static unsigned int selected_events; // events per second for generic pattern
static uint8_t last_key;

// patterns 0..15 have 2 params : ticker_ticks, frame_length
// pattern 16 is dynamic mode: single channel/7channel, events/channel*frame

static void ui_apply(void) {
    // event rate is f_tick / 8192 * (total_inc+extra_inc) / evt_inc
    // where f_frames_per_second = f_tick / 8192
    // f_tick = f_cpu / ticker_ticks
    // T_frame = 1/f_frames_per_second = 8192 * ticker_ticks / f_cpu
    // number_of_events_per_frame is (total_inc+extra_inc)  / evt_inc
    // longest allowed frame is 52.4288 ms
    // XXX: recalculate params for gemeric pattern
    ticker_stop();
    select_pattern(selected_pattern);
    ticker_start(selected_ticker_ticks);
}


/**************************************************
 * main ui interface, regulary called from main() *
 **************************************************/

void ui_check_event(void) {
    uint8_t key,i;
    char buff[10];
    int increment;

    memset(buff, 0, sizeof(buff));

    key = tm1638_read_keys();
    if (!key) {
        // no key pressed
        keypress_timer = 0;
        if (selected_param == -1) {
            tm1638_put_string(0, (systick_seconds & 4) ? VERSION2 : VERSION1);
        }
        return;
    }

    // a key is pressed
    if ((key == KEY_PREV) || (key == KEY_NEXT)) {
        if (last_key == key) {
            increment = 1;
            if (keypress_timer > 512) {
                increment = (keypress_timer * keypress_timer) >> (2*9);
            }
        }
   } else {
      increment = 0;
   }

   switch (selected_param) {
   case 1: // select pattern
           if ((key == KEY_PREV) && (selected_pattern > 0)) {
               selected_pattern--;
               ui_apply(); // also recalculates!
           }
           if ((key == KEY_NEXT) && (selected_pattern < get_max_pattern_number())) {
               selected_pattern++;
               ui_apply(); // also recalculates!
           }
           tm1638_put_string(0, pattern_names[selected_pattern]);
           break;
   case 2: // select timebase
           if ((key == KEY_PREV) && (selected_ticker_ticks > 72)) {
               selected_ticker_ticks = max(72, selected_ticker_ticks - increment);
               ui_apply(); // also recalculates!
           }
           if ((key == KEY_NEXT) && (selected_ticker_ticks < 65535)) {
               selected_ticker_ticks = min(65535, selected_ticker_ticks + increment);
               ui_apply(); // also recalculates!
           }
           snprintf(buff, 9, "%.2fus  ", selected_ticker_ticks / 72.0);
           tm1638_put_string(0, buff);
           break;
   case 3: // select target events per channel per second
           if (get_pattern_number() != 16) {
              tm1638_put_string(0," --  -- ");
           } else {
              if (key == KEY_PREV) {
                 selected_events = max(1, selected_events - increment);
                 ui_apply(); // also recalculates!
              }
              if (key == KEY_NEXT) {
                 selected_events = min(25600, selected_events + increment);
                 ui_apply(); // also recalculates!
              }
              snprintf(buff, 9, "%d evts", selected_events);
              tm1638_put_string(0, buff);
           }
           break;
   case 4:
   case 5:
   case 6:
   case -1:
           tm1638_put_string(0," -- -- ");
           break;
   default:
           snprintf(buff,9, "ERROR %d", selected_param);
           tm1638_put_string(0, buff);
   }

    // any 'param-select key' pressed?
    if (key & 0x7e) {
        tm1638_set_led(selected_param, 0);
    }

    // PREV & NEXT
    if (key == 0x81) {
        tm1638_set_led(selected_param, 0);
        selected_param = -1;
    }

    for (i=1; i<7; i++) {
       if (key & (1<<i)) { // Key-select for param<i> pressed
          selected_param = i;
          tm1638_set_led(selected_param, 1); // also light led
       }
    }
}

