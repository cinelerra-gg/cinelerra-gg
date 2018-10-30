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

#include "bcsignals.h"
#include "bccolors.h"
#include "clip.h"
#include "cursors.h"
#include "language.h"
#include "scopewindow.h"
#include "theme.h"


#include <string.h>



ScopePackage::ScopePackage()
 : LoadPackage()
{
}






ScopeUnit::ScopeUnit(ScopeGUI *gui,
	ScopeEngine *server)
 : LoadClient(server)
{
	this->gui = gui;
}





void ScopeUnit::draw_point(unsigned char **rows,
	int x,
	int y,
	int r,
	int g,
	int b)
{
	unsigned char *pixel = rows[y] + x * 4;
	pixel[0] = b;
	pixel[1] = g;
	pixel[2] = r;
}

#define PROCESS_PIXEL(column) \
{ \
/* Calculate histogram */ \
	if(use_hist) \
	{ \
		int v_i = (intensity - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(v_i, 0, TOTAL_BINS - 1); \
		bins[3][v_i]++; \
	} \
	else \
	if(use_hist_parade) \
	{ \
		int r_i = (r - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int g_i = (g - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		int b_i = (b - FLOAT_MIN) * (TOTAL_BINS / (FLOAT_MAX - FLOAT_MIN)); \
		CLAMP(r_i, 0, TOTAL_BINS - 1); \
		CLAMP(g_i, 0, TOTAL_BINS - 1); \
		CLAMP(b_i, 0, TOTAL_BINS - 1); \
		bins[0][r_i]++; \
		bins[1][g_i]++; \
		bins[2][b_i]++; \
	} \
 \
/* Calculate waveform */ \
	if(use_wave || use_wave_parade) \
	{ \
		x = (column) * wave_w / w; \
		if(x >= 0 && x < wave_w)  \
		{ \
			if(use_wave_parade) \
			{ \
				y = wave_h -  \
					(int)((r - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * \
						wave_h); \
	 \
				if(y >= 0 && y < wave_h)  \
					draw_point(waveform_rows, x / 3, y, 0xff, 0x0, 0x0);  \
	 \
				y = wave_h -  \
					(int)((g - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * \
						wave_h); \
	 \
				if(y >= 0 && y < wave_h)  \
					draw_point(waveform_rows, x / 3 + wave_w / 3, y, 0x0, 0xff, 0x0);  \
	 \
				y = wave_h -  \
					(int)((b - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * \
						wave_h); \
	 \
				if(y >= 0 && y < wave_h)  \
					draw_point(waveform_rows, x / 3 + wave_w / 3 * 2, y, 0x0, 0x0, 0xff);  \
	 \
			} \
			else \
			{ \
				y = wave_h -  \
					(int)((intensity - FLOAT_MIN) /  \
						(FLOAT_MAX - FLOAT_MIN) * \
						wave_h); \
			 \
				if(y >= 0 && y < wave_h)  \
					draw_point(waveform_rows,  \
						x,  \
						y,  \
						0xff,  \
						0xff,  \
						0xff);  \
			} \
		} \
	} \
 \
/* Calculate vectorscope */ \
	if(use_vector) \
	{ \
		float adjacent = cos((h + 90) / 360 * 2 * M_PI); \
		float opposite = sin((h + 90) / 360 * 2 * M_PI); \
	 \
		x = (int)(vector_w / 2 +  \
			adjacent * (s) / (FLOAT_MAX) * radius); \
	 \
		y = (int)(vector_h / 2 -  \
			opposite * (s) / (FLOAT_MAX) * radius); \
	 \
	 \
		CLAMP(x, 0, vector_w - 1); \
		CLAMP(y, 0, vector_h - 1); \
	 \
	/* Get color with full saturation & value */ \
		float r_f, g_f, b_f; \
		HSV::hsv_to_rgb(r_f, \
				g_f,  \
				b_f,  \
				h,  \
				s,  \
				1); \
	 \
		draw_point(vector_rows, \
			x,  \
			y,  \
			(int)(CLIP(r_f, 0, 1) * 255),  \
			(int)(CLIP(g_f, 0, 1) * 255),  \
			(int)(CLIP(b_f, 0, 1) * 255)); \
	} \
}

#define PROCESS_RGB_PIXEL(column, max) \
{ \
	r = (float)*row++ / max; \
	g = (float)*row++ / max; \
	b = (float)*row++ / max; \
	HSV::rgb_to_hsv(r,  \
		g,  \
		b,  \
		h,  \
		s,  \
		v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}

#define PROCESS_YUV_PIXEL(column,  \
	y_in,  \
	u_in,  \
	v_in) \
{ \
	YUV::yuv.yuv_to_rgb_f(r, g, b, (float)y_in / 255, (float)(u_in - 0x80) / 255, (float)(v_in - 0x80) / 255); \
	HSV::rgb_to_hsv(r,  \
		g,  \
		b,  \
		h,  \
		s,  \
		v); \
	intensity = v; \
	PROCESS_PIXEL(column) \
}


void ScopeUnit::process_package(LoadPackage *package)
{
	ScopePackage *pkg = (ScopePackage*)package;

	float r, g, b;
	float h, s, v;
	int x, y;
	float intensity;
	int use_hist = gui->use_hist;
	int use_hist_parade = gui->use_hist_parade;
	int use_vector = gui->use_vector;
	int use_wave = gui->use_wave;
	int use_wave_parade = gui->use_wave_parade;
	BC_Bitmap *waveform_bitmap = gui->waveform_bitmap;
	BC_Bitmap *vector_bitmap = gui->vector_bitmap;
	int wave_h = waveform_bitmap->get_h();
	int wave_w = waveform_bitmap->get_w();
	int vector_h = vector_bitmap->get_h();
	int vector_w = vector_bitmap->get_w();

	int w = gui->output_frame->get_w();
	float radius = MIN(gui->vector_w / 2, gui->vector_h / 2);




	unsigned char **waveform_rows = waveform_bitmap->get_row_pointers();
	unsigned char **vector_rows = vector_bitmap->get_row_pointers();


	switch(gui->output_frame->get_color_model())
	{
		case BC_RGB888:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *row = gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_RGB_PIXEL(j, 255)
				}
			}
			break;

		case BC_RGBA8888:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *row = gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_RGB_PIXEL(j, 255)
					row++;
				}
			}
			break;

		case BC_RGB_FLOAT:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				float *row = (float*)gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_RGB_PIXEL(j, 1.0)
				}
			}
			break;

		case BC_RGBA_FLOAT:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				float *row = (float*)gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_RGB_PIXEL(j, 1.0)
					row++;
				}
			}
			break;

		case BC_YUV888:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *row = gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_YUV_PIXEL(j, row[0], row[1], row[2])
					row += 3;
				}
			}
			break;

		case BC_YUVA8888:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *row = gui->output_frame->get_rows()[i];
				for(int j = 0; j < w; j++)
				{
					PROCESS_YUV_PIXEL(j, row[0], row[1], row[2])
					row += 4;
				}
			}
			break;


		case BC_YUV420P:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *y_row = gui->output_frame->get_y() + i * gui->output_frame->get_w();
				unsigned char *u_row = gui->output_frame->get_u() + (i / 2) * (gui->output_frame->get_w() / 2);
				unsigned char *v_row = gui->output_frame->get_v() + (i / 2) * (gui->output_frame->get_w() / 2);
				for(int j = 0; j < w; j += 2)
				{
					PROCESS_YUV_PIXEL(j, *y_row, *u_row, *v_row);
					y_row++;
					PROCESS_YUV_PIXEL(j + 1, *y_row, *u_row, *v_row);
					y_row++;

					u_row++;
					v_row++;
				}
			}
			break;

		case BC_YUV422:
			for(int i = pkg->row1; i < pkg->row2; i++)
			{
				unsigned char *row = gui->output_frame->get_rows()[i];
				for(int j = 0; j < gui->output_frame->get_w(); j += 2)
				{
					PROCESS_YUV_PIXEL(j, row[0], row[1], row[3]);
					PROCESS_YUV_PIXEL(j + 1, row[2], row[1], row[3]);
					row += 4;
				}
			}
			break;

		default:
			printf("ScopeUnit::process_package %d: color_model=%d unrecognized\n",
				__LINE__,
				gui->output_frame->get_color_model());
			break;
	}

}






