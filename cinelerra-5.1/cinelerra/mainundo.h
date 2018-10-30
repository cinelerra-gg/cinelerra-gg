
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

#ifndef MAINUNDO_H
#define MAINUNDO_H

#include <stdio.h>
#include <stdint.h>

#include "bctimer.inc"
#include "filexml.inc"
#include "mwindow.inc"
#include "undostack.inc"


class MainUndo
{
public:
	MainUndo(MWindow *mwindow);
	~MainUndo();

// For tweeking operations:
// If a pair of update_undo_before and update_undo_after are called
// within a certain time limit and the creator is nonzero and equal,
// the before undo is discarded and the previous after undo is replaced.
	void update_undo_before(const char *description = "",
		void *creator = 0);
	void update_undo_after(const char *description,
		uint32_t load_flags,
		int changes_made = 1);

// Used in undo and redo to reset the creators in all the records.
	void reset_creators();

	int undo();
	int redo();

// load_flags for the next undo/redo stack item
	int undo_load_flags();
	int redo_load_flags();
	void dump(FILE *fp=stdout);

	void save(FILE *fp);
	void load(FILE *fp);
private:
// Entry point for all update commands
	void update_undo_entry(const char *description,
		uint32_t load_flags,
		void *creator,
		int changes_made);
	int load_from_undo(FileXML *file, uint32_t load_flags);

// Placing the before & after undo operations in the same stack makes the
// compression more efficient.
// So even numbers are before and odd numbers are after
	UndoStack *undo_stack;
	UndoStackItem *next_undo();
	UndoStackItem *next_redo();

// loads undo from the stringfile to the project
	int load_undo_before(FileXML *file, uint32_t load_flags);
	int load_undo_after(FileXML *file, uint32_t load_flags);


	MWindow *mwindow;
	Timer *last_update;
};

#endif
