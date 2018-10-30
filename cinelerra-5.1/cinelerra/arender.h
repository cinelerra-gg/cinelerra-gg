
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

#ifndef ARENDER_H
#define ARENDER_H

#include "atrack.inc"
#include "commonrender.h"
#include "maxchannels.h"
#include "meterhistory.h"
#include "samples.inc"

class ARender : public CommonRender
{
public:
	ARender(RenderEngine *renderengine);
	~ARender();

	void arm_command();
	void allocate_buffers(int samples);
// Used by realtime rendering
	void init_output_buffers();
	void interrupt_playback();
	VirtualConsole* new_vconsole_object();
	int get_total_tracks();
	Module* new_module(Track *track);

// set up and start thread
	int arm_playback(int64_t current_position,
				int64_t input_length,
				int64_t module_render_fragment,
				int64_t playback_buffer,
				int64_t output_length);
	int wait_for_startup();

// process a buffer
// renders into buffer_out argument when no audio device
// handles playback autos
	int process_buffer(Samples **buffer_out,
		int64_t input_len,
		int64_t input_position);
// renders to a device when there's a device
	int process_buffer(int64_t input_len, int64_t input_position);

	void run();

	void send_last_buffer();
	int stop_audio(int wait);

// Calculate number of samples in each meter fragment and how many
// meter fragments to buffer.
	int init_meters();
	int calculate_history_size();
	int total_peaks;
	MeterHistory *meter_history;
// samples to use for one meter update.  Must be multiple of fragment_len
	int64_t meter_render_fragment;

	int64_t tounits(double position, int round);
	double fromunits(int64_t position);

// pointers to output buffers for VirtualConsole
	Samples *audio_out[MAXCHANNELS];
// allocated memory for output
	Samples *buffer[MAXCHANNELS];
// allocated buffer sizes for nested EDL rendering
	int buffer_allocated[MAXCHANNELS];
// Make VirtualAConsole block before the first buffer until video is ready
	int first_buffer;
};



#endif
