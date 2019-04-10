
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

#include "asset.h"
#include "bcsignals.h"
#include "bchash.h"
#include "edl.h"
#include "file.h"
#include "filesystem.h"
#include "indexfile.h"
#include "indexstate.h"
#include "condition.h"
#include "language.h"
#include "loadfile.h"
#include "guicast.h"
#include "mainerror.h"
#include "mainindexes.h"
#include "mainprogress.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"

#include <string.h>


MainIndexes::MainIndexes(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	set_synchronous(1);
	this->mwindow = mwindow;
	input_lock = new Condition(0, "MainIndexes::input_lock");
	next_lock = new Mutex("MainIndexes::next_lock");
	index_lock = new Mutex("MainIndexes::index_lock");
	interrupt_lock = new Condition(1, "MainIndexes::interrupt_lock");
	interrupt_flag = 0;
	indexfile = 0;
	done = 0;
}

MainIndexes::~MainIndexes()
{
	mwindow->mainprogress->cancelled = 1;
	stop_loop();
	delete next_lock;
	delete input_lock;
	delete interrupt_lock;
	delete indexfile;
	delete index_lock;
}

void MainIndexes::add_next_asset(File *file, Indexable *indexable)
{
	next_lock->lock("MainIndexes::add_next_asset");

SET_TRACE
// Test current asset
	IndexFile indexfile(mwindow, indexable);
	IndexState *index_state = indexable->index_state;

SET_TRACE
	int ret = indexfile.open_index();
	if( !ret ) {
		index_state->index_status = INDEX_READY;
		indexfile.close_index();
	}
	else {
// Put source in stack
		index_state->index_status = INDEX_NOTTESTED;
		next_indexables.append(indexable);
		indexable->add_user();
	}
	next_lock->unlock();
}

void MainIndexes::start_loop()
{
	interrupt_flag = 0;
	Thread::start();
}

void MainIndexes::stop_loop()
{
	interrupt_flag = 1;
	done = 1;
	input_lock->unlock();
	interrupt_lock->unlock();
	Thread::join();
}


void MainIndexes::start_build()
{
//printf("MainIndexes::start_build 1\n");
	interrupt_flag = 0;
// Locked up when indexes were already being built and an indexable was
// pasted.
//	interrupt_lock.lock();
	input_lock->unlock();
}

void MainIndexes::interrupt_build()
{
//printf("MainIndexes::interrupt_build 1\n");
	interrupt_flag = 1;
	index_lock->lock("MainIndexes::interrupt_build");
	if(indexfile) indexfile->interrupt_index();
	index_lock->unlock();
//printf("MainIndexes::interrupt_build 2\n");
	interrupt_lock->lock("MainIndexes::interrupt_build");
//printf("MainIndexes::interrupt_build 3\n");
	interrupt_lock->unlock();
//printf("MainIndexes::interrupt_build 4\n");
}

void MainIndexes::load_next_sources()
{
// Transfer from new list
	next_lock->lock("MainIndexes::load_next_sources");
	for(int i = 0; i < next_indexables.size(); i++)
		current_indexables.append(next_indexables.get(i));

// Clear pointers from new list only
	next_indexables.remove_all();
	next_lock->unlock();
}


void MainIndexes::run()
{
	while(!done) {
// Wait for new indexables to be released
		input_lock->lock("MainIndexes::run 1");
		if(done) return;

		interrupt_lock->lock("MainIndexes::run 2");
		load_next_sources();
		interrupt_flag = 0;

// test index of each indexable
		MainProgressBar *progress = 0;

		next_lock->lock("MainIndexes::run");
		for( int i=0; i<current_indexables.size() && !interrupt_flag; ++i ) {
			Indexable *indexable = current_indexables[i];
			IndexState *index_state = indexable->index_state;
// if status is known, no probe
			if( index_state->index_status != INDEX_NOTTESTED ) continue;
			next_lock->unlock();

			IndexFile indexfile(mwindow, indexable);
			int ret = indexfile.open_index();
			if( !ret ) {
				indexfile.close_index();
// use existing index
				index_state->index_status = INDEX_READY;
				if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 2");
				mwindow->edl->set_index_file(indexable);
				if(mwindow->gui) mwindow->gui->unlock_window();
				continue;
			}
// Doesn't exist
			if( !progress ) {
				if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 1");
				progress = mwindow->mainprogress->start_progress(_("Building Indexes..."), 1);
				if(mwindow->gui) mwindow->gui->unlock_window();
			}
// only if audio tracks
			indexfile.create_index(progress);
			if( progress->is_cancelled() )
				interrupt_flag = 1;
			next_lock->lock("MainIndexes::run");
		}

		for( int i=0; i<current_indexables.size(); ++i )
			current_indexables[i]->remove_user();
		current_indexables.remove_all();
		next_lock->unlock();

		if(progress) {	// progress box is only created when an index is built
			if(mwindow->gui) mwindow->gui->lock_window("MainIndexes::run 3");
			progress->stop_progress();
			delete progress;
			if(mwindow->gui) mwindow->gui->unlock_window();
			progress = 0;
		}

		interrupt_lock->unlock();
	}
}

