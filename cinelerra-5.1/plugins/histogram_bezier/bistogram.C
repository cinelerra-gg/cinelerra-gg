
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
#include <unistd.h>

#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "bistogram.h"
#include "bistogramconfig.h"
#include "bistogramwindow.h"
#include "keyframe.h"
#include "language.h"
#include "loadbalance.h"
#include "bccolors.h"
#include "vframe.h"



class HistogramMain;
class HistogramEngine;
class HistogramWindow;

REGISTER_PLUGIN(HistogramMain)

HistogramMain::HistogramMain(PluginServer *server)
 : PluginVClient(server)
{
	engine = 0;
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		lookup[i] = 0;
		smoothed[i] = 0;
		linear[i] = 0;
		accum[i] = 0;
	}
	current_point = -1;
	mode = HISTOGRAM_VALUE;
	dragging_point = 0;
	input = 0;
	output = 0;
	slots = 0;
}

HistogramMain::~HistogramMain()
{
	reset();
}

void HistogramMain::reset()
{
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		delete [] lookup[i];    lookup[i] = 0;
		delete [] smoothed[i];  smoothed[i] = 0;
		delete [] linear[i];    linear[i] = 0;
		delete [] accum[i];     accum[i] = 0;
	}
	delete engine;  engine = 0;
	slots = 0;
}

const char* HistogramMain::plugin_title() { return N_("Histogram Bezier"); }
int HistogramMain::is_realtime() { return 1; }


NEW_WINDOW_MACRO(HistogramMain, HistogramWindow)
LOAD_CONFIGURATION_MACRO(HistogramMain, HistogramConfig)

void HistogramMain::render_gui(void *data)
{
	if(thread)
	{
		VFrame *input = (VFrame *)data;
		calculate_histogram(input);
		if( config.automatic )
			calculate_automatic(input);

		HistogramWindow *window = (HistogramWindow *)thread->window;
		window->lock_window("HistogramMain::render_gui");
		window->update_canvas();
		if(config.automatic)
		{
			window->update_input();
		}
		window->unlock_window();
	}
}

void HistogramMain::update_gui()
{
	if( !thread ) return;
	HistogramWindow *window = (HistogramWindow *)thread->window;
// points delete in load_configuration,read_data
	window->lock_window("HistogramMain::update_gui");
	if( load_configuration() ) {
		window->update(0);
		if(!config.automatic)
			window->update_input();
	}
	window->unlock_window();
}


void HistogramMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("HISTOGRAM");

	char string[BCTEXTLEN];


	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		sprintf(string, "OUTPUT_MIN_%d", i);
		output.tag.set_property(string, config.output_min[i]);
		sprintf(string, "OUTPUT_MAX_%d", i);
	   	output.tag.set_property(string, config.output_max[i]);
//printf("HistogramMain::save_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
	}

	output.tag.set_property("AUTOMATIC", config.automatic);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("SPLIT", config.split);
	output.tag.set_property("INTERPOLATION", config.smoothMode);
	output.append_tag();
	output.tag.set_title("/HISTOGRAM");
	output.append_tag();
	output.append_newline();





	for( int j=0; j<HISTOGRAM_MODES; ++j ) {
		output.tag.set_title("POINTS");
		output.append_tag();
		output.append_newline();


		HistogramPoint *current = config.points[j].first;
		while( current ) {
			output.tag.set_title("POINT");
			output.tag.set_property("X", current->x);
			output.tag.set_property("Y", current->y);
			output.tag.set_property("GRADIENT", current->gradient);
			output.tag.set_property("XOFFSET_LEFT", current->xoffset_left);
			output.tag.set_property("XOFFSET_RIGHT", current->xoffset_right);
			output.append_tag();
			output.tag.set_title("/POINT");
			output.append_tag();
			output.append_newline();
			current = NEXT;
		}


		output.tag.set_title("/POINTS");
		output.append_tag();
		output.append_newline();
	}






	output.terminate_string();
}

void HistogramMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	int current_input_mode = 0;


	while( !(result = input.read_tag()) ) {
		if( input.tag.title_is("HISTOGRAM") ) {
			char string[BCTEXTLEN];
			for( int i=0; i<HISTOGRAM_MODES; ++i ) {
				sprintf(string, "OUTPUT_MIN_%d", i);
				config.output_min[i] = input.tag.get_property(string, config.output_min[i]);
				sprintf(string, "OUTPUT_MAX_%d", i);
				config.output_max[i] = input.tag.get_property(string, config.output_max[i]);
//printf("HistogramMain::read_data %d %f %d\n", config.input_min[i], config.input_mid[i], config.input_max[i]);
			}
			config.automatic = input.tag.get_property("AUTOMATIC", config.automatic);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.split = input.tag.get_property("SPLIT", config.split);
			config.smoothMode = input.tag.get_property("INTERPOLATION", config.smoothMode);
		}
		else if( input.tag.title_is("POINTS") ) {
			if( current_input_mode < HISTOGRAM_MODES ) {
				HistogramPoints *points = &config.points[current_input_mode];
				while( points->last )
					delete points->last;
				while( !(result = input.read_tag()) ) {
					if( input.tag.title_is("/POINTS") ) break;
					if(input.tag.title_is("POINT") ) {
						points->insert(
							input.tag.get_property("X", 0.0),
							input.tag.get_property("Y", 0.0));
						points->last->gradient =
							input.tag.get_property("GRADIENT", 1.0);
						points->last->xoffset_left =
							input.tag.get_property("XOFFSET_LEFT", -0.02);
						points->last->xoffset_right =
							input.tag.get_property("XOFFSET_RIGHT", 0.02);
					}
				}
				++current_input_mode;
			}
		}
	}

	config.boundaries();

}

float HistogramMain::calculate_linear(float input,
	int subscript,
	int use_value)
{
	int done = 0;
	float output;

	if(input < 0) {
		output = 0;
		done = 1;
	}

	if(input > 1) {
		output = 1;
		done = 1;
	}

	if(!done) {

		float x1 = 0, y1 = 0;
		float grad1 = 1.0;
		float x1right = 0;
		float x2 = 1, y2 = 1;
		float grad2 = 1.0;
		float x2left = 0;

// Get 2 points surrounding current position
		HistogramPoints *points = &config.points[subscript];
		HistogramPoint *current = points->first;
		int done = 0;
		while( current && !done ) {
			if( current->x > input ) {
				x2 = current->x;
				y2 = current->y;
				grad2 = current->gradient;
				x2left = current->xoffset_left;
				done = 1;
			}
			else
				current = NEXT;
		}

		current = points->last;
		done = 0;
		while( current && !done ) {
			if( current->x <= input ) {
				x1 = current->x;
				y1 = current->y;
				grad1 = current->gradient;
				done = 1;
				x1right = current->xoffset_right;
			}
			else
				current = PREVIOUS;
		}

		if( !EQUIV(x2 - x1, 0) ) {
		  if( config.smoothMode == HISTOGRAM_LINEAR )
			output = (input - x1) * (y2 - y1) / (x2 - x1) + y1;
		  else if( config.smoothMode == HISTOGRAM_POLYNOMINAL ) {
		  	/* Construct third grade polynom between every two points */
			float dx = x2 - x1;
			float dy = y2 - y1;
			float delx = input - x1;
			output = (grad2 * dx + grad1 * dx - 2*dy) / (dx * dx * dx) * delx * delx * delx +
				(3*dy - 2* grad1*dx - grad2*dx) / (dx * dx) * delx * delx + grad1*delx + y1;
		  }
		  else if( config.smoothMode == HISTOGRAM_BEZIER ) {
		  	/* Using standart DeCasteljau algorithm */
		  	float y1right = y1 + grad1 * x1right;
		  	float y2left = y2 + grad2 * x2left;

		  	float t = (input - x1) / (x2 - x1);

			float pointAy = y1 + (y1right - y1) * t;
			float pointBy = y1right + (y2left - y1right) * t;
			float pointCy = y2left + (y2 - y2left) * t;
			float pointABy = pointAy + (pointBy - pointAy) * t;
			float pointBCy = pointBy + (pointCy - pointBy) * t;
			output = pointABy + (pointBCy - pointABy) * t;
		  }
		}
		else
// Linear
			output = input * y2;
	}

// Apply value curve
	if( use_value ) {
		output = calculate_linear(output, HISTOGRAM_VALUE, 0);
	}


	float output_min = config.output_min[subscript];
	float output_max = config.output_max[subscript];

// Compress output for value followed by channel
	output = output_min + output * (output_max - output_min);
	return output;
}

