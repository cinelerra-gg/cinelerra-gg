
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
#include "trackpopup.h"
#include "trackcanvas.h"

#include <string.h>

TrackPopup::TrackPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	track = 0;
	edit = 0;
	pluginset = 0;
	plugin = 0;
	position = 0;
}

TrackPopup::~TrackPopup()
{
}

void TrackPopup::create_objects()
{
	add_item(new TrackAttachEffect(mwindow, this));
	add_item(new TrackMoveUp(mwindow, this));
	add_item(new TrackMoveDown(mwindow, this));
	add_item(new TrackPopupDeleteTrack(mwindow, this));
	add_item(new TrackPopupAddTrack(mwindow, this));
	add_item(new TrackPopupFindAsset(mwindow, this));
	add_item(new TrackPopupShow(mwindow, this));
	add_item(new TrackPopupUserTitle(mwindow, this));
	add_item(new TrackPopupTitleColor(mwindow, this));
	resize_option = 0;
	matchsize_option = 0;
}

int TrackPopup::activate_menu(Track *track, Edit *edit,
		PluginSet *pluginset, Plugin *plugin, double position)
{
	this->track = track;
	this->edit = edit;
	this->pluginset = pluginset;
	this->plugin = plugin;
	this->position = position;

	if( track->data_type == TRACK_VIDEO && !resize_option ) {
		add_item(resize_option = new TrackPopupResize(mwindow, this));
		add_item(matchsize_option = new TrackPopupMatchSize(mwindow, this));
	}
	else if( track->data_type == TRACK_AUDIO && resize_option ) {
		del_item(resize_option);     resize_option = 0;
		del_item(matchsize_option);  matchsize_option = 0;
	}

	return BC_PopupMenu::activate_menu();
}


TrackAttachEffect::TrackAttachEffect(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Attach effect..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new PluginDialogThread(mwindow);
}

TrackAttachEffect::~TrackAttachEffect()
{
	delete dialog_thread;
}

int TrackAttachEffect::handle_event()
{
	dialog_thread->start_window(popup->track,
		0, _(PROGRAM_NAME ": Attach Effect"),
		0, popup->track->data_type);
	return 1;
}


