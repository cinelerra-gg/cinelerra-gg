
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edits.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginclient.h"
#include "pluginserver.h"
#include "preferences.h"
#include "track.h"
#include "transportque.inc"


#include <ctype.h>
#include <errno.h>
#include <string.h>





PluginClientThread::PluginClientThread(PluginClient *client)
 : Thread(1, 0, 0)
{
	this->client = client;
	window = 0;
	init_complete = new Condition(0, "PluginClientThread::init_complete");
}

PluginClientThread::~PluginClientThread()
{
	join();
//printf("PluginClientThread::~PluginClientThread %p %d\n", this, __LINE__);
	delete window;  window = 0;
//printf("PluginClientThread::~PluginClientThread %p %d\n", this, __LINE__);
	delete init_complete;
}

void PluginClientThread::run()
{
	BC_DisplayInfo info;
	int result = 0;
	if(client->window_x < 0) client->window_x = info.get_abs_cursor_x();
	if(client->window_y < 0) client->window_y = info.get_abs_cursor_y();
	if(!window)
		window = (PluginClientWindow*)client->new_window();

	if(window) {
		window->lock_window("PluginClientThread::run");
		window->create_objects();
		VFrame *picon = client->server->get_picon();
		if( picon ) window->set_icon(picon);
		window->unlock_window();

/* Only set it here so tracking doesn't update it until everything is created. */
 		client->thread = this;
		init_complete->unlock();

		result = window->run_window();
		window->lock_window("PluginClientThread::run");
//printf("PluginClientThread::run %p %d\n", this, __LINE__);
		window->hide_window(1);
		window->unlock_window();
		window->done_event(result);
// Can't save defaults in the destructor because it's not called immediately
// after closing.
		/* if(client->defaults) */ client->save_defaults_xml();
/* This is needed when the GUI is closed from itself */
		if(result) client->client_side_close();
	}
	else
// No window
	{
 		client->thread = this;
		init_complete->unlock();
	}
}

BC_WindowBase* PluginClientThread::get_window()
{
	return window;
}

PluginClient* PluginClientThread::get_client()
{
	return client;
}






PluginClientFrame::PluginClientFrame(int data_size,
	int period_n,
	int period_d)
{
	this->data_size = data_size;
	force = 0;
	this->period_n = period_n;
	this->period_d = period_d;
}

PluginClientFrame::~PluginClientFrame()
{

}





PluginClientWindow::PluginClientWindow(PluginClient *client,
	int w, int h, int min_w, int min_h, int allow_resize)
 : BC_Window(client->gui_string,
	client->window_x /* - w / 2 */,
	client->window_y /* - h / 2 */,
	(int)(w*get_resources()->font_scale+0.5), (int)(h*get_resources()->font_scale+0.5),
	(int)(min_w*get_resources()->font_scale+0.5), (int)(min_h*get_resources()->font_scale+0.5),
	allow_resize, 0, 1)
{
	this->client = client;
}

PluginClientWindow::PluginClientWindow(const char *title,
	int x, int y, int w, int h, int min_w, int min_h, int allow_resize)
 : BC_Window(title, x, y,
        (int)(w*get_resources()->font_scale+0.5), (int)(h*get_resources()->font_scale+0.5),
        (int)(min_w*get_resources()->font_scale+0.5), (int)(min_h*get_resources()->font_scale+0.5),
	allow_resize, 0, 1)
{
	this->client = 0;
}

PluginClientWindow::~PluginClientWindow()
{
}


int PluginClientWindow::translation_event()
{
	if(client)
	{
		client->window_x = get_x();
		client->window_y = get_y();
	}

	return 1;
}

int PluginClientWindow::close_event()
{
/* Set result to 1 to indicate a client side close */
	set_done(1);
	return 1;
}





PluginClient::PluginClient(PluginServer *server)
{
	reset();
	this->server = server;
	smp = server->preferences->project_smp;
	defaults = 0;
	update_timer = new Timer;
// Virtual functions don't work here.
}

