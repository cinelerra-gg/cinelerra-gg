
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

#ifndef FILE_H
#define FILE_H

#include <stdlib.h>

#include "asset.inc"
#include "condition.inc"
#include "edit.inc"
#include "filebase.inc"
#include "file.inc"

#include "filethread.inc"
#include "filexml.inc"
#include "formattools.inc"
#include "formatwindow.inc"
#include "formattools.h"
#include "framecache.inc"
#include "guicast.h"
#include "indexfile.inc"
#include "mainprogress.inc"
#include "mutex.inc"
#include "packagingengine.inc"
#include "pluginserver.inc"
#include "preferences.inc"
#include "samples.inc"
#include "vframe.inc"
#include "packagingengine.h"

// ======================================= include file types here



// generic file opened by user
class File
{
public:
	File();
	~File();

	int probe();
// Get attributes for various file formats.
// The dither parameter is carried over from recording, where dither is done at the device.
	int get_options(FormatTools *format,
                int audio_options,
                int video_options);

	int raise_window();
// Close parameter window
	void close_window();

// ===================================== start here
	int set_processors(int cpus);   // Set the number of cpus for certain codecs.
// Set the number of bytes to preload during reads for Quicktime.
	int set_preload(int64_t size);
// Set the subtitle for libzmpeg3.  -1 disables subtitles.
	void set_subtitle(int value);
// Set whether to interpolate raw images
	void set_interpolate_raw(int value);
// Set whether to white balance raw images.  Always 0 if no interpolation.
	void set_white_balance_raw(int value);
// When loading, the asset is deleted and a copy created in the EDL.
//	void set_asset(Asset *asset);

// Enable or disable frame caching.  Must be tied to file to know when
// to delete the file object.  Otherwise we'd delete just the cached frames
// while the list of open files grew.
	void set_cache_frames(int value);
// Delete oldest frame from cache.
// Return number of bytes freed if successful.
// Return 0 if nothing to delete.
	int purge_cache();
// Delete oldest frame from cache.  Return 0 if successful.  Return 1 if
// nothing to delete.
	int delete_oldest();

// Format may be preset if the asset format is not 0.
	int open_file(Preferences *preferences,
		Asset *asset,
		int rd,
		int wr);

// Get index from the file if one exists.  Returns 0 on success.
	int get_index(IndexFile *index_file, MainProgressBar *progress_bar);

// start a thread for writing to avoid blocking during record
	int start_audio_thread(int buffer_size, int ring_buffers);
	int stop_audio_thread();
// The ring buffer must either be 1 or 2.
// The buffer_size for video needs to be > 1 on SMP systems to utilize
// multiple processors.
// For audio it's the number of samples per buffer.
// compressed - if 1 write_compressed_frame is called
//              if 0 write_frames is called
	int start_video_thread(int buffer_size,
		int color_model,
		int ring_buffers,
		int compressed);
	int stop_video_thread();

	int start_video_decode_thread();

// Return the thread.
// Used by functions that read only.
	FileThread* get_video_thread();

// write any headers and close file
// ignore_thread is used by SigHandler to break out of the threads.
	int close_file(int ignore_thread = 0);
	void delete_temp_samples_buffer();
	void delete_temp_frame_buffer();

// get length of file normalized to base samplerate
	int64_t get_audio_length();
	int64_t get_video_length();

// get current position
	int64_t get_audio_position();
	int64_t get_video_position();



// write samples for the current channel
// written to disk and file pointer updated after last channel is written
// return 1 if failed
// subsequent writes must be <= than first write's size because of buffers
	int write_samples(Samples **buffer, int64_t len);

// Only called by filethread to write an array of an array of channels of frames.
	int write_frames(VFrame ***frames, int len);



// For writing buffers in a background thread use these functions to get the buffer.
// Get a pointer to a buffer to write to.
	Samples** get_audio_buffer();
	VFrame*** get_video_buffer();

// Used by ResourcePixmap to directly access the cache.
	FrameCache* get_frame_cache();

// Schedule a buffer for writing on the thread.
// thread calls write_samples
	int write_audio_buffer(int64_t len);
	int write_video_buffer(int64_t len);




// set channel for buffer accesses
	int set_channel(int channel);
	int get_channel();

// set dvd program, no=-1 get current program
	int set_program(int no);
// access cell times
	int get_cell_time(int no, double &time);
// return stream pid
	int get_video_pid(int track);
// return video info
	int get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title);
// update asset with selected audio/video layer data
	int select_video_stream(Asset *asset, int vstream);
	int select_audio_stream(Asset *asset, int astream);
// get dvb system_time
	int get_system_time(int64_t &tm);
// get dvb audio channels for video stream
	int get_audio_for_video(int vstream, int astream, int64_t &channel_mask);
// get frame thumbnail data
	int get_thumbnail(int stream,
		int64_t &position, unsigned char *&thumbnail, int &ww, int &hh);
	int set_skimming(int track, int skim, skim_fn fn, void *vp);
	int skim_video(int track, void *vp, skim_fn fn);

// set position in samples
	int set_audio_position(int64_t position);

// Read samples for one channel into a shared memory segment.
// The offset is the offset in floats from the beginning of the buffer and the len
// is the length in floats from the offset.
// advances file pointer
// return 1 if failed
	int read_samples(Samples *buffer, int64_t len);

