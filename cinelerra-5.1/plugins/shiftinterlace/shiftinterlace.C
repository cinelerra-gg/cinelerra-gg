
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

#include "shiftinterlace.h"





PluginClient* new_plugin(PluginServer *server)
{
	return new ShiftInterlaceMain(server);
}




ShiftInterlaceConfig::ShiftInterlaceConfig()
{
	reset(RESET_ALL);
}

void ShiftInterlaceConfig::reset(int clear)
{
	switch(clear) {
		case RESET_ODD_OFFSET : odd_offset = 0;
			break;
		case RESET_EVEN_OFFSET : even_offset = 0;
			break;
		case RESET_ALL :
		default:
			odd_offset = 0;
			even_offset = 0;
			break;
	}
}


int ShiftInterlaceConfig::equivalent(ShiftInterlaceConfig &that)
{
	return (odd_offset == that.odd_offset &&
		even_offset == that.even_offset);
}

void ShiftInterlaceConfig::copy_from(ShiftInterlaceConfig &that)
{
	odd_offset = that.odd_offset;
	even_offset = that.even_offset;
}

void ShiftInterlaceConfig::interpolate(ShiftInterlaceConfig &prev,
		ShiftInterlaceConfig &next,
		long prev_frame,
		long next_frame,
		long current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	this->odd_offset = (int)(prev.odd_offset * prev_scale + next.odd_offset * next_scale);
	this->even_offset = (int)(prev.even_offset * prev_scale + next.even_offset * next_scale);
}






ShiftInterlaceWindow::ShiftInterlaceWindow(ShiftInterlaceMain *plugin)
 : PluginClientWindow(plugin,
	370,
	110,
	370,
	110,
	0)
{
	this->plugin = plugin;
}


void ShiftInterlaceWindow::create_objects()
{
	int x = 10, y = 10;
	int margin = 30;
	int x1 = 0; int clrBtn_w = 50;

	add_subwindow(new BC_Title(x, y, _("Odd offset:")));
	add_subwindow(odd_offset = new ShiftInterlaceOdd(plugin, x + 90, y));
	x1 = x + 90 + odd_offset->get_w() + 10;
	add_subwindow(odd_offsetClr = new ShiftInterlaceSliderClr(plugin, this, x1, y, clrBtn_w, RESET_ODD_OFFSET));

	y += margin;
	add_subwindow(new BC_Title(x, y, _("Even offset:")));
	add_subwindow(even_offset = new ShiftInterlaceEven(plugin, x + 90, y));
	add_subwindow(even_offsetClr = new ShiftInterlaceSliderClr(plugin, this, x1, y, clrBtn_w, RESET_EVEN_OFFSET));

	y += 40;
	add_subwindow(reset = new ShiftInterlaceReset(plugin, this, x, y));
	show_window();
	flush();
}

// for Reset button
void ShiftInterlaceWindow::update_gui(int clear)
{
	switch(clear) {
		case RESET_ODD_OFFSET : odd_offset->update(plugin->config.odd_offset);
			break;
		case RESET_EVEN_OFFSET : even_offset->update(plugin->config.even_offset);
			break;
		case RESET_ALL :
		default:
			odd_offset->update(plugin->config.odd_offset);
			even_offset->update(plugin->config.even_offset);
			break;
	}
}



ShiftInterlaceOdd::ShiftInterlaceOdd(ShiftInterlaceMain *plugin, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	200,
	200,
	-100,
	100,
	plugin->config.odd_offset)
{
	this->plugin = plugin;
}
int ShiftInterlaceOdd::handle_event()
{
	plugin->config.odd_offset = get_value();
	plugin->send_configure_change();
	return 1;
}




ShiftInterlaceEven::ShiftInterlaceEven(ShiftInterlaceMain *plugin, int x, int y)
 : BC_ISlider(x,
 	y,
	0,
	200,
	200,
	-100,
	100,
	plugin->config.even_offset)
{
	this->plugin = plugin;
}


int ShiftInterlaceEven::handle_event()
{
	plugin->config.even_offset = get_value();
	plugin->send_configure_change();
	return 1;
}




ShiftInterlaceReset::ShiftInterlaceReset(ShiftInterlaceMain *plugin, ShiftInterlaceWindow *gui, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
	this->gui = gui;
}
ShiftInterlaceReset::~ShiftInterlaceReset()
{
}
int ShiftInterlaceReset::handle_event()
{
	plugin->config.reset(RESET_ALL);
	gui->update_gui(RESET_ALL);
	plugin->send_configure_change();
	return 1;
}


ShiftInterlaceSliderClr::ShiftInterlaceSliderClr(ShiftInterlaceMain *plugin, ShiftInterlaceWindow *gui, int x, int y, int w, int clear)
 : BC_GenericButton(x, y, w, _("⌂"))
{
	this->plugin = plugin;
	this->gui = gui;
	this->clear = clear;
}
ShiftInterlaceSliderClr::~ShiftInterlaceSliderClr()
{
}
int ShiftInterlaceSliderClr::handle_event()
{
	// clear==1 ==> Odd slider
	// clear==2 ==> Even slider
	plugin->config.reset(clear);
	gui->update_gui(clear);
	plugin->send_configure_change();
	return 1;
}









