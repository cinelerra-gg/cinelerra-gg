
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

#include "autoconf.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "gwindowgui.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "tracks.h"
#include "trackcanvas.h"
#include "zoombar.h"

#include <math.h>



GWindowGUI::GWindowGUI(MWindow *mwindow, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Overlays"),
	mwindow->session->gwindow_x, mwindow->session->gwindow_y,
	w, h, w, h, 0, 0, 1)
{
	this->mwindow = mwindow;
	camera_xyz = 0;
	projector_xyz = 0;
}

GWindowGUI::~GWindowGUI()
{
}

const char *GWindowGUI::non_auto_text[NON_AUTOMATION_TOTAL] =
{
	N_("Assets"),
	N_("Titles"),
	N_("Transitions"),
	N_("Plugin Keyframes"),
};

const char *GWindowGUI::auto_text[AUTOMATION_TOTAL] =
{
	N_("Mute"),
	N_("Camera X"),
	N_("Camera Y"),
	N_("Camera Z"),
	N_("Projector X"),
	N_("Projector Y"),
	N_("Projector Z"),
	N_("Fade"),
	N_("Pan"),
	N_("Mode"),
	N_("Mask"),
	N_("Speed")
};

int GWindowGUI::auto_colors[AUTOMATION_TOTAL] =
{
	PINK,
	RED,
	GREEN,
	BLUE,
	LTPINK,
	LTGREEN,
	LTBLUE,
	LTPURPLE,
	0,
	0,
	0,
	ORANGE,
};

void GWindowGUI::load_defaults()
{
	BC_Hash *defaults = mwindow->defaults;
	auto_colors[AUTOMATION_MUTE] = defaults->get("AUTO_COLOR_MUTE", auto_colors[AUTOMATION_MUTE]);
	auto_colors[AUTOMATION_CAMERA_X] = defaults->get("AUTO_COLOR_CAMERA_X", auto_colors[AUTOMATION_CAMERA_X]);
	auto_colors[AUTOMATION_CAMERA_Y] = defaults->get("AUTO_COLOR_CAMERA_Y", auto_colors[AUTOMATION_CAMERA_Y]);
	auto_colors[AUTOMATION_CAMERA_Z] = defaults->get("AUTO_COLOR_CAMERA_Z", auto_colors[AUTOMATION_CAMERA_Z]);
	auto_colors[AUTOMATION_PROJECTOR_X] = defaults->get("AUTO_COLOR_PROJECTOR_X", auto_colors[AUTOMATION_PROJECTOR_X]);
	auto_colors[AUTOMATION_PROJECTOR_Y] = defaults->get("AUTO_COLOR_PROJECTOR_Y", auto_colors[AUTOMATION_PROJECTOR_Y]);
	auto_colors[AUTOMATION_PROJECTOR_Z] = defaults->get("AUTO_COLOR_PROJECTOR_Z", auto_colors[AUTOMATION_PROJECTOR_Z]);
	auto_colors[AUTOMATION_FADE] = defaults->get("AUTO_COLOR_FADE", auto_colors[AUTOMATION_FADE]);
	auto_colors[AUTOMATION_SPEED] = defaults->get("AUTO_COLOR_SPEED", auto_colors[AUTOMATION_SPEED]);
}

void GWindowGUI::save_defaults()
{
	BC_Hash *defaults = mwindow->defaults;
	defaults->update("AUTO_COLOR_MUTE", auto_colors[AUTOMATION_MUTE]);
	defaults->update("AUTO_COLOR_CAMERA_X", auto_colors[AUTOMATION_CAMERA_X]);
	defaults->update("AUTO_COLOR_CAMERA_Y", auto_colors[AUTOMATION_CAMERA_Y]);
	defaults->update("AUTO_COLOR_CAMERA_Z", auto_colors[AUTOMATION_CAMERA_Z]);
	defaults->update("AUTO_COLOR_PROJECTOR_X", auto_colors[AUTOMATION_PROJECTOR_X]);
	defaults->update("AUTO_COLOR_PROJECTOR_Y", auto_colors[AUTOMATION_PROJECTOR_Y]);
	defaults->update("AUTO_COLOR_PROJECTOR_Z", auto_colors[AUTOMATION_PROJECTOR_Z]);
	defaults->update("AUTO_COLOR_FADE", auto_colors[AUTOMATION_FADE]);
	defaults->update("AUTO_COLOR_SPEED", auto_colors[AUTOMATION_SPEED]);
}

