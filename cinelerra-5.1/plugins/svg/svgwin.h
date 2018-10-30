
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

#ifndef SVGWIN_H
#define SVGWIN_H

#include "guicast.h"
#include "filexml.h"
#include "mutex.h"
#include "pluginclient.h"
#include "svg.h"
#include "thread.h"

class SvgThread;
class SvgWin;

class SvgCoord;
class NewSvgButton;
class NewSvgWindow;
class EditSvgButton;
class DpiValue;
class DpiButton;

class SvgWin : public PluginClientWindow
{
public:
	SvgWin(SvgMain *client);
	~SvgWin();

	void create_objects();
	int close_event();
	void update_gui(SvgConfig &config);
	int hide_window(int flush = 1);

	SvgCoord *out_x, *out_y, *out_w, *out_h;
	DpiValue *dpi;
	DpiButton *dpi_button;
	SvgMain *client;
	BC_Title *svg_file_title;
	BC_Title *svg_file_mstime;
	NewSvgButton *new_svg_button;
	NewSvgWindow *new_svg_thread;
	EditSvgButton *edit_svg_button;
	Mutex editing_lock;
	int editing;

};

class SvgCoord : public BC_TumbleTextBox
{
public:
	SvgCoord(SvgWin *win, SvgMain *client,
		int x, int y, float *value);
	~SvgCoord();
	int handle_event();

	SvgMain *client;
	SvgWin *win;
	float *value;

};

class NewSvgButton : public BC_GenericButton, public Thread
{
public:
	NewSvgButton(SvgMain *client, SvgWin *window, int x, int y);
	~NewSvgButton();
	int handle_event();
	void run();
	void stop();

	SvgMain *client;
	SvgWin *window;
	NewSvgWindow *new_window;
};

class EditSvgButton : public BC_GenericButton, public Thread
{
public:
	EditSvgButton(SvgMain *client, SvgWin *window, int x, int y);
	~EditSvgButton();
	int handle_event();
	void stop();
	void run();

	SvgMain *client;
	SvgWin *window;
	int fh_fifo;
};

class SvgInkscapeThread : public Thread
{
public:
	SvgInkscapeThread(EditSvgButton *edit);
	~SvgInkscapeThread();
	void run();
	EditSvgButton *edit;
};

class NewSvgWindow : public BC_FileBox
{
public:
	NewSvgWindow(SvgMain *client, SvgWin *window, char *init_directory);
	~NewSvgWindow();
	SvgMain *client;
	SvgWin *window;
};

class DpiValue : public BC_TumbleTextBox
{
public:
	DpiValue(SvgWin *win, SvgMain *client,
		int x, int y, float *value);
	~DpiValue();
	int handle_event();

	SvgMain *client;
	SvgWin *win;
	float *value;

};

class DpiButton : public BC_GenericButton
{
public:
	DpiButton( SvgWin *window, SvgMain *client, int x, int y);
	~DpiButton();
	int handle_event();

	SvgMain *client;
	SvgWin *window;
};


#endif
