
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

#include "audioscope.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "bccolors.h"
#include "samples.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"


#include <string.h>



REGISTER_PLUGIN(AudioScope)


AudioScopeConfig::AudioScopeConfig()
{
	window_size = MAX_WINDOW;
	history_size = 4;
	mode = XY_MODE;
	trigger_level = 0;
}

int AudioScopeConfig::equivalent(AudioScopeConfig &that)
{
	return window_size == that.window_size &&
		history_size == that.history_size &&
		mode == that.mode &&
		EQUIV(trigger_level, that.trigger_level);
}

void AudioScopeConfig::copy_from(AudioScopeConfig &that)
{
	window_size = that.window_size;
	history_size = that.history_size;
	mode = that.mode;
	trigger_level = that.trigger_level;
}

void AudioScopeConfig::interpolate(AudioScopeConfig &prev,
	AudioScopeConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(prev);

	CLAMP(history_size, MIN_HISTORY, MAX_HISTORY - 1);
}



AudioScopeFrame::AudioScopeFrame(int data_size, int channels)
{
	this->size = data_size;
	this->channels = channels;
	for(int i = 0; i < CHANNELS; i++)
		data[i] = new float[data_size];
	force = 0;
}

AudioScopeFrame::~AudioScopeFrame()
{
	for(int i = 0; i < CHANNELS; i++)
		delete [] data[i];
}







AudioScopeHistory::AudioScopeHistory(AudioScope *plugin,
	int x,
	int y)
 : BC_IPot(x,
	y,
	plugin->config.history_size,
	MIN_HISTORY,
	MAX_HISTORY)
{
	this->plugin = plugin;
}

int AudioScopeHistory::handle_event()
{
	plugin->config.history_size = get_value();
	plugin->send_configure_change();
	return 1;
}








AudioScopeWindowSize::AudioScopeWindowSize(AudioScope *plugin,
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

int AudioScopeWindowSize::handle_event()
{
	plugin->config.window_size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}


AudioScopeWindowSizeTumbler::AudioScopeWindowSizeTumbler(AudioScope *plugin, int x, int y)
 : BC_Tumbler(x,
 	y)
{
	this->plugin = plugin;
}

int AudioScopeWindowSizeTumbler::handle_up_event()
{
	plugin->config.window_size *= 2;
	if(plugin->config.window_size > MAX_WINDOW)
		plugin->config.window_size = MAX_WINDOW;
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.window_size);
	((AudioScopeWindow*)plugin->get_thread()->get_window())->window_size->set_text(string);
	plugin->send_configure_change();
	return 1;
}

int AudioScopeWindowSizeTumbler::handle_down_event()
{
	plugin->config.window_size /= 2;
	if(plugin->config.window_size < MIN_WINDOW)
		plugin->config.window_size = MIN_WINDOW;
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.window_size);
	((AudioScopeWindow*)plugin->get_thread()->get_window())->window_size->set_text(string);
	plugin->send_configure_change();
	return 1;
}





AudioScopeCanvas::AudioScopeCanvas(AudioScope *plugin,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	current_operation = NONE;
}

int AudioScopeCanvas::button_press_event()
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

int AudioScopeCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		current_operation = NONE;
		return 1;
	}
	return 0;
}

int AudioScopeCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		calculate_point();
		return 1;
	}
	return 0;
}


void AudioScopeCanvas::calculate_point()
{
	int x = get_cursor_x();
	int y = get_cursor_y();
	CLAMP(x, 0, get_w() - 1);
	CLAMP(y, 0, get_h() - 1);

	((AudioScopeWindow*)plugin->thread->window)->calculate_probe(
		x,
		y,
		1);

//printf("AudioScopeCanvas::calculate_point %d %d\n", __LINE__, Freq::tofreq(freq_index));
}







AudioScopeTriggerLevel::AudioScopeTriggerLevel(AudioScope *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.trigger_level, (float)-1.0, (float)1.0)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int AudioScopeTriggerLevel::handle_event()
{
	AudioScopeWindow *window = (AudioScopeWindow*)plugin->thread->window;
	window->draw_trigger();
	plugin->config.trigger_level = get_value();
	plugin->send_configure_change();
	window->draw_trigger();
	window->canvas->flash();
	return 1;
}





AudioScopeMode::AudioScopeMode(AudioScope *plugin,
	int x,
	int y)
 : BC_PopupMenu(x,
	y,
	180,
	mode_to_text(plugin->config.mode))
{
	this->plugin = plugin;
}

