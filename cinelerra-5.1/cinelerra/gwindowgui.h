
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

#ifndef GWINDOWGUI_H
#define GWINDOWGUI_H

#include "automation.inc"
#include "condition.inc"
#include "colorpicker.h"
#include "guicast.h"
#include "gwindowgui.inc"
#include "mwindow.inc"

enum {
	NONAUTOTOGGLES_ASSETS,
	NONAUTOTOGGLES_TITLES,
	NONAUTOTOGGLES_TRANSITIONS,
	NONAUTOTOGGLES_PLUGIN_AUTOS,
	NONAUTOTOGGLES_CAMERA_XYZ,
	NONAUTOTOGGLES_PROJECTOR_XYZ,
	NONAUTOTOGGLES_BAR1,
	NONAUTOTOGGLES_BAR2,
	NONAUTOTOGGLES_BAR3,
	NONAUTOTOGGLES_COUNT
};

struct toggleinfo {
	int isauto, ref;
};

class GWindowGUI : public BC_Window
{
public:
	GWindowGUI(MWindow *mwindow, int w, int h);
	~GWindowGUI();
	static void calculate_extents(BC_WindowBase *gui, int *w, int *h);
	void create_objects();
	int translation_event();
	int close_event();
	int keypress_event();
	void update_toggles(int use_lock);
	void toggle_camera_xyz();
	void toggle_projector_xyz();
	void update_mwindow(int toggles, int overlays);
	void load_defaults();
	void save_defaults();
	int *get_main_value(toggleinfo *info);
	int check_xyz(int group);
	void xyz_check(int group, int v);
	void set_cool(int reset, int all=0);
	void set_hot(GWindowToggle *toggle);

	static const char *non_auto_text[];
	static const char *auto_text[];
	static const char *xyz_group[];
	static const char *xyz_accel[];
	static int auto_colors[];
	static const char *toggle_text(toggleinfo *tp);

	MWindow *mwindow;
	GWindowToggle *toggles[NONAUTOTOGGLES_COUNT + AUTOMATION_TOTAL];
	GWindowToggle *camera_xyz, *projector_xyz;
};

class GWindowToggle : public BC_CheckBox
{
public:
	GWindowToggle(GWindowGUI *gui, int x, int y,
		const char *text, int color, toggleinfo *info);
	~GWindowToggle();

	int handle_event();
	void update();
	void update_gui(int color);
	int draw_face(int flash, int flush);

	int color;
	int hot, hot_value;
	toggleinfo *info;
	GWindowGUI *gui;
	GWindowColorButton *color_button;
};

class GWindowColorButton : public ColorCircleButton
{
public:
	GWindowColorButton(GWindowToggle *auto_toggle,
		int x, int y, int w, int color);
	~GWindowColorButton();
	int handle_new_color(int color, int alpha);
	void handle_done_event(int result);

	GWindowToggle *auto_toggle;
};

#endif
