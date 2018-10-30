
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

#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include <stdio.h>
#include "bctrace.inc"

class Mutex : public trace_info
{
public:
	Mutex(const char *title = 0, int recursive = 0);
	~Mutex();

	int lock(const char *location = 0);
	int unlock();
// Calls pthread_mutex_trylock, whose effect depends on library version.
	int trylock(const char *location = 0);
	int reset();
// Returns 1 if count is > 0
	int is_locked();

// Number of times the thread currently holding the mutex has locked it.
// For recursive locking.
	int count;
// ID of the thread currently holding the mutex.  For recursive locking.
	pthread_t thread_id;
	int thread_id_valid;
	int recursive;
// Lock the variables for recursive locking.
	pthread_mutex_t recursive_lock;
	pthread_mutex_t mutex;
	const char *title;
};

#if 0
#include <stdio.h>

class TMutex : public Mutex
{ // logged
public:
	TMutex(const char *title = 0, int recursive = 0)
         : Mutex(title, recursive) {
		printf("new TMutex(%s) = %p\n", title, this);
	}
	~TMutex() {
		printf("del TMutex(%s) = %p\n", title, this);
	}
	int lock(const char *location = 0) {
		printf("locking %p: %s\n", this, title);  int ret = Mutex::lock(location);
		printf("locked %p:%d %s\n", this, ret,  title);  return ret;
	}
	int unlock() {
		printf("unlocking %p: %s\n", this, title);  return Mutex::unlock();
	}
	int trylock(const char *location = 0) {
		printf("try locking %p: %s\n", this, title);  int ret = Mutex::trylock(location);
		printf("try locked %p:%d %s\n", this, ret, title);  return ret;
	}
	int reset() {
		printf("reset %p: %s\n", this, title);
		return Mutex::reset();
	}
	int is_locked() {
		int ret = Mutex::is_locked();
		printf("is_locked %p:%d %s\n", this, ret, title);
		return ret;
	}
};

#endif
#endif
