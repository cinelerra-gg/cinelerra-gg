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

#include "bcsignals.h"
#include "cstrdup.h"
#include <dirent.h>
#include <errno.h>
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>


#include "filesystem.h"

FileItem::FileItem()
{
	path = 0;
	name = 0;
	reset();
}

FileItem::FileItem(const char *path, const char *name, int is_dir,
	int64_t size, time_t mtime, int item_no)
{
	this->path = new char[strlen(path)];
	this->name = new char[strlen(name)];
	if(this->path) strcpy(this->path, path);
	if(this->name) strcpy(this->name, name);
	this->is_dir = is_dir;
	this->size = size;
	this->mtime = mtime;
	this->item_no = item_no;
}

FileItem::~FileItem()
{
	reset();
}

int FileItem::reset()
{
	if(this->path) delete [] this->path;
	if(this->name) delete [] this->name;
	path = 0;
	name = 0;
	is_dir = 0;
	size = 0;
	mtime = 0;
	item_no = -1;
	return 0;
}

int FileItem::set_path(char *path)
{
	if(this->path) delete [] this->path;
	this->path = new char[strlen(path) + 1];
	strcpy(this->path, path);
	return 0;
}

int FileItem::set_name(char *name)
{
	if(this->name) delete [] this->name;
	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
	return 0;
}



FileSystem::FileSystem()
{
	reset_parameters();
	(void)getcwd(current_dir, BCTEXTLEN);
}

FileSystem::~FileSystem()
{
	delete_directory();
}

int FileSystem::reset_parameters()
{
 	show_all_files = 0;
	want_directory = 0;
	strcpy(filter, "");
	strcpy(current_dir, "");
	sort_order = SORT_ASCENDING;
	sort_field = SORT_PATH;
	return 0;
}

int FileSystem::delete_directory()
{
	dir_list.remove_all_objects();
	return 0;
}

int FileSystem::set_sort_order(int value)
{
	this->sort_order = value;
	return 0;
}

int FileSystem::set_sort_field(int field)
{
	this->sort_field = field;
	return 0;
}

// filename.with.dots.extension
//   becomes
// extension.dots.with.filename

int FileSystem::dot_reverse_filename(char *out, const char *in)
{
	int i, i2, j=0, lastdot;
	lastdot = strlen(in);
	for ( i=strlen(in); --i >= 0; ) {
		if (in[i] == '.') {
			i2 = i+1;
				while (i2 < lastdot)
					out[j++] = in[i2++];
			out[j++] = in[i];
			lastdot = i;
		}
	}
	if (in[++i] != '.') {
		while (i < lastdot) out[j++] = in[i++];
	}
	out[j++] = '\0';
	return 0;
}

int FileSystem::path_ascending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
//printf("path_ascending %p %p\n", ptr1, ptr2);
	int ret = strcasecmp(item1->name, item2->name);
	if( ret != 0 ) return ret;
	return item1->item_no - item2->item_no;
}

int FileSystem::path_descending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	int ret = strcasecmp(item2->name, item1->name);
	if( ret != 0 ) return ret;
	return item2->item_no - item1->item_no;
}


int FileSystem::size_ascending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	return item1->size == item2->size ?
		item1->item_no - item2->item_no :
		item1->size > item2->size;
}

int FileSystem::size_descending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	return item2->size == item1->size ?
		item2->item_no - item1->item_no :
		item2->size > item1->size;
}


int FileSystem::date_ascending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	return item1->mtime == item2->mtime ?
		item1->item_no - item2->item_no :
		item1->mtime > item2->mtime;
}

int FileSystem::date_descending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	return item2->mtime == item1->mtime ?
		item2->item_no - item1->item_no :
		item2->mtime > item1->mtime;
}

