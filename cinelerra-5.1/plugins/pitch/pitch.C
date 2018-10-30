
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
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "pitch.h"
#include "samples.h"
#include "theme.h"
#include "units.h"
#include "vframe.h"

#include <math.h>
#include <string.h>


// It needs to be at least 40Hz yet high enough to have enough precision
#define WINDOW_SIZE 2048


//#define WINDOW_SIZE 131072

REGISTER_PLUGIN(PitchEffect);





PitchEffect::PitchEffect(PluginServer *server)
 : PluginAClient(server)
{

	reset();
}

PitchEffect::~PitchEffect()
{


	if(fft) delete fft;
}

const char* PitchEffect::plugin_title() { return N_("Pitch shift"); }
int PitchEffect::is_realtime() { return 1; }



void PitchEffect::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("PITCH"))
			{
				config.scale = input.tag.get_property("SCALE", config.scale);
				config.size = input.tag.get_property("SIZE", config.size);
			}
		}
	}
}

void PitchEffect::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("PITCH");
	output.tag.set_property("SCALE", config.scale);
	output.tag.set_property("SIZE", config.size);
	output.append_tag();
	output.tag.set_title("/PITCH");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}



LOAD_CONFIGURATION_MACRO(PitchEffect, PitchConfig)

NEW_WINDOW_MACRO(PitchEffect, PitchWindow)



void PitchEffect::reset()
{
	fft = 0;
}

void PitchEffect::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("PitchEffect::update_gui");
			((PitchWindow*)thread->window)->update();
			thread->window->unlock_window();
		}
	}
}



int PitchEffect::process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate)
{
//printf("PitchEffect::process_buffer %d\n", __LINE__);
	load_configuration();

	if(fft && config.size != fft->window_size)
	{
		delete fft;
		fft = 0;
	}

	if(!fft)
	{
		fft = new PitchFFT(this);
		fft->initialize(config.size);
	}

	fft->process_buffer(start_position,
		size,
		buffer,
		get_direction());

	return 0;
}









PitchFFT::PitchFFT(PitchEffect *plugin)
 : CrossfadeFFT()
{
	this->plugin = plugin;
	last_phase = 0;
	new_freq = 0;
	new_magn = 0;
	sum_phase = 0;
	anal_magn = 0;
	anal_freq = 0;
}

PitchFFT::~PitchFFT()
{
	delete [] last_phase;
	delete [] new_freq;
	delete [] new_magn;
	delete [] sum_phase;
	delete [] anal_magn;
	delete [] anal_freq;
}


int PitchFFT::signal_process()
{
#if 0
	double scale = plugin->config.scale;
	double oversample = 1.0;

	if(!new_freq)
	{
		last_phase = new double[window_size];
		new_freq = new double[window_size];
		new_magn = new double[window_size];
		sum_phase = new double[window_size];
		anal_magn = new double[window_size];
		anal_freq = new double[window_size];
		memset (last_phase, 0, window_size * sizeof(double));
		memset (sum_phase, 0, window_size * sizeof(double));
	}

	memset(new_freq, 0, window_size * sizeof(double));
	memset(new_magn, 0, window_size * sizeof(double));

// expected phase difference between windows
	double expected_phase_diff = 2.0 * M_PI / oversample;
// frequency per bin
	double freq_per_bin = (double)plugin->PluginAClient::project_sample_rate / window_size;

	for (int i = 0; i < window_size / 2; i++)
	{
// Convert to magnitude and phase
		double magn = sqrt(freq_real[i] * freq_real[i] + freq_imag[i] * freq_imag[i]);
		double phase = atan2(freq_imag[i], freq_real[i]);

// Remember last phase
		double temp = phase - last_phase[i];
		last_phase[i] = phase;

// Substract the expected advancement of phase
		temp -= (double)i * expected_phase_diff;


// wrap temp into -/+ PI ...  good trick!
		int qpd = (int)(temp/M_PI);
		if (qpd >= 0)
			qpd += qpd & 1;
		else
			qpd -= qpd & 1;
		temp -= M_PI * (double)qpd;

// Deviation from bin frequency
		temp = oversample * temp / (2.0 * M_PI);

		temp = (double)(temp + i) * freq_per_bin;

// Now temp is the real freq... move it!
		int new_bin = (int)(i * scale);
		if (new_bin >= 0 && new_bin < window_size / 2)
		{
			new_freq[new_bin] = temp*scale;
			new_magn[new_bin] += magn;
		}

	}




// Synthesize back the fft window
	for (int i = 0; i < window_size / 2; i++)
	{
		double magn = new_magn[i];
		double temp = new_freq[i];
// substract the bin frequency
		temp -= (double)(i) * freq_per_bin;

// get bin deviation from freq deviation
		temp /= freq_per_bin;

// oversample
		temp = 2.0 * M_PI * temp / oversample;

// add back the expected phase difference (that we substracted in analysis)
		temp += (double)(i) * expected_phase_diff;

// accumulate delta phase, to get bin phase
		sum_phase[i] += temp;

		double phase = sum_phase[i];

		freq_real[i] = magn * cos(phase);
		freq_imag[i] = magn * sin(phase);
	}

	for (int i = window_size / 2; i < window_size; i++)
	{
		freq_real[i] = 0;
		freq_imag[i] = 0;
	}
#endif


#if 1
	int min_freq =
		1 + (int)(20.0 / ((double)plugin->PluginAClient::project_sample_rate /
			window_size * 2) + 0.5);
	if(plugin->config.scale < 1)
	{
		for(int i = min_freq; i < window_size / 2; i++)
		{
			double destination = i * plugin->config.scale;
			int dest_i = (int)(destination + 0.5);
			if(dest_i != i)
			{
				if(dest_i <= window_size / 2)
				{
					freq_real[dest_i] = freq_real[i];
					freq_imag[dest_i] = freq_imag[i];
				}
				freq_real[i] = 0;
				freq_imag[i] = 0;
			}
		}
	}
	else
	if(plugin->config.scale > 1)
	{
		for(int i = window_size / 2 - 1; i >= min_freq; i--)
		{
			double destination = i * plugin->config.scale;
			int dest_i = (int)(destination + 0.5);
			if(dest_i != i)
			{
				if(dest_i <= window_size / 2)
				{
					freq_real[dest_i] = freq_real[i];
					freq_imag[dest_i] = freq_imag[i];
				}
				freq_real[i] = 0;
				freq_imag[i] = 0;
			}
		}
	}

	symmetry(window_size, freq_real, freq_imag);
#endif


	return 0;
}

