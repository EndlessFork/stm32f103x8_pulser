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
#include "stlinky.h"

#ifdef USE_STLINKY
#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>


/*
 * stlinky
 */
#define STLINKY_MAGIC 0xDEADF00D
#define STLINKY_BUFSIZE 256
typedef struct stlinky {
   uint32_t magic;
   uint32_t bufsize;
   uint32_t up_tail;
   uint32_t up_head;
   uint32_t dwn_tail;
   uint32_t dwn_head;
   char upbuff[STLINKY_BUFSIZE];
   char dwnbuff[STLINKY_BUFSIZE];
} __attribute__ ((packed)) stlinky_t;

static volatile stlinky_t stl = {
   .magic = STLINKY_MAGIC,
   .bufsize = STLINKY_BUFSIZE,
   .up_tail = 0,
   .up_head = 0,
   .dwn_tail = 0,
   .dwn_head = 0,
};

void stl_putchar(char c) {
   uint32_t next_head;
   next_head = (stl.up_head +1) % STLINKY_BUFSIZE;
   // wait for buffer space
//   while (next_head == stl.up_tail) ;
   if (next_head ==stl.up_tail)
      return; // if no DBG frontend, eat messages....
   stl.upbuff[stl.up_head] = c;
   stl.up_head = next_head;
}

// provide backend for stdout/stderr
int _write(int file, char *ptr, int len);
int _write(int file, char *ptr, int len) {
   int i;

   if (file == STDOUT_FILENO || file == STDERR_FILENO) {
      for (i = 0; i < len; i++) {
         stl_putchar(ptr[i]);
      }
      return i;
   }
   errno = EIO;
   return -1;
}


#endif
