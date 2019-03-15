
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
#include "overlayframe.inc"
#include "patchbay.h"
#include "theme.h"
#include "trackcanvas.h"
#include "vpatchgui.h"
#include "vtrack.h"
#include "vwindow.h"

#include <string.h>


VPatchGUI::VPatchGUI(MWindow *mwindow, PatchBay *patchbay, VTrack *track, int x, int y)
 : PatchGUI(mwindow, patchbay, track, x, y)
{
	data_type = TRACK_VIDEO;
	this->vtrack = track;
	mode = 0;
	fade = 0;
}

VPatchGUI::~VPatchGUI()
{
	if( fade ) delete fade;
	if( mode ) delete mode;
}

void VPatchGUI::create_objects()
{
	update(x, y);
}

int VPatchGUI::reposition(int x, int y)
{
	//int x1 = 0;
	int y1 = PatchGUI::reposition(x, y);

	if( fade )
		fade->reposition_window(fade->get_x(), y1+y);
	y1 += mwindow->theme->fade_h;
	if( mix )
		mix->reposition_window(mix->get_x(), y1+y);
	if( mode )
		mode->reposition_window(mode->get_x(), y1+y);
	if( nudge )
		nudge->reposition_window(nudge->get_x(), y1+y);
	y1 += mwindow->theme->mode_h;
	return y1;
}

int VPatchGUI::update(int x, int y)
{
	int h = track->vertical_span(mwindow->theme);
	int x1 = 0;
	int y1 = PatchGUI::update(x, y);

	int y2 = y1 + mwindow->theme->fade_h;
	if( fade ) {
		if( h < y2 ) {
			delete fade;  fade = 0;
		}
		else {
			fade->update(fade->get_w(), mwindow->get_float_auto(this, AUTOMATION_FADE)->get_value(),
				     mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
				     mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE]);
		}
	}
	else if( h >= y2 ) {
		int64_t v = mwindow->get_float_auto(this, AUTOMATION_FADE)->get_value();
		patchbay->add_subwindow(fade = new VFadePatch(this, x1+x, y1+y,
			patchbay->get_w() - 10, v));
	}
	y1 = y2;

	y2 = y1 + mwindow->theme->mode_h;
	if( mode ) {
		if( h < y2 ) {
			delete mix;    mix = 0;
			delete mode;   mode = 0;
			delete nudge;  nudge = 0;
		}
		else {
			if( mwindow->session->selected_zwindow >= 0 ) {
				int v = mwindow->mixer_track_active(track);
				mix->update(v);
			}
			mode->update(mwindow->get_int_auto(this, AUTOMATION_MODE)->value);
			nudge->update();
		}
	}
	else if( h >= y2 ) {
		patchbay->add_subwindow(mix = new VMixPatch(mwindow, this, x1+x, y1+y+5));
		x1 += mix->get_w();
		patchbay->add_subwindow(mode = new VModePatch(mwindow, this, x1+x, y1+y));
		mode->create_objects();
		x1 += mode->get_w();
		patchbay->add_subwindow(nudge = new NudgePatch(mwindow, this, x1+x, y1+y,
			patchbay->get_w() - x1-x - 10));
	}
	y1 = y2;

	return y1;
}

VFadePatch::VFadePatch(VPatchGUI *patch, int x, int y, int w, int64_t v)
 : BC_ISlider(x, y, 0, w, w,
		patch->mwindow->edl->local_session->automation_mins[AUTOGROUPTYPE_VIDEO_FADE],
		patch->mwindow->edl->local_session->automation_maxs[AUTOGROUPTYPE_VIDEO_FADE],
		v)
{
	this->patch = patch;
}

