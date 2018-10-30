/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
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

#ifndef EFFECTLIST_H
#define EFFECTLIST_H

#include "awindowgui.inc"
#include "guicast.h"
#include "effectlist.inc"
#include "mwindow.inc"

class EffectTipDialog : public BC_DialogThread
{
public:
	EffectTipDialog(MWindow *mwindow, AWindow *awindow);
	~EffectTipDialog();
	void start(int x, int y, const char *effect, const char *text);
	BC_Window* new_gui();

	MWindow *mwindow;
	AWindow *awindow;
	EffectTipWindow *effect_gui;
	int x, y, w, h;
	const char *effect, *text;
};

class EffectTipWindow : public BC_Window
{
public:
	EffectTipWindow(AWindowGUI *gui, EffectTipDialog *thread);
	~EffectTipWindow();
	void create_objects();

	AWindowGUI *gui;
	EffectTipDialog *thread;
	BC_Title *tip_text;
};

class EffectTipItem : public BC_MenuItem
{
public:
	EffectTipItem(AWindowGUI *gui);
	~EffectTipItem();
	int handle_event();

	AWindowGUI *gui;
};

class EffectListMenu : public BC_PopupMenu
{
public:
	EffectListMenu(MWindow *mwindow, AWindowGUI *gui);
	~EffectListMenu();
	void create_objects();
	void update();

	MWindow *mwindow;
	AWindowGUI *gui;
	AWindowListFormat *format;
};

#endif
