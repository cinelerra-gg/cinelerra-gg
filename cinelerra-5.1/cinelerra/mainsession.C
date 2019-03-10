
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

#include "auto.h"
#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "guicast.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "meterpanel.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"

MainSession::MainSession(MWindow *mwindow)
{
	this->mwindow = mwindow;
	changes_made = 0;
	filename[0] = 0;
//	playback_cursor_visible = 0;
//	is_playing_back = 0;
	track_highlighted = 0;
	plugin_highlighted = 0;
	pluginset_highlighted = 0;
	edit_highlighted = 0;
	current_operation = NO_OPERATION;
	drag_pluginservers = new ArrayList<PluginServer*>;
	drag_plugin = 0;
	drag_assets = new ArrayList<Indexable*>;
	drag_auto_gang = new ArrayList<Auto*>;
	drag_clips = new ArrayList<EDL*>;
	drag_edits = new ArrayList<Edit*>;
	drag_edit = 0;
	drag_group = 0;
	drag_group_edit = 0;
	drag_group_position = 0;
	drag_group_first_track = 0;
	group_number = 1;
	clip_number = 1;
	brender_end = 0;
	cwindow_controls = 1;
	trim_edits = 0;
	current_tip = -1;
	selected_zwindow = -1;
	actual_frame_rate = 0;
	title_bar_alpha = 0;
	window_config = 0;
	a_x11_host[0] = 0;
	b_x11_host[0] = 0;
	record_scope = 0;

	drag_auto = 0;
	drag_button = 0;
	drag_handle = 0;
	drag_position = 0;
	drag_start = 0;
	drag_origin_x = drag_origin_y = 0;
	drag_start_percentage = 0;
	drag_start_position = 0;
	cwindow_output_x = cwindow_output_y = 0;
	batchrender_x = batchrender_y = batchrender_w = batchrender_h = 0;
	lwindow_x = lwindow_y = lwindow_w = lwindow_h = 0;
	mwindow_x = mwindow_y = mwindow_w = mwindow_h = 0;
	vwindow_x = vwindow_y = vwindow_w = vwindow_h = 0;
	cwindow_x = cwindow_y = cwindow_w = cwindow_h = 0;
	ctool_x = ctool_y = 0;
	awindow_x = awindow_y = awindow_w = awindow_h = 0;
	bwindow_w = bwindow_h = 0;
	rmonitor_x = rmonitor_y = rmonitor_w = rmonitor_h = 0;
	rwindow_x = rwindow_y = rwindow_w = rwindow_h = 0;
	gwindow_x = gwindow_y = 0;
	cswindow_x = cswindow_y = cswindow_w = cswindow_h = 0;
	swindow_x = swindow_y = swindow_w = swindow_h = 0;
	ewindow_w = ewindow_h = 0;
	channels_x = channels_y = 0;
	picture_x = picture_y = 0;
	scope_x = scope_y = scope_w = scope_h = 0;
	histogram_x = histogram_y = histogram_w = histogram_h = 0;
	use_hist = 0;
	use_wave = 0;
	use_vector = 0;
	use_hist_parade = 0;
	use_wave_parade = 0;
	afolders_w = 0;
	show_vwindow = show_awindow = show_cwindow = show_gwindow = show_lwindow = 0;
	plugindialog_w = plugindialog_h = 0;
//	presetdialog_w = presetdialog_h = 0;
	keyframedialog_w = keyframedialog_h = 0;
	keyframedialog_column1 = 0;
	keyframedialog_column2 = 0;
	keyframedialog_all = 0;
	menueffect_w = menueffect_h = 0;
	transitiondialog_w = transitiondialog_h = 0;
}

MainSession::~MainSession()
{
	delete drag_pluginservers;
	delete drag_assets;
	delete drag_auto_gang;
	delete drag_clips;
	delete drag_edits;
	if( drag_group )
		drag_group->remove_user();
}

void MainSession::boundaries()
{
	lwindow_x = MAX(0, lwindow_x);
	lwindow_y = MAX(0, lwindow_y);
	mwindow_x = MAX(0, mwindow_x);
	mwindow_y = MAX(0, mwindow_y);
	cwindow_x = MAX(0, cwindow_x);
	cwindow_y = MAX(0, cwindow_y);
	vwindow_x = MAX(0, vwindow_x);
	vwindow_y = MAX(0, vwindow_y);
	awindow_x = MAX(0, awindow_x);
	awindow_y = MAX(0, awindow_y);
	gwindow_x = MAX(0, gwindow_x);
	gwindow_y = MAX(0, gwindow_y);
	rwindow_x = MAX(0, rwindow_x);
	rwindow_y = MAX(0, rwindow_y);
	rmonitor_x = MAX(0, rmonitor_x);
	rmonitor_y = MAX(0, rmonitor_y);
	CLAMP(cwindow_controls, 0, 1);
}

