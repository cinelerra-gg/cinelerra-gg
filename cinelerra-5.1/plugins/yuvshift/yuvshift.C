
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

#include "yuvshift.h"



REGISTER_PLUGIN(YUVShiftEffect)






YUVShiftConfig::YUVShiftConfig()
{
	reset(RESET_ALL);
}

void YUVShiftConfig::reset(int clear)
{
	switch(clear) {
		case RESET_Y_DX : y_dx = 0;
			break;
		case RESET_Y_DY : y_dy = 0;
			break;
		case RESET_U_DX : u_dx = 0;
			break;
		case RESET_U_DY : u_dy = 0;
			break;
		case RESET_V_DX : v_dx = 0;
			break;
		case RESET_V_DY : v_dy = 0;
			break;
		case RESET_ALL :
		default:
			y_dx = y_dy = 0;
			u_dx = u_dy = 0;
			v_dx = v_dy = 0;
			break;
	}
}

void YUVShiftConfig::copy_from(YUVShiftConfig &src)
{
	y_dx = src.y_dx;  y_dy = src.y_dy;
	u_dx = src.u_dx;  u_dy = src.u_dy;
	v_dx = src.v_dx;  v_dy = src.v_dy;
}

int YUVShiftConfig::equivalent(YUVShiftConfig &src)
{
	return EQUIV(y_dx, src.y_dx) && EQUIV(u_dx, src.u_dx) && EQUIV(v_dx, src.v_dx) &&
		EQUIV(y_dy, src.y_dy) && EQUIV(u_dy, src.u_dy) && EQUIV(v_dy, src.v_dy);
}

void YUVShiftConfig::interpolate(YUVShiftConfig &prev,
	YUVShiftConfig &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	y_dx = prev.y_dx * prev_scale + next.y_dx * next_scale;
	y_dy = prev.y_dy * prev_scale + next.y_dy * next_scale;
	u_dx = prev.u_dx * prev_scale + next.u_dx * next_scale;
	u_dy = prev.u_dy * prev_scale + next.u_dy * next_scale;
	v_dx = prev.v_dx * prev_scale + next.v_dx * next_scale;
	v_dy = prev.v_dy * prev_scale + next.v_dy * next_scale;
}






#define MAXVALUE 100

YUVShiftLevel::YUVShiftLevel(YUVShiftEffect *plugin, int *output, int x, int y)
 : BC_ISlider(x, y, 0, 200, 200, -MAXVALUE, MAXVALUE, *output)
{
	this->plugin = plugin;
	this->output = output;
}

int YUVShiftLevel::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


YUVShiftReset::YUVShiftReset(YUVShiftEffect *plugin, YUVShiftWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->window = window;
}
YUVShiftReset::~YUVShiftReset()
{
}
int YUVShiftReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	window->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


YUVShiftSliderClr::YUVShiftSliderClr(YUVShiftEffect *plugin, YUVShiftWindow *window, int x, int y, int w, int clear)
 : BC_GenericButton(x, y, w, _("⌂"))
{
	this->plugin = plugin;
	this->window = window;
	this->clear = clear;
}
YUVShiftSliderClr::~YUVShiftSliderClr()
{
}
int YUVShiftSliderClr::handle_event()
{
	// clear==1 ==> y_dx slider --- clear==2 ==> y_dy slider
	// clear==3 ==> u_dx slider --- clear==4 ==> u_dy slider
	// clear==5 ==> v_dx slider --- clear==6 ==> v_dy slider
	plugin->config.reset(clear);
	window->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}


YUVShiftWindow::YUVShiftWindow(YUVShiftEffect *plugin)
 : PluginClientWindow(plugin, 320, 230, 320, 230, 0)
{
	this->plugin = plugin;
}

void YUVShiftWindow::create_objects()
{
	int x = 10, y = 10, x1 = 50;
	int x2 = 0; int clrBtn_w = 50;

	add_subwindow(new BC_Title(x, y, _("Y_dx:")));
	add_subwindow(y_dx = new YUVShiftLevel(plugin, &plugin->config.y_dx, x1, y));
	x2 = x1 + y_dx->get_w() + 10;
	add_subwindow(y_dxClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_Y_DX));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("Y_dy:")));
	add_subwindow(y_dy = new YUVShiftLevel(plugin, &plugin->config.y_dy, x1, y));
	add_subwindow(y_dyClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_Y_DY));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("U_dx:")));
	add_subwindow(u_dx = new YUVShiftLevel(plugin, &plugin->config.u_dx, x1, y));
	add_subwindow(u_dxClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_U_DX));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("U_dy:")));
	add_subwindow(u_dy = new YUVShiftLevel(plugin, &plugin->config.u_dy, x1, y));
	add_subwindow(u_dyClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_U_DY));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("V_dx:")));
	add_subwindow(v_dx = new YUVShiftLevel(plugin, &plugin->config.v_dx, x1, y));
	add_subwindow(v_dxClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_V_DX));

	y += 30;
	add_subwindow(new BC_Title(x, y, _("V_dy:")));
	add_subwindow(v_dy = new YUVShiftLevel(plugin, &plugin->config.v_dy, x1, y));
	add_subwindow(v_dyClr = new YUVShiftSliderClr(plugin, this, x2, y, clrBtn_w, RESET_V_DY));

	y += 40;
	add_subwindow(reset = new YUVShiftReset(plugin, this, x, y));

	show_window();
	flush();
}


