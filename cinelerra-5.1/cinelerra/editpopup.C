
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
	add_item(new EditPopupFindAsset(mwindow, this));
	add_item(new EditPopupTitle(mwindow, this));
	add_item(new EditPopupShow(mwindow, this));
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
	mwindow->selected_to_clipboard(0);
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
	mwindow->selected_to_clipboard(1);
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

EditPopupFindAsset::EditPopupFindAsset(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Find in Resources"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int EditPopupFindAsset::handle_event()
{
	Edit *edit = popup->edit;
	if( edit ) {
		Indexable *idxbl = (Indexable *)edit->asset;
		if( !idxbl ) idxbl = (Indexable *)edit->nested_edl;
		if( idxbl ) {
			AWindowGUI *agui = mwindow->awindow->gui;
			agui->lock_window("EditPopupFindAsset::handle_event");
			AssetPicon *picon = 0;
			for( int i=0, n=agui->assets.size(); i<n; ++i ) {
				AssetPicon *ap = (AssetPicon *)agui->assets[i];
				int found = ap->indexable && ( idxbl == ap->indexable ||
					!strcmp(idxbl->path, ap->indexable->path) );
				if( found && !picon ) picon = ap;
				ap->set_selected(found);
			}
			if( picon ) {
				int selected_folder = picon->indexable->folder_no;
				mwindow->edl->session->awindow_folder = selected_folder;
				for( int i=0,n=agui->folders.size(); i<n; ++i ) {
					AssetPicon *folder_item = (AssetPicon *)agui->folders[i];
					int selected = folder_item->foldernum == selected_folder ? 1 : 0;
					folder_item->set_selected(selected);
				}
			}
			agui->unlock_window();
			agui->async_update_assets();
		}
	}
	return 1;
}


EditPopupTitle::EditPopupTitle(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("User title..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new EditTitleDialogThread(this);
}

EditPopupTitle::~EditPopupTitle()
{
	delete dialog_thread;
}

int EditPopupTitle::handle_event()
{
	if( popup->edit ) {
		dialog_thread->close_window();
		int wx = mwindow->gui->get_abs_cursor_x(0) - 400 / 2;
		int wy = mwindow->gui->get_abs_cursor_y(0) - 500 / 2;
		dialog_thread->start(wx, wy);
	}
	return 1;
}

void EditTitleDialogThread::start(int wx, int wy)
{
	this->wx = wx;  this->wy = wy;
	BC_DialogThread::start();
}

EditTitleDialogThread::EditTitleDialogThread(EditPopupTitle *edit_title)
{
	this->edit_title = edit_title;
	window = 0;
}
EditTitleDialogThread::~EditTitleDialogThread()
{
	close_window();
}

BC_Window* EditTitleDialogThread::new_gui()
{
	MWindow *mwindow = edit_title->mwindow;
	EditPopup *popup = edit_title->popup;
	window = new EditPopupTitleWindow(mwindow, popup, wx, wy);
	window->create_objects();
	return window;
}

void EditTitleDialogThread::handle_close_event(int result)
{
	window = 0;
}

void EditTitleDialogThread::handle_done_event(int result)
{
	if( result ) return;
	MWindow *mwindow = edit_title->mwindow;
	EditPopup *popup = edit_title->popup;
	strcpy(popup->edit->user_title, window->title_text->get_text());
	mwindow->gui->lock_window("EditTitleDialogThread::handle_done_event");
	mwindow->gui->draw_canvas(1, 0);
	mwindow->gui->flash_canvas(1);
	mwindow->gui->unlock_window();
}

EditPopupTitleWindow::EditPopupTitleWindow(MWindow *mwindow,
		EditPopup *popup, int wx, int wy)
 : BC_Window(_(PROGRAM_NAME ": Set edit title"), wx, wy,
	300, 130, 300, 130, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->popup = popup;
	strcpy(new_text, !popup->edit ? "" : popup->edit->user_title);
}

EditPopupTitleWindow::~EditPopupTitleWindow()
{
}

void EditPopupTitleWindow::create_objects()
{
	lock_window("EditPopupTitleWindow::create_objects");
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, _("User title:")));
	title_text = new EditPopupTitleText(this, mwindow, x+15, y+20, new_text);
	add_subwindow(title_text);
	add_tool(new BC_OKButton(this));
	add_tool(new BC_CancelButton(this));


	show_window();
	flush();
	unlock_window();
}


