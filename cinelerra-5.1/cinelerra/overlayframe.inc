
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

#ifndef OVERLAYFRAME_INC
#define OVERLAYFRAME_INC

// Modes

#define TRANSFER_TYPES 30

#define TRANSFER_NORMAL		0
#define TRANSFER_ADDITION	1
#define TRANSFER_SUBTRACT	2
#define TRANSFER_MULTIPLY	3
#define TRANSFER_DIVIDE		4
#define TRANSFER_REPLACE	5
#define TRANSFER_MAX		6
#define TRANSFER_MIN		7
#define TRANSFER_DARKEN		8
#define TRANSFER_LIGHTEN	9
#define TRANSFER_DST		10
#define TRANSFER_DST_ATOP	11
#define TRANSFER_DST_IN		12
#define TRANSFER_DST_OUT	13
#define TRANSFER_DST_OVER	14
#define TRANSFER_SRC		15
#define TRANSFER_SRC_ATOP	16
#define TRANSFER_SRC_IN		17
#define TRANSFER_SRC_OUT	18
#define TRANSFER_SRC_OVER	19
#define TRANSFER_AND		20
#define TRANSFER_OR		21
#define TRANSFER_XOR		22
#define TRANSFER_OVERLAY	23
#define TRANSFER_SCREEN		24
#define TRANSFER_BURN		25
#define TRANSFER_DODGE		26
#define TRANSFER_HARDLIGHT	27
#define TRANSFER_SOFTLIGHT	28
#define TRANSFER_DIFFERENCE	29


// Interpolation types


#define NEAREST_NEIGHBOR 0        // Nearest for enlargement and reduction
#define CUBIC_CUBIC      1        // Bicubic enlargement and reduction
#define CUBIC_LINEAR     2        // Bicubic enlargement and linear reduction
#define LINEAR_LINEAR    3        // Not explicitely called but used for user
#define LANCZOS_LANCZOS  4        // Just because we can

class OverlayFrame;

#endif
