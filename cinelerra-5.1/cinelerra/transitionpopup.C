
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "plugin.h"
#include "transition.h"
#include "track.h"
#include "tracks.h"
#include "transitionpopup.h"


TransitionLengthThread::TransitionLengthThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

TransitionLengthThread::~TransitionLengthThread()
{
	close_window();
}

void TransitionLengthThread::start(Transition *transition, double length)
{
	this->transition = transition;
	this->orig_length = length;
	this->new_length = length;
	BC_DialogThread::start();
}

BC_Window* TransitionLengthThread::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() - 150;
	int y = display_info.get_abs_cursor_y() - 50;
	TransitionLengthDialog *gui = new TransitionLengthDialog(mwindow, this, x, y);
	gui->create_objects();
	return gui;
}

void TransitionLengthThread::handle_close_event(int result)
{
	if( !result ) {
		if( transition )
			mwindow->set_transition_length(transition, new_length);
		else
			mwindow->set_transition_length(new_length);
	}
	else
		update(orig_length);
}

int TransitionLengthThread::update(double length)
{
	if( !EQUIV(this->new_length, length) ) {
		this->new_length = length;
		if( transition ) {
			transition->length = transition->track->to_units(length, 1);
			mwindow->sync_parameters(CHANGE_EDL);
			mwindow->gui->lock_window();
			mwindow->gui->draw_overlays(1);
			mwindow->gui->unlock_window();
		}
	}
	return 1;
}


TransitionUnitsItem::TransitionUnitsItem(TransitionUnitsPopup *popup,
		const char *text, int id)
 : BC_MenuItem(text)
{
        this->popup = popup;
	this->id = id;
}

TransitionUnitsItem::~TransitionUnitsItem()
{
}

int TransitionUnitsItem::handle_event()
{
	TransitionUnitsPopup *units_popup = (TransitionUnitsPopup *)get_popup_menu();
	TransitionLengthDialog *gui = units_popup->gui;
	TransitionLengthText *length_text = gui->text;
	EDLSession *session = gui->mwindow->edl->session;
	double length = gui->thread->new_length;
	char text[BCSTRLEN];
	units_popup->units = id;
	Units::totext(text, length, units_popup->units, session->sample_rate,
			session->frame_rate, session->frames_per_foot);
	length_text->update(text);
	units_popup->set_text(get_text());
	return 1;
}

TransitionUnitsPopup::TransitionUnitsPopup(TransitionLengthDialog *gui, int x, int y)
 : BC_PopupMenu(x, y, 120, "", 1)
{
	this->gui = gui;
	units = TIME_SECONDS;
}

TransitionUnitsPopup::~TransitionUnitsPopup()
{
}

void TransitionUnitsPopup::create_objects()
{
	TransitionUnitsItem *units_item;
	add_item(units_item = new TransitionUnitsItem(this, _("Seconds"), TIME_SECONDS));
	add_item(new TransitionUnitsItem(this, _("Frames"), TIME_FRAMES));
	add_item(new TransitionUnitsItem(this, _("Samples"), TIME_SAMPLES));
	add_item(new TransitionUnitsItem(this, _("H:M:S.xxx"), TIME_HMS));
	add_item(new TransitionUnitsItem(this, _("H:M:S:frm"), TIME_HMSF));
	set_text(units_item->get_text());
}


