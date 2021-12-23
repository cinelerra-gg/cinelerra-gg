/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>
// work arounds (centos)
#include <lzma.h>
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif

#include "asset.h"
#include "bchash.h"
#include "bcsignals.h"
#include "byteorder.h"
#include "cache.inc"
#include "condition.h"
#include "errorbox.h"
#include "fileac3.h"
#include "filebase.h"
#include "filecr2.h"
#include "filedb.h"
#include "filedv.h"
#include "fileexr.h"
#include "fileffmpeg.h"
#include "fileflac.h"
#include "filegif.h"
#include "file.h"
#include "filejpeg.h"
#include "filempeg.h"
#undef HAVE_STDLIB_H // automake conflict
#include "filepng.h"
#include "fileppm.h"
#include "filescene.h"
#include "filesndfile.h"
#include "filetga.h"
#include "filethread.h"
#include "filetiff.h"
#include "filexml.h"
#include "formatwindow.h"
#include "formattools.h"
#include "framecache.h"
#include "language.h"
#include "mainprogress.inc"
#include "mutex.h"
#include "mwindow.h"
#include "packagingengine.h"
#include "pluginserver.h"
#include "preferences.h"
#include "probeprefs.h"
#include "samples.h"
#include "vframe.h"

File::File()
{
	cpus = 1;
	asset = new Asset;
	format_completion = new Condition(1, "File::format_completion");
	write_lock = new Condition(1, "File::write_lock");
	frame_cache = new FrameCache;
	reset_parameters();
}

File::~File()
{
	if( getting_options ) {
		if( format_window ) format_window->set_done(0);
		format_completion->lock("File::~File");
		format_completion->unlock();
	}

	if( temp_frame ) {
//printf("File::~File %d temp_debug=%d\n", __LINE__, --temp_debug);
		delete temp_frame;
	}

	close_file(0);

	asset->Garbage::remove_user();
	delete format_completion;
	delete write_lock;
	delete frame_cache;
}

void File::reset_parameters()
{
	file = 0;
	audio_thread = 0;
	video_thread = 0;
	getting_options = 0;
	format_window = 0;
	temp_frame = 0;
	current_sample = 0;
	current_frame = 0;
	current_channel = 0;
	current_program = 0;
	current_layer = 0;
	normalized_sample = 0;
	use_cache = 0;
	preferences = 0;
	playback_subtitle = -1;
	interpolate_raw = 1;


	temp_samples_buffer = 0;
	temp_frame_buffer = 0;
	current_frame_buffer = 0;
	audio_ring_buffers = 0;
	video_ring_buffers = 0;
	video_buffer_size = 0;
}

int File::raise_window()
{
	if( getting_options && format_window ) {
		format_window->raise_window();
		format_window->flush();
	}
	return 0;
}

void File::close_window()
{
	if( getting_options ) {
		format_window->lock_window("File::close_window");
		format_window->set_done(1);
		format_window->unlock_window();
		getting_options = 0;
	}
}

int File::get_options(FormatTools *format,
	int audio_options, int video_options)
{
	BC_WindowBase *parent_window = format->window;
	Asset *asset = format->asset;
	EDL *edl = format->mwindow ? format->mwindow->edl : 0;
	format_window = 0;
	getting_options = 1;
	format_completion->lock("File::get_options");
	switch( asset->format ) {
#ifdef HAVE_CIN_3RDPARTY
	case FILE_AC3: FileAC3::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#endif
#ifdef HAVE_DV
	case FILE_RAWDV:
		FileDV::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#endif
	case FILE_PCM:
	case FILE_WAV:
	case FILE_AU:
	case FILE_AIFF:
	case FILE_SND:
		FileSndFile::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	case FILE_FFMPEG:
		FileFFMPEG::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#ifdef HAVE_LIBZMPEG
	case FILE_AMPEG:
	case FILE_VMPEG:
		FileMPEG::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#endif
	case FILE_JPEG:
	case FILE_JPEG_LIST:
		FileJPEG::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#ifdef HAVE_OPENEXR
	case FILE_EXR:
	case FILE_EXR_LIST:
		FileEXR::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
#endif
	case FILE_FLAC:
		FileFLAC::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	case FILE_PNG:
	case FILE_PNG_LIST:
		FilePNG::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	case FILE_PPM:
	case FILE_PPM_LIST:
		FilePPM::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	case FILE_TGA:
	case FILE_TGA_LIST:
		FileTGA::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	case FILE_TIFF:
	case FILE_TIFF_LIST:
		FileTIFF::get_parameters(parent_window, asset, format_window,
			audio_options, video_options, edl);
		break;
	default:
		break;
	}

	if( !format_window ) {
		ErrorBox *errorbox = new ErrorBox(_(PROGRAM_NAME ": Error"),
			parent_window->get_abs_cursor_x(1),
			parent_window->get_abs_cursor_y(1));
		format_window = errorbox;
		getting_options = 1;
		if( audio_options )
			errorbox->create_objects(_("This format doesn't support audio."));
		else
		if( video_options )
			errorbox->create_objects(_("This format doesn't support video."));
		errorbox->run_window();
		delete errorbox;
	}

	getting_options = 0;
	format_window = 0;
	format_completion->unlock();
	return 0;
}

int File::can_scale_input(Asset *asset)
{
	switch( asset->format ) {
	case FILE_MPEG:
	case FILE_FFMPEG:
		return 1;
	case FILE_EXR:
	case FILE_JPEG:
	case FILE_PNG:
	case FILE_PPM:
	case FILE_TGA:
	case FILE_TIFF:
		if( asset->video_length < 0 )
			return 1;
	}
	return 0;
}


int File::set_processors(int cpus)   // Set the number of cpus for certain codecs
{
	if( cpus > 8 )		// mpegvideo max_threads = 16, more causes errs
		cpus = 8;	//  8 cpus ought to decode just about anything
// Set all instances so gets work.
	this->cpus = cpus;

	return 0;
}

