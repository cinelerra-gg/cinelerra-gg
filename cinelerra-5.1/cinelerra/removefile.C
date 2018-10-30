
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

#include "bcwindowbase.inc"
#include "condition.h"
#include "removefile.h"

#include <string.h>

extern "C"
{
#include <uuid.h>
}


RemoveFile::RemoveFile(const char *path,int autodelete)
 : Thread(1, 0, autodelete) // joinable
{
// Rename to temporary
	set_synchronous(0);
	uuid_t id;
	uuid_generate(id);
	strcpy(file_path, path);
	uuid_unparse(id, file_path + strlen(file_path));
	rename(path, file_path);
}

void RemoveFile::run()
{
	run_lock.lock("RemoveFile::run_lock");
//printf("RemoveFile::run: deleting %s\n", file_path);
	remove(file_path);
	run_lock.unlock();
}

Condition RemoveFile::run_lock(1,"RemoveThread::run_lock");


RemoveAll::RemoveAll(const char *path)
{
	set_show_all();
	update(path);
	for( int i=0; i<dir_list.total; ++i ) {
		FileItem *fi = get_entry(i);
		const char *fp = fi->get_path();
		if( fi->get_is_dir() )
			removeDir(fp);
		else
			RemoveFile::removeFileWait(fp);
	}
//printf("RemoveAll::deleting directory %s\n", path);
	rmdir(path);
}

