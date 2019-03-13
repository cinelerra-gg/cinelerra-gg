
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "apatchgui.inc"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mainsession.h"
#include "mtimebar.h"
#include "preferences.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "units.h"
#include "vpatchgui.inc"
#include "zoombar.h"


ZoomBar::ZoomBar(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mzoom_x,
 	mwindow->theme->mzoom_y,
	mwindow->theme->mzoom_w,
	mwindow->theme->mzoom_h)
{
	this->gui = gui;
	this->mwindow = mwindow;
}

ZoomBar::~ZoomBar()
{
	delete sample_zoom;
	delete amp_zoom;
	delete track_zoom;
}

void ZoomBar::create_objects()
{
	int x = 3;
	int y = get_h() / 2 -
		mwindow->theme->get_image_set("zoombar_menu", 0)[0]->get_h() / 2;

	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	sample_zoom = new SampleZoomPanel(mwindow, this, x, y);
	sample_zoom->create_objects();
	sample_zoom->set_tooltip(_("Duration visible in the timeline"));
	x += sample_zoom->get_w();
	amp_zoom = new AmpZoomPanel(mwindow, this, x, y);
	amp_zoom->create_objects();
	amp_zoom->set_tooltip(_("Audio waveform scale"));
	x += amp_zoom->get_w();
	track_zoom = new TrackZoomPanel(mwindow, this, x, y);
	track_zoom->create_objects();
	track_zoom->set_tooltip(_("Height of tracks in the timeline"));
	x += track_zoom->get_w() + 10;

	int wid = 120;
	for( int i=AUTOGROUPTYPE_AUDIO_FADE; i<=AUTOGROUPTYPE_Y; ++i ) {
		int ww = BC_GenericButton::calculate_w(this, AutoTypeMenu::to_text(i));
		if( ww > wid ) wid = ww;
	}
	add_subwindow(auto_type = new AutoTypeMenu(mwindow, this, x, y, wid));
	auto_type->create_objects();
	x += auto_type->get_w() + 10;
#define DEFAULT_TEXT "000.00 to 000.00"
	add_subwindow(auto_zoom = new AutoZoom(mwindow, this, x, y, 0));
	x += auto_zoom->get_w();
	add_subwindow(auto_zoom_text = new ZoomTextBox(
		mwindow,
		this,
		x,
		y,
		DEFAULT_TEXT));
	x += auto_zoom_text->get_w() + 5;
	add_subwindow(auto_zoom = new AutoZoom(mwindow, this, x, y, 1));
	update_autozoom();
	x += auto_zoom->get_w() + 5;

	add_subwindow(from_value = new FromTextBox(mwindow, this, x, y));
	x += from_value->get_w() + 5;
	add_subwindow(length_value = new LengthTextBox(mwindow, this, x, y));
	x += length_value->get_w() + 5;
	add_subwindow(to_value = new ToTextBox(mwindow, this, x, y));
	x += to_value->get_w() + 5;
	add_subwindow(title_alpha_bar = new TitleAlphaBar(mwindow, this, x, y));
	x += title_alpha_bar->get_w() + 5;
	add_subwindow(title_alpha_text = new TitleAlphaText(mwindow, this, x, y));

	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);

	update();
}


void ZoomBar::update_formatting(BC_TextBox *dst)
{
	dst->set_separators(
		Units::format_to_separators(mwindow->edl->session->time_format));
}

