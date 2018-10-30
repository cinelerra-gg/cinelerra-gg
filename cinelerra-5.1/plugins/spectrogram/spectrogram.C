
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
#include "clip.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "bccolors.h"
#include "samples.h"
#include "spectrogram.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"


#include <string.h>



REGISTER_PLUGIN(Spectrogram)


#define HALF_WINDOW (config.window_size / 2)

SpectrogramConfig::SpectrogramConfig()
{
	level = 0.0;
	window_size = MAX_WINDOW;
	xzoom = 1;
	frequency = 440;
	normalize = 0;
	mode = VERTICAL;
	history_size = 4;
}

int SpectrogramConfig::equivalent(SpectrogramConfig &that)
{
	return EQUIV(level, that.level) &&
		xzoom == that.xzoom &&
		frequency == that.frequency &&
		window_size == that.window_size &&
		normalize == that.normalize &&
		mode == that.mode &&
		history_size == that.history_size;
}

void SpectrogramConfig::copy_from(SpectrogramConfig &that)
{
	level = that.level;
	xzoom = that.xzoom;
	frequency = that.frequency;
	window_size = that.window_size;
	normalize = that.normalize;
	mode = that.mode;
	history_size = that.history_size;

	CLAMP(history_size, MIN_HISTORY, MAX_HISTORY);
	CLAMP(frequency, MIN_FREQ, MAX_FREQ);
	CLAMP(xzoom, MIN_XZOOM, MAX_XZOOM);
}

void SpectrogramConfig::interpolate(SpectrogramConfig &prev,
	SpectrogramConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(prev);
}



SpectrogramFrame::SpectrogramFrame(int data_size)
{
	this->data_size = data_size;
	data = new float[data_size];
	force = 0;
}

SpectrogramFrame::~SpectrogramFrame()
{
	delete [] data;
}



SpectrogramLevel::SpectrogramLevel(Spectrogram *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, INFINITYGAIN, 40)
{
	this->plugin = plugin;
}

int SpectrogramLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}





SpectrogramMode::SpectrogramMode(Spectrogram *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
	y,
	120,
	mode_to_text(plugin->config.mode))
{
	this->plugin = plugin;
}

void SpectrogramMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(VERTICAL)));
	add_item(new BC_MenuItem(mode_to_text(HORIZONTAL)));
}

int SpectrogramMode::handle_event()
{
	if(plugin->config.mode != text_to_mode(get_text()))
	{
		SpectrogramWindow *window = (SpectrogramWindow*)plugin->thread->window;
		window->probe_x = -1;
		window->probe_y = -1;
		plugin->config.mode = text_to_mode(get_text());
		window->canvas->clear_box(0, 0, window->canvas->get_w(), window->canvas->get_h());
		plugin->send_configure_change();
	}
	return 1;
}

const char* SpectrogramMode::mode_to_text(int mode)
{
	switch(mode)
	{
		case VERTICAL:
			return _("Vertical");
		case HORIZONTAL:
		default:
			return _("Horizontal");
	}
}

int SpectrogramMode::text_to_mode(const char *text)
{
	if(!strcmp(mode_to_text(VERTICAL), text)) return VERTICAL;
	return HORIZONTAL;
}




SpectrogramHistory::SpectrogramHistory(Spectrogram *plugin,
	int x,
	int y)
 : BC_IPot(x, y, plugin->config.history_size, MIN_HISTORY, MAX_HISTORY)
{
	this->plugin = plugin;
}

int SpectrogramHistory::handle_event()
{
	plugin->config.history_size = get_value();
	plugin->send_configure_change();
	return 1;
}






SpectrogramWindowSize::SpectrogramWindowSize(Spectrogram *plugin,
	int x,
	int y,
	char *text)
 : BC_PopupMenu(x,
	y,
	80,
	text)
{
	this->plugin = plugin;
}

int SpectrogramWindowSize::handle_event()
{
	plugin->config.window_size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}


SpectrogramWindowSizeTumbler::SpectrogramWindowSizeTumbler(Spectrogram *plugin, int x, int y)
 : BC_Tumbler(x,
 	y)
{
	this->plugin = plugin;
}

int SpectrogramWindowSizeTumbler::handle_up_event()
{
	plugin->config.window_size *= 2;
	if(plugin->config.window_size > MAX_WINDOW)
		plugin->config.window_size = MAX_WINDOW;
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.window_size);
	((SpectrogramWindow*)plugin->get_thread()->get_window())->window_size->set_text(string);
	plugin->send_configure_change();
	return 0;
}