static toggleinfo toggle_order[] =
{
	{0, NON_AUTOMATION_ASSETS},
	{0, NON_AUTOMATION_TITLES},
	{0, NON_AUTOMATION_TRANSITIONS},
	{0, NON_AUTOMATION_PLUGIN_AUTOS},
	{0, -1}, // bar
	{1, AUTOMATION_FADE},
	{1, AUTOMATION_MUTE},
	{1, AUTOMATION_SPEED},
	{1, AUTOMATION_MODE},
	{1, AUTOMATION_PAN},
	{1, AUTOMATION_MASK},
	{0, -1}, // bar
	{1, AUTOMATION_CAMERA_X},
	{1, AUTOMATION_CAMERA_Y},
	{1, AUTOMATION_CAMERA_Z},
	{-1, NONAUTOTOGGLES_CAMERA_XYZ},
	{0, -1}, // bar
	{1, AUTOMATION_PROJECTOR_X},
	{1, AUTOMATION_PROJECTOR_Y},
	{1, AUTOMATION_PROJECTOR_Z},
	{-1, NONAUTOTOGGLES_PROJECTOR_XYZ},
};

const char *GWindowGUI::toggle_text(toggleinfo *tp)
{
	if( tp->isauto > 0 ) return _(auto_text[tp->ref]);
	if( !tp->isauto ) return _(non_auto_text[tp->ref]);
	return _("XYZ");
}

void GWindowGUI::calculate_extents(BC_WindowBase *gui, int *w, int *h)
{
	int temp1, temp2, temp3, temp4, temp5, temp6, temp7;
	int current_w, current_h;
	*w = 10;
	*h = 10;

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		toggleinfo *tp = &toggle_order[i];
		int ref = tp->ref;
		if( ref < 0 ) {
			*h += get_resources()->bar_data->get_h() + 5;
			continue;
		}
		BC_Toggle::calculate_extents(gui,
			BC_WindowBase::get_resources()->checkbox_images,
			0, &temp1, &current_w, &current_h,
			&temp2, &temp3, &temp4, &temp5, &temp6, &temp7,
			toggle_text(tp), MEDIUMFONT);
		current_w += current_h;
		*w = MAX(current_w, *w);
		*h += current_h + 5;
	}

	*h += 10;
	*w += 20;
}

GWindowColorButton::GWindowColorButton(GWindowToggle *auto_toggle,
		int x, int y, int w, int color)
 : ColorCircleButton(auto_toggle->caption, x, y, w, w, color, -1, 1)
{
	this->auto_toggle = auto_toggle;
	this->color = color;
}

GWindowColorButton::~GWindowColorButton()
{
}

int GWindowColorButton::handle_new_color(int color, int alpha)
{
	this->color = color;
	color_thread->update_lock->unlock();
	return 1;
}

void GWindowColorButton::handle_done_event(int result)
{
	ColorCircleButton::handle_done_event(result);
	int ref = auto_toggle->info->ref;
	GWindowGUI *gui = auto_toggle->gui;
	gui->lock_window("GWindowColorThread::handle_done_event");
	if( !result ) {
		GWindowGUI::auto_colors[ref] = color;
		auto_toggle->update_gui(color);
		gui->save_defaults();
	}
	else {
		color = GWindowGUI::auto_colors[ref];
		update_gui(color);
	}
	gui->unlock_window();
	MWindowGUI *mwindow_gui = gui->mwindow->gui;
	mwindow_gui->lock_window("GWindowColorUpdate::run");
	mwindow_gui->draw_overlays(1);
	mwindow_gui->unlock_window();
}