void ZoomBar::resize_event()
{
	reposition_window(mwindow->theme->mzoom_x, mwindow->theme->mzoom_y,
		mwindow->theme->mzoom_w, mwindow->theme->mzoom_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
}

void ZoomBar::redraw_time_dependancies()
{
// Recalculate sample zoom menu
	sample_zoom->update_menu();
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	update_formatting(from_value);
	update_formatting(length_value);
	update_formatting(to_value);
	update_autozoom();
	update_clocks();
}

int ZoomBar::draw()
{
	update();
	return 0;
}

void ZoomBar::update_autozoom()
{
	update_autozoom(mwindow->edl->local_session->zoombar_showautocolor);
}

void ZoomBar::update_autozoom(int grouptype, int color)
{
	mwindow->edl->local_session->zoombar_showautotype = grouptype;
	update_autozoom(color);
}

void ZoomBar::update_autozoom(int color)
{
	char string[BCTEXTLEN];
	int autogroup_type = mwindow->edl->local_session->zoombar_showautotype;
	mwindow->edl->local_session->zoombar_showautocolor = color;
	if( color < 0 ) color = get_resources()->default_text_color;
	switch( autogroup_type ) {
	case AUTOGROUPTYPE_AUDIO_FADE:
	case AUTOGROUPTYPE_VIDEO_FADE:
		sprintf(string, "%0.01f to %0.01f\n",
			mwindow->edl->local_session->automation_mins[autogroup_type],
			mwindow->edl->local_session->automation_maxs[autogroup_type]);
		break;
	case AUTOGROUPTYPE_ZOOM:
	case AUTOGROUPTYPE_SPEED:
		sprintf(string, "%0.03f to %0.03f\n",
			mwindow->edl->local_session->automation_mins[autogroup_type],
			mwindow->edl->local_session->automation_maxs[autogroup_type]);
		break;
	case AUTOGROUPTYPE_X:
	case AUTOGROUPTYPE_Y:
		sprintf(string, "%0.0f to %.0f\n",
			mwindow->edl->local_session->automation_mins[autogroup_type],
			mwindow->edl->local_session->automation_maxs[autogroup_type]);
		break;
	}
	auto_zoom_text->update(string);
	const char *group_name = AutoTypeMenu::to_text(autogroup_type);
	auto_type->set_text(group_name);
}


int ZoomBar::update()
{
	sample_zoom->update(mwindow->edl->local_session->zoom_sample);
	amp_zoom->update(mwindow->edl->local_session->zoom_y);
	track_zoom->update(mwindow->edl->local_session->zoom_track);
	update_autozoom();
	update_clocks();
	return 0;
}

int ZoomBar::update_clocks()
{
	from_value->update_position(mwindow->edl->local_session->get_selectionstart(1));
	length_value->update_position(mwindow->edl->local_session->get_selectionend(1) -
		mwindow->edl->local_session->get_selectionstart(1));
	to_value->update_position(mwindow->edl->local_session->get_selectionend(1));
	return 0;
}

TitleAlphaBar::TitleAlphaBar(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_FSlider(x, y, 0, 150, 200, 0, 1.0, mwindow->session->title_bar_alpha, 0)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_precision(0.01);
	enable_show_value(0);
}

int TitleAlphaBar::handle_event()
{
	float v = get_value();
	mwindow->session->title_bar_alpha = v;
	zoombar->title_alpha_text->update(v);
	mwindow->gui->draw_trackmovement();
	mwindow->gui->flush();
	return 1;
}

TitleAlphaText::TitleAlphaText(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 48, 1, mwindow->session->title_bar_alpha, 0, MEDIUMFONT, 2)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Title Alpha"));
}

int TitleAlphaText::handle_event()
{
	float v = atof(get_text());
	mwindow->session->title_bar_alpha = v;
	zoombar->title_alpha_bar->update(v);
	mwindow->gui->draw_trackmovement();
	mwindow->gui->flush();
	return 1;
}

int ZoomBar::resize_event(int w, int h)
{
// don't change anything but y and width
	reposition_window(0, h - this->get_h(), w, this->get_h());
	return 0;
}


// Values for which_one
#define SET_FROM 1
#define SET_LENGTH 2
#define SET_TO 3