void VPatchGUI::update_faders(float v)
{
	if( fade )
		fade->update(v);

	change_source = 1;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *fade_autos = vtrack->automation->autos[AUTOMATION_FADE];
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

int VFadePatch::handle_event()
{
	if( shift_down() ) {
		update(100);
		set_tooltip(get_caption());
	}
	patch->update_faders(get_value());
	return 1;
}

VKeyFadePatch::VKeyFadePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : BC_SubWindow(x,y, 200,20, GWindowGUI::auto_colors[AUTOMATION_FADE])
{
	this->mwindow = mwindow;
	this->patch = patch;
}

void VKeyFadePatch::create_objects()
{
	int x = 0, y = 0;
	int64_t v = mwindow->get_float_auto(patch, AUTOMATION_FADE)->get_value();
	add_subwindow(vkey_fade_text = new VKeyFadeText(this, x, y, 64, v));
	x += vkey_fade_text->get_w();
	VFrame **lok_images = mwindow->theme->get_image_set("lok");
	int w1 = get_w() - x - lok_images[0]->get_w();
	add_subwindow(vkey_fade_slider = new VKeyFadeSlider(this, x, y, w1, v));
	x += vkey_fade_slider->get_w();
	add_subwindow(vkey_fade_ok = new VKeyFadeOK(this, x, y, lok_images));
	activate();
	show_window();
}

void VKeyFadePatch::update(int64_t v)
{
	vkey_fade_text->update(v);
	vkey_fade_slider->update(v);
	patch->update_faders(v);
}

VKeyFadeOK::VKeyFadeOK(VKeyFadePatch *vkey_fade_patch, int x, int y, VFrame **images)
 : BC_Button(x, y, images)
{
	this->vkey_fade_patch = vkey_fade_patch;
}

int VKeyFadeOK::handle_event()
{
	MWindowGUI *mgui = vkey_fade_patch->mwindow->gui;
	delete mgui->keyvalue_popup;
	mgui->keyvalue_popup = 0;
	return 1;
}

VKeyFadeText::VKeyFadeText(VKeyFadePatch *vkey_fade_patch, int x, int y, int w, int64_t v)
 : BC_TextBox(x, y, w, 1, v, 1, MEDIUMFONT)
{
	this->vkey_fade_patch = vkey_fade_patch;
}

int VKeyFadeText::handle_event()
{
	int64_t v = atoi(get_text());
	vkey_fade_patch->update(v);
	return get_keypress() != RETURN ? 1 :
		vkey_fade_patch->vkey_fade_ok->handle_event();
}

VKeyFadeSlider::VKeyFadeSlider(VKeyFadePatch *vkey_fade_patch,
	int x, int y, int w, int64_t v)
 : VFadePatch(vkey_fade_patch->patch, x,y, w, v)
{
	this->vkey_fade_patch = vkey_fade_patch;
}

int VKeyFadeSlider::handle_event()
{
	int64_t v = get_value();
	vkey_fade_patch->update(v);
	return 1;
}


VModePatch::VModePatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : BC_PopupMenu(x, y, patch->patchbay->mode_icons[0]->get_w() + 20,
	"", 1, mwindow->theme->get_image_set("mode_popup", 0), 0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	this->mode = mwindow->get_int_auto(patch, AUTOMATION_MODE)->value;
	set_icon(patch->patchbay->mode_to_icon(this->mode));
	set_tooltip(_("Overlay mode"));
}

VModePatch::VModePatch(MWindow *mwindow, VPatchGUI *patch)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->patch = patch;
	this->mode = mwindow->get_int_auto(patch, AUTOMATION_MODE)->value;
}