int SpectrogramWindowSizeTumbler::handle_down_event()
{
	plugin->config.window_size /= 2;
	if(plugin->config.window_size < MIN_WINDOW)
		plugin->config.window_size = MIN_WINDOW;
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.window_size);
	((SpectrogramWindow*)plugin->get_thread()->get_window())->window_size->set_text(string);
	plugin->send_configure_change();
	return 1;
}





SpectrogramNormalize::SpectrogramNormalize(Spectrogram *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.normalize, _("Normalize"))
{
	this->plugin = plugin;
}

int SpectrogramNormalize::handle_event()
{
	plugin->config.normalize = get_value();
	plugin->send_configure_change();
	return 1;
}




SpectrogramFreq::SpectrogramFreq(Spectrogram *plugin, int x, int y)
 : BC_TextBox(x,
		y,
		100,
		1,
		(int)plugin->config.frequency)
{
	this->plugin = plugin;
}

int SpectrogramFreq::handle_event()
{
	plugin->config.frequency = atoi(get_text());
	CLAMP(plugin->config.frequency, MIN_FREQ, MAX_FREQ);
	plugin->send_configure_change();
	return 1;
}





SpectrogramXZoom::SpectrogramXZoom(Spectrogram *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.xzoom, MIN_XZOOM, MAX_XZOOM)
{
	this->plugin = plugin;
}

int SpectrogramXZoom::handle_event()
{
	plugin->config.xzoom = get_value();
	plugin->send_configure_change();
	return 1;
}



SpectrogramCanvas::SpectrogramCanvas(Spectrogram *plugin, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	current_operation = NONE;
}

int SpectrogramCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		calculate_point();
		current_operation = DRAG;
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

int SpectrogramCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		current_operation = NONE;
		return 1;
	}
	return 0;
}

int SpectrogramCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		calculate_point();
	}
	return 0;
}


void SpectrogramCanvas::calculate_point()
{
	int x = get_cursor_x();
	int y = get_cursor_y();
	CLAMP(x, 0, get_w() - 1);
	CLAMP(y, 0, get_h() - 1);

	((SpectrogramWindow*)plugin->thread->window)->calculate_frequency(
		x,
		y,
		1);

//printf("SpectrogramCanvas::calculate_point %d %d\n", __LINE__, Freq::tofreq(freq_index));
}

void SpectrogramCanvas::draw_overlay()
{
	SpectrogramWindow *window = (SpectrogramWindow*)plugin->thread->window;
	if(window->probe_x >= 0 || window->probe_y >= 0)
	{
		set_color(GREEN);
		set_inverse();
		if(plugin->config.mode == HORIZONTAL) draw_line(0, window->probe_y, get_w(), window->probe_y);
		draw_line(window->probe_x, 0, window->probe_x, get_h());
		set_opaque();
	}
}








SpectrogramWindow::SpectrogramWindow(Spectrogram *plugin)
 : PluginClientWindow(plugin,
	plugin->w,
	plugin->h,
	320,
	320,
	1)
{
	this->plugin = plugin;
	probe_x = probe_y = -1;
}

SpectrogramWindow::~SpectrogramWindow()
{
}

void SpectrogramWindow::create_objects()
{
	int x = plugin->get_theme()->widget_border;
	int y = x;
	int x1 = x;
	int y1 = y;
	char string[BCTEXTLEN];



	add_subwindow(canvas = new SpectrogramCanvas(plugin,
		0,
		0,
		get_w(),
		get_h() -
			BC_Pot::calculate_h() * 2 -
			plugin->get_theme()->widget_border * 3));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);

	x = plugin->get_theme()->widget_border;
	y = canvas->get_y() + canvas->get_h() + plugin->get_theme()->widget_border;

	x1 = x;
	y1 = y;
	add_subwindow(level_title = new BC_Title(x, y, _("Level:")));
	x += level_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(level = new SpectrogramLevel(plugin, x, y));
	x += level->get_w() + plugin->get_theme()->widget_border;
	y += level->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(normalize = new SpectrogramNormalize(plugin, x1, y));

	x = x1 + level_title->get_w() + level->get_w() + plugin->get_theme()->widget_border * 2;
	x1 = x;
	y = y1;

	sprintf(string, "%d", plugin->config.window_size);
	add_subwindow(window_size_title = new BC_Title(x, y, _("Window size:")));

	x += window_size_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(window_size = new SpectrogramWindowSize(plugin, x, y, string));
	x += window_size->get_w();
	add_subwindow(window_size_tumbler = new SpectrogramWindowSizeTumbler(plugin, x, y));

	for(int i = MIN_WINDOW; i <= MAX_WINDOW; i *= 2)
	{
		sprintf(string, "%d", i);
		window_size->add_item(new BC_MenuItem(string));
	}

