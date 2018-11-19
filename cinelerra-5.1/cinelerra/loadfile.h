
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

#ifndef LOADFILE_H
#define LOADFILE_H

#include "bcdialog.h"
#include "guicast.h"
#include "loadmode.inc"
#include "mainmenu.inc"
#include "mwindow.inc"

class Load;
class LoadFileThread;
class LoadFileWindow;
class LocateFileWindow;
class LoadPrevious;
class LoadBackup;
class LoadFileApply;

class Load : public BC_MenuItem
{
public:
	Load(MWindow *mwindow, MainMenu *mainmenu);
	~Load();

	void create_objects();
	int handle_event();

	MWindow *mwindow;
	MainMenu *mainmenu;
	LoadFileThread *thread;
};

class LoadFileThread : public BC_DialogThread
{
public:
	LoadFileThread(MWindow *mwindow, Load *menuitem);
	~LoadFileThread();


	BC_Window* new_gui();
	void handle_done_event(int result);
	void load_apply();

	MWindow *mwindow;
	Load *load;
	int load_mode;
	LoadFileWindow *window;
};

class LoadFileWindow : public BC_FileBox
{
public:
	LoadFileWindow(MWindow *mwindow,
		LoadFileThread *thread,
		char *init_directory);
	~LoadFileWindow();

	void create_objects();
	int resize_event(int w, int h);

	LoadFileThread *thread;
	LoadMode *loadmode;
	LoadFileApply *load_file_apply;
	MWindow *mwindow;
};

class LocateFileWindow : public BC_FileBox
{
public:
	LocateFileWindow(MWindow *mwindow, char *init_directory, char *old_filename);
	~LocateFileWindow();
	MWindow *mwindow;
};

class LoadPrevious : public BC_MenuItem, public Thread
{
public:
	LoadPrevious(MWindow *mwindow, Load *loadfile);
	int handle_event();
	void run();

	int set_path(char *path);

	MWindow *mwindow;
	Load *loadfile;
	char path[1024];
};

class LoadBackup : public BC_MenuItem
{
public:
	LoadBackup(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class LoadFileApply : public BC_GenericButton
{
public:
	LoadFileApply(LoadFileWindow *load_file_window);
	int handle_event();
	LoadFileWindow *load_file_window;
};

#endif