TransitionLengthDialog::TransitionLengthDialog(MWindow *mwindow,
	TransitionLengthThread *thread, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Transition length"), x, y,
		300, 100, -1, -1, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

TransitionLengthDialog::~TransitionLengthDialog()
{
}


void TransitionLengthDialog::create_objects()
{
	lock_window("TransitionLengthDialog::create_objects");
	add_subwindow(units_popup = new TransitionUnitsPopup(this, 10, 10));
	units_popup->create_objects();
	text = new TransitionLengthText(mwindow, this, 160, 10);
	text->create_objects();
	text->set_precision(3);
	text->set_increment(0.1);
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int TransitionLengthDialog::close_event()
{
	set_done(0);
	return 1;
}


TransitionLengthText::TransitionLengthText(MWindow *mwindow,
	TransitionLengthDialog *gui, int x, int y)
 : BC_TumbleTextBox(gui, (float)gui->thread->new_length,
		0.f, 100.f, x, y, 100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TransitionLengthText::handle_event()
{
	const char *text = get_text();
	int units = gui->units_popup->units;
	EDLSession *session = gui->mwindow->edl->session;
	double result = Units::text_to_seconds(text, session->sample_rate,
		units, session->frame_rate, session->frames_per_foot);
	return gui->thread->update(result);
}

int TransitionLengthText::handle_up_down(int dir)
{
	double delta = 0;
	int units = gui->units_popup->units;
	EDLSession *session = gui->mwindow->edl->session;
	switch( units ) {
	case TIME_SECONDS:
	case TIME_HMS:
		delta = 0.1;
		break;
	case TIME_HMSF:
	case TIME_FRAMES:
		delta = session->frame_rate > 0 ? 1./session->frame_rate : 0;
		break;
	case TIME_SAMPLES:
		delta = session->sample_rate > 0 ? 1./session->sample_rate : 0;
		break;
	}
	double length = gui->thread->new_length + delta * dir;
	char text[BCSTRLEN];
	Units::totext(text, length, units,
		session->sample_rate, session->frame_rate,
		session->frames_per_foot);
	update(text);
	return gui->thread->update(length);
}

int TransitionLengthText::handle_up_event()
{
	return handle_up_down(+1);
}
int TransitionLengthText::handle_down_event()
{
	return handle_up_down(-1);
}


TransitionPopup::TransitionPopup(MWindow *mwindow, MWindowGUI *gui)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

TransitionPopup::~TransitionPopup()
{
	delete length_thread;
//	delete dialog_thread;
}


void TransitionPopup::create_objects()
{
	length_thread = new TransitionLengthThread(mwindow);
//	add_item(attach = new TransitionPopupAttach(mwindow, this));
	add_item(show = new TransitionPopupShow(mwindow, this));
	add_item(on = new TransitionPopupOn(mwindow, this));
	add_item(length_item = new TransitionPopupLength(mwindow, this));
	add_item(detach = new TransitionPopupDetach(mwindow, this));
}

int TransitionPopup::update(Transition *transition)
{
	this->transition = transition;
	show->set_checked(transition->show);
	on->set_checked(transition->on);
	char len_text[50];
	sprintf(len_text, _("Length: %2.2f sec"), transition->track->from_units(transition->length));
	length_item->set_text(len_text);
	return 0;
}


TransitionPopupAttach::TransitionPopupAttach(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Attach..."))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupAttach::~TransitionPopupAttach()
{
}

int TransitionPopupAttach::handle_event()
{
//	popup->dialog_thread->start();
	return 1;
}


TransitionPopupDetach::TransitionPopupDetach(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Detach"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupDetach::~TransitionPopupDetach()
{
}

int TransitionPopupDetach::handle_event()
{
	mwindow->detach_transition(popup->transition);
	return 1;
}


TransitionPopupOn::TransitionPopupOn(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("On"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupOn::~TransitionPopupOn()
{
}

int TransitionPopupOn::handle_event()
{
	popup->transition->on = !get_checked();
	mwindow->sync_parameters(CHANGE_EDL);
	return 1;
}


TransitionPopupShow::TransitionPopupShow(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Show"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupShow::~TransitionPopupShow()
{
}

int TransitionPopupShow::handle_event()
{
	mwindow->show_plugin(popup->transition);
	return 1;
}


TransitionPopupLength::TransitionPopupLength(MWindow *mwindow, TransitionPopup *popup)
 : BC_MenuItem(_("Length"))
{
	this->mwindow = mwindow;
	this->popup = popup;
}

TransitionPopupLength::~TransitionPopupLength()
{
}

int TransitionPopupLength::handle_event()
{
	Transition *transition = popup->transition;
	double length = transition->edit->track->from_units(transition->length);
	popup->length_thread->start(popup->transition, length);
	return 1;
}