//	x += window_size_tumbler->get_w() + plugin->get_theme()->widget_border;
	x = x1;
	y += window_size->get_h() + plugin->get_theme()->widget_border;



	add_subwindow(mode_title = new BC_Title(x, y, _("Mode:")));
	x += mode_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(mode = new SpectrogramMode(plugin,
		x,
		y));
	mode->create_objects();
	x += mode->get_w() + plugin->get_theme()->widget_border;

	x = x1 = window_size_tumbler->get_x() +
		window_size_tumbler->get_w() +
		plugin->get_theme()->widget_border;
	y = y1;
	add_subwindow(history_title = new BC_Title(x, y, _("History:")));
	x += history_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(history = new SpectrogramHistory(plugin,
		x,
		y));

	x = x1;
	y += history->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(xzoom_title = new BC_Title(x, y, _("X Zoom:")));
	x += xzoom_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(xzoom = new SpectrogramXZoom(plugin, x, y));
	x += xzoom->get_w() + plugin->get_theme()->widget_border;

	y = y1;
	x1 = x;
	add_subwindow(freq_title = new BC_Title(x1, y, _("Freq: 0 Hz")));
//	x += freq_title->get_w() + plugin->get_theme()->widget_border;
	y += freq_title->get_h() + plugin->get_theme()->widget_border;
//	add_subwindow(freq = new SpectrogramFreq(plugin, x, y));
//	y += freq->get_h() + plugin->get_theme()->widget_border;
	x = x1;
	add_subwindow(amplitude_title = new BC_Title(x, y, _("Amplitude: 0 dB")));



	show_window();
}

int SpectrogramWindow::resize_event(int w, int h)
{
	int canvas_h = canvas->get_h();
	int canvas_difference = get_h() - canvas_h;

	canvas->reposition_window(0,
		0,
		w,
		h - canvas_difference);
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	probe_x = -1;
	probe_y = -1;

	int y_diff = -canvas_h + canvas->get_h();

// Remove all columns which may be a different size.
	plugin->frame_buffer.remove_all_objects();
	plugin->frame_history.remove_all_objects();

	level_title->reposition_window(
		level_title->get_x(),
		level_title->get_y() + y_diff);
	level->reposition_window(level->get_x(),
		level->get_y() + y_diff);

	window_size_title->reposition_window(
		window_size_title->get_x(),
		window_size_title->get_y() + y_diff);

	normalize->reposition_window(normalize->get_x(),
		normalize->get_y() + y_diff);
	window_size->reposition_window(window_size->get_x(),
		window_size->get_y() + y_diff);
	window_size_tumbler->reposition_window(window_size_tumbler->get_x(),
		window_size_tumbler->get_y() + y_diff);



	mode_title->reposition_window(mode_title->get_x(),
		mode_title->get_y() + y_diff);
	mode->reposition_window(mode->get_x(),
		mode->get_y() + y_diff);


	history_title->reposition_window(history_title->get_x(),
		history_title->get_y() + y_diff);
	history->reposition_window(history->get_x(),
		history->get_y() + y_diff);

	xzoom_title->reposition_window(xzoom_title->get_x(),
		xzoom_title->get_y() + y_diff);
	xzoom->reposition_window(xzoom->get_x(),
		xzoom->get_y() + y_diff);
	freq_title->reposition_window(freq_title->get_x(),
		freq_title->get_y() + y_diff);
//	freq->reposition_window(freq->get_x(),
//		freq->get_y() + y_diff);
	amplitude_title->reposition_window(amplitude_title->get_x(),
		amplitude_title->get_y() + y_diff);

	flush();
	plugin->w = w;
	plugin->h = h;
	return 0;
}


