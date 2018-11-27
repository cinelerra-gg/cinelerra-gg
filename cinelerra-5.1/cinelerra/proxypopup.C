
/*
 * CINELERRA
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

#include "assetedit.h"
#include "assetremove.h"
#include "proxypopup.h"
#include "awindow.h"
#include "awindowgui.h"
#include "bcsignals.h"
#include "clipedit.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "filexml.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "track.h"
#include "tracks.h"
#include "vwindow.h"
#include "vwindowgui.h"



ProxyPopup::ProxyPopup(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

ProxyPopup::~ProxyPopup()
{
}

void ProxyPopup::create_objects()
{
	BC_MenuItem *menu_item;
	BC_SubMenu *submenu;
	add_item(info = new ProxyPopupInfo(mwindow, this));
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(new ProxyPopupSort(mwindow, this));
	add_item(view = new ProxyPopupView(mwindow, this));
	add_item(view_window = new ProxyPopupViewWindow(mwindow, this));
	add_item(new ProxyPopupCopy(mwindow, this));
	add_item(new ProxyPopupPaste(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Remove...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new ProxyPopupProjectRemove(mwindow, this));
	submenu->add_submenuitem(new ProxyPopupDiskRemove(mwindow, this));
}

void ProxyPopup::paste_assets()
{
// Collect items into the drag vectors for temporary storage
	gui->lock_window("ProxyPopup::paste_assets");
	mwindow->gui->lock_window("ProxyPopup::paste_assets");
	mwindow->cwindow->gui->lock_window("ProxyPopup::paste_assets");

	gui->collect_assets(1);
	mwindow->paste_assets(mwindow->edl->local_session->get_selectionstart(1),
		mwindow->edl->tracks->first, 0);   // do not overwrite

	gui->unlock_window();
	mwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
}

int ProxyPopup::update()
{
	format->update();
	gui->collect_assets(1);
	return 0;
}


ProxyPopupInfo::ProxyPopupInfo(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Info..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupInfo::~ProxyPopupInfo()
{
}

int ProxyPopupInfo::handle_event()
{
	int cur_x, cur_y;
	popup->gui->get_abs_cursor(cur_x, cur_y, 0);

	if( mwindow->session->drag_assets->total ) {
		AssetEdit *asset_edit = mwindow->awindow->get_asset_editor();
		asset_edit->edit_asset(
			mwindow->session->drag_assets->values[0], cur_x, cur_y);
	}
	else
	if( mwindow->session->drag_clips->total ) {
		popup->gui->awindow->clip_edit->edit_clip(
			mwindow->session->drag_clips->values[0], cur_x, cur_y);
	}
	return 1;
}


ProxyPopupSort::ProxyPopupSort(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupSort::~ProxyPopupSort()
{
}

int ProxyPopupSort::handle_event()
{
	mwindow->awindow->gui->sort_assets();
	return 1;
}


ProxyPopupView::ProxyPopupView(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("View"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupView::~ProxyPopupView()
{
}

int ProxyPopupView::handle_event()
{
	VWindow *vwindow = mwindow->get_viewer(1, DEFAULT_VWINDOW);

	if( mwindow->session->drag_assets->total )
		vwindow->change_source(
			mwindow->session->drag_assets->values[0]);
	else
	if( mwindow->session->drag_clips->total )
		vwindow->change_source(
			mwindow->session->drag_clips->values[0]);

	return 1;
}


ProxyPopupViewWindow::ProxyPopupViewWindow(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("View in new window"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupViewWindow::~ProxyPopupViewWindow()
{
}

int ProxyPopupViewWindow::handle_event()
{
	for( int i=0; i<mwindow->session->drag_assets->size(); ++i ) {
		VWindow *vwindow = mwindow->get_viewer(1);
		vwindow->gui->lock_window("ProxyPopupView::handle_event 1");
		vwindow->change_source(mwindow->session->drag_assets->get(i));
		vwindow->gui->unlock_window();
	}
	for( int i=0; i<mwindow->session->drag_clips->size(); ++i ) {
		VWindow *vwindow = mwindow->get_viewer(1);
		vwindow->gui->lock_window("ProxyPopupView::handle_event 2");
		vwindow->change_source(mwindow->session->drag_clips->get(i));
		vwindow->gui->unlock_window();
	}
	return 1;
}


ProxyPopupCopy::ProxyPopupCopy(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Copy"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
ProxyPopupCopy::~ProxyPopupCopy()
{
}

int ProxyPopupCopy::handle_event()
{
	MWindowGUI *gui = mwindow->gui;
	gui->lock_window("ProxyPopupCopy::handle_event");
	if( mwindow->session->drag_clips->total > 0 ) {
		EDL *edl = mwindow->session->drag_clips->values[0];
		EDL *copy_edl = new EDL; // no parent or assets wont be copied
		copy_edl->create_objects();
		copy_edl->copy_all(edl);
		FileXML file;
		double start = 0, end = edl->tracks->total_length();
		copy_edl->copy(start, end, 1, &file, "", 1);
		copy_edl->remove_user();
		const char *file_string = file.string();
		long file_length = strlen(file_string);
		gui->to_clipboard(file_string, file_length, SECONDARY_SELECTION);
		gui->to_clipboard(file_string, file_length, BC_PRIMARY_SELECTION);
	}
	gui->unlock_window(); 
	return 1;
}


ProxyPopupPaste::ProxyPopupPaste(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Paste"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupPaste::~ProxyPopupPaste()
{
}

int ProxyPopupPaste::handle_event()
{
	popup->paste_assets();
	return 1;
}


ProxyPopupProjectRemove::ProxyPopupProjectRemove(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Remove from project"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

ProxyPopupProjectRemove::~ProxyPopupProjectRemove()
{
}

int ProxyPopupProjectRemove::handle_event()
{
	popup->gui->collect_assets();
	mwindow->remove_assets_from_project(1, 1,
		mwindow->session->drag_assets,
		mwindow->session->drag_clips);
	return 1;
}


ProxyPopupDiskRemove::ProxyPopupDiskRemove(MWindow *mwindow, ProxyPopup *popup)
 : BC_MenuItem(_("Remove from disk"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}


ProxyPopupDiskRemove::~ProxyPopupDiskRemove()
{
}

int ProxyPopupDiskRemove::handle_event()
{
	popup->gui->collect_assets();
	mwindow->awindow->asset_remove->start();
	return 1;
}



ProxyListMenu::ProxyListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

ProxyListMenu::~ProxyListMenu()
{
}

void ProxyListMenu::create_objects()
{
	add_item(format = new AWindowListFormat(mwindow, gui));
	add_item(select_used = new AssetSelectUsed(mwindow, gui));
	BC_SubMenu *submenu;
	select_used->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("All"), SELECT_ALL));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("Used"), SELECT_USED));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("Unused"), SELECT_UNUSED));
	submenu->add_submenuitem(new AssetSelectUsedItem(select_used, _("None"), SELECT_NONE));
	add_item(new AWindowListSort(mwindow, gui));
	update();
}

void ProxyListMenu::update()
{
	format->update();
}


