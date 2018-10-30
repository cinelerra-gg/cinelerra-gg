
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

#include "automation.h"
#include "bcsignals.h"
#include "cplayback.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "intauto.h"
#include "intautos.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "patchgui.h"
#include "playbackengine.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"
#include "zwindow.h"


PatchGUI::PatchGUI(MWindow *mwindow,
		PatchBay *patchbay,
		Track *track,
		int x,
		int y)
{
	this->mwindow = mwindow;
	this->patchbay = patchbay;
	this->track = track;
	this->x = x;
	this->y = y;
	title = 0;
	record = 0;
	play = 0;
//	automate = 0;
	gang = 0;
	draw = 0;
	mute = 0;
	expand = 0;
	nudge = 0;
	mix = 0;
	change_source = 0;
	track_id = track ? track->get_id() : -1;
	mixer = 0;
}

PatchGUI::~PatchGUI()
{
	delete title;
	delete record;
	delete play;
//	delete automate;
	delete gang;
	delete draw;
	delete mute;
	delete expand;
	delete nudge;
	delete mix;
}

void PatchGUI::create_objects()
{
	update(x, y);
}

int PatchGUI::reposition(int x, int y)
{
	int x1 = 0;
	int y1 = 0;


	if( x != this->x || y != this->y ) {
		this->x = x;  this->y = y;

		if( title )
			title->reposition_window(title->get_x(), y1 + y, 0);
		if( expand )
			expand->reposition_window(expand->get_x(), y1 + y);
		y1 += mwindow->theme->title_h;

		if( play ) {
			play->reposition_window(play->get_x(), y1 + y);
			x1 += play->get_w();
			record->reposition_window(record->get_x(), y1 + y);
			x1 += record->get_w();
//			automate->reposition_window(x1, y1 + y);
//			x1 += automate->get_w();
			gang->reposition_window(gang->get_x(), y1 + y);
			x1 += gang->get_w();
			draw->reposition_window(draw->get_x(), y1 + y);
			x1 += draw->get_w();
			mute->reposition_window(mute->get_x(), y1 + y);
			x1 += mute->get_w();
		}
		y1 += mwindow->theme->play_h;
	}
	else {
		if( title )
			y1 += mwindow->theme->title_h;
		if( play )
			y1 += mwindow->theme->play_h;
	}

	return y1;
}

int PatchGUI::update(int x, int y)
{
//TRACE("PatchGUI::update 1");
	reposition(x, y);
//TRACE("PatchGUI::update 10");

	int h = track->vertical_span(mwindow->theme);
	int y1 = 0;
	int x1 = 0;
//printf("PatchGUI::update 10\n");

	int y2 = y1 + mwindow->theme->title_h;
	if( title ) {
		if( h < y2 ) {
			delete title;   title = 0;
			delete expand;  expand = 0;
		}
		else {
			title->update(track->title);
			expand->update(track->expand_view);
		}
	}
	else if( h >= y2 ) {
		VFrame **expandpatch_data = mwindow->theme->get_image_set("expandpatch_data");
		int x2 = patchbay->get_w() - expandpatch_data[0]->get_w() - 5;
		patchbay->add_subwindow(title = new TitlePatch(mwindow, this, x1 + x, y1 + y, x2-x1-5));
		patchbay->add_subwindow(expand = new ExpandPatch(mwindow, this, x2, y1 + y));
	}

	if( title )
		y1 = y2;

	y2 = y1 + mwindow->theme->play_h;
	if( play ) {
		if( h < y2 ) {
			delete play;    play = 0;
			delete record;  record = 0;
			delete gang;    gang = 0;
			delete draw;    draw = 0;
			delete mute;    mute = 0;
		}
		else {
			play->update(track->play);
			record->update(track->record);
			gang->update(track->gang);
			draw->update(track->draw);
			mute->update(mwindow->get_int_auto(this, AUTOMATION_MUTE)->value);
		}
	}
	else if( h >= y2 ) {
		patchbay->add_subwindow(play = new PlayPatch(mwindow, this, x1 + x, y1 + y));
//printf("PatchGUI::update %d %d\n", __LINE__, play->get_h());
		x1 += play->get_w();
		patchbay->add_subwindow(record = new RecordPatch(mwindow, this, x1 + x, y1 + y));
		x1 += record->get_w();
		patchbay->add_subwindow(gang = new GangPatch(mwindow, this, x1 + x, y1 + y));
		x1 += gang->get_w();
		patchbay->add_subwindow(draw = new DrawPatch(mwindow, this, x1 + x, y1 + y));
		x1 += draw->get_w();
		patchbay->add_subwindow(mute = new MutePatch(mwindow, this, x1 + x, y1 + y));
		x1 += mute->get_w();
	}
	if( play )
		y1 = y2;

//UNTRACE
	return y1;
}


