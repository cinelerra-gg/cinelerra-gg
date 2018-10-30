
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

#include "bcdragwindow.h"
#include "bcbitmap.h"
#include "bcpixmap.h"

#include "vframe.h"
#include <unistd.h>

BC_DragWindow::BC_DragWindow(BC_WindowBase *parent_window,
	BC_Pixmap *pixmap, int center_x, int center_y)
 : BC_Popup(parent_window,
	center_x - pixmap->get_w() / 2, center_y - pixmap->get_h() / 2,
	pixmap->get_w(), pixmap->get_h(), -1, 0,
	prepare_pixmap(pixmap, parent_window))
{
	drag_pixmap = 0;
	init_x = get_x();
	init_y = get_y();
	end_x = BC_INFINITY;
	end_y = BC_INFINITY;
	do_animation = 1;
}


BC_DragWindow::BC_DragWindow(BC_WindowBase *parent_window,
	VFrame *frame, int center_x, int center_y)
 : BC_Popup(parent_window,
	center_x - frame->get_w() / 2, center_y - frame->get_h() / 2,
	frame->get_w(), frame->get_h(), -1, 0,
	prepare_frame(frame, parent_window))
{
	init_x = get_x();
	init_y = get_y();
	end_x = BC_INFINITY;
	end_y = BC_INFINITY;
	do_animation = 1;
}

BC_DragWindow::~BC_DragWindow()
{
	delete drag_pixmap;
}

int BC_DragWindow::cursor_motion_event()
{
	int cx, cy;
	get_abs_cursor(cx, cy);
	cx -= get_w() / 2;
	cy -= get_h() / 2;
	reposition_window(cx, cy, get_w(), get_h());
	flush();
	return 1;
}
int BC_DragWindow::button_release_event()
{
	cursor_motion_event();
	sync();
	return BC_WindowBase::button_release_event();
}

int BC_DragWindow::drag_failure_event()
{
	if(!do_animation) return 0;

	if(end_x == BC_INFINITY) {
		end_x = get_x();
		end_y = get_y();
	}

	for(int i = 0; i < 10; i++) {
		int new_x = end_x + (init_x - end_x) * i / 10;
		int new_y = end_y + (init_y - end_y) * i / 10;

		reposition_window(new_x, new_y, get_w(), get_h());
		flush();
		usleep(250000/10);
	}
	return 0;
}

void BC_DragWindow::set_animation(int value)
{
	this->do_animation = value;
}

BC_Pixmap *BC_DragWindow::prepare_frame(VFrame *frame, BC_WindowBase *parent_window)
{
	VFrame *temp_frame = frame;
	int tw = frame->get_w(), th = frame->get_h();

	if( frame->get_color_model() != BC_RGBA8888 ) {
		temp_frame = new VFrame(tw, th, BC_RGBA8888, 0);
		temp_frame->transfer_from(frame);
	}

	int tx = tw/2, ty = th/2, tx1 = tx-1, ty1 = ty-1, tx2 = tx+2, ty2 = ty+2;
	int bpp = BC_CModels::calculate_pixelsize(temp_frame->get_color_model());
	unsigned char **rows = temp_frame->get_rows();
	for( int y=ty1; y<ty2; ++y ) {
		for( int x=tx1; x<tx2; ++x ) {
			unsigned char *rp = rows[y] + x*bpp;
			rp[3] = 0; // alpha of center pixels = 0
		}
	}
	drag_pixmap = new BC_Pixmap(parent_window, temp_frame, PIXMAP_ALPHA);

	if( temp_frame != frame )
		delete temp_frame;
	return drag_pixmap;
}

BC_Pixmap *BC_DragWindow::prepare_pixmap(BC_Pixmap *pixmap, BC_WindowBase *parent_window)
{
	int pix_w = pixmap->get_w(), pix_h = pixmap->get_h();
	BC_Bitmap bitmap(parent_window, pix_w, pix_h, BC_RGB888, 0);
	Pixmap xpixmap = pixmap->get_pixmap();
	VFrame frame(pix_w, pix_h, BC_RGB888);
	bitmap.read_drawable(xpixmap, 0,0,&frame);
	return prepare_frame(&frame, parent_window);
}

