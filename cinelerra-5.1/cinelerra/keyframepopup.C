
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
#include "autos.h"
#include "bcwindowbase.h"
#include "cpanel.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "floatauto.h"
#include "gwindow.h"
#include "gwindowgui.h"
#include "intauto.h"
#include "keys.h"
#include "keyframe.h"
#include "keyframepopup.h"
#include "language.h"
#include "localsession.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainundo.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "patchbay.h"
#include "patchgui.h"
#include "theme.h"
#include "timelinepane.h"
#include "track.h"
#include "trackcanvas.h"
#include "vtrack.h"

KeyframePopup::KeyframePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	key_hide = 0;
	key_show = 0;
	key_delete = 0;
	key_copy = 0;
	key_smooth = 0;
	key_linear = 0;
	key_free = 0;
	key_mbar = 0;
	key_mode_displayed = false;
	key_edit_displayed = false;
}

KeyframePopup::~KeyframePopup()
{
	if( !key_mode_displayed ) {
		delete key_mbar;
		delete key_smooth;
		delete key_linear;
		delete key_free_t;
		delete key_free;
	}
	if( !key_edit_displayed ) {
		delete key_edit;
	}
}

void KeyframePopup::create_objects()
{
	add_item(key_show = new KeyframePopupShow(mwindow, this));
	add_item(key_hide = new KeyframePopupHide(mwindow, this));
	add_item(key_delete = new KeyframePopupDelete(mwindow, this));
	add_item(key_copy = new KeyframePopupCopy(mwindow, this));

	key_edit   = new KeyframePopupEdit(mwindow, this);
	key_mbar   = new BC_MenuItem("-");
	key_smooth = new KeyframePopupCurveMode(mwindow, this, FloatAuto::SMOOTH);
	key_linear = new KeyframePopupCurveMode(mwindow, this, FloatAuto::LINEAR);
	key_free_t = new KeyframePopupCurveMode(mwindow, this, FloatAuto::TFREE );
	key_free   = new KeyframePopupCurveMode(mwindow, this, FloatAuto::FREE  );
}

int KeyframePopup::update(Plugin *plugin, KeyFrame *keyframe)
{
	key_show->set_text(_("Show Plugin Settings"));
	this->keyframe_plugin = plugin;
	this->keyframe_auto = keyframe;
	this->keyframe_autos = 0;
	this->keyframe_automation = 0;
	handle_curve_mode(0, 0);
	return 0;
}

int KeyframePopup::update(Automation *automation, Autos *autos, Auto *auto_keyframe)
{
	key_show->set_text(_(GWindowGUI::auto_text[autos->autoidx]));
	this->keyframe_plugin = 0;
	this->keyframe_automation = automation;
	this->keyframe_autos = autos;
	this->keyframe_auto = auto_keyframe;
	handle_curve_mode(autos, auto_keyframe);

	/* snap to cursor */
	double current_position = mwindow->edl->local_session->get_selectionstart(1);
	double new_position = keyframe_automation->track->from_units(keyframe_auto->position);
	mwindow->edl->local_session->set_selectionstart(new_position);
	mwindow->edl->local_session->set_selectionend(new_position);
	if (current_position != new_position)
	{
		mwindow->edl->local_session->set_selectionstart(new_position);
		mwindow->edl->local_session->set_selectionend(new_position);
		mwindow->gui->lock_window();
		mwindow->gui->update(1, NORMAL_DRAW, 1, 1, 1, 1, 0);
		mwindow->gui->unlock_window();
	}
	return 0;
}

