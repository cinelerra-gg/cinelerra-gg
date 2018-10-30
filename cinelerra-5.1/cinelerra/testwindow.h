#ifndef __TESTWINDOW_H__
#define __TESTWINDOW_H__

#include "bcwindow.h"
#include "bcsignals.h"
#include "bccolors.h"
#include "fonts.h"
#include "thread.h"

class TestWindowGUI : public BC_Window
{
	int tx, ty;
	int last_x, last_y;
	int in_motion;
public:
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	TestWindowGUI();
	~TestWindowGUI();
};


class TestWindow : public Thread
{
	TestWindowGUI *gui;
public:
	TestWindow() : Thread(1,0,0) { gui = new TestWindowGUI(); start(); }
	~TestWindow() { delete gui; }
	void run() { gui->run_window(); }
};

#endif
