
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

#ifndef BC_RENAME_H
#define BC_RENAME_H


#include "bcfilebox.inc"
#include "bcwindow.h"
#include "thread.h"


class BC_Rename : public BC_Window
{
public:
	BC_Rename(BC_RenameThread *thread, int x, int y, BC_FileBox *filebox);
	~BC_Rename();

	void create_objects();
	const char* get_text();

private:
	BC_TextBox *textbox;
	BC_RenameThread *thread;
};

class BC_RenameThread : public Thread
{
public:
	BC_RenameThread(BC_FileBox *filebox);
	~BC_RenameThread();

	void run();
	int interrupt();
	int start_rename();

	char orig_path[BCTEXTLEN];
	char orig_name[BCTEXTLEN];

private:
	Mutex *change_lock;
	Condition *completion_lock;
	BC_FileBox *filebox;
	BC_Rename *window;
};





#endif