void SpectrogramWindow::calculate_frequency(int x, int y, int do_overlay)
{
	if(x < 0 && y < 0) return;

// Clear previous overlay
	if(do_overlay) canvas->draw_overlay();

// New probe position
	probe_x = x;
	probe_y = y;

// Convert to coordinates in frame history
	int freq_pixel, time_pixel;
	if(plugin->config.mode == VERTICAL)
	{
		freq_pixel = get_w() - x;
		time_pixel = 0;
	}
	else
	{
		freq_pixel = y;
		time_pixel = get_w() - x;
	}

	CLAMP(time_pixel, 0, plugin->frame_history.size() - 1);
	if(plugin->frame_history.size())
	{
		SpectrogramFrame *ptr = plugin->frame_history.get(
			plugin->frame_history.size() - time_pixel - 1);

		int pixels;
		int freq_index;
		if(plugin->config.mode == VERTICAL)
		{
			pixels = canvas->get_w();
			freq_index = (pixels - freq_pixel) * TOTALFREQS / pixels;
		}
		else
		{
			pixels = canvas->get_h();
			freq_index = (pixels - freq_pixel)  * TOTALFREQS / pixels;
		}

		int freq = Freq::tofreq(freq_index);


		CLAMP(freq_pixel, 0, ptr->data_size - 1);
		double level = ptr->data[freq_pixel];

		char string[BCTEXTLEN];
		sprintf(string, _("Freq: %d Hz"), freq);
		freq_title->update(string);

		sprintf(string, _("Amplitude: %.2f dB"), level);
		amplitude_title->update(string);
	}

	if(do_overlay)
	{
		canvas->draw_overlay();
		canvas->flash();
	}
}



void SpectrogramWindow::update_gui()
{
	char string[BCTEXTLEN];
	level->update(plugin->config.level);
	sprintf(string, "%d", plugin->config.window_size);
	window_size->set_text(string);

	mode->set_text(mode->mode_to_text(plugin->config.mode));
	history->update(plugin->config.history_size);

//	sprintf(string, "%d", plugin->config.window_fragment);
//	window_fragment->set_text(string);

	normalize->set_value(plugin->config.normalize);
}

























Spectrogram::Spectrogram(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	timer = new Timer;
	w = 640;
	h = 480;
}

Spectrogram::~Spectrogram()
{
	delete fft;
	delete [] data;
	delete audio_buffer;
	delete [] freq_real;
	delete [] freq_imag;
	delete timer;
	frame_buffer.remove_all_objects();
	frame_history.remove_all_objects();
}


void Spectrogram::reset()
{
	thread = 0;
	fft = 0;
	done = 0;
	data = 0;
	audio_buffer = 0;
	audio_buffer_start = -MAX_WINDOW * 2;
	freq_real = 0;
	freq_imag = 0;
	allocated_data = 0;
	total_windows = 0;
	bzero(&header, sizeof(data_header_t));
}


const char* Spectrogram::plugin_title() { return N_("Spectrogram"); }
int Spectrogram::is_realtime() { return 1; }

