
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

#include "bccolors.h"
#include "bchash.h"
#include "clip.h"
#include "filexml.h"
#include "keyframe.h"
#include "language.h"
#include "bccolors.h"
#include "timeavg.h"
#include "timeavgwindow.h"
#include "transportque.h"
#include "vframe.h"




#include <stdint.h>
#include <string.h>




REGISTER_PLUGIN(TimeAvgMain)





TimeAvgConfig::TimeAvgConfig()
{
	frames = 1;
	mode = TimeAvgConfig::AVERAGE;
	paranoid = 0;
	nosubtract = 0;
	threshold = 1;
	border = 2;
}

void TimeAvgConfig::copy_from(TimeAvgConfig *src)
{
	this->frames = src->frames;
	this->mode = src->mode;
	this->paranoid = src->paranoid;
	this->nosubtract = src->nosubtract;
	this->threshold = src->threshold;
	this->border = src->border;
}

int TimeAvgConfig::equivalent(TimeAvgConfig *src)
{
	return frames == src->frames &&
		mode == src->mode &&
		paranoid == src->paranoid &&
		nosubtract == src->nosubtract &&
		threshold == src->threshold &&
		border == src->border;
}













TimeAvgMain::TimeAvgMain(PluginServer *server)
 : PluginVClient(server)
{

	accumulation = 0;
	history = 0;
	history_size = 0;
	history_start = -0x7fffffff;
	history_frame = 0;
	history_valid = 0;
	prev_frame = -1;
}

TimeAvgMain::~TimeAvgMain()
{


	if(accumulation) delete [] accumulation;
	if(history)
	{
		for(int i = 0; i < config.frames; i++)
			delete history[i];
		delete [] history;
	}
	if(history_frame) delete [] history_frame;
	if(history_valid) delete [] history_valid;
}

const char* TimeAvgMain::plugin_title() { return N_("Time Average"); }
int TimeAvgMain::is_realtime() { return 1; }



NEW_WINDOW_MACRO(TimeAvgMain, TimeAvgWindow);