int PitchFFT::read_samples(int64_t output_sample,
	int samples,
	Samples *buffer)
{
	return plugin->read_samples(buffer,
		0,
		plugin->get_samplerate(),
		output_sample,
		samples);
}







PitchConfig::PitchConfig()
{
	scale = 1.0;
	size = 2048;
}

int PitchConfig::equivalent(PitchConfig &that)
{
	return EQUIV(scale, that.scale) && size == that.size;
}

void PitchConfig::copy_from(PitchConfig &that)
{
	scale = that.scale;
	size = that.size;
}

void PitchConfig::interpolate(PitchConfig &prev,
	PitchConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	scale = prev.scale * prev_scale + next.scale * next_scale;
	size = prev.size;
}

















PitchWindow::PitchWindow(PitchEffect *plugin)
 : PluginClientWindow(plugin,
	150,
	100,
	150,
	100,
	0)
{
	this->plugin = plugin;
}

void PitchWindow::create_objects()
{
	int x1 = 10, x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Scale:")));
	x += title->get_w() + plugin->get_theme()->widget_border;
	add_subwindow(scale = new PitchScale(plugin, x, y));
	x = x1;
	y += scale->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Window Size:")));
	y += title->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(size = new PitchSize(this, plugin, x, y));
	size->create_objects();
	size->update(plugin->config.size);
	show_window();
	flush();
}



void PitchWindow::update()
{
	scale->update(plugin->config.scale);
	size->update(plugin->config.size);
}












PitchScale::PitchScale(PitchEffect *plugin, int x, int y)
 : BC_FPot(x, y, (float)plugin->config.scale, .5, 1.5)
{
	this->plugin = plugin;
	set_precision(0.01);
}

int PitchScale::handle_event()
{
	plugin->config.scale = get_value();
	plugin->send_configure_change();
	return 1;
}



PitchSize::PitchSize(PitchWindow *window, PitchEffect *plugin, int x, int y)
 : BC_PopupMenu(x, y, 100, "4096", 1)
{
	this->plugin = plugin;
}

int PitchSize::handle_event()
{
	plugin->config.size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

void PitchSize::create_objects()
{
	add_item(new BC_MenuItem("2048"));
	add_item(new BC_MenuItem("4096"));
	add_item(new BC_MenuItem("8192"));
	add_item(new BC_MenuItem("16384"));
	add_item(new BC_MenuItem("32768"));
	add_item(new BC_MenuItem("65536"));
	add_item(new BC_MenuItem("131072"));
	add_item(new BC_MenuItem("262144"));
}

void PitchSize::update(int size)
{
	char string[BCTEXTLEN];
	sprintf(string, "%d", size);
	set_text(string);
}














