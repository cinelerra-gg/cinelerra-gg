/*
 * CINELERRA
 * Copyright (C) 1997-2014 Adam Williams <broadcast at earthling dot net>
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


// debugging functions go here

#ifndef BCSIGNALS_H
#define BCSIGNALS_H

#include "arraylist.h"
#include "linklist.h"
#include "bcsignals.inc"
#include "bctrace.h"
#include <stdio.h>
#include <pthread.h>
#include <signal.h>
#include <X11/Xlib.h>

// BC_Signals must be initialized at the start of every program using
// debugging.

class BC_Signals : public BC_Trace
{
	int (*old_err_handler)(Display *, XErrorEvent *);
	static int x_error_handler(Display *display, XErrorEvent *event);
public:
	BC_Signals();
	~BC_Signals();
	void initialize(const char *trap_path=0);
	void initialize2();
	void terminate();

	virtual void signal_handler(int signum);
	static void dump_stack(FILE *fp=stdout);
	static void signal_dump(int signum);
	static void kill_subs();
	static void set_sighup_exit(int enable);

	static void set_trap_path(const char *path);
	static void set_trap_hook(void (*hook)(FILE *fp, void *data), void *data);
	static void set_catch_segv(bool v);
	static void set_catch_intr(bool v);

// Convert signum to text
	static const char* sig_to_str(int number);

	static BC_Signals *global_signals;
	static const char *trap_path;
	static void *trap_data;
	static void (*trap_hook)(FILE *fp, void *vp);
	static bool trap_sigsegv, trap_sigintr;
};

#endif