void MainSession::save_x11_host(int play_config, const char *x11_host)
{
	strcpy(!play_config ? a_x11_host : b_x11_host, x11_host);
}

// set default x11 host, window_config, return screens
int MainSession::set_default_x11_host(int win_config)
{
	if( win_config < 0 ) win_config = window_config;
	const char *x11_host = win_config!=1 ? a_x11_host : b_x11_host;
	BC_DisplayInfo display_info(x11_host,0);
	int screen = display_info.get_screen();
	if( screen < 0 && strcmp(a_x11_host, b_x11_host) ) {
		win_config = win_config==1 ? 0 : 1;
		x11_host = win_config!=1 ? a_x11_host : b_x11_host;
		display_info.init_window(x11_host,0);
		screen = display_info.get_screen();
	}
	if( screen < 0 ) {
		x11_host = "";
		display_info.init_window(x11_host,1);
	}
	int screens = 1;
	if( display_info.get_screen_count() > 1 )
		screens = strcmp(a_x11_host, b_x11_host) != 0 ? 2 : 1;
	window_config = win_config;
	BC_Window::set_default_x11_host(x11_host);
	return screens;
}

void MainSession::default_window_positions(int window_config)
{
// 0 - all windows on a, playback_config a
// 1 - all windows on b, playback_config b
// 2 - all windows on a, except composer on b, playback_config b
	int screens = set_default_x11_host(window_config);
	mwindow->set_screens(screens);

	BC_DisplayInfo display_info(BC_Window::get_default_x11_host());
// Get defaults based on root window size
	int root_x = 0;
	int root_y = 0;
	int root_w = display_info.get_root_w();
	int root_h = display_info.get_root_h();

	int border_left = display_info.get_left_border();
	int border_right = display_info.get_right_border();
	int border_top = display_info.get_top_border();
	int border_bottom = display_info.get_bottom_border();

	int dual_head = screens > 1 ? 1 : 0;
	int left_w = 0, right_w = root_w;
	int xin_screens = display_info.get_xinerama_screens();
	if( xin_screens > 1 ) {
		dual_head = 1;
		int x, y, w, h;
		for( int s=0; s<xin_screens; ++s ) {
			if( display_info.xinerama_geometry(s, x, y, w, h) )
				continue;
			if( !y && !x ) {
				left_w = w;
				break;
			}
		}
		if( left_w > 0 ) {
			for( int s=0; s<xin_screens; ++s ) {
				if( display_info.xinerama_geometry(s, x, y, w, h) )
					continue;
				if( !y && x == left_w ) {
					right_w = w;
					screens = 2;
					break;
				}
			}
			if( window_config == 1 ) {
				root_x = left_w;
				root_w = right_w;
			}
			else {
			// use same aspect ratio to compute left height
				root_w = left_w;
				root_h = (root_w*root_h) / right_w;
			}
		}
	}
// Wider than 16:9, narrower than dual head
	if( screens < 2 && (float)root_w / root_h > 1.8 ) {
		dual_head = 1;
		switch( root_h ) {
		case 600:  right_w = 800;   break;
		case 720:  right_w = 1280;  break;
		case 1024: right_w = 1280;  break;
		case 1200: right_w = 1600;  break;
		case 1080: right_w = 1920;  break;
		default:   right_w = root_w/2;  break;
		}
		if( window_config == 1 ) {
			root_x = root_w - right_w;
			root_w = right_w;
		}
		else {
			// use same aspect ratio to compute left height
			root_w -= right_w;
			root_h = (root_w*root_h) / right_w;
		}
	}

	vwindow_x = root_x;
	vwindow_y = root_y;
	vwindow_w = root_w / 2 - border_left - border_right;
	vwindow_h = root_h * 6 / 10 - border_top - border_bottom;

	int b_root_w = root_w;
	int b_root_h = root_h;
	if( !dual_head || window_config != 2 ) {
		cwindow_x = root_x + root_w / 2;
		cwindow_y = root_y;
		cwindow_w = vwindow_w;
		cwindow_h = vwindow_h;
	}
	else {
// Get defaults based on root window size
		BC_DisplayInfo b_display_info(b_x11_host);
		b_root_w = b_display_info.get_root_w();
		b_root_h = b_display_info.get_root_h();
		cwindow_x = 50;
		cwindow_y = 50;
		cwindow_w = b_root_w-100;
		cwindow_h = b_root_h-100;
	}

	ctool_x = cwindow_x + cwindow_w / 2;
	ctool_y = cwindow_y + cwindow_h / 2;

	mwindow_x = root_x;
	mwindow_y = vwindow_y + vwindow_h + border_top + border_bottom;
	mwindow_w = root_w * 2 / 3 - border_left - border_right;
	mwindow_h = root_h - mwindow_y - border_top - border_bottom;

	awindow_x = mwindow_x + border_left + border_right + mwindow_w;
	awindow_y = mwindow_y;
	awindow_w = root_x + root_w - awindow_x - border_left - border_right;
	awindow_h = mwindow_h;

	bwindow_w = 600;
	bwindow_h = 360;

	ewindow_w = 640;
	ewindow_h = 240;

	channels_x = 0;
	channels_y = 0;
	picture_x = 0;
	picture_y = 0;
	scope_x = 0;
	scope_y = 0;
	scope_w = 640;
	scope_h = 320;
	histogram_x = 0;
	histogram_y = 0;
	histogram_w = 320;
	histogram_h = 480;
	record_scope = 0;
	use_hist = 1;
	use_wave = 1;
	use_vector = 1;
	use_hist_parade = 1;
	use_wave_parade = 1;

	if(mwindow->edl)
		lwindow_w = MeterPanel::get_meters_width(mwindow->theme,
			mwindow->edl->session->audio_channels,
			1);
	else
		lwindow_w = 100;

	lwindow_y = 0;
	lwindow_x = root_w - lwindow_w;
	lwindow_h = mwindow_y;

	rwindow_x = root_x;
	rwindow_y = root_y;
	rwindow_h = 500;
	rwindow_w = 650;

	cswindow_x = root_x;
	cswindow_y = root_y;
	cswindow_w = 1280;
	cswindow_h = 600;

	if( !dual_head || window_config != 2 ) {
		rmonitor_x = rwindow_x + rwindow_w + 10;
		rmonitor_y = rwindow_y;
		rmonitor_w = root_x + root_w - rmonitor_x;
		rmonitor_h = rwindow_h;
	}
	else {
		rmonitor_x = cswindow_x = 50;
		rmonitor_y = cswindow_y = 50;
		rmonitor_w = b_root_w-100;
		rmonitor_h = b_root_h-100;
		if( cswindow_w < rmonitor_w ) cswindow_w = rmonitor_w;
		if( cswindow_h < rmonitor_h ) cswindow_h = rmonitor_h;
	}

	swindow_x = root_x;
	swindow_y = root_y;
	swindow_w = 600;
	swindow_h = 400;

	batchrender_w = 750;
	batchrender_h = 400;
	batchrender_x = root_w / 2 - batchrender_w / 2;
	batchrender_y = root_h / 2 - batchrender_h / 2;
}

