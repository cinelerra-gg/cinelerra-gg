
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "sizes.h"

class FileItem
{
public:
	FileItem();
	FileItem(const char *path, const char *name, int is_dir,
		int64_t size, time_t mtime, int item_no=-1);
	~FileItem();

	int set_path(char *path);
	int set_name(char *name);
	int reset();

	const char *get_path() { return path; }
	const char *get_name() { return name; }
	int get_is_dir() { return is_dir; }

	char *path;
	char *name;
	int is_dir;
	int64_t size;
	time_t mtime;
	int item_no;
};

class FileSystem
{
public:
// sets the working directory to the user
	FileSystem();
	virtual ~FileSystem();

// Load the new directory and change current_dir to it.
// This does not complete the dir path.
// If any of the files failed to stat, it returns nonzero.
	int scan_directory(const char*);
	int update(const char *new_dir = 0);
	int update_sort();

// Complete the path in the string and change to the directory in the string.
// Does not change new_dir
// update - causes the directory to be loaded
	int change_dir(const char *new_dir, int update = 1);
// Set the current_dir to something without completing the path.
	int set_current_dir(const char *new_dir);

	int move_up();
// Syntax of filter is
// single filter without [].
// multiple filters enclosed in [].
	int set_filter(const char *new_filter);
	int set_show_all();     // show hidden files
	int set_want_directory();
	int set_sort_order(int value);
	int set_sort_field(int field);
	int create_dir(const char *new_dir_);    // create a new directory
	int complete_path(char *filename);   // use the filename and the current_dir to create a complete filename
// return 1 if the text is a directory
	int is_dir(const char *new_dir_);
	int extract_dir(char *out, const char *in);    // extract the directory from the path
	int extract_name(char *out, const char *in, int test_dir = 1);	// extract the name from the path
	int join_names(char *out, const char *dir_in, const char *name_in);    // combine a directory and filename
	static int64_t get_date(const char *path);        // get the date of the filename modification
	static void set_date(const char *path, int64_t value); // set the date of the file
	static int64_t get_size(char *filename);        // Get the number of bytes in the file.
	int add_end_slash(char *new_dir);

	int parse_tildas(char *new_dir);     // expand tildas
	int parse_directories(char *new_dir);  // add directories
	int parse_dots(char *new_dir);         // move up directory tree after expanding tildas
	static char *basepath(const char *path); // collapse ".", "..", "//" elements
	static int test_filter(const char *url, const char *filter); // returns 0 if url matches filter

// Array of files and directories in the directory pointed to by current_dir.
// Directories are first.
	ArrayList<FileItem*> dir_list;
	char *get_current_dir() { return current_dir; }
	FileItem* get_entry(int entry) { return dir_list.values[entry]; }
	int total_files() { return dir_list.total; }
	void alphabetize() { sort_table(&dir_list); }

// Sorting order and sorting field.  These are identical in BC_ListBox.
	enum
	{
		SORT_ASCENDING,
		SORT_DESCENDING
	};

// Match column definitions in BC_FileBox.
	enum
	{
		SORT_PATH,
		SORT_SIZE,
		SORT_DATE,
		SORT_EXTENSION
	};

private:
	int sort_table(ArrayList<FileItem*> *dir_list);
	static int path_ascending(const void *ptr1, const void *ptr2);
	static int path_descending(const void *ptr1, const void *ptr2);
	static int size_ascending(const void *ptr1, const void *ptr2);
	static int size_descending(const void *ptr1, const void *ptr2);
	static int date_ascending(const void *ptr1, const void *ptr2);
	static int date_descending(const void *ptr1, const void *ptr2);
	static int ext_ascending(const void *ptr1, const void *ptr2);
	static int ext_descending(const void *ptr1, const void *ptr2);
	static int dot_reverse_filename(char *out, const char *in);

// Combine the directories and files into the master list, directories first.
	int combine(ArrayList<FileItem*> *dir_list, ArrayList<FileItem*> *file_list);
// Whether or not the file passes the current filter.
	int test_filter(FileItem *file);
	int reset_parameters();
	int delete_directory();

	char filter[BCTEXTLEN];     // what filenames have to end in to get displayed
	int want_directory;
	int show_all_files;       // shows . files
	char current_dir[BCTEXTLEN];
	int sort_order;
	int sort_field;
};

#endif
