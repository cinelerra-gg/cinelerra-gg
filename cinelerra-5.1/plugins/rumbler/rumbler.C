
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

#include "bcdisplayinfo.h"
#include "affine.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "guicast.h"
#include "keyframe.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.h"

#include <math.h>
#include <string.h>
#include <stdint.h>


class Rumbler;
class RumblerConfig;
class RumblerRate;
class RumblerSeq;
class RumblerWindow;


class RumblerConfig
{
public:
	RumblerConfig();
	float time_rumble, time_rate;
	float space_rumble, space_rate;
	int sequence;
	void copy_from(RumblerConfig &that);
	int equivalent(RumblerConfig &that);
	void interpolate(RumblerConfig &prev, RumblerConfig &next,
	        int64_t prev_frame, int64_t next_frame, int64_t current_frame);
};

class RumblerRate : public BC_TextBox
{
public:
	RumblerRate(Rumbler *plugin, RumblerWindow *gui,
		float &value, int x, int y);
	int handle_event();
	Rumbler *plugin;
	RumblerWindow *gui;
	float *value;
};

class RumblerSeq : public BC_TextBox
{
public:
	RumblerSeq(Rumbler *plugin, RumblerWindow *gui,
		int &value, int x, int y);
	int handle_event();
	Rumbler *plugin;
	RumblerWindow *gui;
	int *value;
};


class RumblerWindow : public PluginClientWindow
{
public:
	RumblerWindow(Rumbler *plugin);
	~RumblerWindow();
	void create_objects();

	Rumbler *plugin;
	RumblerRate *time_rumble, *time_rate;
	RumblerRate *space_rumble, *space_rate;
	RumblerSeq *seq;
};

class Rumbler : public PluginVClient
{
public:
	Rumbler(PluginServer *server);
	~Rumbler();

	PLUGIN_CLASS_MEMBERS(RumblerConfig)

	int process_buffer(VFrame *frame, int64_t start_position, double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();
	static double rumble(int64_t seq, double cur, double per, double amp);

	VFrame *input, *output, *temp;
	float x1,y1, x2,y2, x3,y3, x4,y4;
	AffineEngine *engine;
};


RumblerConfig::RumblerConfig()
{
	time_rumble = 10;
	time_rate = 1.;
	space_rumble = 10;
	space_rate = 1.;
	sequence = 0;
}

void RumblerConfig::copy_from(RumblerConfig &that)
{
	time_rumble = that.time_rumble;
	time_rate = that.time_rate;
	space_rumble = that.space_rumble;
	space_rate = that.space_rate;
	sequence = that.sequence;
}

int RumblerConfig::equivalent(RumblerConfig &that)
{
	return time_rumble == that.time_rumble &&
		time_rate == that.time_rate &&
		space_rumble == that.space_rumble &&
		space_rate == that.space_rate &&
		sequence == that.sequence;
}

void RumblerConfig::interpolate(RumblerConfig &prev, RumblerConfig &next,
        int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
        double next_scale  = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
        double prev_scale  = (double)(next_frame - current_frame) / (next_frame - prev_frame);
        this->time_rumble  = prev.time_rumble * prev_scale + next.time_rumble * next_scale;
        this->time_rate    = prev.time_rate * prev_scale + next.time_rate * next_scale;
        this->space_rumble = prev.space_rumble * prev_scale + next.space_rumble * next_scale;
        this->space_rate   = prev.space_rate * prev_scale + next.space_rate * next_scale;
        this->sequence     = prev.sequence;
}

RumblerWindow::RumblerWindow(Rumbler *plugin)
 : PluginClientWindow(plugin, 300, 120, 300, 120, 0)
{
	this->plugin = plugin;
}

RumblerWindow::~RumblerWindow()
{
}

void RumblerWindow::create_objects()
{
	int x = 10, x1 = x + 64, x2 =x1 + 100;
	int y = 10;
	BC_Title *title;

	add_subwindow(title = new BC_Title(x1, y, _("rumble")));
	add_subwindow(title = new BC_Title(x2, y, _("rate")));
	y += title->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("time:")));
	add_subwindow(time_rumble = new RumblerRate(plugin, this, plugin->config.time_rumble, x1, y));
	add_subwindow(time_rate = new RumblerRate(plugin, this, plugin->config.time_rate, x2, y));
	y += title->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("space:")));
	add_subwindow(space_rumble = new RumblerRate(plugin, this, plugin->config.space_rumble, x1, y));
	add_subwindow(space_rate = new RumblerRate(plugin, this, plugin->config.space_rate, x2, y));
	y += title->get_h() + 10;
	add_subwindow(title = new BC_Title(x, y, _("seq:")));
	add_subwindow(seq = new RumblerSeq(plugin, this, plugin->config.sequence, x1, y));

	show_window();
	flush();
}


RumblerRate::RumblerRate(Rumbler *plugin, RumblerWindow *gui,
	float &value, int x, int y)
 : BC_TextBox(x, y, 90, 1, value)
{
	this->plugin = plugin;
	this->value = &value;
	this->gui = gui;
}

int RumblerRate::handle_event()
{
	*value = atof(get_text());
	plugin->send_configure_change();
	return 1;
}

