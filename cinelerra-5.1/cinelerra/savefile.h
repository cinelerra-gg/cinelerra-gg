
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

#ifndef SAVEFILE_H
#define SAVEFILE_H

#include "guicast.h"
#include "mainmenu.inc"
#include "mwindow.inc"
#include "savefile.inc"

class SaveBackup : public BC_MenuItem
{
public:
	SaveBackup(MWindow *mwindow);
	int handle_event();
	MWindow *mwindow;
};

class Save : public BC_MenuItem
{
public:
	Save(MWindow *mwindow);
	int handle_event();
	void create_objects(SaveAs *saveas);
	int save_before_quit();

	int quit_now;
	MWindow *mwindow;
	SaveAs *saveas;
};

class SaveAs : public BC_MenuItem, public Thread
{
public:
	SaveAs(MWindow *mwindow);
	int set_mainmenu(MainMenu *mmenu);
	int handle_event();
	void run();

	int quit_now;
	MWindow *mwindow;
	MainMenu *mmenu;
};

class SaveFileWindow : public BC_FileBox
{
public:
	SaveFileWindow(MWindow *mwindow, char *init_directory);
	~SaveFileWindow();
	MWindow *mwindow;
};


class SaveProjectModeItem : public BC_MenuItem
{
public:
	SaveProjectModeItem(const char *txt, int id)
	 : BC_MenuItem(txt) { this->id = id; }

	int handle_event();
	int id;
};

class SaveProjectMode : public BC_PopupMenu
{
	const char *save_modes[SAVE_PROJECT_MODES];
	int mode;
public:
	SaveProjectMode(SaveProjectWindow *gui, int x, int y);
	~SaveProjectMode();

	void create_objects();
	void update(int mode);

	SaveProjectWindow *gui;
};

class SaveProjectTextBox : public BC_TextBox
{
public:
	SaveProjectTextBox(SaveProjectWindow *gui, int x, int y, int w);
	~SaveProjectTextBox();
	int handle_event();


	SaveProjectWindow *gui;
};

class SaveProjectWindow : public BC_Window
{
public:
	SaveProjectWindow(MWindow *mwindow, const char *dir_path,
			int save_mode, int overwrite, int reload);
	~SaveProjectWindow();
	void create_objects();

	MWindow *mwindow;
	SaveProjectTextBox *textbox;
	BC_RecentList *recent_project;
	BrowseButton *browse_button;
	SaveProjectMode *mode_popup;

	char dir_path[BCTEXTLEN];
	int overwrite;
	int save_mode;
	int reload;

	int get_overwrite() { return overwrite; }
	int get_save_mode() { return save_mode; }
	int get_reload() { return reload; }
};

class SaveProject : public BC_MenuItem, public Thread
{
public:
	SaveProject(MWindow *mwindow);
	int handle_event();
	void run();
	MWindow *mwindow;
};

#endif