int TimeAvgMain::process_buffer(VFrame *frame,
		int64_t start_position,
		double frame_rate)
{
	int h = frame->get_h();
	int w = frame->get_w();
	int color_model = frame->get_color_model();

	int reset = load_configuration();

// reset buffer on the keyframes
	int64_t actual_previous_number = start_position;
	if(get_direction() == PLAY_FORWARD)
	{
		actual_previous_number--;
		if(actual_previous_number < get_source_start())
			reset = 1;
		else
		{
			KeyFrame *keyframe = get_prev_keyframe(start_position, 1);
			if(keyframe->position > 0 &&
				actual_previous_number < keyframe->position)
				reset = 1;
		}
	}
	else
	{
		actual_previous_number++;
		if(actual_previous_number >= get_source_start() + get_total_len())
			reset = 1;
		else
		{
			KeyFrame *keyframe = get_next_keyframe(start_position, 1);
			if(keyframe->position > 0 &&
				actual_previous_number >= keyframe->position)
				reset = 1;
		}
	}

// Allocate accumulation
	if(!accumulation || reset)
	{
		if(!accumulation) accumulation = new unsigned char[w *
			h *
			BC_CModels::components(color_model) *
			MAX(sizeof(float), sizeof(int))];
		reset_accum(w, h, color_model);
	}

	if(!config.nosubtract &&
		(config.mode == TimeAvgConfig::AVERAGE ||
		config.mode == TimeAvgConfig::ACCUMULATE ||
		config.mode == TimeAvgConfig::GREATER ||
		config.mode == TimeAvgConfig::LESS))
	{
// Reallocate history
		if(history)
		{
			if(config.frames != history_size)
			{
				VFrame **history2;
				int64_t *history_frame2;
				int *history_valid2;
				history2 = new VFrame*[config.frames];
				history_frame2 = new int64_t[config.frames];
				history_valid2 = new int[config.frames];

// Copy existing frames over
				int i, j;
				for(i = 0, j = 0; i < config.frames && j < history_size; i++, j++)
				{
					history2[i] = history[j];
					history_frame2[i] = history_frame[i];
					history_valid2[i] = history_valid[i];
				}

// Delete extra previous frames and subtract from accumulation
				for( ; j < history_size; j++)
				{
					subtract_accum(history[j]);
					delete history[j];
				}
				delete [] history;
				delete [] history_frame;
				delete [] history_valid;


// Create new frames
				for( ; i < config.frames; i++)
				{
					history2[i] = new VFrame(w, h, color_model, 0);
					history_frame2[i] = -0x7fffffff;
					history_valid2[i] = 0;
				}

				history = history2;
				history_frame = history_frame2;
				history_valid = history_valid2;

				history_size = config.frames;
			}
		}
		else
// Allocate history
		{
			history = new VFrame*[config.frames];
			for(int i = 0; i < config.frames; i++)
				history[i] = new VFrame(w, h, color_model, 0);
			history_size = config.frames;
			history_frame = new int64_t[config.frames];
			bzero(history_frame, sizeof(int64_t) * config.frames);
			history_valid = new int[config.frames];
			bzero(history_valid, sizeof(int) * config.frames);
		}

//printf("TimeAvgMain::process_buffer %d\n", __LINE__);




// Create new history frames based on current frame
		int64_t *new_history_frames = new int64_t[history_size];
		for(int i = 0; i < history_size; i++)
		{
			new_history_frames[history_size - i - 1] = start_position - i;
		}

// Subtract old history frames from accumulation buffer
// which are not in the new vector
		int no_change = 1;
		for(int i = 0; i < history_size; i++)
		{
// Old frame is valid
			if(history_valid[i])
			{
				int got_it = 0;
				for(int j = 0; j < history_size; j++)
				{
// Old frame is equal to a new frame
					if(history_frame[i] == new_history_frames[j])
					{
						got_it = 1;
						break;
					}
				}

// Didn't find old frame in new frames
				if(!got_it)
				{
					if(config.mode == TimeAvgConfig::AVERAGE ||
						config.mode == TimeAvgConfig::ACCUMULATE)
					{
						subtract_accum(history[i]);
					}

					history_valid[i] = 0;
					no_change = 0;
				}
			}
		}
// If all frames are still valid, assume tweek occurred upstream and reload.
		if(config.paranoid && no_change)
		{
			for(int i = 0; i < history_size; i++)
			{
				history_valid[i] = 0;
			}

			if(config.mode == TimeAvgConfig::AVERAGE ||
				config.mode == TimeAvgConfig::ACCUMULATE)
			{
				reset_accum(w, h, color_model);
			}
		}

// Add new history frames which are not in the old vector
		for(int i = 0; i < history_size; i++)
		{
// Find new frame in old vector
			int got_it = 0;
			for(int j = 0; j < history_size; j++)
			{
				if(history_valid[j] && history_frame[j] == new_history_frames[i])
				{
					got_it = 1;
					break;
				}
			}

// Didn't find new frame in old vector
			if(!got_it)
			{
// Get first unused entry
				for(int j = 0; j < history_size; j++)
				{
					if(!history_valid[j])
					{
// Load new frame into it
						history_frame[j] = new_history_frames[i];
						history_valid[j] = 1;
						read_frame(history[j],
							0,
							history_frame[j],
							frame_rate,
							0);
						if(config.mode == TimeAvgConfig::AVERAGE ||
							config.mode == TimeAvgConfig::ACCUMULATE)
						{
							add_accum(history[j]);
						}
						break;
					}
				}
			}
		}
		delete [] new_history_frames;
	}
	else
// No history subtraction
	{
		if(history)
		{
			for(int i = 0; i < config.frames; i++)
				delete history[i];
			delete [] history;
			history = 0;
		}

		if(history_frame) delete [] history_frame;
		if(history_valid) delete [] history_valid;
		history_frame = 0;
		history_valid = 0;
		history_size = 0;

// Clamp prev_frame to history size
		prev_frame = MAX(start_position - config.frames + 1, prev_frame);

// Force reload if not repositioned or just started
		if( (config.paranoid && prev_frame == start_position) ||
			prev_frame < 0)
		{
//printf("TimeAvgMain::process_buffer %d\n", __LINE__);
			prev_frame = start_position - config.frames + 1;
			prev_frame = MAX(0, prev_frame);
			reset_accum(w, h, color_model);
		}

// printf("TimeAvgMain::process_buffer %d prev_frame=%jd start_position=%jd\n",
//   __LINE__, prev_frame, start_position);
		for(int64_t i = prev_frame; i <= start_position; i++)
		{
			read_frame(frame,
				0,
				i,
				frame_rate,
				0);
			add_accum(frame);
printf("TimeAvgMain::process_buffer %d prev_frame=%jd start_position=%jd i=%jd\n",
  __LINE__, prev_frame, start_position, i);
		}

// If we don't add 1, it rereads the frame again
		prev_frame = start_position + 1;
	}







// Transfer accumulation to output with division if average is desired.
	transfer_accum(frame);

//printf("TimeAvgMain::process_buffer %d\n", __LINE__);


	return 0;
}