int File::set_preload(int64_t size)
{
	this->playback_preload = size;
	return 0;
}

void File::set_subtitle(int value)
{
	this->playback_subtitle = value;
	if( file ) file->set_subtitle(value);
}

void File::set_interpolate_raw(int value)
{
	this->interpolate_raw = value;
}

void File::set_white_balance_raw(int value)
{
	this->white_balance_raw = value;
}

void File::set_cache_frames(int value)
{
// caching only done locally
	if( !video_thread )
		use_cache = value;
}

int File::purge_cache()
{
// caching only done locally
	int result = 0;
	if( frame_cache->cache_items() > 0 ) {
		frame_cache->remove_all();
		result = 1;
	}
	return result;
}

int File::delete_oldest()
{
// return the number of bytes freed
	return frame_cache->delete_oldest();
}

// file driver in order of probe precidence
//  can be reordered in preferences->interface
const char *File::default_probes[] = { 
	"FFMPEG_Early",
	"Scene", 
	"DB",
#ifdef HAVE_DV 
	"DV",   
#endif 
	"SndFile",
	"PNG",
	"PPM",
	"JPEG",
	"GIF",
#ifdef HAVE_OPENEXR
	"EXR",
#endif
	"FLAC",
	"CR2",
	"TGA",
	"TIFF",
	"MPEG",
	"EDL",
       	"FFMPEG_Late", 
}; 
const int File::nb_probes =
	sizeof(File::default_probes)/sizeof(File::default_probes[0]); 


int File::probe()
{
	FILE *fp = fopen(this->asset->path, "rb");
	if( !fp ) return FILE_NOT_FOUND;
	char data[16];
	memset(data,0,sizeof(data));
	int ret = fread(data, 1, 16, fp);
	fclose(fp);
	if( !ret ) return FILE_NOT_FOUND;

	for( int i=0; i<preferences->file_probes.size(); ++i ) {
		ProbePref *pref = preferences->file_probes[i];
		if( !pref->armed ) continue;
		if( !strncmp(pref->name,"FFMPEG",6) ) { // FFMPEG Early/Late
			if( !FileFFMPEG::check_sig(this->asset) ) continue;
			file = new FileFFMPEG(this->asset, this);
			return FILE_OK;
		}
#ifdef HAVE_COMMERCIAL
		if( !strcmp(pref->name,"DB") ) { // MediaDB
			if( !FileDB::check_sig(this->asset) ) continue;
			file = new FileDB(this->asset, this);
			return FILE_OK;
		}
#endif
		if( !strcmp(pref->name,"Scene") ) { // scene file
			if( !FileScene::check_sig(this->asset, data)) continue;
			file = new FileScene(this->asset, this);
			return FILE_OK;
		}
#ifdef HAVE_DV
		if( !strcmp(pref->name,"DV") ) { // libdv
			if( !FileDV::check_sig(this->asset) ) continue;
			file = new FileDV(this->asset, this);
			return FILE_OK;
		}
#endif
		if( !strcmp(pref->name,"SndFile") ) { // libsndfile
			if( !FileSndFile::check_sig(this->asset) ) continue;
			file = new FileSndFile(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"PNG") ) { // PNG file
			if( !FilePNG::check_sig(this->asset) ) continue;
			file = new FilePNG(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"PPM") ) { // PPM file
			if( !FilePPM::check_sig(this->asset) ) continue;
			file = new FilePPM(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"JPEG") ) { // JPEG file
			if( !FileJPEG::check_sig(this->asset) ) continue;
			file = new FileJPEG(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"GIF") ) { // GIF file
			if( FileGIFList::check_sig(this->asset) )
				file = new FileGIFList(this->asset, this);
			else if( FileGIF::check_sig(this->asset) )
				file = new FileGIF(this->asset, this);
			else continue;
			return FILE_OK;
		}
#ifdef HAVE_EXR
		if( !strcmp(pref->name,"EXR") ) { // EXR file
			if( !FileEXR::check_sig(this->asset, data)) continue;
			file = new FileEXR(this->asset, this);
			return FILE_OK;
		}
#endif
		if( !strcmp(pref->name,"FLAC") ) { // FLAC file
			if( !FileFLAC::check_sig(this->asset, data)) continue;
			file = new FileFLAC(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"CR2") ) { // CR2 file
			if( !FileCR2::check_sig(this->asset)) continue;
			file = new FileCR2(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"TGA") ) { // TGA file
			if( !FileTGA::check_sig(this->asset) ) continue;
			file = new FileTGA(this->asset, this);
			return FILE_OK;
		}
		if( !strcmp(pref->name,"TIFF") ) { // TIFF file
			if( !FileTIFF::check_sig(this->asset) ) continue;
			file = new FileTIFF(this->asset, this);
			return FILE_OK;
		}
#ifdef HAVE_LIBZMPEG
		if( !strcmp(pref->name,"MPEG") ) { // MPEG file
			if( !FileMPEG::check_sig(this->asset) ) continue;
			file = new FileMPEG(this->asset, this);
			return FILE_OK;
		}
#endif
		if( !strcmp(pref->name,"EDL") ) { // XML file
			if( data[0] != '<' ) continue;
			if( !strncmp(&data[1],"EDL>",4) ||
			    !strncmp(&data[1],"HTAL>",5) ||
			    !strncmp(&data[1],"?xml",4) )
				return FILE_IS_XML;
		}
	}
	return FILE_UNRECOGNIZED_CODEC; // maybe PCM file ?
}

