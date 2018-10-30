
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


// Originally from the following:
/* vocoder.c
   Version 0.3

   LADSPA Unique ID: 1441

   Version 0.3
   Added support for changing bands in real time 2003-12-09

   Version 0.2
   Adapted to LADSPA by Josh Green <jgreen@users.sourceforge.net>
   15.6.2001 (for the LinuxTag 2001!)

   Original program can be found at:
   http://www.sirlab.de/linux/
   Author: Achim Settelmeier <settel-linux@sirlab.de>
*/



#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "theme.h"
#include "units.h"
#include "vframe.h"
#include "vocoder.h"

#include <math.h>
#include <string.h>









REGISTER_PLUGIN(Vocoder)




//#define USE_BANDWIDTH

const double decay_table[] =
{
  1/100.0,
  1/100.0, 1/100.0, 1/100.0,
  1/125.0, 1/125.0, 1/125.0,
  1/166.0, 1/166.0, 1/166.0,
  1/200.0, 1/200.0, 1/200.0,
  1/250.0, 1/250.0, 1/250.0
};


VocoderBand::VocoderBand()
{
	reset();
}

void VocoderBand::reset()
{
	c = f = att = 0;

	freq = 0;
	low1 = low2 = 0;
	mid1 = mid2 = 0;
	high1 = high2 = 0;
	y = 0;
}

void VocoderBand::copy_from(VocoderBand *src)
{
	c = src->c;
	f = src->f;
	att = src->att;

	freq = src->freq;
	low1 = src->low1;
	low2 = src->low2;
	mid1 = src->mid1;
	mid2 = src->mid2;
	high1 = src->high1;
	high2 = src->high2;
	y = src->y;
}


VocoderOut::VocoderOut()
{
	reset();
}

void VocoderOut::reset()
{
	decay = 0;
	oldval = 0;
	level = 0;  	 /* 0.0 - 1.0 level of this output band */
}


VocoderConfig::VocoderConfig()
{
	wetness = INFINITYGAIN;
	carrier_track = 0;
	bands = MAX_BANDS;
	level = 0.0;
}


int VocoderConfig::equivalent(VocoderConfig &that)
{
	if(!EQUIV(wetness, that.wetness) ||
		!EQUIV(level, that.level) ||
		carrier_track != that.carrier_track ||
		bands != that.bands) return 0;
	return 1;
}

void VocoderConfig::copy_from(VocoderConfig &that)
{
	wetness = that.wetness;
	carrier_track = that.carrier_track;
	bands = that.bands;
	level = that.level;
	CLAMP(bands, 1, MAX_BANDS);
}

void VocoderConfig::interpolate(VocoderConfig &prev,
		VocoderConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);
	wetness = prev.wetness * prev_scale + next.wetness * next_scale;
	level = prev.level * prev_scale + next.level * next_scale;
	carrier_track = prev.carrier_track;
	bands = prev.bands;
	CLAMP(bands, 1, MAX_BANDS);
}












VocoderWetness::VocoderWetness(Vocoder *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.wetness, INFINITYGAIN, 0)
{
	this->plugin = plugin;
}

int VocoderWetness::handle_event()
{
	plugin->config.wetness = get_value();
	plugin->send_configure_change();
	return 1;
}





VocoderLevel::VocoderLevel(Vocoder *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, INFINITYGAIN, 40)
{
	this->plugin = plugin;
}

int VocoderLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}









VocoderCarrier::VocoderCarrier(Vocoder *plugin,
	VocoderWindow *window,
	int x,
	int y)
 : BC_TumbleTextBox(window,
 	plugin->config.carrier_track,
	0,
	256,
	x,
	y,
	100)
{
	this->plugin = plugin;
}

int VocoderCarrier::handle_event()
{
	plugin->config.carrier_track = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}



VocoderBands::VocoderBands(Vocoder *plugin,
	VocoderWindow *window,
	int x,
	int y)
 : BC_TumbleTextBox(window,
 	plugin->config.bands,
	1,
	MAX_BANDS,
	x,
	y,
	100)
{
	this->plugin = plugin;
}

int VocoderBands::handle_event()
{
	plugin->config.bands = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}





VocoderWindow::VocoderWindow(Vocoder *plugin)
 : PluginClientWindow(plugin,
	320,
	150,
	320,
	150,
	0)
{
	this->plugin = plugin;
}

VocoderWindow::~VocoderWindow()
{
}

void VocoderWindow::create_objects()
{
	int x = plugin->get_theme()->widget_border;
	int y = plugin->get_theme()->widget_border;
	BC_Title *title = 0;

	int x1 = x;
	int y1 = y;
	add_subwindow(title = new BC_Title(x, y, _("Wetness:")));
	int x2 = x + title->get_w() + plugin->get_theme()->widget_border;
	y += BC_Pot::calculate_h() + plugin->get_theme()->widget_border;
	add_subwindow(title = new BC_Title(x, y, _("Level:")));
	x2 = MAX(x2, x + title->get_w() + plugin->get_theme()->widget_border);

	x = x2;
	y = y1;
	add_subwindow(wetness = new VocoderWetness(plugin, x, y));
	y += wetness->get_h() + plugin->get_theme()->widget_border;
	add_subwindow(level = new VocoderLevel(plugin, x, y));
	y += level->get_h() + plugin->get_theme()->widget_border;

	x = x1;
	add_subwindow(title = new BC_Title(x, y, _("Carrier Track:")));
	output = new VocoderCarrier(plugin,
		this,
		x + title->get_w() + plugin->get_theme()->widget_border,
		y);
	output->create_objects();
	y += output->get_h() + plugin->get_theme()->widget_border;

	add_subwindow(title = new BC_Title(x, y, _("Bands:")));
	bands = new VocoderBands(plugin,
		this,
		x + title->get_w() + plugin->get_theme()->widget_border,
		y);
	bands->create_objects();
	y += bands->get_h() + plugin->get_theme()->widget_border;

	show_window();
}



