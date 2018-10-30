
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

#ifndef BC_WINDOWBASE_INC
#define BC_WINDOWBASE_INC

class BC_ActiveBitmaps;
class BC_WindowBase;
class BC_WindowList;
#define BCTEXTLEN 1024
#define BCSTRLEN 64
#define KEYPRESSLEN 6
// Milliseconds before cursor disappears during video playback
#define VIDEO_CURSOR_TIMEOUT 2000

// mouse wheel
#define WHEEL_UP 4
#define WHEEL_DOWN 5


#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3

// textbox policy mask
#define CLICK_ACTIVATE 1
#define CLICK_DEACTIVATE 2

// Use single thread for all windows.
// This is a bad idea because it's very slow.  Windows which share the same
// display must be locked in their create_objects routines.
//#define SINGLE_THREAD

#endif