void KeyframePopup::handle_curve_mode(Autos *autos, Auto *auto_keyframe)
// determines the type of automation node. if floatauto, adds
// menu entries showing the curve mode of the node
{
	deactivate();
	if( !key_edit_displayed && keyframe_plugin ) {
		add_item(key_edit);
		key_edit_displayed = true;
	}
	else if( key_edit_displayed && !keyframe_plugin ) {
		remove_item(key_edit);
		key_edit_displayed = false;
	}

	if(!key_mode_displayed && autos && autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{ // append additional menu entries showing the curve_mode
		add_item(key_mbar);
		add_item(key_smooth);
		add_item(key_linear);
		add_item(key_free_t);
		add_item(key_free);
		key_mode_displayed = true;
	}
	else if(key_mode_displayed && (!autos || autos->get_type() != AUTOMATION_TYPE_FLOAT))
	{ // remove additional menu entries
		remove_item(key_free);
		remove_item(key_free_t);
		remove_item(key_linear);
		remove_item(key_smooth);
		remove_item(key_mbar);
		key_mode_displayed = false;
	}
	if(key_mode_displayed && auto_keyframe)
	{ // set checkmarks to display current mode
		key_smooth->toggle_mode((FloatAuto*)auto_keyframe);
		key_linear->toggle_mode((FloatAuto*)auto_keyframe);
		key_free_t->toggle_mode((FloatAuto*)auto_keyframe);
		key_free  ->toggle_mode((FloatAuto*)auto_keyframe);
	}
	activate();
}

KeyframePopupDelete::KeyframePopupDelete(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Delete keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupDelete::~KeyframePopupDelete()
{
}

int KeyframePopupDelete::handle_event()
{
	mwindow->undo->update_undo_before(_("delete keyframe"), 0);
	mwindow->speed_before();
	delete popup->keyframe_auto;
	mwindow->speed_after(1);
	mwindow->undo->update_undo_after(_("delete keyframe"), LOAD_ALL);

	mwindow->save_backup();
	mwindow->gui->update(0, NORMAL_DRAW,
		0, 0, 0, 0, 0);
	mwindow->update_plugin_guis();
	mwindow->restart_brender();
	mwindow->sync_parameters(CHANGE_EDL);

	return 1;
}

KeyframePopupHide::KeyframePopupHide(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Hide keyframe type"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupHide::~KeyframePopupHide()
{
}

int KeyframePopupHide::handle_event()
{
	if( popup->keyframe_autos )
		mwindow->set_auto_visibility(popup->keyframe_autos, 0);
	return 1;
}

KeyframePopupShow::KeyframePopupShow(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Show keyframe settings"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupShow::~KeyframePopupShow()
{
}

int KeyframePopupShow::handle_event()
{
	MWindowGUI *mgui = mwindow->gui;
	CWindowGUI *cgui = mwindow->cwindow->gui;
	int cx = mgui->get_relative_cursor_x()+15, cy = mgui->get_relative_cursor_y()-15;
	delete mgui->keyvalue_popup;
	mgui->keyvalue_popup = 0;

	if( popup->keyframe_plugin ) {
		mwindow->update_plugin_guis();
		mwindow->show_plugin(popup->keyframe_plugin);
	}
	else if( popup->keyframe_automation ) {
		cgui->lock_window();
		int show_window = 1;

		switch( popup->keyframe_autos->autoidx ) {
		case AUTOMATION_CAMERA_X:
		case AUTOMATION_CAMERA_Y:
		case AUTOMATION_CAMERA_Z: {
			cgui->set_operation(CWINDOW_CAMERA);
			break; }

		case AUTOMATION_PROJECTOR_X:
		case AUTOMATION_PROJECTOR_Y:
		case AUTOMATION_PROJECTOR_Z: {
			cgui->set_operation(CWINDOW_PROJECTOR);
			break; }

		case AUTOMATION_MASK: {
			cgui->set_operation(CWINDOW_MASK);
			break; }

		default: {
			show_window = 0;
			PatchGUI *patchgui = mwindow->get_patchgui(popup->keyframe_automation->track);
			if( !patchgui ) break;

			switch( popup->keyframe_autos->autoidx ) {
			case AUTOMATION_MODE: {
				VKeyModePatch *mode = new VKeyModePatch(mwindow, (VPatchGUI *)patchgui);
				mgui->add_subwindow(mode);
				mode->create_objects();
				mode->activate_menu();
				mgui->keyvalue_popup = mode;
			break; }

			case AUTOMATION_PAN: {
				AKeyPanPatch *pan = new AKeyPanPatch(mwindow, (APatchGUI *)patchgui);
				mgui->add_subwindow(pan);
				pan->create_objects();
				pan->activate(cx, cy);
				mgui->keyvalue_popup = pan;
			break; }

			case AUTOMATION_FADE: {
				switch( patchgui->data_type ) {
				case TRACK_AUDIO: {
					AKeyFadePatch *fade = new AKeyFadePatch(mwindow, (APatchGUI *)patchgui, cx, cy);
					mgui->add_subwindow(fade);
					fade->create_objects();
					mgui->keyvalue_popup = fade;
					break; }
				case TRACK_VIDEO: {
					VKeyFadePatch *fade = new VKeyFadePatch(mwindow, (VPatchGUI *)patchgui, cx, cy);
					mgui->add_subwindow(fade);
					fade->create_objects();
					mgui->keyvalue_popup = fade;
					break; }
				}
				break; }

			case AUTOMATION_SPEED: {
				KeySpeedPatch *speed = new KeySpeedPatch(mwindow, patchgui, cx, cy);
				mgui->add_subwindow(speed);
				speed->create_objects();
				mgui->keyvalue_popup = speed;
				break; }

			case AUTOMATION_MUTE: {
				KeyMutePatch *mute = new KeyMutePatch(mwindow, (APatchGUI *)patchgui, cx, cy);
				mgui->add_subwindow(mute);
				mute->create_objects();
				mgui->keyvalue_popup = mute;
				break; }
			}
			break; }
		}

// ensure bringing to front
		if( show_window ) {
			mwindow->show_cwindow();
			CPanelToolWindow *panel_tool_window =
				(CPanelToolWindow *)cgui->composite_panel->operation[CWINDOW_TOOL_WINDOW];
			panel_tool_window->set_shown(0);
			panel_tool_window->set_shown(1);
		}
		cgui->unlock_window();
	}
	return 1;
}



KeyframePopupCopy::KeyframePopupCopy(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Copy keyframe"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupCopy::~KeyframePopupCopy()
{
}

int KeyframePopupCopy::handle_event()
{
/*
	FIXME:
	we want to copy just keyframe under cursor, NOT all keyframes at this frame
	- very hard to do, so this is good approximation for now...
*/

#if 0
	if (popup->keyframe_automation)
	{
		FileXML file;
		EDL *edl = mwindow->edl;
		Track *track = popup->keyframe_automation->track;
		int64_t position = popup->keyframe_auto->position;
		AutoConf autoconf;
// first find out type of our auto
		autoconf.set_all(0);
		if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->projector_autos)
			autoconf.projector = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pzoom_autos)
			autoconf.pzoom = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->camera_autos)
			autoconf.camera = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->czoom_autos)
			autoconf.czoom = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mode_autos)
		   	autoconf.mode = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mask_autos)
			autoconf.mask = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->pan_autos)
			autoconf.pan = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->fade_autos)
			autoconf.fade = 1;
		else if (popup->keyframe_autos == (Autos *)popup->keyframe_automation->mute_autos)
			autoconf.mute = 1;