int File::open_file(Preferences *preferences,
	Asset *asset,
	int rd,
	int wr)
{
	const int debug = 0;

	this->preferences = preferences;
	this->asset->copy_from(asset, 1);
	this->rd = rd;
	this->wr = wr;
	file = 0;

	if( debug ) printf("File::open_file %p %d\n", this, __LINE__);

	switch( this->asset->format ) {
// get the format now
// If you add another format to case 0, you also need to add another case for the
// file format #define.
	case FILE_UNKNOWN: {
		int ret = probe();
		if( ret != FILE_OK ) return ret;
		break; }
// format already determined
#ifdef HAVE_CIN_3RDPARTY
	case FILE_AC3:
		file = new FileAC3(this->asset, this);
		break;
#endif
	case FILE_SCENE:
		file = new FileScene(this->asset, this);
		break;

	case FILE_FFMPEG:
		file = new FileFFMPEG(this->asset, this);
		break;

	case FILE_PCM:
	case FILE_WAV:
	case FILE_AU:
	case FILE_AIFF:
	case FILE_SND:
//printf("File::open_file 1\n");
		file = new FileSndFile(this->asset, this);
		break;

	case FILE_PNG:
	case FILE_PNG_LIST:
		file = new FilePNG(this->asset, this);
		break;

	case FILE_PPM:
	case FILE_PPM_LIST:
		file = new FilePPM(this->asset, this);
		break;

	case FILE_JPEG:
	case FILE_JPEG_LIST:
		file = new FileJPEG(this->asset, this);
		break;

	case FILE_GIF:
		file = new FileGIF(this->asset, this);
		break;
	case FILE_GIF_LIST:
		file = new FileGIFList(this->asset, this);
		break;

#ifdef HAVE_OPENEXR
	case FILE_EXR:
	case FILE_EXR_LIST:
		file = new FileEXR(this->asset, this);
		break;
#endif
	case FILE_FLAC:
		file = new FileFLAC(this->asset, this);
		break;

	case FILE_CR2:
	case FILE_CR2_LIST:
		file = new FileCR2(this->asset, this);
		break;

	case FILE_TGA_LIST:
	case FILE_TGA:
		file = new FileTGA(this->asset, this);
		break;

	case FILE_TIFF:
	case FILE_TIFF_LIST:
		file = new FileTIFF(this->asset, this);
		break;
#ifdef HAVE_COMMERCIAL
	case FILE_DB:
		file = new FileDB(this->asset, this);
		break;
#endif

#ifdef HAVE_LIBZMPEG
	case FILE_MPEG:
	case FILE_AMPEG:
	case FILE_VMPEG:
		file = new FileMPEG(this->asset, this);
		break;
#endif
#ifdef HAVE_DV
	case FILE_RAWDV:
		file = new FileDV(this->asset, this);
		break;
#endif
// try plugins
	default:
		return 1;
		break;
	}


// Reopen file with correct parser and get header.
	if( file->open_file(rd, wr) ) {
		delete file;  file = 0;
		return FILE_NOT_FOUND;
	}


// If file type is a list verify that all files match in dimensions.
// Should be done only after the file open function has been performed
// Reason: although this function checks if file exists or not but
// it has no way of relaying this information back and if this function
// is called before open_file the program may accidently interpret file
// not found as file size don't match
	if( !file->verify_file_list() ) {
		delete file;  file = 0;
		return FILE_SIZE_DONT_MATCH;
	}


// Set extra writing parameters to mandatory settings.
	if( wr ) {
		if( this->asset->dither ) file->set_dither();
	}

	if( rd ) {
// one frame image file, not brender, no specific length
		if( !this->asset->audio_data && this->asset->use_header &&
		    this->asset->video_data && !this->asset->single_frame &&
		    this->asset->video_length >= 0 && this->asset->video_length <= 1 ) {
			this->asset->single_frame = 1;
			this->asset->video_length = -1;
		}
	}

// Synchronize header parameters
	asset->copy_from(this->asset, 1);
//asset->dump();

	if( debug ) printf("File::open_file %d file=%p\n", __LINE__, file);
// sleep(1);

	return FILE_OK;
}

void File::delete_temp_samples_buffer()
{

	if( temp_samples_buffer ) {
		for( int j = 0; j < audio_ring_buffers; j++ ) {
			for( int i = 0; i < asset->channels; i++ ) {
				delete temp_samples_buffer[j][i];
			}
			delete [] temp_samples_buffer[j];
		}

		delete [] temp_samples_buffer;
		temp_samples_buffer = 0;
		audio_ring_buffers = 0;
	}
}

void File::delete_temp_frame_buffer()
{

	if( temp_frame_buffer ) {
		for( int k = 0; k < video_ring_buffers; k++ ) {
			for( int i = 0; i < asset->layers; i++ ) {
				for( int j = 0; j < video_buffer_size; j++ ) {
					delete temp_frame_buffer[k][i][j];
				}
				delete [] temp_frame_buffer[k][i];
			}
			delete [] temp_frame_buffer[k];
		}

		delete [] temp_frame_buffer;
		temp_frame_buffer = 0;
		video_ring_buffers = 0;
		video_buffer_size = 0;
	}
}

int File::close_file(int ignore_thread)
{
	if( !ignore_thread ) {
		stop_audio_thread();
		stop_video_thread();
	}
	if( file ) {
// The file's asset is a copy of the argument passed to open_file so the
// user must copy lengths from the file's asset.
		if( asset && wr ) {
			asset->audio_length = current_sample;
			asset->video_length = current_frame;
		}

		file->close_file();
		delete file;
	}

	delete_temp_samples_buffer();
	delete_temp_frame_buffer();
	reset_parameters();
	return 0;
}

int File::get_index(IndexFile *index_file, MainProgressBar *progress_bar)
{
	return !file ? -1 : file->get_index(index_file, progress_bar);
}

