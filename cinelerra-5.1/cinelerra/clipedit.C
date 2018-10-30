
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
#include "bcsignals.h"
#include "clipedit.h"
#include "edl.h"
#include "fonts.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"
#include "errorbox.h"
#include "tracks.h"



ClipEdit::ClipEdit(MWindow *mwindow, AWindow *awindow, VWindow *vwindow)
 : BC_DialogThread()
{
	this->mwindow = mwindow;
	this->awindow = awindow;
	this->vwindow = vwindow;
	this->clip = 0;
	this->create_it = 0;
}

ClipEdit::~ClipEdit()
{
	close_window();
}

// After the window is closed and deleted, this is called.
void ClipEdit::handle_close_event(int result)
{
	if( !result ) {
		int name_ok = 1;
		for( int i=0; name_ok && i<mwindow->edl->clips.size(); ++i ) {
			if( !strcasecmp(clip->local_session->clip_title,
			      mwindow->edl->clips[i]->local_session->clip_title) &&
			    (create_it || strcasecmp(clip->local_session->clip_title,
			       original->local_session->clip_title)) )
				name_ok = 0;
		}
		if( !name_ok ) {
			eprintf(_("A clip with that name already exists."));
			result = 1;
		}
	}

	if( !result ) {
// Add to EDL
		if( create_it )
			mwindow->edl->add_clip(clip);
		else // Copy clip to existing clip in EDL
			original->copy_session(clip);
//		mwindow->vwindow->gui->update_sources(mwindow->vwindow->gui->source->get_text());
		mwindow->awindow->gui->async_update_assets();

// Change VWindow to it if vwindow was called
// But this doesn't let you easily create a lot of clips.
		if( vwindow && create_it ) {
//				vwindow->change_source(new_edl);
		}
		mwindow->session->update_clip_number();
	}

// always a copy from new_gui
	clip->remove_user();
	original = 0;
	clip = 0;
	create_it = 0;
}


// User creates the window and initializes it here.
BC_Window* ClipEdit::new_gui()
{
	original = clip;
	this->clip = new EDL(mwindow->edl);
	clip->create_objects();
	clip->copy_all(original);

	window = new ClipEditWindow(mwindow, this);
	window->create_objects();
	return window;
}



void ClipEdit::edit_clip(EDL *clip, int x, int y)
{
	close_window();

	this->clip = clip;
	this->create_it = 0;
	this->x = x;  this->y = y;
	start();
}

void ClipEdit::create_clip(EDL *clip, int x, int y)
{
	close_window();

	this->clip = clip;
	this->create_it = 1;
	this->x = x;  this->y = y;
	start();
}


ClipEditWindow::ClipEditWindow(MWindow *mwindow, ClipEdit *thread)
 : BC_Window(_(PROGRAM_NAME ": Clip Info"),
	thread->x -400/2, thread->y - 350/2,
	400, 350, 400, 430, 0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ClipEditWindow::~ClipEditWindow()
{
}


void ClipEditWindow::create_objects()
{
	lock_window("ClipEditWindow::create_objects");
	this->create_it = thread->create_it;

	int x = 10, y = 10;
	int x1 = x;
	BC_TextBox *textbox;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("Title:")));
	y += title->get_h() + 5;
	add_subwindow(titlebox = new ClipEditTitle(this, x1, y, get_w() - x1 * 2));

	int end = strlen(titlebox->get_text());
	titlebox->set_selection(0, end, end);

	y += titlebox->get_h() + 10;
	add_subwindow(title = new BC_Title(x1, y, _("Comments:")));
	y += title->get_h() + 5;
	add_subwindow(textbox = new ClipEditComments(this,
		x1,
		y,
		get_w() - x1 * 2,
		BC_TextBox::pixels_to_rows(this,
			MEDIUMFONT,
			get_h() - 10 - BC_OKButton::calculate_h() - y)));



	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	titlebox->activate();
	unlock_window();
}





ClipEditTitle::ClipEditTitle(ClipEditWindow *window, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, window->thread->clip->local_session->clip_title)
{
	this->window = window;
}

int ClipEditTitle::handle_event()
{
	strcpy(window->thread->clip->local_session->clip_title, get_text());
	return 1;
}





ClipEditComments::ClipEditComments(ClipEditWindow *window, int x, int y, int w, int rows)
 : BC_TextBox(x, y, w, rows, window->thread->clip->local_session->clip_notes)
{
	this->window = window;
}

int ClipEditComments::handle_event()
{
	strcpy(window->thread->clip->local_session->clip_notes, get_text());
	return 1;
}
