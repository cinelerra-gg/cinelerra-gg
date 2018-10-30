
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

#include "asset.h"
#include "bcsignals.h"
#include "clip.h"
#include "condition.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "indexfile.h"
#include "indexstate.h"
#include "indexthread.h"
#include "language.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "mainsession.h"
#include "samples.h"
#include <unistd.h>
#include "trackcanvas.h"
#include "tracks.h"

// Read data from buffers and calculate peaks

IndexThread::IndexThread(MWindow *mwindow,
	IndexFile *index_file,
	char *index_filename,
	int64_t buffer_size,
	int64_t length_source)
 : Thread(1, 0, 0)
{
	this->buffer_size = buffer_size;
	this->length_source = length_source;
	this->mwindow = mwindow;
	this->index_filename = index_filename;
	this->index_file = index_file;

// initialization is completed in run
	for(int i = 0; i < TOTAL_INDEX_BUFFERS; i++)
	{
// Must allocate MAX_CHANNELS for a nested EDL
		int min_channels = MAX(MAX_CHANNELS, index_file->source_channels);
		buffer_in[i] = new Samples*[min_channels];
		bzero(buffer_in[i], sizeof(Samples*) * min_channels);

		output_lock[i] = new Condition(0, "IndexThread::output_lock");
		input_lock[i] = new Condition(1, "IndexThread::input_lock");

		for(int j = 0; j < index_file->source_channels; j++)
		{
			buffer_in[i][j] = new Samples(buffer_size);
		}
	}

//printf("IndexThread::IndexThread %d\n", __LINE__);
//index_state->dump();

	interrupt_flag = 0;
}

IndexThread::~IndexThread()
{
	for(int i = 0; i < TOTAL_INDEX_BUFFERS; i++)
	{
		for(int j = 0; j < index_file->source_channels; j++)
		{
			delete buffer_in[i][j];
		}
		delete [] buffer_in[i];
		delete output_lock[i];
		delete input_lock[i];
	}
}

void IndexThread::start_build()
{
	interrupt_flag = 0;
	current_buffer = 0;
	for(int i = 0; i <  TOTAL_INDEX_BUFFERS; i++) last_buffer[i] = 0;
	start();
}

void IndexThread::stop_build()
{
	join();
}

void IndexThread::run()
{
	int done = 0;
	IndexState *index_state = index_file->get_state();
	int index_channels = index_file->source_channels;
	index_state->add_audio_stream(index_channels, index_file->source_length);
	index_state->init_scan(mwindow->preferences->index_size);
	index_state->index_status = INDEX_BUILDING;

	while(!interrupt_flag && !done) {
		output_lock[current_buffer]->lock("IndexThread::run");

		if(last_buffer[current_buffer]) done = 1;
		if(!interrupt_flag && !done) {
// process buffer
			int64_t len = input_len[current_buffer];
			for( int ch=0; ch<index_channels; ++ch ) {
				double *samples = buffer_in[current_buffer][ch]->get_data();
				index_state->put_data(ch,samples,len);
			}

// draw simultaneously with build
			index_file->redraw_edits(0);
		}

		input_lock[current_buffer]->unlock();
		current_buffer++;
		if(current_buffer >= TOTAL_INDEX_BUFFERS) current_buffer = 0;
	}

	index_file->redraw_edits(1);

// write the index file to disk
	Asset *asset = index_file->indexable->is_asset ? (Asset*)index_file->indexable : 0;
	index_state->create_index(index_filename, asset);
}