int ZoomBar::set_selection(int which_one)
{
	double start_position = mwindow->edl->local_session->get_selectionstart(1);
	double end_position = mwindow->edl->local_session->get_selectionend(1);
	double length = end_position - start_position;

// Fix bogus results

	switch(which_one)
	{
		case SET_LENGTH:
			start_position = Units::text_to_seconds(from_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			length = Units::text_to_seconds(length_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = start_position + length;

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;

		case SET_FROM:
			start_position = Units::text_to_seconds(from_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				end_position = start_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;

		case SET_TO:
			start_position = Units::text_to_seconds(from_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);
			end_position = Units::text_to_seconds(to_value->get_text(),
				mwindow->edl->session->sample_rate,
				mwindow->edl->session->time_format,
				mwindow->edl->session->frame_rate,
				mwindow->edl->session->frames_per_foot);

			if(end_position < start_position)
			{
				start_position = end_position;
				mwindow->edl->local_session->set_selectionend(
					mwindow->edl->local_session->get_selectionstart(1));
			}
			break;
	}

	mwindow->edl->local_session->set_selectionstart(
		mwindow->edl->align_to_frame(start_position, 1));
	mwindow->edl->local_session->set_selectionend(
		mwindow->edl->align_to_frame(end_position, 1));


	mwindow->gui->update_timebar_highlights();
	mwindow->gui->hide_cursor(1);
	mwindow->gui->show_cursor(1);
	update();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->flash_canvas(1);

	return 0;
}












SampleZoomPanel::SampleZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, zoombar, mwindow->edl->local_session->zoom_sample,
		x, y, 130, MIN_ZOOM_TIME, MAX_ZOOM_TIME, ZOOM_TIME)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int SampleZoomPanel::handle_event()
{
	mwindow->zoom_sample((int64_t)get_value());
	return 1;
}


AmpZoomPanel::AmpZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, zoombar, mwindow->edl->local_session->zoom_y,
		x, y, 100, MIN_AMP_ZOOM, MAX_AMP_ZOOM, ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int AmpZoomPanel::handle_event()
{
	mwindow->zoom_amp((int64_t)get_value());
	return 1;
}

TrackZoomPanel::TrackZoomPanel(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : ZoomPanel(mwindow, zoombar, mwindow->edl->local_session->zoom_track,
		x, y, 90, MIN_TRACK_ZOOM, MAX_TRACK_ZOOM, ZOOM_LONG)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
}
int TrackZoomPanel::handle_event()
{
	mwindow->zoom_track((int64_t)get_value());
	zoombar->amp_zoom->update(mwindow->edl->local_session->zoom_y);
	return 1;
}




AutoZoom::AutoZoom(MWindow *mwindow, ZoomBar *zoombar, int x, int y, int changemax)
 : BC_Tumbler(x, y, mwindow->theme->get_image_set("zoombar_tumbler"))
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	this->changemax = changemax;
	if (changemax)
		set_tooltip(_("Automation range maximum"));
	else
		set_tooltip(_("Automation range minimum"));
}

int AutoZoom::handle_up_event()
{
	mwindow->change_currentautorange(mwindow->edl->local_session->zoombar_showautotype,1,changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->gui->draw_overlays(0);
	mwindow->gui->update_patchbay();
	mwindow->gui->flash_canvas(1);
	return 1;
}

int AutoZoom::handle_down_event()
{
	mwindow->change_currentautorange(mwindow->edl->local_session->zoombar_showautotype,0,changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->gui->draw_overlays(0);
	mwindow->gui->update_patchbay();
	mwindow->gui->flash_canvas(1);
	return 1;
}



AutoTypeMenu::AutoTypeMenu(MWindow *mwindow, ZoomBar *zoombar, int x, int y, int wid)
 : BC_PopupMenu(x, y, wid + 24,
	to_text(mwindow->edl->local_session->zoombar_showautotype), 1, 0, 12)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Automation Type"));
}

void AutoTypeMenu::create_objects()
{
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_AUDIO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_VIDEO_FADE)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_ZOOM)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_SPEED)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_X)));
	add_item(new BC_MenuItem(to_text(AUTOGROUPTYPE_Y)));
}

const char* AutoTypeMenu::to_text(int mode)
{
	switch(mode) {
	case AUTOGROUPTYPE_AUDIO_FADE: return _("Audio Fade:");
	case AUTOGROUPTYPE_VIDEO_FADE: return _("Video Fade:");
	case AUTOGROUPTYPE_ZOOM: return _("Zoom:");
	case AUTOGROUPTYPE_SPEED: return _("Speed:");
	case AUTOGROUPTYPE_X: return "X:";
	case AUTOGROUPTYPE_Y: return "Y:";
	}
	return "??";
}

int AutoTypeMenu::from_text(char *text)
{
	if(!strcmp(text, to_text(AUTOGROUPTYPE_AUDIO_FADE))) return AUTOGROUPTYPE_AUDIO_FADE;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_VIDEO_FADE))) return AUTOGROUPTYPE_VIDEO_FADE;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_ZOOM))) return AUTOGROUPTYPE_ZOOM;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_SPEED))) return AUTOGROUPTYPE_SPEED;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_X))) return AUTOGROUPTYPE_X;
	if(!strcmp(text, to_text(AUTOGROUPTYPE_Y))) return AUTOGROUPTYPE_Y;
	return AUTOGROUPTYPE_INT255;
}