void VocoderWindow::update_gui()
{
	wetness->update(plugin->config.wetness);
	level->update(plugin->config.level);
	output->update((int64_t)plugin->config.carrier_track);
	bands->update((int64_t)plugin->config.bands);
}











Vocoder::Vocoder(PluginServer *server)
 : PluginAClient(server)
{
	need_reconfigure = 1;
	current_bands = 0;
}

Vocoder::~Vocoder()
{
}

NEW_WINDOW_MACRO(Vocoder, VocoderWindow)

LOAD_CONFIGURATION_MACRO(Vocoder, VocoderConfig)


const char* Vocoder::plugin_title() { return N_("Vocoder"); }
int Vocoder::is_realtime() { return 1; }
int Vocoder::is_multichannel() { return 1; }

void Vocoder::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("VOCODER"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);
				config.level = input.tag.get_property("LEVEL", config.level);
				config.carrier_track = input.tag.get_property("OUTPUT", config.carrier_track);
				config.bands = input.tag.get_property("BANDS", config.bands);
			}
		}
	}
}

void Vocoder::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("VOCODER");
	output.tag.set_property("WETNESS", config.wetness);
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("OUTPUT", config.carrier_track);
	output.tag.set_property("BANDS", config.bands);
	output.append_tag();
	output.tag.set_title("/VOCODER");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void Vocoder::reconfigure()
{
	need_reconfigure = 0;

	if(current_bands != config.bands)
	{
		current_bands = config.bands;
		for(int i = 0; i < config.bands; i++)
		{
			formant_bands[i].reset();
			double a = 16.0 * i / (double)current_bands;

			if(a < 4.0)
				formant_bands[i].freq = 150 + 420 * a / 4.0;
			else
				formant_bands[i].freq = 600 * pow (1.23, a - 4.0);

		  	double c = formant_bands[i].freq * 2 * M_PI / get_samplerate();
		  	formant_bands[i].c = c * c;

		  	formant_bands[i].f = 0.4 / c;
		  	formant_bands[i].att =
	    		1  / (6.0 + ((exp (formant_bands[i].freq
				    / get_samplerate()) - 1) * 10));

		  carrier_bands[i].copy_from(&formant_bands[i]);

		  output_bands[i].decay = decay_table[(int)a];
		}
	}
}

void Vocoder::do_bandpasses(VocoderBand *bands, double sample)
{
  	for(int i = 0; i < current_bands; i++)
    {
      	bands[i].high1 = sample - bands[i].f * bands[i].mid1 - bands[i].low1;
      	bands[i].mid1 += bands[i].high1 * bands[i].c;
      	bands[i].low1 += bands[i].mid1;

      	bands[i].high2 = bands[i].low1 - bands[i].f * bands[i].mid2 -
			bands[i].low2;
      	bands[i].mid2 += bands[i].high2 * bands[i].c;
      	bands[i].low2 += bands[i].mid2;
      	bands[i].y = bands[i].high2 * bands[i].att;
    }
}


int Vocoder::process_buffer(int64_t size,
	Samples **buffer,
	int64_t start_position,
	int sample_rate)
{
	need_reconfigure |= load_configuration();
	if(need_reconfigure) reconfigure();

// Process all except output channel
	int carrier_track = config.carrier_track;
	CLAMP(carrier_track, 0, PluginClient::get_total_buffers() - 1);
	int formant_track = 0;
	if(carrier_track == 0) formant_track = 1;
	CLAMP(formant_track, 0, PluginClient::get_total_buffers() - 1);


// Copy level controls to band levels
	for(int i = 0; i < current_bands; i++)
	{
		output_bands[i].level = 1.0;
	}


	for(int i = 0; i < get_total_buffers(); i++)
	{
		read_samples(buffer[i],
			i,
			get_samplerate(),
			start_position,
			size);
	}

	double *carrier_samples = buffer[carrier_track]->get_data();
	double *formant_samples = buffer[formant_track]->get_data();
	double *output = buffer[0]->get_data();
	double wetness = DB::fromdb(config.wetness);
	double level = DB::fromdb(config.level);
	for(int i = 0; i < size; i++)
	{
		do_bandpasses(carrier_bands, carrier_samples[i]);
		do_bandpasses(formant_bands, formant_samples[i]);

		output[i] *= wetness;
		double accum = 0;
		for(int j = 0; j < current_bands; j++)
		{
			output_bands[j].oldval = output_bands[j].oldval +
	    	  	(fabs (formant_bands[j].y) -
		    		output_bands[j].oldval) *
	    	 	output_bands[j].decay;
			double x = carrier_bands[j].y * output_bands[j].oldval;
			accum += x * output_bands[j].level;
		}

		accum *= level;

		output[i] += accum;
	}

	return 0;
}










void Vocoder::reset()
{
	need_reconfigure = 1;
	thread = 0;
}

void Vocoder::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			((VocoderWindow*)thread->window)->lock_window("Vocoder::update_gui");
			((VocoderWindow*)thread->window)->update_gui();
			((VocoderWindow*)thread->window)->unlock_window();
		}
	}
}