// Reset accumulation
#define SET_ACCUM(type, components, luma, chroma) \
{ \
	type *row = (type*)accumulation; \
	if(chroma) \
	{ \
		for(int i = 0; i < w * h; i++) \
		{ \
			*row++ = luma; \
			*row++ = chroma; \
			*row++ = chroma; \
			if(components == 4) *row++ = luma; \
		} \
	} \
	else \
	{ \
		bzero(row, w * h * sizeof(type) * components); \
	} \
}


void TimeAvgMain::reset_accum(int w, int h, int color_model)
{
	if(config.mode == TimeAvgConfig::LESS)
	{
		switch(color_model)
		{
			case BC_RGB888:
				SET_ACCUM(int, 3, 0xff, 0xff)
				break;
			case BC_RGB_FLOAT:
				SET_ACCUM(float, 3, 1.0, 1.0)
				break;
			case BC_RGBA8888:
				SET_ACCUM(int, 4, 0xff, 0xff)
				break;
			case BC_RGBA_FLOAT:
				SET_ACCUM(float, 4, 1.0, 1.0)
				break;
			case BC_YUV888:
				SET_ACCUM(int, 3, 0xff, 0x80)
				break;
			case BC_YUVA8888:
				SET_ACCUM(int, 4, 0xff, 0x80)
				break;
			case BC_YUV161616:
				SET_ACCUM(int, 3, 0xffff, 0x8000)
				break;
			case BC_YUVA16161616:
				SET_ACCUM(int, 4, 0xffff, 0x8000)
				break;
		}
	}
	else
	{
		switch(color_model)
		{
			case BC_RGB888:
				SET_ACCUM(int, 3, 0x0, 0x0)
				break;
			case BC_RGB_FLOAT:
				SET_ACCUM(float, 3, 0x0, 0x0)
				break;
			case BC_RGBA8888:
				SET_ACCUM(int, 4, 0x0, 0x0)
				break;
			case BC_RGBA_FLOAT:
				SET_ACCUM(float, 4, 0x0, 0x0)
				break;
			case BC_YUV888:
				SET_ACCUM(int, 3, 0x0, 0x80)
				break;
			case BC_YUVA8888:
				SET_ACCUM(int, 4, 0x0, 0x80)
				break;
			case BC_YUV161616:
				SET_ACCUM(int, 3, 0x0, 0x8000)
				break;
			case BC_YUVA16161616:
				SET_ACCUM(int, 4, 0x0, 0x8000)
				break;
		}
	}
}

#define RGB_TO_VALUE(r, g, b) YUV::yuv.rgb_to_y_f((r),(g),(b))

