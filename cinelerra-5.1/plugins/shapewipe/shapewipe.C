
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
#include "bchash.h"
#include "cstrdup.h"
#include "edl.inc"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "overlayframe.h"
#include "theme.h"
#include "vframe.h"
#include "shapewipe.h"

#include <png.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#define SHAPE_SEARCHPATH "/shapes"
#define DEFAULT_SHAPE "circle"

REGISTER_PLUGIN(ShapeWipeMain)

ShapeWipeW2B::ShapeWipeW2B(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x,
		y,
		plugin->direction == 0,
		_("White to Black"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeW2B::handle_event()
{
	update(1);
	plugin->direction = 0;
	window->right->update(0);
	plugin->send_configure_change();
	return 0;
}

ShapeWipeB2W::ShapeWipeB2W(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Radial(x,
		y,
		plugin->direction == 1,
		_("Black to White"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeB2W::handle_event()
{
	update(1);
	plugin->direction = 1;
	window->left->update(0);
	plugin->send_configure_change();
	return 0;
}

ShapeWipeAntiAlias::ShapeWipeAntiAlias(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox (x,y,plugin->antialias, _("Anti-aliasing"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipeAntiAlias::handle_event()
{
	plugin->antialias = get_value();
	plugin->send_configure_change();
	return 0;
}

ShapeWipePreserveAspectRatio::ShapeWipePreserveAspectRatio(ShapeWipeMain *plugin,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_CheckBox (x, y, plugin->preserve_aspect, _("Preserve shape aspect ratio"))
{
	this->plugin = plugin;
	this->window = window;
}

int ShapeWipePreserveAspectRatio::handle_event()
{
	plugin->preserve_aspect = get_value();
	plugin->send_configure_change();
	return 0;
}






ShapeWipeTumble::ShapeWipeTumble(ShapeWipeMain *client,
	ShapeWipeWindow *window,
	int x,
	int y)
 : BC_Tumbler(x, y)
{
	this->client = client;
	this->window = window;
}

int ShapeWipeTumble::handle_up_event()
{
	window->prev_shape();
	return 1;
}

int ShapeWipeTumble::handle_down_event()
{
	window->next_shape();
	return 0;
}







ShapeWipeShape::ShapeWipeShape(ShapeWipeMain *client,
	ShapeWipeWindow *window,
	int x,
	int y,
	int text_w,
	int list_h)
 : BC_PopupTextBox(window,
 	&window->shapes,
	client->shape_name,
	x,
	y,
	text_w,
	list_h)
{
	this->client = client;
	this->window = window;
}

int ShapeWipeShape::handle_event()
{
	strcpy(client->shape_name, get_text());
	client->send_configure_change();
	return 1;
}








ShapeWipeWindow::ShapeWipeWindow(ShapeWipeMain *plugin)
 : PluginClientWindow(plugin,
	450,
	125,
	450,
	125,
	0)
{
	this->plugin = plugin;
}

ShapeWipeWindow::~ShapeWipeWindow()
{
	shapes.remove_all_objects();
}



void ShapeWipeWindow::create_objects()
{
	BC_Title *title = 0;
	lock_window("ShapeWipeWindow::create_objects");
	int widget_border = plugin->get_theme()->widget_border;
	int window_border = plugin->get_theme()->window_border;
	int x = window_border, y = window_border;

	plugin->init_shapes();
	for(int i = 0; i < plugin->shape_titles.size(); i++)
	{
		shapes.append(new BC_ListBoxItem(plugin->shape_titles.get(i)));
	}

	add_subwindow(title = new BC_Title(x, y, _("Direction:")));
	x += title->get_w() + widget_border;
	add_subwindow(left = new ShapeWipeW2B(plugin,
		this,
		x,
		y));
	x += left->get_w() + widget_border;
	add_subwindow(right = new ShapeWipeB2W(plugin,
		this,
		x,
		y));
	x = window_border;
	y += right->get_h() + widget_border;


	add_subwindow(title = new BC_Title(x, y, _("Shape:")));
	x += title->get_w() + widget_border;

// 	add_subwindow(filename_widget = new
// 	ShapeWipeFilename(plugin,
// 		this,
// 		plugin->filename,
// 		x,
// 		y));
// 	x += 200;
// 	add_subwindow(new ShapeWipeBrowseButton(
// 		plugin,
// 		this,
// 		filename_widget,
// 		x,
// 		y));

	shape_text = new ShapeWipeShape(plugin,
		this,
		x,
		y,
		150,
		200);
	shape_text->create_objects();
	x += shape_text->get_w() + widget_border;
	add_subwindow(new ShapeWipeTumble(plugin,
		this,
		x,
		y));
	x = window_border;
	y += shape_text->get_h() + widget_border;

	ShapeWipeAntiAlias *anti_alias;
	add_subwindow(anti_alias = new ShapeWipeAntiAlias(
		plugin,
		this,
		x,
		y));
	y += anti_alias->get_h() + widget_border;
	ShapeWipePreserveAspectRatio *aspect_ratio;
	add_subwindow(aspect_ratio = new ShapeWipePreserveAspectRatio(
		plugin,
		this,
		x,
		y));
	y += aspect_ratio->get_h() + widget_border;

	show_window();
	unlock_window();
}

void ShapeWipeWindow::next_shape()
{
	for(int i = 0; i < plugin->shape_titles.size(); i++)
	{
		if(!strcmp(plugin->shape_titles.get(i), plugin->shape_name))
		{
			i++;
			if(i >= plugin->shape_titles.size()) i = 0;
			strcpy(plugin->shape_name, plugin->shape_titles.get(i));
			shape_text->update(plugin->shape_name);
			break;
		}
	}
	client->send_configure_change();
}

void ShapeWipeWindow::prev_shape()
{
	for(int i = 0; i < plugin->shape_titles.size(); i++)
	{
		if(!strcmp(plugin->shape_titles.get(i), plugin->shape_name))
		{
			i--;
			if(i < 0) i = plugin->shape_titles.size() - 1;
			strcpy(plugin->shape_name, plugin->shape_titles.get(i));
			shape_text->update(plugin->shape_name);
			break;
		}
	}
	client->send_configure_change();
}





ShapeWipeMain::ShapeWipeMain(PluginServer *server)
 : PluginVClient(server)
{
	direction = 0;
	filename[0] = 0;
	last_read_filename[0] = '\0';
	strcpy(shape_name, DEFAULT_SHAPE);
	current_name[0] = 0;
	pattern_image = NULL;
	min_value = (unsigned char)255;
	max_value = (unsigned char)0;
	antialias = 0;
	preserve_aspect = 0;
	last_preserve_aspect = 0;
	shapes_initialized = 0;
	shape_paths.set_array_delete();
	shape_titles.set_array_delete();
}

ShapeWipeMain::~ShapeWipeMain()
{
	reset_pattern_image();
	shape_paths.remove_all_objects();
	shape_titles.remove_all_objects();
}

const char* ShapeWipeMain::plugin_title() { return N_("Shape Wipe"); }
int ShapeWipeMain::is_transition() { return 1; }
int ShapeWipeMain::uses_gui() { return 1; }

NEW_WINDOW_MACRO(ShapeWipeMain, ShapeWipeWindow);



void ShapeWipeMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("SHAPEWIPE");
	output.tag.set_property("DIRECTION", direction);
	output.tag.set_property("ANTIALIAS", antialias);
	output.tag.set_property("PRESERVE_ASPECT", preserve_aspect);
	output.tag.set_property("FILENAME", filename);
	output.tag.set_property("SHAPE_NAME", shape_name);
	output.append_tag();
	output.tag.set_title("/SHAPEWIPE");
	output.append_tag();
	output.terminate_string();
}

void ShapeWipeMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("SHAPEWIPE"))
		{
			direction = input.tag.get_property("DIRECTION", direction);
			antialias = input.tag.get_property("ANTIALIAS", antialias);
			preserve_aspect = input.tag.get_property("PRESERVE_ASPECT", preserve_aspect);
			input.tag.get_property("FILENAME", filename);
			input.tag.get_property("SHAPE_NAME", shape_name);
		}
	}
}

void ShapeWipeMain::init_shapes()
{
	if(!shapes_initialized)
	{
		FileSystem fs;
		fs.set_filter("*.png");
		char shape_path[BCTEXTLEN];
		sprintf(shape_path, "%s%s", get_plugin_dir(), SHAPE_SEARCHPATH);
		fs.update(shape_path);

		for(int i = 0; i < fs.total_files(); i++)
		{
			FileItem *file_item = fs.get_entry(i);
			if(!file_item->get_is_dir())
			{
				shape_paths.append(cstrdup(file_item->get_path()));
				char *ptr = cstrdup(file_item->get_name());
				char *ptr2 = strrchr(ptr, '.');
				if(ptr2) *ptr2 = 0;
				shape_titles.append(ptr);
			}
		}

		shapes_initialized = 1;
	}
}


int ShapeWipeMain::load_configuration()
{
	read_data(get_prev_keyframe(get_source_position()));
	return 1;
}

int ShapeWipeMain::read_pattern_image(int new_frame_width, int new_frame_height)
{
	png_byte header[8];
	int is_png;
	int row;
	int col;
	int scaled_row;
	int scaled_col;
	int pixel_width;
	unsigned char value;
	png_uint_32 width;
	png_uint_32 height;
	png_byte color_type;
	png_byte bit_depth;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_bytep *image;
	frame_width = new_frame_width;
	frame_height = new_frame_height;

// Convert name to filename
	for(int i = 0; i < shape_paths.size(); i++)
	{
		if(!strcmp(shape_titles.get(i), shape_name))
		{
			strcpy(filename, shape_paths.get(i));
			break;
		}
	}

	FILE *fp = fopen(filename, "rb");
	if (!fp)
	{
		return 1;
	}

	fread(header, 1, 8, fp);
	is_png = !png_sig_cmp(header, 0, 8);

	if (!is_png)
	{
		fclose(fp);
		return 1;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL);

	if (!png_ptr)
	{
		fclose(fp);
		return 1;
	}

	/* Tell libpng we already checked the first 8 bytes */
	png_set_sig_bytes(png_ptr, 8);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return 1;
	}

	end_info = png_create_info_struct(png_ptr);
	if (!end_info)
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return 1;
	}

	png_init_io(png_ptr, fp);
	png_read_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	width  = png_get_image_width (png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);

	/* Skip the alpha channel if present
	* stripping alpha currently doesn't work in conjunction with
	* converting to grayscale in libpng */
	if (color_type & PNG_COLOR_MASK_ALPHA)
		pixel_width = 2;
	else
		pixel_width = 1;

	/* Convert 16 bit data to 8 bit */
	if (bit_depth == 16) png_set_strip_16(png_ptr);

	/* Expand to 1 pixel per byte if necessary */
	if (bit_depth < 8) png_set_packing(png_ptr);

	/* Convert to grayscale */
	if (color_type == PNG_COLOR_TYPE_RGB ||
		color_type == PNG_COLOR_TYPE_RGB_ALPHA)
	png_set_rgb_to_gray_fixed(png_ptr, 1, -1, -1);

	/* Allocate memory to hold the original png image */
	image = (png_bytep*)malloc(sizeof(png_bytep)*height);
	for (row = 0; row < (int)height; row++)
	{
		image[row] = (png_byte*)malloc(sizeof(png_byte)*width*pixel_width);
	}

	/* Allocate memory for the pattern image that will actually be
	* used for the wipe */
	pattern_image = (unsigned char**)malloc(sizeof(unsigned char*)*frame_height);


	png_read_image(png_ptr, image);
	png_read_end(png_ptr, end_info);

	double row_factor, col_factor;
	double row_offset = 0.5, col_offset = 0.5;	// for rounding

	if (preserve_aspect && aspect_w != 0 && aspect_h != 0)
	{
		row_factor = (height-1)/aspect_h;
		col_factor = (width-1)/aspect_w;
		if (row_factor < col_factor)
			col_factor = row_factor;
		else
			row_factor = col_factor;
		row_factor *= aspect_h/(double)(frame_height-1);
		col_factor *= aspect_w/(double)(frame_width-1);

		// center the pattern over the frame
		row_offset += (height-1-(frame_height-1)*row_factor)/2;
		col_offset += (width-1-(frame_width-1)*col_factor)/2;
	}
	else
	{
		// Stretch (or shrink) the pattern image to fill the frame
		row_factor = (double)(height-1)/(double)(frame_height-1);
		col_factor = (double)(width-1)/(double)(frame_width-1);
	}

	for (scaled_row = 0; scaled_row < frame_height; scaled_row++)
	{
		row = (int)(row_factor*scaled_row + row_offset);
		pattern_image[scaled_row] = (unsigned char*)malloc(sizeof(unsigned char)*frame_width);
		for (scaled_col = 0; scaled_col < frame_width; scaled_col++)
		{
			col = (int)(col_factor*scaled_col + col_offset)*pixel_width;
			value = image[row][col];
			pattern_image[scaled_row][scaled_col] = value;
			if (value < min_value) min_value = value;
			if (value > max_value) max_value = value;

		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	fclose(fp);
	/* Deallocate the original image as it is no longer needed */
	for (row = 0; row < (int)height; row++)
	{
		free(image[row]);
	}
	free (image);
	return 0;
}

void ShapeWipeMain::reset_pattern_image()
{
	int row;
	if (pattern_image != NULL)
	{
		for (row = 0; row < frame_height; row++)
		{
			free (pattern_image[row]);
		}
		free (pattern_image);
		pattern_image = NULL;
		min_value = (unsigned char)255;
		max_value = (unsigned char)0;	// are recalc'd in read_pattern_image
	}
}

#define SHAPEWIPE(type, components) \
{ \
\
	type  **in_rows = (type**)incoming->get_rows(); \
	type **out_rows = (type**)outgoing->get_rows(); \
	\
	type *in_row; \
	type *out_row; \
	\
	if( !direction ) { \
		for(j = 0; j < h; j++) { \
			in_row = (type*) in_rows[j]; \
			out_row = (type*)out_rows[j]; \
			pattern_row = pattern_image[j]; \
			\
			col_offset = 0; \
			for(k = 0; k < w; k++, col_offset += components ) { \
				value = pattern_row[k]; \
				if (value < threshold) continue; \
				out_row[col_offset]     = in_row[col_offset]; \
				out_row[col_offset + 1] = in_row[col_offset + 1]; \
				out_row[col_offset + 2] = in_row[col_offset + 2]; \
				if(components == 4) \
					out_row[col_offset + 3] = in_row[col_offset + 3]; \
			} \
		} \
	} \
	else { \
		for(j = 0; j < h; j++) { \
			in_row = (type*) in_rows[j]; \
			out_row = (type*)out_rows[j]; \
			pattern_row = pattern_image[j]; \
			\
			col_offset = 0; \
			for(k = 0; k < w; k++, col_offset += components ) { \
				value = pattern_row[k]; \
				if (value > threshold) continue; \
				out_row[col_offset]     = in_row[col_offset]; \
				out_row[col_offset + 1] = in_row[col_offset + 1]; \
				out_row[col_offset + 2] = in_row[col_offset + 2]; \
				if(components == 4) \
					out_row[col_offset + 3] = in_row[col_offset + 3]; \
			} \
		} \
	} \
}

#define COMPARE1(x,y) \
{ \
	if (pattern_image[x][y] <= threshold) opacity++; \
}

#define COMPARE2(x,y) \
{ \
	if (pattern_image[x][y] >= threshold) opacity++; \
}

// components is always 4
#define BLEND_ONLY_4_NORMAL(temp_type, type, max, chroma_offset,x,y) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type blend_opacity = (temp_type)(alpha * ((temp_type)1 << bits) + 0.5); \
	temp_type blend_transparency = ((temp_type)1 << bits) - blend_opacity; \
 \
	col = y * 4; \
	type* in_row = (type*)incoming->get_rows()[x]; \
	type* output = (type*)outgoing->get_rows()[x]; \
 \
	output[col] = ((temp_type)in_row[col] * blend_opacity + output[col] * blend_transparency) >> bits; \
	output[col+1] = ((temp_type)in_row[col+1] * blend_opacity + output[col+1] * blend_transparency) >> bits; \
	output[col+2] = ((temp_type)in_row[col+2] * blend_opacity + output[col+2] * blend_transparency) >> bits; \
}


// components is always 3
#define BLEND_ONLY_3_NORMAL(temp_type, type, max, chroma_offset,x,y) \
{ \
	const int bits = sizeof(type) * 8; \
	temp_type blend_opacity = (temp_type)(alpha * ((temp_type)1 << bits) + 0.5); \
	temp_type blend_transparency = ((temp_type)1 << bits) - blend_opacity; \
 \
	col = y * 3; \
	type* in_row = (type*)incoming->get_rows()[x]; \
	type* output = (type*)outgoing->get_rows()[x]; \
 \
	output[col] = ((temp_type)in_row[col] * blend_opacity + output[col] * blend_transparency) >> bits; \
	output[col+1] = ((temp_type)in_row[col+1] * blend_opacity + output[col+1] * blend_transparency) >> bits; \
	output[col+2] = ((temp_type)in_row[col+2] * blend_opacity + output[col+2] * blend_transparency) >> bits; \
}

/* opacity is defined as opacity of incoming frame */
#define BLEND(x,y,total) \
{ \
	float pixel_opacity = (float)opacity / total; \
	float alpha = pixel_opacity; \
	float pixel_transparency = 1.0 - pixel_opacity; \
	int col; \
 \
	if (pixel_opacity > 0.0) \
	{ \
		switch(incoming->get_color_model()) \
		{ \
		case BC_RGB_FLOAT: \
		{ \
			float  *in_row = (float*)incoming->get_rows()[x]; \
			float *out_row = (float*)outgoing->get_rows()[x]; \
			col = y * 3; \
			out_row[col] = in_row[col] * pixel_opacity + \
				out_row[col] * pixel_transparency; \
			out_row[col+1] = in_row[col+1] * pixel_opacity + \
				out_row[col+1] * pixel_transparency; \
			out_row[col+2] = in_row[col+2] * pixel_opacity + \
				out_row[col+2] * pixel_transparency; \
			break; \
		} \
		case BC_RGBA_FLOAT: \
		{ \
			float  *in_row = (float*)incoming->get_rows()[x]; \
			float *out_row = (float*)outgoing->get_rows()[x]; \
			col = y * 4; \
			out_row[col] = in_row[col] * pixel_opacity + \
				out_row[col] * pixel_transparency; \
			out_row[col+1] = in_row[col+1] * pixel_opacity + \
				out_row[col+1] * pixel_transparency; \
			out_row[col+2] = in_row[col+2] * pixel_opacity + \
				out_row[col+2] * pixel_transparency; \
			break; \
		} \
		case BC_RGB888: \
			BLEND_ONLY_3_NORMAL(uint32_t, unsigned char, 0xff, 0,x,y); \
			break; \
		case BC_YUV888: \
			BLEND_ONLY_3_NORMAL(int32_t, unsigned char, 0xff, 0x80,x,y); \
			break; \
		case BC_RGBA8888: \
			BLEND_ONLY_4_NORMAL(uint32_t, unsigned char, 0xff, 0,x,y); \
			break; \
		case BC_YUVA8888: \
			BLEND_ONLY_4_NORMAL(int32_t, unsigned char, 0xff, 0x80,x,y); \
			break; \
		case BC_RGB161616: \
			BLEND_ONLY_3_NORMAL(uint64_t, uint16_t, 0xffff, 0,x,y); \
			break; \
		case BC_YUV161616: \
			BLEND_ONLY_3_NORMAL(int64_t, uint16_t, 0xffff, 0x8000,x,y); \
			break; \
		case BC_RGBA16161616: \
			BLEND_ONLY_4_NORMAL(uint64_t, uint16_t, 0xffff, 0,x,y); \
			break; \
		case BC_YUVA16161616: \
			BLEND_ONLY_4_NORMAL(int64_t, uint16_t, 0xffff, 0x8000,x,y); \
			break; \
      } \
   } \
}

int ShapeWipeMain::process_realtime(VFrame *incoming, VFrame *outgoing)
{
	unsigned char *pattern_row;
	int col_offset;
	unsigned char threshold;
	unsigned char value;
	int j, k;
	int opacity;

	init_shapes();
	load_configuration();

	int w = incoming->get_w();
	int h = incoming->get_h();

	if (strncmp(filename, last_read_filename, BCTEXTLEN) ||
		strncmp(shape_name, current_name, BCTEXTLEN) ||
		preserve_aspect != last_preserve_aspect)
	{
		reset_pattern_image();
	}

	if (!pattern_image)
	{
		read_pattern_image(w, h);
		strncpy(last_read_filename, filename, BCTEXTLEN);
		last_preserve_aspect = preserve_aspect;

		if (pattern_image)
		{
			strncpy(last_read_filename, filename, BCTEXTLEN);
			strncpy(current_name, shape_name, BCTEXTLEN);
			last_preserve_aspect = preserve_aspect;
		}
		else {
			fprintf(stderr, _("Shape Wipe: cannot load shape %s\n"), filename);
			last_read_filename[0] = 0;
			return 0;
		}
	}

	if (direction)
	{
		threshold = (unsigned char)(
				(float)PluginClient::get_source_position() /
				(float)PluginClient::get_total_len() *
				(float)(max_value - min_value))
			+ min_value;
	}
	else
	{
		threshold = (unsigned char)((max_value - min_value) - (
				(float)PluginClient::get_source_position() /
				(float)PluginClient::get_total_len() *
				(float)(max_value - min_value)))
			+ min_value;
	}

	if (antialias)
	{
		if (direction)
		{
			/* Top left corner */
			opacity = 0;
			COMPARE1(0,0);
			COMPARE1(0,1);
			COMPARE1(1,0);
			COMPARE1(1,1);
			BLEND(0,0,4.0);

			/* Top edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE1(0,k-1);
				COMPARE1(0,k);
				COMPARE1(0,k+1);
				COMPARE1(1,k-1);
				COMPARE1(1,k);
				COMPARE1(1,k+1);
				BLEND(0,k,6.0);
			}

			/* Top right corner */
			opacity = 0;
			COMPARE1(0,w-1);
			COMPARE1(0,w-2);
			COMPARE1(1,w-1);
			COMPARE1(1,w-2);
			BLEND(0,w-1,4.0);

			/* Left edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE1(j-1,0);
				COMPARE1(j,0);
				COMPARE1(j+1,0);
				COMPARE1(j-1,1);
				COMPARE1(j,1);
				COMPARE1(j+1,1);
				BLEND(j,0,6.0);
			}

			/* Middle */
			for (j = 1; j < h-1; j++)
			{
				for (k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE1(j-1,k-1);
					COMPARE1(j,k-1);
					COMPARE1(j+1,k-1);
					COMPARE1(j-1,k);
					COMPARE1(j,k);
					COMPARE1(j+1,k);
					COMPARE1(j-1,k+1);
					COMPARE1(j,k+1);
					COMPARE1(j+1,k+1);
					BLEND(j,k,9.0);
				}
			}

			/* Right edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE1(j-1,w-1);
				COMPARE1(j,w-1);
				COMPARE1(j+1,w-1);
				COMPARE1(j-1,w-2);
				COMPARE1(j,w-2);
				COMPARE1(j+1,w-2);
				BLEND(j,w-1,6.0);
			}

			/* Bottom left corner */
			opacity = 0;
			COMPARE1(h-1,0);
			COMPARE1(h-1,1);
			COMPARE1(h-2,0);
			COMPARE1(h-2,1);
			BLEND(h-1,0,4.0);

			/* Bottom edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE1(h-1,k-1);
				COMPARE1(h-1,k);
				COMPARE1(h-1,k+1);
				COMPARE1(h-2,k-1);
				COMPARE1(h-2,k);
				COMPARE1(h-2,k+1);
				BLEND(h-1,k,6.0);
			}

			/* Bottom right corner */
			opacity = 0;
			COMPARE1(h-1,w-1);
			COMPARE1(h-1,w-2);
			COMPARE1(h-2,w-1);
			COMPARE1(h-2,w-2);
			BLEND(h-1,w-1,4.0);
		}
		else
		{
			/* Top left corner */
			opacity = 0;
			COMPARE2(0,0);
			COMPARE2(0,1);
			COMPARE2(1,0);
			COMPARE2(1,1);
			BLEND(0,0,4.0);

			/* Top edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE2(0,k-1);
				COMPARE2(0,k);
				COMPARE2(0,k+1);
				COMPARE2(1,k-1);
				COMPARE2(1,k);
				COMPARE2(1,k+1);
				BLEND(0,k,6.0);
			}

			/* Top right corner */
			opacity = 0;
			COMPARE2(0,w-1);
			COMPARE2(0,w-2);
			COMPARE2(1,w-1);
			COMPARE2(1,w-2);
			BLEND(0,w-1,4.0);

			/* Left edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE2(j-1,0);
				COMPARE2(j,0);
				COMPARE2(j+1,0);
				COMPARE2(j-1,1);
				COMPARE2(j,1);
				COMPARE2(j+1,1);
				BLEND(j,0,6.0);
			}

			/* Middle */
			for (j = 1; j < h-1; j++)
			{
				for (k = 1; k < w-1; k++)
				{
					opacity = 0;
					COMPARE2(j-1,k-1);
					COMPARE2(j,k-1);
					COMPARE2(j+1,k-1);
					COMPARE2(j-1,k);
					COMPARE2(j,k);
					COMPARE2(j+1,k);
					COMPARE2(j-1,k+1);
					COMPARE2(j,k+1);
					COMPARE2(j+1,k+1);
					BLEND(j,k,9.0);
				}
			}

			/* Right edge */
			for (j = 1; j < h-1; j++)
			{
				opacity = 0;
				COMPARE2(j-1,w-1);
				COMPARE2(j,w-1);
				COMPARE2(j+1,w-1);
				COMPARE2(j-1,w-2);
				COMPARE2(j,w-2);
				COMPARE2(j+1,w-2);
				BLEND(j,w-1,6.0);
			}

			/* Bottom left corner */
			opacity = 0;
			COMPARE2(h-1,0);
			COMPARE2(h-1,1);
			COMPARE2(h-2,0);
			COMPARE2(h-2,1);
			BLEND(h-1,0,4.0);

			/* Bottom edge */
			for (k = 1; k < w-1; k++)
			{
				opacity = 0;
				COMPARE2(h-1,k-1);
				COMPARE2(h-1,k);
				COMPARE2(h-1,k+1);
				COMPARE2(h-2,k-1);
				COMPARE2(h-2,k);
				COMPARE2(h-2,k+1);
				BLEND(h-1,k,6.0);
			}

			/* Bottom right corner */
			opacity = 0;
			COMPARE2(h-1,w-1);
			COMPARE2(h-1,w-2);
			COMPARE2(h-2,w-1);
			COMPARE2(h-2,w-2);
			BLEND(h-1,w-1,4.0);
		}
	}
	else
	{
		switch(incoming->get_color_model())
		{
		case BC_RGB_FLOAT:
			SHAPEWIPE(float, 3)
			break;
		case BC_RGB888:
		case BC_YUV888:
			SHAPEWIPE(unsigned char, 3)
			break;
		case BC_RGBA_FLOAT:
			SHAPEWIPE(float, 4)
			break;
		case BC_RGBA8888:
		case BC_YUVA8888:
			SHAPEWIPE(unsigned char, 4)
			break;
		case BC_RGB161616:
		case BC_YUV161616:
			SHAPEWIPE(uint16_t, 3)
			break;
		case BC_RGBA16161616:
		case BC_YUVA16161616:
			SHAPEWIPE(uint16_t, 4)
			break;
		}
	}
	return 0;
}