int VModePatch::handle_event()
{
// Set menu items
//	for( int i = 0; i < total_items(); i++ )
//	{
//		VModePatchItem *item = (VModePatchItem*)get_item(i);
//		if( item->mode == mode )
//			item->set_checked(1);
//		else
//			item->set_checked(0);
//	}
	update(mode);

// Set keyframe
	IntAuto *current;
	double position = mwindow->edl->local_session->get_selectionstart(1);
	Autos *mode_autos = patch->vtrack->automation->autos[AUTOMATION_MODE];
	int need_undo = !mode_autos->auto_exists_for_editing(position);

	mwindow->undo->update_undo_before(_("mode"), need_undo ? 0 : this);

	current = (IntAuto*)mode_autos->get_auto_for_editing(position);
	current->value = mode;

	mwindow->undo->update_undo_after(_("mode"), LOAD_AUTOMATION);

	mwindow->sync_parameters(CHANGE_PARAMS);

	if( mwindow->edl->session->auto_conf->autos[AUTOMATION_MODE] ) {
		mwindow->gui->draw_overlays(1);
	}
	mwindow->session->changes_made = 1;
	return 1;
}

void VModePatch::create_objects()
{
	VModePatchItem *mode_item;
	VModePatchSubMenu *submenu;
	add_item(mode_item = new VModePatchItem(this, mode_to_text(TRANSFER_NORMAL),  TRANSFER_NORMAL));
	add_item(mode_item = new VModePatchItem(this, _("Arithmetic..."), -1));
	mode_item->add_submenu(submenu = new VModePatchSubMenu(mode_item));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_ADDITION), TRANSFER_ADDITION));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SUBTRACT), TRANSFER_SUBTRACT));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DIVIDE),   TRANSFER_DIVIDE));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_MULTIPLY), TRANSFER_MULTIPLY));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_REPLACE),  TRANSFER_REPLACE));
	add_item(mode_item = new VModePatchItem(this, _("PorterDuff..."), -1));
	mode_item->add_submenu(submenu = new VModePatchSubMenu(mode_item));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DST),	TRANSFER_DST));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DST_ATOP), TRANSFER_DST_ATOP));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DST_IN),   TRANSFER_DST_IN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DST_OUT),  TRANSFER_DST_OUT));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DST_OVER), TRANSFER_DST_OVER));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SRC),	TRANSFER_SRC));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SRC_ATOP), TRANSFER_SRC_ATOP));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SRC_IN),   TRANSFER_SRC_IN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SRC_OUT),  TRANSFER_SRC_OUT));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SRC_OVER), TRANSFER_SRC_OVER));
	add_item(mode_item = new VModePatchItem(this, _("Logical..."), -1));
	mode_item->add_submenu(submenu = new VModePatchSubMenu(mode_item));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_MIN),	TRANSFER_MIN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_MAX),	TRANSFER_MAX));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DARKEN),	TRANSFER_DARKEN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_LIGHTEN),	TRANSFER_LIGHTEN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_AND),	TRANSFER_AND));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_OR),	TRANSFER_OR));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_XOR),	TRANSFER_XOR));
	add_item(mode_item = new VModePatchItem(this, _("Graphic Art..."), -1));
	mode_item->add_submenu(submenu = new VModePatchSubMenu(mode_item));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_OVERLAY),	TRANSFER_OVERLAY));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SCREEN),	TRANSFER_SCREEN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_BURN),	TRANSFER_BURN));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DODGE),	TRANSFER_DODGE));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_DIFFERENCE),TRANSFER_DIFFERENCE));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_HARDLIGHT),TRANSFER_HARDLIGHT));
	submenu->add_submenuitem(new VModeSubMenuItem(submenu, mode_to_text(TRANSFER_SOFTLIGHT),TRANSFER_SOFTLIGHT));
}

void VModePatch::update(int mode)
{
	set_icon(patch->patchbay->mode_to_icon(mode));
	for( int i = 0; i < total_items(); i++ ) {
		VModePatchItem *item = (VModePatchItem*)get_item(i);
		item->set_checked(item->mode == mode);
		VModePatchSubMenu *submenu = (VModePatchSubMenu *)item->get_submenu();
		if( !submenu ) continue;
		int n = submenu->total_items();
		for( int j=0; j<n; ++j ) {
			VModePatchItem *subitem = (VModePatchItem*)submenu->get_item(j);
			subitem->set_checked(subitem->mode == mode);
		}
	}
}


