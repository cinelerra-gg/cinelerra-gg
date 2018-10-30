
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

#ifndef CONDITION_H
#define CONDITION_H

#include <pthread.h>
#include "bctrace.inc"

class Condition : public trace_info
{
public:
	Condition(int init_value = 0, const char *title = 0, int is_binary = 0);
	~Condition();

// Reset to init_value whether locked or not.
	void reset();
// Block if value <= 0, then decrease value
	void lock(const char *location = 0);
// Increase value
	void unlock();
// Block for requested duration if value <= 0.
// value is decreased whether or not the condition is unlocked in time
// Returns 1 if timeout or 0 if success.
	int timed_lock(int microseconds, const char *location = 0);
	int get_value();

	pthread_cond_t cond;
	pthread_mutex_t mutex;
	int value;
	int init_value;
	int is_binary;
	const char *title;
};

#if 0
#include <stdio.h>

class TCondition : public Condition
{
public:
	TCondition(int init_value = 0, const char *title = 0, int is_binary = 0)
	 : Condition(init_value, title, is_binary) {
		printf("new TCondition(%s) = %p\n", title, this);
	}
	~TCondition() {
		printf("del TCondition(%s) = %p\n", title, this);
	}
	void lock(const char *location = 0) {
		printf("Locking %p: %s\n", this, title);  Condition::lock(location);
		printf("Locked %p: %s\n", this, title);
	}
	void unlock() {
		printf("Unlocking %p: %s\n", this, title);  Condition::unlock();
	}
	int timed_lock(int microseconds, const char *location = 0) {
		printf("timed locking %p: %s\n", this, title);
		int ret = Condition::timed_lock(microseconds, location);
		printf("timed locked %p:%d %s\n", this, ret,  title);  return ret;
	}
	void reset() {
		printf("Reset %p: %s\n", this, title);
		Condition::reset();
	}
	int get_value() {
		int ret = Condition::get_value();
		printf("get_value %p:%d %s\n", this, ret, title);
		return ret;
	}
};

#endif
#endif
