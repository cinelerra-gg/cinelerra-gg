
/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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

#include "arender.h"
#include "asset.h"
#include "automation.h"
#include "autos.h"
#include "bcsignals.h"
#include "bctimer.h"
#include "cache.h"
#include "clip.h"
#include "condition.h"
#include "edit.h"
#include "edl.h"
#include "edlsession.h"
#include "errorbox.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatauto.h"
#include "floatautos.h"
#include "indexable.h"
#include "indexfile.h"
#include "indexstate.h"
#include "indexthread.h"
#include "language.h"
#include "localsession.h"
#include "mainprogress.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "preferences.h"
#include "removefile.h"
#include "renderengine.h"
#include "resourcepixmap.h"
#include "samples.h"
#include "theme.h"
#include "timelinepane.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <linux/iso_fs.h>

// check for isofs volume_id for dvd/cdrom

static int udf_volume_id(const char *path, char *fname)
{
	struct stat st;
	if( stat(path,&st) ) return 1;
	// search mounted devices
	FILE *fp = fopen("/proc/mounts","r");
	if( !fp ) return 1;

	int result = 1;
	while( result && !feof(fp) && !ferror(fp) ) {
		char devpath[BCTEXTLEN], mpath[BCTEXTLEN];
		char options[BCTEXTLEN], line[BCTEXTLEN];
		char fstype[64], zero1[16], zero2[16];
		if( !fgets(&line[0], sizeof(line)-1, fp) ) break;
		int n = sscanf(&line[0], "%s %s %s %s %s %s\n",
			 devpath, mpath, fstype, options, zero1, zero2);
		if( n != 6 ) continue;
		// check udf filesystems
		if( strcmp(fstype,"udf") != 0 ) continue;
		struct stat dst;
		if( stat(devpath,&dst) ) continue;
		if( st.st_dev != dst.st_rdev ) continue;
		int fd = open(devpath,O_RDONLY);
		if( fd < 0 ) continue;
		struct iso_primary_descriptor id;
		if( lseek(fd,0x8000,SEEK_SET) == 0x8000 )
			n = read(fd,&id,sizeof(id));
		close(fd);
		if( n != sizeof(id) ) continue;
		// look for magic number
		if( strncmp(ISO_STANDARD_ID,id.id,sizeof(id.id)) ) continue;
		// look for volume_id
		if( !isalnum(id.volume_id[0]) ) continue;
		char *bp = (char*)&id.volume_id[0], *cp = fname;
		for( int i=0; i<(int)sizeof(id.volume_id); ++i ) *cp++ = *bp++;
		while( --cp>=fname && *cp==' ' ) *cp = 0;
		if( !*fname ) continue;
		// fname = volume_id _ creation_date
		++cp;  *cp++ = '_';  bp = (char*)&id.creation_date[0];
		for( int i=0; i<(int)sizeof(id.creation_date)-1; ++i ) {
			if( !isdigit(*bp) ) break;
			*cp++ = *bp++;
		}
		*cp++ = 0;
		if( cp-fname > 4 ) result = 0;
	}

	fclose(fp);
	return result;
}

// Use native sampling rates for files so the same index can be used in
// multiple projects.

IndexFile::IndexFile(MWindow *mwindow)
{
//printf("IndexFile::IndexFile 1\n");
	reset();
	this->mwindow = mwindow;
//printf("IndexFile::IndexFile 2\n");
	redraw_timer = new Timer;
}

IndexFile::IndexFile(MWindow *mwindow,
	Indexable *indexable)
{
//printf("IndexFile::IndexFile 2\n");
	reset();
	this->mwindow = mwindow;
	this->indexable = indexable;
	redraw_timer = new Timer;
	if(indexable)
	{
		indexable->add_user();
		source_channels = indexable->get_audio_channels();
		source_samplerate = indexable->get_sample_rate();
		source_length = indexable->get_audio_samples();
	}
}

IndexFile::~IndexFile()
{
//printf("IndexFile::~IndexFile 1\n");
	delete redraw_timer;
	if(indexable) indexable->remove_user();
	close_source();
}