int File::start_audio_thread(int buffer_size, int ring_buffers)
{
	this->audio_ring_buffers = ring_buffers;


	if( !audio_thread ) {
		audio_thread = new FileThread(this, 1, 0);
		audio_thread->start_writing(buffer_size, 0, ring_buffers, 0);
	}
	return 0;
}

int File::start_video_thread(int buffer_size, int color_model,
		int ring_buffers, int compressed)
{
	this->video_ring_buffers = ring_buffers;
	this->video_buffer_size = buffer_size;

	if( !video_thread ) {
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_writing(buffer_size, color_model,
				ring_buffers, compressed);
	}
	return 0;
}

int File::start_video_decode_thread()
{
// Currently, CR2 is the only one which won't work asynchronously, so
// we're not using a virtual function yet.
	if( !video_thread /* && asset->format != FILE_CR2 */ ) {
		video_thread = new FileThread(this, 0, 1);
		video_thread->start_reading();
		use_cache = 0;
	}
	return 0;
}

int File::stop_audio_thread()
{
	if( audio_thread ) {
		audio_thread->stop_writing();
		delete audio_thread;
		audio_thread = 0;
	}
	return 0;
}

int File::stop_video_thread()
{
	if( video_thread ) {
		video_thread->stop_reading();
		video_thread->stop_writing();
		delete video_thread;
		video_thread = 0;
	}
	return 0;
}

FileThread* File::get_video_thread()
{
	return video_thread;
}

int File::set_channel(int channel)
{
	if( file && channel < asset->channels ) {
		current_channel = channel;
		return 0;
	}
	else
		return 1;
}

int File::get_channel()
{
	return current_channel;
}

// if no>=0, sets new program
//  returns current program
int File::set_program(int no)
{
	int result = file ? file->set_program(no) : current_program;
	current_program = no < 0 ? result : no;
	return result;
}

int File::get_cell_time(int no, double &time)
{
	return file ? file->get_cell_time(no, time) : -1;
}

int File::get_system_time(int64_t &tm)
{
	return file ? file->get_system_time(tm) : -1;
}

int File::get_audio_for_video(int vstream, int astream, int64_t &channel_mask)
{
	return file ? file->get_audio_for_video(vstream, astream, channel_mask) : -1;
}

int File::get_video_pid(int track)
{
	return file ? file->get_video_pid(track) : -1;
}



int File::get_video_info(int track, int &pid, double &framerate,
		int &width, int &height, char *title)
{
	return !file ? -1 :
		 file->get_video_info(track, pid, framerate, width, height, title);
}

int File::select_video_stream(Asset *asset, int vstream)
{
	return !file ? -1 :
		 file->select_video_stream(asset, vstream);
}

int File::select_audio_stream(Asset *asset, int astream)
{
	return !file ? -1 :
		 file->select_audio_stream(asset, astream);
}


int File::get_thumbnail(int stream,
	int64_t &position, unsigned char *&thumbnail, int &ww, int &hh)
{
	return file->get_thumbnail(stream, position, thumbnail, ww, hh);
}

int File::set_skimming(int track, int skim, skim_fn fn, void *vp)
{
	return file->set_skimming(track, skim, fn, vp);
}

int File::skim_video(int track, void *vp, skim_fn fn)
{
	return file->skim_video(track, vp, fn);
}


int File::set_layer(int layer, int is_thread)
{
	if( file && layer < asset->layers ) {
		if( !is_thread && video_thread ) {
			video_thread->set_layer(layer);
		}
		else {
			current_layer = layer;
		}
		return 0;
	}
	else
		return 1;
}

int64_t File::get_audio_length()
{
	int64_t result = asset->audio_length;
	int64_t base_samplerate = -1;
	if( result > 0 ) {
		if( base_samplerate > 0 )
			return (int64_t)((double)result / asset->sample_rate * base_samplerate + 0.5);
		else
			return result;
	}
	else
		return -1;
}

int64_t File::get_video_length()
{
	int64_t result = asset->video_length;
	float base_framerate = -1;
	if( result > 0 ) {
		if( base_framerate > 0 )
			return (int64_t)((double)result / asset->frame_rate * base_framerate + 0.5);
		else
			return result;
	}
	else
		return -1;  // infinity
}


int64_t File::get_video_position()
{
	float base_framerate = -1;
	if( base_framerate > 0 )
		return (int64_t)((double)current_frame / asset->frame_rate * base_framerate + 0.5);
	else
		return current_frame;
}

int64_t File::get_audio_position()
{
// 	int64_t base_samplerate = -1;
// 	if( base_samplerate > 0 )
// 	{
// 		if( normalized_sample_rate == base_samplerate )
// 			return normalized_sample;
// 		else
// 			return (int64_t)((double)current_sample /
// 				asset->sample_rate *
// 				base_samplerate +
// 				0.5);
// 	}
// 	else
		return current_sample;
}



int File::set_audio_position(int64_t position)
{
	int result = 0;

	if( !file ) return 1;

#define REPOSITION(x, y) \
	(labs((x) - (y)) > 1)

	float base_samplerate = asset->sample_rate;
	current_sample = normalized_sample = position;

// printf("File::set_audio_position %d normalized_sample=%ld\n",
// __LINE__,
// normalized_sample);
		result = file->set_audio_position(current_sample);

		if( result )
			printf("File::set_audio_position position=%jd"
				" base_samplerate=%f asset=%p asset->sample_rate=%d\n",
				position, base_samplerate, asset, asset->sample_rate);

//printf("File::set_audio_position %d %d %d\n", current_channel, current_sample, position);

	return result;
}

int File::set_video_position(int64_t position,
	int is_thread)
{
	int result = 0;
	if( !file ) return 0;

// Convert to file's rate
// 	if( base_framerate > 0 )
// 		position = (int64_t)((double)position /
// 			base_framerate *
// 			asset->frame_rate +
// 			0.5);


	if( video_thread && !is_thread ) {
// Call thread.  Thread calls this again to set the file state.
		video_thread->set_video_position(position);
	}
	else
	if( current_frame != position ) {
		if( file ) {
			current_frame = position;
			result = file->set_video_position(current_frame);
		}
	}

	return result;
}