ScopeEngine::ScopeEngine(ScopeGUI *gui, int cpus)
 : LoadServer(cpus, cpus)
{
//printf("ScopeEngine::ScopeEngine %d cpus=%d\n", __LINE__, cpus);
	this->gui = gui;
}

ScopeEngine::~ScopeEngine()
{
}

void ScopeEngine::init_packages()
{
	for(int i = 0; i < LoadServer::get_total_packages(); i++)
	{
		ScopePackage *pkg = (ScopePackage*)get_package(i);
		pkg->row1 = gui->output_frame->get_h() * i / LoadServer::get_total_packages();
		pkg->row2 = gui->output_frame->get_h() * (i + 1) / LoadServer::get_total_packages();
	}

	for(int i = 0; i < get_total_clients(); i++)
	{
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for(int j = 0; j < HIST_SECTIONS; j++)
			bzero(unit->bins[j], sizeof(int) * TOTAL_BINS);
	}
}


LoadClient* ScopeEngine::new_client()
{
	return new ScopeUnit(gui, this);
}

LoadPackage* ScopeEngine::new_package()
{
	return new ScopePackage;
}

void ScopeEngine::process()
{
	process_packages();

	for(int i = 0; i < HIST_SECTIONS; i++)
		bzero(gui->bins[i], sizeof(int) * TOTAL_BINS);

	for(int i = 0; i < get_total_clients(); i++)
	{
		ScopeUnit *unit = (ScopeUnit*)get_client(i);
		for(int j = 0; j < HIST_SECTIONS; j++)
		{
			for(int k = 0; k < TOTAL_BINS; k++)
			{
				gui->bins[j][k] += unit->bins[j][k];
			}
		}
	}
}