void IndexFile::reset()
{
	fd = 0;
	source = 0;
	interrupt_flag = 0;
	source_length = 0;
	source_channels = 0;
	indexable = 0;
	render_engine = 0;
	cache = 0;
}

IndexState* IndexFile::get_state()
{
	IndexState *index_state = 0;
	if(indexable) index_state = indexable->index_state;
	return index_state;
}



int IndexFile::open_index()
{
	IndexState *index_state = 0;
	int result = 0;

// use buffer if being built
	index_state = get_state();

	if(index_state->index_status == INDEX_BUILDING)
	{
// use buffer
		result = 0;
	}
	else
	if(!(result = open_file()))
	{
// opened existing file
		if(read_info())
		{
			result = 1;
			close_index();
		}
		else
		{
			index_state->index_status = INDEX_READY;
		}
	}
	else
	{
		result = 1;
	}

	return result;
}

void IndexFile::delete_index(Preferences *preferences,
	Indexable *indexable, const char *suffix)
{
	char index_filename[BCTEXTLEN];
	char source_filename[BCTEXTLEN];
	const char *path = indexable->path;

	get_index_filename(source_filename,
		preferences->index_directory,
		index_filename, path, suffix);
//printf("IndexFile::delete_index %s %s\n", source_filename, index_filename);
	remove_file(index_filename);
}

int IndexFile::open_file()
{
	int result = 0;
	const int debug = 0;
	const char *path = indexable->path;


//printf("IndexFile::open_file %f\n", indexable->get_frame_rate());

	get_index_filename(source_filename,
		mwindow->preferences->index_directory,
		index_filename,
		path);

	if(debug) printf("IndexFile::open_file %d index_filename=%s\n",
		__LINE__,
		index_filename);
	fd = fopen(index_filename, "rb");
	if( fd != 0 )
	{
// Index file already exists.
// Get its last size without changing the real asset status.
		Indexable *test_indexable = new Indexable(0);
		if(indexable)
			test_indexable->copy_indexable(indexable);
		read_info(test_indexable);
		IndexState *index_state = test_indexable->index_state;

		FileSystem fs;
		if(fs.get_date(index_filename) < fs.get_date(test_indexable->path))
		{
			if(debug) printf("IndexFile::open_file %d index_date=%jd source_date=%jd\n",
				__LINE__,
				fs.get_date(index_filename),
				fs.get_date(test_indexable->path));

// index older than source
			result = 2;
			fclose(fd);
			fd = 0;
		}
		else
		if(fs.get_size(test_indexable->path) != index_state->index_bytes)
		{
// source file is a different size than index source file
			if(debug) printf("IndexFile::open_file %d index_size=%jd source_size=%jd\n",
				__LINE__,
				index_state->index_bytes,
				fs.get_size(test_indexable->path));
			result = 2;
			fclose(fd);
			fd = 0;
		}
		else
		{
			if(debug) printf("IndexFile::open_file %d\n",
				__LINE__);
			fseek(fd, 0, SEEK_END);
			file_length = ftell(fd);
			fseek(fd, 0, SEEK_SET);
			result = 0;
		}
		test_indexable->Garbage::remove_user();
	}
	else
	{
// doesn't exist
		if(debug) printf("IndexFile::open_file %d index_filename=%s doesn't exist\n",
			__LINE__,
			index_filename);
		result = 1;
	}

	return result;
}

int IndexFile::open_source()
{
//printf("IndexFile::open_source %p %s\n", asset, asset->path);
	int result = 0;
	if(indexable && indexable->is_asset)
	{
		if(!source) source = new File;

		Asset *asset = (Asset*)indexable;
		if(source->open_file(mwindow->preferences,
			asset, 1, 0))
		{
			//printf("IndexFile::open_source() Couldn't open %s.\n", asset->path);
			result = 1;
		}
		else
		{
			FileSystem fs;
			asset->index_state->index_bytes = fs.get_size(asset->path);
			source_length = source->get_audio_length();
		}
	}
	else
	{
		TransportCommand command;
		command.command = NORMAL_FWD;
		command.get_edl()->copy_all((EDL*)indexable);
		command.change_type = CHANGE_ALL;
		command.realtime = 0;
		cache = new CICache(mwindow->preferences);
		render_engine = new RenderEngine(0,
			mwindow->preferences, 0, 0);
		render_engine->set_acache(cache);
		render_engine->arm_command(&command);
		FileSystem fs;
		indexable->index_state->index_bytes = fs.get_size(indexable->path);
	}

	return result;
}