void GWindowGUI::create_objects()
{
	int x = 10, y = 10;
	lock_window("GWindowGUI::create_objects");

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		toggleinfo *tp = &toggle_order[i];
		int ref = tp->ref;
		if( ref < 0 ) {
			BC_Bar *bar = new BC_Bar(x,y,get_w()-x-10);
			add_tool(bar);
			toggles[i] = 0;
			y += bar->get_h() + 5;
			continue;
		}
		const char *label = toggle_text(tp);
		int color = tp->isauto > 0 ? auto_colors[tp->ref] : WHITE;
		GWindowToggle *toggle = new GWindowToggle(this, x, y, label, color, tp);
		add_tool(toggles[i] = toggle);
		if( tp->isauto > 0 ) {
			VFrame *vframe = 0;
			switch( ref ) {
			case AUTOMATION_MODE: vframe = mwindow->theme->modekeyframe_data;  break;
			case AUTOMATION_PAN:  vframe = mwindow->theme->pankeyframe_data;   break;
			case AUTOMATION_MASK: vframe = mwindow->theme->maskkeyframe_data;  break;
			}
			if( !vframe ) {
				int wh = toggle->get_h() - 4;
				GWindowColorButton *color_button =
					new GWindowColorButton(toggle, get_w()-wh-10, y+2, wh, color);
				add_tool(color_button);
				color_button->create_objects();
			}
			else
				draw_vframe(vframe, get_w()-vframe->get_w()-10, y);
		}
		else if( tp->isauto < 0 ) {
			const char *accel = 0;
			switch( ref ) {
			case NONAUTOTOGGLES_CAMERA_XYZ:
				camera_xyz = toggle;
				accel = _("Shift-F1");
				break;
			case NONAUTOTOGGLES_PROJECTOR_XYZ:
				projector_xyz = toggle;
				accel = _("Shift-F2");
				break;
			}
			 if( accel ) {
				int x1 = get_w() - BC_Title::calculate_w(this, accel) - 10;
				add_subwindow(new BC_Title(x1, y, accel));
			}
		}
		y += toggles[i]->get_h() + 5;
	}
	update_toggles(0);
	unlock_window();
}

void GWindowGUI::update_mwindow(int toggles, int overlays)
{
	unlock_window();
	mwindow->gui->lock_window("GWindowGUI::update_mwindow");
	if( toggles )
		mwindow->gui->mainmenu->update_toggles(0);
	if( overlays )
		mwindow->gui->draw_overlays(1);
	mwindow->gui->unlock_window();
	lock_window("GWindowGUI::update_mwindow");
}

void GWindowGUI::update_toggles(int use_lock)
{
	if(use_lock) {
		lock_window("GWindowGUI::update_toggles");
		set_cool(0);
	}

	for( int i=0; i<(int)(sizeof(toggle_order)/sizeof(toggle_order[0])); ++i ) {
		if( toggles[i] ) toggles[i]->update();
	}

	camera_xyz->set_value(check_xyz(AUTOMATION_CAMERA_X) > 0 ? 1 : 0);
	projector_xyz->set_value(check_xyz(AUTOMATION_PROJECTOR_X) > 0 ? 1 : 0);

	if(use_lock) unlock_window();
}

void GWindowGUI::toggle_camera_xyz()
{
	int v = camera_xyz->get_value() ? 0 : 1;
	camera_xyz->set_value(v);
	xyz_check(AUTOMATION_CAMERA_X, v);
	update_toggles(0);
	update_mwindow(1, 1);
}

void GWindowGUI::toggle_projector_xyz()
{
	int v = projector_xyz->get_value() ? 0 : 1;
	projector_xyz->set_value(v);
	xyz_check(AUTOMATION_PROJECTOR_X, v);
	update_toggles(0);
	update_mwindow(1, 1);
}