int FileSystem::ext_ascending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	char *ext1 = strrchr(item1->name,'.');
	if( !ext1 ) ext1 = item1->name;
	char *ext2 = strrchr(item2->name,'.');
	if( !ext2 ) ext2 = item2->name;
	int ret = strcasecmp(ext1, ext2);
	if( ret ) return ret;
	if( item1->item_no >= 0 && item2->item_no >= 0 )
		return item1->item_no - item2->item_no;
	char dotreversedname1[BCTEXTLEN], dotreversedname2[BCTEXTLEN];
	dot_reverse_filename(dotreversedname1,item1->name);
	dot_reverse_filename(dotreversedname2,item2->name);
	return strcasecmp(dotreversedname1, dotreversedname2);
}

int FileSystem::ext_descending(const void *ptr1, const void *ptr2)
{
	FileItem *item1 = *(FileItem**)ptr1;
	FileItem *item2 = *(FileItem**)ptr2;
	char *ext1 = strrchr(item1->name,'.');
	if( !ext1 ) ext1 = item1->name;
	char *ext2 = strrchr(item2->name,'.');
	if( !ext2 ) ext2 = item2->name;
	int ret = strcasecmp(ext2, ext1);
	if( ret ) return ret;
	if( item2->item_no >= 0 && item1->item_no >= 0 )
		return item2->item_no - item1->item_no;
	char dotreversedname1[BCTEXTLEN], dotreversedname2[BCTEXTLEN];
	dot_reverse_filename(dotreversedname1,item1->name);
	dot_reverse_filename(dotreversedname2,item2->name);
	return strcasecmp(dotreversedname2, dotreversedname1);
}

int FileSystem::sort_table(ArrayList<FileItem*> *dir_list)
{
	if(!dir_list || !dir_list->size()) return 0;
	static int (*cmpr[][2])(const void *ptr1, const void *ptr2) = {
		{ &path_ascending, &path_descending },
		{ &size_ascending, &size_descending },
		{ &date_ascending, &date_descending },
		{ &ext_ascending,  &ext_descending  },
	};

	qsort(dir_list->values,
		dir_list->size(), sizeof(FileItem*),
		cmpr[sort_field][sort_order]);

	return 0;
}

int FileSystem::combine(ArrayList<FileItem*> *dir_list, ArrayList<FileItem*> *file_list)
{
	int i;

	sort_table(dir_list);
	for(i = 0; i < dir_list->total; i++)
	{
		this->dir_list.append(dir_list->values[i]);
	}

	sort_table(file_list);
	for(i = 0; i < file_list->total; i++)
	{
		this->dir_list.append(file_list->values[i]);
	}
	return 0;
}

int FileSystem::test_filter(const char *url, const char *filter)
{
	char string[BCTEXTLEN], string2[BCTEXTLEN];
	const char *filter1 = 0, *filter2 = filter;
	int done = 0, result = 0, token_number = 0;

	do { // Get next token
		filter1 = strchr(filter2, '[');
		string[0] = 0;
		if( filter1 ) { // Get next filter
			filter2 = strchr(++filter1, ']');
			if( filter2 ) {
				int n = filter2 - filter1;
				for( int i=0; i<n; ++i ) string[i] = filter1[i];
				string[n] = 0;
			}
			else {
				strcpy(string, filter1);
				done = 1;
			}
		}
		else {
			if( !token_number )
				strcpy(string, filter);
			else
				done = 1;
		}

		if( string[0] ) { // Process the token
			const char *path = url;
			const char *subfilter1 = string, *subfilter2 = 0;
			int token_done = 0;
			result = 0;
			do {
				string2[0] = 0;
				subfilter2 = strchr(subfilter1, '*');
				if( subfilter2 ) {
					int n = subfilter2 - subfilter1;
					for( int i=0; i<n; ++i ) string2[i] = subfilter1[i];
					string2[n] = 0;
				}
				else {
					strcpy(string2, subfilter1);
					token_done = 1;
				}

				if( string2[0] ) { // Subfilter exists
					if( subfilter1 > string ) {
						const char *cp = strstr(path, string2);
						if( !cp )
							result = token_done = 1;
						else
							path = cp + strlen(string2);
					}
					else { // Subfilter exists
						if( strncmp(path, string2, strlen(string2)) ) // strncasecmp?
							result = token_done = 1;
						else
							path += strlen(string2);
					}

// String must terminate after subfilter
					if( !subfilter2 ) {
						if( *path )
							result = token_done = 1;
					}
				}
				subfilter1 = subfilter2 + 1;
// Let pass if no subfilter
			} while( !token_done && !result );
		}
		++token_number;
	} while( !done && result );

	return result;
}