void IndexFile::close_source()
{
	delete source;
	source = 0;

	delete render_engine;
	render_engine = 0;

	delete cache;
	cache = 0;
}

int64_t IndexFile::get_required_scale()
{
	int64_t result = 1;


// get scale of index file
// Total peaks which may be stored in buffer
	int64_t peak_count = mwindow->preferences->index_size /
		(2 * sizeof(float) * source_channels);
	for(result = 1;
		source_length / result > peak_count;
		result *= 2)
		;

// Takes too long to draw from source on a CDROM.  Make indexes for
// everything.

	return result;
}

int IndexFile::get_index_filename(char *source_filename,
	char *index_directory,
	char *index_filename,
	const char *input_filename,
	const char *suffix)
{
	const char *input_fn = input_filename;
	char volume_id[BCTEXTLEN];
// Replace mount/directory with volume_id if isofs
	if( !udf_volume_id(input_filename, volume_id) )
	{
		char *cp = strrchr((char*)input_filename,'/');
		if( cp ) input_fn = cp + 1;
		for( cp=volume_id; *cp; ++cp );
		*cp++ = '_';  strcpy(cp, input_fn);
		input_fn = volume_id;
	}
// Replace slashes and dots
	int i, j;
	int len = strlen(input_fn);
	for(i = 0, j = 0; i < len; i++)
	{
		if(input_fn[i] != '/' &&
			input_fn[i] != '.')
			source_filename[j++] = input_fn[i];
		else
		{
			if(i > 0)
				source_filename[j++] = '_';
		}
	}
	source_filename[j] = 0;
	FileSystem fs;
	fs.join_names(index_filename, index_directory, source_filename);
	strcat(index_filename, suffix ? suffix : ".idx");
	return 0;
}

int IndexFile::interrupt_index()
{
	interrupt_flag = 1;
	return 0;
}

// Read data into buffers