int GWindowGUI::translation_event()
{
	mwindow->session->gwindow_x = get_x();
	mwindow->session->gwindow_y = get_y();
	return 0;
}

int GWindowGUI::close_event()
{
	hide_window();
	mwindow->session->show_gwindow = 0;
	unlock_window();

	mwindow->gui->lock_window("GWindowGUI::close_event");
	mwindow->gui->mainmenu->show_gwindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("GWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}

int GWindowGUI::keypress_event()
{
	if( ctrl_down() && shift_down() ) {
		switch(get_keypress()) {
		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
			if( ctrl_down() && shift_down() ) {
				resend_event(mwindow->gui);
				return 1;
			}
		}
	}
	else if( !ctrl_down() && shift_down() ) {
		switch(get_keypress()) {
		case KEY_F1:
			toggle_camera_xyz();
			return 1;
		case KEY_F2:
			toggle_projector_xyz();
			return 1;
		}
	}
	switch(get_keypress()) {
	case 'w':
	case 'W':
	case '0':
		if( ctrl_down() ) {
			close_event();
			return 1;
		}
		break;
	}
	return 0;
}

int GWindowGUI::check_xyz(int group)
{
// returns 1=all set, -1=all clear, 0=mixed
	int *autos = mwindow->edl->session->auto_conf->autos;
	int v = autos[group], ret = v ? 1 : -1;
	if( autos[group+1] != v || autos[group+2] != v ) ret = 0;
	return ret;
}
void GWindowGUI::xyz_check(int group, int v)
{
	int *autos = mwindow->edl->session->auto_conf->autos;
	autos[group+0] = v;
	autos[group+1] = v;
	autos[group+2] = v;
}

int* GWindowGUI::get_main_value(toggleinfo *info)
{
	if( info->isauto > 0 )
		return &mwindow->edl->session->auto_conf->autos[info->ref];
	if( !info->isauto ) {
		switch( info->ref ) {
		case NON_AUTOMATION_ASSETS: return &mwindow->edl->session->show_assets;
		case NON_AUTOMATION_TITLES: return &mwindow->edl->session->show_titles;
		case NON_AUTOMATION_TRANSITIONS: return &mwindow->edl->session->auto_conf->transitions;
		case NON_AUTOMATION_PLUGIN_AUTOS: return &mwindow->edl->session->auto_conf->plugins;
		}
	}
	return 0;
}


GWindowToggle::GWindowToggle(GWindowGUI *gui, int x, int y,
	const char *text, int color, toggleinfo *info)
 : BC_CheckBox(x, y, 0, text, MEDIUMFONT, color)
{
	this->gui = gui;
	this->info = info;
	this->color = color;
	this->color_button = 0;
	hot = hot_value = 0;
}

GWindowToggle::~GWindowToggle()
{
	delete color_button;
}

int GWindowToggle::handle_event()
{
	int value = get_value();
	if( shift_down() ) {
		if( !hot ) {
			gui->set_hot(this);
			value = 1;
		}
		else {
			gui->set_cool(1);
			value = hot_value;
		}
	}
	else
		gui->set_cool(0);
	if( info->isauto >= 0 ) {
		*gui->get_main_value(info) = value;
		switch( info->ref ) {
		case AUTOMATION_CAMERA_X:
		case AUTOMATION_CAMERA_Y:
		case AUTOMATION_CAMERA_Z: {
			int v = gui->check_xyz(AUTOMATION_CAMERA_X);
			gui->camera_xyz->set_value(v > 0 ? 1 : 0);
			break; }
		case AUTOMATION_PROJECTOR_X:
		case AUTOMATION_PROJECTOR_Y:
		case AUTOMATION_PROJECTOR_Z: {
			int v = gui->check_xyz(AUTOMATION_PROJECTOR_X);
			gui->projector_xyz->set_value(v > 0 ? 1 : 0);
			break; }
		}
	}
	else {
		int group = -1;
		switch( info->ref ) {
		case NONAUTOTOGGLES_CAMERA_XYZ:     group = AUTOMATION_CAMERA_X;     break;
		case NONAUTOTOGGLES_PROJECTOR_XYZ:  group = AUTOMATION_PROJECTOR_X;  break;
		}
		if( group >= 0 ) {
			gui->xyz_check(group, value);
			gui->update_toggles(0);
		}
	}
	gui->update_mwindow(1, 0);

// Update stuff in MWindow
	unlock_window();
	MWindow *mwindow = gui->mwindow;
	mwindow->gui->lock_window("GWindowToggle::handle_event");

	mwindow->gui->update(1, NORMAL_DRAW, 0, 0, 1, 0, 0);
	mwindow->gui->draw_overlays(1);

	if( value && info->isauto > 0 ) {
		int autogroup_type = -1;
		switch( info->ref ) {
		case AUTOMATION_FADE:
			autogroup_type = mwindow->edl->tracks->recordable_video_tracks() ?
				AUTOGROUPTYPE_VIDEO_FADE : AUTOGROUPTYPE_AUDIO_FADE ;
			break;
		case AUTOMATION_SPEED:
			autogroup_type = AUTOGROUPTYPE_SPEED;
			break;
		case AUTOMATION_CAMERA_X:
		case AUTOMATION_PROJECTOR_X:
			autogroup_type = AUTOGROUPTYPE_X;
			break;
		case AUTOMATION_CAMERA_Y:
		case AUTOMATION_PROJECTOR_Y:
			autogroup_type = AUTOGROUPTYPE_Y;
			break;
		case AUTOMATION_CAMERA_Z:
		case AUTOMATION_PROJECTOR_Z:
			autogroup_type = AUTOGROUPTYPE_ZOOM;
			break;
		}
		if( autogroup_type >= 0 ) {
			mwindow->edl->local_session->zoombar_showautotype = autogroup_type;
			mwindow->gui->zoombar->update_autozoom();
		}
	}

	mwindow->gui->unlock_window();
	lock_window("GWindowToggle::handle_event");

	return 1;
}

void GWindowToggle::update()
{
	int *vp = gui->get_main_value(info);
	if( vp ) set_value(*vp);
}

void GWindowToggle::update_gui(int color)
{
	BC_Toggle::color = color;
	draw_face(1,0);
}

int GWindowToggle::draw_face(int flash, int flush)
{
	int ret = BC_Toggle::draw_face(flash, flush);
	if( hot ) {
		set_color(color);
		set_opaque();
		draw_rectangle(text_x-1, text_y-1, text_w+1, text_h+1);
		if( flash ) this->flash(0);
		if( flush ) this->flush();
	}
	return ret;
}

void GWindowGUI::set_cool(int reset, int all)
{
	for( int i=0; i<(int)(sizeof(toggles)/sizeof(toggles[0])); ++i ) {
		GWindowToggle* toggle = toggles[i];
		if( !toggle ) continue;
		int *vp = get_main_value(toggle->info);
		if( !vp ) continue;
		if( toggle->hot ) {
			toggle->hot = 0;
			toggle->draw_face(1, 0);
		}
		if( reset > 0 )
			*vp = toggle->hot_value;
		else {
			toggle->hot_value = *vp;
			if( reset < 0 ) {
				if ( all || toggle->info->isauto > 0 )
					*vp = 0;
			}
		}
	}
	if( reset )
		update_toggles(0);
}

void GWindowGUI::set_hot(GWindowToggle *toggle)
{
	int *vp = get_main_value(toggle->info);
	if( !vp ) return;
	set_cool(-1, !toggle->info->isauto ? 1 : 0);
	toggle->hot = 1;
	toggle->set_value(*vp = 1);
}

