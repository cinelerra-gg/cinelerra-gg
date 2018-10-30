
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

#ifndef THREAD_H
#define THREAD_H

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>

static inline int gettid() { return syscall(SYS_gettid, 0, 0, 0); }

// The thread does not autodelete by default.
// If autodelete is 1 the thread autodeletes.
// If it's synchronous the deletion occurs in join().
// If it's asynchronous the deletion occurs in entrypoint.


class Thread
{
	static void* entrypoint(void *parameters);

protected:
	virtual void run() = 0;
public:
	Thread(int synchronous = 0, int realtime = 0, int autodelete = 0);
	virtual ~Thread();
	void start();
	int cancel();            // end this thread
	int join();              // join this thread
	int suspend_thread();    // suspend this thread
	int continue_thread();   // continue this thread
	void exit_thread();      // exit this thread
	int enable_cancel();
	int disable_cancel();
	int get_cancel_enabled();
	bool exists() { return tid != ((pthread_t)-1); }
	bool running() { return exists() && !finished; }

	int get_synchronous();
	int set_synchronous(int value);
	int get_realtime();
	int set_realtime(int value = 1);
	int get_autodelete();
	int set_autodelete(int value);
// Return realtime variable
// Return 1 if querying the kernel returned a realtime policy
	static bool calculate_realtime();
	unsigned long get_tid();
	static pthread_t get_self() { return pthread_self(); }
	static void yield() { sched_yield(); }
	static void dump_threads(FILE *fp=stdout);

private:
	bool synchronous;        // force join() to end
	bool realtime;           // schedule realtime
	bool autodelete;         // autodelete when run() finishes
	bool finished;
	bool cancel_enabled, cancelled;
  	pthread_t owner, tid;
};

#endif
