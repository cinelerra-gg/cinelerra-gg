/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "editlength.h"
#include "edl.h"
#include "edlsession.h"
#include "guicast.h"
#include "language.h"
#include "menueditlength.h"
#include "mwindow.h"
#include "plugindialog.inc"

MenuEditLength::MenuEditLength(MWindow *mwindow)
 : BC_MenuItem(_("Edit Length..."))
{
	this->mwindow = mwindow;
	thread = new EditLengthThread(mwindow);
}

MenuEditLength::~MenuEditLength()
{
	delete thread;
}


int MenuEditLength::handle_event()
{
	thread->start(0);
	return 1;
}



MenuEditShuffle::MenuEditShuffle(MWindow *mwindow)
 : BC_MenuItem(_("Shuffle Edits"))
{
	this->mwindow = mwindow;
}



int MenuEditShuffle::handle_event()
{
	mwindow->shuffle_edits();
	return 1;
}


MenuEditReverse::MenuEditReverse(MWindow *mwindow)
 : BC_MenuItem(_("Reverse Edits"))
{
	this->mwindow = mwindow;
}



int MenuEditReverse::handle_event()
{
	mwindow->reverse_edits();
	return 1;
}





MenuEditAlign::MenuEditAlign(MWindow *mwindow)
 : BC_MenuItem(_("Align Edits"))
{
	this->mwindow = mwindow;
}



int MenuEditAlign::handle_event()
{
	mwindow->align_edits();
	return 1;
}


