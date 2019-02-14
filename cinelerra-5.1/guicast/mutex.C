
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

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "bcsignals.h"
#include "mutex.h"


Mutex::Mutex(const char *title, int recursive)
{
	this->title = title;
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	pthread_mutex_init(&recursive_lock, &attr);
	count = 0;
	this->recursive = recursive;
	thread_id = 0;
	thread_id_valid = 0;
}

Mutex::~Mutex()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&recursive_lock);
	UNSET_ALL_LOCKS(this);
}

int Mutex::lock(const char *location)
{
// Test recursive owner and give up if we already own it
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		if(thread_id_valid && pthread_self() == thread_id)
		{
			count++;
			pthread_mutex_unlock(&recursive_lock);
			return 0;
		}
		pthread_mutex_unlock(&recursive_lock);
	}


	SET_LOCK(this, title, location);
	if(pthread_mutex_lock(&mutex)) perror("Mutex::lock");
	set_owner();


// Update recursive status for the first lock
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		count = 1;
		thread_id = pthread_self();
		thread_id_valid = 1;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
	{
		count = 1;
	}


	SET_LOCK2
	return 0;
}

int Mutex::unlock()
{
	if( count <= 0 ) {
		printf("Mutex::unlock not locked: %s\n", title);
		booby();
		return 0;
	}
// Remove from recursive status
	if(recursive)
	{
		pthread_mutex_lock(&recursive_lock);
		count--;
// Still locked
		if(count > 0)
		{
			pthread_mutex_unlock(&recursive_lock);
			return 0;
		}
// Not owned anymore
		thread_id = 0;
		thread_id_valid = 0;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
		count = 0;


	UNSET_LOCK(this);
	unset_owner();

	int ret = pthread_mutex_unlock(&mutex);
	if( ret ) fprintf(stderr, "Mutex::unlock: %s\n",strerror(ret));
	return 0;
}

int Mutex::trylock(const char *location)
{
	if( count ) return EBUSY;
	int ret = pthread_mutex_trylock(&mutex);
	if( ret ) return ret;
	set_owner();

// Update recursive status for the first lock
	if(recursive) {
		pthread_mutex_lock(&recursive_lock);
		count = 1;
		thread_id = pthread_self();
		thread_id_valid = 1;
		pthread_mutex_unlock(&recursive_lock);
	}
	else
		count = 1;

	SET_LOCK(this, title, location);
	SET_LOCK2
	return 0;
}

int Mutex::is_locked()
{
	return count;
}

int Mutex::reset()
{
	pthread_mutex_destroy(&mutex);
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutex_init(&mutex, &attr);
	UNSET_ALL_LOCKS(this)
	unset_owner();
	trace = 0;
	count = 0;
	thread_id = 0;
	thread_id_valid = 0;
	return 0;
}
