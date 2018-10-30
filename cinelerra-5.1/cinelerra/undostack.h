
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

#ifndef UNDOSTACK_H
#define UNDOSTACK_H

#include <stdio.h>
#include <stdint.h>

#include "linklist.h"
#include "stringfile.inc"

#define UNDOLEVELS 500
#define UNDO_KEY_INTERVAL 100


#define hash_sz2 8
#define hash_sz  (1<<hash_sz2)
#define hash_sz1 (hash_sz-1)
#define va 0
#define vb 1

class UndoHash
{
public:
	UndoHash(char *txt, int len, UndoHash *nxt);
	~UndoHash();

	UndoHash *nxt;
	char *txt;  int len;
	int line[2], occurs[2];
};

class UndoHashTable
{
public:
	UndoHashTable();
	~UndoHashTable();
	UndoHash *add(char *txt, int len);

	UndoHash *table[hash_sz];
	UndoHash *bof, *eof;
};

class UndoLine
{
public:
	UndoLine(UndoHash *hash, char *tp);
	UndoLine(UndoHashTable *hash, char *txt, int len);
	~UndoLine();
	int eq(UndoLine *ln);

	char *txt;  int len;
	UndoHash *hash;
};

class UndoVersion : public ArrayList<UndoLine *>
{
public:
	UndoVersion(int v) { ver = v; }
	~UndoVersion() { remove_all_objects(); }
	int ver;

	void scan_lines(UndoHashTable *hash, char *sp, char *ep);
};


class UndoStackItem : public ListItem<UndoStackItem>
{
public:
	UndoStackItem();
	~UndoStackItem();

// Must be inserted into the list before calling this, so it can get the
// previous key buffer.
	void set_data(char *data);
	void set_description(char *description);
	void set_filename(char *filename);
	const char* get_description();
	void set_flags(uint64_t flags);

// Decompress the buffers and return them in a newly allocated string.
// The string must be deleted by the user.
	char* get_data();
	char* get_filename();
	int get_size();
	int is_key();
	uint64_t get_flags();

	void set_creator(void *creator);
	void* get_creator();

	void save(FILE *fp);
	void load(FILE *fp);
private:
// command description for the menu item
	char *description;

// key undo buffer or incremental undo buffer
	int key;

// type of modification
	uint64_t load_flags;

// data after the modification for redos
	char *data;
	int data_size;

// pointer to the object which set this undo buffer
	void *creator;

	char *session_filename;
};

class UndoStack : public List<UndoStackItem>
{
public:
	UndoStack();
	~UndoStack();

// get current undo/redo stack item
	UndoStackItem *get_current_undo();
	UndoStackItem *get_current_redo();

// Create a new undo entry and put on the stack.
// The current pointer points to the new entry.
// delete future undos if in the middle
// delete undos older than UNDOLEVELS if last
	UndoStackItem* push();

// move to the next undo entry for a redo
	UndoStackItem* pull_next();

	void save(FILE *fp);
	void load(FILE *fp);
	void dump(FILE *fp=stdout);

	UndoStackItem* current;
};

#endif