RumblerSeq::RumblerSeq(Rumbler *plugin, RumblerWindow *gui,
	int &value, int x, int y)
 : BC_TextBox(x, y, 72, 1, value)
{
	this->plugin = plugin;
	this->value = &value;
	this->gui = gui;
}

int RumblerSeq::handle_event()
{
	*value = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}


REGISTER_PLUGIN(Rumbler)

Rumbler::Rumbler(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
}

Rumbler::~Rumbler()
{
	delete engine;
}


double Rumbler::rumble(int64_t seq, double cur, double per, double amp)
{
	if( !per || !amp ) return 0.;
	int64_t t = cur / per;
	double t0 = t * per;
	srandom(t += seq);
	static const int64_t rmax1 = ((int64_t)RAND_MAX) + 1;
	double prev = (double)random() / rmax1 - 0.5;
	srandom(t+1);
	double next = (double)random() / rmax1 - 0.5;
	double v = (cur-t0) / per, u = 1. - v;
	return amp*(u*prev + v*next);
}

int Rumbler::process_buffer(VFrame *frame, int64_t start_position, double frame_rate)
{
	load_configuration();
	input = frame;
	output = frame;
	double cur = frame_rate > 0 ? start_position / frame_rate : 0;
	double per = config.time_rate > 0 ? 1./config.time_rate : 0;
	double rum = config.time_rumble;
	int seq = config.sequence;
	int64_t pos = rumble(seq+0x0000000, cur, per, rum) + start_position;
	read_frame(frame, 0, pos, frame_rate, 0);
	cur = frame_rate > 0 ? start_position / frame_rate : 0;
	per = config.space_rate > 0 ? 1./config.space_rate : 0;
	rum = config.space_rumble;
	x1 = rumble(seq+0x1000000, cur, per, rum) + 0;
	y1 = rumble(seq+0x2000000, cur, per, rum) + 0;
	x2 = rumble(seq+0x3000000, cur, per, rum) + 100;
	y2 = rumble(seq+0x4000000, cur, per, rum) + 0;
	x3 = rumble(seq+0x5000000, cur, per, rum) + 100;
	y3 = rumble(seq+0x6000000, cur, per, rum) + 100;
	x4 = rumble(seq+0x7000000, cur, per, rum) + 0;
	y4 = rumble(seq+0x8000000, cur, per, rum) + 100;
	if( !engine ) {
		int cpus = get_project_smp() + 1;
		engine = new AffineEngine(cpus, cpus);
	}

	if( get_use_opengl() )
		return run_opengl();

	int w = frame->get_w(), h = frame->get_h();
	int color_model = frame->get_color_model();
	input = new_temp(w, h, color_model);
	input->copy_from(frame);
	output->clear_frame();
        engine->process(output, input, input,
		AffineEngine::PERSPECTIVE,
		x1, y1, x2, y2, x3, y3, x4, y4, 1);

	return 0;
}

int Rumbler::handle_opengl()
{
#ifdef HAVE_GL
	engine->set_opengl(1);
	engine->process(output, input, input,
		AffineEngine::PERSPECTIVE,
		x1, y1, x2, y2, x3, y3, x4, y4, 1);
        engine->set_opengl(0);
#endif
        return 0;
}


const char* Rumbler::plugin_title() { return N_("Rumbler"); }
int Rumbler::is_realtime() { return 1; }

NEW_WINDOW_MACRO(Rumbler, RumblerWindow)

LOAD_CONFIGURATION_MACRO(Rumbler, RumblerConfig)

void Rumbler::save_data(KeyFrame *keyframe)
{
	FileXML output;

	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("RUMBLER");
	output.tag.set_property("TIME_RUMBLE", config.time_rumble);
	output.tag.set_property("TIME_RATE", config.time_rate);
	output.tag.set_property("SPACE_RUMBLE", config.space_rumble);
	output.tag.set_property("SPACE_RATE", config.space_rate);
	output.tag.set_property("SEQUENCE", config.sequence);
	output.append_tag();
	output.tag.set_title("/RUMBLER");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Rumbler::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while( !input.read_tag() ) {
		if( input.tag.title_is("RUMBLER") ) {
			config.time_rumble = input.tag.get_property("TIME_RUMBLE", config.time_rumble);
			config.time_rate = input.tag.get_property("TIME_RATE", config.time_rate);
			config.space_rumble = input.tag.get_property("SPACE_RUMBLE", config.space_rumble);
			config.space_rate = input.tag.get_property("SPACE_RATE", config.space_rate);
			config.sequence = input.tag.get_property("SEQUENCE", config.sequence);
		}
	}
}

void Rumbler::update_gui()
{
	if( !thread ) return;
	if( !load_configuration() ) return;
	RumblerWindow *window = (RumblerWindow*)thread->window;
	window->lock_window("Rumbler::update_gui");
	window->time_rumble->update(config.time_rumble);
	window->time_rate->update(config.time_rate);
	window->space_rumble->update(config.space_rumble);
	window->space_rate->update(config.space_rate);
	window->seq->update((float)config.sequence);
	window->unlock_window();
}