int Spectrogram::process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate)
{
// Pass through
	read_samples(buffer,
		0,
		sample_rate,
		start_position,
		size);


	load_configuration();

// Reset audio buffer
	if(window_size != config.window_size)
	{
		render_stop();
		window_size = config.window_size;
	}





	if(!fft)
	{
		fft = new FFT;
	}

	if(!data)
	{
		data = new unsigned char[sizeof(data_header_t)];
		allocated_data = 0;
	}

	if(!freq_real)
	{
		freq_real = new double[MAX_WINDOW];
		freq_imag = new double[MAX_WINDOW];
	}

	if(!audio_buffer)
	{
		audio_buffer = new Samples(MAX_WINDOW);
	}


// Accumulate audio
	int needed = buffer_size + size;
	if(audio_buffer->get_allocated() < needed)
	{
		Samples *new_samples = new Samples(needed);
		memcpy(new_samples->get_data(),
			audio_buffer->get_data(),
			sizeof(double) * buffer_size);
		delete audio_buffer;
		audio_buffer = new_samples;
	}

	double *audio_samples = audio_buffer->get_data();
	memcpy(audio_samples + buffer_size,
		buffer->get_data(),
		sizeof(double) * size);
	buffer_size += size;


//printf("Spectrogram::process_buffer %d %d\n", __LINE__, buffer_size);

// Append a windows to end of GUI buffer
	total_windows = 0;
	while(buffer_size >= window_size)
	{
// Process FFT
		fft->do_fft(window_size,  // must be a power of 2
    		0,         // 0 = forward FFT, 1 = inverse
    		audio_samples,     // array of input's real samples
    		0,     // array of input's imag samples
    		freq_real,    // array of output's reals
    		freq_imag);

// Get peak in waveform
		double max = 0;
		for(int i = 0; i < window_size; i++)
		{
			double sample = fabs(audio_samples[i]);
			if(sample > max) max = sample;
		}

// Append to end of data buffer
		if(allocated_data < (total_windows + 1) * (HALF_WINDOW + 1))
		{
			int new_allocation = (total_windows + 1) * (HALF_WINDOW + 1);
			unsigned char *new_data = new unsigned char[sizeof(data_header_t) +
				sizeof(float) * new_allocation];
			data_header_t *new_header = (data_header_t*)new_data;
			data_header_t *old_header = (data_header_t*)data;
			memcpy(new_header->samples,
				old_header->samples,
				sizeof(float) * allocated_data);
			delete [] data;
			data = new_data;
			allocated_data = new_allocation;
		}

		data_header_t *header = (data_header_t*)data;
		float *sample_output = header->samples + total_windows * (HALF_WINDOW + 1);
// 1st sample is maximum
		sample_output[0] = max;
		for(int i = 0; i < HALF_WINDOW; i++)
		{
			sample_output[i + 1] = sqrt(freq_real[i] * freq_real[i] +
				freq_imag[i] * freq_imag[i]);
//			sample_output[i + 1] = freq_real[i];
		}

// Shift audio buffer out
		memcpy(audio_samples,
			audio_samples + window_size,
			(buffer_size - window_size) * sizeof(double));

		total_windows++;
		buffer_size -= window_size;
	}

	data_header_t *header = (data_header_t*)data;
	header->window_size = window_size;
	header->sample_rate = sample_rate;
	header->total_windows = total_windows;
// Linear output level
	header->level = DB::fromdb(config.level);

	send_render_gui(data,
		sizeof(data_header_t) +
			sizeof(float) * total_windows * (HALF_WINDOW + 1));

	return 0;
}

void Spectrogram::render_stop()
{
	buffer_size = 0;
	audio_buffer_start = -MAX_WINDOW * 2;
	frame_buffer.remove_all_objects();
	frame_history.remove_all_objects();
}




NEW_WINDOW_MACRO(Spectrogram, SpectrogramWindow)

