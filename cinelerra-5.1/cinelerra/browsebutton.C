
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

#include "bcsignals.h"
#include "browsebutton.h"
#include "language.h"
#include "mutex.h"
#include "theme.h"


BrowseButton::BrowseButton(Theme *theme,
	BC_WindowBase *parent_window,
	BC_TextBox *textbox,
	int x,
	int y,
	const char *init_directory,
	const char *title,
	const char *caption,
	int want_directory)
 : BC_Button(x, y, theme->get_image_set("magnify_button")),
   Thread(1, 0, 0)
{
	this->parent_window = parent_window;
	this->want_directory = want_directory;
	this->title = title;
	this->caption = caption;
	this->init_directory = init_directory;
	this->textbox = textbox;
	this->theme = theme;
	set_tooltip(_("Look for file"));
	gui = 0;
	startup_lock = new Mutex("BrowseButton::startup_lock");
}

BrowseButton::~BrowseButton()
{
	startup_lock->lock("BrowseButton::~BrowseButton");
	if(gui)
	{
		gui->lock_window();
		gui->set_done(1);
		gui->unlock_window();
	}
	startup_lock->unlock();
	Thread::join();
	delete startup_lock;
}

int BrowseButton::handle_event()
{
	if(Thread::running())
	{
		if(gui)
		{
			gui->lock_window("BrowseButton::handle_event");
			gui->raise_window();
			gui->unlock_window();
		}
		return 1;
	}

	x = parent_window->get_abs_cursor_x(0);
	y = parent_window->get_abs_cursor_y(0);
	startup_lock->lock("BrowseButton::handle_event 1");
	Thread::start();

	startup_lock->lock("BrowseButton::handle_event 2");
	startup_lock->unlock();
	return 1;
}

void BrowseButton::run()
{
	BrowseButtonWindow browsewindow(theme,
		get_x() - BC_WindowBase::get_resources()->filebox_w / 2,
 		get_y() - BC_WindowBase::get_resources()->filebox_h / 2,
		parent_window,
		textbox->get_text(),
		title,
		caption,
		want_directory);
	gui = &browsewindow;
	startup_lock->unlock();

	browsewindow.lock_window("BrowseButton::run");
	browsewindow.create_objects();
	browsewindow.unlock_window();
	int result2 = browsewindow.run_window();

	if(!result2)
	{
// 		if(want_directory)
// 		{
// 			textbox->update(browsewindow.get_directory());
// 		}
// 		else
// 		{
// 			textbox->update(browsewindow.get_filename());
// 		}

		parent_window->lock_window("BrowseButton::run");
		textbox->update(browsewindow.get_submitted_path());
		parent_window->flush();
		textbox->handle_event();
		parent_window->unlock_window();
	}

	startup_lock->lock("BrowseButton::run");
	gui = 0;
	startup_lock->unlock();
}

BrowseButtonWindow::BrowseButtonWindow(Theme *theme, int x, int y,
	BC_WindowBase *parent_window, const char *init_directory,
	const char *title, const char *caption, int want_directory)
 : BC_FileBox(x, y, init_directory, title, caption,
	want_directory, // Set to 1 to get hidden files.
	want_directory, // Want only directories
	0, theme->browse_pad)
{
}

BrowseButtonWindow::~BrowseButtonWindow()
{
}