const char* VModePatch::mode_to_text(int mode)
{
	switch( mode ) {
	case TRANSFER_NORMAL:		return _("Normal");
	case TRANSFER_ADDITION:		return _("Addition");
	case TRANSFER_SUBTRACT:		return _("Subtract");
	case TRANSFER_MULTIPLY:		return _("Multiply");
	case TRANSFER_DIVIDE:  		return _("Divide");
	case TRANSFER_REPLACE:		return _("Replace");
	case TRANSFER_MAX:   		return _("Max");
	case TRANSFER_MIN:   		return _("Min");
	case TRANSFER_DARKEN:		return _("Darken");
	case TRANSFER_LIGHTEN:		return _("Lighten");
	case TRANSFER_DST:		return _("Dst");
	case TRANSFER_DST_ATOP:		return _("DstAtop");
	case TRANSFER_DST_IN:		return _("DstIn");
	case TRANSFER_DST_OUT:		return _("DstOut");
	case TRANSFER_DST_OVER:		return _("DstOver");
	case TRANSFER_SRC:		return _("Src");
	case TRANSFER_SRC_ATOP:		return _("SrcAtop");
	case TRANSFER_SRC_IN:		return _("SrcIn");
	case TRANSFER_SRC_OUT:		return _("SrcOut");
	case TRANSFER_SRC_OVER:		return _("SrcOver");
	case TRANSFER_AND:		return _("AND");
	case TRANSFER_OR:		return _("OR");
	case TRANSFER_XOR:		return _("XOR");
	case TRANSFER_OVERLAY:		return _("Overlay");
	case TRANSFER_SCREEN:		return _("Screen");
	case TRANSFER_BURN:		return _("Burn");
	case TRANSFER_DODGE:		return _("Dodge");
	case TRANSFER_HARDLIGHT:	return _("Hardlight");
	case TRANSFER_SOFTLIGHT:	return _("Softlight");
	case TRANSFER_DIFFERENCE:	return _("Difference");
	}
	return _("Normal");
}



VModePatchItem::VModePatchItem(VModePatch *popup, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->mode = mode;
	if( this->mode == popup->mode ) set_checked(1);
}

int VModePatchItem::handle_event()
{
	if( mode >= 0 ) {
		popup->mode = mode;
//		popup->set_icon(popup->patch->patchbay->mode_to_icon(mode));
		popup->handle_event();
	}
	return 1;
}

VModePatchSubMenu::VModePatchSubMenu(VModePatchItem *mode_item)
{
	this->mode_item = mode_item;
}
VModePatchSubMenu::~VModePatchSubMenu()
{
}

VModeSubMenuItem::VModeSubMenuItem(VModePatchSubMenu *submenu, const char *text, int mode)
 : BC_MenuItem(text)
{
	this->submenu = submenu;
	this->mode = mode;
	VModePatch *popup = submenu->mode_item->popup;
	if( this->mode == popup->mode ) set_checked(1);
}
VModeSubMenuItem::~VModeSubMenuItem()
{
}

int VModeSubMenuItem::handle_event()
{
	VModePatch *popup = submenu->mode_item->popup;
	popup->mode = mode;
//	popup->set_icon(popup->patch->patchbay->mode_to_icon(mode));
	popup->handle_event();
	return 1;
}


VKeyModePatch::VKeyModePatch(MWindow *mwindow, VPatchGUI *patch)
 : VModePatch(mwindow, patch)
{
}

int VKeyModePatch::handle_event()
{
	int ret = VModePatch::handle_event();
	VModePatch *mode = patch->mode;
	if( mode )
		mode->update(this->mode);
	return ret;
}


VMixPatch::VMixPatch(MWindow *mwindow, VPatchGUI *patch, int x, int y)
 : MixPatch(mwindow, patch, x, y)
{
	set_tooltip(_("Mixer"));
}

VMixPatch::~VMixPatch()
{
}

