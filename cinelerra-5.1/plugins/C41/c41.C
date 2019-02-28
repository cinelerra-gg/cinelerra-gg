/*
 * C41 plugin for Cinelerra
 * Copyright (C) 2011 Florent Delannoy <florent at plui dot es>
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
#include "c41.h"
#include "bccolors.h"
#include "clip.h"
#include "edlsession.h"
#include "filexml.h"
#include "language.h"
#include "vframe.h"

REGISTER_PLUGIN(C41Effect);

C41Config::C41Config()
{
	active = 0;
	compute_magic = 0;
	postproc = 0;
	show_box = 0;

	fix_min_r = fix_min_g = fix_min_b = fix_light = 0.;
	fix_gamma_g = fix_gamma_b = fix_coef1 = fix_coef2 = 0.;
	min_col = max_col = min_row = max_row = 0;
	window_w = 500; window_h = 510;
}

void C41Config::copy_from(C41Config &src)
{
	active = src.active;
	compute_magic = src.compute_magic;
	postproc = src.postproc;
	show_box = src.show_box;

	fix_min_r = src.fix_min_r;
	fix_min_g = src.fix_min_g;
	fix_min_b = src.fix_min_b;
	fix_light = src.fix_light;
	fix_gamma_g = src.fix_gamma_g;
	fix_gamma_b = src.fix_gamma_b;
	fix_coef1 = src.fix_coef1;
	fix_coef2 = src.fix_coef2;
	min_row = src.min_row;
	max_row = src.max_row;
	min_col = src.min_col;
	max_col = src.max_col;
}

int C41Config::equivalent(C41Config &src)
{
	return (active == src.active &&
		compute_magic == src.compute_magic &&
		postproc == src.postproc &&
		show_box == src.show_box &&
		EQUIV(fix_min_r, src.fix_min_r) &&
		EQUIV(fix_min_g, src.fix_min_g) &&
		EQUIV(fix_min_b, src.fix_min_b) &&
		EQUIV(fix_light, src.fix_light) &&
		EQUIV(fix_gamma_g, src.fix_gamma_g) &&
		EQUIV(fix_gamma_b, src.fix_gamma_b) &&
		EQUIV(fix_coef1, src.fix_coef1) &&
		EQUIV(fix_coef2, src.fix_coef2) &&
		min_row == src.min_row &&
		max_row == src.max_row &&
		min_col == src.min_col &&
		max_col == src.max_col);
}

void C41Config::interpolate(C41Config &prev, C41Config &next,
		long prev_frame, long next_frame, long current_frame)
{
	active = prev.active;
	compute_magic = prev.compute_magic;
	postproc = prev.postproc;
	show_box = prev.show_box;

	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	fix_min_r = prev.fix_min_r * prev_scale + next.fix_min_r * next_scale;
	fix_min_g = prev.fix_min_g * prev_scale + next.fix_min_g * next_scale;
	fix_min_b = prev.fix_min_b * prev_scale + next.fix_min_b * next_scale;
	fix_light = prev.fix_light * prev_scale + next.fix_light * next_scale;
	fix_gamma_g = prev.fix_gamma_g * prev_scale + next.fix_gamma_g * next_scale;
	fix_gamma_b = prev.fix_gamma_b * prev_scale + next.fix_gamma_b * next_scale;
	fix_coef1 = prev.fix_coef1 * prev_scale + next.fix_coef1 * next_scale;
	fix_coef2 = prev.fix_coef2 * prev_scale + next.fix_coef2 * next_scale;
	min_row = round(prev.min_row * prev_scale + next.min_row * next_scale);
	min_col = round(prev.min_col * prev_scale + next.min_col * next_scale);
	max_row = round(prev.max_row * prev_scale + next.max_row * next_scale);
	max_col = round(prev.max_col * prev_scale + next.max_col * next_scale);
}

// C41Enable
C41Enable::C41Enable(C41Effect *plugin, int *output, int x, int y, const char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->plugin = plugin;
	this->output = output;
}

int C41Enable::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

// C41TextBox
C41TextBox::C41TextBox(C41Effect *plugin, float *value, int x, int y)
 : BC_TextBox(x, y, 160, 1, *value)
{
	this->plugin = plugin;
	this->boxValue = value;
}

int C41TextBox::handle_event()
{
	*boxValue = atof(get_text());
	plugin->send_configure_change();
	return 1;
}


// C41Button
C41Button::C41Button(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Apply values"))
{
	this->plugin = plugin;
	this->window = window;
}

int C41Button::handle_event()
{
	plugin->config.fix_min_r = plugin->values.min_r;
	plugin->config.fix_min_g = plugin->values.min_g;
	plugin->config.fix_min_b = plugin->values.min_b;
	plugin->config.fix_light = plugin->values.light;
	plugin->config.fix_gamma_g = plugin->values.gamma_g;
	plugin->config.fix_gamma_b = plugin->values.gamma_b;
	if( plugin->values.coef1 > 0 )
		plugin->config.fix_coef1 = plugin->values.coef1;
	if( plugin->values.coef2 > 0 )
		plugin->config.fix_coef2 = plugin->values.coef2;

	window->update();

	plugin->send_configure_change();
	return 1;
}


C41BoxButton::C41BoxButton(C41Effect *plugin, C41Window *window, int x, int y)
 : BC_GenericButton(x, y, _("Apply default box"))
{
	this->plugin = plugin;
	this->window = window;
}

int C41BoxButton::handle_event()
{
	plugin->config.min_row = plugin->values.shave_min_row;
	plugin->config.max_row = plugin->values.shave_max_row;
	plugin->config.min_col = plugin->values.shave_min_col;
	plugin->config.max_col = plugin->values.shave_max_col;

	window->update();

	plugin->send_configure_change();
	return 1;
}


C41Slider::C41Slider(C41Effect *plugin, int *output, int x, int y, int is_row)
 : BC_ISlider(x, y, 0, 200, 200, 0, is_row ?
	plugin->get_edlsession()->output_h :
	plugin->get_edlsession()->output_w , *output)
{
	this->plugin = plugin;
	this->output = output;
	this->is_row = is_row;
	EDLSession *session = plugin->get_edlsession();
	this->max = is_row ? session->output_h : session->output_w;
}

int C41Slider::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}

int C41Slider::update(int v)
{
	EDLSession *session = plugin->get_edlsession();
	int max = is_row ? session->output_h : session->output_w;
	bclamp(v, 0, max);
	if( this->max != max ) return BC_ISlider::update(200, v, 0, this->max = max);
	if( v != get_value() ) return BC_ISlider::update(v);
	return 1;
}

const char* C41Effect::plugin_title() { return N_("C41"); }
int C41Effect::is_realtime() { return 1; }
int C41Effect::is_synthesis() { return 1; }

NEW_WINDOW_MACRO(C41Effect, C41Window);
LOAD_CONFIGURATION_MACRO(C41Effect, C41Config)

// C41Window
C41Window::C41Window(C41Effect *plugin)
 : PluginClientWindow(plugin,
	plugin->config.window_w, plugin->config.window_h,
	500, 510, 1)
{
}

void C41Window::create_objects()
{
	int x = 10, y = 10;
	C41Effect *plugin = (C41Effect *)client;
	add_subwindow(active = new C41Enable(plugin,
		&plugin->config.active, x, y, _("Activate processing")));
	y += 40;

	add_subwindow(compute_magic = new C41Enable(plugin,
		&plugin->config.compute_magic, x, y, _("Compute negfix values")));
	y += 20;

	add_subwindow(new BC_Title(x + 20, y, _("(uncheck for faster rendering)")));
	y += 40;

	add_subwindow(new BC_Title(x, y, _("Computed negfix values:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min/Max R:")));
	add_subwindow(min_r = new BC_Title(x + 80, y, "0.0000 / 0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min/Max G:")));
	add_subwindow(min_g = new BC_Title(x + 80, y, "0.0000 / 0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min/Max B:")));
	add_subwindow(min_b = new BC_Title(x + 80, y, "0.0000 / 0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(light = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(gamma_g = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(gamma_b = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Contrast:")));
	add_subwindow(coef1 = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Brightness:")));
	add_subwindow(coef2 = new BC_Title(x + 80, y, "0.0000"));
	y += 30;

	add_subwindow(lock = new C41Button(plugin, this, x, y));
	y += 30;

#define BOX_COL 120
	add_subwindow(new BC_Title(x, y, _("Box col:")));
	add_subwindow(box_col_min = new BC_Title(x + 80, y, "0"));
	add_subwindow(box_col_max = new BC_Title(x + BOX_COL, y, "0"));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Box row:")));
	add_subwindow(box_row_min = new BC_Title(x + 80, y, "0"));
	add_subwindow(box_row_max = new BC_Title(x + BOX_COL, y, "0"));
	y += 30;

	add_subwindow(boxlock = new C41BoxButton(plugin, this, x, y));

	y = 10;
	x = 250;
	add_subwindow(show_box = new C41Enable(plugin,
		&plugin->config.show_box, x, y, _("Show active area")));
	y += 40;

	add_subwindow(postproc = new C41Enable(plugin,
		&plugin->config.postproc, x, y, _("Postprocess")));
	y += 60;

	add_subwindow(new BC_Title(x, y, _("negfix values to apply:")));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min R:")));
	add_subwindow(fix_min_r = new C41TextBox(plugin,
		&plugin->config.fix_min_r, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min G:")));
	add_subwindow(fix_min_g = new C41TextBox(plugin,
		&plugin->config.fix_min_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Min B:")));
	add_subwindow(fix_min_b = new C41TextBox(plugin,
		&plugin->config.fix_min_b, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Light:")));
	add_subwindow(fix_light = new C41TextBox(plugin,
		 &plugin->config.fix_light, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma G:")));
	add_subwindow(fix_gamma_g = new C41TextBox(plugin,
		 &plugin->config.fix_gamma_g, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Gamma B:")));
	add_subwindow(fix_gamma_b = new C41TextBox(plugin,
		 &plugin->config.fix_gamma_b, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Contrast:")));
	add_subwindow(fix_coef1 = new C41TextBox(plugin,
		 &plugin->config.fix_coef1, x + 80, y));
	y += 30;

	add_subwindow(new BC_Title(x, y, _("Brightness:")));
	add_subwindow(fix_coef2 = new C41TextBox(plugin,
		 &plugin->config.fix_coef2, x + 80, y));
	y += 30;

	x += 40;
	add_subwindow(new BC_Title(x - 40, y, _("Col:")));
	add_subwindow(min_col = new C41Slider(plugin,
		 &plugin->config.min_col, x, y, 0));
	y += 25;

	add_subwindow(max_col = new C41Slider(plugin,
		 &plugin->config.max_col, x, y, 0));
	y += 25;

	add_subwindow(new BC_Title(x - 40, y, _("Row:")));
	add_subwindow(min_row = new C41Slider(plugin,
		 &plugin->config.min_row, x, y, 1));
	y += 25;

	add_subwindow(max_row = new C41Slider(plugin,
		 &plugin->config.max_row, x, y, 1));
	y += 25;

	update_magic();
	show_window(1);
}

void C41Window::update()
{
	C41Effect *plugin = (C41Effect *)client;
	// Updating the GUI itself
	active->update(plugin->config.active);
	compute_magic->update(plugin->config.compute_magic);
	postproc->update(plugin->config.postproc);
	show_box->update(plugin->config.show_box);

	fix_min_r->update(plugin->config.fix_min_r);
	fix_min_g->update(plugin->config.fix_min_g);
	fix_min_b->update(plugin->config.fix_min_b);
	fix_light->update(plugin->config.fix_light);
	fix_gamma_g->update(plugin->config.fix_gamma_g);
	fix_gamma_b->update(plugin->config.fix_gamma_b);
	fix_coef1->update(plugin->config.fix_coef1);
	fix_coef2->update(plugin->config.fix_coef2);

	min_row->update(plugin->config.min_row);
	max_row->update(plugin->config.max_row);
	min_col->update(plugin->config.min_col);
	max_col->update(plugin->config.max_col);

	update_magic();
}

void C41Window::update_magic()
{
	C41Effect *plugin = (C41Effect *)client;
	char text[BCSTRLEN];
	sprintf(text, "%0.4f / %0.4f", plugin->values.min_r, plugin->values.max_r);
	min_r->update(text);
	sprintf(text, "%0.4f / %0.4f", plugin->values.min_g, plugin->values.max_g);
	min_g->update(text);
	sprintf(text, "%0.4f / %0.4f", plugin->values.min_b, plugin->values.max_b);
	min_b->update(text);
	light->update(plugin->values.light);
	gamma_g->update(plugin->values.gamma_g);
	gamma_b->update(plugin->values.gamma_b);
	// Avoid blinking
	if( plugin->values.coef1 > 0 || plugin->values.coef2 > 0 ) {
		coef1->update(plugin->values.coef1);
		coef2->update(plugin->values.coef2);
	}
	sprintf(text, "%d", plugin->values.shave_min_col);
	box_col_min->update(text);
	sprintf(text, "%d", plugin->values.shave_max_col);
	box_col_max->update(text);
	sprintf(text, "%d", plugin->values.shave_min_row);
	box_row_min->update(text);
	sprintf(text, "%d", plugin->values.shave_max_row);
	box_row_max->update(text);
}


// C41Effect
C41Effect::C41Effect(PluginServer *server)
: PluginVClient(server)
{
	frame = 0;
	tmp_frame = 0;
	blurry_frame = 0;
	memset(&values, 0, sizeof(values));
	shave_min_row = shave_max_row = 0;
	shave_min_col = shave_max_col = 0;
	min_col = max_col = 0;
	min_row = max_row = 0;
	pix_max = 0;  pix_len = 0;
}

C41Effect::~C41Effect()
{
	delete frame;
	delete tmp_frame;
	delete blurry_frame;
}

void C41Effect::render_gui(void* data)
{
	// Updating values computed by process_frame
	struct magic *vp = (struct magic *)data;
	values = *vp;

	if( thread ) {
		C41Window *window = (C41Window *) thread->window;
		window->lock_window("C41Effect::render_gui");
		window->update_magic();
		window->unlock_window();
	}
}

void C41Effect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("C41");
	output.tag.set_property("ACTIVE", config.active);
	output.tag.set_property("COMPUTE_MAGIC", config.compute_magic);
	output.tag.set_property("POSTPROC", config.postproc);
	output.tag.set_property("SHOW_BOX", config.show_box);

	output.tag.set_property("FIX_MIN_R", config.fix_min_r);
	output.tag.set_property("FIX_MIN_G", config.fix_min_g);
	output.tag.set_property("FIX_MIN_B", config.fix_min_b);
	output.tag.set_property("FIX_LIGHT", config.fix_light);
	output.tag.set_property("FIX_GAMMA_G", config.fix_gamma_g);
	output.tag.set_property("FIX_GAMMA_B", config.fix_gamma_b);
	output.tag.set_property("FIX_COEF1", config.fix_coef1);
	output.tag.set_property("FIX_COEF2", config.fix_coef2);
	output.tag.set_property("MIN_ROW", config.min_row);
	output.tag.set_property("MAX_ROW", config.max_row);
	output.tag.set_property("MIN_COL", config.min_col);
	output.tag.set_property("MAX_COL", config.max_col);
	output.append_tag();
	output.tag.set_title("/C41");
	output.append_tag();
	output.terminate_string();
}

void C41Effect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if( input.tag.title_is("C41") ) {
			config.active = input.tag.get_property("ACTIVE", config.active);
			config.compute_magic = input.tag.get_property("COMPUTE_MAGIC", config.compute_magic);
			config.postproc = input.tag.get_property("POSTPROC", config.postproc);
			config.show_box = input.tag.get_property("SHOW_BOX", config.show_box);
			config.fix_min_r = input.tag.get_property("FIX_MIN_R", config.fix_min_r);
			config.fix_min_g = input.tag.get_property("FIX_MIN_G", config.fix_min_g);
			config.fix_min_b = input.tag.get_property("FIX_MIN_B", config.fix_min_b);
			config.fix_light = input.tag.get_property("FIX_LIGHT", config.fix_light);
			config.fix_gamma_g = input.tag.get_property("FIX_GAMMA_G", config.fix_gamma_g);
			config.fix_gamma_b = input.tag.get_property("FIX_GAMMA_B", config.fix_gamma_b);
			config.fix_coef1 = input.tag.get_property("FIX_COEF1", config.fix_coef2);
			config.fix_coef2 = input.tag.get_property("FIX_COEF2", config.fix_coef2);
			config.min_row = input.tag.get_property("MIN_ROW", config.min_row);
			config.max_row = input.tag.get_property("MAX_ROW", config.max_row);
			config.min_col = input.tag.get_property("MIN_COL", config.min_col);
			config.max_col = input.tag.get_property("MAX_COL", config.max_col);
		}
	}
}

#if defined(C41_FAST_POW)

float C41Effect::myLog2(float i)
{
	float x, y;
	float LogBodge = 0.346607f;
	x = *(int *)&i;
	x *= 1.0 / (1 << 23); // 1/pow(2,23);
	x = x - 127;

	y = x - floorf(x);
	y = (y - y * y) * LogBodge;
	return x + y;
}

float C41Effect::myPow2(float i)
{
	float PowBodge = 0.33971f;
	float x;
	float y = i - floorf(i);
	y = (y - y * y) * PowBodge;

	x = i + 127 - y;
	x *= (1 << 23);
	*(int*)&x = (int)x;
	return x;
}

float C41Effect::myPow(float a, float b)
{
	return myPow2(b * myLog2(a));
}

#endif

void C41Effect::pix_fix(float &pix, float min, float gamma)
{
	float val = pix;
	if( val >= 1e-3 ) {
		val = min / val;
		if( gamma != 1 ) val = POWF(val, gamma);
		bclamp(val -= config.fix_light, 0, pix_max);
	        if( config.postproc )
			val = config.fix_coef1 * val + config.fix_coef2;
	}
	else
		val = 1;
	pix = val;
}

int C41Effect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

	int frame_w = input->get_w(), frame_h = input->get_h();
	int input_model = input->get_color_model();
	int has_alpha = BC_CModels::has_alpha(input_model);
	int color_model = has_alpha ? BC_RGBA_FLOAT : BC_RGB_FLOAT;

	if( input_model != color_model ) {
		if( frame &&
		    ( frame->get_w() != frame_w || frame->get_h() != frame_h ||
		      frame->get_color_model() != color_model ) ) {
			delete frame;  frame = 0;
		}
		if( !frame )
			frame = new VFrame(frame_w, frame_h, color_model);
		frame->transfer_from(input);
	}
	else {
		delete frame;
		frame = input;
	}

	pix_max = BC_CModels::calculate_max(color_model);
	pix_len = BC_CModels::components(color_model);

	min_row = config.min_row;  bclamp(min_row, 0,frame_h);
	max_row = config.max_row;  bclamp(max_row, 0,frame_h);
	min_col = config.min_col;  bclamp(min_col, 0,frame_w);
	max_col = config.max_col;  bclamp(max_col, 0,frame_w);

	if( config.compute_magic ) {
		// Convert frame to RGB for magic computing
		if( tmp_frame &&
		    (tmp_frame->get_w() != frame_w || tmp_frame->get_h() != frame_h) ) {
			delete tmp_frame;  tmp_frame = 0;
		}
		if( !tmp_frame )
			tmp_frame = new VFrame(frame_w, frame_h, BC_RGB_FLOAT);
		if( blurry_frame &&
		    (blurry_frame->get_w() != frame_w || blurry_frame->get_h() != frame_h) ) {
			delete blurry_frame;  blurry_frame = 0;
		}
		if( !blurry_frame )
			blurry_frame = new VFrame(frame_w, frame_h, BC_RGB_FLOAT);

		float **rows = (float **)frame->get_rows();
		float **tmp_rows = (float **)tmp_frame->get_rows();
		float **blurry_rows = (float **)blurry_frame->get_rows();

		for( int i=0; i<frame_h; ++i ) {
			float *row = rows[i], *tmp = tmp_rows[i];
			for( int j=frame_w; --j>=0; row+=pix_len ) {
				*tmp++ = bclip(row[0], 0, pix_max);
				*tmp++ = bclip(row[1], 0, pix_max);
				*tmp++ = bclip(row[2], 0, pix_max);
			}
		}

		// 10 passes of Box blur should be good
		int dx = 5, dy = 5, y_up, y_dn, x_rt, x_lt;
		float **src = tmp_rows, **tgt = blurry_rows;
		for( int pass = 0; pass < 10; pass++ ) {
			for( int y = 0; y < frame_h; y++ ) {
				if( (y_up=y-dy) < 0 ) y_up = 0;
				if( (y_dn=y+dy) >= frame_h ) y_dn = frame_h-1;
				float *up = src[y_up], *dn = src[y_dn], *row = tgt[y];
				for( int x = 0; x < frame_w; ++x ) {
					if( (x_lt=x-dx) < 0 ) x_lt = 0;
					if( (x_rt=x+dx) >= frame_w ) x_lt = frame_w-1;
					float *dn_l = dn + 3*x_lt, *dn_r = dn + 3*x_rt;
					float *up_l = up + 3*x_lt, *up_r = up + 3*x_rt;
					row[0] = (dn_l[0] + dn_r[0] + up_l[0] + up_r[0]) / 4;
					row[1] = (dn_l[1] + dn_r[1] + up_l[1] + up_r[1]) / 4;
					row[2] = (dn_l[2] + dn_r[2] + up_l[2] + up_r[2]) / 4;
					row += 3;
				}
			}
			float **rows = src;  src = tgt;  tgt = rows;
		}

		// Shave image: cut off border areas where min max difference
		// is less than C41_SHAVE_TOLERANCE, starting/ending at C41_SHAVE_MARGIN

		shave_min_row = shave_min_col = 0;
		shave_max_col = frame_w;
		shave_max_row = frame_h;
		int min_w = frame_w * C41_SHAVE_MARGIN;
		int max_w = frame_w * (1-C41_SHAVE_MARGIN);
		int min_h = frame_h * C41_SHAVE_MARGIN;
		int max_h = frame_h * (1-C41_SHAVE_MARGIN);

		float yv_min[frame_h], yv_max[frame_h];
		for( int y=0; y<frame_h; ++y ) {
			yv_min[y] = pix_max;
			yv_max[y] = 0.;
		}

		for( int y = 0; y < frame_h; y++ ) {
			float *row = blurry_rows[y];
			for( int x = 0; x < frame_w; x++, row += 3 ) {
				float yv = (row[0] + row[1] + row[2]) / 3;
				if( yv_min[y] > yv ) yv_min[y] = yv;
				if( yv_max[y] < yv ) yv_max[y] = yv;
			}
		}

		for( shave_min_row = min_h; shave_min_row < max_h; shave_min_row++ )
			if( yv_max[shave_min_row] - yv_min[shave_min_row] > C41_SHAVE_TOLERANCE )
				break;
		for( shave_max_row = max_h - 1; shave_max_row > shave_min_row; shave_max_row-- )
			if( yv_max[shave_max_row] - yv_min[shave_max_row] > C41_SHAVE_TOLERANCE )
				break;
		shave_max_row++;

		float xv_min[frame_w], xv_max[frame_w];
		for( int x=0; x<frame_w; ++x ) {
			xv_min[x] = pix_max;
			xv_max[x] = 0.;
		}

		for( int y = shave_min_row; y < shave_max_row; y++ ) {
			float *row = blurry_rows[y];
			for( int x = 0; x < frame_w; x++, row += 3 ) {
				float xv = (row[0] + row[1] + row[2]) / 3;
				if( xv_min[x] > xv ) xv_min[x] = xv;
				if( xv_max[x] < xv ) xv_max[x] = xv;
			}
		}

		for( shave_min_col = min_w; shave_min_col < max_w; shave_min_col++ )
			if( xv_max[shave_min_col] - xv_min[shave_min_col] > C41_SHAVE_TOLERANCE )
				break;
		for( shave_max_col = max_w - 1; shave_max_col > shave_min_col; shave_max_col-- )
			if( xv_max[shave_max_col] - xv_min[shave_max_col] > C41_SHAVE_TOLERANCE )
				break;
		shave_max_col++;

		// Compute magic negfix values
		float minima_r = 50., minima_g = 50., minima_b = 50.;
		float maxima_r = 0., maxima_g = 0., maxima_b = 0.;

		// Check if config_parameters are usable
		if( config.min_col >= config.max_col || config.min_row >= config.max_row ) {
			min_col = shave_min_col;
			max_col = shave_max_col;
			min_row = shave_min_row;
			max_row = shave_max_row;
		}

		for( int i = min_row; i < max_row; i++ ) {
			float *row = blurry_rows[i] + 3 * min_col;
			for( int j = min_col; j < max_col; j++, row+=3 ) {
				if( row[0] < minima_r ) minima_r = row[0];
				if( row[0] > maxima_r ) maxima_r = row[0];
				if( row[1] < minima_g ) minima_g = row[1];
				if( row[1] > maxima_g ) maxima_g = row[1];
				if( row[2] < minima_b ) minima_b = row[2];
				if( row[2] > maxima_b ) maxima_b = row[2];
			}
		}
		values.min_r = minima_r;  values.max_r = maxima_r;
		values.min_g = minima_g;  values.max_g = maxima_g;
		values.min_b = minima_b;  values.max_b = maxima_b;
		values.light = maxima_r < 1e-6 ? 1.0 : minima_r / maxima_r;
		bclamp(minima_r, 1e-6, 1-1e-6);  bclamp(maxima_r, 1e-6, 1-1e-6);
		bclamp(minima_g, 1e-6, 1-1e-6);  bclamp(maxima_g, 1e-6, 1-1e-6);
		bclamp(minima_b, 1e-6, 1-1e-6);  bclamp(maxima_b, 1e-6, 1-1e-6);
		float log_r = logf(maxima_r / minima_r);
		float log_g = logf(maxima_g / minima_g);
		float log_b = logf(maxima_b / minima_b);
		if( log_g < 1e-6 ) log_g = 1e-6;
		if( log_b < 1e-6 ) log_b = 1e-6;
		values.gamma_g = log_r / log_g;
		values.gamma_b = log_r / log_b;
		values.shave_min_col = shave_min_col;
		values.shave_max_col = shave_max_col;
		values.shave_min_row = shave_min_row;
		values.shave_max_row = shave_max_row;
		values.coef1 = values.coef2 = -1;

		// Update GUI
		send_render_gui(&values);
	}

	if( config.compute_magic ) {
		float minima_r = 50., minima_g = 50., minima_b = 50.;
		float maxima_r = 0., maxima_g = 0., maxima_b = 0.;

		for( int i = min_row; i < max_row; i++ ) {
			float *row = (float*)frame->get_rows()[i];
			row += 3 * min_col;
			for( int j = min_col; j < max_col; j++, row += 3 ) {
				if( row[0] < minima_r ) minima_r = row[0];
				if( row[0] > maxima_r ) maxima_r = row[0];

				if( row[1] < minima_g ) minima_g = row[1];
				if( row[1] > maxima_g ) maxima_g = row[1];

				if( row[2] < minima_b ) minima_b = row[2];
				if( row[2] > maxima_b ) maxima_b = row[2];
			}
		}

		// Calculate postprocessing coeficents
		values.coef2 = minima_r;
		if( minima_g < values.coef2 ) values.coef2 = minima_g;
		if( minima_b < values.coef2 ) values.coef2 = minima_b;
		values.coef1 = maxima_r;
		if( maxima_g > values.coef1 ) values.coef1 = maxima_g;
		if( maxima_b > values.coef1 ) values.coef1 = maxima_b;
		// Try not to overflow RGB601 (235 - 16) / 256 * 0.9
		float den = values.coef1 - values.coef2;
		if( fabs(den) < 1e-6 ) den = 1e-6;
		values.coef1 = 0.770 / den;
		values.coef2 = 0.065 - values.coef2 * values.coef1;
		send_render_gui(&values);
	}

	// Apply the transformation
	if( config.active ) {
		for( int i = 0; i < frame_h; i++ ) {
			float *row = (float*)frame->get_rows()[i];
			for( int j = 0; j < frame_w; j++, row += pix_len ) {
				pix_fix(row[0], config.fix_min_r, 1);
				pix_fix(row[1], config.fix_min_g, config.fix_gamma_g);
				pix_fix(row[2], config.fix_min_b, config.fix_gamma_b);
			}
		}
	}

	if( config.show_box ) {
		EDLSession *session = get_edlsession();
		int line_w = bmax(session->output_w,session->output_h) / 600 + 1;
		for( int k=0; k<line_w; ++k ) {
			float **rows = (float **)frame->get_rows();
			if( min_row < max_row - 1 ) {
				float *row1 = (float *)rows[min_row+k];
				float *row2 = (float *)rows[max_row-k - 1];

				for( int i = 0; i < frame_w; i++ ) {
					for( int j = 0; j < 3; j++ ) {
						row1[j] = pix_max - row1[j];
						row2[j] = pix_max - row2[j];
					}
					if( has_alpha ) {
						row1[3] = pix_max;
						row2[3] = pix_max;
					}
					row1 += pix_len;
					row2 += pix_len;
				}
			}

			if( min_col < max_col - 1 ) {
				int pix1 = pix_len * (min_col+k);
				int pix2 = pix_len * (max_col-k - 1);

				for( int i = 0; i < frame_h; i++ ) {
					float *row1 = rows[i] + pix1;
					float *row2 = rows[i] + pix2;

					for( int j = 0; j < 3; j++ ) {
						row1[j] = pix_max - row1[j];
						row2[j] = pix_max - row2[j];
					}
					if( has_alpha ) {
						row1[3] = pix_max;
						row2[3] = pix_max;
					}
				}
			}
		}
	}

	if( frame != output )
		output->transfer_from(frame);
	if( frame == input )
		frame = 0;

	return 0;
}

