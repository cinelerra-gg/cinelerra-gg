
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

#include "apatchgui.h"
#include "atrack.h"
#include "autoconf.h"
#include "automation.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "gwindowgui.h"
#include "intauto.h"
#include "intautos.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "panauto.h"
#include "panautos.h"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"
#include "zwindow.h"

APatchGUI::APatchGUI(MWindow *mwindow, PatchBay *patchbay,
		ATrack *track, int x, int y)
 : PatchGUI(mwindow, patchbay, track, x, y)
{
	data_type = TRACK_AUDIO;
	this->atrack = track;
	meter = 0;
	pan = 0;
	fade = 0;
}

APatchGUI::~APatchGUI()
{
	if( fade ) delete fade;
	if( meter ) delete meter;
	if( pan ) delete pan;
}

void APatchGUI::create_objects()
{
	update(x, y);
}

int APatchGUI::reposition(int x, int y)
{
	int y1 = PatchGUI::reposition(x, y);

	if( fade )
		fade->reposition_window(fade->get_x(), y1+y);
	y1 += mwindow->theme->fade_h;
	if( meter )
		meter->reposition_window(meter->get_x(), y1+y, -1, meter->get_w());
	y1 += mwindow->theme->meter_h;
	if( mix )
		mix->reposition_window(mix->get_x(), y1+y);
	if( pan )
		pan->reposition_window(pan->get_x(), y1+y);
	if( nudge )
		nudge->reposition_window(nudge->get_x(), y1+y);
	y1 += mwindow->theme->pan_h;

	return y1;
}

int APatchGUI::update(int x, int y)
{
	int h = track->vertical_span(mwindow->theme);
	int x1 = 0;
	int y1 = PatchGUI::update(x, y);

	int y2 = y1 + mwindow->theme->fade_h;
	if( fade ) {
		if( h < y2 ) {
			delete fade;
			fade = 0;
		}
		else {
			FloatAuto *previous = 0, *next = 0;
			double unit_position = mwindow->edl->local_session->get_selectionstart(1);
			unit_position = mwindow->edl->align_to_frame(unit_position, 0);
			unit_position = atrack->to_units(unit_position, 0);
			FloatAutos *ptr = (FloatAutos*)atrack->automation->autos[AUTOMATION_FADE];
			float value = ptr->get_value((long)unit_position, PLAY_FORWARD, previous, next);
			fade->update(fade->get_w(), value,
				     mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
				     mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE]);
		}
	}
	else if( h >= y2 ) {
		float v = mwindow->get_float_auto(this, AUTOMATION_FADE)->get_value();
		patchbay->add_subwindow(fade = new AFadePatch(this, x1+x, y1+y,
			patchbay->get_w() - 10, v));
	}
	y1 = y2;

	y2 = y1 + mwindow->theme->meter_h;
	if( meter ) {
		if( h < y2 ) {
			delete meter;  meter = 0;
		}
	}
	else if( h >= y2 ) {
		patchbay->add_subwindow(meter = new AMeterPatch(mwindow, this, x1+x, y1+y));
	}
	y1 = y2;

	y2 = y1 + mwindow->theme->pan_h;
	if( pan ) {
		if( h < y2 ) {
			delete mix;    mix = 0;
			delete pan;    pan = 0;
			delete nudge;  nudge = 0;
		}
		else {
			if( mwindow->session->selected_zwindow >= 0 ) {
				int v = mwindow->mixer_track_active(track);
				mix->update(v);
			}
			if( pan->get_total_values() != mwindow->edl->session->audio_channels ) {
				pan->change_channels(mwindow->edl->session->audio_channels,
					mwindow->edl->session->achannel_positions);
			}
			else {
				int handle_x, handle_y;
				PanAuto *previous = 0, *next = 0;
				double unit_position = mwindow->edl->local_session->get_selectionstart(1);
				unit_position = mwindow->edl->align_to_frame(unit_position, 0);
				unit_position = atrack->to_units(unit_position, 0);
				PanAutos *ptr = (PanAutos*)atrack->automation->autos[AUTOMATION_PAN];
				ptr->get_handle(handle_x, handle_y, (long)unit_position,
						PLAY_FORWARD, previous, next);
				pan->update(handle_x, handle_y);
			}
			nudge->update();
		}
	}
	else if( h >= y2 ) {
		patchbay->add_subwindow(mix = new AMixPatch(mwindow, this, x1+x, y1+y+5));
		x1 += mix->get_w() + 10;
		patchbay->add_subwindow(pan = new APanPatch(mwindow, this, x1+x, y1+y));
		x1 += pan->get_w() + 20;
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow, this, x1+x, y1+y,
				patchbay->get_w() - x1-x - 10));
	}
	y1 = y2;

	return y1;
}

void APatchGUI::update_faders(float v)
{
	if( fade )
		fade->update(v);

	change_source = 1;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *fade_autos = atrack->automation->autos[AUTOMATION_FADE];
	int need_undo = !fade_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("fade"), need_undo ? 0 : this);
	FloatAuto *current = (FloatAuto*)fade_autos->get_auto_for_editing(position);
	float change = v - current->get_value();
	current->set_value(v);

	if( track->gang && track->record )
		patchbay->synchronize_faders(change, TRACK_AUDIO, track);
	mwindow->undo->update_undo_after(_("fade"), LOAD_AUTOMATION);
	change_source = 0;

	mwindow->sync_parameters(CHANGE_PARAMS);

	if( mwindow->edl->session->auto_conf->autos[AUTOMATION_FADE] ) {
		mwindow->gui->draw_overlays(1);
	}
}

