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

#ifndef _PATTERN_H_
#define _PATTERN_H_

#include <stdint.h>

uint8_t get_max_pattern_number(void);
uint8_t get_pattern_number(void);
void pattern_select(uint8_t pattern);

int fill_buffer(uint8_t *buffer, uint32_t bufsize, uint32_t ofs);

char* pattern_get_name(void);
uint32_t pattern_get_period(void);

extern unsigned int generic_pattern_total_increments; // calculated
extern unsigned int generic_pattern_event_increment; // >= (pgc==7)? 510 : 73
extern unsigned int generic_pattern_extra_increment; // to make total_increments n$
extern unsigned int generic_pattern_start_increment; // stored for next round
extern uint8_t generic_pattern_channels; // 1 or 7
extern uint8_t generic_pattern_interpolate_env; // if 1: linear interpolation, els$

#endif