float HistogramMain::calculate_smooth(float input, int subscript)
{
	int bins = slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
	int bins1 = bins-1;
	float x_f = (input - HIST_MIN_INPUT) * bins / FLOAT_RANGE;
	int x_i1 = (int)x_f;
	int x_i2 = x_i1 + 1;
	CLAMP(x_i1, 0, bins1);
	CLAMP(x_i2, 0, bins1);
	CLAMP(x_f, 0, bins1);

	float smooth1 = smoothed[subscript][x_i1];
	float smooth2 = smoothed[subscript][x_i2];
	float result = smooth1 + (smooth2 - smooth1) * (x_f - x_i1);
	CLAMP(result, 0, 1.0);
	return result;
}


void HistogramMain::calculate_histogram(VFrame *data)
{
	int color_model = data->get_color_model();
	int pix_sz = BC_CModels::calculate_pixelsize(color_model);
	int comp_sz = pix_sz / BC_CModels::components(color_model);
	int needed_slots = comp_sz > 1 ? 0x10000 : 0x100;
	if( slots != needed_slots ) {
		reset();
		slots = needed_slots;
	}
	int bins = slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
	if( !accum[0] ) {
		for( int i=0; i<HISTOGRAM_MODES; ++i )
			accum[i] = new int[bins];
	}

	if( !engine ) {
		int cpus = data->get_w() * data->get_h() * pix_sz / 0x80000 + 2;
		int smps = get_project_smp();
		if( cpus > smps ) cpus = smps;
		engine = new HistogramEngine(this, cpus, cpus);
	}
	engine->process_packages(HistogramEngine::HISTOGRAM, data);

	for( int i=0; i<engine->get_total_clients(); ++i ) {
		HistogramUnit *unit = (HistogramUnit*)engine->get_client(i);
		if( i == 0 ) {
			for( int j=0; j<HISTOGRAM_MODES; ++j )
				memcpy(accum[j], unit->accum[j], sizeof(int)*bins);
		}
		else {
			for( int j=0; j<HISTOGRAM_MODES; ++j ) {
				int *out = accum[j], *in = unit->accum[j];
				for( int k=0; k<bins; ++k ) out[k] += in[k];
			}
		}
	}

// Remove top and bottom from calculations.  Doesn't work in high
// precision colormodels.
	for( int i=0; i<HISTOGRAM_MODES; ++i ) {
		accum[i][0] = 0;
		accum[i][bins-1] = 0;
	}
}


