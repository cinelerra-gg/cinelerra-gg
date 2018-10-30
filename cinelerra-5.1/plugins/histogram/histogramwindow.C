
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
#include "bcsignals.h"
#include "cursors.h"
#include "histogram.h"
#include "histogramconfig.h"
#include "histogramwindow.h"
#include "keys.h"
#include "language.h"
#include "theme.h"

#include <unistd.h>




HistogramWindow::HistogramWindow(HistogramMain *plugin)
 : PluginClientWindow(plugin,
	plugin->w,
	plugin->h,
	440,
	500,
	1)
{
	this->plugin = plugin;
	active_value = 0;
}

HistogramWindow::~HistogramWindow()
{
}


void HistogramWindow::create_objects()
{
	int margin = plugin->get_theme()->widget_border;
	int x = margin, y = margin, x1 = margin;

	add_subwindow(mode_v = new HistogramMode(plugin,
		x,
		y,
		HISTOGRAM_VALUE,
		_("Value")));
	x += mode_v->get_w() + margin;
	add_subwindow(mode_r = new HistogramMode(plugin,
		x,
		y,
		HISTOGRAM_RED,
		_("Red")));
	x += mode_r->get_w() + margin;
	add_subwindow(mode_g = new HistogramMode(plugin,
		x,
		y,
		HISTOGRAM_GREEN,
		_("Green")));
	x += mode_g->get_w() + margin;
	add_subwindow(mode_b = new HistogramMode(plugin,
		x,
		y,
		HISTOGRAM_BLUE,
		_("Blue")));


	x = get_w() - margin - plugin->get_theme()->get_image_set("histogram_rgb_toggle")[0]->get_w();
	add_subwindow(parade_on = new HistogramParade(plugin,
		this,
		x,
		y,
		1));
	x -= parade_on->get_w() + margin;
	add_subwindow(parade_off = new HistogramParade(plugin,
		this,
		x,
		y,
		0));


	x = x1;
	y += parade_on->get_h() + margin;
	add_subwindow(canvas_title1 = new BC_Title(margin,
		y,
		"-10%"));
	add_subwindow(canvas_title2 = new BC_Title(get_w() - get_text_width(MEDIUMFONT, "110%") - margin,
		y,
		"110%"));

	y += canvas_title2->get_h() + margin;
	x = x1;
	canvas_h = get_h() - y - 210;


	add_subwindow(low_input_carrot = new HistogramCarrot(plugin,
		this,
		x,
		y + canvas_h));

	x = low_input_carrot->get_w() / 2 + x;
	canvas_w = get_w() - x - x;

	title1_x = x;
	title2_x = x + (int)(canvas_w * -HIST_MIN_INPUT / FLOAT_RANGE);
	title3_x = x + (int)(canvas_w * (1.0 - HIST_MIN_INPUT) / FLOAT_RANGE);
	title4_x = x + (int)(canvas_w);





	add_subwindow(canvas = new HistogramCanvas(plugin,
		this,
		x,
		y,
		canvas_w,
		canvas_h));

// Canvas border
	draw_3d_border(x - 2,
		y - 2,
		canvas_w + 4,
		canvas_h + 4,
		get_bg_color(),
		BLACK,
		MDGREY,
		get_bg_color());

// Calculate output curve with no value function
	plugin->tabulate_curve(plugin->mode, 0);

	y += canvas->get_h();
	x = margin;

	add_subwindow(gamma_carrot = new HistogramCarrot(plugin,
		this,
		canvas->get_x() +
			canvas->get_w() / 2 -
			low_input_carrot->get_w() / 2 ,
		y));

	add_subwindow(high_input_carrot = new HistogramCarrot(plugin,
		this,
		canvas->get_x() +
			canvas->get_w() -
			low_input_carrot->get_w() / 2,
		y));
	y += low_input_carrot->get_h() + margin;


//	add_subwindow(title = new BC_Title(x, y, _("Input:")));
//	x += title->get_w() + margin;
	low_input = new HistogramText(plugin,
		this,
		x,
		y);
	low_input->create_objects();

	x = get_w() / 2 - low_input->get_w() / 2;
	gamma = new HistogramText(plugin,
		this,
		x,
		y);
	gamma->create_objects();


	x = get_w() - low_input->get_w() - margin;
	high_input = new HistogramText(plugin,
		this,
		x,
		y);
	high_input->create_objects();


	y += high_input->get_h() + margin;
	x = x1;



	add_subwindow(output = new HistogramSlider(plugin,
		this,
		canvas->get_x(),
		y,
		canvas->get_w(),
		20,
		0));
	output->update();

// Output border
	draw_3d_border(output->get_x() - 2,
		output->get_y() - 2,
		output->get_w() + 4,
		output->get_h() + 4,
		get_bg_color(),
		BLACK,
		MDGREY,
		get_bg_color());


	y += output->get_h();



	add_subwindow(low_output_carrot = new HistogramCarrot(plugin,
		this,
		margin,
		y));

	add_subwindow(high_output_carrot = new HistogramCarrot(plugin,
		this,
		canvas->get_x() +
			canvas->get_w() -
			low_output_carrot->get_w() / 2,
		y));
	y += high_output_carrot->get_h() + margin;


//	add_subwindow(title = new BC_Title(x, y, _("Output:")));
//	x += title->get_w() + margin;
	low_output = new HistogramText(plugin,
		this,
		x,
		y);
	low_output->create_objects();
	high_output = new HistogramText(plugin,
		this,
		get_w() - low_output->get_w() - margin,
		y);
	high_output->create_objects();

	x = x1;
	y += high_output->get_h() + margin;

	add_subwindow(bar = new BC_Bar(x, y, get_w() - margin * 2));
	y += bar->get_h() + margin;

	add_subwindow(automatic = new HistogramAuto(plugin,
		x,
		y));

	//int y1 = y;
	x = 200;
	add_subwindow(threshold_title = new BC_Title(x, y, _("Threshold:")));
	x += threshold_title->get_w() + margin;
	threshold = new HistogramText(plugin,
		this,
		x,
		y);
	threshold->create_objects();

	x = get_w() / 2;
	add_subwindow(reset = new HistogramReset(plugin,
		x,
		y + threshold->get_h() + margin));

	x = x1;
	y += automatic->get_h() + margin;
	add_subwindow(plot = new HistogramPlot(plugin,
		x,
		y));

	y += plot->get_h() + 5;
	add_subwindow(split = new HistogramSplit(plugin,
		x,
		y));

	update(1, 1, 1, 1);

	flash(0);
	show_window();
}



