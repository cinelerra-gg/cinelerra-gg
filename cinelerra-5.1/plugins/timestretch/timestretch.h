
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#ifndef TIMESTRETCH_H
#define TIMESTRETCH_H

#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mainprogress.inc"
#include "pluginaclient.h"
#include "resample.inc"
#include "timestretchengine.inc"
#include "vframe.inc"




class TimeStretch;
class TimeStretchWindow;




class TimeStretchFraction : public BC_TextBox
{
public:
	TimeStretchFraction(TimeStretch *plugin, int x, int y);
	int handle_event();
	TimeStretch *plugin;
};


class TimeStretchFreq : public BC_Radial
{
public:
	TimeStretchFreq(TimeStretch *plugin, TimeStretchWindow *gui, int x, int y);
	int handle_event();
	TimeStretch *plugin;
	TimeStretchWindow *gui;
};

class TimeStretchTime : public BC_Radial
{
public:
	TimeStretchTime(TimeStretch *plugin, TimeStretchWindow *gui, int x, int y);
	int handle_event();
	TimeStretch *plugin;
	TimeStretchWindow *gui;
};


class TimeStretchWindow : public BC_Window
{
public:
	TimeStretchWindow(TimeStretch *plugin, int x, int y);
	~TimeStretchWindow();

	void create_objects();

	TimeStretch *plugin;
	TimeStretchFreq *freq;
	TimeStretchTime *time;
};


class PitchEngine : public CrossfadeFFT
{
public:
	PitchEngine(TimeStretch *plugin);
	~PitchEngine();


	int read_samples(int64_t output_sample,
		int samples,
		Samples *buffer);
	int signal_process();

	TimeStretch *plugin;
	double *temp;
	double *input_buffer;
	int input_size;
	int input_allocated;
	int64_t current_position;
};

class TimeStretchResample : public Resample
{
public:
	TimeStretchResample(TimeStretch *plugin);

	int read_samples(Samples *buffer, int64_t start, int64_t len);

	TimeStretch *plugin;
};



class TimeStretch : public PluginAClient
{
public:
	TimeStretch(PluginServer *server);
	~TimeStretch();


	const char* plugin_title();
	int get_parameters();
	int start_loop();
	int process_loop(Samples *buffer, int64_t &write_length);
	int stop_loop();

	int load_defaults();
	int save_defaults();




	PitchEngine *pitch;
	TimeStretchResample *resample;
	double *temp;
	int temp_allocated;
	Samples *input;
	int input_allocated;

	int use_fft;
	TimeStretchEngine *stretch;

	MainProgressBar *progress;
	double scale;
	int64_t scaled_size;
	int64_t current_position;
	int64_t total_written;
	int64_t current_written;
	int64_t total_read;
};


#endif