void PatchGUI::toggle_behavior(int type,
		int value,
		BC_Toggle *toggle,
		int *output)
{
	if(toggle->shift_down()) {
		int sense = type != Tracks::MUTE ? 1 : 0;
		// all selected if nothing previously selected or
		// if this patch was previously the only one selected
		int total_type = mwindow->edl->tracks->total_of(type);
		int total_selected = sense ? total_type :
			mwindow->edl->tracks->total() - total_type;
		int selected = !total_selected || (total_selected == 1 &&
			 *output == sense ) ? sense : 1-sense;
		mwindow->edl->tracks->select_all(type, selected);
		if( selected != sense ) *output = sense;

		patchbay->drag_operation = type;
		patchbay->new_status = 1;
	}
	else
	{
		*output = value;
// Select + drag behavior
		patchbay->drag_operation = type;
		patchbay->new_status = value;
	}

	switch(type)
	{
		case Tracks::PLAY:
			mwindow->gui->unlock_window();
			mwindow->restart_brender();
			mwindow->sync_parameters(CHANGE_EDL);
			mwindow->gui->lock_window("PatchGUI::toggle_behavior 1");
			break;

		case Tracks::MUTE:
			mwindow->gui->unlock_window();
			mwindow->restart_brender();
			mwindow->sync_parameters(CHANGE_PARAMS);
			mwindow->gui->lock_window("PatchGUI::toggle_behavior 2");
			break;

// Update affected tracks in cwindow
		case Tracks::RECORD:
			mwindow->cwindow->update(0, 1, 1);
			break;

		case Tracks::GANG:
			break;

		case Tracks::DRAW:
			mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
			break;

		case Tracks::EXPAND:
			break;
	}

// update all panes
	mwindow->gui->update_patchbay();
}


char* PatchGUI::calculate_nudge_text(int *changed)
{
	if(changed) *changed = 0;
	if(track->edl->session->nudge_format)
	{
		sprintf(string_return, "%.4f", track->from_units(track->nudge));
		if(changed && nudge && atof(nudge->get_text()) - atof(string_return) != 0)
			*changed = 1;
	}
	else
	{
		sprintf(string_return, "%jd", track->nudge);
		if(changed && nudge && atoi(nudge->get_text()) - atoi(string_return) != 0)
			*changed = 1;
	}
	return string_return;
}


int64_t PatchGUI::calculate_nudge(const char *string)
{
	if(mwindow->edl->session->nudge_format)
	{
		float result;
		sscanf(string, "%f", &result);
		return track->to_units(result, 0);
	}
	else
	{
		int64_t temp;
		sscanf(string, "%jd", &temp);
		return temp;
	}
}


PlayPatch::PlayPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x,
 		y,
		mwindow->theme->get_image_set("playpatch_data"),
		patch->track->play,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Play track"));
	set_select_drag(1);
}

int PlayPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::PLAY,
			get_value(),
			this,
			&patch->track->play);
		return 1;
	}
	return 0;
}

int PlayPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::PLAY)
	{
		mwindow->undo->update_undo_after(_("play patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











RecordPatch::RecordPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x,
 		y,
		mwindow->theme->get_image_set("recordpatch_data"),
		patch->track->record,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Arm track"));
	set_select_drag(1);
}

int RecordPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::RECORD,
			get_value(),
			this,
			&patch->track->record);
		patch->title->set_back_color(patch->track->record ?
			get_resources()->text_background :
			get_resources()->text_background_disarmed);
		patch->title->set_text_row(0);
		return 1;
	}
	return 0;
}

int RecordPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::RECORD)
	{
		mwindow->undo->update_undo_after(_("record patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











GangPatch::GangPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y,
		mwindow->theme->get_image_set("gangpatch_data"),
		patch->track->gang,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Gang faders"));
	set_select_drag(1);
}

int GangPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::GANG,
			get_value(),
			this,
			&patch->track->gang);
		return 1;
	}
	return 0;
}

int GangPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::GANG)
	{
		mwindow->undo->update_undo_after(_("gang patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}











DrawPatch::DrawPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y,
		mwindow->theme->get_image_set("drawpatch_data"),
		patch->track->draw,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Draw media"));
	set_select_drag(1);
}

int DrawPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::DRAW,
			get_value(),
			this,
			&patch->track->draw);
		return 1;
	}
	return 0;
}

int DrawPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::DRAW)
	{
		mwindow->undo->update_undo_after(_("draw patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}










MutePatch::MutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y,
		mwindow->theme->get_image_set("mutepatch_data"),
		mwindow->get_int_auto(patch, AUTOMATION_MUTE)->value,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Don't send to output"));
	set_select_drag(1);
}

int MutePatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		IntAuto *current;
		double position = mwindow->edl->local_session->get_selectionstart(1);
		Autos *mute_autos = patch->track->automation->autos[AUTOMATION_MUTE];


		current = (IntAuto*)mute_autos->get_auto_for_editing(position);
		current->value = get_value();

		patch->toggle_behavior(Tracks::MUTE,
			get_value(),
			this,
			&current->value);



		if(mwindow->edl->session->auto_conf->autos[AUTOMATION_MUTE])
		{
			mwindow->gui->draw_overlays(1);
		}
		return 1;
	}
	return 0;
}

int MutePatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::MUTE)
	{
		mwindow->undo->update_undo_after(_("mute patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}



ExpandPatch::ExpandPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x,
 		y,
		mwindow->theme->get_image_set("expandpatch_data"),
		patch->track->expand_view,
		"",
		0,
		0,
		0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_select_drag(1);
}

int ExpandPatch::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		mwindow->undo->update_undo_before();
		set_status(BC_Toggle::TOGGLE_DOWN);
		update(!get_value());
		patch->toggle_behavior(Tracks::EXPAND,
			get_value(),
			this,
			&patch->track->expand_view);
		mwindow->edl->tracks->update_y_pixels(mwindow->theme);
		mwindow->gui->draw_trackmovement();
		return 1;
	}
	return 0;
}

int ExpandPatch::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	if(patch->patchbay->drag_operation == Tracks::EXPAND)
	{
		mwindow->undo->update_undo_after(_("expand patch"), LOAD_PATCHES);
		patch->patchbay->drag_operation = Tracks::NONE;
	}
	return result;
}


TitlePatch::TitlePatch(MWindow *mwindow, PatchGUI *patch, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, patch->track->title, 1, MEDIUMFONT, 1)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_back_color(patch->track->record ?
			get_resources()->text_background :
			get_resources()->text_background_disarmed);
}

void TitlePatch::update(const char *text)
{
	set_back_color(patch->track->record ?
			get_resources()->text_background :
			get_resources()->text_background_disarmed);
	BC_TextBox::update(text);
}

int TitlePatch::handle_event()
{
	mwindow->undo->update_undo_before(_("track title"), this);
	strcpy(patch->track->title, get_text());
	mwindow->update_plugin_titles();
	mwindow->gui->draw_overlays(1);
	mwindow->undo->update_undo_after(_("track title"), LOAD_PATCHES);
	return 1;
}


NudgePatch::NudgePatch(MWindow *mwindow,
	PatchGUI *patch,
	int x,
	int y,
	int w)
 : BC_TextBox(x,
 	y,
	w,
	1,
	patch->calculate_nudge_text(0))
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Nudge"));
}

int NudgePatch::handle_event()
{
	set_value(patch->calculate_nudge(get_text()));
	return 1;
}

void NudgePatch::set_value(int64_t value)
{
	mwindow->undo->update_undo_before(_("nudge."), this);
	patch->track->nudge = value;

	if(patch->track->gang && patch->track->record)
		patch->patchbay->synchronize_nudge(patch->track->nudge, patch->track);

	mwindow->undo->update_undo_after(_("nudge."), LOAD_PATCHES);

	mwindow->gui->unlock_window();
	if(patch->track->data_type == TRACK_VIDEO)
		mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_PARAMS);
	mwindow->gui->lock_window("NudgePatch::handle_event 2");

	mwindow->session->changes_made = 1;
}


int NudgePatch::button_press_event()
{
	int result = 0;

	if(is_event_win() && cursor_inside())
	{
		if(get_buttonpress() == 4)
		{
			int value = patch->calculate_nudge(get_text());
			value += calculate_increment();
			set_value(value);
			update();
			result = 1;
		}
		else
		if(get_buttonpress() == 5)
		{
			int value = patch->calculate_nudge(get_text());
			value -= calculate_increment();
			set_value(value);
			update();
			result = 1;
		}
		else
		if(get_buttonpress() == 3)
		{
			patch->patchbay->nudge_popup->activate_menu(patch);
			result = 1;
		}
	}

	if(!result)
		return BC_TextBox::button_press_event();
	else
		return result;
}

int64_t NudgePatch::calculate_increment()
{
	if(patch->track->data_type == TRACK_AUDIO)
	{
		return (int64_t)ceil(patch->track->edl->session->sample_rate / 10.0);
	}
	else
	{
		return (int64_t)ceil(1.0 / patch->track->edl->session->frame_rate);
	}
}

void NudgePatch::update()
{
	int changed;
	char *string = patch->calculate_nudge_text(&changed);
	if(changed)
		BC_TextBox::update(string);
}


MixPatch::MixPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set("mixpatch_data"),
	patch->mixer, "", 0, 0, 0)
{
	this->mwindow = mwindow;
	this->patch = patch;
}

MixPatch::~MixPatch()
{
}

int MixPatch::handle_event()
{
	int v = patch->track ? get_value() : 0;
	if( patch->mixer != v ) {
		if( patch->track )
			mwindow->gui->update_mixers(patch->track, v);
		else
			update(v);
		mwindow->update_mixer_tracks();
	}
	return 1;
}

void MixPatch::update(int v)
{
	patch->mixer = v;
	BC_Toggle::update(v);
}