// for Reset button
void YUVShiftWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_Y_DX : y_dx->update(plugin->config.y_dx);
			break;
		case RESET_Y_DY : y_dy->update(plugin->config.y_dy);
			break;
		case RESET_U_DX : u_dx->update(plugin->config.u_dx);
			break;
		case RESET_U_DY : u_dy->update(plugin->config.u_dy);
			break;
		case RESET_V_DX : v_dx->update(plugin->config.v_dx);
			break;
		case RESET_V_DY : v_dy->update(plugin->config.v_dy);
			break;
		case RESET_ALL :
		default:
			y_dx->update(plugin->config.y_dx);
			y_dy->update(plugin->config.y_dy);
			u_dx->update(plugin->config.u_dx);
			u_dy->update(plugin->config.u_dy);
			v_dx->update(plugin->config.v_dx);
			v_dy->update(plugin->config.v_dy);
			break;
	}
}






YUVShiftEffect::YUVShiftEffect(PluginServer *server)
 : PluginVClient(server)
{
	temp_frame = 0;
}
YUVShiftEffect::~YUVShiftEffect()
{
	delete temp_frame;
}

const char* YUVShiftEffect::plugin_title() { return N_("YUVShift"); }
int YUVShiftEffect::is_realtime() { return 1; }


NEW_WINDOW_MACRO(YUVShiftEffect, YUVShiftWindow)
LOAD_CONFIGURATION_MACRO(YUVShiftEffect, YUVShiftConfig)

void YUVShiftEffect::update_gui()
{
	if(thread)
	{
		YUVShiftWindow *yuv_wdw = (YUVShiftWindow*)thread->window;
		yuv_wdw->lock_window("YUVShiftEffect::update_gui");
		load_configuration();
		yuv_wdw->y_dx->update(config.y_dx);
		yuv_wdw->y_dy->update(config.y_dy);
		yuv_wdw->u_dx->update(config.u_dx);
		yuv_wdw->u_dy->update(config.u_dy);
		yuv_wdw->v_dx->update(config.v_dx);
		yuv_wdw->v_dy->update(config.v_dy);
		yuv_wdw->unlock_window();
	}
}

void YUVShiftEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("YUVSHIFT");
	output.tag.set_property("Y_DX", config.y_dx);
	output.tag.set_property("Y_DY", config.y_dy);
	output.tag.set_property("U_DX", config.u_dx);
	output.tag.set_property("U_DY", config.u_dy);
	output.tag.set_property("V_DX", config.v_dx);
	output.tag.set_property("V_DY", config.v_dy);
	output.append_tag();
	output.tag.set_title("/YUVSHIFT");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void YUVShiftEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	while(!input.read_tag())
	{
		if(input.tag.title_is("YUVSHIFT"))
		{
			config.y_dx = input.tag.get_property("Y_DX", config.y_dx);
			config.y_dy = input.tag.get_property("Y_DY", config.y_dy);
			config.u_dx = input.tag.get_property("U_DX", config.u_dx);
			config.u_dy = input.tag.get_property("U_DY", config.u_dy);
			config.v_dx = input.tag.get_property("V_DX", config.v_dx);
			config.v_dy = input.tag.get_property("V_DY", config.v_dy);
		}
	}
}


#define YUV_MACRO(type, temp_type, components) \
{ \
	for(int i = 0; i < h; i++) { \
		int yi = i + config.y_dy, ui = i + config.u_dy, vi = i + config.v_dy; \
		type *in_y = yi >= 0 && yi < h ? (type *)frame->get_rows()[yi] : 0; \
		type *in_u = ui >= 0 && ui < h ? (type *)frame->get_rows()[ui] : 0; \
		type *in_v = vi >= 0 && vi < h ? (type *)frame->get_rows()[vi] : 0; \
		type *out_row = (type *)output->get_rows()[i]; \
		for(int j = 0; j < w; j++) { \
			int yj = j + config.y_dx, uj = j + config.u_dx, vj = j + config.v_dx; \
			type *yp = in_y && yj >= 0 && yj < w ? in_y + yj*components: 0; \
			type *up = in_u && uj >= 0 && uj < w ? in_u + uj*components: 0; \
			type *vp = in_v && vj >= 0 && vj < w ? in_v + vj*components: 0; \
			out_row[0] = yp ? yp[0] : 0; \
			out_row[1] = up ? up[1] : (1<<(8*sizeof(type)-1)); \
			out_row[2] = vp ? vp[2] : (1<<(8*sizeof(type)-1)); \
			out_row += components; \
		} \
	} \
}