// set layer for video read
// is_thread is used by FileThread::run to prevent recursive lockup.
	int set_layer(int layer, int is_thread = 0);
// set position in frames
// is_thread is set by FileThread::run to prevent recursive lockup.
//	int set_video_position(int64_t position, float base_framerate /* = -1 */, int is_thread /* = 0 */);
	int set_video_position(int64_t position, int is_thread /* = 0 */);

// Read frame of video into the argument
// is_thread is used by FileThread::run to prevent recursive lockup.
	int read_frame(VFrame *frame, int is_thread = 0);


// The following involve no extra copies.
// Direct copy routines for direct copy playback
	int can_copy_from(Asset *asset, int64_t position, int output_w, int output_h); // This file can copy frames directly from the asset
	int get_render_strategy(ArrayList<int>* render_strategies);
	int64_t compressed_frame_size();
	int read_compressed_frame(VFrame *buffer);
	int write_compressed_frame(VFrame *buffer);

// These are separated into two routines so a file doesn't have to be
// allocated.
// Get best colormodel to translate for hardware accelerated playback.
// Called by VRender.
	int get_best_colormodel(int driver, int vstream=-1);
// Get best colormodel for hardware accelerated recording.
// Called by VideoDevice.
	static int get_best_colormodel(Asset *asset, int driver);
// asset scaling on read_frame
	static int can_scale_input(Asset *asset);
// Get nearest colormodel that can be decoded without a temporary frame.
// Used by read_frame.
	int colormodel_supported(int colormodel);

// stubs for now
	static const char *compressiontostr(const char *codec) { return codec; }
	static const char *strtocompression(const char *string) { return string; }
// subclass memory usage
	int64_t file_memory_usage();
// Used by CICache to calculate the total size of the cache.
// Based on temporary frames and a call to the file subclass.
// The return value is limited 1MB each in case of audio file.
// The minimum setting for cache_size should be bigger than 1MB.
	int64_t get_memory_usage();

	static int renders_video(int format);
	static int renders_video(Asset *asset);
	static int renders_audio(int format);
	static int renders_audio(Asset *asset);
// Get the extension for the filename
	static const char* get_tag(int format);
	static const char* get_prefix(int format);
	static int strtoformat(const char *format);
	static const char* formattostr(int format);
	static int is_image_render(int format);
	static int strtobits(const char *bits);
	static const char* bitstostr(int bits);
	static int str_to_byteorder(const char *string);
	static const char* byteorder_to_str(int byte_order);
	int bytes_per_sample(int bits); // Convert the bit descriptor into a byte count.
// get record stream file descriptor
	int record_fd();

	Asset *asset;    // Copy of asset since File outlives EDL
	FileBase *file; // virtual class for file type
// Threads for writing data in the background.
	FileThread *audio_thread, *video_thread;

// Temporary storage for color conversions
	VFrame *temp_frame;

// Temporary storage for get_audio_buffer.
// [ring buffers][channels][Samples]
	Samples ***temp_samples_buffer;

// Temporary storage for get_video_buffer.
// [Ring buffers][layers][temp_frame_size][VFrame]
	VFrame ****temp_frame_buffer;
// Return value of get_video_buffer
	VFrame ***current_frame_buffer;
// server copies of variables for threaded recording
	int audio_ring_buffers;
	int video_ring_buffers;
// Number of frames in the temp_frame_buffer
	int video_buffer_size;

// Lock writes while recording video and audio.
// A binary lock won't do.  We need a FIFO lock.
	Condition *write_lock;
	int cpus;
	int64_t playback_preload;
	int playback_subtitle;
	int interpolate_raw;
	int white_balance_raw;

// Position information is migrated here to allow samplerate conversion.
// Current position in file's samplerate.
// Can't normalize to base samplerate because this would
// require fractional positioning to know if the file's position changed.
	int64_t current_sample;
	int64_t current_frame;
	int current_channel;
	int current_layer;
	int current_program;

// Position information normalized to project rates
	int64_t normalized_sample;
//	int64_t normalized_sample_rate;
	Preferences *preferences;
	int wr, rd;
	static PackagingEngine *new_packaging_engine(Asset *asset);

	static void init_cin_path();
	static void get_exe_path(char *result, char *bnp=0);
	static void getenv_path(char *result, const char *path);
	static void setenv_path(const char *var, const char *path, int overwrite);
	static const char *get_cin() { return getenv("CIN_PKG"); }
	static const char *get_cin_path() { return getenv("CIN_PATH"); }
	static const char *get_cindat_path() { return getenv("CIN_DAT"); }
	static const char *get_cinlib_path() { return getenv("CIN_LIB"); }
	static const char *get_config_path() { return getenv("CIN_CONFIG"); }
	static const char *get_plugin_path() { return getenv("CIN_PLUGIN"); }
	static const char *get_ladspa_path() { return getenv("CIN_LADSPA"); }
	static const char *get_locale_path() { return getenv("CIN_LOCALE"); }
	static const char *get_render_path() { return getenv("CIN_RENDER"); }
	static const char *get_browser_path() { return getenv("CIN_BROWSER"); }

	static const char *default_probes[];
	static const int nb_probes;

private:
	void reset_parameters();

	int getting_options;
	BC_WindowBase *format_window;
	Condition *format_completion;
	FrameCache *frame_cache;
// Copy read frames to the cache
	int use_cache;
};

#endif
