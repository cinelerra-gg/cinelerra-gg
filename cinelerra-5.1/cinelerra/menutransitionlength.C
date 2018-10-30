
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

#include "edl.h"
#include "edlsession.h"
#include "guicast.h"
#include "language.h"
#include "menutransitionlength.h"
#include "mwindow.h"
#include "plugindialog.inc"
#include "transitionpopup.h"

MenuTransitionLength::MenuTransitionLength(MWindow *mwindow)
 : BC_MenuItem(_("Transition Length..."))
{
	this->mwindow = mwindow;
	thread = new TransitionLengthThread(mwindow);
}

MenuTransitionLength::~MenuTransitionLength()
{
	delete thread;
}


int MenuTransitionLength::handle_event()
{
	thread->start(0, mwindow->edl->session->default_transition_length);
	return 1;
}