int FileSystem::test_filter(FileItem *file)
{
// Don't filter directories
	if( file->is_dir ) return 0;
	if( !file->name ) return 1;
	return test_filter(file->name, this->filter);
}

int FileSystem::scan_directory(const char *new_dir)
{
	if( new_dir != 0 )
		strcpy(current_dir, new_dir);
	DIR *dirstream = opendir(current_dir);
	if( !dirstream ) return 1;          // failed to open directory

	struct dirent64 *new_filename;
	while( (new_filename = readdir64(dirstream)) != 0 ) {
		FileItem *new_file = 0;
		int include_this = 1;

// File is directory heirarchy
		if(!strcmp(new_filename->d_name, ".") ||
			!strcmp(new_filename->d_name, "..")) include_this = 0;

// File is hidden and we don't want all files
		if(include_this &&
			!show_all_files &&
			new_filename->d_name[0] == '.') include_this = 0;

// file not hidden
  		if(include_this)
		{
			new_file = new FileItem;
			char full_path[BCTEXTLEN], name_only[BCTEXTLEN];
			sprintf(full_path, "%s", current_dir);
			add_end_slash(full_path);
			strcat(full_path, new_filename->d_name);
			strcpy(name_only, new_filename->d_name);
			new_file->set_path(full_path);
			new_file->set_name(name_only);

// Get information about the file.
			struct stat ostat;
			if(!stat(full_path, &ostat))
			{
				new_file->size = ostat.st_size;
				new_file->mtime = ostat.st_mtime;

				if(S_ISDIR(ostat.st_mode))
				{
					strcat(name_only, "/"); // is a directory
					new_file->is_dir = 1;
				}

// File is excluded from filter
				if(include_this && test_filter(new_file)) include_this = 0;
//printf("FileSystem::update 3 %d %d\n", include_this, test_filter(new_file));

// File is not a directory and we just want directories
				if(include_this && want_directory && !new_file->is_dir) include_this = 0;
			}
			else
			{
				printf("FileSystem::update %s %s\n", full_path, strerror(errno));
				include_this = 0;
			}

// add to list
			if(include_this)
				dir_list.append(new_file);
			else
				delete new_file;
		}
	}
	closedir(dirstream);
	return 0;
}

int FileSystem::update(const char *new_dir)
{
	delete_directory();
	int result = scan_directory(new_dir);
// combine the directories and files in the master list
	return !result ? update_sort() : result;
}

int FileSystem::update_sort()
{
	ArrayList<FileItem*> directories, files;
	for( int i=0; i< dir_list.size(); ++i ) {
		FileItem *item = dir_list[i];
		item->item_no = i;
		(item->is_dir ? &directories : &files)->append(item);
	}
	dir_list.remove_all();
// combine the directories and files in the master list
	combine(&directories, &files);
	return 0;
// success
}


int FileSystem::set_filter(const char *new_filter)
{
	strcpy(filter, new_filter);
	return 0;
}

int FileSystem::set_show_all()
{
	show_all_files = 1;
	return 0;
}

int FileSystem::set_want_directory()
{
	want_directory = 1;
	return 0;
}