int HistogramWindow::resize_event(int w, int h)
{
	int xdiff = w - get_w();
	int ydiff = h - get_h();

	parade_on->reposition_window(parade_on->get_x() + xdiff,
		parade_on->get_y());
	parade_off->reposition_window(parade_off->get_x() + xdiff,
		parade_on->get_y());
	canvas_title2->reposition_window(canvas_title2->get_x() + xdiff,
		canvas_title2->get_y());

// Canvas follows window size
	canvas_w = canvas_w + xdiff;
	canvas_h = canvas_h + ydiff;
	canvas->reposition_window(canvas->get_x(),
		canvas->get_y(),
		canvas_w,
		canvas_h);

// Canvas border
	draw_3d_border(canvas->get_x() - 2,
		canvas->get_y() - 2,
		canvas_w + 4,
		canvas_h + 4,
		get_bg_color(),
		BLACK,
		MDGREY,
		get_bg_color());

	low_input_carrot->reposition_window(low_input_carrot->get_x(),
		low_input_carrot->get_y() + ydiff);
	gamma_carrot->reposition_window(gamma_carrot->get_x(),
		gamma_carrot->get_y() + ydiff);
	high_input_carrot->reposition_window(high_input_carrot->get_x(),
		high_input_carrot->get_y() + ydiff);

	low_input->reposition_window(low_input->get_x(),
		low_input->get_y() + ydiff);
	gamma->reposition_window(w / 2 - gamma->get_w() / 2,
		gamma->get_y() + ydiff);
	high_input->reposition_window(high_input->get_x() + xdiff,
		low_input->get_y() + ydiff);

	output->reposition_window(output->get_x(),
		output->get_y() + ydiff,
		output->get_w() + xdiff,
		output->get_h());
	output->update();

// Output border
	draw_3d_border(output->get_x() - 2,
		output->get_y() - 2,
		output->get_w() + 4,
		output->get_h() + 4,
		get_bg_color(),
		BLACK,
		MDGREY,
		get_bg_color());

	low_output_carrot->reposition_window(low_output_carrot->get_x(),
		low_output_carrot->get_y() + ydiff);
	high_output_carrot->reposition_window(high_output_carrot->get_x(),
		high_output_carrot->get_y() + ydiff);

	low_output->reposition_window(low_output->get_x(),
		low_output->get_y() + ydiff);
	high_output->reposition_window(high_output->get_x() + xdiff,
		high_output->get_y() + ydiff);

	bar->reposition_window(bar->get_x(),
		bar->get_y() + ydiff,
		bar->get_w() + xdiff);

	automatic->reposition_window(automatic->get_x(),
		automatic->get_y() + ydiff);
	threshold_title->reposition_window(threshold_title->get_x(),
		threshold_title->get_y() + ydiff);
	threshold->reposition_window(threshold->get_x(),
		threshold->get_y() + ydiff);
	reset->reposition_window(reset->get_x(),
		reset->get_y() + ydiff);

	plot->reposition_window(plot->get_x(),
		plot->get_y() + ydiff);
	split->reposition_window(split->get_x(),
		split->get_y() + ydiff);

	update(1, 1, 1, 1);

	plugin->w = w;
	plugin->h = h;
	flash();
	return 1;
}



