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

#include "pattern.h"
#include "random.h"
#include "config.h"

// for memset
#include <string.h>

// for debug printf
#include <stdio.h>

typedef void(*fptr_t)(uint8_t *, uint32_t, uint32_t);

typedef struct {
char *name;
fptr_t generate;
uint32_t period;
} pattern_t;

static void null_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void stupid_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void stupid_pattern2(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void stupid_pattern3(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void binary_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void slow_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void medium_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void fast_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void step_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void fast_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void medium_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void rare_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void slow_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void fast_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void bin_walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static void generic_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);

static uint8_t pattern_number = 0;

pattern_t patterns[] = {
/*0*/   {"  null  ", null_pattern, .period=256},           // 0 evts/frame, 150us, 26Hz, 0evt/s
/*1*/   {"slow_reg", slow_regular_pattern, 256},           // 3 evts/frame, 150us, 26Hz->78evt/s
/*2*/   {"stupid 1", stupid_pattern, 256},                 // 7 evts/frame, 150us, 26Hz->182evt/s
/*C*/   {"stupid 2", stupid_pattern2, 8192},               // 7 evts/26frame, 150us, 26Hz
/*D*/   {"stupid 3", stupid_pattern3, 8192},               // 7 evts/frame, 150us, 26Hz->182evt/s
/*E*/   {"walking ", walking_pattern, 8192},               // 7 evts/frame, 150us, 26Hz->182evt/s
/*F*/   {"bin_walk", bin_walking_pattern, 8192},           // 7 evts/frame, 150us, 26Hz->182evt/s
/*3*/ //  {"slow_rnd", slow_random_pattern, 128},            // 53 evts/frame, 300us, 26Hz->1378evt/s
/*4*/   {"med_reg ", medium_regular_pattern, 2048},        // 120 evts/frame, 15us, 32.55Hz->4K
/*5*/   {"rare_lin", rare_linear_shaped_pattern, 8192},    // 226 evts/frame, 4.5us, 27.125Hz->6K
/*6*/   {"bin-ctr ", binary_pattern, 256},                 // 254 evts/frame, 150us, 26Hz->6K6
/*7*/   {"med_lin ", medium_linear_shaped_pattern, 8192},  // 711 evts/frame, 4.5us, 27.125Hz->19K
/*8*/   {"stepShap", step_shaped_pattern, 8192},           // 1580 evts/frame, 4.5us, 27.1Hz->43K
/*9*/ //  {"fast_rnd", fast_random_pattern, 1024},           // 2390 evts/frame, 30us, 32.55Hz->78K
/*A*/   {"fast_lin", fast_linear_shaped_pattern, 8192},    // 5237 evts/frame, 4.5us, 27.1Hz->142K
/*B*/   {"fast_reg", fast_regular_pattern, 8192},          // 8192 evts/frame, 4.5us, 27.1Hz->222K
/*G*/   {"generic ", generic_pattern, 8192},               // parameterized 1..8192 evts/frame
};

char* pattern_get_name(void) {
    return patterns[pattern_number].name;
}

uint32_t pattern_get_period(void) {
    return patterns[pattern_number].period;
}

void pattern_select(uint8_t pattern) {
   pattern_number = \
      (((unsigned int) pattern) <= (get_max_pattern_number()))?
      pattern : 0;
}

uint8_t get_max_pattern_number(void) {
   return (sizeof(patterns)/sizeof(pattern_t))-1;
}

uint8_t get_pattern_number(void) {
   return pattern_number;
}

/*
 * fill buffer with data and return offset to be used for next call
 * if bufsize is 0: return the repeat period
 * if ofs==0, also generate refclock pulse
 */
int fill_buffer(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   pattern_t *current_pattern = &patterns[pattern_number];
   uint32_t basepos=0, remaining=bufsize, period = current_pattern->period;

   if(!bufsize) {
      // period should be at least MIN_REPEAT !!!
      return (period >= MIN_REPEAT)?(period):(MIN_REPEAT);
   }

   // We have to fill the buffer
   while(remaining) {
      // safeguard: should not be needed
      while (ofs >= period)
         ofs -= period;
      if (remaining + ofs >= period) {
         // would 'underflow', fill in the remaining bytes and restart
         current_pattern->generate(buffer + basepos, period - ofs, ofs);
         // set refclock marker on first byte in first block (bufsize!=0, ofs==0)
         if (!ofs)
            *(buffer + basepos) |= 0x80;
         remaining -= period - ofs;
         basepos += period - ofs;
         ofs = 0; // restart from 0
      } else {
         // snip out a piece of pattern to fill the buffer
         current_pattern->generate(buffer + basepos, remaining, ofs);
         // set refclock marker on first byte in first block (bufsize!=0, ofs==0)
         if (!ofs)
            *(buffer + basepos) |= 0x80;
         basepos += remaining;
         ofs += remaining;
         remaining = 0;
      }
   }
   return ofs;
}

// define several selectable patterns
// patterns should only write to bits 0..6 of the buffer bytes !

// null-pattern: no pulses anywhere
// smalles sensible bufsize is 2
static void null_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   (void) ofs; // silence compiler. no use for the ofs parameter...
   memset(buffer, 0, bufsize);
}