PluginClient::~PluginClient()
{
	if( thread ) {
		hide_gui();
		thread->join();
		delete thread;
	}

// Virtual functions don't work here.
	if(defaults) delete defaults;
	frame_buffer.remove_all_objects();
	delete update_timer;
}

int PluginClient::reset()
{
	window_x = -1;
	window_y = -1;
	interactive = 0;
	show_initially = 0;
	wr = rd = 0;
	master_gui_on = 0;
	client_gui_on = 0;
	realtime_priority = 0;
	gui_string[0] = 0;
	total_in_buffers = 0;
	total_out_buffers = 0;
	source_position = 0;
	source_start = 0;
	total_len = 0;
	direction = PLAY_FORWARD;
	thread = 0;
	using_defaults = 0;
	return 0;
}


void PluginClient::hide_gui()
{
	if(thread && thread->window)
	{
//printf("PluginClient::delete_thread %d\n", __LINE__);
/* This is needed when the GUI is closed from elsewhere than itself */
/* Since we now use autodelete, this is all that has to be done, thread will take care of itself ... */
/* Thread join will wait if this was not called from the thread itself or go on if it was */
		thread->window->lock_window("PluginClient::hide_gui");
		thread->window->set_done(0);
//printf("PluginClient::hide_gui %d thread->window=%p\n", __LINE__, thread->window);
		thread->window->unlock_window();
	}
}

// For realtime plugins initialize buffers
int PluginClient::plugin_init_realtime(int realtime_priority,
	int total_in_buffers,
	int buffer_size)
{

// Get parameters for all
	master_gui_on = get_gui_status();



// get parameters depending on video or audio
	init_realtime_parameters();

	this->realtime_priority = realtime_priority;
	this->total_in_buffers = this->total_out_buffers = total_in_buffers;
	this->out_buffer_size = this->in_buffer_size = buffer_size;
	return 0;
}

int PluginClient::plugin_start_loop(int64_t start,
	int64_t end,
	int64_t buffer_size,
	int total_buffers)
{
//printf("PluginClient::plugin_start_loop %d %ld %ld %ld %d\n",
// __LINE__, start, end, buffer_size, total_buffers);
	this->source_start = start;
	this->total_len = end - start;
	this->start = start;
	this->end = end;
	this->in_buffer_size = this->out_buffer_size = buffer_size;
	this->total_in_buffers = this->total_out_buffers = total_buffers;
	start_loop();
	return 0;
}

int PluginClient::plugin_process_loop()
{
	return process_loop();
}

int PluginClient::plugin_stop_loop()
{
	return stop_loop();
}

MainProgressBar* PluginClient::start_progress(char *string, int64_t length)
{
	return server->start_progress(string, length);
}


// Non realtime parameters
int PluginClient::plugin_get_parameters()
{
	int result = get_parameters();
	if(defaults) save_defaults();
	return result;
}

// ========================= main loop

int PluginClient::is_multichannel() { return 0; }
int PluginClient::is_synthesis() { return 0; }
int PluginClient::is_realtime() { return 0; }
int PluginClient::is_fileio() { return 0; }
int PluginClient::delete_buffer_ptrs() { return 0; }
const char* PluginClient::plugin_title() { return _("Untitled"); }

Theme* PluginClient::new_theme() { return 0; }

int PluginClient::load_configuration()
{
	return 0;
}

Theme* PluginClient::get_theme()
{
	return server->get_theme();
}

int PluginClient::show_gui()
{
	load_configuration();
	thread = new PluginClientThread(this);
	thread->start();
	thread->init_complete->lock("PluginClient::show_gui");
// Must wait before sending any hide_gui
	if( !thread->window ) return 1;
	thread->window->init_wait();
	return 0;
}

void PluginClient::raise_window()
{
	if(thread && thread->window)
	{
		thread->window->lock_window("PluginClient::raise_window");
		thread->window->raise_window();
		thread->window->flush();
		thread->window->unlock_window();
	}
}

