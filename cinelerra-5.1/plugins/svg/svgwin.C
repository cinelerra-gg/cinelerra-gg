
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

#include "svgwin.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>

#include "empty_svg.h"

struct fifo_struct {
	int pid;
// 1 = update from client, 2 = client closes, 3 = quit
	int action;
};

SvgWin::SvgWin(SvgMain *client)
 : PluginClientWindow(client, 420, 210, 420, 210, 1)
{
	this->client = client;
	this->editing = 0;
}

SvgWin::~SvgWin()
{
}

void SvgWin::create_objects()
{
	BC_Title *title;
	int x0 = 10, y = 10;

	add_tool(title = new BC_Title(x0, y, _("Out X:")));
	int x1 = x0 + title->get_w() + 10;
	out_x = new SvgCoord(this, client, x1, y, &client->config.out_x);
	out_x->create_objects();
	int x2 = x1 + out_x->get_w() + 15;
	add_tool(title = new BC_Title(x2, y, _("Out W:")));
	int x3 = x2 + title->get_w() + 10;
	out_w = new SvgCoord(this, client, x3, y, &client->config.out_w);
	out_w->create_objects();
	y += out_x->get_h() + 5;

	add_tool(new BC_Title(x0, y, _("Out Y:")));
	out_y = new SvgCoord(this, client, x1, y, &client->config.out_y);
	out_y->create_objects();
	add_tool(title = new BC_Title(x2, y, _("Out H:")));
	out_h = new SvgCoord(this, client, x3, y, &client->config.out_h);
	out_h->create_objects();
	y += out_y->get_h() + 20;

	add_tool(title = new BC_Title(x0, y, _("DPI:")));
        dpi = new DpiValue(this, client, x1, y, &client->config.dpi);
        dpi->create_objects();
	add_tool(dpi_button = new DpiButton(this, client, x2, y));
	dpi_button->create_objects();
        y += dpi->get_h() + 20;

	add_tool(svg_file_title = new BC_Title(x0, y, client->config.svg_file));
	y += svg_file_title->get_h() + 5;
	struct stat st;
	int64_t ms_time = stat(client->config.svg_file, &st) ? 0 :
		st.st_mtim.tv_sec*1000 + st.st_mtim.tv_nsec/1000000;
	char mtime[BCSTRLEN];  mtime[0] = 0;
	if( ms_time > 0 ) {
		time_t tm = ms_time/1000;
		ctime_r(&tm ,mtime);
	}
	add_tool(svg_file_mstime = new BC_Title(x0, y, mtime));
	y += svg_file_mstime->get_h() + 15;

	y = get_h() - NewSvgButton::calculate_h() - 5;
	add_tool(new_svg_button = new NewSvgButton(client, this, x0, y));
	y = get_h() - EditSvgButton::calculate_h() - 5;
	add_tool(edit_svg_button = new EditSvgButton(client, this, x0+300, y));

	show_window();
	flush();
}

int SvgWin::close_event()
{
	new_svg_button->stop();
	edit_svg_button->stop();
	set_done(1);
	return 1;
}

int SvgWin::hide_window(int flush)
{
	new_svg_button->stop();
	edit_svg_button->stop();
	return BC_WindowBase::hide_window(flush);
}


void SvgWin::update_gui(SvgConfig &config)
{
	lock_window("SvgWin::update_gui");
	out_x->update(config.out_x);
	out_y->update(config.out_y);
	out_w->update(config.out_w);
	out_h->update(config.out_h);
	dpi->update(config.dpi);
	svg_file_title->update(config.svg_file);
	char mtime[BCSTRLEN];  mtime[0] = 0;
	if( config.ms_time > 0 ) {
		time_t tm = config.ms_time/1000;
		ctime_r(&tm ,mtime);
	}
	svg_file_mstime->update(mtime);
	unlock_window();
}

SvgCoord::SvgCoord(SvgWin *win, SvgMain *client, int x, int y, float *value)
 : BC_TumbleTextBox(win, *value, (float)0, (float)3000, x, y, 100)
{
//printf("SvgWidth::SvgWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

SvgCoord::~SvgCoord()
{
}

int SvgCoord::handle_event()
{
	*value = atof(get_text());
	client->send_configure_change();
	return 1;
}

NewSvgButton::NewSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("New/Open SVG..."))
{
	this->client = client;
	this->window = window;
	new_window = 0;
}
NewSvgButton::~NewSvgButton()
{
	stop();
}


