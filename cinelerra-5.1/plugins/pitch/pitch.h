
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

#ifndef PITCH_H
#define PITCH_H



#include "bchash.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"


class PitchEffect;
class PitchWindow;

class PitchScale : public BC_FPot
{
public:
	PitchScale(PitchEffect *plugin, int x, int y);
	int handle_event();
	PitchEffect *plugin;
};


class PitchSize : public BC_PopupMenu
{
public:
	PitchSize(PitchWindow *window, PitchEffect *plugin, int x, int y);

	int handle_event();
	void create_objects();         // add initial items
	void update(int size);

	PitchEffect *plugin;
};

class PitchWindow : public PluginClientWindow
{
public:
	PitchWindow(PitchEffect *plugin);
	void create_objects();
	void update();

	PitchScale *scale;
	PitchSize *size;
	PitchEffect *plugin;
};




class PitchConfig
{
public:
	PitchConfig();


	int equivalent(PitchConfig &that);
	void copy_from(PitchConfig &that);
	void interpolate(PitchConfig &prev,
		PitchConfig &next,
		int64_t prev_frame,
		int64_t next_frame,
		int64_t current_frame);


	double scale;
	int size;
};

class PitchFFT : public CrossfadeFFT
{
public:
	PitchFFT(PitchEffect *plugin);
	~PitchFFT();
	int signal_process();
	int read_samples(int64_t output_sample,
		int samples,
		Samples *buffer);
	PitchEffect *plugin;
	double *last_phase;
	double *new_freq;
	double *new_magn;
	double *sum_phase;
	double *anal_freq;
	double *anal_magn;
};

class PitchEffect : public PluginAClient
{
public:
	PitchEffect(PluginServer *server);
	~PitchEffect();

	PLUGIN_CLASS_MEMBERS(PitchConfig);

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_buffer(int64_t size,
		Samples *buffer,
		int64_t start_position,
		int sample_rate);




	void reset();
	void update_gui();


	PitchFFT *fft;
};


#endif