// stupid-pattern: exactly one pulse close to beginning (one pulse for each channel, but at a different time)
// smallest sensible bufsize is 8
static void stupid_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint8_t m = (ofs) ? 0: 0x80;

   memset(buffer, 0, bufsize);
   while(bufsize) {
      *buffer++ = m;
      m>>= 1;
      if(!m)
         break;
   }
}


// stupid2-pattern: exactly one pulse in the middle, once every 32 frames
// smallest sensible bufsize is 2
static void stupid_pattern2(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint_fast32_t m;

   if (!bufsize)
      // safeguard
      return;
   memset(buffer, 0, bufsize);
   if (!ofs) {
      if(!m)
         m = (1<<25);
      buffer[bufsize/2] |= (m & 0x7f);
      m >>= 1;
   }
}


// stupid3-pattern: exactly one pulse close to beginning
static void stupid_pattern3(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint32_t i;

   memset(buffer, 0, bufsize);
   for(i=ofs;i<bufsize+ofs;i++) {
      if (i==1) {
         // extend refclock
         buffer[i] = 0x80;
      }
      if (i==3) { // XXX: make location a parameter? (could omit stupid2 as a variant of this)
         // exactly one pulse
         buffer[i] = 0x7f;
         break;
      }
   }
}


// output a binary code
// only sensible bufsize is 256
static void binary_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint8_t val = 0x80 + ofs; // truncate upper bits of ofs!
   
   bufsize &= 0xff; // only fill up to 256 locations
   for(;bufsize;bufsize--)
      *buffer++ = val++;
}


// output a regular pattern
// smallest sensible bufsize is 2
static void slow_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint8_t m, i;

   (void) ofs; // silence compiler. we rely on beeing called 'in-order', no use for ofs.

   for(;bufsize;bufsize--) {
      if (!i--) {
         m >>= 1;
         if (!m) {
             m = 0x40;
         }
         *buffer++ = m;
         i = 84;  // XXX: selectable parameter? (generic pattern with constant env?)
      } else {
         *buffer++ = 0;
      }
   }
}


static void medium_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint8_t m, i;

   (void) ofs; // silence compiler. we rely on beeing called 'in-order', no use for ofs.

   for(;bufsize;bufsize--) {
      if (!i--) {
         if (!m)
             m = 0x40;
         *buffer++ = m;
         m >>= 1;
         i = 16;
      } else {
         *buffer++ = 0;  // XXX: selectable parameter? (generic pattern with constant env?)
      }
   }
}


static void fast_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint8_t m;

   (void) ofs; // silence compiler. we rely on beeing called 'in-order', no use for ofs.

   for(;bufsize;bufsize--) {
      if (!m)
          m = 0x40;
      *buffer++ = m;
      m >>= 1;
   }
}


// envelope for a 'nice' histogram. needs to contain 33 values, first value == last value !
static const uint8_t env[] = {
   0x00,
   0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xff,
   0xcc, 0x80,
   0x55, 0x40, 0xcc,
   0x33, 0x20, 0x66,
   0x19, 0x10, 0x33,
   0x0c, 0x08, 0x22,
   0x06, 0x04, 0x11,
   0x03, 0x02, 0x0f,
   0x01, 0x00,
   0x00 // duplicate of first entry
};


// divide the 8192 time slots into 32 slots a 256 times
// frequency of events in each slot is given by env (0=no event, 255=max freq)
// events are distributed over 7 channels (at env=255, every 7'th time an event is generated per channel)
static void step_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static int interp; // interpolation counter
   static uint8_t m;
   int pos=ofs;
   uint8_t cenv; // current envelope value

   cenv = env[32 & (ofs >> 8)];
   for(;bufsize;bufsize--) {
      if (!(pos & 0xff)) {
         // first entry of next slot
         cenv = env[31 & (pos >> 8)]; // obtain current frequency
      }
      interp += cenv;
      if (interp >= 255) { // XXX: make this configurable as in generic_pattern?
         interp -= 255;
         m >>= 1;
         if (!m) {
             m = 0x40;
         }
         *buffer++ = m;
      } else {
         *buffer++=0;
      }
      pos++;
   }
}