// No resampling here.
int File::write_samples(Samples **buffer, int64_t len)
{
	int result = 1;

	if( file ) {
		write_lock->lock("File::write_samples");

// Convert to arrays for backwards compatability
		double *temp[asset->channels];
		for( int i = 0; i < asset->channels; i++ ) {
			temp[i] = buffer[i]->get_data();
		}

		result = file->write_samples(temp, len);
		current_sample += len;
		normalized_sample += len;
		asset->audio_length += len;
		write_lock->unlock();
	}
	return result;
}





// Can't put any cmodel abstraction here because the filebase couldn't be
// parallel.
int File::write_frames(VFrame ***frames, int len)
{
//printf("File::write_frames %d\n", __LINE__);
//PRINT_TRACE
// Store the counters in temps so the filebase can choose to overwrite them.
	int result;
	int current_frame_temp = current_frame;
	int video_length_temp = asset->video_length;

	write_lock->lock("File::write_frames");

//PRINT_TRACE
	result = file->write_frames(frames, len);
//PRINT_TRACE

	current_frame = current_frame_temp + len;
	asset->video_length = video_length_temp + len;
	write_lock->unlock();
//PRINT_TRACE
	return result;
}

// Only called by FileThread
int File::write_compressed_frame(VFrame *buffer)
{
	int result = 0;
	write_lock->lock("File::write_compressed_frame");
	result = file->write_compressed_frame(buffer);
	current_frame++;
	asset->video_length++;
	write_lock->unlock();
	return result;
}


int File::write_audio_buffer(int64_t len)
{
	int result = 0;
	if( audio_thread ) {
		result = audio_thread->write_buffer(len);
	}
	return result;
}

int File::write_video_buffer(int64_t len)
{
	int result = 0;
	if( video_thread ) {
		result = video_thread->write_buffer(len);
	}

	return result;
}

Samples** File::get_audio_buffer()
{
	if( audio_thread ) return audio_thread->get_audio_buffer();
	return 0;
}

VFrame*** File::get_video_buffer()
{
	if( video_thread ) {
		VFrame*** result = video_thread->get_video_buffer();

		return result;
	}

	return 0;
}


int File::read_samples(Samples *samples, int64_t len)
{
// Never try to read more samples than exist in the file
	if( asset->audio_length >= 0 && current_sample + len > asset->audio_length ) {
		len = asset->audio_length - current_sample;
	}
	if( len <= 0 ) return 0;

	int result = 0;
	const int debug = 0;
	if( debug ) PRINT_TRACE

	if( debug ) PRINT_TRACE

	double *buffer = samples->get_data();

	int64_t base_samplerate = asset->sample_rate;

	if( file ) {
// Resample recursively calls this with the asset sample rate
		if( base_samplerate == 0 ) base_samplerate = asset->sample_rate;

		if( debug ) PRINT_TRACE
		result = file->read_samples(buffer, len);

		if( debug ) PRINT_TRACE
		current_sample += len;

		normalized_sample += len;
	}
	if( debug ) PRINT_TRACE

	return result;
}

int File::read_frame(VFrame *frame, int is_thread)
{
	const int debug = 0;
//printf("File::read_frame pos=%jd cache=%d 1frame=%d\n",
// current_frame, use_cache, asset->single_frame);
	if( video_thread && !is_thread ) return video_thread->read_frame(frame);
	if( !file ) return 1;
	if( debug ) PRINT_TRACE
	int result = 0;
	int supported_colormodel = colormodel_supported(frame->get_color_model());
	int advance_position = 1;
	int cache_active = use_cache || asset->single_frame ? 1 : 0;
	int64_t cache_position = !asset->single_frame ? current_frame : -1;

// Test cache
	if( cache_active && frame_cache->get_frame(frame, cache_position,
			current_layer, asset->frame_rate) ) {
// Can't advance position if cache used.
//printf("File::read_frame %d\n", __LINE__);
		advance_position = 0;
	}
// Need temp
	else if( frame->get_color_model() != BC_COMPRESSED &&
		(supported_colormodel != frame->get_color_model() ||
		(!file->can_scale_input() &&
			(frame->get_w() != asset->width ||
			 frame->get_h() != asset->height))) ) {

//			printf("File::read_frame %d\n", __LINE__);
// Can't advance position here because it needs to be added to cache
		if( temp_frame ) {
			if( !temp_frame->params_match(asset->width, asset->height, supported_colormodel) ) {
				delete temp_frame;
				temp_frame = 0;
			}
		}

		if( !temp_frame ) {
			temp_frame = new VFrame(asset->width, asset->height, supported_colormodel, 0);
			temp_frame->clear_frame();
		}

//			printf("File::read_frame %d\n", __LINE__);
		temp_frame->copy_stacks(frame);
		result = file->read_frame(temp_frame);
		if( !result )
			frame->transfer_from(temp_frame);
		else if( result && frame->get_status() > 0 )
			frame->set_status(-1);
//printf("File::read_frame %d\n", __LINE__);
	}
	else {
// Can't advance position here because it needs to be added to cache
//printf("File::read_frame %d\n", __LINE__);
		result = file->read_frame(frame);
		if( result && frame->get_status() > 0 )
			frame->set_status(-1);
//for( int i = 0; i < 100 * 1000; i++ ) ((float*)frame->get_rows()[0])[i] = 1.0;
	}

	if( result && !current_frame )
		frame->clear_frame();

	if( cache_active && advance_position && frame->get_status() > 0 )
		frame_cache->put_frame(frame, cache_position,
			current_layer, asset->frame_rate, 1, 0);
//printf("File::read_frame %d\n", __LINE__);

	if( advance_position ) current_frame++;
	if( debug ) PRINT_TRACE
	return 0;
}