ScopeGUI::ScopeGUI(Theme *theme,
	int x,
	int y,
	int w,
	int h,
	int cpus)
 : PluginClientWindow(_(PROGRAM_NAME ": Scopes"),
 	x,
	y,
	w,
	h,
	MIN_SCOPE_W,
	MIN_SCOPE_H,
	1)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	this->theme = theme;
	this->cpus = cpus;
	reset();
}

ScopeGUI::ScopeGUI(PluginClient *plugin,
	int w,
	int h)
 : PluginClientWindow(plugin,
 	w,
	h,
	MIN_SCOPE_W,
	MIN_SCOPE_H,
	1)
{
	this->x = get_x();
	this->y = get_y();
	this->w = w;
	this->h = h;
	this->theme = plugin->get_theme();
	this->cpus = plugin->PluginClient::smp + 1;
	reset();
}

ScopeGUI::~ScopeGUI()
{
	delete waveform_bitmap;
	delete vector_bitmap;
	delete engine;
}

void ScopeGUI::reset()
{
	frame_w = 1;
	waveform_bitmap = 0;
	vector_bitmap = 0;
	engine = 0;
	use_hist = 0;
	use_wave = 1;
	use_vector = 1;
	use_hist_parade = 0;
	use_wave_parade = 0;
	waveform = 0;
	vectorscope = 0;
	histogram = 0;
	wave_w = wave_h = vector_w = vector_h = 0;
}


