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
 *  Copyright (C) 2016-2017 Enrico Faulhaber <enrico.faulhaber@frm2.tum.de>
 *
 *****************************************************************************/

#include <stdint.h>
#include <inttypes.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "delay.h"
#include "tm1638.h"
#include "usart.h"
#include "pattern.h"
#include "ticker.h"
#include "config.h"

#include "ui.h"

#define VERSION1 " Pulser "
#define VERSION2 "16.01.2017"

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
static int selected_timer_ticks=88; // one tick is 1/72MHz
static int selected_frame_events=1000; // events per second for generic pattern
static uint8_t last_key;

static uint32_t frames_per_hundred_s=1000; // frames per 100 s

// patterns 0..N-1 have 2 params : timer_ticks, frame_length
// pattern N is dynamic mode: single channel/7channel, events/channel*frame

static void ui_apply(void) {
    uint32_t t_frame_us;

    // event rate is f_tick / 8192 * (total_inc+extra_inc) / evt_inc
    // where f_frames_per_second = f_tick / 8192
    // f_tick = f_cpu / timer_ticks
    // T_frame = 1/f_frames_per_second = 8192 * timer_ticks / f_cpu
    // number_of_events_per_frame is (total_inc+extra_inc)  / evt_inc
    // longest allowed frame is 52.4288 ms
    // XXX: recalculate params for gemeric pattern
    do {
       t_frame_us = pattern_get_period() * selected_timer_ticks / 72;
       if (t_frame_us <= MAX_FRAME_TIME) {
           break;
       }
       selected_timer_ticks--;
    } while (1);

    if (selected_timer_ticks < 72) {
       selected_timer_ticks = 72;
       t_frame_us = pattern_get_period() * selected_timer_ticks / 72;
    }

    // calculate event_increment for requested event rate
    // event rate is f_tick / 8192 * (total_inc+extra_inc) / evt_inc
    // selected_frame_events is (total_inc + extra_inc) / evt_inc
    if (selected_frame_events < 1) {
        selected_frame_events = 1;
    }
    generic_pattern_event_increment = generic_pattern_total_increments / selected_frame_events;
    if (generic_pattern_event_increment < 510) {
        generic_pattern_event_increment = 510;
    }
    if (generic_pattern_event_increment > generic_pattern_total_increments) {
        generic_pattern_event_increment = generic_pattern_total_increments;
    }
    selected_frame_events = generic_pattern_total_increments / generic_pattern_event_increment;

    if (generic_pattern_total_increments % generic_pattern_event_increment == 0) {
        generic_pattern_extra_increment = 1;
    } else {
        generic_pattern_extra_increment = 0;
    }

        if (selected_pattern == get_max_pattern_number()) {
           printf("Total increments: %d\n", generic_pattern_total_increments);
           printf("Event increment:  %d\n", generic_pattern_event_increment);
           printf("Extra increment: %d\n", generic_pattern_extra_increment);
        }
    if ((timer_ticks != selected_timer_ticks) || (get_pattern_number() != selected_pattern)) {
        printf("ticker_stop\n");
        ticker_stop();
        pattern_select(selected_pattern);
        printf("pattern_select %d (%s)\n", selected_pattern, pattern_get_name());
        printf("ticker_start(%d)\n", selected_timer_ticks);
        ticker_start(selected_timer_ticks);
    }
}


/**************************************************
 * main ui interface, regulary called from main() *
 **************************************************/