// Only AVERAGE and ACCUMULATE use this
#define SUBTRACT_ACCUM(type, \
	accum_type, \
	components, \
	chroma) \
{ \
	for(int i = 0; i < h; i++) \
	{ \
		accum_type *accum_row = (accum_type*)accumulation + \
			i * w * components; \
		type *frame_row = (type*)frame->get_rows()[i]; \
		for(int j = 0; j < w; j++) \
		{ \
			*accum_row++ -= *frame_row++; \
			*accum_row++ -= (accum_type)*frame_row++ - chroma; \
			*accum_row++ -= (accum_type)*frame_row++ - chroma; \
			if(components == 4) *accum_row++ -= *frame_row++; \
		} \
	} \
}


void TimeAvgMain::subtract_accum(VFrame *frame)
{
// Just accumulate
	if(config.nosubtract) return;
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			SUBTRACT_ACCUM(unsigned char, int, 3, 0x0)
			break;
		case BC_RGB_FLOAT:
			SUBTRACT_ACCUM(float, float, 3, 0x0)
			break;
		case BC_RGBA8888:
			SUBTRACT_ACCUM(unsigned char, int, 4, 0x0)
			break;
		case BC_RGBA_FLOAT:
			SUBTRACT_ACCUM(float, float, 4, 0x0)
			break;
		case BC_YUV888:
			SUBTRACT_ACCUM(unsigned char, int, 3, 0x80)
			break;
		case BC_YUVA8888:
			SUBTRACT_ACCUM(unsigned char, int, 4, 0x80)
			break;
		case BC_YUV161616:
			SUBTRACT_ACCUM(uint16_t, int, 3, 0x8000)
			break;
		case BC_YUVA16161616:
			SUBTRACT_ACCUM(uint16_t, int, 4, 0x8000)
			break;
	}
}


// The behavior has to be very specific to the color model because we rely on
// the value of full black to determine what pixel to show.
#define ADD_ACCUM(type, accum_type, components, chroma, max) \
{ \
	if(config.mode == TimeAvgConfig::REPLACE) \
	{ \
		type threshold = config.threshold; \
		if(sizeof(type) == 4) \
			threshold /= 256; \
/* Compare all pixels if border */ \
		if(config.border > 0) \
		{ \
			int border = config.border; \
			int h_border = h - border - 1; \
			int w_border = w - border - 1; \
			int kernel_size = (border * 2 + 1) * (border * 2 + 1); \
			for(int i = border; i < h_border; i++) \
			{ \
				for(int j = border; j < w_border; j++) \
				{ \
					int copy_it = 0; \
					for(int k = -border; k <= border; k++) \
					{ \
						type *frame_row = (type*)frame->get_rows()[i + k]; \
						for(int l = -border; l <= border; l++) \
						{ \
							type *frame_pixel = frame_row + (j + l) * components; \
/* Compare alpha if 4 channel */ \
							if(components == 4) \
							{ \
								if(frame_pixel[3] > threshold) \
									copy_it++; \
							} \
							else \
							if(sizeof(type) == 4) \
							{ \
/* Compare luma if 3 channel */ \
								if(RGB_TO_VALUE(frame_pixel[0], frame_pixel[1], frame_pixel[2]) >= \
									threshold) \
								{ \
									copy_it++; \
								} \
							} \
							else \
							if(chroma) \
							{ \
								if(frame_pixel[0] >= threshold) \
								{ \
									copy_it++; \
								} \
							} \
							else \
							if(RGB_TO_VALUE(frame_pixel[0], frame_pixel[1], frame_pixel[2]) >= threshold) \
							{ \
								copy_it++; \
							} \
						} \
					} \
 \
					if(copy_it == kernel_size) \
					{ \
						accum_type *accum_row = (accum_type*)accumulation + \
							i * w * components + j * components; \
						type *frame_row = (type*)frame->get_rows()[i] + j * components; \
						*accum_row++ = *frame_row++; \
						*accum_row++ = *frame_row++; \
						*accum_row++ = *frame_row++; \
						if(components == 4) *accum_row++ = *frame_row++; \
					} \
 \
				} \
			} \
		} \
		else \
/* Compare only relevant pixel if no border */ \
		{ \
			for(int i = 0; i < h; i++) \
			{ \
				accum_type *accum_row = (accum_type*)accumulation + \
					i * w * components; \
				type *frame_row = (type*)frame->get_rows()[i]; \
				for(int j = 0; j < w; j++) \
				{ \
					int copy_it = 0; \
/* Compare alpha if 4 channel */ \
					if(components == 4) \
					{ \
						if(frame_row[3] > threshold) \
							copy_it = 1; \
					} \
					else \
					if(sizeof(type) == 4) \
					{ \
/* Compare luma if 3 channel */ \
						if(RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2]) >= \
							threshold) \
						{ \
							copy_it = 1; \
						} \
					} \
					else \
					if(chroma) \
					{ \
						if(frame_row[0] >= threshold) \
						{ \
							copy_it = 1; \
						} \
					} \
					else \
					if(RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2]) >= threshold) \
					{ \
						copy_it = 1; \
					} \
 \
					if(copy_it) \
					{ \
						*accum_row++ = *frame_row++; \
						*accum_row++ = *frame_row++; \
						*accum_row++ = *frame_row++; \
						if(components == 4) *accum_row++ = *frame_row++; \
					} \
					else \
					{ \
						frame_row += components; \
						accum_row += components; \
					} \
				} \
			} \
		} \
	} \
	else \
	if(config.mode == TimeAvgConfig::GREATER) \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				int copy_it = 0; \