void ScopeGUI::create_objects()
{
	if(use_hist && use_hist_parade)
	{
		use_hist = 0;
	}

	if(use_wave && use_wave_parade)
	{
		use_wave = 0;
	}

	if(!engine) engine = new ScopeEngine(this,
		cpus);

	lock_window("ScopeGUI::create_objects");


	int x = theme->widget_border;
	int y = theme->widget_border;


	add_subwindow(hist_on = new ScopeToggle(this,
		x,
		y,
		&use_hist));
	x += hist_on->get_w() + theme->widget_border;

	add_subwindow(hist_parade_on = new ScopeToggle(this,
		x,
		y,
		&use_hist_parade));
	x += hist_parade_on->get_w() + theme->widget_border;

	add_subwindow(waveform_on = new ScopeToggle(this,
		x,
		y,
		&use_wave));
	x += waveform_on->get_w() + theme->widget_border;
	add_subwindow(waveform_parade_on = new ScopeToggle(this,
		x,
		y,
		&use_wave_parade));
	x += waveform_parade_on->get_w() + theme->widget_border;

	add_subwindow(vector_on = new ScopeToggle(this,
		x,
		y,
		&use_vector));
	x += vector_on->get_w() + theme->widget_border;

	add_subwindow(value_text = new BC_Title(x, y, ""));
	x += value_text->get_w() + theme->widget_border;

	y += vector_on->get_h() + theme->widget_border;


//PRINT_TRACE

	create_panels();
//PRINT_TRACE







	update_toggles();
	show_window();
	unlock_window();
}



void ScopeGUI::create_panels()
{
	calculate_sizes(get_w(), get_h());


	if((use_wave || use_wave_parade))
	{
		if(!waveform)
		{
			add_subwindow(waveform = new ScopeWaveform(this,
				wave_x,
				wave_y,
				wave_w,
				wave_h));
			waveform->create_objects();
		}
		else
		{
			waveform->reposition_window(wave_x,
				wave_y,
				wave_w,
				wave_h);
			waveform->clear_box(0, 0, wave_w, wave_h);
		}
	}
	else
	if(!(use_wave || use_wave_parade) && waveform)
	{
		delete waveform;
		waveform = 0;
	}

	if(use_vector)
	{
		if(!vectorscope)
		{
			add_subwindow(vectorscope = new ScopeVectorscope(this,
				vector_x,
				vector_y,
				vector_w,
				vector_h));
			vectorscope->create_objects();
		}
		else
		{
			vectorscope->reposition_window(vector_x,
				vector_y,
				vector_w,
				vector_h);
			vectorscope->clear_box(0, 0, vector_w, vector_h);
		}
	}
	else
	if(!use_vector && vectorscope)
	{
		delete vectorscope;
		vectorscope = 0;
	}

	if((use_hist || use_hist_parade))
	{
		if(!histogram)
		{
// printf("ScopeGUI::create_panels %d %d %d %d %d\n", __LINE__, hist_x,
// hist_y,
// hist_w,
// hist_h);
			add_subwindow(histogram = new ScopeHistogram(this,
				hist_x,
				hist_y,
				hist_w,
				hist_h));
			histogram->create_objects();
		}
		else
		{
			histogram->reposition_window(hist_x,
				hist_y,
				hist_w,
				hist_h);
			histogram->clear_box(0, 0, hist_w, hist_h);
		}
	}
	else
	if(!(use_hist || use_hist_parade))
	{
		delete histogram;
		histogram = 0;
	}



	allocate_bitmaps();
	clear_points(0);
	draw_overlays(1, 1, 0);
}

void ScopeGUI::clear_points(int flash)
{
	if(histogram) histogram->clear_point();
	if(waveform) waveform->clear_point();
	if(vectorscope) vectorscope->clear_point();
	if(histogram && flash) histogram->flash(0);
	if(waveform && flash) waveform->flash(0);
	if(vectorscope && flash) vectorscope->flash(0);
}

void ScopeGUI::toggle_event()
{

}

void ScopeGUI::calculate_sizes(int w, int h)
{
	int margin = theme->widget_border;
	int text_w = get_text_width(SMALLFONT, "000") + margin * 2;
	int total_panels = ((use_hist || use_hist_parade) ? 1 : 0) +
		((use_wave || use_wave_parade) ? 1 : 0) +
		(use_vector ? 1 : 0);
	int x = margin;

	int panel_w = (w - margin) / (total_panels > 0 ? total_panels : 1);
// Vectorscope determines the size of everything else
// Always last panel
	vector_w = 0;
	if(use_vector)
	{
		vector_x = w - panel_w + text_w;
		vector_w = w - margin - vector_x;
		vector_y = vector_on->get_h() + margin * 2;
		vector_h = h - vector_y - margin;

		if(vector_w > vector_h)
		{
			vector_w = vector_h;
			vector_x = w - theme->widget_border - vector_w;
		}

		total_panels--;
		if(total_panels > 0)
			panel_w = (vector_x - text_w - margin) / total_panels;
	}

// Histogram is always 1st panel
	if(use_hist || use_hist_parade)
	{
		hist_x = x;
		hist_y = vector_on->get_h() + margin * 2;
		hist_w = panel_w - margin;
		hist_h = h - hist_y - margin;

		total_panels--;
		x += panel_w;
	}

	if(use_wave || use_wave_parade)
	{
		wave_x = x + text_w;
		wave_y = vector_on->get_h() + margin * 2;
		wave_w = panel_w - margin - text_w;
		wave_h = h - wave_y - margin;
	}

}