// XXX: make fast/medium/rare configurable!
// similiar to step_shaped pattern, but do a linear interpolation of the envelope.
// basically a variant of the generic_pattern with fixed events/frame and distribution to 7 channels
static void fast_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static int ctr; // linear interpolation counter
   static int interp; // interpolation counter
   static uint8_t m;
   int pos=ofs;
   int cenv; // current env value
   int diff; // diff from current env point to next one
   uint8_t v;

   cenv = env[31 & (pos >> 8)];
   diff = env[31 & (1+(pos >> 8))] -(int)cenv;
   for(;bufsize;bufsize--) {
      if (!(bufsize & 0xff)) {
         cenv = env[31 & (pos >> 8)];
         diff = env[31 & (1+(pos >> 8))] -(int)cenv;
      }
      ctr += diff;
      if (ctr >= 255) {
         ctr-=255;
         cenv++;
      }
      if (ctr < 0) {
         ctr+=255;
         cenv--;
      }
      v = 0;
      interp += cenv;
      while (interp >= 77) {
         interp-=77;
         if (!m)
             m = 0x40;
         v |= m;
         m >>= 1;
      }
      *buffer++=v;
      pos++;
   }
}


static void medium_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static int ctr; // linear interpolation counter
   static int interp; // interpolation counter
   static uint8_t m;
   int pos=ofs;
   int cenv; // current env value
   int diff; // diff from current env point to next one
   uint8_t v;

   cenv = env[31 & (pos >> 8)];
   diff = env[31 & (1+(pos >> 8))] -(int)cenv;
   for(;bufsize;bufsize--) {
      if (!(bufsize & 0xff)) {
         cenv = env[31 & (pos >> 8)];
         diff = env[31 & (1+(pos >> 8))] -(int)cenv;
      }
      ctr += diff;
      if (ctr >= 255) {
         ctr-=255;
         cenv++;
      }
      if (ctr < 0) {
         ctr+=255;
         cenv--;
      }
      v = 0;
      interp += cenv;
      if (interp >= 567) {
         interp-=567;
         if (!m)
             m = 0x40;
         v |= m;
         m >>= 1;
      }
      *buffer++=v;
      pos++;
   }
}


static void rare_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static int ctr; // linear interpolation counter
   static int interp; // interpolation counter
   static uint8_t m;
   int pos=ofs;
   int cenv; // current env value
   int diff; // diff from current env point to next one
   uint8_t v;

   cenv = env[31 & (pos >> 8)];
   diff = env[31 & (1+(pos >> 8))] -(int)cenv;
   for(;bufsize;bufsize--) {
      if (!(bufsize & 0xff)) {
         cenv = env[31 & (pos >> 8)];
         diff = env[31 & (1+(pos >> 8))] -(int)cenv;
      }
      ctr += diff;
      if (ctr >= 255) {
         ctr-=255;
         cenv++;
      }
      if (ctr < 0) {
         ctr+=255;
         cenv--;
      }
      v = 0;
      interp += cenv;
      if (interp >= 1777) {
         interp-=1777;
         if (!m)
             m = 0x40;
         v |= m;
         m >>= 1;
      }
      *buffer++=v;
      pos++;
   }
}


// output a random pattern
// dont use this at higher ticker frequencies, as this uses too much cpu-time
// also seems not very useful atm.
static void slow_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint32_t r;
   static uint8_t last;
   ofs++;
   for(;bufsize;bufsize--) {
      r = _random();
      r &= r >> 14;
      r &= r >> 7;
      *buffer++ = last = (r & 0x7f) & ~last;
   }
}


static void fast_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint32_t r;
   static uint8_t last;
   ofs++;
   for(;bufsize;bufsize--) {
      if (!r) {
         r = _random();
      }
      *buffer++ = last = (r & 0x7f) & ~last;
      r >>= 8;
   }
}