int MainSession::load_defaults(BC_Hash *defaults)
{
// Setup main windows
	strcpy(a_x11_host, defaults->get("A_X11_HOST", a_x11_host));
	strcpy(b_x11_host, defaults->get("B_X11_HOST", b_x11_host));
	window_config = defaults->get("WINDOW_CONFIG", window_config);
	default_window_positions(window_config);

	vwindow_x = defaults->get("VWINDOW_X", vwindow_x);
	vwindow_y = defaults->get("VWINDOW_Y", vwindow_y);
	vwindow_w = defaults->get("VWINDOW_W", vwindow_w);
	vwindow_h = defaults->get("VWINDOW_H", vwindow_h);


	cwindow_x = defaults->get("CWINDOW_X", cwindow_x);
	cwindow_y = defaults->get("CWINDOW_Y", cwindow_y);
	cwindow_w = defaults->get("CWINDOW_W", cwindow_w);
	cwindow_h = defaults->get("CWINDOW_H", cwindow_h);

	ctool_x = defaults->get("CTOOL_X", ctool_x);
	ctool_y = defaults->get("CTOOL_Y", ctool_y);

	gwindow_x = defaults->get("GWINDOW_X", gwindow_x);
	gwindow_y = defaults->get("GWINDOW_Y", gwindow_y);

	mwindow_x = defaults->get("MWINDOW_X", mwindow_x);
	mwindow_y = defaults->get("MWINDOW_Y", mwindow_y);
	mwindow_w = defaults->get("MWINDOW_W", mwindow_w);
	mwindow_h = defaults->get("MWINDOW_H", mwindow_h);

	lwindow_x = defaults->get("LWINDOW_X", lwindow_x);
	lwindow_y = defaults->get("LWINDOW_Y", lwindow_y);
	lwindow_w = defaults->get("LWINDOW_W", lwindow_w);
	lwindow_h = defaults->get("LWINDOW_H", lwindow_h);


	awindow_x = defaults->get("AWINDOW_X", awindow_x);
	awindow_y = defaults->get("AWINDOW_Y", awindow_y);
	awindow_w = defaults->get("AWINDOW_W", awindow_w);
	awindow_h = defaults->get("AWINDOW_H", awindow_h);

	ewindow_w = defaults->get("EWINDOW_W", ewindow_w);
	ewindow_h = defaults->get("EWINDOW_H", ewindow_h);

	channels_x = defaults->get("CHANNELS_X", channels_x);
	channels_y = defaults->get("CHANNELS_Y", channels_y);
	picture_x = defaults->get("PICTURE_X", picture_x);
	picture_y = defaults->get("PICTURE_Y", picture_y);
	scope_x = defaults->get("SCOPE_X", scope_x);
	scope_y = defaults->get("SCOPE_Y", scope_y);
	scope_w = defaults->get("SCOPE_W", scope_w);
	scope_h = defaults->get("SCOPE_H", scope_h);
	histogram_x = defaults->get("HISTOGRAM_X", histogram_x);
	histogram_y = defaults->get("HISTOGRAM_Y", histogram_y);
	histogram_w = defaults->get("HISTOGRAM_W", histogram_w);
	histogram_h = defaults->get("HISTOGRAM_H", histogram_h);
	record_scope = defaults->get("RECORD_SCOPE", record_scope);
	use_hist = defaults->get("USE_HIST", use_hist);
	use_wave = defaults->get("USE_WAVE", use_wave);
	use_vector = defaults->get("USE_VECTOR", use_vector);
	use_hist_parade = defaults->get("USE_HIST_PARADE", use_hist_parade);
	use_wave_parade = defaults->get("USE_WAVE_PARADE", use_wave_parade);

//printf("MainSession::load_defaults 1\n");

// Other windows
	afolders_w = defaults->get("ABINS_W", 200);

	bwindow_w = defaults->get("BWINDOW_W", bwindow_w);
	bwindow_h = defaults->get("BWINDOW_H", bwindow_h);

	rwindow_x = defaults->get("RWINDOW_X", rwindow_x);
	rwindow_y = defaults->get("RWINDOW_Y", rwindow_y);
	rwindow_w = defaults->get("RWINDOW_W", rwindow_w);
	rwindow_h = defaults->get("RWINDOW_H", rwindow_h);

	cswindow_x = defaults->get("CSWINDOW_X", cswindow_x);
	cswindow_y = defaults->get("CSWINDOW_Y", cswindow_y);
	cswindow_w = defaults->get("CSWINDOW_W", cswindow_w);
	cswindow_h = defaults->get("CSWINDOW_H", cswindow_h);

	swindow_x = defaults->get("SWINDOW_X", swindow_x);
	swindow_y = defaults->get("SWINDOW_Y", swindow_y);
	swindow_w = defaults->get("SWINDOW_W", swindow_w);
	swindow_h = defaults->get("SWINDOW_H", swindow_h);

	rmonitor_x = defaults->get("RMONITOR_X", rmonitor_x);
	rmonitor_y = defaults->get("RMONITOR_Y", rmonitor_y);
	rmonitor_w = defaults->get("RMONITOR_W", rmonitor_w);
	rmonitor_h = defaults->get("RMONITOR_H", rmonitor_h);

	batchrender_x = defaults->get("BATCHRENDER_X", batchrender_x);
	batchrender_y = defaults->get("BATCHRENDER_Y", batchrender_y);
	batchrender_w = defaults->get("BATCHRENDER_W", batchrender_w);
	batchrender_h = defaults->get("BATCHRENDER_H", batchrender_h);

	show_vwindow = defaults->get("SHOW_VWINDOW", 1);
	show_awindow = defaults->get("SHOW_AWINDOW", 1);
	show_cwindow = defaults->get("SHOW_CWINDOW", 1);
	show_lwindow = defaults->get("SHOW_LWINDOW", 0);
	show_gwindow = defaults->get("SHOW_GWINDOW", 0);

	cwindow_controls = defaults->get("CWINDOW_CONTROLS", cwindow_controls);

	plugindialog_w = defaults->get("PLUGINDIALOG_W", 510);
	plugindialog_h = defaults->get("PLUGINDIALOG_H", 415);
//	presetdialog_w = defaults->get("PRESETDIALOG_W", 510);
//	presetdialog_h = defaults->get("PRESETDIALOG_H", 415);
	keyframedialog_w = defaults->get("KEYFRAMEDIALOG_W", 320);
	keyframedialog_h = defaults->get("KEYFRAMEDIALOG_H", 415);
	keyframedialog_column1 = defaults->get("KEYFRAMEDIALOG_COLUMN1", 150);
	keyframedialog_column2 = defaults->get("KEYFRAMEDIALOG_COLUMN2", 100);
	keyframedialog_all = defaults->get("KEYFRAMEDIALOG_ALL", 0);
	menueffect_w = defaults->get("MENUEFFECT_W", 580);
	menueffect_h = defaults->get("MENUEFFECT_H", 350);
	transitiondialog_w = defaults->get("TRANSITIONDIALOG_W", 320);
	transitiondialog_h = defaults->get("TRANSITIONDIALOG_H", 512);

	current_tip = defaults->get("CURRENT_TIP", current_tip);
	actual_frame_rate = defaults->get("ACTUAL_FRAME_RATE", (float)-1);
	title_bar_alpha = defaults->get("TITLE_BAR_ALPHA", (float)1);

	boundaries();
	return 0;
}