int File::can_copy_from(Asset *asset, int64_t position, int output_w, int output_h)
{
	if( asset && file ) {
		return asset->width == output_w &&
			asset->height == output_h &&
			file->can_copy_from(asset, position);
	}
	return 0;
}

// Fill in queries about formats when adding formats here.


int File::strtoformat(const char *format)
{
	if( !strcasecmp(format, _(AC3_NAME)) ) return FILE_AC3;
	if( !strcasecmp(format, _(SCENE_NAME)) ) return FILE_SCENE;
	if( !strcasecmp(format, _(WAV_NAME)) ) return FILE_WAV;
	if( !strcasecmp(format, _(PCM_NAME)) ) return FILE_PCM;
	if( !strcasecmp(format, _(AU_NAME)) ) return FILE_AU;
	if( !strcasecmp(format, _(AIFF_NAME)) ) return FILE_AIFF;
	if( !strcasecmp(format, _(SND_NAME)) ) return FILE_SND;
	if( !strcasecmp(format, _(PNG_NAME)) ) return FILE_PNG;
	if( !strcasecmp(format, _(PNG_LIST_NAME)) ) return FILE_PNG_LIST;
	if( !strcasecmp(format, _(PPM_NAME)) ) return FILE_PPM;
	if( !strcasecmp(format, _(PPM_LIST_NAME)) ) return FILE_PPM_LIST;
	if( !strcasecmp(format, _(TIFF_NAME)) ) return FILE_TIFF;
	if( !strcasecmp(format, _(TIFF_LIST_NAME)) ) return FILE_TIFF_LIST;
	if( !strcasecmp(format, _(JPEG_NAME)) ) return FILE_JPEG;
	if( !strcasecmp(format, _(JPEG_LIST_NAME)) ) return FILE_JPEG_LIST;
	if( !strcasecmp(format, _(EXR_NAME)) ) return FILE_EXR;
	if( !strcasecmp(format, _(EXR_LIST_NAME)) ) return FILE_EXR_LIST;
	if( !strcasecmp(format, _(FLAC_NAME)) ) return FILE_FLAC;
	if( !strcasecmp(format, _(GIF_NAME)) ) return FILE_GIF;
	if( !strcasecmp(format, _(GIF_LIST_NAME)) ) return FILE_GIF_LIST;
	if( !strcasecmp(format, _(CR2_NAME)) ) return FILE_CR2;
	if( !strcasecmp(format, _(CR2_LIST_NAME)) ) return FILE_CR2_LIST;
	if( !strcasecmp(format, _(MPEG_NAME)) ) return FILE_MPEG;
	if( !strcasecmp(format, _(AMPEG_NAME)) ) return FILE_AMPEG;
	if( !strcasecmp(format, _(VMPEG_NAME)) ) return FILE_VMPEG;
	if( !strcasecmp(format, _(TGA_NAME)) ) return FILE_TGA;
	if( !strcasecmp(format, _(TGA_LIST_NAME)) ) return FILE_TGA_LIST;
	if( !strcasecmp(format, _(RAWDV_NAME)) ) return FILE_RAWDV;
	if( !strcasecmp(format, _(FFMPEG_NAME)) ) return FILE_FFMPEG;
	if( !strcasecmp(format, _(DBASE_NAME)) ) return FILE_DB;

	return 0;
}


const char* File::formattostr(int format)
{
	switch( format ) {
	case FILE_SCENE:	return _(SCENE_NAME);
	case FILE_AC3:		return _(AC3_NAME);
	case FILE_WAV:		return _(WAV_NAME);
	case FILE_PCM:		return _(PCM_NAME);
	case FILE_AU:		return _(AU_NAME);
	case FILE_AIFF:		return _(AIFF_NAME);
	case FILE_SND:		return _(SND_NAME);
	case FILE_PNG:		return _(PNG_NAME);
	case FILE_PNG_LIST:	return _(PNG_LIST_NAME);
	case FILE_PPM:		return _(PPM_NAME);
	case FILE_PPM_LIST:	return _(PPM_LIST_NAME);
	case FILE_JPEG:		return _(JPEG_NAME);
	case FILE_JPEG_LIST:	return _(JPEG_LIST_NAME);
	case FILE_CR2:		return _(CR2_NAME);
	case FILE_CR2_LIST:	return _(CR2_LIST_NAME);
	case FILE_FLAC:		return _(FLAC_NAME);
	case FILE_GIF:		return _(GIF_NAME);
	case FILE_GIF_LIST:	return _(GIF_LIST_NAME);
	case FILE_EXR:		return _(EXR_NAME);
	case FILE_EXR_LIST:	return _(EXR_LIST_NAME);
#ifdef HAVE_LIBZMPEG
	case FILE_MPEG:		return _(MPEG_NAME);
#endif
	case FILE_AMPEG:	return _(AMPEG_NAME);
	case FILE_VMPEG:	return _(VMPEG_NAME);
	case FILE_TGA:		return _(TGA_NAME);
	case FILE_TGA_LIST:	return _(TGA_LIST_NAME);
	case FILE_TIFF:		return _(TIFF_NAME);
	case FILE_TIFF_LIST:	return _(TIFF_LIST_NAME);
	case FILE_RAWDV:	return _(RAWDV_NAME);
	case FILE_FFMPEG:	return _(FFMPEG_NAME);
	case FILE_DB:		return _(DBASE_NAME);
	}
	return _("Unknown");
}