int FileSystem::is_dir(const char *path)      // return 0 if the text is a directory
{
	if(!strlen(path)) return 0;

	char new_dir[BCTEXTLEN];
	struct stat ostat;    // entire name is a directory

	strcpy(new_dir, path);
	complete_path(new_dir);
	if(!stat(new_dir, &ostat) && S_ISDIR(ostat.st_mode))
		return 1;
	else
		return 0;
}

int FileSystem::create_dir(const char *new_dir_)
{
	char new_dir[BCTEXTLEN];
	strcpy(new_dir, new_dir_);
	complete_path(new_dir);
	return mkdir(new_dir, S_IRWXU | S_IRWXG | S_IRWXO);
}

int FileSystem::parse_tildas(char *new_dir)
{
	if(new_dir[0] == 0) return 1;

// Our home directory
	if(new_dir[0] == '~')
	{

		if(new_dir[1] == '/' || new_dir[1] == 0)
		{
// user's home directory
			char *home;
			char string[BCTEXTLEN];
			home = getenv("HOME");

// print starting after tilda
			if(home) sprintf(string, "%s%s", home, &new_dir[1]);
			strcpy(new_dir, string);
			return 0;
		}
		else
// Another user's home directory
		{
			char string[BCTEXTLEN], new_user[BCTEXTLEN];
			struct passwd *pw;
			int i, j;

			for(i = 1, j = 0; new_dir[i] != 0 && new_dir[i] != '/'; i++, j++)
			{                // copy user name
				new_user[j] = new_dir[i];
			}
			new_user[j] = 0;

			setpwent();
			while( (pw = getpwent()) != 0 )
			{
// get info for user
				if(!strcmp(pw->pw_name, new_user))
				{
// print starting after tilda
      				sprintf(string, "%s%s", pw->pw_dir, &new_dir[i]);
      				strcpy(new_dir, string);
      				break;
      			}
			}
			endpwent();
			return 0;
		}
	}
	return 0;
}

int FileSystem::parse_directories(char *new_dir)
{
//printf("FileSystem::parse_directories 1 %s\n", new_dir);
	if( *new_dir != '/' && current_dir[0] ) {  // expand to abs path
		char string[BCTEXTLEN];
		strcpy(string, current_dir);
		add_end_slash(string);
		strcat(string, new_dir);
		strcpy(new_dir, string);
	}
	return 0;
}

int FileSystem::parse_dots(char *new_dir)
{
// recursively remove ..s
	int changed = 1;
	while(changed)
	{
		int i, j, len;
		len = strlen(new_dir);
		changed = 0;
		for(i = 0, j = 1; !changed && j < len; i++, j++)
		{
// Got 2 dots
			if(new_dir[i] == '.' && new_dir[j] == '.')
			{
// Ignore if character after .. doesn't qualify
				if(j + 1 < len &&
					new_dir[j + 1] != ' ' &&
					new_dir[j + 1] != '/')
					continue;

// Ignore if character before .. doesn't qualify
				if(i > 0 &&
					new_dir[i - 1] != '/') continue;

				changed = 1;
				while(new_dir[i] != '/' && i > 0)
				{
// look for first / before ..
					i--;
				}

// find / before this /
				if(i > 0) i--;
				while(new_dir[i] != '/' && i > 0)
				{
// look for first / before first / before ..
					i--;
				}

// i now equals /first filename before ..
// look for first / after ..
				while(new_dir[j] != '/' && j < len)
				{
					j++;
				}

// j now equals /first filename after ..
				while(j < len)
				{
					new_dir[i++] = new_dir[j++];
				}

				new_dir[i] = 0;
// default to root directory
				if((new_dir[0]) == 0) sprintf(new_dir, "/");
				break;
			}
		}
	}
	return 0;
}

int FileSystem::complete_path(char *filename)
{
//printf("FileSystem::complete_path 1\n");
	if(!strlen(filename)) return 1;
//printf("FileSystem::complete_path 1\n");
	parse_tildas(filename);
//printf("FileSystem::complete_path 1\n");
	parse_directories(filename);
//printf("FileSystem::complete_path 1\n");
	parse_dots(filename);
// don't add end slash since this requires checking if dir
//printf("FileSystem::complete_path 2\n");
	return 0;
}

