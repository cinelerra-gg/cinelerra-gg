
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

#include "bcpopup.h"
#include "language.h"


BC_FullScreen::BC_FullScreen(BC_WindowBase *parent_window, int w, int h,
		int bg_color, int vm_scale, int hide, BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
#ifdef HAVE_LIBXXF86VM
   if (vm_scale)
	   create_window(parent_window, _("Fullscreen"),
		   parent_window->get_screen_x(0, -1), parent_window->get_screen_y(0, -1),
		   w, h, w, h, 0, parent_window->top_level->private_color, hide,
		   bg_color, NULL, VIDMODE_SCALED_WINDOW, bg_pixmap, 0);
   else
#endif
   create_window(parent_window, _("Fullscreen"),
		   parent_window->get_screen_x(0, -1), parent_window->get_screen_y(0, -1),
		   w, h, w, h, 0, parent_window->top_level->private_color, hide,
		   bg_color, NULL, POPUP_WINDOW, bg_pixmap, 0);
}


BC_FullScreen::~BC_FullScreen()
{
}


BC_Popup::BC_Popup(BC_WindowBase *parent_window,
		int x, int y, int w, int h, int bg_color, int hide, BC_Pixmap *bg_pixmap)
 : BC_WindowBase()
{
	create_window(parent_window,
		_("Popup"), x, y, w, h, w, h, 0, parent_window->top_level->private_color,
		hide, bg_color, NULL, POPUP_WINDOW, bg_pixmap, 0);
	grabbed = 0;
}


BC_Popup::~BC_Popup()
{
}


void BC_Popup::grab_keyboard()
{
	if( !grabbed ) {
		lock_window("BC_Popup::grab_keyboard");
		if( !getenv("NO_KEYBOARD_GRAB") )
			XGrabKeyboard(top_level->display, win, False,
				GrabModeAsync, GrabModeAsync, CurrentTime);
		else
			XSetInputFocus(top_level->display, win, RevertToParent, CurrentTime);
		unlock_window();  flush();  //  printf("grabbed\n");
		set_active_subwindow(this);
		grabbed = 1;
	}
}

void BC_Popup::ungrab_keyboard()
{
	if( grabbed ) {
		lock_window("BC_Popup::ungrab_keyboard");
		if( !getenv("NO_KEYBOARD_GRAB") )
			XUngrabKeyboard(top_level->display, CurrentTime);
		unlock_window();  flush(); //  printf("ungrabbed\n");
		set_active_subwindow(0);
		grabbed = 0;
	}
}