void Spectrogram::update_gui()
{
	if(thread)
	{
		int result = load_configuration();
		SpectrogramWindow *window = (SpectrogramWindow*)thread->get_window();
		window->lock_window("Spectrogram::update_gui");
		if(result) window->update_gui();


//printf("Spectrogram::update_gui %d\n", __LINE__);
// Shift in accumulated canvas columns
		if(frame_buffer.size())
		{
			SpectrogramCanvas *canvas = (SpectrogramCanvas*)window->canvas;
			canvas->draw_overlay();
// Z to draw in this iteration
			int total_frames = timer->get_difference() *
				header.sample_rate /
				header.window_size /
				1000;

//printf("Spectrogram::update_gui %d %d %ld\n", __LINE__, frame_buffer.size(), timer->get_difference());
			if(total_frames) timer->subtract(total_frames *
				header.window_size *
				1000 /
				header.sample_rate);

// Add forced column drawing
			for(int i = 0; i < frame_buffer.size(); i++)
				if(frame_buffer.get(i)->force) total_frames++;
			total_frames = MIN(frame_buffer.size(), total_frames);

// Limit to canvas width
			if(config.mode == HORIZONTAL)
				total_frames = MIN(canvas->get_w(), total_frames);



			if(config.mode == HORIZONTAL)
			{
// Shift left
				int pixels = canvas->get_h();
				canvas->copy_area(total_frames * config.xzoom,
					0,
					0,
					0,
					canvas->get_w() - total_frames * config.xzoom,
					canvas->get_h());


// Draw new columns
				for(int frame = 0;
					frame < total_frames;
					frame++)
				{
					int x = canvas->get_w() - (total_frames - frame) * config.xzoom;
					SpectrogramFrame *ptr = frame_buffer.get(0);

					for(int i = 0; i < pixels; i++)
					{
						float db = ptr->data[
							MIN(i, ptr->data_size - 1)];
						float h, s, v;
						float r_out, g_out, b_out;
						int r, g, b;

#define DIVISION1 0.0
#define DIVISION2 -20.0
#define DIVISION3 INFINITYGAIN
						if(db > DIVISION2)
						{
							h = 240 - (float)(db - DIVISION2) / (DIVISION1 - DIVISION2) *
								240;
							CLAMP(h, 0, 240);
							s = 1.0;
							v = 1.0;
							HSV::hsv_to_rgb(r_out, g_out, b_out, h, s, v);
							r = (int)(r_out * 0xff);
							g = (int)(g_out * 0xff);
							b = (int)(b_out * 0xff);
						}
						else
						if(db > DIVISION3)
						{
							h = 0.0;
							s = 0.0;
							v = (float)(db - DIVISION3) / (DIVISION2 - DIVISION3);
							HSV::hsv_to_rgb(r_out, g_out, b_out, h, s, v);
							r = (int)(r_out * 0xff);
							g = (int)(g_out * 0xff);
							b = (int)(b_out * 0xff);
						}
						else
						{
							r = g = b = 0;
						}

						canvas->set_color((r << 16) |
							(g << 8) |
							(b));
						if(config.xzoom == 1)
							canvas->draw_pixel(x, i);
						else
							canvas->draw_line(x,
								i,
								x + config.xzoom,
								i);
					}

// Push frames onto history
					for(int i = 0; i < config.xzoom; i++)
					{
						SpectrogramFrame *new_frame = new SpectrogramFrame(
							ptr->data_size);
						frame_history.append(new_frame);
						memcpy(new_frame->data, ptr->data, sizeof(float) * ptr->data_size);
					}

// Clip history to canvas size
					while(frame_history.size() > canvas->get_w())
						frame_history.remove_object_number(0);

					frame_buffer.remove_object_number(0);
				}
			}
			else
// mode == VERTICAL
			{
// Shift frames into history
				for(int frame = 0; frame < total_frames; frame++)
				{
					if(frame_history.size() >= config.history_size)
						frame_history.remove_object_number(0);

					frame_history.append(frame_buffer.get(0));
					frame_buffer.remove_number(0);
				}

// Reduce history size
				while(frame_history.size() > config.history_size)
					frame_history.remove_object_number(0);

// Draw frames from history
				canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
				for(int frame = 0; frame < frame_history.size(); frame++)
				{
					SpectrogramFrame *ptr = frame_history.get(frame);
//printf("%d %d\n", canvas->get_w(), ptr->data_size);

					int luma = (frame + 1) * 0x80 / frame_history.size();
					if(frame == frame_history.size() - 1)
					{
						canvas->set_color(WHITE);
						canvas->set_line_width(2);
					}
					else
						canvas->set_color((luma << 16) |
							(luma << 8) |
							luma);


					int x1 = 0;
					int y1 = 0;
					int w = canvas->get_w();
					int h = canvas->get_h();
					int number = 0;

//printf("Spectrogram::update_gui %d ", __LINE__);

					for(int x2 = 0; x2 < w; x2++)
					{
						float db = ptr->data[
							MIN((w - x2), ptr->data_size - 1)];
//if(x2 > w - 10) printf("%.02f ", ptr->data[x2]);
						int y2 = h - 1 - (int)((db - INFINITYGAIN) /
							(0 - INFINITYGAIN) *
							h);
						CLAMP(y2, 0, h - 1);

						if(number)
						{
							canvas->draw_line(x1, y1, x2, y2);
						}
						else
						{
							number++;
						}

						x1 = x2;
						y1 = y2;
					}

					canvas->set_line_width(1);
//printf("\n");
				}

			}


// Recompute probe level
			window->calculate_frequency(window->probe_x, window->probe_y, 0);

			canvas->draw_overlay();
			canvas->flash();
		}

		while(frame_buffer.size() > MAX_COLUMNS)
			frame_buffer.remove_object_number(0);

		window->unlock_window();
	}
}