// now create a clipboard
		file.tag.set_title("AUTO_CLIPBOARD");
		file.tag.set_property("LENGTH", 0);
		file.tag.set_property("FRAMERATE", edl->session->frame_rate);
		file.tag.set_property("SAMPLERATE", edl->session->sample_rate);
		file.append_tag();
		file.append_newline();
		file.append_newline();

/*		track->copy_automation(position,
			position,
			&file,
			0,
			0);
			*/
		file.tag.set_title("TRACK");
// Video or audio
		track->save_header(&file);
		file.append_tag();
		file.append_newline();

		track->automation->copy(position,
			position,
			&file,
			0,
			0,
			&autoconf);
		file.tag.set_title("/TRACK");
		file.append_tag();
		file.append_newline();
		file.append_newline();
		file.append_newline();
		file.append_newline();

		file.tag.set_title("/AUTO_CLIPBOARD");
		file.append_tag();
		file.append_newline();
		file.terminate_string();

		mwindow->gui->lock_window();
		mwindow->gui->to_clipboard(file.string, strlen(file.string), SECONDARY_SELECTION);
		mwindow->gui->unlock_window();

	} else
#endif
		mwindow->copy_automation();
	return 1;
}



KeyframePopupCurveMode::KeyframePopupCurveMode(
	MWindow *mwindow,
	KeyframePopup *popup,
	int curve_mode)
 : BC_MenuItem( get_labeltext(curve_mode))
{
	this->curve_mode = curve_mode;
	this->mwindow = mwindow;
	this->popup = popup;
}

KeyframePopupCurveMode::~KeyframePopupCurveMode() { }


const char* KeyframePopupCurveMode::get_labeltext(int mode)
{
	switch(mode) {
	case FloatAuto::SMOOTH: return _("smooth curve");
	case FloatAuto::LINEAR: return _("linear segments");
	case FloatAuto::TFREE:  return _("tangent edit");
	case FloatAuto::FREE:   return _("disjoint edit");
	}
	return _("misconfigured");
}


