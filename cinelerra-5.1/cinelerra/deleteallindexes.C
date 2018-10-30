
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

#include "deleteallindexes.h"
#include "filesystem.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "question.h"
#include "theme.h"
#include <string.h>

#include <libintl.h>
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

DeleteAllIndexes::DeleteAllIndexes(MWindow *mwindow, PreferencesWindow *pwindow,
	int x, int y, const char *text, const char *filter)
 : BC_GenericButton(x, y, text), Thread()
{
	this->mwindow = mwindow;
	this->pwindow = pwindow;
	this->filter = filter;
}

DeleteAllIndexes::~DeleteAllIndexes()
{
}

int DeleteAllIndexes::handle_event()
{
	start();
	return 1;
}

void DeleteAllIndexes::run()
{
	char string1[BCTEXTLEN], string2[BCTEXTLEN];
	strcpy(string1, pwindow->thread->preferences->index_directory);
	FileSystem dir;
	dir.update(pwindow->thread->preferences->index_directory);
	dir.complete_path(string1);

	for( int i=0; i<dir.dir_list.total; ++i ) {
		const char *fn = dir.dir_list.values[i]->name;
		if( FileSystem::test_filter(fn, filter) ) continue;
		sprintf(string2, "%s%s", string1, dir.dir_list.values[i]->name);
		remove(string2);
printf("DeleteAllIndexes::run %s\n", string2);
	}

	pwindow->thread->redraw_indexes = 1;
}


ConfirmDeleteAllIndexes::ConfirmDeleteAllIndexes(MWindow *mwindow, char *string)
 : BC_Window(_(PROGRAM_NAME ": Delete All Indexes"),
 		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1),
		340,
		140)
{
	this->string = string;
}

ConfirmDeleteAllIndexes::~ConfirmDeleteAllIndexes()
{
}

void ConfirmDeleteAllIndexes::create_objects()
{
	lock_window("ConfirmDeleteAllIndexes::create_objects");
	int x = 10, y = 10;
	add_subwindow(new BC_Title(x, y, string));

	y += 20;
	add_subwindow(new BC_OKButton(x, y));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(x, y));
	unlock_window();
}


