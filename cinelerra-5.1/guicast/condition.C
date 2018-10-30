
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

#include "bcsignals.h"
#include "condition.h"

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>

Condition::Condition(int init_value, const char *title, int is_binary)
{
	this->is_binary = is_binary;
	this->title = title;
	this->trace = 0;
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	this->value = this->init_value = init_value;
}

Condition:: ~Condition()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	UNSET_ALL_LOCKS(this);
}

void Condition::reset()
{
	pthread_cond_destroy(&cond);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_init(&mutex, 0);
	pthread_cond_init(&cond, NULL);
	UNSET_ALL_LOCKS(this);
	value = init_value;
	trace = 0;
}

void Condition::lock(const char *location)
{
	SET_LOCK(this, title, location);
	pthread_mutex_lock(&mutex);
	while(value <= 0) pthread_cond_wait(&cond, &mutex);
	UNSET_LOCK2
	if(is_binary)
		value = 0;
	else
		value--;
	pthread_mutex_unlock(&mutex);
}

void Condition::unlock()
{
// The lock trace is created and removed by the acquirer
//	UNSET_LOCK(this);
	pthread_mutex_lock(&mutex);
	if(is_binary)
		value = 1;
	else
		value++;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);
}

int Condition::timed_lock(int microseconds, const char *location)
{
	int result = 0;

	SET_LOCK(this, title, location);
	pthread_mutex_lock(&mutex);

	struct timeval now;
	gettimeofday(&now, 0);
#if 1
	int64_t nsec = now.tv_usec * 1000 + (microseconds % 1000000) * 1000;
	int64_t sec = nsec / 1000000000;  nsec %= 1000000000;
	sec += now.tv_sec + microseconds / 1000000;
	struct timespec timeout;  timeout.tv_sec = sec;   timeout.tv_nsec = nsec;
	while( value <= 0 && result != ETIMEDOUT ) {
		result = pthread_cond_timedwait(&cond, &mutex, &timeout);
	}

	if( result )
		result = result == ETIMEDOUT ? 1 : -1;

#else
	struct timeval timeout;
	int64_t timeout_msec = ((int64_t)microseconds / 1000);
// This is based on the most common frame rate since it's mainly used in
// recording.
	while( value <= 0 && !result ) {
		pthread_mutex_unlock(&mutex);
		usleep(20000);
		gettimeofday(&timeout, 0);
		timeout.tv_usec -= now.tv_usec;
		timeout.tv_sec -= now.tv_sec;
		pthread_mutex_lock(&mutex);
		if( value > 0 ) break;
		if( (int64_t)timeout.tv_sec * 1000 +
		    (int64_t)timeout.tv_usec / 1000 > timeout_msec )
			result = 1;
	}
#endif

	UNSET_LOCK2
//printf("Condition::timed_lock 2 %d %s %s\n", result, title, location);
	if( !result ) {
		if(is_binary)
			value = 0;
		else
			--value;
	}
	pthread_mutex_unlock(&mutex);
	return result;
}


int Condition::get_value()
{
	return value;
}
