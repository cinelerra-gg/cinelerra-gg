
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
#include "theme.h"
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
	add_item(new EditPopupShow(mwindow, this));
	add_item(new EditPopupUserTitle(mwindow, this));
	add_item(new EditPopupTitleColor(mwindow, this));
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


EditPopupUserTitle::EditPopupUserTitle(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("User title..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new EditUserTitleDialogThread(this);
}

EditPopupUserTitle::~EditPopupUserTitle()
{
	delete dialog_thread;
}

int EditPopupUserTitle::handle_event()
{
	if( popup->edit ) {
		dialog_thread->close_window();
		int wx = mwindow->gui->get_abs_cursor_x(0) - 400 / 2;
		int wy = mwindow->gui->get_abs_cursor_y(0) - 500 / 2;
		dialog_thread->start(wx, wy);
	}
	return 1;
}

void EditUserTitleDialogThread::start(int wx, int wy)
{
	this->wx = wx;  this->wy = wy;
	BC_DialogThread::start();
}

EditUserTitleDialogThread::EditUserTitleDialogThread(EditPopupUserTitle *edit_title)
{
	this->edit_title = edit_title;
	window = 0;
}
EditUserTitleDialogThread::~EditUserTitleDialogThread()
{
	close_window();
}

BC_Window* EditUserTitleDialogThread::new_gui()
{
	MWindow *mwindow = edit_title->mwindow;
	EditPopup *popup = edit_title->popup;
	window = new EditPopupUserTitleWindow(mwindow, popup, wx, wy);
	window->create_objects();
	return window;
}

void EditUserTitleDialogThread::handle_close_event(int result)
{
	window = 0;
}

void EditUserTitleDialogThread::handle_done_event(int result)
{
	if( result ) return;
	MWindow *mwindow = edit_title->mwindow;
	EditPopup *popup = edit_title->popup;
	EDL *edl = mwindow->edl;
	const char *text = window->title_text->get_text();
	int count = 0;
	for( Track *track=edl->tracks->first; track; track=track->next ) {
		if( !track->record ) continue;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			if( !edit->is_selected ) continue;
			strcpy(edit->user_title, text);
			++count;
		}
	}
	if( count )
		edl->tracks->clear_selected_edits();
	else if( popup->edit ) {
		strcpy(popup->edit->user_title, text);
	}
	mwindow->gui->lock_window("EditUserTitleDialogThread::handle_done_event");
	mwindow->gui->draw_canvas(1, 0);
	mwindow->gui->flash_canvas(1);
	mwindow->gui->unlock_window();
}

EditPopupUserTitleWindow::EditPopupUserTitleWindow(MWindow *mwindow,
		EditPopup *popup, int wx, int wy)
 : BC_Window(_(PROGRAM_NAME ": Set edit title"), wx, wy,
	300, 130, 300, 130, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->popup = popup;
	strcpy(new_text, !popup->edit ? "" : popup->edit->user_title);
}

EditPopupUserTitleWindow::~EditPopupUserTitleWindow()
{
}

void EditPopupUserTitleWindow::create_objects()
{
	lock_window("EditPopupUserTitleWindow::create_objects");
	int x = 10, y = 10, x1;
	BC_Title *title = new BC_Title(x1=x, y, _("User title:"));
	add_subwindow(title);  x1 += title->get_w() + 10;
	title_text = new EditPopupUserTitleText(this, mwindow, x1, y, new_text);
	add_subwindow(title_text);

	add_tool(new BC_OKButton(this));
	add_tool(new BC_CancelButton(this));


	show_window();
	flush();
	unlock_window();
}


EditPopupUserTitleText::EditPopupUserTitleText(EditPopupUserTitleWindow *window,
	MWindow *mwindow, int x, int y, const char *text)
 : BC_TextBox(x, y, window->get_w()-x-15, 1, text)
{
	this->window = window;
	this->mwindow = mwindow;
}

EditPopupUserTitleText::~EditPopupUserTitleText()
{
}

int EditPopupUserTitleText::handle_event()
{
	if( get_keypress() == RETURN )
		window->set_done(0);
	return 1;
}


EditPopupTitleColor::EditPopupTitleColor(MWindow *mwindow, EditPopup *popup)
 : BC_MenuItem(_("Bar Color..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	color_picker = 0;
}
EditPopupTitleColor::~EditPopupTitleColor()
{
	delete color_picker;
}

int EditPopupTitleColor::handle_event()
{
	if( popup->edit ) {
		int color = popup->mwindow->get_title_color(popup->edit);
		if( !color ) color = popup->mwindow->theme->get_color_title_bg();
		delete color_picker;
		color_picker = new EditTitleColorPicker(popup, color);
		int alpha = (~color>>24) & 0xff;
		color_picker->start_window(color & 0xffffff, alpha, 1);
	}
	return 1;
}

EditTitleColorDefault::EditTitleColorDefault(
	EditTitleColorPicker *color_picker, int x, int y)
 : BC_GenericButton(x, y, _("default"))
{
	this->color_picker = color_picker;
}

int EditTitleColorDefault::handle_event()
{
	const int color = 0, alpha = 0xff;
	color_picker->color = color | (~alpha << 24);
	color_picker->update_gui(color, alpha);
	return 1;
}

EditTitleColorPicker::EditTitleColorPicker(EditPopup *popup, int color)
 : ColorPicker(1, _("Bar Color"))
{
	this->popup = popup;
	this->color = color;
}
EditTitleColorPicker::~EditTitleColorPicker()
{
}
void EditTitleColorPicker::create_objects(ColorWindow *gui)
{
	int y = gui->get_h() - BC_CancelButton::calculate_h() + 10;
	int x = gui->get_w() - BC_CancelButton::calculate_w() - 10;
	x -= BC_GenericButton::calculate_w(gui, _("default")) + 15;
	gui->add_subwindow(new EditTitleColorDefault(this, x, y));
}

int EditTitleColorPicker::handle_new_color(int color, int alpha)
{
	this->color = color | (~alpha << 24);
	return 1;
}

void EditTitleColorPicker::handle_done_event(int result)
{
	if( !result ) {
		EDL *edl = popup->mwindow->edl;
		int count = 0;
		for( Track *track=edl->tracks->first; track; track=track->next ) {
			if( !track->record ) continue;
			for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
				if( !edit->is_selected ) continue;
				edit->color = color;
				++count;
			}
		}
		if( count )
			edl->tracks->clear_selected_edits();
		else
			popup->edit->color = color;
	}
	MWindowGUI *mwindow_gui = popup->mwindow->gui;
	mwindow_gui->lock_window("GWindowColorUpdate::run");
	mwindow_gui->draw_trackmovement();
	mwindow_gui->unlock_window();
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