/* Compare alpha if 4 channel */ \
				if(components == 4) \
				{ \
					if(frame_row[3] > accum_row[3]) copy_it = 1; \
				} \
				else \
				if(chroma) \
				{ \
/* Compare YUV luma if 3 channel */ \
					if(frame_row[0] > accum_row[0]) copy_it = 1; \
				} \
				else \
				{ \
/* Compare RGB luma if 3 channel */ \
					if(RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2]) > \
						RGB_TO_VALUE(accum_row[0], accum_row[1], accum_row[2])) \
						copy_it = 1; \
				} \
 \
 				if(copy_it) \
				{ \
					*accum_row++ = *frame_row++; \
					*accum_row++ = *frame_row++; \
					*accum_row++ = *frame_row++; \
					if(components == 4) *accum_row++ = *frame_row++; \
				} \
				else \
				{ \
					accum_row += components; \
					frame_row += components; \
				} \
			} \
		} \
	} \
	else \
	if(config.mode == TimeAvgConfig::LESS) \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				int copy_it = 0; \
/* Compare alpha if 4 channel */ \
				if(components == 4) \
				{ \
					if(frame_row[3] < accum_row[3]) copy_it = 1; \
				} \
				else \
				if(chroma) \
				{ \
/* Compare YUV luma if 3 channel */ \
					if(frame_row[0] < accum_row[0]) copy_it = 1; \
				} \
				else \
				{ \
/* Compare RGB luma if 3 channel */ \
					if(RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2]) < \
						RGB_TO_VALUE(accum_row[0], accum_row[1], accum_row[2])) \
						copy_it = 1; \
				} \
 \
 				if(copy_it) \
				{ \
					*accum_row++ = *frame_row++; \
					*accum_row++ = *frame_row++; \
					*accum_row++ = *frame_row++; \
					if(components == 4) *accum_row++ = *frame_row++; \
				} \
				else \
				{ \
					accum_row += components; \
					frame_row += components; \
				} \
			} \
		} \
	} \
	else \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				*accum_row++ += *frame_row++; \
				*accum_row++ += (accum_type)*frame_row++ - chroma; \
				*accum_row++ += (accum_type)*frame_row++ - chroma; \
				if(components == 4) *accum_row++ += *frame_row++; \
			} \
		} \
	} \
}