int HistogramWindow::keypress_event()
{
	int result = 0;

	if(active_value)
	{
		float sign = 1;
		for(int i = 0; i < HISTOGRAM_MODES; i++)
		{
			if(active_value == &plugin->config.gamma[i])
				sign = -1;
		}

		if(get_keypress() == RIGHT || get_keypress() == UP)
		{
			*active_value += sign * PRECISION;
			plugin->config.boundaries();
			update(1, 1, 1, 0);
			plugin->send_configure_change();
			return 1;
		}
		else
		if(get_keypress() == LEFT || get_keypress() == DOWN)
		{
			*active_value -= sign * PRECISION;
			plugin->config.boundaries();
			update(1, 1, 1, 0);
			plugin->send_configure_change();
			return 1;
		}
	}

	return result;
}

void HistogramWindow::update(int do_canvases,
	int do_carrots,
	int do_text,
	int do_toggles)
{
	if(do_toggles)
	{
		automatic->update(plugin->config.automatic);
		mode_v->update(plugin->mode == HISTOGRAM_VALUE ? 1 : 0);
		mode_r->update(plugin->mode == HISTOGRAM_RED ? 1 : 0);
		mode_g->update(plugin->mode == HISTOGRAM_GREEN ? 1 : 0);
		mode_b->update(plugin->mode == HISTOGRAM_BLUE ? 1 : 0);
		plot->update(plugin->config.plot);
		split->update(plugin->config.split);
		parade_on->update(plugin->parade ? 1 : 0);
		parade_off->update(plugin->parade ? 0 : 1);
		output->update();
	}

	if(do_canvases)
	{
		update_canvas();
	}

	if(do_carrots)
	{
		low_input_carrot->update();
		high_input_carrot->update();
		gamma_carrot->update();
		low_output_carrot->update();
		high_output_carrot->update();
	}

	if(do_text)
	{
		low_input->update();
		gamma->update();
		high_input->update();
		low_output->update();
		high_output->update();
		threshold->update();
	}


}


void HistogramWindow::draw_canvas_mode(int mode, int color, int y, int h)
{
	int *accum = plugin->accum[mode];
	int accum_per_canvas_i = HISTOGRAM_SLOTS / canvas_w + 1;
	float accum_per_canvas_f = (float)HISTOGRAM_SLOTS / canvas_w;
	int normalize = 0;
	int max = 0;


// Draw histogram
	for(int i = 0; i < HISTOGRAM_SLOTS; i++)
	{
		if(accum && accum[i] > normalize) normalize = accum[i];
	}


	if(normalize)
	{
		for(int i = 0; i < canvas_w; i++)
		{
			int accum_start = (int)(accum_per_canvas_f * i);
			int accum_end = accum_start + accum_per_canvas_i;
			max = 0;
			for(int j = accum_start; j < accum_end; j++)
			{
				max = MAX(accum[j], max);
			}

//			max = max * h / normalize;
			max = (int)(log(max) / log(normalize) * h);

			canvas->set_color(BLACK);
			canvas->draw_line(i, y, i, y + h - max);
			canvas->set_color(color);
			canvas->draw_line(i, y + h - max, i, y + h);
		}
	}
	else
	{
		canvas->set_color(BLACK);
		canvas->draw_box(0, y, canvas_w, h);
	}

// Draw overlays
	canvas->set_color(WHITE);
	canvas->set_line_width(2);
	int y1 = 0;

// Draw output line
	for(int i = 0; i < canvas_w; i++)
	{
		float input = (float)i /
				canvas_w *
				FLOAT_RANGE +
				HIST_MIN_INPUT;
		float output = plugin->calculate_level(input,
			mode,
			0);

		int y2 = h - (int)(output * h);
		if(i > 0)
		{
			canvas->draw_line(i - 1, y + y1, i, y + y2);
		}
		y1 = y2;
	}

	canvas->set_line_width(1);

}