TrackMoveUp::TrackMoveUp(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Move up"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
TrackMoveUp::~TrackMoveUp()
{
}
int TrackMoveUp::handle_event()
{
	mwindow->move_track_up(popup->track);
	return 1;
}



TrackMoveDown::TrackMoveDown(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Move down"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
TrackMoveDown::~TrackMoveDown()
{
}
int TrackMoveDown::handle_event()
{
	mwindow->move_track_down(popup->track);
	return 1;
}


TrackPopupResize::TrackPopupResize(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Resize track..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new ResizeTrackThread(mwindow);
}
TrackPopupResize::~TrackPopupResize()
{
	delete dialog_thread;
}

int TrackPopupResize::handle_event()
{
	dialog_thread->start_window(popup->track);
	return 1;
}


TrackPopupMatchSize::TrackPopupMatchSize(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Match output size"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
TrackPopupMatchSize::~TrackPopupMatchSize()
{
}

int TrackPopupMatchSize::handle_event()
{
	mwindow->match_output_size(popup->track);
	return 1;
}


TrackPopupDeleteTrack::TrackPopupDeleteTrack(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Delete track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}
int TrackPopupDeleteTrack::handle_event()
{
	mwindow->delete_track(popup->track);
	return 1;
}


TrackPopupAddTrack::TrackPopupAddTrack(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Add track"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int TrackPopupAddTrack::handle_event()
{
	switch( popup->track->data_type ) {
	case TRACK_AUDIO:
		mwindow->add_audio_track_entry(1, popup->track);
		break;
	case TRACK_VIDEO:
		mwindow->add_video_track_entry(popup->track);
		break;
	case TRACK_SUBTITLE:
		mwindow->add_subttl_track_entry(popup->track);
		break;
	}
	return 1;
}

TrackPopupFindAsset::TrackPopupFindAsset(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Find in Resources"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

int TrackPopupFindAsset::handle_event()
{
	Edit *edit = popup->edit;
	if( edit ) {
		Indexable *idxbl = (Indexable *)edit->asset;
		if( !idxbl ) idxbl = (Indexable *)edit->nested_edl;
		if( idxbl ) {
			AWindowGUI *agui = mwindow->awindow->gui;
			agui->lock_window("TrackPopupFindAsset::handle_event");
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


TrackPopupUserTitle::TrackPopupUserTitle(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("User title..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new TrackUserTitleDialogThread(this);
}

TrackPopupUserTitle::~TrackPopupUserTitle()
{
	delete dialog_thread;
}

int TrackPopupUserTitle::handle_event()
{
	if( popup->edit ) {
		dialog_thread->close_window();
		int wx = mwindow->gui->get_abs_cursor_x(0) - 400 / 2;
		int wy = mwindow->gui->get_abs_cursor_y(0) - 500 / 2;
		dialog_thread->start(wx, wy);
	}
	return 1;
}

void TrackUserTitleDialogThread::start(int wx, int wy)
{
	this->wx = wx;  this->wy = wy;
	BC_DialogThread::start();
}

TrackUserTitleDialogThread::TrackUserTitleDialogThread(TrackPopupUserTitle *edit_title)
{
	this->edit_title = edit_title;
	window = 0;
}
TrackUserTitleDialogThread::~TrackUserTitleDialogThread()
{
	close_window();
}

BC_Window* TrackUserTitleDialogThread::new_gui()
{
	MWindow *mwindow = edit_title->mwindow;
	TrackPopup *popup = edit_title->popup;
	window = new TrackPopupUserTitleWindow(mwindow, popup, wx, wy);
	window->create_objects();
	return window;
}

void TrackUserTitleDialogThread::handle_close_event(int result)
{
	window = 0;
}

void TrackUserTitleDialogThread::handle_done_event(int result)
{
	if( result ) return;
	MWindow *mwindow = edit_title->mwindow;
	TrackPopup *popup = edit_title->popup;
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
	mwindow->gui->lock_window("TrackUserTitleDialogThread::handle_done_event");
	mwindow->gui->draw_canvas(1, 0);
	mwindow->gui->flash_canvas(1);
	mwindow->gui->unlock_window();
}

TrackPopupUserTitleWindow::TrackPopupUserTitleWindow(MWindow *mwindow,
		TrackPopup *popup, int wx, int wy)
 : BC_Window(_(PROGRAM_NAME ": Set edit title"), wx, wy,
	300, 130, 300, 130, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->popup = popup;
	strcpy(new_text, !popup->edit ? "" : popup->edit->user_title);
}

TrackPopupUserTitleWindow::~TrackPopupUserTitleWindow()
{
}

void TrackPopupUserTitleWindow::create_objects()
{
	lock_window("TrackPopupUserTitleWindow::create_objects");
	int x = 10, y = 10, x1;
	BC_Title *title = new BC_Title(x1=x, y, _("User title:"));
	add_subwindow(title);  x1 += title->get_w() + 10;
	title_text = new TrackPopupUserTitleText(this, mwindow, x1, y, new_text);
	add_subwindow(title_text);

	add_tool(new BC_OKButton(this));
	add_tool(new BC_CancelButton(this));


	show_window();
	flush();
	unlock_window();
}


TrackPopupUserTitleText::TrackPopupUserTitleText(TrackPopupUserTitleWindow *window,
	MWindow *mwindow, int x, int y, const char *text)
 : BC_TextBox(x, y, window->get_w()-x-15, 1, text)
{
	this->window = window;
	this->mwindow = mwindow;
}

TrackPopupUserTitleText::~TrackPopupUserTitleText()
{
}

int TrackPopupUserTitleText::handle_event()
{
	if( get_keypress() == RETURN )
		window->set_done(0);
	return 1;
}


TrackPopupTitleColor::TrackPopupTitleColor(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Bar Color..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
	color_picker = 0;
}
TrackPopupTitleColor::~TrackPopupTitleColor()
{
	delete color_picker;
}

int TrackPopupTitleColor::handle_event()
{
	if( popup->edit ) {
		int color = popup->mwindow->get_title_color(popup->edit);
		if( !color ) color = popup->mwindow->theme->get_color_title_bg();
		delete color_picker;
		color_picker = new TrackTitleColorPicker(popup, color);
		int alpha = (~color>>24) & 0xff;
		color_picker->start_window(color & 0xffffff, alpha, 1);
	}
	return 1;
}

TrackTitleColorDefault::TrackTitleColorDefault(
	TrackTitleColorPicker *color_picker, int x, int y)
 : BC_GenericButton(x, y, _("default"))
{
	this->color_picker = color_picker;
}

int TrackTitleColorDefault::handle_event()
{
	const unsigned color = 0, alpha = 0xff;
	color_picker->color = color | (~alpha << 24);
	color_picker->update_gui(color, alpha);
	return 1;
}

TrackTitleColorPicker::TrackTitleColorPicker(TrackPopup *popup, int color)
 : ColorPicker(1, _("Bar Color"))
{
	this->popup = popup;
	this->color = color;
}
TrackTitleColorPicker::~TrackTitleColorPicker()
{
}
void TrackTitleColorPicker::create_objects(ColorWindow *gui)
{
	int y = gui->get_h() - BC_CancelButton::calculate_h() + 10;
	int x = gui->get_w() - BC_CancelButton::calculate_w() - 10;
	x -= BC_GenericButton::calculate_w(gui, _("default")) + 15;
	gui->add_subwindow(new TrackTitleColorDefault(this, x, y));
}

int TrackTitleColorPicker::handle_new_color(int color, int alpha)
{
	this->color = color | (~alpha << 24);
	return 1;
}

void TrackTitleColorPicker::handle_done_event(int result)
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


TrackPopupShow::TrackPopupShow(MWindow *mwindow, TrackPopup *popup)
 : BC_MenuItem(_("Show edit"))
{
	this->mwindow = mwindow;
	this->popup = popup;
	dialog_thread = new TrackShowDialogThread(this);
}

TrackPopupShow::~TrackPopupShow()
{
	delete dialog_thread;
}

int TrackPopupShow::handle_event()
{
	if( popup->edit ) {
		dialog_thread->close_window();
		int wx = mwindow->gui->get_abs_cursor_x(0) - 400 / 2;
		int wy = mwindow->gui->get_abs_cursor_y(0) - 500 / 2;
		dialog_thread->start(wx, wy);
	}
	return 1;
}

void TrackShowDialogThread::start(int wx, int wy)
{
	this->wx = wx;  this->wy = wy;
	BC_DialogThread::start();
}

TrackShowDialogThread::TrackShowDialogThread(TrackPopupShow *edit_show)
{
	this->edit_show = edit_show;
	window = 0;
}
TrackShowDialogThread::~TrackShowDialogThread()
{
	close_window();
}

BC_Window* TrackShowDialogThread::new_gui()
{
	MWindow *mwindow = edit_show->mwindow;
	TrackPopup *popup = edit_show->popup;
	window = new TrackPopupShowWindow(mwindow, popup, wx, wy);
	window->create_objects();
	return window;
}

void TrackShowDialogThread::handle_close_event(int result)
{
	window = 0;
}

TrackPopupShowWindow::TrackPopupShowWindow(MWindow *mwindow,
		TrackPopup *popup, int wx, int wy)
 : BC_Window(_(PROGRAM_NAME ": Show edit"), wx, wy,
	300, 220, 300, 220, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TrackPopupShowWindow::~TrackPopupShowWindow()
{
}

void TrackPopupShowWindow::create_objects()
{
	lock_window("TrackPopupShowWindow::create_objects");
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
	show_text = new TrackPopupShowText(this, mwindow, x+15, y+10, text);
	add_subwindow(show_text);
	add_tool(new BC_OKButton(this));

	show_window();
	flush();
	unlock_window();
}


TrackPopupShowText::TrackPopupShowText(TrackPopupShowWindow *window,
	MWindow *mwindow, int x, int y, const char *text)
 : BC_TextBox(x, y, 250, 4, text)
{
	this->window = window;
	this->mwindow = mwindow;
}

TrackPopupShowText::~TrackPopupShowText()
{
}

