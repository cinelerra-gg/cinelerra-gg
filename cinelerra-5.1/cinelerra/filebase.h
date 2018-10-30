
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

#ifndef FILEBASE_H
#define FILEBASE_H

#include "asset.inc"
#include "assets.inc"
#include "bccmodels.h"
#include "edit.inc"
#include "edl.inc"
#include "ffmpeg.inc"
#include "guicast.h"
#include "file.inc"
#include "filelist.inc"
#include "indexfile.inc"
#include "mainprogress.inc"
#include "overlayframe.inc"
#include "strategies.inc"
#include "vframe.inc"

#include <sys/types.h>

// Number of samples saved before the current read position
#define HISTORY_MAX 0x10000

// inherited by every file interpreter
class FileBase
{
public:
	FileBase(Asset *asset, File *file);
	virtual ~FileBase();


	friend class File;
	friend class FFMPEG;
	friend class FileList;
	friend class FrameWriter;

	void get_mode(char *mode, int rd, int wr);
	void reset_parameters();

	virtual int get_index(IndexFile *index_file, MainProgressBar *progress_bar) { return -1; }
	virtual int check_header() { return 0; }  // Test file to see if it is of this type.
	virtual int reset_parameters_derived() { return 0; }
	virtual int read_header() { return 1; }   // WAV files for getting header
	virtual int get_best_colormodel(int driver, int vstream);

	virtual int open_file(int rd, int wr) { return 1; }
	virtual int close_file();
	virtual int close_file_derived() { return 0; }
	int set_dither();
	virtual int seek_end() { return 0; }
	virtual int seek_start() { return 0; }
	virtual int64_t get_video_position() { return 0; }
	virtual int64_t get_audio_position() { return 0; }
	virtual int set_video_position(int64_t x) { return 0; }
	virtual int set_audio_position(int64_t x) { return 0; }
	virtual int set_subtitle(int value) { return -1; }
	virtual int select_video_stream(Asset *asset, int vstream) { return -1; }
	virtual int select_audio_stream(Asset *asset, int astream) { return -1; }
	virtual int set_program(int no) { return -1; }
	virtual int get_cell_time(int no, double &time) { time = -1.; return 1; }
	virtual int get_system_time(int64_t &tm) { tm = -1; return 1; }
	virtual int get_audio_for_video(int vstream, int astream, int64_t &channel_mask) {
		 channel_mask = 0;  return -1;
	}
	virtual int get_video_pid(int track) { return -1; }
	virtual int get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title=0) { return -1; }

	virtual int get_thumbnail(int stream, int64_t &position,
		unsigned char *&thumbnail, int &ww, int &hh) { return -1; }
	virtual int set_skimming(int track, int skim, skim_fn fn, void *vp) { return -1; }
	virtual int skim_video(int track, void *vp, skim_fn fn) { return -1; }

// Subclass should call this to add the base class allocation.
// Only used in read mode.
	virtual int64_t base_memory_usage();

	virtual int write_samples(double **buffer,
		int64_t len) { return 0; }
	virtual int write_frames(VFrame ***frames, int len) { return 0; }
	virtual int read_compressed_frame(VFrame *buffer) { return 0; }
	virtual int write_compressed_frame(VFrame *buffers) { return 0; }
	virtual int64_t compressed_frame_size() { return 0; }
// Doubles are used to allow resampling
	virtual int read_samples(double *buffer, int64_t len) { return 0; }
	virtual int read_frame(VFrame *frame) { return 1; }
// get dvb record stream file descriptor
	virtual int record_fd() { return -1; }

	virtual int prefer_samples_float() { return 0; }
	virtual int read_samples_float(float *buffer, int64_t len) { return 0; }

// Return either the argument or another colormodel which read_frame should
// use.
	virtual int colormodel_supported(int colormodel) { return BC_RGB888; }
// This file can copy compressed frames directly from the asset
	virtual int can_copy_from(Asset *asset, int64_t position) { return 0; }
	virtual int can_scale_input() { return 0; }
	virtual int get_render_strategy(ArrayList<int>* render_strategies) { return VRENDER_VPIXEL; }

// Manages an audio history buffer
	void update_pcm_history(int64_t len);
// Returns history_start + history_size
	int64_t get_history_sample();
// contiguous float
	void append_history(float **new_data, int len);
// Interleaved short
	void append_history(short *new_data, int len);
	void read_history(double *dst,
		int64_t start_sample,
		int channel,
		int64_t len);
	void allocate_history(int len);

// For static functions to access it
	Asset *asset;
	int rd, wr;

protected:
// Return 1 if the render_strategy is present on the list.
	static int search_render_strategies(ArrayList<int>* render_strategies, int render_strategy);

// convert samples into file format
	int64_t samples_to_raw(char *out_buffer,
							float **in_buffer, // was **buffer
							int64_t input_len,
							int bits,
							int channels,
							int byte_order,
							int signed_);

// overwrites the buffer from PCM data depending on feather.
	int raw_to_samples(float *out_buffer, char *in_buffer,
		int64_t samples, int bits, int channels, int channel, int feather,
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// Overwrite the buffer from float data using feather.
	int overlay_float_buffer(float *out_buffer, float *in_buffer,
		int64_t samples,
		float lfeather_len, float lfeather_gain, float lfeather_slope);

// convert a frame to and from file format

	int64_t frame_to_raw(unsigned char *out_buffer,
					VFrame *in_frame,
					int w,
					int h,
					int use_alpha,
					int use_float,
					int color_model);

// allocate a buffer for translating int to float
	int get_audio_buffer(char **buffer, int64_t len, int64_t bits, int64_t channels); // audio

// Allocate a buffer for feathering floats
	int get_float_buffer(float **buffer, int64_t len);

// allocate a buffer for translating video to VFrame
	int get_video_buffer(unsigned char **buffer, int depth); // video
	int get_row_pointers(unsigned char *buffer, unsigned char ***pointers, int depth);
	static int match4(const char *in, const char *out);   // match 4 bytes for a fourcc type

	float *float_buffer;          // for floating point feathering
	unsigned char **row_pointers_in, **row_pointers_out;
	int64_t prev_buffer_position;  // for audio determines if reading raw data is necessary
	int64_t prev_frame_position;   // for video determines if reading raw video data is necessary
	int64_t prev_bytes; // determines if new raw buffer is needed and used for getting memory usage
	int64_t prev_len;
	int prev_track;
	int prev_layer;
	int dither;
	int internal_byte_order;
	File *file;

// ================================= Audio compression
	double **pcm_history;
	int64_t history_allocated;
	int64_t history_size;
	int64_t history_start;
	int history_channels;
// Range to decode to fill history buffer.  Maintained by FileBase.
	int64_t decode_start;
	int64_t decode_len;
// End of last decoded sample.  Maintained by user for seeking.
	int64_t decode_end;

private:
// ULAW
	float ulawtofloat(char ulaw);
	char floattoulaw(float value);
	int generate_ulaw_tables();
	int delete_ulaw_tables();
	float *ulawtofloat_table, *ulawtofloat_ptr;
	unsigned char *floattoulaw_table, *floattoulaw_ptr;
};

#endif