void ScopeGUI::allocate_bitmaps()
{
	if(waveform_bitmap) delete waveform_bitmap;
	if(vector_bitmap) delete vector_bitmap;

	int w;
	int h;
// printf("ScopeGUI::allocate_bitmaps %d %d %d %d %d\n",
// __LINE__,
// wave_w,
// wave_h,
// vector_w,
// vector_h);
	w = MAX(wave_w, 16);
	h = MAX(wave_h, 16);
	waveform_bitmap = new_bitmap(w, h);
	w = MAX(vector_w, 16);
	h = MAX(vector_h, 16);
	vector_bitmap = new_bitmap(w, h);
}


int ScopeGUI::resize_event(int w, int h)
{
	clear_box(0, 0, w, h);
	this->w = w;
	this->h = h;
	calculate_sizes(w, h);

	if(waveform)
	{
		waveform->reposition_window(wave_x, wave_y, wave_w, wave_h);
		waveform->clear_box(0, 0, wave_w, wave_h);
	}

	if(histogram)
	{
		histogram->reposition_window(hist_x, hist_y, hist_w, hist_h);
		histogram->clear_box(0, 0, hist_w, hist_h);
	}

	if(vectorscope)
	{
		vectorscope->reposition_window(vector_x, vector_y, vector_w, vector_h);
		vectorscope->clear_box(0, 0, vector_w, vector_h);
	}

	allocate_bitmaps();


	clear_points(0);
	draw_overlays(1, 1, 1);

	return 1;
}

int ScopeGUI::translation_event()
{
	x = get_x();
	y = get_y();

	PluginClientWindow::translation_event();
	return 0;
}


void ScopeGUI::draw_overlays(int overlays, int borders, int flush)
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int text_color = GREEN;
	if(resources->bg_color == 0xffffff)
	{
		text_color = BLACK;
	}

	if(overlays && borders)
	{
		clear_box(0, 0, get_w(), get_h());
	}

	if(overlays)
	{
		set_line_dashes(1);
		set_color(text_color);
		set_font(SMALLFONT);

		if(histogram && (use_hist || use_hist_parade))
		{
			histogram->draw_line(hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN),
					0,
					hist_w * -FLOAT_MIN / (FLOAT_MAX - FLOAT_MIN),
					hist_h);
			histogram->draw_line(hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN),
					0,
					hist_w * (1.0 - FLOAT_MIN) / (FLOAT_MAX - FLOAT_MIN),
					hist_h);
			set_line_dashes(0);
			histogram->draw_point();
			set_line_dashes(1);
			histogram->flash(0);
		}

// Waveform overlay
		if(waveform && (use_wave || use_wave_parade))
		{
			set_color(text_color);
			for(int i = 0; i <= WAVEFORM_DIVISIONS; i++)
			{
				int y = wave_h * i / WAVEFORM_DIVISIONS;
				int text_y = y + wave_y + get_text_ascent(SMALLFONT) / 2;
				CLAMP(text_y, waveform->get_y() + get_text_ascent(SMALLFONT), waveform->get_y() + waveform->get_h() - 1);
				char string[BCTEXTLEN];
				sprintf(string, "%d",
					(int)((FLOAT_MAX -
					i * (FLOAT_MAX - FLOAT_MIN) / WAVEFORM_DIVISIONS) * 100));
				int text_x = wave_x - get_text_width(SMALLFONT, string) - theme->widget_border;
				draw_text(text_x, text_y, string);

				int y1 = CLAMP(y, 0, waveform->get_h() - 1);
				waveform->draw_line(0, y1, wave_w, y1);
				//waveform->draw_rectangle(0, 0, wave_w, wave_h);
			}
			set_line_dashes(0);
			waveform->draw_point();
			set_line_dashes(1);
			waveform->flash(0);
		}


