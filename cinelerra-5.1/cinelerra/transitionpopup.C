
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

void TransitionLengthThread::start(Transition *transition,
	double length)
{
	this->transition = transition;
	this->length = this->orig_length = length;
	BC_DialogThread::start();
}

BC_Window* TransitionLengthThread::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() - 150;
	int y = display_info.get_abs_cursor_y() - 50;
	TransitionLengthDialog *gui = new TransitionLengthDialog(mwindow,
		this,
		x,
		y);
	gui->create_objects();
	return gui;
}

void TransitionLengthThread::handle_close_event(int result)
{
	if(!result)
	{
		if(transition)
		{
			mwindow->set_transition_length(transition, length);
		}
		else
		{
			mwindow->set_transition_length(length);
		}
	}
}









TransitionLengthDialog::TransitionLengthDialog(MWindow *mwindow,
	TransitionLengthThread *thread,
	int x,
	int y)
 : BC_Window(_(PROGRAM_NAME ": Transition length"),
	x,
	y,
	300,
	100,
	-1,
	-1,
	0,
	0,
	1)
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
	add_subwindow(new BC_Title(10, 10, _("Seconds:")));
	text = new TransitionLengthText(mwindow, this, 100, 10);
	text->create_objects();
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
	TransitionLengthDialog *gui,
	int x,
	int y)
 : BC_TumbleTextBox(gui,
 	(float)gui->thread->length,
	(float)0,
	(float)100,
	x,
	y,
	100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int TransitionLengthText::handle_event()
{
	double result = atof(get_text());
	if(!EQUIV(result, gui->thread->length))
	{
		gui->thread->length = result;
		mwindow->gui->lock_window();
		mwindow->gui->update(0, 1, 0, 0, 0, 0, 0);
		mwindow->gui->unlock_window();
	}

	return 1;
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
	this->length = transition->edit->track->from_units(transition->length);
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
	popup->length_thread->start(popup->transition,
		popup->length);
	return 1;
}