AFadePatch::AFadePatch(APatchGUI *patch, int x, int y, int w, float v)
 : BC_FSlider(x, y, 0, w, w,
	patch->mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_AUDIO_FADE],
	patch->mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_AUDIO_FADE],
	v)
{
	this->patch = patch;
}


int AFadePatch::handle_event()
{
	if( shift_down() ) {
		update(0.0);
		set_tooltip(get_caption());
	}
	patch->update_faders(get_value());
	return 1;
}

AKeyFadePatch::AKeyFadePatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_SubWindow(x,y, 200,20, GWindowGUI::auto_colors[AUTOMATION_FADE])
{
	this->mwindow = mwindow;
	this->patch = patch;
}

void AKeyFadePatch::create_objects()
{
	int x = 0, y = 0;
	float v = mwindow->get_float_auto(patch, AUTOMATION_FADE)->get_value();
	add_subwindow(akey_fade_text = new AKeyFadeText(this, x, y, 64, v));
	x += akey_fade_text->get_w();
	VFrame **lok_images = mwindow->theme->get_image_set("lok");
	int w1 = get_w() - x - lok_images[0]->get_w();
	add_subwindow(akey_fade_slider = new AKeyFadeSlider(this, x, y, w1, v));
	x += akey_fade_slider->get_w();
	add_subwindow(akey_fade_ok = new AKeyFadeOK(this, x, y, lok_images));
	activate();
	show_window();
}

void AKeyFadePatch::update(float v)
{
	akey_fade_text->update(v);
	akey_fade_slider->update(v);
	patch->update_faders(v);
}

AKeyFadeOK::AKeyFadeOK(AKeyFadePatch *akey_fade_patch, int x, int y, VFrame **images)
 : BC_Button(x, y, images)
{
	this->akey_fade_patch = akey_fade_patch;
}

int AKeyFadeOK::handle_event()
{
	MWindowGUI *mgui = akey_fade_patch->mwindow->gui;
	delete mgui->keyvalue_popup;
	mgui->keyvalue_popup = 0;
	return 1;
}

AKeyFadeText::AKeyFadeText(AKeyFadePatch *akey_fade_patch, int x, int y, int w, float v)
 : BC_TextBox(x, y, w, 1, v, 1, MEDIUMFONT, 2)
{
	this->akey_fade_patch = akey_fade_patch;
}

int AKeyFadeText::handle_event()
{
	float v = atof(get_text());
	akey_fade_patch->update(v);
	return get_keypress() != RETURN ? 1 :
		akey_fade_patch->akey_fade_ok->handle_event();
}

AKeyFadeSlider::AKeyFadeSlider(AKeyFadePatch *akey_fade_patch, int x, int y, int w, float v)
 : AFadePatch(akey_fade_patch->patch, x, y, w, v)
{
	this->akey_fade_patch = akey_fade_patch;
}

int AKeyFadeSlider::handle_event()
{
	float v = get_value();
	akey_fade_patch->update(v);
	return 1;
}


APanPatch::APanPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Pan(x, y, PAN_RADIUS, MAX_PAN,
	mwindow->edl->session->audio_channels,
	mwindow->edl->session->achannel_positions,
	mwindow->get_pan_auto(patch)->handle_x,
	mwindow->get_pan_auto(patch)->handle_y,
	mwindow->get_pan_auto(patch)->values)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_tooltip(_("Pan"));
}

int APanPatch::handle_event()
{
	PanAuto *current;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *pan_autos = patch->atrack->automation->autos[AUTOMATION_PAN];
	int need_undo = !pan_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("pan"), need_undo ? 0 : this);

	current = (PanAuto*)pan_autos->get_auto_for_editing(position);

	current->handle_x = get_stick_x();
	current->handle_y = get_stick_y();
	memcpy(current->values, get_values(), sizeof(float) * mwindow->edl->session->audio_channels);

	mwindow->undo->update_undo_after(_("pan"), LOAD_AUTOMATION);

	mwindow->sync_parameters(CHANGE_PARAMS);

	if( need_undo && mwindow->edl->session->auto_conf->autos[AUTOMATION_PAN] ) {
		mwindow->gui->draw_overlays(1);
	}
	return 1;
}

AKeyPanPatch::AKeyPanPatch(MWindow *mwindow, APatchGUI *patch)
 : APanPatch(mwindow, patch, -1,-1)
{
}

int AKeyPanPatch::handle_event()
{
	int ret = APanPatch::handle_event();
	APanPatch *pan = patch->pan;
	if( pan )
		pan->update(get_stick_x(), get_stick_y());
	return ret;
}


AMeterPatch::AMeterPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : BC_Meter(x, y, METER_HORIZ, patch->patchbay->get_w() - 10,
	mwindow->edl->session->min_meter_db, mwindow->edl->session->max_meter_db,
	mwindow->edl->session->meter_format, 0, -1)
{
	this->mwindow = mwindow;
	this->patch = patch;
	set_delays(TRACKING_RATE * 10,
			TRACKING_RATE);
}

int AMeterPatch::button_press_event()
{
	if( cursor_inside() && is_event_win() && get_buttonpress() == 1 ) {
		mwindow->reset_meters();
		return 1;
	}

	return 0;
}

AMixPatch::AMixPatch(MWindow *mwindow, APatchGUI *patch, int x, int y)
 : MixPatch(mwindow, patch, x, y)
{
	set_tooltip(_("Mixer"));
}

AMixPatch::~AMixPatch()
{
}

