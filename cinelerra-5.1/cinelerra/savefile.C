
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

#include "confirmsave.h"
#include "bchash.h"
#include "bcrecentlist.h"
#include "edl.h"
#include "file.h"
#include "filexml.h"
#include "fileformat.h"
#include "indexfile.h"
#include "language.h"
#include "mainerror.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playback3d.h"
#include "savefile.h"
#include "mainsession.h"

#include <string.h>









SaveBackup::SaveBackup(MWindow *mwindow)
 : BC_MenuItem(_("Save backup"), "b", 'b')
{
	this->mwindow = mwindow;
}
int SaveBackup::handle_event()
{
	mwindow->save_backup();
	mwindow->gui->show_message(_("Saved backup."));
	return 1;
}











Save::Save(MWindow *mwindow) : BC_MenuItem(_("Save"), "s", 's')
{
	this->mwindow = mwindow;
	quit_now = 0;
}

void Save::create_objects(SaveAs *saveas)
{
	this->saveas = saveas;
}

int Save::handle_event()
{
	if(mwindow->session->filename[0] == 0)
	{
		saveas->start();
	}
	else
	{
// save it
// TODO: Move this into mwindow.
		FileXML file;
		mwindow->edl->save_xml(&file, mwindow->session->filename);
		file.terminate_string();

		if(file.write_to_file(mwindow->session->filename))
		{
			char string2[256];
			sprintf(string2, _("Couldn't open %s"), mwindow->session->filename);
			ErrorBox error(_(PROGRAM_NAME ": Error"),
				mwindow->gui->get_abs_cursor_x(1),
				mwindow->gui->get_abs_cursor_y(1));
			error.create_objects(string2);
			error.raise_window();
			error.run_window();
			return 1;
		}
		else
		{
			char string[BCTEXTLEN];
			sprintf(string, _("\"%s\" %dC written"),
				 mwindow->session->filename, (int)strlen(file.string()));
			mwindow->gui->show_message(string);
		}
		mwindow->session->changes_made = 0;
// Last command in program
		if( saveas->quit_now )
			mwindow->quit();
	}
	return 1;
}

int Save::save_before_quit()
{
	mwindow->gui->lock_window("Save::save_before_quit");
	saveas->quit_now = 1;
	handle_event();
	mwindow->gui->unlock_window();
	return 0;
}

SaveAs::SaveAs(MWindow *mwindow)
 : BC_MenuItem(_("Save as..."), "Shift-S", 'S'), Thread()
{
	set_shift(1);
	this->mwindow = mwindow;
	quit_now = 0;
}

int SaveAs::set_mainmenu(MainMenu *mmenu)
{
	this->mmenu = mmenu;
	return 0;
}

int SaveAs::handle_event()
{
	quit_now = 0;
	start();
	return 1;
}

