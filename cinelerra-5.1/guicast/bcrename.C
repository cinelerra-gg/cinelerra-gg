
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

#include "condition.h"
#include "bcfilebox.h"
#include "bcrename.h"
#include "bctitle.h"
#include "filesystem.h"
#include "language.h"
#include "mutex.h"
#include <string.h>

#include <sys/stat.h>







BC_Rename::BC_Rename(BC_RenameThread *thread, int x, int y, BC_FileBox *filebox)
 : BC_Window(filebox->get_rename_title(),
 	x,
	y,
	320,
	120,
	0,
	0,
	0,
	0,
	1)
{
	this->thread = thread;
}

BC_Rename::~BC_Rename()
{
}


void BC_Rename::create_objects()
{
	int x = 10, y = 10;
	lock_window("BC_Rename::create_objects");
	add_tool(new BC_Title(x, y, _("Enter a new name for the file:")));
	y += 20;
	add_subwindow(textbox = new BC_TextBox(x, y, 300, 1, thread->orig_name));
	y += 30;
	add_subwindow(new BC_OKButton(this));
	x = get_w() - 100;
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

const char* BC_Rename::get_text()
{
	return textbox->get_text();
}


BC_RenameThread::BC_RenameThread(BC_FileBox *filebox)
 : Thread(0, 0, 0)
{
	this->filebox = filebox;
	window = 0;
	change_lock = new Mutex("BC_RenameThread::change_lock");
	completion_lock = new Condition(1, "BC_RenameThread::completion_lock");
}

BC_RenameThread::~BC_RenameThread()
{
 	interrupt();
	delete change_lock;
	delete completion_lock;
}

void BC_RenameThread::run()
{
	int x = filebox->get_abs_cursor_x(1);
	int y = filebox->get_abs_cursor_y(1);
	change_lock->lock("BC_RenameThread::run 1");
	window = new BC_Rename(this,
		x,
		y,
		filebox);
	window->create_objects();
	change_lock->unlock();


	int result = window->run_window();

	if(!result)
	{
		char old_name[BCTEXTLEN];
		char new_name[BCTEXTLEN];
		filebox->fs->join_names(old_name, orig_path, orig_name);
		filebox->fs->join_names(new_name, orig_path, window->get_text());

//printf("BC_RenameThread::run %d %s -> %s\n", __LINE__, old_name, new_name);
		rename(old_name, new_name);


		filebox->lock_window("BC_RenameThread::run");
		filebox->refresh();
		filebox->unlock_window();
	}

	change_lock->lock("BC_RenameThread::run 2");
	delete window;
	window = 0;
	change_lock->unlock();

	completion_lock->unlock();
}

int BC_RenameThread::interrupt()
{
	change_lock->lock("BC_RenameThread::interrupt");
	if(window)
	{
		window->lock_window("BC_RenameThread::interrupt");
		window->set_done(1);
		window->unlock_window();
	}

	change_lock->unlock();

	completion_lock->lock("BC_RenameThread::interrupt");
	completion_lock->unlock();
	return 0;
}

int BC_RenameThread::start_rename()
{
	change_lock->lock();

	if(window)
	{
		window->lock_window("BC_RenameThread::start_rename");
		window->raise_window();
		window->unlock_window();
		change_lock->unlock();
	}
	else
	{
		char string[BCTEXTLEN];
		FileSystem fs;
		strcpy(string, filebox->get_current_path());

		if(fs.is_dir(string))
		{
// Extract last directory from path to rename it
			strcpy(orig_path, string);
			char *ptr = orig_path + strlen(orig_path) - 1;
// Scan for end of path
			while(*ptr == '/' && ptr > orig_path) ptr--;
// Scan for previous separator
			while(*ptr != '/' && ptr > orig_path) ptr--;
// Get final path segment
			if(*ptr == '/') ptr++;
			strcpy(orig_name, ptr);
// Terminate original path
			*ptr = 0;
		}
		else
		{
// Extract filename from path to rename it
			fs.extract_dir(orig_path, string);
			fs.extract_name(orig_name, string);
		}


//printf("BC_RenameThread::start_rename %d %s %s %s\n", __LINE__, string, orig_path, orig_name);


		change_lock->unlock();
		completion_lock->lock("BC_RenameThread::start_rename");

		Thread::start();
	}


	return 0;
}


