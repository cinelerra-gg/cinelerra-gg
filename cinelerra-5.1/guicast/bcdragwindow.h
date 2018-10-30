
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

#ifndef BCDRAGWINDOW_H
#define BCDRAGWINDOW_H

#include "bcpixmap.inc"
#include "bcpopup.h"

class BC_DragWindow : public BC_Popup
{
public:
	BC_DragWindow(BC_WindowBase *parent_window, BC_Pixmap *pixmap, int center_x, int center_y);
	BC_DragWindow(BC_WindowBase *parent_window, VFrame *frame, int center_x, int center_y);
	~BC_DragWindow();

	int cursor_motion_event();
	int button_release_event();
	int drag_failure_event();
// Disable failure animation
	void set_animation(int value);
	BC_Pixmap *prepare_frame(VFrame *frame, BC_WindowBase *parent_window);
	BC_Pixmap *prepare_pixmap(BC_Pixmap *pixmap, BC_WindowBase *parent_window);
private:
	int init_x, init_y;
	int end_x, end_y;
	int do_animation;
	BC_Pixmap *drag_pixmap;
};

#endif
