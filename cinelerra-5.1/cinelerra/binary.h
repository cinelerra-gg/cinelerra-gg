
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef BINARY_H
#define BINARY_H

#include <stdio.h>

#include "sizes.h"

inline void putfourswap(int32_t number, FILE *file) {
  fputc(number, file);
  fputc(number >> 8, file);
  fputc(number >> 16, file);
  fputc(number >> 24, file);
}

inline void putfour(int32_t number, FILE *file) {
  fputc(number >> 24, file);
  fputc(number >> 16, file);
  fputc(number >> 8, file);
  fputc(number, file);
}

inline int32_t getfour(FILE *in) {
  int32_t number = ((int32_t)fgetc(in) << 24);
  number |= ((int32_t)fgetc(in) << 16);
  number |= ((int32_t)fgetc(in) << 8);
  number |= ((int32_t)fgetc(in));
  return number;
}

inline int32_t getfourswap(FILE *in) {
  int32_t number = ((int32_t)fgetc(in));
  number |= ((int32_t)fgetc(in) << 8);
  number |= ((int32_t)fgetc(in) << 16);
  number |= ((int32_t)fgetc(in) << 24);
  return number;
}

inline int16_t gettwo(FILE *in) {
  int16_t number = ((int32_t)fgetc(in) << 8);
  number |= ((int32_t)fgetc(in));
  return number;
}

inline void puttwo(int16_t number, FILE *file) {
  fputc(number >> 8, file);
  fputc(number, file);
}

#endif