void HistogramMain::calculate_automatic(VFrame *data)
{
	calculate_histogram(data);
	config.reset_points();
	int bins = slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;

// Do each channel
	for( int i=0; i<3; ++i ) {
		int *accum = this->accum[i];
		int pixels = data->get_w() * data->get_h();
		float white_fraction = 1.0 - (1.0 - config.threshold) / 2;
		int threshold = (int)(white_fraction * pixels);
		int total = 0;
		float max_level = 1.0;
		float min_level = 0.0;
// Get histogram slot above threshold of pixels
		for( int j=0; j<bins; ++j ) {
			total += accum[j];
			if( total >= threshold ) {
				max_level = (float)j/bins * FLOAT_RANGE + HIST_MIN_INPUT;
				break;
			}
		}

// Get slot below 99% of pixels
		total = 0;
		for( int j=bins; --j>=0; ) {
			total += accum[j];
			if( total >= threshold ) {
				min_level = (float)j/bins * FLOAT_RANGE + HIST_MIN_INPUT;
				break;
			}
		}
		config.points[i].insert(max_level, 1.0);
		config.points[i].insert(min_level, 0.0);
	}
}

int HistogramMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	this->input = input_ptr;
	this->output = output_ptr;

	int need_reconfigure = load_configuration();
	int color_model = input->get_color_model();
	int pix_sz = BC_CModels::calculate_pixelsize(color_model);
	int comp_sz = pix_sz / BC_CModels::components(color_model);
	int needed_slots = comp_sz > 1 ? 0x10000 : 0x100;
	if( slots != needed_slots ) {
		reset();
		slots = needed_slots;
		need_reconfigure = 1;
	}

	send_render_gui(input);

	if( input->get_rows()[0] != output_ptr->get_rows()[0] ) {
		output_ptr->copy_from(input);
	}

SET_TRACE
// Generate tables here.  The same table is used by many packages to render
// each horizontal stripe.  Need to cover the entire output range in  each
// table to avoid green borders
	if( !lookup[0] || !smoothed[0] || !linear[0] || config.automatic)
		need_reconfigure = 1;
	if( need_reconfigure ) {
SET_TRACE
// Calculate new curves
		if( config.automatic ) {
			calculate_automatic(input);
		}
SET_TRACE

// Generate transfer tables for integer colormodels.
		for( int i=0; i<3; ++i )
			tabulate_curve(i, 1);
SET_TRACE
	}

	if( !engine ) {
		int cpus = input->get_w() * input->get_h() * pix_sz / 0x80000 + 2;
		int smps = get_project_smp();
		if( cpus > smps ) cpus = smps;
		engine = new HistogramEngine(this, cpus, cpus);
	}
// Apply histogram
	engine->process_packages(HistogramEngine::APPLY, input);

SET_TRACE
	return 0;
}

void HistogramMain::tabulate_curve(int subscript, int use_value)
{
	int bins = slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
	if(!lookup[subscript])
		lookup[subscript] = new int[bins];
	if(!smoothed[subscript])
		smoothed[subscript] = new float[bins];
	if(!linear[subscript])
		linear[subscript] = new float[bins];

	float *current_smooth = smoothed[subscript];
	float *current_linear = linear[subscript];

// Make linear curve
	for( int i=0; i<bins; ++i ) {
		float input = (float)i/bins * FLOAT_RANGE + HIST_MIN_INPUT;
		current_linear[i] = calculate_linear(input, subscript, use_value);
	}
// Make smooth curve
	//float prev = 0.0;
	for( int i=0; i<bins; ++i ) {
//		current_smooth[i] = current_linear[i] * 0.001 +
//			prev * 0.999;
		current_smooth[i] = current_linear[i];
//		prev = current_smooth[i];
	}
// Generate lookup tables for integer colormodels
	if( input ) {
		int slots1 = slots-1;
		for( int i=0; i<slots; ++i ) {
			lookup[subscript][i] =
				(int)(calculate_smooth((float)i/slots1, subscript) * slots1);
		}
	}
}

HistogramPackage::HistogramPackage()
 : LoadPackage()
{
}

HistogramUnit::HistogramUnit(HistogramEngine *server,
	HistogramMain *plugin)
 : LoadClient(server)
{
	this->plugin = plugin;
	this->server = server;
	int bins = plugin->slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
	for( int i=0; i<HISTOGRAM_MODES; ++i )
		accum[i] = new int[bins];
}

