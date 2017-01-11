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

#include "pattern.h"
#include "random.h"
#include "config.h"

// for memset
#include <string.h>

static int null_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int stupid_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int stupid_pattern2(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int stupid_pattern3(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int binary_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int slow_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int medium_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int fast_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int step_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int fast_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int medium_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int rare_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int slow_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int fast_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int bin_walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);
static int generic_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);

typedef int(*fptr_t)(uint8_t *, uint32_t, uint32_t);

static fptr_t current_pattern = &null_pattern;
static int pattern_number = 0;

const char* pattern_names[] = {
	"  null  ",
	"slow_reg",
	"stupid 1",
	"slow_rnd",
	"med_reg ",
	"rare_lin",
	"bin-ctr ",
	"med_lin ",
	"stepShap",
        "fast_rnd",
	"fast_lin",
	"fast_reg",
	"stupid 2",
	"stupid 3",
	"walking ",
	"bin_walk",
	"generic"
};


static const fptr_t patterns[] = {
/*0*/   &null_pattern,			// 0 evts/frame, 150us, 26Hz, 0evt/s
/*1*/   &slow_regular_pattern,		// 3 evts/frame, 150us, 26Hz->78evt/s
/*2*/   &stupid_pattern,                // 7 evts/frame, 150us, 26Hz->182evt/s
/*3*/   &slow_random_pattern,		// 53 evts/frame, 300us, 26Hz->1378evt/s
/*4*/   &medium_regular_pattern,	// 120 evts/frame, 15us, 32.55Hz->4K
/*5*/   &rare_linear_shaped_pattern,	// 226 evts/frame, 4.5us, 27.125Hz->6K
/*6*/   &binary_pattern,		// 254 evts/frame, 150us, 26Hz->6K6
/*7*/   &medium_linear_shaped_pattern,	// 711 evts/frame, 4.5us, 27.125Hz->19K
/*8*/   &step_shaped_pattern,		// 1580 evts/frame, 4.5us, 27.1Hz->43K
/*9*/   &fast_random_pattern,		// 2390 evts/frame, 30us, 32.55Hz->78K
/*A*/   &fast_linear_shaped_pattern,	// 5237 evts/frame, 4.5us, 27.1Hz->142K
/*B*/   &fast_regular_pattern,		// 8192 evts/frame, 4.5us, 27.1Hz->222K
/*C*/   &stupid_pattern2,               // 7 evts/26frame, 150us, 26Hz
/*D*/   &stupid_pattern3,               // 7 evts/frame, 150us, 26Hz->182evt/s
/*E*/   &walking_pattern,               // 7 evts/frame, 150us, 26Hz->182evt/s
/*F*/   &bin_walking_pattern,           // 7 evts/frame, 150us, 26Hz->182evt/s
/*G*/   &generic_pattern                // parameterized 1..8192 evts/frame
};

void select_pattern(int pattern) {
   pattern_number = \
      (((unsigned int) pattern) < (sizeof(patterns)/sizeof(patterns[0])))?
      pattern : 0;
   current_pattern = patterns[pattern_number];
}

int get_max_pattern_number(void) {
   return (sizeof(patterns)/sizeof(patterns[0]));
}

int get_pattern_number(void) {
   return pattern_number;
}

/*
 * fill buffer with data and return offset to be used for next call
 * if bufsize is 0: return the repeat period or 0 if it can stretch to any
 * if ofs==0, also generate refclock pulse
 */
static uint32_t period = 0;
int fill_buffer(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint32_t basepos=0, remaining=bufsize;
   // querying period -> remember period and return
   if(!bufsize) {
      period = (*current_pattern)(buffer, bufsize, ofs);
      // default stretchable to period MIN_REPEAT
      if (!period)
         period = MIN_REPEAT;
      return period;
   }

   // We have to fill the buffer
   while(remaining) {
      // safeguard: should not be needed
      while (ofs >= period)
         ofs -= period;
      if (remaining+ofs >= period) {
         // would 'underflow', fill in the remaining bytes and restart
         (*current_pattern)(buffer+basepos, period - ofs, ofs);
         // set refclock marker on first byte in first block (bufsize!=0, ofs==0)
         if (!ofs)
            *(buffer+basepos) |= 0x80;
         remaining -= period-ofs;
         basepos += period-ofs;
         ofs = 0; // restart from 0
      } else {
         // snip out a piece of pattern to fill the buffer
         (*current_pattern)(buffer+basepos, remaining, ofs);
         // set refclock marker on first byte in first block (bufsize!=0, ofs==0)
         if (!ofs)
            *(buffer+basepos) |= 0x80;
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
static int null_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   memset(buffer, 0, bufsize);
   return 0*ofs; // smallest size is 1
}

// stupid-pattern: exactly one pulse close to beginning
static int stupid_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint8_t m = (ofs) ? 0: 0x80;
   memset(buffer, 0, bufsize);
   while(bufsize) {
      *buffer++ = m;
      m>>= 1;
      if(!m)
         break;
   }
   return 0; // smallest size is 8
}

// stupid2-pattern: exactly one pulse in the middle, once every 32 frames
static int stupid_pattern2(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   static uint_fast32_t m;
   if (!bufsize)
      return 16384;
   memset(buffer, 0, bufsize);
   if (!ofs) {
      if(!m)
         m = (1<<25);
      buffer[bufsize/2] |= (m & 0x7f);
      m >>= 1;
   }
   return 16384;
}

// output a binary code
static int binary_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   uint8_t val = 0x80 + ofs; // truncate upper bits of ofs!
   int rest = 256 - (int) ofs - (int) bufsize;

   for(;bufsize;bufsize--)
      *buffer++ = val++;
   return rest; // smalles size is 256
}