int MainSession::save_defaults(BC_Hash *defaults)
{
	defaults->update("A_X11_HOST", a_x11_host);
	defaults->update("B_X11_HOST", b_x11_host);
	defaults->update("WINDOW_CONFIG", window_config);
// Window positions
	defaults->update("MWINDOW_X", mwindow_x);
	defaults->update("MWINDOW_Y", mwindow_y);
	defaults->update("MWINDOW_W", mwindow_w);
	defaults->update("MWINDOW_H", mwindow_h);

	defaults->update("LWINDOW_X", lwindow_x);
	defaults->update("LWINDOW_Y", lwindow_y);
	defaults->update("LWINDOW_W", lwindow_w);
	defaults->update("LWINDOW_H", lwindow_h);

	defaults->update("VWINDOW_X", vwindow_x);
	defaults->update("VWINDOW_Y", vwindow_y);
	defaults->update("VWINDOW_W", vwindow_w);
	defaults->update("VWINDOW_H", vwindow_h);

	defaults->update("CWINDOW_X", cwindow_x);
	defaults->update("CWINDOW_Y", cwindow_y);
	defaults->update("CWINDOW_W", cwindow_w);
	defaults->update("CWINDOW_H", cwindow_h);

	defaults->update("CTOOL_X", ctool_x);
	defaults->update("CTOOL_Y", ctool_y);

	defaults->update("GWINDOW_X", gwindow_x);
	defaults->update("GWINDOW_Y", gwindow_y);

	defaults->update("AWINDOW_X", awindow_x);
	defaults->update("AWINDOW_Y", awindow_y);
	defaults->update("AWINDOW_W", awindow_w);
	defaults->update("AWINDOW_H", awindow_h);

	defaults->update("BWINDOW_W", bwindow_w);
	defaults->update("BWINDOW_H", bwindow_h);

	defaults->update("EWINDOW_W", ewindow_w);
	defaults->update("EWINDOW_H", ewindow_h);

	defaults->update("CHANNELS_X", channels_x);
	defaults->update("CHANNELS_Y", channels_y);
	defaults->update("PICTURE_X", picture_x);
	defaults->update("PICTURE_Y", picture_y);
	defaults->update("SCOPE_X", scope_x);
	defaults->update("SCOPE_Y", scope_y);
	defaults->update("SCOPE_W", scope_w);
	defaults->update("SCOPE_H", scope_h);
	defaults->update("HISTOGRAM_X", histogram_x);
	defaults->update("HISTOGRAM_Y", histogram_y);
	defaults->update("HISTOGRAM_W", histogram_w);
	defaults->update("HISTOGRAM_H", histogram_h);
	defaults->update("RECORD_SCOPE", record_scope);
	defaults->update("USE_HIST", use_hist);
	defaults->update("USE_WAVE", use_wave);
	defaults->update("USE_VECTOR", use_vector);
	defaults->update("USE_HIST_PARADE", use_hist_parade);
	defaults->update("USE_WAVE_PARADE", use_wave_parade);

 	defaults->update("ABINS_W", afolders_w);

	defaults->update("RMONITOR_X", rmonitor_x);
	defaults->update("RMONITOR_Y", rmonitor_y);
	defaults->update("RMONITOR_W", rmonitor_w);
	defaults->update("RMONITOR_H", rmonitor_h);

	defaults->update("RWINDOW_X", rwindow_x);
	defaults->update("RWINDOW_Y", rwindow_y);
	defaults->update("RWINDOW_W", rwindow_w);
	defaults->update("RWINDOW_H", rwindow_h);

	defaults->update("CSWINDOW_X", cswindow_x);
	defaults->update("CSWINDOW_Y", cswindow_y);
	defaults->update("CSWINDOW_W", cswindow_w);
	defaults->update("CSWINDOW_H", cswindow_h);

	defaults->update("SWINDOW_X", swindow_x);
	defaults->update("SWINDOW_Y", swindow_y);
	defaults->update("SWINDOW_W", swindow_w);
	defaults->update("SWINDOW_H", swindow_h);

	defaults->update("BATCHRENDER_X", batchrender_x);
	defaults->update("BATCHRENDER_Y", batchrender_y);
	defaults->update("BATCHRENDER_W", batchrender_w);
	defaults->update("BATCHRENDER_H", batchrender_h);

	defaults->update("SHOW_VWINDOW", show_vwindow);
	defaults->update("SHOW_AWINDOW", show_awindow);
	defaults->update("SHOW_CWINDOW", show_cwindow);
	defaults->update("SHOW_LWINDOW", show_lwindow);
	defaults->update("SHOW_GWINDOW", show_gwindow);

	defaults->update("CWINDOW_CONTROLS", cwindow_controls);

	defaults->update("PLUGINDIALOG_W", plugindialog_w);
	defaults->update("PLUGINDIALOG_H", plugindialog_h);
//	defaults->update("PRESETDIALOG_W", presetdialog_w);
//	defaults->update("PRESETDIALOG_H", presetdialog_h);
	defaults->update("KEYFRAMEDIALOG_W", keyframedialog_w);
	defaults->update("KEYFRAMEDIALOG_H", keyframedialog_h);
	defaults->update("KEYFRAMEDIALOG_COLUMN1", keyframedialog_column1);
	defaults->update("KEYFRAMEDIALOG_COLUMN2", keyframedialog_column2);
	defaults->update("KEYFRAMEDIALOG_ALL", keyframedialog_all);

	defaults->update("MENUEFFECT_W", menueffect_w);
	defaults->update("MENUEFFECT_H", menueffect_h);

	defaults->update("TRANSITIONDIALOG_W", transitiondialog_w);
	defaults->update("TRANSITIONDIALOG_H", transitiondialog_h);

	defaults->update("ACTUAL_FRAME_RATE", actual_frame_rate);
	defaults->update("TITLE_BAR_ALPHA", title_bar_alpha);
	defaults->update("CURRENT_TIP", current_tip);


	return 0;
}

