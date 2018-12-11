
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
#include "trackpopup.h"
#include "trackcanvas.h"

#include <string.h>

TrackPopup::TrackPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
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
	resize_option = 0;
	matchsize_option = 0;
}

int TrackPopup::update(Track *track)
{
	this->track = track;

	if(track->data_type == TRACK_VIDEO && !resize_option)
	{
		add_item(resize_option = new TrackPopupResize(mwindow, this));
		add_item(matchsize_option = new TrackPopupMatchSize(mwindow, this));
	}
	else
	if(track->data_type == TRACK_AUDIO && resize_option)
	{
		del_item(resize_option);     resize_option = 0;
		del_item(matchsize_option);  matchsize_option = 0;
	}
	return 0;
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

