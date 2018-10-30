
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

#include <math.h>
#include <stdint.h>
#include <string.h>

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "downsampleengine.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "pluginvclient.h"
#include "vframe.h"


class DownSampleMain;
class DownSampleServer;





class DownSampleConfig
{
public:
	DownSampleConfig();

	int equivalent(DownSampleConfig &that);
	void copy_from(DownSampleConfig &that);
	void interpolate(DownSampleConfig &prev,
		DownSampleConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	int horizontal_x;
	int vertical_y;
	int horizontal;
	int vertical;
	int r;
	int g;
	int b;
	int a;
};


class DownSampleToggle : public BC_CheckBox
{
public:
	DownSampleToggle(DownSampleMain *plugin,
		int x,
		int y,
		int *output,
		char *string);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleSize : public BC_ISlider
{
public:
	DownSampleSize(DownSampleMain *plugin,
		int x,
		int y,
		int *output,
		int min,
		int max);
	int handle_event();
	DownSampleMain *plugin;
	int *output;
};

class DownSampleWindow : public PluginClientWindow
{
public:
	DownSampleWindow(DownSampleMain *plugin);
	~DownSampleWindow();

	void create_objects();

	DownSampleToggle *r, *g, *b, *a;
	DownSampleSize *h, *v, *h_x, *v_y;
	DownSampleMain *plugin;
};





class DownSampleMain : public PluginVClient
{
public:
	DownSampleMain(PluginServer *server);
	~DownSampleMain();

	int process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate);
	int is_realtime();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	int handle_opengl();

	PLUGIN_CLASS_MEMBERS(DownSampleConfig)

	VFrame *input, *output;
	DownSampleServer *engine;
};















REGISTER_PLUGIN(DownSampleMain)



DownSampleConfig::DownSampleConfig()
{
	horizontal = 2;
	vertical = 2;
	horizontal_x = 0;
	vertical_y = 0;
	r = 1;
	g = 1;
	b = 1;
	a = 1;
}

int DownSampleConfig::equivalent(DownSampleConfig &that)
{
	return
		horizontal == that.horizontal &&
		vertical == that.vertical &&
		horizontal_x == that.horizontal_x &&
		vertical_y == that.vertical_y &&
		r == that.r &&
		g == that.g &&
		b == that.b &&
		a == that.a;
}

void DownSampleConfig::copy_from(DownSampleConfig &that)
{
	horizontal = that.horizontal;
	vertical = that.vertical;
	horizontal_x = that.horizontal_x;
	vertical_y = that.vertical_y;
	r = that.r;
	g = that.g;
	b = that.b;
	a = that.a;
}

void DownSampleConfig::interpolate(DownSampleConfig &prev,
	DownSampleConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	this->horizontal = (int)(prev.horizontal * prev_scale + next.horizontal * next_scale);
	this->vertical = (int)(prev.vertical * prev_scale + next.vertical * next_scale);
	this->horizontal_x = (int)(prev.horizontal_x * prev_scale + next.horizontal_x * next_scale);
	this->vertical_y = (int)(prev.vertical_y * prev_scale + next.vertical_y * next_scale);
	r = prev.r;
	g = prev.g;
	b = prev.b;
	a = prev.a;
}












DownSampleWindow::DownSampleWindow(DownSampleMain *plugin)
 : PluginClientWindow(plugin,
	230,
	380,
	230,
	380,
	0)
{
	this->plugin = plugin;
}

DownSampleWindow::~DownSampleWindow()
{
}

void DownSampleWindow::create_objects()
{
	int x = 10, y = 10;

	add_subwindow(new BC_Title(x, y, _("Horizontal")));
	y += 30;
	add_subwindow(h = new DownSampleSize(plugin,
		x,
		y,
		&plugin->config.horizontal,
		1,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Horizontal offset")));
	y += 30;
	add_subwindow(h_x = new DownSampleSize(plugin,
		x,
		y,
		&plugin->config.horizontal_x,
		0,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Vertical")));
	y += 30;
	add_subwindow(v = new DownSampleSize(plugin,
		x,
		y,
		&plugin->config.vertical,
		1,
		100));
	y += 30;
	add_subwindow(new BC_Title(x, y, _("Vertical offset")));
	y += 30;
	add_subwindow(v_y = new DownSampleSize(plugin,
		x,
		y,
		&plugin->config.vertical_y,
		0,
		100));
	y += 30;
	add_subwindow(r = new DownSampleToggle(plugin,
		x,
		y,
		&plugin->config.r,
		_("Red")));
	y += 30;
	add_subwindow(g = new DownSampleToggle(plugin,
		x,
		y,
		&plugin->config.g,
		_("Green")));
	y += 30;
	add_subwindow(b = new DownSampleToggle(plugin,
		x,
		y,
		&plugin->config.b,
		_("Blue")));
	y += 30;
	add_subwindow(a = new DownSampleToggle(plugin,
		x,
		y,
		&plugin->config.a,
		_("Alpha")));
	y += 30;

	show_window();
	flush();
}











DownSampleToggle::DownSampleToggle(DownSampleMain *plugin,
	int x,
	int y,
	int *output,
	char *string)
 : BC_CheckBox(x, y, *output, string)
{
	this->plugin = plugin;
	this->output = output;
}

int DownSampleToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}







DownSampleSize::DownSampleSize(DownSampleMain *plugin,
	int x,
	int y,
	int *output,
	int min,
	int max)
 : BC_ISlider(x, y, 0, 200, 200, min, max, *output)
{
	this->plugin = plugin;
	this->output = output;
}
int DownSampleSize::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}










DownSampleMain::DownSampleMain(PluginServer *server)
 : PluginVClient(server)
{

	engine = 0;
}

DownSampleMain::~DownSampleMain()
{


	if(engine) delete engine;
}

const char* DownSampleMain::plugin_title() { return N_("Downsample"); }
int DownSampleMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(DownSampleMain, DownSampleWindow)

LOAD_CONFIGURATION_MACRO(DownSampleMain, DownSampleConfig)


int DownSampleMain::process_buffer(VFrame *frame,
	int64_t start_position,
	double frame_rate)
{
	this->input = frame;
	this->output = frame;
	load_configuration();

// This can't be done efficiently in a shader because every output pixel
// requires summing a large, arbitrary block of the input pixels.
// Scaling down a texture wouldn't average every pixel.
	read_frame(frame, 0, start_position, frame_rate, 0);
//		get_use_opengl());

// Use hardware
// 	if(get_use_opengl())
// 	{
// 		run_opengl();
// 		return 0;
// 	}

// Process in destination
	if(!engine) engine = new DownSampleServer(get_project_smp() + 1,
		get_project_smp() + 1);
	engine->process_frame(output, output,
		config.r, config.g, config.b, config.a,
		config.vertical, config.horizontal,
		config.vertical_y, config.horizontal_x);

	return 0;
}


void DownSampleMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		((DownSampleWindow*)thread->window)->lock_window();
		((DownSampleWindow*)thread->window)->h->update(config.horizontal);
		((DownSampleWindow*)thread->window)->v->update(config.vertical);
		((DownSampleWindow*)thread->window)->h_x->update(config.horizontal_x);
		((DownSampleWindow*)thread->window)->v_y->update(config.vertical_y);
		((DownSampleWindow*)thread->window)->r->update(config.r);
		((DownSampleWindow*)thread->window)->g->update(config.g);
		((DownSampleWindow*)thread->window)->b->update(config.b);
		((DownSampleWindow*)thread->window)->a->update(config.a);
		((DownSampleWindow*)thread->window)->unlock_window();
	}
}




void DownSampleMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("DOWNSAMPLE");