void AudioScopeMode::create_objects()
{
	add_item(new BC_MenuItem(mode_to_text(XY_MODE)));
	add_item(new BC_MenuItem(mode_to_text(WAVEFORM_NO_TRIGGER)));
	add_item(new BC_MenuItem(mode_to_text(WAVEFORM_RISING_TRIGGER)));
	add_item(new BC_MenuItem(mode_to_text(WAVEFORM_FALLING_TRIGGER)));
}

int AudioScopeMode::handle_event()
{
	if(plugin->config.mode != text_to_mode(get_text()))
	{
		AudioScopeWindow *window = (AudioScopeWindow*)plugin->thread->window;
		window->probe_x = -1;
		window->probe_y = -1;
		plugin->config.mode = text_to_mode(get_text());
		plugin->send_configure_change();

		window->canvas->clear_box(0, 0, window->canvas->get_w(), window->canvas->get_h());
		window->draw_overlay();
		window->canvas->flash();
	}
	return 1;
}

const char* AudioScopeMode::mode_to_text(int mode)
{
	switch(mode)
	{
		case XY_MODE:
			return _("XY Mode");
		case WAVEFORM_NO_TRIGGER:
			return _("Waveform");
		case WAVEFORM_RISING_TRIGGER:
			return _("Rising Trigger");
		case WAVEFORM_FALLING_TRIGGER:
		default:
			return _("Falling Trigger");
	}
}

int AudioScopeMode::text_to_mode(const char *text)
{
	if(!strcmp(mode_to_text(XY_MODE), text)) return XY_MODE;
	if(!strcmp(mode_to_text(WAVEFORM_NO_TRIGGER), text)) return WAVEFORM_NO_TRIGGER;
	if(!strcmp(mode_to_text(WAVEFORM_RISING_TRIGGER), text)) return WAVEFORM_RISING_TRIGGER;
	if(!strcmp(mode_to_text(WAVEFORM_FALLING_TRIGGER), text)) return WAVEFORM_FALLING_TRIGGER;
	return XY_MODE;
}









AudioScopeWindow::AudioScopeWindow(AudioScope *plugin)
 : PluginClientWindow(plugin,
	plugin->w,
	plugin->h,
	320,
	320,
	1)
{
	this->plugin = plugin;
}

AudioScopeWindow::~AudioScopeWindow()
{
}

void AudioScopeWindow::create_objects()
{
	int x = plugin->get_theme()->widget_border;
	int y = 0;
	char string[BCTEXTLEN];

	add_subwindow(canvas = new AudioScopeCanvas(plugin,
		0,
		0,
		get_w(),
		get_h() -
			plugin->get_theme()->widget_border * 4 -
			BC_Title::calculate_h(this, "X") * 3));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);
	draw_overlay();

	y += canvas->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(history_size_title = new BC_Title(x, y, _("History Size:")));
	x += history_size_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(history_size = new AudioScopeHistory(plugin,
		x,
		y));
	x += history_size->get_w() + plugin->get_theme()->widget_border;


	int x1 = x;
	sprintf(string, "%d", plugin->config.window_size);
	add_subwindow(window_size_title = new BC_Title(x, y, _("Window Size:")));

	x += window_size_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(window_size = new AudioScopeWindowSize(plugin, x, y, string));
	x += window_size->get_w();
	add_subwindow(window_size_tumbler = new AudioScopeWindowSizeTumbler(plugin, x, y));

	for(int i = MIN_WINDOW; i <= MAX_WINDOW; i *= 2)
	{
		sprintf(string, "%d", i);
		window_size->add_item(new BC_MenuItem(string));
	}

	x += window_size_tumbler->get_w() + plugin->get_theme()->widget_border;

	int y1 = y;
	x = x1;
	y += window_size->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(mode_title = new BC_Title(x, y, _("Mode:")));

	x += mode_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(mode = new AudioScopeMode(plugin, x, y));
	mode->create_objects();

	x += mode->get_w() + plugin->get_theme()->widget_border;
	y = y1;
	add_subwindow(trigger_level_title = new BC_Title(x, y, _("Trigger level:")));

	x += trigger_level_title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(trigger_level = new AudioScopeTriggerLevel(plugin, x, y));


	x += trigger_level->get_w() + plugin->get_theme()->widget_border;
	y = y1;
	add_subwindow(probe_sample = new BC_Title(x, y, _("Sample: 0")));
	y += probe_sample->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(probe_level0 = new BC_Title(x, y, _("Level 0: 0"), MEDIUMFONT, CHANNEL0_COLOR));
	y += probe_level0->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(probe_level1 = new BC_Title(x, y, _("Level 1: 0"), MEDIUMFONT, CHANNEL1_COLOR));
	y += probe_level1->get_h() + plugin->get_theme()->widget_border;



	show_window();
}

