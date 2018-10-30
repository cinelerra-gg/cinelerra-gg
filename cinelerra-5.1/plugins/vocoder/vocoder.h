
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

#ifndef VOCODER_H
#define VOCODER_H


#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "fourier.h"
#include "pluginaclient.h"
#include "vframe.inc"



#define MAX_BANDS 32

class Vocoder;
class VocoderWindow;






class VocoderConfig
{
public:
	VocoderConfig();

	int equivalent(VocoderConfig &that);
	void copy_from(VocoderConfig &that);
	void interpolate(VocoderConfig &prev,
		VocoderConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);

	int carrier_track;
	int bands;
	double level;
	double wetness;
};






class VocoderWetness : public BC_FPot
{
public:
	VocoderWetness(Vocoder *plugin, int x, int y);
	int handle_event();
	Vocoder *plugin;
};




class VocoderLevel : public BC_FPot
{
public:
	VocoderLevel(Vocoder *plugin, int x, int y);
	int handle_event();
	Vocoder *plugin;
};




class VocoderCarrier : public BC_TumbleTextBox
{
public:
	VocoderCarrier(Vocoder *plugin,
		VocoderWindow *window,
		int x,
		int y);
	int handle_event();
	Vocoder *plugin;
};


class VocoderBands : public BC_TumbleTextBox
{
public:
	VocoderBands(Vocoder *plugin,
		VocoderWindow *window,
		int x,
		int y);
	int handle_event();
	Vocoder *plugin;
};





class VocoderWindow : public PluginClientWindow
{
public:
	VocoderWindow(Vocoder *plugin);
	~VocoderWindow();

	void create_objects();
	void update_gui();

	VocoderCarrier  *output;
	VocoderBands *bands;
	VocoderWetness *wetness;
	VocoderLevel *level;
	Vocoder *plugin;
};

class VocoderBand
{
public:
	VocoderBand();
	void reset();
	void copy_from(VocoderBand *src);

	double c, f, att;

	double freq;
	double low1, low2;
	double mid1, mid2;
	double high1, high2;
	double y;
};

class VocoderOut
{
public:
	VocoderOut();
	void reset();

	double decay;
	double oldval;
	double level;		/* 0.0 - 1.0 level of this output band */
};

class Vocoder : public PluginAClient
{
public:
	Vocoder(PluginServer *server);
	~Vocoder();

	int is_realtime();
	int is_multichannel();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate);

	void reset();
	void reconfigure();
	void update_gui();

	void do_bandpasses(VocoderBand *bands, double sample);
	VocoderBand formant_bands[MAX_BANDS];
	VocoderBand carrier_bands[MAX_BANDS];
	VocoderOut output_bands[MAX_BANDS];
	int current_bands;

	int need_reconfigure;
	PLUGIN_CLASS_MEMBERS2(VocoderConfig)

};



#endif