	output.tag.set_property("HORIZONTAL", config.horizontal);
	output.tag.set_property("VERTICAL", config.vertical);
	output.tag.set_property("HORIZONTAL_X", config.horizontal_x);
	output.tag.set_property("VERTICAL_Y", config.vertical_y);
	output.tag.set_property("R", config.r);
	output.tag.set_property("G", config.g);
	output.tag.set_property("B", config.b);
	output.tag.set_property("A", config.a);
	output.append_tag();
	output.tag.set_title("/DOWNSAMPLE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DownSampleMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	int result = 0;

	while(!result) {
		result = input.read_tag();
		if(!result) {
			if(input.tag.title_is("DOWNSAMPLE")) {
				config.horizontal = input.tag.get_property("HORIZONTAL", config.horizontal);
				config.vertical = input.tag.get_property("VERTICAL", config.vertical);
				config.horizontal_x = input.tag.get_property("HORIZONTAL_X", config.horizontal_x);
				config.vertical_y = input.tag.get_property("VERTICAL_Y", config.vertical_y);
				config.r = input.tag.get_property("R", config.r);
				config.g = input.tag.get_property("G", config.g);
				config.b = input.tag.get_property("B", config.b);
				config.a = input.tag.get_property("A", config.a);
			}
		}
	}
}


int DownSampleMain::handle_opengl()
{
#ifdef HAVE_GL
	static const char *downsample_frag =
		"uniform sampler2D tex;\n"
		"uniform float w;\n"
		"uniform float h;\n"
		"uniform float x_offset;\n"
		"uniform float y_offset;\n";
	(void) downsample_frag; // whatever
#endif
	return 0;
}


