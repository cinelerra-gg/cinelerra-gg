
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#ifndef LOADMODE_INC
#define LOADMODE_INC

class LoadMode;

// used in paste_edls to only add assets, not clips
#define LOADMODE_ASSETSONLY -1

// Load modes for loading files, importing recordings, importing effects
// Be sure to change mode_images in LoadMode::create_objects if you change this.
#define LOADMODE_NOTHING 0
#define LOADMODE_REPLACE 1
#define LOADMODE_REPLACE_CONCATENATE 2
#define LOADMODE_NEW_TRACKS 3
#define LOADMODE_CONCATENATE 4
#define LOADMODE_PASTE 5
#define LOADMODE_RESOURCESONLY 6
#define LOADMODE_NESTED 7
#define TOTAL_LOADMODES 8

#endif
