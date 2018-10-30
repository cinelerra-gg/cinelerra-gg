
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

#ifndef REMOVETHREAD_H
#define REMOVETHREAD_H

#include <unistd.h>
#include "arraylist.h"
#include "condition.inc"
#include "filesystem.h"
#include "mutex.inc"
#include "thread.h"

class RemoveFile : public Thread
{
	char file_path[BCTEXTLEN];
	static Condition run_lock;
	void run();
public:
	RemoveFile(const char *path,int autodelete);
	virtual ~RemoveFile() {}
	void wait() { Thread::join(); delete this; }
	static void removeFile(const char *path) {
		RemoveFile *rf = new RemoveFile(path,1);
		rf->start();
	}
	static void removeFileWait(const char *path) {
		if( ::access(path, F_OK) ) return;
		RemoveFile *rf = new RemoveFile(path,0);
		rf->start();  rf->wait();
		delete rf;
	}
};

#define remove_file(path) (void) RemoveFile::removeFile(path)
#define remove_file_wait(path)  RemoveFile::removeFileWait(path)

class RemoveAll : public FileSystem
{
public:
	RemoveAll(const char *path);
	~RemoveAll() {}

	static void removeDir(const char *path) { RemoveAll rf(path); }
};

#define remove_dir(path)  RemoveAll::removeDir(path)

#endif