int PluginClient::set_string()
{
	if(thread)
	{
		thread->window->lock_window("PluginClient::set_string");
		thread->window->put_title(gui_string);
		thread->window->unlock_window();
	}
	return 0;
}





void PluginClient::begin_process_buffer()
{
// Delete all unused GUI frames
	frame_buffer.remove_all_objects();
}


void PluginClient::end_process_buffer()
{
	if(frame_buffer.size())
	{
		send_render_gui();
	}
}



void PluginClient::plugin_update_gui()
{

	update_gui();

// Delete unused GUI frames
	while(frame_buffer.size() > MAX_FRAME_BUFFER)
		frame_buffer.remove_object_number(0);

}

void PluginClient::update_gui()
{
}

int PluginClient::get_gui_update_frames()
{
	if(frame_buffer.size())
	{
		PluginClientFrame *frame = frame_buffer.get(0);
		int total_frames = update_timer->get_difference() *
			frame->period_d /
			frame->period_n /
			1000;
		if(total_frames) update_timer->subtract(total_frames *
			frame->period_n *
			1000 /
			frame->period_d);

// printf("PluginClient::get_gui_update_frames %d %ld %d %d %d\n",
// __LINE__,
// update_timer->get_difference(),
// frame->period_n * 1000 / frame->period_d,
// total_frames,
// frame_buffer.size());

// Add forced frames
		for(int i = 0; i < frame_buffer.size(); i++)
			if(frame_buffer.get(i)->force) total_frames++;
		total_frames = MIN(frame_buffer.size(), total_frames);


		return total_frames;
	}
	else
	{
		return 0;
	}
}

PluginClientFrame* PluginClient::get_gui_frame()
{
	if(frame_buffer.size())
	{
		PluginClientFrame *frame = frame_buffer.get(0);
		frame_buffer.remove_number(0);
		return frame;
	}
	else
	{
		return 0;
	}
}

void PluginClient::add_gui_frame(PluginClientFrame *frame)
{
	frame_buffer.append(frame);
}

void PluginClient::send_render_gui()
{
	server->send_render_gui(&frame_buffer);
}

void PluginClient::send_render_gui(void *data)
{
	server->send_render_gui(data);
}

void PluginClient::send_render_gui(void *data, int size)
{
	server->send_render_gui(data, size);
}

void PluginClient::plugin_render_gui(void *data, int size)
{
	render_gui(data, size);
}


void PluginClient::plugin_render_gui(void *data)
{
	render_gui(data);
}

void PluginClient::render_gui(void *data)
{
	if(thread)
	{
		thread->get_window()->lock_window("PluginClient::render_gui");

// Set all previous frames to draw immediately
		for(int i = 0; i < frame_buffer.size(); i++)
			frame_buffer.get(i)->force = 1;

		ArrayList<PluginClientFrame*> *src =
			(ArrayList<PluginClientFrame*>*)data;

// Shift GUI data to GUI client
		while(src->size())
		{
			this->frame_buffer.append(src->get(0));
			src->remove_number(0);
		}

// Start the timer for the current buffer
		update_timer->update();
		thread->get_window()->unlock_window();
	}
}

void PluginClient::render_gui(void *data, int size)
{
	printf("PluginClient::render_gui %d\n", __LINE__);
}








int PluginClient::is_audio() { return 0; }
int PluginClient::is_video() { return 0; }
int PluginClient::is_theme() { return 0; }
int PluginClient::uses_gui() { return 1; }
int PluginClient::is_transition() { return 0; }
int PluginClient::load_defaults()
{
//	printf("PluginClient::load_defaults undefined in %s.\n", plugin_title());
	return 0;
}

int PluginClient::save_defaults()
{
	save_defaults_xml();
//	printf("PluginClient::save_defaults undefined in %s.\n", plugin_title());
	return 0;
}