int IndexFile::create_index(MainProgressBar *progress)
{
	int result = 0;
SET_TRACE

	interrupt_flag = 0;

// open the source file
	if(open_source()) return 1;
	source_channels = indexable->get_audio_channels();
	source_samplerate = indexable->get_sample_rate();
	source_length = indexable->get_audio_samples();

SET_TRACE

	get_index_filename(source_filename,
		mwindow->preferences->index_directory,
		index_filename,
		indexable->path);

SET_TRACE

// Some file formats have their own sample index.
// Test for index in stream table of contents
	if(source && !source->get_index(this, progress))
	{
		IndexState *index_state = get_state();
		index_state->index_status = INDEX_READY;
		redraw_edits(1);
	}
	else
// Build index from scratch
	{
SET_TRACE

// Indexes are now built for everything since it takes too long to draw
// from CDROM source.

// get amount to read at a time in floats
		int64_t buffersize = 65536;
		char string[BCTEXTLEN];
		sprintf(string, _("Creating %s."), index_filename);

		progress->update_title(string);
		progress->update_length(source_length);
		redraw_timer->update();
SET_TRACE

// thread out index thread
		IndexThread *index_thread = new IndexThread(mwindow,
			this,
			index_filename,
			buffersize,
			source_length);
		index_thread->start_build();

// current sample in source file
		int64_t position = 0;
		int64_t fragment_size = buffersize;
		int current_buffer = 0;


// pass through file once
// printf("IndexFile::create_index %d source_length=%jd source=%p progress=%p\n",
// __LINE__,
// source_length,
// source,
// progress);
SET_TRACE
		while(position < source_length && !result)
		{
SET_TRACE
			if(source_length - position < fragment_size && fragment_size == buffersize) fragment_size = source_length - position;

			index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 1");
			index_thread->input_len[current_buffer] = fragment_size;

SET_TRACE
			int cancelled = progress->update(position);
//printf("IndexFile::create_index cancelled=%d\n", cancelled);
SET_TRACE
			if(cancelled ||
				index_thread->interrupt_flag ||
				interrupt_flag)
			{
				result = 3;
			}


SET_TRACE
			if(source && !result)
			{
SET_TRACE
				for(int channel = 0;
					!result && channel < source_channels;
					channel++)
				{
// Read from source file
					source->set_audio_position(position);
					source->set_channel(channel);

					if(source->read_samples(
						index_thread->buffer_in[current_buffer][channel],
						fragment_size))
						result = 1;
				}
SET_TRACE
			}
			else
			if(render_engine && !result)
			{
SET_TRACE
				if(render_engine->arender)
				{
					result = render_engine->arender->process_buffer(
						index_thread->buffer_in[current_buffer],
						fragment_size,
						position);
				}
				else
				{
					for(int i = 0; i < source_channels; i++)
					{
						bzero(index_thread->buffer_in[current_buffer][i]->get_data(),
							fragment_size * sizeof(double));
					}
				}
SET_TRACE
			}
SET_TRACE

// Release buffer to thread
			if(!result)
			{
				index_thread->output_lock[current_buffer]->unlock();
				current_buffer++;
				if(current_buffer >= TOTAL_INDEX_BUFFERS) current_buffer = 0;
				position += fragment_size;
			}
			else
			{
				index_thread->input_lock[current_buffer]->unlock();
			}
SET_TRACE
		}


// end thread cleanly
		index_thread->input_lock[current_buffer]->lock("IndexFile::create_index 2");
		index_thread->last_buffer[current_buffer] = 1;
		index_thread->output_lock[current_buffer]->unlock();
		index_thread->stop_build();


		delete index_thread;

	}



	close_source();



	open_index();

	close_index();

	mwindow->edl->set_index_file(indexable);
	return 0;
}



int IndexFile::redraw_edits(int force)
{
	int64_t difference = redraw_timer->get_scaled_difference(1000);

	if(difference > 250 || force)
	{
		redraw_timer->update();
		mwindow->gui->lock_window("IndexFile::redraw_edits");
		mwindow->edl->set_index_file(indexable);
		mwindow->gui->draw_indexes(indexable);
		mwindow->gui->unlock_window();
	}
	return 0;
}




int IndexFile::draw_index(
	TrackCanvas *canvas,
	ResourcePixmap *pixmap,
	Edit *edit,
	int x,
	int w)
{
	const int debug = 0;
	IndexState *index_state = get_state();
	int pane_number = canvas->pane->number;
//index_state->dump();

SET_TRACE
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);
	if(index_state->index_zoom == 0)
	{
		printf(_("IndexFile::draw_index: index has 0 zoom\n"));
		return 0;
	}
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);

// test channel number
	if(edit->channel > source_channels) return 1;
	if(debug) printf("IndexFile::draw_index %d source_samplerate=%d "
			"w=%d samplerate=%jd zoom_sample=%jd\n",
		__LINE__, source_samplerate, w,
		mwindow->edl->session->sample_rate,
		mwindow->edl->local_session->zoom_sample);

// calculate a virtual x where the edit_x should be in floating point
	double virtual_edit_x = 1.0 *
		edit->track->from_units(edit->startproject) *
		mwindow->edl->session->sample_rate /
		mwindow->edl->local_session->zoom_sample -
		mwindow->edl->local_session->view_start[pane_number];

