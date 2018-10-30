
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

#include "asset.h"
#include "assets.h"
#include "bctimer.h"
#include "clipedls.h"
#include "edl.h"
#include "filexml.h"
#include "mainindexes.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include <string.h>
#include "undostack.h"

MainUndo::MainUndo(MWindow *mwindow)
{
	this->mwindow = mwindow;
	undo_stack = new UndoStack;
	last_update = new Timer;
}

MainUndo::~MainUndo()
{
	delete undo_stack;
	delete last_update;
}


void MainUndo::update_undo_entry(const char *description,
	uint32_t load_flags,
	void *creator,
	int changes_made)
{
	FileXML file;

	mwindow->edl->save_xml(&file, "");
	file.terminate_string();
	if(changes_made) mwindow->session->changes_made = 1;

// Remove all entries after current and create new one
	UndoStackItem *current = undo_stack->push();

	current->set_flags(load_flags);
	current->set_data(file.string());
	current->set_description((char*)description);
	current->set_creator(creator);
	current->set_filename(mwindow->session->filename);
//printf("MainUndo::update_undo_entry %d %p %s\n", __LINE__, current, current->get_filename());

// Can't undo only 1 record.
	if(undo_stack->total() > 1)
	{
//		mwindow->gui->lock_window("MainUndo::update_undo");
		mwindow->gui->mainmenu->undo->update_caption(description);
		mwindow->gui->mainmenu->redo->update_caption("");
//		mwindow->gui->unlock_window();
	}

// Update timer
	last_update->update();
}

void MainUndo::update_undo_before(const char *description, void *creator)
{
//printf("MainUndo::update_undo_before %d\n", __LINE__);
	if(undo_stack->current && !(undo_stack->number_of(undo_stack->current) % 2))
	{
		printf("MainUndo::update_undo_before %d \"%s\": must be on an after entry to do this. size=%d\n",
			__LINE__,
			description,
			undo_stack->total());

// dump stack
		dump();

// Move up an entry to get back in sync
//		return;
	}

	mwindow->commit_commercial();

// Discard if creator matches previous before entry and within a time limit
	if(creator)
	{
		UndoStackItem *current = undo_stack->current;
// Currently on an after entry
		if(current)
		{
			current = PREVIOUS;
		}

// Now on a before entry
		if(current)
		{
			if(current->get_creator() == creator &&
				!strcmp(current->get_description(), description) &&
				last_update->get_difference() < UNDO_SPAN)
			{
// Before entry has same creator within minimum time.  Reuse it.
// Stack must point to the before entry
				undo_stack->current = current;
				return;
			}
		}
	}

// Append new entry after current position
	update_undo_entry("", 0, creator, 0);
}

void MainUndo::update_undo_after(const char *description,
	uint32_t load_flags,
	int changes_made)
{
//printf("MainUndo::update_undo_after %d\n", __LINE__);
	if(undo_stack->number_of(undo_stack->current) % 2)
	{
		printf("MainUndo::update_undo_after %d \"%s\": must be on a before entry to do this. size=%d\n",
			__LINE__,
			description,
			undo_stack->total());

// dump stack
		dump();
// Not getting any update_undo_before to get back in sync, so just append 1 here
//		return;
	}

	update_undo_entry(description, load_flags, 0, changes_made);

// Update the before entry flags
	UndoStackItem *current = undo_stack->last;
	if(current) current = PREVIOUS;
	if(current)
	{
		current->set_flags(load_flags);
		current->set_description((char*)description);
	}
}


UndoStackItem *MainUndo::next_undo()
{
	return undo_stack->get_current_undo();
}

UndoStackItem *MainUndo::next_redo()
{
	return undo_stack->get_current_redo();
}

int MainUndo::undo_load_flags()
{
	UndoStackItem *item = next_undo();
	return item ? item->get_flags() : 0;
}

int MainUndo::redo_load_flags()
{
	UndoStackItem *item = next_redo();
	return item ? item->get_flags() : 0;
}