void KeyframePopupCurveMode::toggle_mode(FloatAuto *keyframe)
{
	set_checked(curve_mode == keyframe->curve_mode);
}


int KeyframePopupCurveMode::handle_event()
{
	if (popup->keyframe_autos &&
	    popup->keyframe_autos->get_type() == AUTOMATION_TYPE_FLOAT)
	{
		mwindow->undo->update_undo_before(_("change keyframe curve mode"), 0);
		((FloatAuto*)popup->keyframe_auto)->
			change_curve_mode((FloatAuto::t_mode)curve_mode);

		// if we switched to some "auto" mode, this may imply a
		// real change to parameters, so this needs to be undoable...
		mwindow->undo->update_undo_after(_("change keyframe curve mode"), LOAD_ALL);
		mwindow->save_backup();

		mwindow->gui->update(0, NORMAL_DRAW, 0,0,0,0,0); // incremental redraw for canvas
		mwindow->cwindow->update(0,0, 1, 0,0); // redraw tool window in compositor
		mwindow->update_plugin_guis();
		mwindow->restart_brender();
		mwindow->sync_parameters(CHANGE_EDL);
	}
	return 1;
}


KeyframePopupEdit::KeyframePopupEdit(MWindow *mwindow, KeyframePopup *popup)
 : BC_MenuItem(_("Edit Params..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int KeyframePopupEdit::handle_event()
{
	mwindow->show_keyframe_gui(popup->keyframe_plugin);
	return 1;
}


KeyframeHidePopup::KeyframeHidePopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	this->keyframe_autos = 0;
}

KeyframeHidePopup::~KeyframeHidePopup()
{
}

void KeyframeHidePopup::create_objects()
{
	add_item(new KeyframeHideItem(mwindow, this));
}

int KeyframeHidePopup::update(Autos *autos)
{
	this->keyframe_autos = autos;
	return 0;
}

KeyframeHideItem::KeyframeHideItem(MWindow *mwindow, KeyframeHidePopup *popup)
 : BC_MenuItem(_("Hide keyframe type"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


int KeyframeHideItem::handle_event()
{
	if( popup->keyframe_autos )
		mwindow->set_auto_visibility(popup->keyframe_autos, 0);
	return 1;
}



KeyMutePatch::KeyMutePatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_SubWindow(x, y, 100, 20, GWindowGUI::auto_colors[AUTOMATION_MUTE])
{
	this->mwindow = mwindow;
	this->patch = patch;
}

void KeyMutePatch::create_objects()
{
	key_mute_checkbox = new KeyMuteValue(this);
	add_subwindow(key_mute_checkbox);
	key_mute_checkbox->activate();
	show_window();
}

KeyMuteValue::KeyMuteValue(KeyMutePatch *key_mute_patch)
 : BC_CheckBox(0,0, key_mute_patch->mwindow->
	get_int_auto(key_mute_patch->patch, AUTOMATION_MUTE)->value,
	_("Mute"), MEDIUMFONT, RED)
{
	this->key_mute_patch = key_mute_patch;
}

int KeyMuteValue::button_release_event()
{
	BC_CheckBox::button_release_event();
	MWindowGUI *mgui = key_mute_patch->mwindow->gui;
	delete mgui->keyvalue_popup;
	mgui->keyvalue_popup = 0;
	return 1;
}

int KeyMuteValue::handle_event()
{
	MWindow *mwindow = key_mute_patch->mwindow;
	PatchGUI *patch = key_mute_patch->patch;

	patch->change_source = 1;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *mute_autos = patch->track->automation->autos[AUTOMATION_MUTE];
	int need_undo = !mute_autos->auto_exists_for_editing(position);
	mwindow->undo->update_undo_before(_("mute"), need_undo ? 0 : this);
	IntAuto *current = (IntAuto*)mute_autos->get_auto_for_editing(position);
	current->value = this->get_value();
	mwindow->undo->update_undo_after(_("mute"), LOAD_AUTOMATION);
	patch->change_source = 0;

	mwindow->sync_parameters(CHANGE_PARAMS);
	if( mwindow->edl->session->auto_conf->autos[AUTOMATION_MUTE] ) {
		mwindow->gui->update_patchbay();
		mwindow->gui->draw_overlays(1);
	}
	return 1;
}

KeySpeedPatch::KeySpeedPatch(MWindow *mwindow, PatchGUI *patch, int x, int y)
 : BC_SubWindow(x,y, 200,20, GWindowGUI::auto_colors[AUTOMATION_SPEED])
{
	this->mwindow = mwindow;
	this->patch = patch;
}

void KeySpeedPatch::create_objects()
{
	int x = 0, y = 0;
	float v = mwindow->get_float_auto(patch, AUTOMATION_SPEED)->get_value();
	add_subwindow(key_speed_text = new KeySpeedText(this, x, y, 64, v));
	x += key_speed_text->get_w();
	VFrame **lok_images = mwindow->theme->get_image_set("lok");
	int w1 = get_w() - x - lok_images[0]->get_w();
	add_subwindow(key_speed_slider = new KeySpeedSlider(this, x, y, w1, v));
	x += key_speed_slider->get_w();
	add_subwindow(key_speed_ok = new KeySpeedOK(this, x, y, lok_images));
	activate();
	show_window();
}

int KeySpeedPatch::cursor_enter_event()
{
	if( is_event_win() )
		mwindow->speed_before();
	return 1;
}

int KeySpeedPatch::cursor_leave_event()
{
	if( is_event_win() ) {
		mwindow->speed_after(1);
		mwindow->resync_guis();
	}
	return 1;
}

void KeySpeedPatch::update(float v)
{
	key_speed_text->update(v);
	key_speed_slider->update(v);
	update_speed(v);
}

void KeySpeedPatch::update_speed(float v)
{
	patch->change_source = 1;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Track *track = patch->track;
	Autos *speed_autos = track->automation->autos[AUTOMATION_SPEED];
	int need_undo = !speed_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("speed"), need_undo ? 0 : this);
	FloatAuto *current = (FloatAuto*)speed_autos->get_auto_for_editing(position);
	float change = v - current->get_value();
	current->set_value(v);
	if( track->gang && track->record ) {
		TrackCanvas *track_canvas = patch->patchbay->pane->canvas;
		track_canvas->fill_ganged_autos(1, change, track, current);
		track_canvas->update_ganged_autos(0, track, current);
		track_canvas->clear_ganged_autos();
	}
	mwindow->undo->update_undo_after(_("speed"), LOAD_AUTOMATION+LOAD_EDITS);
	patch->change_source = 0;

	mwindow->sync_parameters(CHANGE_PARAMS);
	if(mwindow->edl->session->auto_conf->autos[AUTOMATION_SPEED]) {
		mwindow->gui->draw_overlays(1);
	}
}

KeySpeedOK::KeySpeedOK(KeySpeedPatch *key_speed_patch, int x, int y, VFrame **images)
 : BC_Button(x, y, images)
{
	this->key_speed_patch = key_speed_patch;
}

int KeySpeedOK::handle_event()
{
	MWindow *mwindow = key_speed_patch->mwindow;
	mwindow->speed_after(1);
	mwindow->resync_guis();
	MWindowGUI *mgui = mwindow->gui;
	delete mgui->keyvalue_popup;
	mgui->keyvalue_popup = 0;
	return 1;
}

KeySpeedText::KeySpeedText(KeySpeedPatch *key_speed_patch, int x, int y, int w, float v)
 : BC_TextBox(x, y, w, 1, v, 1, MEDIUMFONT, 2)
{
	this->key_speed_patch = key_speed_patch;
}

int KeySpeedText::handle_event()
{
	float v = atof(get_text());
	key_speed_patch->update(v);
	return get_keypress() != RETURN ? 1 :
		key_speed_patch->key_speed_ok->handle_event();
}

KeySpeedSlider::KeySpeedSlider(KeySpeedPatch *key_speed_patch,
		int x, int y, int w, float v)
 : BC_FSlider(x, y, 0, w, w,
	key_speed_patch->mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_SPEED],
	key_speed_patch->mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_SPEED],
	v)
{
	this->key_speed_patch = key_speed_patch;
	key_speed_patch->mwindow->speed_before();
	set_precision(0.01);
}

KeySpeedSlider::~KeySpeedSlider()
{
}

int KeySpeedSlider::handle_event()
{
	if( shift_down() ) {
		update(1.0);
		set_tooltip(get_caption());
	}
	key_speed_patch->update(get_value());
	return 1;
}