void PluginClient::load_defaults_xml()
{
	char path[BCTEXTLEN];
	server->get_defaults_path(path);
	FileSystem fs;
	fs.complete_path(path);
	using_defaults = 1;
//printf("PluginClient::load_defaults_xml %d %s\n", __LINE__, path);

	KeyFrame temp_keyframe;
	FILE *fp = fopen(path, "r");
	if( fp ) {
		struct stat st;  int fd = fileno(fp);
		int64_t sz = !fstat(fd, &st) ? st.st_size : BCTEXTLEN;
		char *data = temp_keyframe.get_data(sz+1);
		int data_size = fread(data, 1, sz, fp);
		if( data_size < 0 ) data_size = 0;
		if( data_size > 0 ) {
			data[data_size] = 0;
			temp_keyframe.xbuf->oseek(data_size);
// Get window extents
			int i = 0;
			for( int state=0; i<(data_size-8) && state>=0; ++i ) {
				if( !data[i] || data[i] == '<' ) break;
				if( !isdigit(data[i]) ) continue;
				if( !state ) {
					window_x = atoi(data + i);
					state = 1;
				}
				else {
					window_y = atoi(data + i);
					state = -1;
				}
				while( i<data_size && isdigit(data[i]) ) ++i;
			}
			temp_keyframe.xbuf->iseek(i);
			read_data(&temp_keyframe);
		}

		fclose(fp);
	}
	using_defaults = 0;
//printf("PluginClient::load_defaults_xml %d %s\n", __LINE__, path);
}

void PluginClient::save_defaults_xml()
{
	char path[BCTEXTLEN];
	server->get_defaults_path(path);
	FileSystem fs;
	fs.complete_path(path);
	using_defaults = 1;

	KeyFrame temp_keyframe;
	save_data(&temp_keyframe);

	const char *data = temp_keyframe.get_data();
	int len = strlen(data);
	FILE *fp = fopen(path, "w");

	if( fp ) {
		fprintf(fp, "%d\n%d\n", window_x, window_y);
		if( len > 0 && !fwrite(data, len, 1, fp) ) {
			fprintf(stderr, "PluginClient::save_defaults_xml %d \"%s\" %d bytes: %s\n",
				__LINE__, path, len, strerror(errno));
		}
		fclose(fp);
	}

	using_defaults = 0;
}

int PluginClient::is_defaults()
{
	return using_defaults;
}

BC_Hash* PluginClient::get_defaults()
{
	return defaults;
}
PluginClientThread* PluginClient::get_thread()
{
	return thread;
}

BC_WindowBase* PluginClient::new_window()
{
	printf("PluginClient::new_window undefined in %s.\n", plugin_title());
	return 0;
}
int PluginClient::get_parameters() { return 0; }
int PluginClient::get_samplerate() { return get_project_samplerate(); }
double PluginClient::get_framerate() { return get_project_framerate(); }
int PluginClient::init_realtime_parameters() { return 0; }
int PluginClient::delete_nonrealtime_parameters() { return 0; }
int PluginClient::start_loop() { return 0; };
int PluginClient::process_loop() { return 0; };
int PluginClient::stop_loop() { return 0; };

void PluginClient::set_interactive()
{
	interactive = 1;
}

int64_t PluginClient::get_in_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int64_t PluginClient::get_out_buffers(int64_t recommended_size)
{
	return recommended_size;
}

int PluginClient::get_gui_status()
{
	return server->get_gui_status();
}

// close event from client side
void PluginClient::client_side_close()
{
// Last command executed
	server->client_side_close();
}

int PluginClient::stop_gui_client()
{
	if(!client_gui_on) return 0;
	client_gui_on = 0;
	return 0;
}

int PluginClient::get_project_samplerate()
{
	return server->get_project_samplerate();
}

double PluginClient::get_project_framerate()
{
	return server->get_project_framerate();
}