#define RGB_MACRO(type, temp_type, components) \
{ \
	for(int i = 0; i < h; i++) { \
		int yi = i + config.y_dy, ui = i + config.u_dy, vi = i + config.v_dy; \
		uint8_t *in_y = yi >= 0 && yi < h ? (uint8_t *)frame->get_rows()[yi] : 0; \
		uint8_t *in_u = ui >= 0 && ui < h ? (uint8_t *)frame->get_rows()[ui] : 0; \
		uint8_t *in_v = vi >= 0 && vi < h ? (uint8_t *)frame->get_rows()[vi] : 0; \
		type *out_row = (type *)output->get_rows()[i]; \
		for(int j = 0; j < w; j++) { \
			int yj = j + config.y_dx, uj = j + config.u_dx, vj = j + config.v_dx; \
			uint8_t *yp = in_y && yj >= 0 && yj < w ? in_y + yj*3: 0; \
			uint8_t *up = in_u && uj >= 0 && uj < w ? in_u + uj*3: 0; \
			uint8_t *vp = in_v && vj >= 0 && vj < w ? in_v + vj*3: 0; \
			temp_type r, g, b; \
			temp_type y = yp ? yp[0] : 0x00; \
			temp_type u = up ? up[1] : 0x80; \
			temp_type v = vp ? vp[2] : 0x80; \
			if( sizeof(type) == 4 ) \
				YUV::yuv.yuv_to_rgb_f(r, g, b, y/255., (u-128.)/255., (v-128.)/255.); \
			else if( sizeof(type) == 2 ) \
				YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v); \
			else \
				YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v); \
			out_row[0] = r; \
			out_row[1] = g; \
			out_row[2] = b; \
			out_row += components; \
		} \
	} \
}

int YUVShiftEffect::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();

	if( EQUIV(config.y_dx, 0) && EQUIV(config.u_dx, 0) && EQUIV(config.v_dx, 0) &&
		EQUIV(config.y_dy, 0) && EQUIV(config.u_dy, 0) && EQUIV(config.v_dy, 0) ) {
		if(input->get_rows()[0] != output->get_rows()[0])
			output->copy_from(input);
		return 0;
	}

	int w = input->get_w(), h = input->get_h();
	int color_model = input->get_color_model();
	int is_yuv = BC_CModels::is_yuv(color_model);
	if( !is_yuv ) color_model = BC_YUV888;
	VFrame *frame = input;
	if( input->get_rows()[0] == output->get_rows()[0] || !is_yuv ) {
		if( temp_frame && ( temp_frame->get_color_model() != color_model ||
			temp_frame->get_w() != w || temp_frame->get_h() != h ) ) {
			delete temp_frame;  temp_frame = 0;
		}
		if( !temp_frame )
			temp_frame = new VFrame(w, h, color_model, 0);
		frame = temp_frame;
		if( color_model != input->get_color_model() )
			BC_CModels::transfer(frame->get_rows(), input->get_rows(),
				frame->get_y(), frame->get_u(), frame->get_v(),
				input->get_y(), input->get_u(), input->get_v(),
				0, 0, input->get_w(), input->get_h(),
				0, 0, frame->get_w(), frame->get_h(),
				input->get_color_model(), frame->get_color_model(), 0,
				input->get_bytes_per_line(), w);
		else
			frame->copy_from(input);
	}

	switch( input->get_color_model() ) {
	case BC_YUV888:       YUV_MACRO(unsigned char, int, 3);  break;
	case BC_YUV161616:    YUV_MACRO(uint16_t, int, 3);       break;
	case BC_YUVA8888:     YUV_MACRO(unsigned char, int, 4);  break;
	case BC_YUVA16161616: YUV_MACRO(uint16_t, int, 4);       break;
	case BC_RGB_FLOAT:    RGB_MACRO(float, float, 3);        break;
	case BC_RGB888:       RGB_MACRO(unsigned char, int, 3);  break;
	case BC_RGB161616:    RGB_MACRO(uint16_t, int, 3);       break;
	case BC_RGBA_FLOAT:   RGB_MACRO(float, float, 4);        break;
	case BC_RGBA8888:     RGB_MACRO(unsigned char, int, 4);  break;
	case BC_RGBA16161616: RGB_MACRO(uint16_t, int, 4);       break;
	}

	return 0;
}