HistogramUnit::~HistogramUnit()
{
	for( int i=0; i<HISTOGRAM_MODES; ++i )
		delete [] accum[i];
}

void HistogramUnit::process_package(LoadPackage *package)
{
	HistogramPackage *pkg = (HistogramPackage*)package;
	int bins = plugin->slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
	int bins1 = bins-1;
	int slots1 = -HISTOGRAM_MIN * (plugin->slots-1) / 100;
	if( server->operation == HistogramEngine::HISTOGRAM ) {

#define HISTOGRAM_HEAD(type) \
{ \
	for( int i=pkg->start; i<pkg->end; ++i ) { \
		type *row = (type*)data->get_rows()[i]; \
		for( int j=0; j<w; ++j ) {

#define HISTOGRAM_TAIL(components) \
			v = MAX(r, g); v = MAX(v, b); v += slots1; \
			r += slots1;  g += slots1;  b += slots1; \
			CLAMP(r, 0, bins1);  ++accum_r[r]; \
			CLAMP(g, 0, bins1);  ++accum_g[g]; \
			CLAMP(b, 0, bins1);  ++accum_b[b]; \
			CLAMP(v, 0, bins1);  ++accum_v[v]; \
			row += components; \
		} \
	} \
}

		VFrame *data = server->data;
		int w = data->get_w();
//		int h = data->get_h();
		int *accum_r = accum[HISTOGRAM_RED];
		int *accum_g = accum[HISTOGRAM_GREEN];
		int *accum_b = accum[HISTOGRAM_BLUE];
		int *accum_v = accum[HISTOGRAM_VALUE];
		int r, g, b, y, u, v;

		switch( data->get_color_model() ) {
		case BC_RGB888:
			HISTOGRAM_HEAD(unsigned char)
			r = row[0]; g = row[1]; b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGB_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV888:
			HISTOGRAM_HEAD(unsigned char)
			y = row[0]; u = row[1]; v = row[2];
			YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA8888:
			HISTOGRAM_HEAD(unsigned char)
			r = row[0]; g = row[1]; b = row[2];
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGBA_FLOAT:
			HISTOGRAM_HEAD(float)
			r = (int)(row[0] * 0xffff);
			g = (int)(row[1] * 0xffff);
			b = (int)(row[2] * 0xffff);
			HISTOGRAM_TAIL(4)
			break;
		case BC_YUVA8888:
			HISTOGRAM_HEAD(unsigned char)
			y = row[0]; u = row[1]; v = row[2];
			YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		case BC_RGB161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0]; g = row[1]; b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUV161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0]; u = row[1]; v = row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(3)
			break;
		case BC_RGBA16161616:
			HISTOGRAM_HEAD(uint16_t)
			r = row[0]; g = row[1]; b = row[2];
			HISTOGRAM_TAIL(3)
			break;
		case BC_YUVA16161616:
			HISTOGRAM_HEAD(uint16_t)
			y = row[0]; u = row[1]; v = row[2];
			YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);
			HISTOGRAM_TAIL(4)
			break;
		}
	}
	else
	if( server->operation == HistogramEngine::APPLY ) {

#define PROCESS(type, components) \
{ \
	for( int i=pkg->start; i<pkg->end; ++i ) { \
		type *row = (type*)input->get_rows()[i]; \
		for( int j=0; j<w; ++j ) { \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
			 	continue; \
			row[0] = lookup_r[row[0]]; \
			row[1] = lookup_g[row[1]]; \
			row[2] = lookup_b[row[2]]; \
			row += components; \
		} \
	} \
}

#define PROCESS_YUV(type, components, max) \
{ \
	for( int i=pkg->start; i<pkg->end; ++i ) { \
		type *row = (type*)input->get_rows()[i]; \
		for( int j=0; j<w; ++j ) { \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
				continue; \
			y = row[0]; u = row[1]; v = row[2]; \
			if( max == 0xff ) \
				YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v); \
			else \
				YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
			r = lookup_r[r]; g = lookup_g[g]; b = lookup_b[b]; \
			if( max == 0xff ) \
				YUV::yuv.rgb_to_yuv_8(r, g, b, y, u, v); \
			else \
				YUV::yuv.rgb_to_yuv_16(r, g, b, y, u, v); \
			row[0] = y; row[1] = u; row[2] = v; \
			row += components; \
		} \
	} \
}