const char *PluginClient::get_source_path()
{
	int64_t source_position = server->plugin->startproject;
	Edit *edit = server->plugin->track->edits->editof(source_position,PLAY_FORWARD,0);
	Indexable *indexable = edit ? edit->get_source() : 0;
	return indexable ? indexable->path : 0;
}


void PluginClient::update_display_title()
{
	server->generate_display_title(gui_string);
	set_string();
}

char* PluginClient::get_gui_string()
{
	return gui_string;
}


char* PluginClient::get_path()
{
	return server->path;
}

char* PluginClient::get_plugin_dir()
{
	return server->preferences->plugin_dir;
}

int PluginClient::set_string_client(char *string)
{
	strcpy(gui_string, string);
	set_string();
	return 0;
}


int PluginClient::get_interpolation_type()
{
	return server->get_interpolation_type();
}


float PluginClient::get_red()
{
	EDL *edl = server->mwindow ? server->mwindow->edl : server->edl;
	return !edl ? 0 : edl->local_session->use_max ?
		edl->local_session->red_max :
		edl->local_session->red;
}

float PluginClient::get_green()
{
	EDL *edl = server->mwindow ? server->mwindow->edl : server->edl;
	return !edl ? 0 : edl->local_session->use_max ?
		edl->local_session->green_max :
		edl->local_session->green;
}

float PluginClient::get_blue()
{
	EDL *edl = server->mwindow ? server->mwindow->edl : server->edl;
	return !edl ? 0 : edl->local_session->use_max ?
		edl->local_session->blue_max :
		edl->local_session->blue;
}


int64_t PluginClient::get_source_position()
{
	return source_position;
}

int64_t PluginClient::get_source_start()
{
	return source_start;
}

int64_t PluginClient::get_total_len()
{
	return total_len;
}

int PluginClient::get_direction()
{
	return direction;
}


int64_t PluginClient::local_to_edl(int64_t position)
{
	return position;
}

int64_t PluginClient::edl_to_local(int64_t position)
{
	return position;
}

int PluginClient::get_use_opengl()
{
	return server->get_use_opengl();
}

int PluginClient::get_total_buffers()
{
	return total_in_buffers;
}

int PluginClient::get_buffer_size()
{
	return in_buffer_size;
}

int PluginClient::get_project_smp()
{
//printf("PluginClient::get_project_smp %d %d\n", __LINE__, smp);
	return smp;
}

const char* PluginClient::get_defaultdir()
{
	return File::get_plugin_path();
}


int PluginClient::send_hide_gui()
{
// Stop the GUI server and delete GUI messages
	client_gui_on = 0;
	return 0;
}

int PluginClient::send_configure_change()
{
	if(server->mwindow)
		server->mwindow->undo->update_undo_before(_("tweek"), this);
#ifdef USE_KEYFRAME_SPANNING
	KeyFrame keyframe;
	save_data(&keyframe);
	server->apply_keyframe(&keyframe);
#else
	KeyFrame* keyframe = server->get_keyframe();
// Call save routine in plugin
	save_data(keyframe);
#endif
	if(server->mwindow)
		server->mwindow->undo->update_undo_after(_("tweek"), LOAD_AUTOMATION);
	server->sync_parameters();
	return 0;
}


KeyFrame* PluginClient::get_prev_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_prev_keyframe(position);
}

KeyFrame* PluginClient::get_next_keyframe(int64_t position, int is_local)
{
	if(is_local) position = local_to_edl(position);
	return server->get_next_keyframe(position);
}

void PluginClient::get_camera(float *x, float *y, float *z, int64_t position)
{
	server->get_camera(x, y, z, position, direction);
}

void PluginClient::get_projector(float *x, float *y, float *z, int64_t position)
{
	server->get_projector(x, y, z, position, direction);
}


EDLSession* PluginClient::get_edlsession()
{
	if(server->edl)
		return server->edl->session;
	return 0;
}

int PluginClient::gui_open()
{
	return server->gui_open();
}