int MainUndo::undo()
{
	mwindow->undo_commercial();
	UndoStackItem *current = undo_stack->current;
	if( current ) {
		undo_stack->current = next_undo();
		if( undo_stack->number_of(current) % 2 )
			current = PREVIOUS; // Now have an even number
	}
	if( current ) {
// Set the redo text to the current description
		if( mwindow->gui ) {
			UndoStackItem *next = NEXT;
			mwindow->gui->mainmenu->redo->
				update_caption(next ? next->get_description() : "");
		}
		char *current_data = current->get_data();
		if( current_data ) {
			FileXML file;
			file.read_from_string(current_data);
			delete [] current_data;
			load_from_undo(&file, current->get_flags());
//printf("MainUndo::undo %d %s\n", __LINE__, current->get_filename());
			mwindow->set_filename(current->get_filename());

			if( mwindow->gui ) {
// Now update the menu with the after entry
				UndoStackItem *prev = PREVIOUS;
				mwindow->gui->mainmenu->undo->
					update_caption(prev ? prev->get_description() : "");
			}
		}
	}

	reset_creators();
	mwindow->reset_caches();
	return 0;
}


int MainUndo::redo()
{
	UndoStackItem *current = next_redo();
	if( current ) {
		undo_stack->current = current;
		char *current_data = current->get_data();
		if( current_data ) {
			mwindow->set_filename(current->get_filename());
			FileXML file;
			file.read_from_string(current_data);
			load_from_undo(&file, current->get_flags());
			delete [] current_data;
// Update menu
			mwindow->gui->mainmenu->undo->
				update_caption(current->get_description());
// Get next after entry
			if( (current=NEXT) ) current = NEXT;
			mwindow->gui->mainmenu->redo->
				update_caption(current ? current->get_description() : "");
		}
	}
	reset_creators();
	mwindow->reset_caches();
//dump();
	return 0;
}


// Here the master EDL loads
int MainUndo::load_from_undo(FileXML *file, uint32_t load_flags)
{
	delete mwindow->gui->keyvalue_popup;
	mwindow->gui->keyvalue_popup = 0;

	if( load_flags & LOAD_SESSION ) {
		mwindow->gui->unlock_window();
		mwindow->close_mixers();
		mwindow->gui->lock_window("MainUndo::load_from_undo");
	}
	if( (load_flags & LOAD_ALL) == LOAD_ALL ) {
		mwindow->edl->remove_user();
		mwindow->init_edl();
	}
	mwindow->edl->load_xml(file, load_flags);
	for( Asset *asset=mwindow->edl->assets->first; asset; asset=asset->next ) {
		mwindow->mainindexes->add_next_asset(0, asset);
	}
	for( int i=0; i<mwindow->edl->nested_edls.size(); ++i ) {
		EDL *nested_edl = mwindow->edl->nested_edls[i];
		mwindow->mainindexes->add_next_asset(0, nested_edl);
	}
	mwindow->mainindexes->start_build();
	mwindow->update_plugin_guis(1);
	if( load_flags & LOAD_SESSION ) {
		mwindow->gui->unlock_window();
		mwindow->open_mixers();
		mwindow->gui->lock_window("MainUndo::load_from_undo");
	}
	return 0;
}


void MainUndo::reset_creators()
{
	for( UndoStackItem *current=undo_stack->first; current; current=NEXT ) {
		current->set_creator(0);
	}
}


void MainUndo::dump(FILE *fp)
{
	undo_stack->dump(fp);
}

void MainUndo::save(FILE *fp)
{
	undo_stack->save(fp);
}

void MainUndo::load(FILE *fp)
{
	undo_stack->load(fp);
	UndoStackItem *current = undo_stack->current;
	char *current_data = current ? current->get_data() : 0;
	if( !current_data ) return;
	mwindow->gui->lock_window("MainUndo::load");
	UndoStackItem *next = current->next;
	mwindow->gui->mainmenu->redo->
		update_caption(next ? next->get_description() : "");
	mwindow->set_filename(current->get_filename());
	FileXML file;
	file.read_from_string(current_data);
	load_from_undo(&file, LOAD_ALL);
	delete [] current_data;
	UndoStackItem *prev = current->previous;
	mwindow->gui->mainmenu->undo->
		update_caption(prev ? prev->get_description() : "");
	mwindow->update_project(LOADMODE_REPLACE);
	mwindow->gui->unlock_window();
}

