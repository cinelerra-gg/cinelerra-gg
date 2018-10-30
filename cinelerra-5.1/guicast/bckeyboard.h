#ifndef _BC_KEYBOARD_H_
#define _BC_KEYBOARD_H_

#include "bcwindowbase.inc"
#include "bcsignals.inc"
#include "mutex.h"

// global listeners for remote control

class BC_KeyboardHandler {
	int(BC_WindowBase::*handler)(BC_WindowBase *win);
	BC_WindowBase *win;
	static ArrayList<BC_KeyboardHandler*> listeners;
	friend class BC_WindowBase;
	friend class BC_Signals;
public:
	BC_KeyboardHandler(int(BC_WindowBase::*h)(BC_WindowBase *), BC_WindowBase *d)
		: handler(h), win(d) {}
	~BC_KeyboardHandler() {}
	static int run_listeners(BC_WindowBase *wp);
	int run_event(BC_WindowBase *wp);
	static void kill_grabs();
};

class BC_KeyboardHandlerLock {
	static Mutex keyboard_listener_mutex;
public:
	BC_KeyboardHandlerLock() { keyboard_listener_mutex.lock(); }
	~BC_KeyboardHandlerLock() { keyboard_listener_mutex.unlock(); }
};

#endif