ShiftInterlaceMain::ShiftInterlaceMain(PluginServer *server)
 : PluginVClient(server)
{

}

ShiftInterlaceMain::~ShiftInterlaceMain()
{

}


const char* ShiftInterlaceMain::plugin_title() { return N_("ShiftInterlace"); }
int ShiftInterlaceMain::is_realtime() { return 1; }





LOAD_CONFIGURATION_MACRO(ShiftInterlaceMain, ShiftInterlaceConfig)

NEW_WINDOW_MACRO(ShiftInterlaceMain, ShiftInterlaceWindow)



void ShiftInterlaceMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("SHIFTINTERLACE");
	output.tag.set_property("ODD_OFFSET", config.odd_offset);
	output.tag.set_property("EVEN_OFFSET", config.even_offset);
	output.append_tag();
	output.tag.set_title("/SHIFTINTERLACE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
// data is now in *text
}

void ShiftInterlaceMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SHIFTINTERLACE"))
			{
				config.odd_offset = input.tag.get_property("ODD_OFFSET", config.odd_offset);
				config.even_offset = input.tag.get_property("EVEN_OFFSET", config.even_offset);
			}
		}
	}
}

void ShiftInterlaceMain::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		((ShiftInterlaceWindow*)thread->window)->odd_offset->update(config.odd_offset);
		((ShiftInterlaceWindow*)thread->window)->even_offset->update(config.even_offset);
		thread->window->unlock_window();
	}
}


#define SHIFT_ROW_MACRO(components, type, chroma_offset) \
{ \
	type *input_row = (type*)input_frame->get_rows()[row]; \
	type *output_row = (type*)output_frame->get_rows()[row]; \
 \
	if(offset < 0) \
	{ \
		int i, j; \
		for(i = 0, j = -offset;  \
			j < w;  \
			i++, j++) \
		{ \
			output_row[i * components + 0] = input_row[j * components + 0]; \
			output_row[i * components + 1] = input_row[j * components + 1]; \
			output_row[i * components + 2] = input_row[j * components + 2]; \
			if(components == 4) output_row[i * components + 3] = input_row[j * components + 3]; \
		} \
 \
		for( ; i < w; i++) \
		{ \
			output_row[i * components + 0] = 0; \
			output_row[i * components + 1] = chroma_offset; \
			output_row[i * components + 2] = chroma_offset; \
			if(components == 4) output_row[i * components + 3] = 0; \
		} \
	} \
	else \
	{ \
		int i, j; \
		for(i = w - offset - 1, j = w - 1; \
			j >= offset; \
			i--, \
			j--) \
		{ \
			output_row[j * components + 0] = input_row[i * components + 0]; \
			output_row[j * components + 1] = input_row[i * components + 1]; \
			output_row[j * components + 2] = input_row[i * components + 2]; \
			if(components == 4) output_row[j * components + 3] = input_row[i * components + 3]; \
		} \
 \
		for( ; j >= 0; j--) \
		{ \
			output_row[j * components + 0] = 0; \
			output_row[j * components + 1] = chroma_offset; \
			output_row[j * components + 2] = chroma_offset; \
			if(components == 4) output_row[j * components + 3] = 0; \
		} \
	} \
}


void ShiftInterlaceMain::shift_row(VFrame *input_frame,
	VFrame *output_frame,
	int offset,
	int row)
{
	int w = input_frame->get_w();
	switch(input_frame->get_color_model())
	{
		case BC_RGB888:
			SHIFT_ROW_MACRO(3, unsigned char, 0x0)
			break;
		case BC_RGB_FLOAT:
			SHIFT_ROW_MACRO(3, float, 0x0)
			break;
		case BC_YUV888:
			SHIFT_ROW_MACRO(3, unsigned char, 0x80)
			break;
		case BC_RGBA_FLOAT:
			SHIFT_ROW_MACRO(4, float, 0x0)
			break;
		case BC_RGBA8888:
			SHIFT_ROW_MACRO(4, unsigned char, 0x0)
			break;
		case BC_YUVA8888:
			SHIFT_ROW_MACRO(4, unsigned char, 0x80)
			break;
		case BC_RGB161616:
			SHIFT_ROW_MACRO(3, uint16_t, 0x0)
			break;
		case BC_YUV161616:
			SHIFT_ROW_MACRO(3, uint16_t, 0x8000)
			break;
		case BC_RGBA16161616:
			SHIFT_ROW_MACRO(4, uint16_t, 0x0)
			break;
		case BC_YUVA16161616:
			SHIFT_ROW_MACRO(4, uint16_t, 0x8000)
			break;
	}
}

int ShiftInterlaceMain::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();

	int h = input_ptr->get_h();
	for(int i = 0; i < h; i++)
	{
		if(i % 2)
			shift_row(input_ptr, output_ptr, config.even_offset, i);
		else
			shift_row(input_ptr, output_ptr, config.odd_offset, i);
	}

	return 0;
}