// Vectorscope overlay
		if(vectorscope && use_vector)
		{
			set_color(text_color);
			int radius = MIN(vector_w / 2, vector_h / 2);
			for(int i = 1; i <= VECTORSCOPE_DIVISIONS; i += 2)
			{
				int x = vector_w / 2 - radius * i / VECTORSCOPE_DIVISIONS;
				int y = vector_h / 2 - radius * i / VECTORSCOPE_DIVISIONS;
				int text_y = y + vector_y + get_text_ascent(SMALLFONT) / 2;
				int w = radius * i / VECTORSCOPE_DIVISIONS * 2;
				int h = radius * i / VECTORSCOPE_DIVISIONS * 2;
				char string[BCTEXTLEN];

				sprintf(string, "%d",
					(int)((FLOAT_MAX / VECTORSCOPE_DIVISIONS * i) * 100));
				int text_x = vector_x - get_text_width(SMALLFONT, string) - theme->widget_border;
				draw_text(text_x, text_y, string);
//printf("ScopeGUI::draw_overlays %d %d %d %s\n", __LINE__, text_x, text_y, string);

				vectorscope->draw_circle(x, y, w, h);
		//vectorscope->draw_rectangle(0, 0, vector_w, vector_h);
			}
		// 	vectorscope->draw_circle(vector_w / 2 - radius,
		// 		vector_h / 2 - radius,
		// 		radius * 2,
		// 		radius * 2);

			set_line_dashes(0);
			vectorscope->draw_point();
			set_line_dashes(1);
			vectorscope->flash(0);
		}

		set_font(MEDIUMFONT);
		set_line_dashes(0);
	}

	if(borders)
	{
		if(use_hist || use_hist_parade)
		{
			draw_3d_border(hist_x - 2,
				hist_y - 2,
				hist_w + 4,
				hist_h + 4,
				get_bg_color(),
				BLACK,
				MDGREY,
				get_bg_color());
		}

		if(use_wave || use_wave_parade)
		{
			draw_3d_border(wave_x - 2,
				wave_y - 2,
				wave_w + 4,
				wave_h + 4,
				get_bg_color(),
				BLACK,
				MDGREY,
				get_bg_color());
		}

		if(use_vector)
		{
			draw_3d_border(vector_x - 2,
				vector_y - 2,
				vector_w + 4,
				vector_h + 4,
				get_bg_color(),
				BLACK,
				MDGREY,
				get_bg_color());
		}
	}

	flash(0);
	if(flush) this->flush();
}



void ScopeGUI::process(VFrame *output_frame)
{
	lock_window("ScopeGUI::process");
	this->output_frame = output_frame;
	frame_w = output_frame->get_w();
	//float radius = MIN(vector_w / 2, vector_h / 2);

	bzero(waveform_bitmap->get_data(), waveform_bitmap->get_data_size());
	bzero(vector_bitmap->get_data(), vector_bitmap->get_data_size());


	engine->process();

	if(histogram)
	{
		histogram->draw(0, 0);
	}

	if(waveform)
	{
		waveform->draw_bitmap(waveform_bitmap,
			1,
			0,
			0);
	}

	if(vectorscope)
	{
		vectorscope->draw_bitmap(vector_bitmap,
			1,
			0,
			0);
	}

	draw_overlays(1, 0, 1);
	unlock_window();
}


void ScopeGUI::update_toggles()
{
	hist_parade_on->update(use_hist_parade);
	hist_on->update(use_hist);
	waveform_parade_on->update(use_wave_parade);
	waveform_on->update(use_wave);
	vector_on->update(use_vector);
}










ScopePanel::ScopePanel(ScopeGUI *gui,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->gui = gui;
	is_dragging = 0;
}