void Spectrogram::render_gui(void *data, int size)
{
	if(thread)
	{
		thread->get_window()->lock_window("Spectrogram::render_gui");
		data_header_t *header = (data_header_t*)data;
		memcpy(&this->header, header, sizeof(data_header_t));
		BC_SubWindow *canvas = ((SpectrogramWindow*)thread->get_window())->canvas;
		int pixels = canvas->get_w();
		if(config.mode == HORIZONTAL) pixels = canvas->get_h();

// Set all previous columns to draw immediately
		for(int i = 0; i < frame_buffer.size(); i++)
			frame_buffer.get(i)->force = 1;


		for(int current_window = 0;
			current_window < header->total_windows;
			current_window++)
		{
			float *frame = header->samples +
				current_window * (header->window_size / 2 + 1);
			float frame_max = *frame;
			frame++;
			int niquist = get_project_samplerate() / 2;
			int total_slots = header->window_size / 2;
			int max_slot = total_slots - 1;
//			int slot1 = total_slots - 1;
			SpectrogramFrame *ptr =
				new SpectrogramFrame(
					pixels);

// Scale slots to pixels
			for(int i = 0; i < pixels; i++)
			{
// Low frequency of row
				int freq_index1 = (int)((pixels - i) * TOTALFREQS / pixels);
// High frequency of row
				int freq_index2 = (int)((pixels - i + 1) * TOTALFREQS / pixels);
				int freq1 = Freq::tofreq(freq_index1);
				int freq2 = Freq::tofreq(freq_index2);
				float slot1_f = (float)freq1 * max_slot / niquist;
				float slot2_f = (float)freq2 * max_slot / niquist;
				int slot1 = (int)(slot1_f);
				int slot2 = (int)(slot2_f);
				slot1 = MIN(slot1, max_slot);
				slot2 = MIN(slot2, max_slot);
				double sum = 0;

// Accumulate multiple slots in the same pixel
				if(slot2 > slot1 + 1)
				{
					for(int j = slot1; j <= slot2; j++)
						sum += frame[j];

					sum /= slot2 - slot1 + 1;
				}
				else
// Blend 2 slots to create pixel
				{
					float weight = slot1_f - floor(slot1_f);
					int slot3 = MIN(slot1 + 1, max_slot);
					sum = frame[slot1] * (1.0 - weight) +
						frame[slot3] * weight;
				}

				ptr->data[i] = sum;
//				slot1 = slot2;
			}

// Normalize
			if(config.normalize)
			{
// Get the maximum level in the spectrogram
				float max = 0;
				for(int i = 0; i < pixels; i++)
				{
					if(ptr->data[i] > max) max = ptr->data[i];
				}

// Scale all levels
				for(int i = 0; i < pixels; i++)
				{
					ptr->data[i] = header->level *
						ptr->data[i] /
						max;
				}
			}
			else
			{
// Get the maximum level in the spectrogram
				float max = 0;
				for(int i = 0; i < pixels; i++)
				{
					if(ptr->data[i] > max) max = ptr->data[i];
				}

// Maximum level in spectrogram is the maximum waveform level
				for(int i = 0; i < pixels; i++)
				{
					ptr->data[i] = header->level *
						frame_max *
						ptr->data[i] /
						max;
				}


// 				for(int i = 0; i < pixels; i++)
// 				{
// 					ptr->data[i] = header->level * ptr->data[i];
// 				}
			}

// DB conversion
//printf("Spectrogram::render_gui %d ", __LINE__);
			for(int i = 0; i < pixels; i++)
			{
				ptr->data[i] = DB::todb(ptr->data[i]);
//if(i > pixels - 10) printf("%.02f ", ptr->data[i]);

			}
//printf("\n");


			frame_buffer.append(ptr);
			total_windows++;
		}

		timer->update();
		thread->get_window()->unlock_window();
	}
}

LOAD_CONFIGURATION_MACRO(Spectrogram, SpectrogramConfig)

void Spectrogram::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SPECTROGRAM"))
			{
				config.level = input.tag.get_property("LEVEL", config.level);
				config.normalize = input.tag.get_property("NORMALIZE", config.normalize);
				config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
				config.xzoom = input.tag.get_property("XZOOM", config.xzoom);
				config.mode = input.tag.get_property("MODE", config.mode);
				config.history_size = input.tag.get_property("HISTORY_SIZE", config.history_size);
				if(is_defaults())
				{
					w = input.tag.get_property("W", w);
					h = input.tag.get_property("H", h);
				}
			}
		}
	}
}

void Spectrogram::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SPECTROGRAM");
	output.tag.set_property("LEVEL", (double)config.level);
	output.tag.set_property("NORMALIZE", (double)config.normalize);
	output.tag.set_property("WINDOW_SIZE", (int)config.window_size);
	output.tag.set_property("XZOOM", (int)config.xzoom);
	output.tag.set_property("MODE", (int)config.mode);
	output.tag.set_property("HISTORY_SIZE", (int)config.history_size);
	output.tag.set_property("W", (int)w);
	output.tag.set_property("H", (int)h);
	output.append_tag();
	output.tag.set_title("/SPECTROGRAM");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}





