
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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
#include "editlength.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "track.h"
#include "tracks.h"


EditLengthThread::EditLengthThread(MWindow *mwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
}

EditLengthThread::~EditLengthThread()
{
	close_window();
}

void EditLengthThread::start(Edit *edit)
{
	this->edit = edit;

	orig_length = 0;
	length = 0;

	double start = mwindow->edl->local_session->get_selectionstart();
	double end = mwindow->edl->local_session->get_selectionend();

// get the default length from the first edit selected
	if(!edit)
	{
		Track *track = 0;
		int got_it = 0;
		for(track = mwindow->edl->tracks->first;
			track && !got_it;
			track = track->next)
		{
			if(track->record)
			{
				int64_t start_units = track->to_units(start, 0);
				int64_t end_units = track->to_units(end, 0);

				for(edit = track->edits->first;
					edit && !got_it;
					edit = edit->next)
				{
					if(edit->startproject >= start_units &&
						edit->startproject < end_units)
					{
						this->length =
							this->orig_length =
							track->from_units(edit->length);
						got_it = 1;
					}
				}
			}
		}
	}

	BC_DialogThread::start();
}

BC_Window* EditLengthThread::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x() - 150;
	int y = display_info.get_abs_cursor_y() - 50;
	EditLengthDialog *gui = new EditLengthDialog(mwindow,
		this,
		x,
		y);
	gui->create_objects();
	return gui;
}

void EditLengthThread::handle_close_event(int result)
{
	if(!result)
	{
		if(edit)
		{
//			mwindow->set_edit_length(edit, length);
		}
		else
		{
			mwindow->set_edit_length(length);
		}
	}
}









EditLengthDialog::EditLengthDialog(MWindow *mwindow,
	EditLengthThread *thread,
	int x,
	int y)
 : BC_Window(_(PROGRAM_NAME ": Edit length"),
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

EditLengthDialog::~EditLengthDialog()
{
}


void EditLengthDialog::create_objects()
{
	lock_window("EditLengthDialog::create_objects");
	add_subwindow(new BC_Title(10, 10, _("Seconds:")));
	text = new EditLengthText(mwindow, this, 100, 10);
	text->create_objects();
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int EditLengthDialog::close_event()
{
	set_done(0);
	return 1;
}






EditLengthText::EditLengthText(MWindow *mwindow,
	EditLengthDialog *gui,
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

int EditLengthText::handle_event()
{
	double result = atof(get_text());
	if(fabs(result - gui->thread->length) > 0.000001)
	{
		gui->thread->length = result;
	}

	return 1;
}