void ui_check_event(void) {
    uint8_t key;
    char buff[10];
    int increment, i;

    memset(buff, 0, sizeof(buff));

    frames_per_hundred_s = 72e8 / (pattern_get_period() * selected_timer_ticks);

    key = tm1638_read_keys();
    if (!key) {
        // no key pressed
        keypress_timer = 0;
    } else if (!keypress_timer) {
        keypress_timer = 1;
    }

    // a key is pressed
    if ((key == KEY_PREV) || (key == KEY_NEXT)) {
        if (last_key == key) {
            increment = 1;
            if (keypress_timer > 512) {
                increment = (keypress_timer * keypress_timer ) >> (2*10);
                if (increment < 1) {
                    increment = 1;
                }
            }
        } else {
            increment = 1;
        }
        printf("Increment %d\n", increment);
   } else {
      increment = 0;
   }
   last_key = key;

   switch (selected_param) {
   case -1:
            tm1638_put_string(0, (systick_seconds & 4) ? VERSION2 : VERSION1);
            break;
   case 1: // select pattern
           if ((key == KEY_PREV) && (selected_pattern > 0)) {
               selected_pattern--;
               ui_apply(); // also recalculates!
           }
           if ((key == KEY_NEXT) && (selected_pattern < get_max_pattern_number())) {
               selected_pattern++;
               ui_apply(); // also recalculates!
           }
           if (key == KEY_PAR1) {
               tm1638_put_string(0, "Pattern ");
           } else {
               tm1638_put_string(0, (char*) pattern_get_name());
           }
           break;
   case 2: // select timebase as T_basetick in us (>=1.0)
           if ((key == KEY_PREV) && (selected_timer_ticks > 72)) {
               selected_timer_ticks -= increment;
               if (selected_timer_ticks < 72) {
                  selected_timer_ticks = 72;
               }
               ui_apply(); // also recalculates!
           }
           if ((key == KEY_NEXT) && (selected_timer_ticks < 65535)) {
               selected_timer_ticks += increment;
               if (selected_timer_ticks > 65535) {
                   selected_timer_ticks = 65535;
               }
               ui_apply(); // also recalculates!
           }
           if (key == KEY_PAR2) {
               tm1638_put_string(0, "basetick");
           } else {
               snprintf(buff, 10, "%d.%02d us  ", (selected_timer_ticks / 72), (selected_timer_ticks * 100)/72 - (selected_timer_ticks/72) * 100);
               tm1638_put_string(0, buff);
           }
           break;
   case 3: // select timebase as frames per second
           if ((key == KEY_NEXT) && (selected_timer_ticks > 72)) {
               selected_timer_ticks -= increment;
               if (selected_timer_ticks < 72) {
                  selected_timer_ticks = 72;
               }
               ui_apply(); // also recalculates!
           }
           if ((key == KEY_PREV) && (selected_timer_ticks < 65535)) {
               selected_timer_ticks += increment;
               if (selected_timer_ticks > 65535) {
                   selected_timer_ticks = 65535;
               }
               ui_apply(); // also recalculates!
           }
           if (key == KEY_PAR3) {
               tm1638_put_string(0, (systick_seconds & 1)?" frames ":" per s  ");
           } else {
               snprintf(buff, 10, "%lu.%02lu Hz  ", frames_per_hundred_s / 100, frames_per_hundred_s - 100*(frames_per_hundred_s/100));
               tm1638_put_string(0, buff);
           }
           break;
   case 4: // select target events per channel per frame (display as per second!)
           if (get_pattern_number() != get_max_pattern_number()) {
              tm1638_put_string(0," --  -- ");
           } else {
              selected_frame_events = generic_pattern_total_increments / generic_pattern_event_increment;
              if ((key == KEY_PREV) && (selected_frame_events > 1)) {
                 selected_frame_events -= increment;
                 if (selected_frame_events < 1) {
                     selected_frame_events = 1;
                 }
                 ui_apply(); // also recalculates!
              }
              if ((key == KEY_NEXT) && (selected_frame_events < 222222)) {
                 selected_frame_events += increment;
                 if (selected_frame_events > 22222) {
                    selected_frame_events = 22222;
                 }
                 ui_apply(); // also recalculates!
              }
              if (key == KEY_PAR4) {
                  tm1638_put_string(0, (systick_seconds & 1)?"evts per":"ch frame");
              } else {
                  i = selected_frame_events * frames_per_hundred_s;
                  snprintf(buff, 10, "%d per s ", i/100);
                  tm1638_put_string(0, buff);
              }
           }
           break;
   case 5:
   case 6:
           tm1638_put_string(0," --  -- ");
           break;
   default:
           snprintf(buff,10, "ERROR %d", selected_param);
           tm1638_put_string(0, buff);
   }

    // any 'param-select key' pressed?
    if (key & 0x7e) {
        tm1638_set_led(selected_param, 0);
        for (i=1; i<7; i++) {
           if (key & (1<<i)) { // Key-select for param<i> pressed
              selected_param = i;
              tm1638_set_led(selected_param, 1); // also light led
           }
        }
    }

    // PREV & NEXT
    if (key == 0x81) {
        tm1638_set_led(selected_param, 0);
        selected_param = -1;
    }
}