int File::strtobits(const char *bits)
{
	if( !strcasecmp(bits, _(NAME_8BIT)) ) return BITSLINEAR8;
	if( !strcasecmp(bits, _(NAME_16BIT)) ) return BITSLINEAR16;
	if( !strcasecmp(bits, _(NAME_24BIT)) ) return BITSLINEAR24;
	if( !strcasecmp(bits, _(NAME_32BIT)) ) return BITSLINEAR32;
	if( !strcasecmp(bits, _(NAME_ULAW)) ) return BITSULAW;
	if( !strcasecmp(bits, _(NAME_ADPCM)) ) return BITS_ADPCM;
	if( !strcasecmp(bits, _(NAME_FLOAT)) ) return BITSFLOAT;
	return BITSLINEAR16;
}

const char* File::bitstostr(int bits)
{
//printf("File::bitstostr\n");
	switch( bits ) {
	case BITSLINEAR8:	return (NAME_8BIT);
	case BITSLINEAR16:	return (NAME_16BIT);
	case BITSLINEAR24:	return (NAME_24BIT);
	case BITSLINEAR32:	return (NAME_32BIT);
	case BITSULAW:		return (NAME_ULAW);
	case BITS_ADPCM:	return (NAME_ADPCM);
	case BITSFLOAT:		return (NAME_FLOAT);
	}
	return _("Unknown");
}



int File::str_to_byteorder(const char *string)
{
	if( !strcasecmp(string, _("Lo Hi")) ) return 1;
	return 0;
}

const char* File::byteorder_to_str(int byte_order)
{
	if( byte_order ) return _("Lo Hi");
	return _("Hi Lo");
}

int File::bytes_per_sample(int bits)
{
	switch( bits ) {
	case BITSLINEAR8:	return 1;
	case BITSLINEAR16:	return 2;
	case BITSLINEAR24:	return 3;
	case BITSLINEAR32:	return 4;
	case BITSULAW:		return 1;
	}
	return 1;
}


int File::get_best_colormodel(int driver, int vstream)
{
	return file ? file->get_best_colormodel(driver, vstream) :
		get_best_colormodel(asset, driver);
}

int File::get_best_colormodel(Asset *asset, int driver)
{
	switch( asset->format ) {
#ifdef HAVE_DV
	case FILE_RAWDV:	return FileDV::get_best_colormodel(asset, driver);
#endif
#ifdef HAVE_LIBZMPEG
	case FILE_MPEG:		return FileMPEG::get_best_colormodel(asset, driver);
#endif
	case FILE_JPEG:
	case FILE_JPEG_LIST:	return FileJPEG::get_best_colormodel(asset, driver);
#ifdef HAVE_OPENEXR
	case FILE_EXR:
	case FILE_EXR_LIST:	return FileEXR::get_best_colormodel(asset, driver);
#endif
	case FILE_PNG:
	case FILE_PNG_LIST:	return FilePNG::get_best_colormodel(asset, driver);
	case FILE_PPM:
	case FILE_PPM_LIST:	return FilePPM::get_best_colormodel(asset, driver);
	case FILE_TGA:
	case FILE_TGA_LIST:	return FileTGA::get_best_colormodel(asset, driver);
	case FILE_CR2:
	case FILE_CR2_LIST:	return FileCR2::get_best_colormodel(asset, driver);
#ifdef HAVE_COMMERCIAL
	case FILE_DB:		return FileDB::get_best_colormodel(asset, driver);
#endif
	case FILE_FFMPEG:	return FileFFMPEG::get_best_colormodel(asset, driver);
	}

	return BC_RGB888;
}


int File::colormodel_supported(int colormodel)
{
	if( file )
		return file->colormodel_supported(colormodel);

	return BC_RGB888;
}


int64_t File::file_memory_usage()
{
	return file ? file->base_memory_usage() : 0;
}

int64_t File::get_memory_usage()
{
	int64_t result = 0;

	result += file_memory_usage();
	if( temp_frame ) result += temp_frame->get_data_size();
	result += frame_cache->get_memory_usage();
	if( video_thread ) result += video_thread->get_memory_usage();

	if( result < MIN_CACHEITEM_SIZE ) result = MIN_CACHEITEM_SIZE;
	return result;
}


int File::renders_video(int format)
{
	switch( format ) {
	case FILE_JPEG:
	case FILE_JPEG_LIST:
	case FILE_CR2:
	case FILE_CR2_LIST:
	case FILE_EXR:
	case FILE_EXR_LIST:
	case FILE_GIF:
	case FILE_GIF_LIST:
	case FILE_PNG:
	case FILE_PNG_LIST:
	case FILE_PPM:
	case FILE_PPM_LIST:
	case FILE_TGA:
	case FILE_TGA_LIST:
	case FILE_TIFF:
	case FILE_TIFF_LIST:
	case FILE_VMPEG:
	case FILE_RAWDV:
        case FILE_FFMPEG:
		return 1;
	}
	return 0;
}
int File::renders_video(Asset *asset)
{
	return asset->format == FILE_FFMPEG ?
		FFMPEG::renders_video(asset->fformat) :
		renders_video(asset->format);
}

int File::renders_audio(int format)
{
	switch( format ) {
	case FILE_AC3:
	case FILE_FLAC:
	case FILE_PCM:
	case FILE_WAV:
	case FILE_AMPEG:
	case FILE_AU:
	case FILE_AIFF:
	case FILE_SND:
	case FILE_RAWDV:
        case FILE_FFMPEG:
		return 1;
	}
	return 0;
}
int File::renders_audio(Asset *asset)
{
	return asset->format == FILE_FFMPEG ?
		FFMPEG::renders_audio(asset->fformat) :
		renders_audio(asset->format);
}

int File::is_image_render(int format)
{
	switch( format ) {
	case FILE_EXR:
	case FILE_JPEG:
	case FILE_PNG:
	case FILE_PPM:
	case FILE_TGA:
	case FILE_TIFF:
		return 1;
	}

	return 0;
}