Track *MainSession::drag_handle_track()
{
	Track *track = 0;
	switch( current_operation ) {
	case DRAG_EDITHANDLE1:
	case DRAG_EDITHANDLE2:
		track = drag_edit->edits->track;
		break;
	case DRAG_PLUGINHANDLE1:
	case DRAG_PLUGINHANDLE2:
		track = drag_plugin->edits->track;
		break;
	}
	return track;
}

void MainSession::update_clip_number()
{
	int clip_no = 0;
	for( int i=mwindow->edl->clips.size(); --i>=0; ) {
		EDL *clip_edl = mwindow->edl->clips[i];
		int no = 0;
		if( sscanf(clip_edl->local_session->clip_title,_("Clip %d"),&no) == 1 )
			if( no > clip_no ) clip_no = no;
	}
	clip_number = clip_no+1;
}

int MainSession::load_file(const char *path)
{
	int ret = 1;
	FILE *fp = fopen(path,"r");
	if( fp ) {
		BC_Hash defaults;
		defaults.load_file(fp);
		load_defaults(&defaults);
		fclose(fp);
		ret = 0;
	}
	return ret;
}

int MainSession::save_file(const char *path)
{
	int ret = 1;
	FILE *fp = fopen(path,"w");
	if( fp ) {
		BC_Hash defaults;
		save_defaults(&defaults);
		defaults.save_file(fp);
		fclose(fp);
		ret = 0;
	}
	return ret;
}