int NewSvgButton::handle_event()
{
	window->editing_lock.lock();
	if( !window->editing ) {
		window->editing = 1;
		window->editing_lock.unlock();
		start();
	}
	else {
		flicker();
		window->editing_lock.unlock();
	}

	return 1;
}

void NewSvgButton::run()
{
// ======================================= get path from user
	int result;
//printf("NewSvgButton::run 1\n");
	result = 1;
	char filename[1024];
	strncpy(filename, client->config.svg_file, sizeof(filename));
// Loop until file is chosen
	do {
		char directory[1024];
		strncpy(directory, client->config.svg_file, sizeof(directory));
		char *cp = strrchr(directory, '/');
		if( cp ) *cp = 0;
		if( !directory[0] ) {
			char *cp = getenv("HOME");
			if( cp ) strncpy(directory, cp, sizeof(directory));
		}
		new_window = new NewSvgWindow(client, window, directory);
		new_window->create_objects();
		new_window->update_filter("*.svg");
		result = new_window->run_window();
		const char *filepath = new_window->get_path(0);
		strcpy(filename, filepath ? filepath : "" );
		delete new_window;  new_window = 0;
		window->editing_lock.lock();
		if( result || !filename[0] )
			window->editing = 0;
		window->editing_lock.unlock();
		if( !window->editing ) return;              // cancel or no filename given

// Extend the filename with .svg
		if( strlen(filename) < 4 ||
			strcasecmp(&filename[strlen(filename) - 4], ".svg") ) {
			strcat(filename, ".svg");
		}

		if( !access(filename, R_OK) )
			result = 0;
		else {
			FILE *out = fopen(filename,"w");
			if( out ) {
				unsigned long size = sizeof(empty_svg) - 4;
				fwrite(empty_svg+4, size,  1, out);
				fclose(out);
				result = 0;
			}
		}
	} while(result);        // file doesn't exist so repeat

	strcpy(client->config.svg_file, filename);
	client->config.ms_time = 0;
	window->update_gui(client->config);
	client->send_configure_change();

	window->editing_lock.lock();
	window->editing = 0;
	window->editing_lock.unlock();

	return;
}

void NewSvgButton::stop()
{
	if( new_window ) {
		new_window->set_done(1);
	}
	join();
}

EditSvgButton::EditSvgButton(SvgMain *client, SvgWin *window, int x, int y)
 : BC_GenericButton(x, y, _("Edit")), Thread(1)
{
	this->client = client;
	this->window = window;
	fh_fifo = -1;
}

EditSvgButton::~EditSvgButton()
{
	stop();
}

void EditSvgButton::stop()
{
	if( running() ) {
		if( fh_fifo >= 0 ) {
			struct fifo_struct fifo_buf;
			fifo_buf.pid = getpid();
			fifo_buf.action = 3;
			write(fh_fifo, &fifo_buf, sizeof(fifo_buf));
		}
	}
	join();
}

int EditSvgButton::handle_event()
{

	window->editing_lock.lock();
	if( !window->editing && client->config.svg_file[0] != 0 ) {
		window->editing = 1;
		window->editing_lock.unlock();
		start();
	}
	else {
		flicker();
		window->editing_lock.unlock();
	}
	return 1;
}

