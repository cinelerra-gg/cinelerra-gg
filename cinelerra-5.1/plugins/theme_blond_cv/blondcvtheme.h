
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

#ifndef BLONDTHEME_H
#define BLONDTHEME_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class BlondCVTheme : public Theme
{
public:
	BlondCVTheme();
	~BlondCVTheme();

	void initialize();
	void draw_mwindow_bg(MWindowGUI *gui);

	void draw_rwindow_bg(RecordGUI *gui);
	void draw_rmonitor_bg(RecordMonitorGUI *gui);
	void draw_cwindow_bg(CWindowGUI *gui);
	void draw_vwindow_bg(VWindowGUI *gui);
	void draw_preferences_bg(PreferencesWindow *gui);

	void get_mwindow_sizes(MWindowGUI *gui, int w, int h);
	void get_cwindow_sizes(CWindowGUI *gui, int cwindow_controls);
	void get_vwindow_sizes(VWindowGUI *gui);
	void get_preferences_sizes();
	void get_recordgui_sizes(RecordGUI *gui, int w, int h);
//	void get_rmonitor_sizes(int do_audio,
//		int do_video,
//		int do_channel,
//		int do_interlace,
//		int do_avc,
//		int audio_channels);

	void get_new_sizes(NewWindow *gui);
	void draw_new_bg(NewWindow *gui);
	void draw_setformat_bg(SetFormatWindow *gui);
	void get_plugindialog_sizes();

private:
	void build_icons();
	void build_bg_data();
	void build_patches();
	void build_overlays();


// MWindow
	VFrame *mbutton_left;
	VFrame *mbutton_right;
	VFrame *tracks_bg;
	VFrame *zoombar_left;
	VFrame *zoombar_right;
	VFrame *statusbar_left;
	VFrame *statusbar_right;

// CWindow
	VFrame *cpanel_bg;
	VFrame *cbuttons_left;
	VFrame *cbuttons_right;
	VFrame *cmeter_bg;

// VWindow
	VFrame *vbuttons_left;
	VFrame *vbuttons_right;
	VFrame *vmeter_bg;

	VFrame *preferences_bg;
	VFrame *new_bg;
	VFrame *setformat_bg;

// Record windows
	VFrame *rgui_batch;
	VFrame *rgui_controls;
	VFrame *rgui_list;
	VFrame *rmonitor_panel;
	VFrame *rmonitor_meters;
};



class BlondCVThemeMain : public PluginTClient
{
public:
	BlondCVThemeMain(PluginServer *server);
	~BlondCVThemeMain();

	const char* plugin_title();
	Theme* new_theme();

	BlondCVTheme *theme;
};


#endif