int AudioScopeWindow::resize_event(int w, int h)
{
	int y_diff = h - get_h();

	int canvas_factor = get_h() - canvas->get_h();
	canvas->reposition_window(0,
		0,
		w,
		h - canvas_factor);
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	draw_overlay();
	canvas->flash(0);


// Remove all columns which may be a different size.
	plugin->frame_buffer.remove_all_objects();

	window_size_title->reposition_window(window_size_title->get_x(),
		window_size_title->get_y() + y_diff);
	window_size->reposition_window(window_size->get_x(),
		window_size->get_y() + y_diff);
	window_size_tumbler->reposition_window(window_size_tumbler->get_x(),
		window_size_tumbler->get_y() + y_diff);



	history_size_title->reposition_window(history_size_title->get_x(),
		history_size_title->get_y() + y_diff);
	history_size->reposition_window(history_size->get_x(),
		history_size->get_y() + y_diff);

	mode_title->reposition_window(mode_title->get_x(),
		mode_title->get_y() + y_diff);
	mode->reposition_window(mode->get_x(),
		mode->get_y() + y_diff);


	trigger_level_title->reposition_window(trigger_level_title->get_x(),
		trigger_level_title->get_y() + y_diff);
	trigger_level->reposition_window(trigger_level->get_x(),
		trigger_level->get_y() + y_diff);


	probe_sample->reposition_window(probe_sample->get_x(),
		probe_sample->get_y() + y_diff);
	probe_level0->reposition_window(probe_level0->get_x(),
		probe_level0->get_y() + y_diff);
	probe_level1->reposition_window(probe_level1->get_x(),
		probe_level1->get_y() + y_diff);

	flush();

	plugin->w = w;
	plugin->h = h;
	return 0;
}

void AudioScopeWindow::draw_overlay()
{
	canvas->set_color(GREEN);

	canvas->draw_line(0,
		canvas->get_h() / 2,
		canvas->get_w(),
		canvas->get_h() / 2);
	if(plugin->config.mode == XY_MODE)
	{
		canvas->draw_line(canvas->get_w() / 2,
			0,
			canvas->get_w() / 2,
			canvas->get_h());
	}

	draw_trigger();
	draw_probe();
//printf("AudioScopeWindow::draw_overlay %d\n", __LINE__);
}

void AudioScopeWindow::draw_trigger()
{
	if(plugin->config.mode == WAVEFORM_RISING_TRIGGER ||
		plugin->config.mode == WAVEFORM_FALLING_TRIGGER)
	{
		int y = (int)(-plugin->config.trigger_level *
			canvas->get_h() / 2 +
			canvas->get_h() / 2);
		CLAMP(y, 0, canvas->get_h() - 1);
		canvas->set_inverse();
		canvas->set_color(RED);
		canvas->draw_line(0, y, canvas->get_w(), y);
		canvas->set_opaque();
	}
}

void AudioScopeWindow::draw_probe()
{
	if(probe_x >= 0 || probe_y >= 0)
	{
		canvas->set_color(GREEN);
		canvas->set_inverse();

		if(plugin->config.mode == XY_MODE)
		{
			canvas->draw_line(0, probe_y, get_w(), probe_y);
			canvas->draw_line(probe_x, 0, probe_x, get_h());
		}
		else
		{
			canvas->draw_line(probe_x, 0, probe_x, get_h());
		}

		canvas->set_opaque();
	}
}




void AudioScopeWindow::calculate_probe(int x, int y, int do_overlay)
{
	if(x < 0 && y < 0) return;

// Clear previous overlay
	if(do_overlay) draw_probe();

// New probe position
	probe_x = x;
	probe_y = y;

	int sample = 0;
	double channel0 = 0;
	double channel1 = 0;
	if(plugin->config.mode == XY_MODE)
	{
		channel0 = (float)(probe_x - canvas->get_w() / 2) / (canvas->get_w() / 2);
		channel1 = (float)(canvas->get_h() / 2 - probe_y) / (canvas->get_h() / 2);
	}
	else
	if(plugin->current_frame)
	{
		sample = probe_x * plugin->current_frame->size / canvas->get_w();
		CLAMP(sample, 0, plugin->current_frame->size - 1);
		channel0 = plugin->current_frame->data[0][sample];
		channel1 = plugin->current_frame->data[1][sample];
	}


	char string[BCTEXTLEN];
	sprintf(string, _("Sample: %d"), sample);
	probe_sample->update(string);

	sprintf(string, _("Level 0: %.2f"), channel0);
	probe_level0->update(string);

	sprintf(string, _("Level 1: %.2f"), channel1);
	probe_level1->update(string);


	if(do_overlay)
	{
		draw_probe();
		canvas->flash();
	}
}





