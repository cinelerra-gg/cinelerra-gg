
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

#include "asset.h"
#include "assets.h"
#include "awindow.h"
#include "awindowgui.h"
#include "edit.h"
#include "edits.h"
#include "editpopup.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugindialog.h"
#include "resizetrackthread.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"

#include <string.h>

EditPopup::EditPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;

	track = 0;
	edit = 0;
	plugin = 0;
	pluginset = 0;
	position = 0;
}

EditPopup::~EditPopup()
{
}

void EditPopup::create_objects()
{
	add_item(new EditPopupClearSelect(mwindow, this));
	add_item(new EditPopupCopy(mwindow, this));
	add_item(new EditPopupCut(mwindow, this));
	add_item(new EditPopupMute(mwindow, this));
	add_item(new EditPopupCopyPack(mwindow, this));
	add_item(new EditPopupCutPack(mwindow, this));
	add_item(new EditPopupMutePack(mwindow, this));
	add_item(new EditPopupPaste(mwindow, this));
	add_item(new EditPopupOverwrite(mwindow, this));
	add_item(new EditPopupOverwritePlugins(mwindow, this));
}

int EditPopup::activate_menu(Track *track, Edit *edit,
		PluginSet *pluginset, Plugin *plugin, double position)
{
	this->track = track;
	this->edit = edit;
	this->pluginset = pluginset;
	this->plugin = plugin;
	this->position = position;
	return BC_PopupMenu::activate_menu();
}

EditPopupClearSelect::EditPopupClearSelect(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Clear Select"),_("Ctrl-Shift-A"),'A')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupClearSelect::handle_event()
{
	mwindow->edl->tracks->clear_selected_edits();
	popup->gui->draw_overlays(1);
	return 1;
}

EditPopupCopy::EditPopupCopy(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Copy"),_("Ctrl-c"),'c')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupCopy::handle_event()
{
	mwindow->selected_edits_to_clipboard(0);
	return 1;
}

EditPopupCopyPack::EditPopupCopyPack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Copy pack"),_("Ctrl-Shift-C"),'C')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupCopyPack::handle_event()
{
	mwindow->selected_edits_to_clipboard(1);
	return 1;
}

EditPopupCut::EditPopupCut(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Cut"),_("Ctrl-x"),'x')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupCut::handle_event()
{
	mwindow->cut_selected_edits(1, 0);
	return 1;
}

EditPopupCutPack::EditPopupCutPack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Cut pack"),_("Ctrl-z"),'z')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupCutPack::handle_event()
{
	mwindow->cut_selected_edits(1, 1);
	return 1;
}

EditPopupMute::EditPopupMute(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Mute"),_("Ctrl-m"),'m')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupMute::handle_event()
{
	mwindow->cut_selected_edits(0, 0);
	return 1;
}

EditPopupMutePack::EditPopupMutePack(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Mute pack"),_("Ctrl-Shift-M"),'M')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupMutePack::handle_event()
{
	mwindow->cut_selected_edits(0, 1);
	return 1;
}

EditPopupPaste::EditPopupPaste(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Paste"),_("Ctrl-v"),'v')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupPaste::handle_event()
{
	mwindow->paste(popup->position, popup->track, 0, 0);
	mwindow->edl->tracks->clear_selected_edits();
	popup->gui->draw_overlays(1);
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}

EditPopupOverwrite::EditPopupOverwrite(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Overwrite"),_("Ctrl-b"),'b')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
}

int EditPopupOverwrite::handle_event()
{
	mwindow->paste(popup->position, popup->track, 0, -1);
	mwindow->edl->tracks->clear_selected_edits();
	popup->gui->draw_overlays(1);
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}

EditPopupOverwritePlugins::EditPopupOverwritePlugins(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Overwrite Plugins"),_("Ctrl-Shift-P"),'P')
{
	this->mwindow = mwindow;
	this->popup = popup;
	set_ctrl(1);
	set_shift(1);
}

int EditPopupOverwritePlugins::handle_event()
{
	mwindow->paste_clipboard(popup->track, popup->position, 1, 0,
			mwindow->edl->session->labels_follow_edits,
			mwindow->edl->session->autos_follow_edits,
			mwindow->edl->session->plugins_follow_edits);
	mwindow->edl->tracks->clear_selected_edits();
	if( mwindow->session->current_operation == DROP_TARGETING ) {
		mwindow->session->current_operation = NO_OPERATION;
		popup->gui->update_cursor();
	}
	return 1;
}