void EditSvgButton::run()
{
// ======================================= get path from user
	char filename_png[1024];
	char filename_fifo[1024];
	strcpy(filename_png, client->config.svg_file);
	strcat(filename_png, ".png");
	remove(filename_png);
	strcpy(filename_fifo, filename_png);
	strcat(filename_fifo, ".fifo");
	remove(filename_fifo);
	if( !mkfifo(filename_fifo, S_IRWXU) &&
	    (fh_fifo = ::open(filename_fifo, O_RDWR+O_NONBLOCK)) >= 0 ) {
		SvgInkscapeThread inkscape_thread(this);
		inkscape_thread.start();
		int done = 0;
		while( inkscape_thread.running() && !done ) {
			struct stat st;
			int64_t ms_time = stat(client->config.svg_file, &st) ? 0 :
				st.st_mtim.tv_sec*1000 + st.st_mtim.tv_nsec/1000000;
			if( client->config.ms_time != ms_time ) {
				client->config.ms_time = ms_time;
				client->send_configure_change();
			}
			// select(fh_fifo+1,rds,0,ers,tmo) does not work here
			Timer::delay(200);
			struct fifo_struct fifo_buf; fifo_buf.action = 1;
			int ret = read(fh_fifo, &fifo_buf, sizeof(fifo_buf));
			if( ret < 0 ) {
				if( errno == EAGAIN ) continue;
				perror("fifo");
				break;
			}
			if( ret != sizeof(fifo_buf) ) continue;
			switch( fifo_buf.action ) {
			case 1: break;
			case 2: printf(_("Inkscape has exited\n"));
				break;
			case 3: printf(_("Plugin window has closed\n"));
				done = 1;
				break;
			}
		}
	}
	else
		perror(_("Error opening fifo file"));
	remove(filename_fifo); // fifo destroyed on last close
	::close(fh_fifo);
	window->editing_lock.lock();
	window->editing = 0;
	window->editing_lock.unlock();
	struct stat st;
	client->config.ms_time = stat(client->config.svg_file, &st) ? 0 :
		st.st_mtim.tv_sec*1000 + st.st_mtim.tv_nsec/1000000;
	client->send_configure_change();
}

SvgInkscapeThread::SvgInkscapeThread(EditSvgButton *edit)
 : Thread(1)
{
	this->edit = edit;;
}

SvgInkscapeThread::~SvgInkscapeThread()
{
	cancel();
	join();
}

static int exec_command(char* const*argv)
{
        pid_t pid = vfork();
        if( pid < 0 ) return -1;
        if( pid > 0 ) {
                int stat = 0;
		waitpid(pid, &stat, 0);
                if( stat ) {
                        char msg[BCTEXTLEN];
                        sprintf(msg, "%s: error exit status %d", argv[0], stat);
                        MainError::show_error(msg);
                }
                return 0;
	}
        execvp(argv[0], &argv[0]);
        return -1;
}

void SvgInkscapeThread::run()
{
// Runs the inkscape
	char command[1024];
	snprintf(command, sizeof(command),
		"inkscape --with-gui %s", edit->client->config.svg_file);
	printf(_("Running external SVG editor: %s\n"), command);
	char *const argv[] = {
		(char*) "inkscape",
		(char*)"--with-gui",
		edit->client->config.svg_file,
		0,
	};

	enable_cancel();
	exec_command(argv);
	printf(_("External SVG editor finished\n"));
	struct fifo_struct fifo_buf;
	fifo_buf.pid = getpid();
	fifo_buf.action = 2;
	write(edit->fh_fifo, &fifo_buf, sizeof(fifo_buf));
	disable_cancel();

	return;
}


NewSvgWindow::NewSvgWindow(SvgMain *client, SvgWin *window, char *init_directory)
 : BC_FileBox(0,
 	BC_WindowBase::get_resources()->filebox_h / 2,
 	init_directory,
	_("SVG Plugin: Pick SVG file"),
	_("Open an existing SVG file or create a new one"))
{
	this->window = window;
}

NewSvgWindow::~NewSvgWindow() {}


DpiValue::DpiValue(SvgWin *win, SvgMain *client, int x, int y, float *value)
 : BC_TumbleTextBox(win, *value, (float)10, (float)1000, x, y, 100)
{
//printf("SvgWidth::SvgWidth %f\n", client->config.w);
	this->client = client;
	this->win = win;
	this->value = value;
}

DpiValue::~DpiValue()
{
}

int DpiValue::handle_event()
{
	*value = atof(get_text());
	return 1;
}


DpiButton::DpiButton( SvgWin *window, SvgMain *client, int x, int y)
 : BC_GenericButton(x, y, _("update dpi"))
{
	this->client = client;
	this->window = window;
}

DpiButton::~DpiButton()
{
}

int DpiButton::handle_event()
{
	client->send_configure_change();
	return 1;
};