const char* File::get_tag(int format)
{
	switch( format ) {
	case FILE_AC3:          return "ac3";
	case FILE_AIFF:         return "aif";
	case FILE_AMPEG:        return "mp3";
	case FILE_AU:           return "au";
	case FILE_RAWDV:        return "dv";
	case FILE_DB:           return "db";
	case FILE_EXR:          return "exr";
	case FILE_EXR_LIST:     return "exrs";
	case FILE_FLAC:         return "flac";
	case FILE_JPEG:         return "jpg";
	case FILE_JPEG_LIST:    return "jpgs";
	case FILE_GIF:          return "gif";
	case FILE_GIF_LIST:     return "gifs";
	case FILE_PCM:          return "pcm";
	case FILE_PNG:          return "png";
	case FILE_PNG_LIST:     return "pngs";
	case FILE_PPM:          return "ppm";
	case FILE_PPM_LIST:     return "ppms";
	case FILE_TGA:          return "tga";
	case FILE_TGA_LIST:     return "tgas";
	case FILE_TIFF:         return "tif";
	case FILE_TIFF_LIST:    return "tifs";
	case FILE_VMPEG:        return "m2v";
	case FILE_WAV:          return "wav";
	case FILE_FFMPEG:       return "ffmpg";
	}
	return 0;
}

const char* File::get_prefix(int format)
{
	switch( format ) {
	case FILE_PCM:		return "PCM";
	case FILE_WAV:		return "WAV";
	case FILE_PNG:		return "PNG";
	case FILE_PPM:		return "PPM";
	case FILE_JPEG:		return "JPEG";
	case FILE_TIFF:		return "TIFF";
	case FILE_GIF:		return "GIF";
	case FILE_JPEG_LIST:	return "JPEG_LIST";
	case FILE_AU:		return "AU";
	case FILE_AIFF:		return "AIFF";
	case FILE_SND:		return "SND";
	case FILE_TGA_LIST:	return "TGA_LIST";
	case FILE_TGA:		return "TGA";
	case FILE_MPEG:		return "MPEG";
	case FILE_AMPEG:	return "AMPEG";
	case FILE_VMPEG:	return "VMPEG";
	case FILE_RAWDV:	return "RAWDV";
	case FILE_TIFF_LIST:	return "TIFF_LIST";
	case FILE_PNG_LIST:	return "PNG_LIST";
	case FILE_PPM_LIST:	return "PPM_LIST";
	case FILE_AC3:		return "AC3";
	case FILE_EXR:		return "EXR";
	case FILE_EXR_LIST:	return "EXR_LIST";
	case FILE_CR2:		return "CR2";
	case FILE_FLAC:		return "FLAC";
	case FILE_FFMPEG:	return "FFMPEG";
	case FILE_SCENE:	return "SCENE";
	case FILE_CR2_LIST:	return "CR2_LIST";
	case FILE_GIF_LIST:	return "GIF_LIST";
	case FILE_DB:		return "DB";
	}
	return _("UNKNOWN");
}


PackagingEngine *File::new_packaging_engine(Asset *asset)
{
	PackagingEngine *result = (PackagingEngine*) new PackagingEngineDefault();
	return result;
}


int File::record_fd()
{
	return file ? file->record_fd() : -1;
}


void File::get_exe_path(char *result, char *bnp)
{
// Get executable path, basename
	int len = readlink("/proc/self/exe", result, BCTEXTLEN-1);
	if( len >= 0 ) {
		result[len] = 0;
		char *ptr = strrchr(result, '/');
		if( ptr ) *ptr++ = 0; else ptr = result;
		if( bnp ) strncpy(bnp, ptr, BCTEXTLEN);
	}
	else {
		*result = 0;
		if( bnp ) *bnp = 0;
	}
}

void File::getenv_path(char *result, const char *path)
{
	char *rp = result, *ep = rp + BCTEXTLEN-1;
	const char *cp = path;
// any env var can be used here to expand a path as:
//   "path...$id...": and id is a word as alpha,alnum...
//   expands as "path...getenv(id)..."
// CIN_PATH, CIN_LIB are set in main.C,
	for( int ch=*cp++; ch && rp < ep; ch=*cp++ ) {
		if( ch == '$' && isalpha(*cp) ) { // scan alpha,alnum...
			const char *bp = cp;  char *p = rp;
			while( p < ep && *bp && (isalnum(*bp) || *bp == '_') ) *p++ = *bp++;
			*p = 0;
			const char *envp = getenv(rp); // read env value
			if( !envp ) continue;
			while( *envp && rp < ep ) *rp++ = *envp++;
			cp = bp;		 // subst id=value
			continue;
		}
		*rp++ = ch;
	}
	*rp = 0;
}

void File::setenv_path(const char *var, const char *path, int overwrite)
{
	char env_path[BCTEXTLEN];
	getenv_path(env_path, path);
	setenv(var, env_path, overwrite);
}

void File::init_cin_path()
{
	char env_path[BCTEXTLEN], env_pkg[BCTEXTLEN];
// these values are advertised for forks/shell scripts
	get_exe_path(env_path, env_pkg);
	setenv_path("CIN_PATH", env_path, 1);
	setenv_path("CIN_PKG", env_pkg, 1);
	setenv_path("CIN_DAT", CINDAT_DIR, 0);
	setenv_path("CIN_LIB", CINLIB_DIR, 0);
	setenv_path("CIN_CONFIG", CONFIG_DIR, 0);
	setenv_path("CIN_PLUGIN", PLUGIN_DIR, 0);
	setenv_path("CIN_LADSPA", LADSPA_DIR, 0);
	setenv_path("CIN_LOCALE", LOCALE_DIR, 0);
	setenv_path("CIN_BROWSER", CIN_BROWSER, 0);
}