void AudioScopeWindow::update_gui()
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", plugin->config.window_size);
	window_size->set_text(string);
	history_size->update(plugin->config.history_size);
}

























AudioScope::AudioScope(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	timer = new Timer;
	w = 640;
	h = 480;
}

AudioScope::~AudioScope()
{
	delete [] data;
	for(int i = 0; i < CHANNELS; i++)
		delete audio_buffer[i];
	delete timer;
	frame_buffer.remove_all_objects();
	frame_history.remove_all_objects();
}


void AudioScope::reset()
{
	thread = 0;
	data = 0;
	for(int i = 0; i < CHANNELS; i++)
		audio_buffer[i] = 0;
	allocated_data = 0;
	total_windows = 0;
	buffer_size = 0;
	bzero(&header, sizeof(data_header_t));
	current_frame = 0;
}


const char* AudioScope::plugin_title() { return N_("AudioScope"); }
int AudioScope::is_realtime() { return 1; }
int AudioScope::is_multichannel() { return 1; }

int AudioScope::process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate)
{
	int channels = MIN(get_total_buffers(), CHANNELS);

// Pass through
	for(int i = 0; i < get_total_buffers(); i++)
	{
		read_samples(buffer[i],
			i,
			sample_rate,
			start_position,
			size);
	}

	load_configuration();

// Reset audio buffer
	if(window_size != config.window_size)
	{
		render_stop();
		window_size = config.window_size;
	}

	if(!data)
	{
		data = new unsigned char[sizeof(data_header_t)];
		allocated_data = 0;
	}

	if(!audio_buffer[0])
	{
		for(int i = 0; i < CHANNELS; i++)
		{
			audio_buffer[i] = new Samples(MAX_WINDOW);
		}
	}

// Accumulate audio
	int needed = buffer_size + size;
	if(audio_buffer[0]->get_allocated() < needed)
	{
		for(int i = 0; i < CHANNELS; i++)
		{
			Samples *new_samples = new Samples(needed);
			memcpy(new_samples->get_data(),
				audio_buffer[i]->get_data(),
				sizeof(double) * buffer_size);
			delete audio_buffer[i];
			audio_buffer[i] = new_samples;
		}
	}

	for(int i = 0; i < CHANNELS; i++)
	{
		memcpy(audio_buffer[i]->get_data() + buffer_size,
			buffer[MIN(i, channels - 1)]->get_data(),
			sizeof(double) * size);
	}
	buffer_size += size;


// Append a windows to end of GUI buffer
	total_windows = 0;
	while(buffer_size >= window_size)
	{
		if(allocated_data < (total_windows + 1) * window_size * CHANNELS)
		{
			int new_allocation = (total_windows + 1) *
				window_size *
				CHANNELS;
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

// Search for trigger
		int need_trigger = 0;
		int got_trigger = 0;
		int trigger_sample = 0;
//printf("AudioScope::process_buffer %d\n", __LINE__);
		if(config.mode == WAVEFORM_RISING_TRIGGER)
		{
			need_trigger = 1;
			double *trigger_data = audio_buffer[0]->get_data();
			for(int i = 1; i < buffer_size - window_size; i++)
			{
				if(trigger_data[i - 1] < config.trigger_level &&
					trigger_data[i] >= config.trigger_level)
				{
					got_trigger = 1;
					trigger_sample = i;
					break;
				}
			}
		}
		else
		if(config.mode == WAVEFORM_FALLING_TRIGGER)
		{
			need_trigger = 1;
			double *trigger_data = audio_buffer[0]->get_data();
			for(int i = 1; i < buffer_size - window_size; i++)
			{
				if(trigger_data[i - 1] > config.trigger_level &&
					trigger_data[i] <= config.trigger_level)
				{
					got_trigger = 1;
					trigger_sample = i;
					break;
				}
			}
		}

//printf("AudioScope::process_buffer %d\n", __LINE__);
// Flush buffer
		if(need_trigger && !got_trigger)
		{
//printf("AudioScope::process_buffer %d\n", __LINE__);
			for(int j = 0; j < CHANNELS; j++)
			{
				double *sample_input = audio_buffer[j]->get_data();
				memcpy(sample_input,
					sample_input + buffer_size - window_size,
					sizeof(double) * window_size);
			}

			buffer_size = window_size;
			if(buffer_size == window_size) break;
//printf("AudioScope::process_buffer %d\n", __LINE__);
		}
		else
		{
			data_header_t *header = (data_header_t*)data;
//printf("AudioScope::process_buffer %d\n", __LINE__);
			for(int j = 0; j < CHANNELS; j++)
			{
				float *sample_output = header->samples +
					total_windows * CHANNELS * window_size +
					window_size * j;
				double *sample_input = audio_buffer[j]->get_data();
				for(int i = 0; i < window_size; i++)
				{
					sample_output[i] = sample_input[i + trigger_sample];
				}

// Shift accumulation buffer
//printf("AudioScope::process_buffer %d\n", __LINE__);
				memcpy(sample_input,
					sample_input + trigger_sample + window_size,
					sizeof(double) * (buffer_size - trigger_sample - window_size));
//printf("AudioScope::process_buffer %d\n", __LINE__);
			}

//printf("AudioScope::process_buffer %d\n", __LINE__);

			total_windows++;
			buffer_size -= window_size + trigger_sample;
//printf("AudioScope::process_buffer %d\n", __LINE__);
		}
	}

	data_header_t *header = (data_header_t*)data;
	header->window_size = window_size;
	header->sample_rate = sample_rate;
	header->channels = channels;
	header->total_windows = total_windows;

	send_render_gui(data,
		sizeof(data_header_t) +
			sizeof(float) * total_windows * window_size * CHANNELS);

	return 0;
}

void AudioScope::render_stop()
{
	buffer_size = 0;
	frame_buffer.remove_all_objects();
}




NEW_WINDOW_MACRO(AudioScope, AudioScopeWindow)

void AudioScope::update_gui()
{
	if(thread)
	{
		int result = load_configuration();
		AudioScopeWindow *window = (AudioScopeWindow*)thread->get_window();
		window->lock_window("AudioScope::update_gui");
		if(result) window->update_gui();

// Shift in accumulated frames
		if(frame_buffer.size())
		{
			BC_SubWindow *canvas = window->canvas;
// Frames to draw in this iteration
			int total_frames = timer->get_difference() *
				header.sample_rate /
				header.window_size /
				1000;
			if(total_frames) timer->subtract(total_frames *
				header.window_size *
				1000 /
				header.sample_rate);

// Add forced frame drawing
			for(int i = 0; i < frame_buffer.size(); i++)
				if(frame_buffer.get(i)->force) total_frames++;
			total_frames = MIN(frame_buffer.size(), total_frames);
//printf("AudioScope::update_gui %d %d\n", __LINE__, total_frames);



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

// Point probe data at history
			current_frame = frame_history.get(frame_history.size() - 1);

// Draw frames from history
			canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
			for(int frame = 0; frame < frame_history.size(); frame++)
			{
				AudioScopeFrame *ptr = frame_history.get(frame);

				int luma = (frame + 1) * 0x80 / frame_history.size();
				if(frame == frame_history.size() - 1)
				{
					canvas->set_color(WHITE);
					canvas->set_line_width(2);
				}
				else
				{
					canvas->set_color((luma << 16) |
						(luma << 8) |
						luma);
				}

				int x1 = 0;
				int y1 = 0;
				int w = canvas->get_w();
				int h = canvas->get_h();
				float *channel0 = ptr->data[0];
				float *channel1 = ptr->data[1];

				if(config.mode == XY_MODE)
				{

					int number = 0;
					for(int point = 0; point < ptr->size; point++)
					{
						int x2 = (int)(channel0[point] * w / 2 + w / 2);
						int y2 = (int)(-channel1[point] * h / 2 + h / 2);
						CLAMP(x2, 0, w - 1);
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
				}
				else
				{
					for(int channel = ptr->channels - 1; channel >= 0; channel--)
					{
						if(channel > 0)
						{
							if(frame == frame_history.size() - 1)
							{
								canvas->set_color(PINK);
								canvas->set_line_width(2);
							}
							else
								canvas->set_color(((luma * 0xff / 0xff) << 16) |
									((luma * 0x80 / 0xff) << 8) |
									((luma * 0x80 / 0xff)));
						}
						else
						{
							if(frame == frame_history.size() - 1)
								canvas->set_color(WHITE);
							else
								canvas->set_color((luma << 16) |
									(luma << 8) |
									luma);
						}

						if(ptr->size < w)
						{
							for(int point = 0; point < ptr->size; point++)
							{
								int x2 = point * w / ptr->size;
								int y2 = (int)(-ptr->data[channel][point] * h / 2 + h / 2);
								CLAMP(y2, 0, h - 1);
								if(point > 0)
								{
									canvas->draw_line(x1, y1, x2, y2);
								}

								x1 = x2;
								y1 = y2;
							}
						}
						else
						{
							for(int x2 = 0; x2 < w; x2++)
							{
								int sample1 = x2 * ptr->size / w;
								int sample2 = (x2 + 1) * ptr->size / w;
								double min = ptr->data[channel][sample1];
								double max = ptr->data[channel][sample1];
								for(int i = sample1 + 1; i < sample2; i++)
								{
									double value = ptr->data[channel][i];
									if(value < min) min = value;
									if(value > max) max = value;
								}

								int min2 = (int)(-min * h / 2 + h / 2);
								int max2 = (int)(-max * h / 2 + h / 2);
								CLAMP(min2, 0, h - 1);
								CLAMP(max2, 0, h - 1);
								int y2 = (min2 + max2) / 2;
								canvas->draw_line(x2, min2, x2, max2);
								if(x2 > 0)
								{
									canvas->draw_line(x2, y1, x2, y2);
								}

								y1 = y2;
							}
						}
					}
				}

				canvas->set_line_width(1);
			}

// Recompute probe level
			window->calculate_probe(window->probe_x, window->probe_y, 0);


// Draw overlay
			window->draw_overlay();
			canvas->flash();
		}



		while(frame_buffer.size() > MAX_COLUMNS)
			frame_buffer.remove_object_number(0);

		thread->get_window()->unlock_window();
	}
}

void AudioScope::render_gui(void *data, int size)
{
	if(thread)
	{
		thread->get_window()->lock_window("AudioScope::render_gui");
		data_header_t *header = (data_header_t*)data;
		memcpy(&this->header, header, sizeof(data_header_t));
		//BC_SubWindow *canvas = ((AudioScopeWindow*)thread->get_window())->canvas;
		//int h = canvas->get_h();

// Set all previous frames to draw immediately
		for(int i = 0; i < frame_buffer.size(); i++)
			frame_buffer.get(i)->force = 1;

		for(int current_fragment = 0;
			current_fragment < header->total_windows;
			current_fragment++)
		{
			float *in_frame = header->samples +
				current_fragment * header->window_size * CHANNELS;
			AudioScopeFrame *out_frame = new AudioScopeFrame(
				header->window_size,
				header->channels);

// Copy the window to the frame
			for(int j = 0; j < CHANNELS; j++)
			{
				for(int i = 0; i < header->window_size; i++)
				{
					out_frame->data[j][i] = in_frame[header->window_size * j + i];
				}
			}

			frame_buffer.append(out_frame);
			total_windows++;
		}

		timer->update();
		thread->get_window()->unlock_window();
	}
}








LOAD_CONFIGURATION_MACRO(AudioScope, AudioScopeConfig)

void AudioScope::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("AUDIOSCOPE"))
			{
				if(is_defaults())
				{
					w = input.tag.get_property("W", w);
					h = input.tag.get_property("H", h);
				}

				config.history_size = input.tag.get_property("HISTORY_SIZE", config.history_size);
				config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
				config.mode = input.tag.get_property("MODE", config.mode);
				config.trigger_level = input.tag.get_property("TRIGGER_LEVEL", config.trigger_level);
			}
		}
	}
}

void AudioScope::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

//printf("AudioScope::save_data %d %d %d\n", __LINE__, config.w, config.h);
	output.tag.set_title("AUDIOSCOPE");
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.tag.set_property("HISTORY_SIZE", config.history_size);
	output.tag.set_property("WINDOW_SIZE", (int)config.window_size);
	output.tag.set_property("MODE", (int)config.mode);
	output.tag.set_property("TRIGGER_LEVEL", (float)config.trigger_level);
	output.append_tag();
	output.tag.set_title("/AUDIOSCOPE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}