void ScopePanel::create_objects()
{
	set_cursor(CROSS_CURSOR, 0, 0);
	clear_box(0, 0, get_w(), get_h());
}

void ScopePanel::update_point(int x, int y)
{
}

void ScopePanel::draw_point()
{
}

void ScopePanel::clear_point()
{
}

int ScopePanel::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		gui->clear_points(1);

		is_dragging = 1;
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::cursor_motion_event()
{
	if(is_dragging)
	{
		int x = get_cursor_x();
		int y = get_cursor_y();
		CLAMP(x, 0, get_w() - 1);
		CLAMP(y, 0, get_h() - 1);
		update_point(x, y);
		return 1;
	}
	return 0;
}


int ScopePanel::button_release_event()
{
	if(is_dragging)
	{
		is_dragging = 0;
		return 1;
	}
	return 0;
}









ScopeWaveform::ScopeWaveform(ScopeGUI *gui,
		int x,
		int y,
		int w,
		int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
	drag_y = -1;
}

void ScopeWaveform::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	drag_y = y;
	int frame_x = x * gui->frame_w / get_w();

	if(gui->use_wave_parade)
	{
		if(x > get_w() / 3 * 2)
			frame_x = (x - get_w() / 3 * 2) * gui->frame_w / (get_w() / 3);
		else
		if(x > get_w() / 3)
			frame_x = (x - get_w() / 3) * gui->frame_w / (get_w() / 3);
		else
			frame_x = x * gui->frame_w / (get_w() / 3);
	}

	float value = ((float)get_h() - y) / get_h() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;

	char string[BCTEXTLEN];
	sprintf(string, "X: %d Value: %.3f", frame_x, value);
	gui->value_text->update(string, 0);

	draw_point();
	flash(1);
}