EditPopupTitleText::EditPopupTitleText(EditPopupTitleWindow *window,
	MWindow *mwindow, int x, int y, const char *text)
 : BC_TextBox(x, y, 250, 1, text)
{
	this->window = window;
	this->mwindow = mwindow;
}

EditPopupTitleText::~EditPopupTitleText()
{
}

int EditPopupTitleText::handle_event()
{
	if( get_keypress() == RETURN )
		window->set_done(0);
	return 1;
}


EditPopupShow::EditPopupShow(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Show edit"))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new EditShowDialogThread(this);
}

EditPopupShow::~EditPopupShow()
{
	delete dialog_thread;
}

int EditPopupShow::handle_event()
{
	if( popup->edit ) {
		dialog_thread->close_window();
		int wx = mwindow->gui->get_abs_cursor_x(0) - 400 / 2;
		int wy = mwindow->gui->get_abs_cursor_y(0) - 500 / 2;
		dialog_thread->start(wx, wy);
	}
	return 1;
}

void EditShowDialogThread::start(int wx, int wy)
{
	this->wx = wx;  this->wy = wy;
	BC_DialogThread::start();
}

EditShowDialogThread::EditShowDialogThread(EditPopupShow *edit_show)
{
	this->edit_show = edit_show;
	window = 0;
}
EditShowDialogThread::~EditShowDialogThread()
{
	close_window();
}

BC_Window* EditShowDialogThread::new_gui()
{
	MWindow *mwindow = edit_show->mwindow;
	EditPopup *popup = edit_show->popup;
	window = new EditPopupShowWindow(mwindow, popup, wx, wy);
	window->create_objects();
	return window;
}

void EditShowDialogThread::handle_close_event(int result)
{
	window = 0;
}

EditPopupShowWindow::EditPopupShowWindow(MWindow *mwindow,
		EditPopup *popup, int wx, int wy)
 : BC_Window(_(PROGRAM_NAME ": Show edit"), wx, wy,
	300, 220, 300, 220, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->popup = popup;
}

EditPopupShowWindow::~EditPopupShowWindow()
{
}

void EditPopupShowWindow::create_objects()
{
	lock_window("EditPopupShowWindow::create_objects");
	int x = 10, y = 10;
	BC_Title *title;
	char text[BCTEXTLEN];
	Edit *edit = popup->edit;
	Track *track = edit->track;
	sprintf(text, _("Track %d:"), mwindow->edl->tracks->number_of(track)+1);
	add_subwindow(title = new BC_Title(x, y, text));
	int x1 = x + title->get_w() + 10;
	int tw = get_w() - x1 - 20;
	truncate_text(text, track->title, tw);
	add_subwindow(new BC_Title(x1, y, text));
	y += title->get_h() + 5;
	sprintf(text, _("Edit %d:"), track->edits->number_of(edit)+1);
	add_subwindow(title = new BC_Title(x, y, text));
	char edit_title[BCTEXTLEN];
	edit->get_title(edit_title);
	truncate_text(text, edit_title, tw);
	add_subwindow(new BC_Title(x1, y, text));
	y += title->get_h() + 5;

	EDLSession *session = mwindow->edl->session;
	int time_format = session->time_format;
	int sample_rate = session->sample_rate;
	double frame_rate = session->frame_rate;
	double frames_per_foot = session->frames_per_foot;

	double startsource = track->from_units(edit->startsource);
	double startproject = track->from_units(edit->startproject);
	double length = track->from_units(edit->length);

	char text_startsource[BCSTRLEN];
	char text_startproject[BCSTRLEN];
	char text_length[BCSTRLEN];
	sprintf(text, _("StartSource: %s\nStartProject: %s\nLength: %s\n"),
		Units::totext(text_startsource, startsource,
			time_format, sample_rate, frame_rate, frames_per_foot),
		Units::totext(text_startproject, startproject,
			time_format, sample_rate, frame_rate, frames_per_foot),
		Units::totext(text_length, length,
			time_format, sample_rate, frame_rate, frames_per_foot));
	show_text = new EditPopupShowText(this, mwindow, x+15, y+10, text);
	add_subwindow(show_text);
	add_tool(new BC_OKButton(this));

	show_window();
	flush();
	unlock_window();
}


EditPopupShowText::EditPopupShowText(EditPopupShowWindow *window,
	MWindow *mwindow, int x, int y, const char *text)
 : BC_TextBox(x, y, 250, 4, text)
{
	this->window = window;
	this->mwindow = mwindow;
}

EditPopupShowText::~EditPopupShowText()
{
}