void HistogramWindow::update_canvas()
{
	if(plugin->parade)
	{
		draw_canvas_mode(HISTOGRAM_RED, RED, 0, canvas_h / 3);
		draw_canvas_mode(HISTOGRAM_GREEN, GREEN, canvas_h / 3, canvas_h / 3);
		draw_canvas_mode(HISTOGRAM_BLUE, BLUE, canvas_h * 2 / 3, canvas_h - canvas_h * 2 / 3);
	}
	else
	{
		draw_canvas_mode(plugin->mode, MEGREY, 0, canvas_h);
	}


// Draw 0 and 100% lines.
	canvas->set_color(RED);
	int x = (int)(canvas_w * -HIST_MIN_INPUT / FLOAT_RANGE);
	canvas->draw_line(x,
		0,
		x,
		canvas_h);
	x = (int)(canvas_w * (1.0 - HIST_MIN_INPUT) / FLOAT_RANGE);
	canvas->draw_line(x,
		0,
		x,
		canvas_h);
	canvas->flash();
}




HistogramParade::HistogramParade(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int value)
 : BC_Toggle(x,
 	y,
	value ? plugin->get_theme()->get_image_set("histogram_rgb_toggle") :
		plugin->get_theme()->get_image_set("histogram_toggle"),
	0)
{
	this->plugin = plugin;
	this->gui = gui;
	this->value = value;
	if(value)
		set_tooltip(_("RGB Parade on"));
	else
		set_tooltip(_("RGB Parade off"));
}

int HistogramParade::handle_event()
{
	update(1);
	plugin->parade = value;
	gui->update(1,
		0,
		0,
		1);
	return 1;
}







HistogramCanvas::HistogramCanvas(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x,
 	y,
	w,
	h,
	BLACK)
{
	this->plugin = plugin;
	this->gui = gui;
}

int HistogramCanvas::button_press_event()
{
	int result = 0;
	if(is_event_win() && cursor_inside())
	{
		if(!plugin->dragging_point &&
			(!plugin->config.automatic || plugin->mode == HISTOGRAM_VALUE))
		{
			gui->deactivate();
		}
	}
	return result;
}

int HistogramCanvas::cursor_motion_event()
{
	if(is_event_win() && cursor_inside())
	{
	}
	return 0;
}

int HistogramCanvas::button_release_event()
{
	return 0;
}














HistogramReset::HistogramReset(HistogramMain *plugin,
	int x,
	int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->plugin = plugin;
}
int HistogramReset::handle_event()
{
	plugin->config.reset(0);
	((HistogramWindow*)plugin->thread->window)->update(1, 1, 1, 1);
	plugin->send_configure_change();
	return 1;
}







HistogramCarrot::HistogramCarrot(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y)
 : BC_Toggle(x,
 	y,
	plugin->get_theme()->get_image_set("histogram_carrot"),
	0)
{
	this->plugin = plugin;
	this->gui = gui;
	drag_operation = 0;
}

HistogramCarrot::~HistogramCarrot()
{
}

float* HistogramCarrot::get_value()
{
	if(this == gui->low_input_carrot)
	{
		return &plugin->config.low_input[plugin->mode];
	}
	else
	if(this == gui->high_input_carrot)
	{
		return &plugin->config.high_input[plugin->mode];
	}
	else
	if(this == gui->gamma_carrot)
	{
		return &plugin->config.gamma[plugin->mode];
	}
	else
	if(this == gui->low_output_carrot)
	{
		return &plugin->config.low_output[plugin->mode];
	}
	else
	if(this == gui->high_output_carrot)
	{
		return &plugin->config.high_output[plugin->mode];
	}
	return 0;
}