// samples in segment to draw relative to asset
	FloatAutos *speed_autos = !edit->track->has_speed() ? 0 :
		(FloatAutos *)edit->track->automation->autos[AUTOMATION_SPEED];
	double project_zoom = mwindow->edl->local_session->zoom_sample;
	int64_t edit_position = (x + pixmap->pixmap_x - virtual_edit_x) * project_zoom;
	int64_t start_position = edit->startsource;
	start_position += !speed_autos ? edit_position :
		speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);
	int64_t end_position = edit->startsource;
	edit_position = (x + w + pixmap->pixmap_x - virtual_edit_x) * project_zoom;
	end_position += !speed_autos ? edit_position :
		speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);
	double session_sample_rate = mwindow->edl->session->sample_rate;
	double asset_over_session = (double)indexable->get_sample_rate() / session_sample_rate;
	int64_t start_source = start_position * asset_over_session;
	if( start_source < 0 ) start_source = 0;
	int64_t start_index = start_source / index_state->index_zoom;
	int64_t end_source = end_position * asset_over_session;
	if( end_source < 0 ) end_source = 0;
	int64_t end_index = end_source / index_state->index_zoom;
// start/length of index to read in floats
	start_index *= 2;  end_index *= 2;
// length of index available in floats
	int64_t size_index = index_state->index_status == INDEX_BUILDING ?
		index_state->get_channel_used(edit->channel) * 2 :
		index_state->get_index_size(edit->channel);
// Clamp length of index to read by available data
	if( end_index >= size_index ) end_index = size_index;
	int64_t length_index = end_index - start_index;
	if( length_index <= 0 ) return 0;

// Start and length of fragment to read from file in bytes.
	float *buffer = 0;
	int buffer_shared = 0;
	int center_pixel = mwindow->edl->local_session->zoom_track / 2;
	if( edit->track->show_titles() )
		center_pixel += mwindow->theme->get_image("title_bg_data")->get_h();

	if( index_state->index_status == INDEX_BUILDING ) {
// index is in RAM, being built
		buffer = index_state->get_channel_buffer(edit->channel);
		if( !buffer ) return 0;
		buffer += start_index;
		buffer_shared = 1;
	}
	else {
		buffer = new float[length_index + 1];
		int64_t length_buffer = length_index * sizeof(float);
// add file/channel offset
		int64_t index_offset = index_state->get_index_offset(edit->channel);
		int64_t file_offset = (index_offset + start_index) * sizeof(float);
		int64_t file_pos = index_state->index_start + file_offset;
		int64_t read_length = file_length - file_pos;
		if( read_length > length_buffer )
			read_length = length_buffer;
		int64_t length_read = 0;
		if( read_length > 0 ) {
			fseek(fd, file_pos, SEEK_SET);
			length_read = fread(buffer, 1, read_length + sizeof(float), fd);
			length_read &= ~(sizeof(float)-1);
		}
		if( (read_length-=length_read) > 0 )
			memset((char*)buffer + length_read, 0, read_length);
		buffer_shared = 0;
	}

	canvas->set_color(mwindow->theme->audio_color);

	int prev_y1 = center_pixel;
	int prev_y2 = center_pixel;
	int first_frame = 1;
	int zoom_y = mwindow->edl->local_session->zoom_y, zoom_y2 = zoom_y / 2;
	int max_y = center_pixel + zoom_y2 - 1;
	edit_position = (x + pixmap->pixmap_x - virtual_edit_x) * project_zoom;
	int64_t speed_position = edit->startsource;
	speed_position += !speed_autos ? edit_position :
		speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);
	int64_t source_position  = speed_position * asset_over_session;
	int64_t index_position = source_position / index_state->index_zoom;
	int64_t i = 2 * index_position - start_index;
	CLAMP(i, 0, length_index);