// walking-pattern
static void walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint32_t walker;
   static uint32_t wcounts;
   static uint8_t wmask;
   uint32_t i;

   (void) ofs; // we rely on beeing called 'in-order', hence no use for ofs.
   if (!bufsize) {
      // re-init
      walker = 0;
      wcounts = 0;
      wmask = 0x40;
      return;
   }

   memset(buffer, 0, bufsize);
   for (i=walker;i<bufsize; i+=1024) { // XXX: make walker distance configurable
      wmask >>= 1;
      if (!wmask) {
         wmask = 0x40;
      }
      buffer[i] = wmask;
      buffer[i+1] = wmask;
      buffer[i+2] = wmask;
   }
   if (++wcounts == 1000) { // XXX: make events_per_position configurable
      wcounts = 0;
      walker += 8;
      if (walker >= 1024) {
         walker -= 1024;
      }
   }
}


#define TARGET_COUNTS_PER_BIN 1000
static void bin_walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint32_t walker;
   static uint32_t wcounts;
   static uint8_t wmask;

   if (!bufsize) {
      // re-init
      walker = 0;
      wcounts = 0;
      wmask = 0x40;
      return;
   }

   memset(buffer, 0, bufsize);
   // early exit if walker walked too far...
   if (walker >= 8192)
      return;

   // walker in currently requested piece: set it
   if ((ofs <= walker) && (walker < ofs+bufsize)) {
      buffer[walker-ofs] = wmask;

      // count events
      if (++wcounts == TARGET_COUNTS_PER_BIN) {
         wcounts = 0;
         wmask >>=1; // flip to other module
         if (!wmask) {
            // advance walker
            if (walker) {
               walker *= 2;
            } else {
               walker = 1;
            }
            wmask = 0x40;
         }
      }
   }
}

// parametrized versions below this line....


unsigned int generic_pattern_total_increments = 0; // calculated
unsigned int generic_pattern_event_increment = 8058; // >= (pgc==7)? 510 : 73
unsigned int generic_pattern_extra_increment = 1; // to make total_increments not divisible by event_increment
unsigned int generic_pattern_start_increment = 0; // stored for next round
uint8_t generic_pattern_channels = 7; // 1 or 7
uint8_t generic_pattern_interpolate_env = 1; // if 1: linear interpolation, else staircase

static void generic_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static int ctr; // linear interpolation counter
   static int interp; // interpolation counter
   uint32_t pos = ofs;
   uint8_t cenv; // current env value
   int diff; // diff from current env point to next one
   uint8_t v;

   if(!generic_pattern_total_increments) {
      // determine total increments
      interp = 0;
      ctr = 128;
      for (pos = 0; pos < 32*256; pos++) {
         if (!(bufsize & 0xff)) {
            cenv = env[31 & (pos >> 8)];
            diff = env[31 & (1+(pos >> 8))] -(int)cenv;
         }
         ctr += diff;
         if (ctr >= 256) {
            ctr-=256;
            cenv++;
         }
         if (ctr < 0) {
            ctr+=256;
            cenv--;
         }
         generic_pattern_total_increments += cenv;
      }
      printf("Generic Pattern total increments are %u\n", generic_pattern_total_increments);
      interp = 0;
      ctr = 128;
   }

   if (generic_pattern_event_increment < 510) {
      generic_pattern_event_increment = 510;
   }

   if (!bufsize) {
      // check extra incr
      if ((generic_pattern_total_increments % generic_pattern_event_increment == 0) || \
          ((generic_pattern_total_increments & generic_pattern_event_increment & 1) == 0)) {
         generic_pattern_extra_increment = 1;
      } else {
         generic_pattern_extra_increment = 0;
      }
      printf("Generic Pattern event increment is %u\n", generic_pattern_event_increment);
      printf("Generic Pattern extra increment is %u\n", generic_pattern_extra_increment);
   }

   // normal operation
   interp = generic_pattern_start_increment;
   if(!ofs)
      interp += generic_pattern_extra_increment; // Start-Of-Frame
   cenv = env[31 & (pos >> 8)];
   diff = env[31 & (1+(pos >> 8))] -(int)cenv;
   for(;bufsize;bufsize--) {
      if (!(bufsize & 0xff)) {
         cenv = env[31 & (pos >> 8)];
         diff = env[31 & (1+(pos >> 8))] -(int)cenv;
      }
      ctr += diff;
      if (ctr >= 256) {
         ctr-=256;
         cenv++;
      }
      if (ctr < 0) {
         ctr+=256;
         cenv--;
      }
      v = 0;
      interp += cenv;
      if (interp >= generic_pattern_event_increment) {
         interp -= generic_pattern_event_increment;
         v = 0x7f; // 7 channel mode
      }
      *buffer++=v;
      pos++;
   }
   generic_pattern_start_increment = interp;
}
