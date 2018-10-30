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



// Various functions for macro editing


#ifndef MENUEDITLENGTH_H
#define MENUEDITLENGTH_H


#include "editlength.inc"
#include "guicast.h"
#include "mwindow.inc"
#include "plugindialog.inc"
#include "transitionpopup.inc"

class MenuEditLength : public BC_MenuItem
{
public:
	MenuEditLength(MWindow *mwindow);
	~MenuEditLength();

	int handle_event();

	MWindow *mwindow;
	EditLengthThread *thread;
};

class MenuEditShuffle : public BC_MenuItem
{
public:
	MenuEditShuffle(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};


class MenuEditReverse : public BC_MenuItem
{
public:
	MenuEditReverse(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};


class MenuEditAlign : public BC_MenuItem
{
public:
	MenuEditAlign(MWindow *mwindow);

	int handle_event();

	MWindow *mwindow;
};


#endif