// output a regular pattern
static uint8_t m=0, i=0;
static int slow_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 256 - (int) ofs - (int) bufsize;

   for(;bufsize;bufsize--) {
      if (!i--) {
         if (!m)
             m = 0x40;
         *buffer++ = m;
         m >>= 1;
         i = 84;
      } else {
         *buffer++ = 0;
      }
   }
   return rest;
}

static int medium_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 2048 - (int) ofs - (int) bufsize;

   for(;bufsize;bufsize--) {
      if (!i--) {
         if (!m)
             m = 0x40;
         *buffer++ = m;
         m >>= 1;
         i = 16;
      } else {
         *buffer++ = 0;
      }
   }
   return rest;
}

static int fast_regular_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 8192 - (int) ofs - (int) bufsize;

   for(;bufsize;bufsize--) {
      if (!m)
          m = 0x40;
      *buffer++ = m;
      m >>= 1;
   }
   return rest;
}

// envelopes
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

static int interp = 128;
static int step_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 32*256 - (int) ofs - (int) bufsize;
   int pos=ofs;
   uint8_t cenv; // current env value

   for(;bufsize;bufsize--) {
      cenv = env[31 & (pos / 256)];
      interp += cenv;
      if (interp >= 255) {
         interp-=255;
         if (!m)
             m = 0x40;
         *buffer++ = m;
         m >>= 1;
      } else *buffer++=0;
      pos++;
   }
   return rest;
}

static int ctr=128; // linear interpolation counter
static int fast_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 32*256 - (int) ofs - (int) bufsize;
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
   return rest;
}


static int medium_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 32*256 - (int) ofs - (int) bufsize;
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
   return rest;
}

static int rare_linear_shaped_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 32*256 - (int) ofs - (int) bufsize;
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
   return rest;
}

// output a random pattern
static uint32_t r=0;
static uint8_t last=0;
static int slow_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   ofs++;
   for(;bufsize;bufsize--) {
      r = _random();
      r &= r >> 14;
      r &= r >> 7;
      *buffer++ = last = (r & 0x7f) & ~last;
   }
   return 128;
}

static int fast_random_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   ofs++;
   for(;bufsize;bufsize--) {
      if (!r) {
         r = _random();
      }
      *buffer++ = last = (r & 0x7f) & ~last;
      r >>= 8;
   }
   return 1024;
}

// stupid3-pattern: exactly one pulse close to beginning
static int stupid_pattern3(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int i;
   memset(buffer, 0, bufsize);
   for(i=0;i<bufsize;i++) {
      if (i==1) {
         buffer[i] = 0x80;
      }
//      if (i==2) {
//         buffer[i] = 0x7f;
//      }
      if (i==3) {
         buffer[i] = 0x7f;
         break;
      }
   }
   return 8192; // smallest size is 16
}


// walking-pattern
static int walker=0;
static int wcounts=0;
static uint8_t wmask = 0x40;
static int walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int i;
   if (!bufsize)
      return 16384;

   memset(buffer, 0, bufsize);
   for (i=walker;i<bufsize; i+=1024) {
      buffer[i] = wmask;
      buffer[i+1] = wmask;
      buffer[i+2] = wmask;
      wmask ^= 0x7f;
   }
   if (++wcounts == 100000) {
      wcounts = 0;
      walker += 8;
      if (walker >= 1024) {
         walker -= 1024;
         wmask ^= 0x7f;
      }
   }
   return 16384;
}

#define TARGET_COUNTS_PER_BIN 100000
//static int walker=0;
//static int wcounts=0;
//static uint8_t wmask = 0x40;
static int bin_walking_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int i;
   if (!bufsize) {
      // re-init
      walker=0;
      wcounts = 0;
      wmask = 0x40;
      return 16384;
   }

   memset(buffer, 0, bufsize);
   // early exit if walker walked too far...
   if (walker >= 16384)
      return 16384;

   // walker in currently requested piece: set it
   if ((ofs <= walker) && (walker < ofs+bufsize)) {
      buffer[walker-ofs] = wmask;

      // count events
      if (++wcounts == TARGET_COUNTS_PER_BIN) {
         wcounts = 0;
         wmask ^= 0x7f; // flip to other module
         if (wmask == 0x40) {
            // advance walker
            if (walker) {
               walker*=2;
            } else {
               walker = 1;
            }
         }
      }
   }
   return 16384;
}

unsigned int generic_pattern_total_increments = 0; // calculated
unsigned int generic_pattern_event_increment = 510; // >= (pgc==7)? 510 : 73
unsigned int generic_pattern_extra_increment = 0; // to make total_increments not divisible by event_increment
unsigned int generic_pattern_start_increment = 0; // stored for next round
uint8_t generic_pattern_channels = 7; // 1 or 7
uint8_t generic_pattern_interpolate_env = 1; // if 1: linear interpolation, else staircase

static int generic_pattern(uint8_t *buffer, uint32_t bufsize, uint32_t ofs) {
   int rest = 32*256 - (int) ofs - (int) bufsize;
   int pos = ofs;
   int cenv; // current env value
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
   }

   if (!bufsize) {
      // check extra incr
      if (generic_pattern_total_increments % generic_pattern_event_increment == 0) {
         generic_pattern_extra_increment = 1 + generic_pattern_event_increment>>4;
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
   return rest;
}
