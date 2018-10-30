
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

#ifndef DEFAULTdefaulttheme_H
#define DEFAULTdefaulttheme_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class BlondTheme : public Theme
{
public:
	BlondTheme();
	~BlondTheme();

	void initialize();
	void draw_mwindow_bg(MWindowGUI *gui);

	void draw_rwindow_bg(RecordGUI *gui);
	void draw_rmonitor_bg(RecordMonitorGUI *gui);
	void draw_cwindow_bg(CWindowGUI *gui);
	void draw_vwindow_bg(VWindowGUI *gui);
	void draw_preferences_bg(PreferencesWindow *gui);


	void draw_new_bg(NewWindow *gui);
	void draw_setformat_bg(SetFormatWindow *gui);

private:
	void build_bg_data();
	void build_patches();
	void build_overlays();
};



class BlondThemeMain : public PluginTClient
{
public:
	BlondThemeMain(PluginServer *server);
	~BlondThemeMain();

	const char* plugin_title();
	Theme* new_theme();

	BlondTheme *theme;
};


#endif