SET_TRACE

	for( int64_t x1=0; x1<w && i < length_index; ++x1 ) {
		float highsample = buffer[i];  ++i;
		float lowsample = buffer[i];   ++i;
		int x2 = x1 + x + 1;
		edit_position = (x2 + pixmap->pixmap_x - virtual_edit_x) * project_zoom;
		int64_t speed_position = edit->startsource;
		speed_position += !speed_autos ? edit_position :
			speed_autos->automation_integral(edit->startproject, edit_position, PLAY_FORWARD);
		source_position  = speed_position * asset_over_session;
		index_position = source_position / index_state->index_zoom;
		int64_t k = 2 * index_position - start_index;
		CLAMP(k, 0, length_index);
		while( i < k ) {
			highsample = MAX(highsample, buffer[i]); ++i;
			lowsample = MIN(lowsample, buffer[i]);   ++i;
		}

		int y1 = (int)(center_pixel - highsample * zoom_y2);
		int y2 = (int)(center_pixel - lowsample * zoom_y2);
		CLAMP(y1, 0, max_y);  int next_y1 = y1;
		CLAMP(y2, 0, max_y);  int next_y2 = y2;
//printf("draw_line (%f,%f) = %d,%d,  %d,%d\n", lowsample, highsample, x2, y1, x2, y2);

//SET_TRACE
// A different algorithm has to be used if it's 1 sample per pixel and the
// index is used.  Now the min and max values are equal so we join the max samples.
		if(mwindow->edl->local_session->zoom_sample == 1) {
			canvas->draw_line(x2 - 1, prev_y1, x2, y1, pixmap);
		}
		else {
// Extend line height if it doesn't connect to previous line
			if(!first_frame) {
				if(y1 > prev_y2) y1 = prev_y2 + 1;
				if(y2 < prev_y1) y2 = prev_y1 - 1;
			}
			else {
				first_frame = 0;
			}
			canvas->draw_line(x2, y1, x2, y2, pixmap);
		}
		prev_y1 = next_y1;
		prev_y2 = next_y2;
	}

SET_TRACE

	if(!buffer_shared) delete [] buffer;
SET_TRACE
	if(debug) printf("IndexFile::draw_index %d\n", __LINE__);
	return 0;
}

int IndexFile::close_index()
{
	if(fd)
	{
		fclose(fd);
		fd = 0;
	}
	return 0;
}

int IndexFile::remove_index()
{
	IndexState *index_state = get_state();
	if(index_state->index_status == INDEX_READY ||
		index_state->index_status == INDEX_NOTTESTED)
	{
		close_index();
		remove(index_filename);
	}
	return 0;
}

int IndexFile::read_info(Indexable *test_indexable)
{
	const int debug = 0;

// Store format in actual asset.
// If it's a nested EDL, we never want the format, just the index info.
	if(!test_indexable) test_indexable = indexable;
	if(!test_indexable) return 1;

	IndexState * index_state = test_indexable->index_state;
	if(index_state->index_status == INDEX_NOTTESTED)
	{
// read start of index data
		int temp = fread((char*)&(index_state->index_start), sizeof(int64_t), 1, fd);
//printf("IndexFile::read_info %d %f\n", __LINE__, test_indexable->get_frame_rate());

		if(!temp) return 1;
// read test_indexable info from index
		char *data;

		data = new char[index_state->index_start];
		temp = fread(data, index_state->index_start - sizeof(int64_t), 1, fd);
		if(!temp) return 1;

		data[index_state->index_start - sizeof(int64_t)] = 0;
		FileXML xml;
		xml.read_from_string(data);
		delete [] data;



// Read the file format & index state.
		if(test_indexable->is_asset)
		{
			Asset *test_asset = (Asset *)test_indexable;
			Asset *asset = new Asset;
			asset->read(&xml);
			int ret = 0;
//printf("IndexFile::read_info %d %f\n", __LINE__, asset->get_frame_rate());

			if( asset->format == FILE_UNKNOWN ||
			    test_asset->format != asset->format ) {
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
				ret = 1;
			}
			asset->remove_user();
			if( ret ) return ret;
		}
		else
		{
// Read only the index state for a nested EDL
			int result = 0;
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
			while(!result)
			{
				result = xml.read_tag();
				if(!result)
				{
					if(xml.tag.title_is("INDEX"))
					{
						index_state->read_xml(&xml, source_channels);
if(debug) printf("IndexFile::read_info %d\n", __LINE__);
if(debug) index_state->dump();
						result = 1;
					}
				}
			}
		}
	}

	return 0;
}








