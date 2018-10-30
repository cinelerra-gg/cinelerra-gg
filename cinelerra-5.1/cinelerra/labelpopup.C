/*
 * CINELERRA
 * Copyright (C) 2006 Pierre Dumuid
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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

#include "awindow.h"
#include "awindowgui.h"
#include "bcdialog.h"
#include "labeledit.h"
#include "labelpopup.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"


LabelPopup::LabelPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

LabelPopup::~LabelPopup()
{
}

void LabelPopup::create_objects()
{
	add_item(editlabel = new LabelPopupEdit(mwindow, gui));
	add_item(new LabelPopupDelete(mwindow, gui));
	add_item(new LabelPopupGoTo(mwindow, gui));
}

int LabelPopup::update()
{
	gui->collect_assets();
	return 0;
}


LabelPopupEdit::LabelPopupEdit(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Edit..."))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

LabelPopupEdit::~LabelPopupEdit()
{
}

int LabelPopupEdit::handle_event()
{
	AssetPicon *result = (AssetPicon*)gui->asset_list->get_selection(0,0);
	if( result && result->label ) {
		int cur_x, cur_y;
		gui->get_abs_cursor(cur_x, cur_y, 0);
		gui->awindow->label_edit->start(result->label, cur_x, cur_y);
	}
	return 1;
}

LabelPopupDelete::LabelPopupDelete(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
LabelPopupDelete::~LabelPopupDelete()
{
}

int LabelPopupDelete::handle_event()
{
	AssetPicon *result = (AssetPicon*)gui->asset_list->get_selection(0,0);
	if( result && result->label ) {
		delete result->label;
		mwindow->gui->lock_window("LabelPopupDelete::handle_event");
		mwindow->gui->update_timebar(0);
		mwindow->gui->unlock_window();
		gui->async_update_assets();
	}
	return 1;
}


LabelPopupGoTo::LabelPopupGoTo(MWindow *mwindow, AWindowGUI *gui)
 : BC_MenuItem(_("Go to"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
LabelPopupGoTo::~LabelPopupGoTo()
{
}

int LabelPopupGoTo::handle_event()
{
	AssetPicon *result = (AssetPicon*)gui->asset_list->get_selection(0,0);
	if( result && result->label ) {
		double position = result->label->position;
		gui->unlock_window();
		int locked = mwindow->gui->get_window_lock();
		if( locked ) mwindow->gui->unlock_window();
		PlayTransport *transport = mwindow->gui->mbuttons->transport;
		transport->change_position(position);
		if( locked ) mwindow->gui->lock_window("LabelPopupGoTo::handle_event");
		gui->lock_window("LabelPopupGoTo::handle_event 1");
	}
	return 1;
}


LabelListMenu::LabelListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

LabelListMenu:: ~LabelListMenu()
{
}

void LabelListMenu::create_objects()
{
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new AWindowListSort(mwindow, gui));
}

void LabelListMenu::update()
{
	format->update();
}

