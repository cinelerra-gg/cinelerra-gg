
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

#include "awindow.h"
#include "awindowgui.h"
#include "folderlistmenu.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"


FolderListMenu::FolderListMenu(MWindow *mwindow, AWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

FolderListMenu::~FolderListMenu()
{
}

void FolderListMenu::create_objects()
{
	BC_MenuItem *menu_item;
	BC_SubMenu *submenu;
	add_item(format = new FolderListFormat(mwindow, this));
	add_item(new FolderListSort(mwindow, this));
	add_item(menu_item = new BC_MenuItem(_("Folder...")));
	menu_item->add_submenu(submenu = new BC_SubMenu());
	submenu->add_submenuitem(new FolderListNew(mwindow, this, _("New Media"), 0));
	submenu->add_submenuitem(new FolderListNew(mwindow, this, _("New Clips"), 1));
	submenu->add_submenuitem(new FolderListModify(mwindow, this));
	submenu->add_submenuitem(new FolderListDelete(mwindow, this));
	update_titles();
}

void FolderListMenu::update_titles()
{
	format->set_text(mwindow->edl->session->folderlist_format == FOLDERS_TEXT ?
		(char*)_("Display icons") : (char*)_("Display text"));
}


FolderListFormat::FolderListFormat(MWindow *mwindow, FolderListMenu *menu)
 : BC_MenuItem("")
{
	this->mwindow = mwindow;
	this->menu = menu;
}

int FolderListFormat::handle_event()
{
	switch(mwindow->edl->session->folderlist_format) {
	case FOLDERS_TEXT:
		mwindow->edl->session->folderlist_format = FOLDERS_ICONS;
		break;
	case FOLDERS_ICONS:
		mwindow->edl->session->folderlist_format = FOLDERS_TEXT;
		break;
	}

	mwindow->awindow->gui->folder_list->update_format(
		mwindow->edl->session->folderlist_format, 1);
	menu->update_titles();

	return 1;
}


FolderListSort::FolderListSort(MWindow *mwindow, FolderListMenu *menu)
 : BC_MenuItem(_("Sort items"))
{
	this->mwindow = mwindow;
	this->menu = menu;
}

int FolderListSort::handle_event()
{
	mwindow->awindow->gui->sort_folders();
	return 1;
}

FolderListNew::FolderListNew(MWindow *mwindow, FolderListMenu *menu,
		const char *text, int is_clips)
 : BC_MenuItem(text)
{
	this->mwindow = mwindow;
	this->menu = menu;
	this->is_clips = is_clips;
}

int FolderListNew::handle_event()
{
	int cx, cy, cw = 320, ch = 120;
	menu->gui->get_abs_cursor(cx, cy);
	if( (cx-=cw/2) < 50 ) cx = 50;
	if( (cy-=ch/2) < 50 ) cy = 50;
	menu->gui->new_folder_thread->start(cx, cy, cw, ch, is_clips);
	return 1;
}

FolderListModify::FolderListModify(MWindow *mwindow, FolderListMenu *menu)
 : BC_MenuItem(_("Modify folder"))
{
	this->mwindow = mwindow;
	this->menu = menu;
}

int FolderListModify::handle_event()
{
	int awindow_folder = mwindow->edl->session->awindow_folder;
	BinFolder *folder = mwindow->edl->get_folder(awindow_folder);
	if( folder ) {
		int bw = mwindow->session->bwindow_w;
		int bh = mwindow->session->bwindow_h;
		int cx, cy;
		menu->gui->get_abs_cursor(cx, cy);
		if( (cx-=bw/2) < 50 ) cx = 50;
		if( (cy-=bh/2) < 50 ) cy = 50;
		menu->gui->modify_folder_thread->start(folder, cx, cy, bw, bh);
	}
	return 1;
}

FolderListDelete::FolderListDelete(MWindow *mwindow, FolderListMenu *menu)
 : BC_MenuItem(_("Delete folder"))
{
	this->mwindow = mwindow;
	this->menu = menu;
}

int FolderListDelete::handle_event()
{
	AssetPicon *picon = (AssetPicon *)menu->gui->folder_list->get_selection(0, 0);
	if( picon && picon->foldernum >= AWINDOW_USER_FOLDERS ) {
		int foldernum = picon->foldernum;
		BinFolder *folder = mwindow->edl->get_folder(foldernum);
		mwindow->delete_folder(folder->title);
		if( mwindow->edl->session->awindow_folder == foldernum )
			mwindow->edl->session->awindow_folder = AW_MEDIA_FOLDER;
		mwindow->awindow->gui->async_update_assets();
	}
	return 1;
}