int AutoTypeMenu::draw_face(int dx, int color)
{
	BC_PopupMenu::draw_face(dx+8, color);
	color = mwindow->edl->local_session->zoombar_showautocolor;
	if( color >= 0 ) {
		set_color(color);
		int margin = get_margin();
		int mx = margin+4, my = 3*margin/8;
		int bh = get_h() - 2*my;
		draw_box(mx,my, bh,bh);
	}
	return 1;
}

int AutoTypeMenu::handle_event()
{
	mwindow->edl->local_session->zoombar_showautotype = from_text(this->get_text());
	this->zoombar->update_autozoom();
	return 1;
}


ZoomTextBox::ZoomTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y, const char *text)
 : BC_TextBox(x, y, 130, 1, text)
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Automation range"));
}

int ZoomTextBox::button_press_event()
{
	if (!(get_buttonpress() == 4 || get_buttonpress() == 5)) {
		BC_TextBox::button_press_event();
		return 0;
	}
	if (!is_event_win()) return 0;

	int changemax = 1;
	if (get_relative_cursor_x() < get_w()/2)
		changemax = 0;

	// increment
	if (get_buttonpress() == 4)
		mwindow->change_currentautorange(mwindow->edl->local_session->zoombar_showautotype, 1, changemax);

	// decrement
	if (get_buttonpress() == 5)
		mwindow->change_currentautorange(mwindow->edl->local_session->zoombar_showautotype, 0, changemax);

	mwindow->gui->zoombar->update_autozoom();
	mwindow->gui->draw_overlays(0);
	mwindow->gui->update_patchbay();
	mwindow->gui->flash_canvas(1);
	return 1;
}

int ZoomTextBox::handle_event()
{
	float min, max;
	if (sscanf(this->get_text(),"%f to%f",&min, &max) == 2)
	{
		AUTOMATIONVIEWCLAMPS(min, mwindow->edl->local_session->zoombar_showautotype);
		AUTOMATIONVIEWCLAMPS(max, mwindow->edl->local_session->zoombar_showautotype);
		if (max > min)
		{
			mwindow->edl->local_session->automation_mins[mwindow->edl->local_session->zoombar_showautotype] = min;
			mwindow->edl->local_session->automation_maxs[mwindow->edl->local_session->zoombar_showautotype] = max;
			mwindow->gui->zoombar->update_autozoom();
			mwindow->gui->draw_overlays(0);
			mwindow->gui->update_patchbay();
			mwindow->gui->flash_canvas(1);
		}
	}
	// TODO: Make the text turn red when it's a bad range..
	return 0;
}





FromTextBox::FromTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection start time"));
}

int FromTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_FROM);
		return 1;
	}
	return 0;
}

int FromTextBox::update_position(double new_position)
{
	Units::totext(string,
		new_position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
//printf("FromTextBox::update_position %f %s\n", new_position, string);
	update(string);
	return 0;
}






LengthTextBox::LengthTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection length"));
}

int LengthTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_LENGTH);
		return 1;
	}
	return 0;
}

int LengthTextBox::update_position(double new_position)
{
	Units::totext(string,
		new_position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}





ToTextBox::ToTextBox(MWindow *mwindow, ZoomBar *zoombar, int x, int y)
 : BC_TextBox(x, y, 90, 1, "")
{
	this->mwindow = mwindow;
	this->zoombar = zoombar;
	set_tooltip(_("Selection end time"));
}

int ToTextBox::handle_event()
{
	if(get_keypress() == 13)
	{
		zoombar->set_selection(SET_TO);
		return 1;
	}
	return 0;
}

int ToTextBox::update_position(double new_position)
{
	Units::totext(string, new_position,
		mwindow->edl->session->time_format,
		mwindow->edl->session->sample_rate,
		mwindow->edl->session->frame_rate,
		mwindow->edl->session->frames_per_foot);
	update(string);
	return 0;
}