void TimeAvgMain::add_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			ADD_ACCUM(unsigned char, int, 3, 0x0, 0xff)
			break;
		case BC_RGB_FLOAT:
			ADD_ACCUM(float, float, 3, 0x0, 1.0)
			break;
		case BC_RGBA8888:
			ADD_ACCUM(unsigned char, int, 4, 0x0, 0xff)
			break;
		case BC_RGBA_FLOAT:
			ADD_ACCUM(float, float, 4, 0x0, 1.0)
			break;
		case BC_YUV888:
			ADD_ACCUM(unsigned char, int, 3, 0x80, 0xff)
			break;
		case BC_YUVA8888:
			ADD_ACCUM(unsigned char, int, 4, 0x80, 0xff)
			break;
		case BC_YUV161616:
			ADD_ACCUM(uint16_t, int, 3, 0x8000, 0xffff)
			break;
		case BC_YUVA16161616:
			ADD_ACCUM(uint16_t, int, 4, 0x8000, 0xffff)
			break;
	}
}

#define TRANSFER_ACCUM(type, accum_type, components, chroma, max) \
{ \
	if(config.mode == TimeAvgConfig::AVERAGE) \
	{ \
		accum_type denominator = config.frames; \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				*frame_row++ = *accum_row++ / denominator; \
				*frame_row++ = (*accum_row++ - chroma) / denominator + chroma; \
				*frame_row++ = (*accum_row++ - chroma) / denominator + chroma; \
				if(components == 4) *frame_row++ = *accum_row++ / denominator; \
			} \
		} \
	} \
	else \
/* Rescan history every time for these modes */ \
	if(!config.nosubtract && config.mode == TimeAvgConfig::GREATER) \
	{ \
		frame->copy_from(history[0]); \
		for(int k = 1; k < config.frames; k++) \
		{ \
			VFrame *history_frame = history[k]; \
 \
			for(int i = 0; i < h; i++) \
			{ \
				type *history_row = (type*)history_frame->get_rows()[i]; \
				type *frame_row = (type*)frame->get_rows()[i]; \
 \
				for(int j = 0; j < w; j++) \
				{ \
					int copy_it = 0; \
/* Compare alpha if 4 channel */ \
					if(components == 4) \
					{ \
						if(history_row[3] > frame_row[3]) copy_it = 1; \
					} \
					else \
					if(chroma) \
					{ \
/* Compare YUV luma if 3 channel */ \
						if(history_row[0] > frame_row[0]) copy_it = 1; \
					} \
					else \
					{ \
/* Compare RGB luma if 3 channel */ \
						if(RGB_TO_VALUE(history_row[0], history_row[1], history_row[2]) > \
							RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2])) \
							copy_it = 1; \
					} \
 \
 					if(copy_it) \
					{ \
						*frame_row++ = *history_row++; \
						*frame_row++ = *history_row++; \
						*frame_row++ = *history_row++; \
						if(components == 4) *frame_row++ = *history_row++; \
					} \
					else \
					{ \
						frame_row += components; \
						history_row += components; \
					} \
				} \
			} \
		} \
	} \
	else \
	if(!config.nosubtract && config.mode == TimeAvgConfig::LESS) \
	{ \
		frame->copy_from(history[0]); \
		for(int k = 1; k < config.frames; k++) \
		{ \
			VFrame *history_frame = history[k]; \
 \
			for(int i = 0; i < h; i++) \
			{ \
				type *history_row = (type*)history_frame->get_rows()[i]; \
				type *frame_row = (type*)frame->get_rows()[i]; \
 \
				for(int j = 0; j < w; j++) \
				{ \
					int copy_it = 0; \
/* Compare alpha if 4 channel */ \
					if(components == 4) \
					{ \
						if(history_row[3] < frame_row[3]) copy_it = 1; \
					} \
					else \
					if(chroma) \
					{ \
/* Compare YUV luma if 3 channel */ \
						if(history_row[0] < frame_row[0]) copy_it = 1; \
					} \
					else \
					{ \
/* Compare RGB luma if 3 channel */ \
						if(RGB_TO_VALUE(history_row[0], history_row[1], history_row[2]) < \
							RGB_TO_VALUE(frame_row[0], frame_row[1], frame_row[2])) \
							copy_it = 1; \
					} \
 \
 					if(copy_it) \
					{ \
						*frame_row++ = *history_row++; \
						*frame_row++ = *history_row++; \
						*frame_row++ = *history_row++; \
						if(components == 4) *frame_row++ = *history_row++; \
					} \
					else \
					{ \
						frame_row += components; \
						history_row += components; \
					} \
				} \
			} \
		} \
	} \
	else \
	{ \
		for(int i = 0; i < h; i++) \
		{ \
			accum_type *accum_row = (accum_type*)accumulation + \
				i * w * components; \
			type *frame_row = (type*)frame->get_rows()[i]; \
			for(int j = 0; j < w; j++) \
			{ \
				*frame_row++ = *accum_row++; \
				*frame_row++ = *accum_row++; \
				*frame_row++ = *accum_row++; \
				if(components == 4) *frame_row++ = *accum_row++; \
			} \
		} \
	} \
}