int FileSystem::extract_dir(char *out, const char *in)
{
	strcpy(out, in);
	if(!is_dir(in))
	{
// complete string is not directory
		int i;

		complete_path(out);

		for(i = strlen(out); i > 0 && out[i - 1] != '/'; i--)
		{
			;
		}
		if(i >= 0) out[i] = 0;
	}
	return 0;
}

int FileSystem::extract_name(char *out, const char *in, int test_dir)
{
	int i;

	if(test_dir && is_dir(in))
		out[0] = 0; // complete string is directory
	else
	{
		for(i = strlen(in)-1; i > 0 && in[i] != '/'; i--)
		{
			;
		}
		if(in[i] == '/') i++;
		strcpy(out, &in[i]);
	}
	return 0;
}

int FileSystem::join_names(char *out, const char *dir_in, const char *name_in)
{
	strcpy(out, dir_in);
	int len = strlen(out);
	int result = 0;

	while(!result)
		if(len == 0 || out[len] != 0) result = 1; else len--;

	if(len != 0)
	{
		if(out[len] != '/') strcat(out, "/");
	}

	strcat(out, name_in);
	return 0;
}

int64_t FileSystem::get_date(const char *filename)
{
	struct stat file_status;
	bzero(&file_status, sizeof(struct stat));
	int result = stat(filename, &file_status);
	return !result ? file_status.st_mtime : -1;
}

void FileSystem::set_date(const char *path, int64_t value)
{
	struct utimbuf new_time;
	new_time.actime = value;
	new_time.modtime = value;
	utime(path, &new_time);
}

int64_t FileSystem::get_size(char *filename)
{
	struct stat file_status;
	bzero(&file_status, sizeof(struct stat));
	int result = stat(filename, &file_status);
	return !result ? file_status.st_size : -1;
}

int FileSystem::change_dir(const char *new_dir, int update)
{
	char new_dir_full[BCTEXTLEN];

	strcpy(new_dir_full, new_dir);

	complete_path(new_dir_full);
// cut ending slash
	if(strcmp(new_dir_full, "/") &&
		new_dir_full[strlen(new_dir_full) - 1] == '/')
		new_dir_full[strlen(new_dir_full) - 1] = 0;

	if(update)
		this->update(new_dir_full);
	else
	{
		strcpy(current_dir, new_dir_full);
	}
	return 0;
}

int FileSystem::set_current_dir(const char *new_dir)
{
	strcpy(current_dir, new_dir);
	return 0;
}

int FileSystem::add_end_slash(char *new_dir)
{
	if(new_dir[strlen(new_dir) - 1] != '/') strcat(new_dir, "/");
	return 0;
}


// collapse ".",  "..", "//"  eg. x/./..//y = y
char *FileSystem::basepath(const char *path)
{
	char fpath[BCTEXTLEN];
	unsigned len = strlen(path);
	if( len >= sizeof(fpath) ) return 0;
	strcpy(fpath, path);
	char *flat = cstrdup("");

	int r = 0;
	char *fn = fpath + len;
	while( fn > fpath ) {
		while( --fn >= fpath )
			if( *fn == '/' ) { *fn = 0;  break; }
		fn = fn < fpath ? fpath : fn+1;
		if( !*fn || !strcmp(fn, ".") ) continue;
		if( !strcmp(fn, "..") ) { ++r; continue; }
		if( r < 0 ) continue;
		if( r > 0 ) { --r;  continue; }
		char *cp = cstrcat(3, "/",fn,flat);
		delete [] flat;  flat = cp;
	}

	if( *path != '/' ) {
		char *cp = cstrcat(2, ".",flat);
		delete [] flat;  flat = cp;
	}

	return flat;
}