void SaveAs::run()
{
// ======================================= get path from user
	int result;
//printf("SaveAs::run 1\n");
	char directory[1024], filename[1024];
	sprintf(directory, "~");
	mwindow->defaults->get("DIRECTORY", directory);

// Loop if file exists
	do{
		SaveFileWindow *window;

		window = new SaveFileWindow(mwindow, directory);
		window->lock_window("SaveAs::run");
		window->create_objects();
		window->unlock_window();
		result = window->run_window();
		mwindow->defaults->update("DIRECTORY", window->get_submitted_path());
		strcpy(filename, window->get_submitted_path());
		delete window;

// Extend the filename with .xml
		if(strlen(filename) < 4 ||
			strcasecmp(&filename[strlen(filename) - 4], ".xml"))
		{
			strcat(filename, ".xml");
		}

// ======================================= try to save it
		if(filename[0] == 0) return;              // no filename given
		if(result == 1) return;          // user cancelled
		result = ConfirmSave::test_file(mwindow, filename);
	}while(result);        // file exists so repeat

//printf("SaveAs::run 6 %s\n", filename);




// save it
	FileXML file;
	mwindow->gui->lock_window("SaveAs::run 1");
// update the project name
	mwindow->set_filename(filename);
	mwindow->edl->save_xml(&file, filename);
	mwindow->gui->unlock_window();
	file.terminate_string();

	if(file.write_to_file(filename))
	{
		char string2[256];
		mwindow->set_filename("");      // update the project name
		sprintf(string2, _("Couldn't open %s."), filename);
		ErrorBox error(_(PROGRAM_NAME ": Error"),
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
		error.create_objects(string2);
		error.raise_window();
		error.run_window();
		return;
	}
	else
	{
		char string[BCTEXTLEN];
		sprintf(string, _("\"%s\" %dC written"), filename, (int)strlen(file.string()));
		mwindow->gui->lock_window("SaveAs::run 2");
		mwindow->gui->show_message(string);
		mwindow->gui->unlock_window();
	}


	mwindow->session->changes_made = 0;
	mmenu->add_load(filename);
// Last command in program
	if( quit_now )
		mwindow->quit();
	return;
}


SaveFileWindow::SaveFileWindow(MWindow *mwindow, char *init_directory)
 : BC_FileBox(mwindow->gui->get_abs_cursor_x(1),
	mwindow->gui->get_abs_cursor_y(1) - BC_WindowBase::get_resources()->filebox_h / 2,
	init_directory,
	_(PROGRAM_NAME ": Save"),
	_("Enter a filename to save as"))
{
	this->mwindow = mwindow;
}

SaveFileWindow::~SaveFileWindow() {}



int SaveProjectModeItem::handle_event()
{
	((SaveProjectMode *)get_popup_menu())->update(id);
	return 1;
}

SaveProjectMode::SaveProjectMode(SaveProjectWindow *gui, int x, int y)
 : BC_PopupMenu(x, y, 100, "")
{
	this->gui = gui;
	save_modes[SAVE_PROJECT_COPY]     = _("Copy");
	save_modes[SAVE_PROJECT_SYMLINK]  = _("SymLink");
	save_modes[SAVE_PROJECT_RELLINK]  = _("RelLink");
}

SaveProjectMode::~SaveProjectMode()
{
}

void SaveProjectMode::create_objects()
{
	add_item(new SaveProjectModeItem(save_modes[SAVE_PROJECT_COPY],    SAVE_PROJECT_COPY));
	add_item(new SaveProjectModeItem(save_modes[SAVE_PROJECT_SYMLINK], SAVE_PROJECT_SYMLINK));
	add_item(new SaveProjectModeItem(save_modes[SAVE_PROJECT_RELLINK], SAVE_PROJECT_RELLINK));
	set_text(save_modes[gui->save_mode]);
}

void SaveProjectMode::update(int mode)
{
	if( gui->save_mode == mode ) return;
	set_text(save_modes[gui->save_mode = mode]);
}

SaveProjectTextBox::SaveProjectTextBox(SaveProjectWindow *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, gui->dir_path)
{
	this->gui = gui;
}
SaveProjectTextBox::~SaveProjectTextBox()
{
}

int SaveProjectTextBox::handle_event()
{
	return 1;
}

SaveProjectWindow::SaveProjectWindow(MWindow *mwindow, const char *dir_path,
			int save_mode, int overwrite, int reload)
 : BC_Window(_(PROGRAM_NAME ": Export Project"),
		mwindow->gui->get_abs_cursor_x(1),
		mwindow->gui->get_abs_cursor_y(1) -
			BC_WindowBase::get_resources()->filebox_h / 2,
		540, 220, 540, 220, 0)
{
	this->mwindow = mwindow;
	strcpy(this->dir_path, dir_path);
	this->overwrite = overwrite;
	this->save_mode = save_mode;
	this->reload = reload;
}
SaveProjectWindow::~SaveProjectWindow()
{
}

void SaveProjectWindow::create_objects()
{
	int x = 20, y = 20, x1 = get_w()-80;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Project Directory:")));
	y += title->get_h() + 10;
	add_subwindow(textbox = new SaveProjectTextBox(this, x, y, x1-x));
	x1 += 10;
	add_subwindow(recent_project = new BC_RecentList("RECENT_PROJECT",
		mwindow->defaults, textbox, 10, x1, y, 300, 100));
	recent_project->load_items("RECENT_PROJECT");
	x1 += recent_project->get_w() + 10;
	add_subwindow(browse_button = new BrowseButton(mwindow->theme, this,
		textbox, x1, y-5, "", "", "", 1));
	y += textbox->get_h() + 20;
	add_subwindow(mode_popup = new SaveProjectMode(this, x, y));
	mode_popup->create_objects();
	y += mode_popup->get_h() + 10;
	x1 = x;
	BC_CheckBox *overwrite_files, *reload_project;
	add_subwindow(overwrite_files = new BC_CheckBox(x1, y, &overwrite, _("Overwrite files")));
	x1 += overwrite_files->get_w() + 20;
	add_subwindow(reload_project = new BC_CheckBox(x1, y, &reload, _("Reload project")));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window(1);
}

SaveProject::SaveProject(MWindow *mwindow)
 : BC_MenuItem(_("Export Project..."), "Alt-s", 's'), Thread()
{
	set_alt(1);
	this->mwindow = mwindow;
}

int SaveProject::handle_event()
{
	start();
	return 1;
}

void SaveProject::run()
{
	char dir_path[1024];  sprintf(dir_path, "~");
	mwindow->defaults->get("PROJECT_DIRECTORY", dir_path);
	int reload = mwindow->defaults->get("PROJECT_RELOAD", 0);
	int overwrite = mwindow->defaults->get("PROJECT_OVERWRITE", 0);
	int save_mode = mwindow->defaults->get("PROJECT_SAVE_MODE", 0);

	SaveProjectWindow window(mwindow, dir_path, save_mode, overwrite, reload);
	window.lock_window("SaveProject::run");
	window.create_objects();
	window.unlock_window();
	int result = window.run_window();
	if( result ) return;

	strcpy(dir_path, window.textbox->get_text());
	window.recent_project->add_item("RECENT_PROJECT", dir_path);
	reload = window.get_reload();
	overwrite = window.get_overwrite();
	save_mode = window.get_save_mode();
	mwindow->defaults->update("PROJECT_DIRECTORY", dir_path);
	mwindow->defaults->update("PROJECT_RELOAD", reload);
	mwindow->defaults->update("PROJECT_OVERWRITE", overwrite);
	mwindow->defaults->update("PROJECT_SAVE_MODE", save_mode);
	mwindow->save_project(dir_path, save_mode, overwrite, reload);
}