void ScopeWaveform::draw_point()
{
	if(drag_x >= 0)
	{
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(0, drag_y, get_w(), drag_y);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeWaveform::clear_point()
{
	draw_point();
	drag_x = -1;
	drag_y = -1;
}







ScopeVectorscope::ScopeVectorscope(ScopeGUI *gui,
		int x,
		int y,
		int w,
		int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::clear_point()
{
// Hide it
	draw_point();
	drag_radius = 0;
	drag_angle = 0;
}

void ScopeVectorscope::update_point(int x, int y)
{
// Hide it
	draw_point();

	int radius = MIN(get_w() / 2, get_h() / 2);
	drag_radius = sqrt(SQR(x - get_w() / 2) + SQR(y - get_h() / 2));
	drag_angle = atan2(y - get_h() / 2, x - get_w() / 2);

	drag_radius = MIN(drag_radius, radius);

	float saturation = (float)drag_radius / radius * FLOAT_MAX;
	float hue = -drag_angle * 360 / 2 / M_PI - 90;
	if(hue < 0) hue += 360;

	char string[BCTEXTLEN];
	sprintf(string, "Hue: %.3f Sat: %.3f", hue, saturation);
	gui->value_text->update(string, 0);

// Show it
	draw_point();
	flash(1);
}

void ScopeVectorscope::draw_point()
{
	if(drag_radius > 0)
	{
		int radius = MIN(get_w() / 2, get_h() / 2);
		set_inverse();
		set_color(0xff0000);
		set_line_width(2);
		draw_circle(get_w() / 2 - drag_radius,
			get_h() / 2 - drag_radius,
			drag_radius * 2,
			drag_radius * 2);

		draw_line(get_w() / 2,
			get_h() / 2,
			get_w() / 2 + radius * cos(drag_angle),
			get_h() / 2 + radius * sin(drag_angle));
		set_line_width(1);
		set_opaque();
	}
}



ScopeHistogram::ScopeHistogram(ScopeGUI *gui,
		int x,
		int y,
		int w,
		int h)
 : ScopePanel(gui, x, y, w, h)
{
	drag_x = -1;
}

void ScopeHistogram::clear_point()
{
// Hide it
	draw_point();
	drag_x = -1;
}

void ScopeHistogram::draw_point()
{
	if(drag_x >= 0)
	{
		set_inverse();
		set_color(0xffffff);
		set_line_width(2);
		draw_line(drag_x, 0, drag_x, get_h());
		set_line_width(1);
		set_opaque();
	}
}

void ScopeHistogram::update_point(int x, int y)
{
	draw_point();
	drag_x = x;
	float value = (float)x / get_w() * (FLOAT_MAX - FLOAT_MIN) + FLOAT_MIN;

	char string[BCTEXTLEN];
	sprintf(string, "Value: %.3f", value);
	gui->value_text->update(string, 0);

	draw_point();
	flash(1);
}



void ScopeHistogram::draw_mode(int mode, int color, int y, int h)
{
// Highest of all bins
	int normalize = 0;
	for(int i = 0; i < TOTAL_BINS; i++)
	{
		if(gui->bins[mode][i] > normalize) normalize = gui->bins[mode][i];
	}



	set_color(color);
	for(int i = 0; i < get_w(); i++)
	{
		int accum_start = (int)(i * TOTAL_BINS / get_w());
		int accum_end = (int)((i + 1) * TOTAL_BINS / get_w());
		CLAMP(accum_start, 0, TOTAL_BINS);
		CLAMP(accum_end, 0, TOTAL_BINS);

		int max = 0;
		for(int k = accum_start; k < accum_end; k++)
		{
			max = MAX(gui->bins[mode][k], max);
		}

//			max = max * h / normalize;
		max = (int)(log(max) / log(normalize) * h);

		draw_line(i, y + h - max, i, y + h);
	}
}

void ScopeHistogram::draw(int flash, int flush)
{
	clear_box(0, 0, get_w(), get_h());

	if(gui->use_hist_parade)
	{
		draw_mode(0, 0xff0000, 0, get_h() / 3);
		draw_mode(1, 0x00ff00, get_h() / 3, get_h() / 3);
		draw_mode(2, 0x0000ff, get_h() / 3 * 2, get_h() / 3);
	}
	else
	{
		draw_mode(3, LTGREY, 0, get_h());
	}

	if(flash) this->flash(0);
	if(flush) this->flush();
}






ScopeToggle::ScopeToggle(ScopeGUI *gui,
	int x,
	int y,
	int *value)
 : BC_Toggle(x,
 	y,
	get_image_set(gui, value),
	*value)
{
	this->gui = gui;
	this->value = value;
	if(value == &gui->use_hist_parade)
	{
		set_tooltip(_("Histogram Parade"));
	}
	else
	if(value == &gui->use_hist)
	{
		set_tooltip(_("Histogram"));
	}
	else
	if(value == &gui->use_wave_parade)
	{
		set_tooltip(_("Waveform Parade"));
	}
	else
	if(value == &gui->use_wave)
	{
		set_tooltip(_("Waveform"));
	}
	else
	{
		set_tooltip(_("Vectorscope"));
	}
}

VFrame** ScopeToggle::get_image_set(ScopeGUI *gui, int *value)
{
	if(value == &gui->use_hist_parade)
	{
		return gui->theme->get_image_set("histogram_rgb_toggle");
	}
	else
	if(value == &gui->use_hist)
	{
		return gui->theme->get_image_set("histogram_toggle");
	}
	else
	if(value == &gui->use_wave_parade)
	{
		return gui->theme->get_image_set("waveform_rgb_toggle");
	}
	else
	if(value == &gui->use_wave)
	{
		return gui->theme->get_image_set("waveform_toggle");
	}
	else
	{
		return gui->theme->get_image_set("scope_toggle");
	}
}

int ScopeToggle::handle_event()
{
	*value = get_value();
	if(value == &gui->use_hist_parade)
	{
		if(get_value()) gui->use_hist = 0;
	}
	else
	if(value == &gui->use_hist)
	{
		if(get_value()) gui->use_hist_parade = 0;
	}
	else
	if(value == &gui->use_wave_parade)
	{
		if(get_value()) gui->use_wave = 0;
	}
	else
	if(value == &gui->use_wave)
	{
		if(get_value()) gui->use_wave_parade = 0;
	}


	gui->toggle_event();
	gui->update_toggles();
	gui->create_panels();
	gui->show_window();
	return 1;
}