void TimeAvgMain::transfer_accum(VFrame *frame)
{
	int w = frame->get_w();
	int h = frame->get_h();

	switch(frame->get_color_model())
	{
		case BC_RGB888:
			TRANSFER_ACCUM(unsigned char, int, 3, 0x0, 0xff)
			break;
		case BC_RGB_FLOAT:
			TRANSFER_ACCUM(float, float, 3, 0x0, 1)
			break;
		case BC_RGBA8888:
			TRANSFER_ACCUM(unsigned char, int, 4, 0x0, 0xff)
			break;
		case BC_RGBA_FLOAT:
			TRANSFER_ACCUM(float, float, 4, 0x0, 1)
			break;
		case BC_YUV888:
			TRANSFER_ACCUM(unsigned char, int, 3, 0x80, 0xff)
			break;
		case BC_YUVA8888:
			TRANSFER_ACCUM(unsigned char, int, 4, 0x80, 0xff)
			break;
		case BC_YUV161616:
			TRANSFER_ACCUM(uint16_t, int, 3, 0x8000, 0xffff)
			break;
		case BC_YUVA16161616:
			TRANSFER_ACCUM(uint16_t, int, 4, 0x8000, 0xffff)
			break;
	}
}



int TimeAvgMain::load_configuration()
{
	KeyFrame *prev_keyframe;
	TimeAvgConfig old_config;
	old_config.copy_from(&config);

	prev_keyframe = get_prev_keyframe(get_source_position());
	read_data(prev_keyframe);
	return !old_config.equivalent(&config);
}

void TimeAvgMain::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title("TIME_AVERAGE");
	output.tag.set_property("FRAMES", config.frames);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("PARANOID", config.paranoid);
	output.tag.set_property("NOSUBTRACT", config.nosubtract);
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("BORDER", config.border);
	output.append_tag();
	output.tag.set_title("/TIME_AVERAGE");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void TimeAvgMain::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	while(!input.read_tag())
	{
		if(input.tag.title_is("TIME_AVERAGE"))
		{
			config.frames = input.tag.get_property("FRAMES", config.frames);
			config.mode = input.tag.get_property("MODE", config.mode);
			config.paranoid = input.tag.get_property("PARANOID", config.paranoid);
			config.nosubtract = input.tag.get_property("NOSUBTRACT", config.nosubtract);
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.border = input.tag.get_property("BORDER", config.border);
		}
	}
}


void TimeAvgMain::update_gui()
{
	if(thread)
	{
		if(load_configuration())
		{
			thread->window->lock_window("TimeAvgMain::update_gui");
			((TimeAvgWindow*)thread->window)->total_frames->update(config.frames);
			((TimeAvgWindow*)thread->window)->threshold->update(config.threshold);
			((TimeAvgWindow*)thread->window)->update_toggles();
			((TimeAvgWindow*)thread->window)->paranoid->update(config.paranoid);
			((TimeAvgWindow*)thread->window)->no_subtract->update(config.nosubtract);
			((TimeAvgWindow*)thread->window)->threshold->update(config.threshold);
			((TimeAvgWindow*)thread->window)->border->update(config.border);
			thread->window->unlock_window();
		}
	}
}










