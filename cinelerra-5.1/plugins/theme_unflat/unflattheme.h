
/*
 * CINELERRA unflat theme
 * Based on S.U.V. theme
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * by Paolo Rampino <info at tuttoainternet dot it>
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

#ifndef DEFAULTunflattheme_H
#define DEFAULTunflattheme_H

#include "new.inc"
#include "plugintclient.h"
#include "preferencesthread.inc"
#include "statusbar.inc"
#include "theme.h"
#include "timebar.inc"

class UNFLATTHEME : public Theme
{
public:
	UNFLATTHEME();
	~UNFLATTHEME();

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



class UNFLATTHEMEMain : public PluginTClient
{
public:
	UNFLATTHEMEMain(PluginServer *server);
	~UNFLATTHEMEMain();

	const char* plugin_title();
	Theme* new_theme();

	UNFLATTHEME *theme;
};


#endif
