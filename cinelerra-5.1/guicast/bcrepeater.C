
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

#include "bcdisplay.h"
#include "bcrepeater.h"
#include "bcsignals.h"
#include "bcwindow.h"
#include "condition.h"

#include <unistd.h>

BC_Repeater::BC_Repeater(BC_WindowBase *window, long delay)
 : Thread(1, 0, 0)
{
	pause_lock = new Condition(0, "BC_Repeater::pause_lock");
	startup_lock = new Condition(0, "BC_Repeater::startup_lock");
	repeat_lock = new Condition(1, "BC_Repeater::repeat_lock");
	repeating = 0;
	interrupted = 0;
	this->delay = delay;
	windows.append(window);

}

BC_Repeater::~BC_Repeater()
{
	interrupted = 1;
	pause_lock->unlock();
	repeat_lock->unlock();

	Thread::cancel();
	Thread::join();

	delete pause_lock;
	delete startup_lock;
	delete repeat_lock;
}

void BC_Repeater::initialize()
{
	start();
// This is important.  It doesn't unblock until the thread is running.
	startup_lock->lock("BC_Repeater::initialize");
}

int BC_Repeater::start_repeating()
{
//printf("BC_Repeater::start_repeating 1 %d\n", delay);
	repeating++;
	if(repeating == 1)
	{
// Resume the loop
		pause_lock->unlock();
	}
	return 0;
}

int BC_Repeater::stop_repeating()
{
// Recursive calling happens when mouse wheel is used.
	if(repeating > 0)
	{
		repeating--;
// Pause the loop
		if(repeating == 0) pause_lock->lock("BC_Repeater::stop_repeating");
	}
	return 0;
}

int BC_Repeater::start_repeating(BC_WindowBase *window)
{
// Test if the window already exists
// 	for(int i = 0; i < windows.size(); i++)
// 	{
// 		if(windows.get(i) == window)
// 		{
// 			return 0;
// 		}
// 	}
//printf("BC_Repeater::start_repeating 1 %d\n", delay);

// Add window to users
	repeating++;
	windows.append(window);
	if(repeating == 1)
	{
// Resume the loop
		pause_lock->unlock();
	}
	return 0;
}

int BC_Repeater::stop_repeating(BC_WindowBase *window)
{
	for(int i = 0; i < windows.size(); i++)
	{
		if(windows.get(i) == window)
		{
// Remove all entries of window
			windows.remove_number(i);
			i--;
// Recursive calling happens when mouse wheel is used.
			if(repeating > 0)
			{
				repeating--;
// Pause the loop
				if(repeating == 0)
				{
					pause_lock->lock("BC_Repeater::stop_repeating");
					return 0;
				}
			}
			break;
		}
	}
	return 0;
}

int BC_Repeater::stop_all_repeating(BC_WindowBase *window)
{
	for(int i = 0; i < windows.size(); i++)
	{
		if(windows.get(i) == window)
		{
// Remove all entries of window
			windows.remove_number(i);
			i--;
// Recursive calling happens when mouse wheel is used.
			if(repeating > 0)
			{
				repeating--;
// Pause the loop
				if(repeating == 0)
				{
					pause_lock->lock("BC_Repeater::stop_repeating");
					return 0;
				}
			}
		}
	}
	return 0;
}

void BC_Repeater::run()
{
//printf("BC_Repeater::run 1 %d\n", getpid());
	next_delay = delay;
	Thread::disable_cancel();
	startup_lock->unlock();

	while(!interrupted)
	{
		Thread::enable_cancel();
		timer.delay(next_delay);
		Thread::disable_cancel();
//if(next_delay <= 0) printf("BC_Repeater::run delay=%d next_delay=%d\n", delay, next_delay);

// Test exit conditions
		if(interrupted) return;
// Busy wait here
//		if(repeating <= 0) continue;

// Test for pause
		pause_lock->lock("BC_Repeater::run");
		pause_lock->unlock();
		timer.update();

// Test exit conditions
		if(interrupted) return;
		if(repeating <= 0) continue;

// Wait for existing signal to be processed before sending a new one
		repeat_lock->lock("BC_Repeater::run");

// Test exit conditions
		if(interrupted)
		{
			repeat_lock->unlock();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			continue;
		}

#ifdef SINGLE_THREAD
		BC_Display::display_global->lock_display("BC_Repeater::run");
// Test exit conditions again
		if(interrupted)
		{
			repeat_lock->unlock();
			BC_Display::display_global->unlock_display();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			BC_Display::display_global->unlock_display();
			continue;
		}

// Stick event into queue
		BC_Display::display_global->arm_repeat(delay);
		BC_Display::display_global->unlock_display();
#else
// Wait for window to become available.
		windows.get(0)->lock_window("BC_Repeater::run");

// Test exit conditions again
		if(interrupted)
		{
			repeat_lock->unlock();
			windows.get(0)->unlock_window();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			windows.get(0)->unlock_window();
			continue;
		}

// Stick event into queue
		windows.get(0)->arm_repeat(delay);
		windows.get(0)->unlock_window();
#endif



		next_delay = delay - timer.get_difference();
		if(next_delay <= 0) next_delay = 0;

// Test exit conditions
		if(interrupted)
		{
			repeat_lock->unlock();
			return;
		}
		if(repeating <= 0)
		{
			repeat_lock->unlock();
			continue;
		}
	}
}