#define PROCESS_FLOAT(components) \
{ \
	for( int i=pkg->start; i<pkg->end; ++i ) { \
		float *row = (float*)input->get_rows()[i]; \
		for( int j=0; j<w; ++j ) { \
			if ( plugin->config.split && ((j + i * w / h) < w) ) \
				continue; \
			float r = row[0], g = row[1], b = row[2]; \
			r = plugin->calculate_smooth(r, HISTOGRAM_RED); \
			g = plugin->calculate_smooth(g, HISTOGRAM_GREEN); \
			b = plugin->calculate_smooth(b, HISTOGRAM_BLUE); \
			row[0] = r; row[1] = g; row[2] = b; \
			row += components; \
		} \
	} \
}


		VFrame *input = plugin->input;
//		VFrame *output = plugin->output;
		int w = input->get_w(), h = input->get_h();
		int *lookup_r = plugin->lookup[0];
		int *lookup_g = plugin->lookup[1];
		int *lookup_b = plugin->lookup[2];
		int r, g, b, y, u, v;
		switch( input->get_color_model() ) {
		case BC_RGB888:
			PROCESS(unsigned char, 3)
			break;
		case BC_RGB_FLOAT:
			PROCESS_FLOAT(3);
			break;
		case BC_RGBA8888:
			PROCESS(unsigned char, 4)
			break;
		case BC_RGBA_FLOAT:
			PROCESS_FLOAT(4);
			break;
		case BC_RGB161616:
			PROCESS(uint16_t, 3)
			break;
		case BC_RGBA16161616:
			PROCESS(uint16_t, 4)
			break;
		case BC_YUV888:
			PROCESS_YUV(unsigned char, 3, 0xff)
			break;
		case BC_YUVA8888:
			PROCESS_YUV(unsigned char, 4, 0xff)
			break;
		case BC_YUV161616:
			PROCESS_YUV(uint16_t, 3, 0xffff)
			break;
		case BC_YUVA16161616:
			PROCESS_YUV(uint16_t, 4, 0xffff)
			break;
		}
	}
}


HistogramEngine::HistogramEngine(HistogramMain *plugin,
		int total_clients, int total_packages)
 : LoadServer(total_clients, total_packages)
{
	this->plugin = plugin;
}

void HistogramEngine::init_packages()
{
	total_size = data->get_h();
	for( int i=0,start=0,n=get_total_packages(); i<n; ) {
		HistogramPackage *package = (HistogramPackage*)get_package(i);
		package->start = start;
		package->end = start = (total_size * ++i)/ n;
	}

	int bins = plugin->slots * (HISTOGRAM_MAX-HISTOGRAM_MIN)/100;
// Initialize clients here in case some don't get run.
	for( int i=0; i<get_total_clients(); ++i ) {
		HistogramUnit *unit = (HistogramUnit*)get_client(i);
		for( int i=0; i<HISTOGRAM_MODES; ++i )
			bzero(unit->accum[i], sizeof(int)*bins);
	}

}

LoadClient* HistogramEngine::new_client()
{
	return new HistogramUnit(this, plugin);
}

LoadPackage* HistogramEngine::new_package()
{
	return new HistogramPackage;
}

void HistogramEngine::process_packages(int operation, VFrame *data)
{
	this->data = data;
	this->operation = operation;
	LoadServer::process_packages();
}