void HistogramCarrot::update()
{
	int new_x = 0;
	float *value = get_value();

	if(this != gui->gamma_carrot)
	{
		new_x = (int)(gui->canvas->get_x() +
			(*value - HIST_MIN_INPUT) *
			gui->canvas->get_w() /
			(HIST_MAX_INPUT - HIST_MIN_INPUT) -
			get_w() / 2);
	}
	else
	{
		float min = plugin->config.low_input[plugin->mode];
		float max = plugin->config.high_input[plugin->mode];
		float delta = (max - min) / 2.0;
		float mid = min + delta;
		float tmp = log10(1.0 / *value);
		tmp = mid + delta * tmp;

//printf("HistogramCarrot::update %d %f %f\n", __LINE__, *value, tmp);

			new_x = gui->canvas->get_x() -
				get_w() / 2 +
				(int)(gui->canvas->get_w() *
				(tmp - HIST_MIN_INPUT) /
				(HIST_MAX_INPUT - HIST_MIN_INPUT));
	}

	reposition_window(new_x, get_y());
}

int HistogramCarrot::button_press_event()
{
	if(is_event_win() && get_buttonpress() == 1)
	{
		//int w = get_w();
		gui->deactivate();
		set_status(BC_Toggle::TOGGLE_DOWN);

		BC_Toggle::update(0);
		gui->active_value = get_value();
// Disable the other toggles
		if(this != gui->low_input_carrot) gui->low_input_carrot->BC_Toggle::update(0);
		if(this != gui->high_input_carrot) gui->high_input_carrot->BC_Toggle::update(0);
		if(this != gui->gamma_carrot) gui->gamma_carrot->BC_Toggle::update(0);
		if(this != gui->low_output_carrot) gui->low_output_carrot->BC_Toggle::update(0);
		if(this != gui->high_output_carrot) gui->high_output_carrot->BC_Toggle::update(0);
		starting_x = get_x();
		offset_x = gui->get_relative_cursor_x();
		offset_y = gui->get_relative_cursor_y();
//printf("HistogramCarrot::button_press_event %d %d %d\n", __LINE__, starting_x, offset_x);
		drag_operation = 1;
		draw_face(1, 1);
		return 1;
	}
	return 0;
}

int HistogramCarrot::button_release_event()
{
	int result = BC_Toggle::button_release_event();
	handle_event();
	drag_operation = 0;
	return result;
}

int HistogramCarrot::cursor_motion_event()
{
	int cursor_x = gui->get_relative_cursor_x();

	if(drag_operation)
	{
//printf("HistogramCarrot::cursor_motion_event %d %d\n", __LINE__, cursor_x);
		int new_x = starting_x + cursor_x - offset_x;

// Clamp x
// Get level from x
		float *value = get_value();
		if(this == gui->gamma_carrot)
		{
			float min = gui->low_input_carrot->get_x();
 			float max = gui->high_input_carrot->get_x();
 			float delta = (max - min) / 2.0;
 			if(delta >= 0.5)
 			{
 				float mid = min + delta;
				float tmp = (float)(new_x - mid) /
					delta;
 				tmp = 1.0 / pow(10, tmp);
				CLAMP(tmp, MIN_GAMMA, MAX_GAMMA);
				*value = tmp;
//printf("HistogramCarrot::update %d %f\n", __LINE__, tmp);
			}
		}
		else
		{
			int min_x = gui->canvas->get_x() - get_w() / 2;
			int max_x = gui->canvas->get_x() + gui->canvas->get_w() - get_w() / 2;
			CLAMP(new_x, min_x, max_x);
			*value = HIST_MIN_INPUT +
				(HIST_MAX_INPUT - HIST_MIN_INPUT) *
				(new_x - min_x) /
				(max_x - min_x);
		}

		reposition_window(new_x, get_y());
		flush();

		gui->update(1,
			(this == gui->low_input_carrot || this == gui->high_input_carrot),
			1,
			0);
		plugin->send_configure_change();

		return 1;
	}
	return 0;
}








HistogramSlider::HistogramSlider(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y,
	int w,
	int h,
	int is_input)
 : BC_SubWindow(x, y, w, h)
{
	this->plugin = plugin;
	this->gui = gui;
	this->is_input = is_input;
	operation = NONE;
}

int HistogramSlider::input_to_pixel(float input)
{
	return (int)((input - HIST_MIN_INPUT) / FLOAT_RANGE * get_w());
}

void HistogramSlider::update()
{
	int w = get_w();
	int h = get_h();
	//int half_h = get_h() / 2;
	//int quarter_h = get_h() / 4;
	int mode = plugin->mode;
	int r = 0xff;
	int g = 0xff;
	int b = 0xff;

	clear_box(0, 0, w, h);

	switch(mode)
	{
		case HISTOGRAM_RED:
			g = b = 0x00;
			break;
		case HISTOGRAM_GREEN:
			r = b = 0x00;
			break;
		case HISTOGRAM_BLUE:
			r = g = 0x00;
			break;
	}

	for(int i = 0; i < w; i++)
	{
		int color = (int)(i * 0xff / w);
		set_color(((r * color / 0xff) << 16) |
			((g * color / 0xff) << 8) |
			(b * color / 0xff));

		draw_line(i, 0, i, h);
	}


	flash();
}









HistogramAuto::HistogramAuto(HistogramMain *plugin,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->config.automatic, _("Automatic"))
{
	this->plugin = plugin;
}

int HistogramAuto::handle_event()
{
	plugin->config.automatic = get_value();
	plugin->send_configure_change();
	return 1;
}




HistogramPlot::HistogramPlot(HistogramMain *plugin,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->config.plot, _("Plot histogram"))
{
	this->plugin = plugin;
}

int HistogramPlot::handle_event()
{
	plugin->config.plot = get_value();
	plugin->send_configure_change();
	return 1;
}




HistogramSplit::HistogramSplit(HistogramMain *plugin,
	int x,
	int y)
 : BC_CheckBox(x, y, plugin->config.split, _("Split output"))
{
	this->plugin = plugin;
}

int HistogramSplit::handle_event()
{
	plugin->config.split = get_value();
	plugin->send_configure_change();
	return 1;
}



HistogramMode::HistogramMode(HistogramMain *plugin,
	int x,
	int y,
	int value,
	char *text)
 : BC_Radial(x, y, plugin->mode == value, text)
{
	this->plugin = plugin;
	this->value = value;
}
int HistogramMode::handle_event()
{
	HistogramWindow *gui = (HistogramWindow*)plugin->thread->window;
	plugin->mode = value;
	plugin->current_point= -1;
	gui->active_value = 0;
	gui->low_input_carrot->BC_Toggle::update(0);
	gui->gamma_carrot->BC_Toggle::update(0);
	gui->high_input_carrot->BC_Toggle::update(0);
	gui->low_output_carrot->BC_Toggle::update(0);
	gui->high_output_carrot->BC_Toggle::update(0);
	gui->update(1, 1, 1, 1);
//	plugin->send_configure_change();
	return 1;
}










HistogramText::HistogramText(HistogramMain *plugin,
	HistogramWindow *gui,
	int x,
	int y)
 : BC_TumbleTextBox(gui,
		0.0,
		(float)HIST_MIN_INPUT,
		(float)HIST_MAX_INPUT,
		x,
		y,
		70)
{
	this->plugin = plugin;
	this->gui = gui;
	set_precision(DIGITS);
	set_increment(PRECISION);
}

float* HistogramText::get_value()
{
	if(this == gui->low_input)
	{
		return &plugin->config.low_input[plugin->mode];
	}
	else
	if(this == gui->high_input)
	{
		return &plugin->config.high_input[plugin->mode];
	}
	else
	if(this == gui->gamma)
	{
		return &plugin->config.gamma[plugin->mode];
	}
	else
	if(this == gui->low_output)
	{
		return &plugin->config.low_output[plugin->mode];
	}
	else
	if(this == gui->high_output)
	{
		return &plugin->config.high_output[plugin->mode];
	}
	else
	if(this == gui->threshold)
	{
		return &plugin->config.threshold;
	}

	return 0;
}

int HistogramText::handle_event()
{
	float *output = get_value();
	if(output)
	{
		*output = atof(get_text());
	}

	gui->update(1, 1, 0, 0);
	plugin->send_configure_change();
	return 1;
}

void HistogramText::update()
{
	float *output = get_value();
	if(output)
	{
		BC_TumbleTextBox::update(*output);
	}
}















