#ifndef _REMOTECONTROL_H_
#define _REMOTECONTROL_H_

#include "bcpopup.h"
#include "bcwindow.h"
#include "mutex.h"
#include "mwindowgui.inc"
#include "thread.h"

class RemoteControl;
class RemoteGUI;
class RemoteHandler;
class RemoteWindow;

class RemoteGUI : public BC_Popup
{
public:
	RemoteControl *remote_control;

	void set_active(RemoteHandler *handler);
	void set_inactive();
	void fill_color(int color);
	void tile_windows(int config);
	int button_press_event();
	int keypress_event();
	int process_key(int key);

	RemoteGUI(BC_WindowBase *wdw, RemoteControl *remote_control);
	~RemoteGUI();
};

class RemoteWindow : public BC_Window
{
public:
	RemoteControl *remote_control;

	 RemoteWindow(RemoteControl *remote_control);
	 ~RemoteWindow();
};

class RemoteHandler
{
public:
	RemoteGUI *gui;
	int color;

	void activate() { gui->set_active(this); }
	virtual int remote_process_key(RemoteControl *remote_control, int key) { return -1; }

	RemoteHandler(RemoteGUI *gui, int color);
	virtual ~RemoteHandler();
};

class RemoteControl : Thread {
public:
	MWindowGUI *mwindow_gui;
	RemoteWindow *remote_window;
	RemoteGUI *gui;
	RemoteHandler *handler;
	Mutex *active_lock;

	void run();
	void set_color(int color);
	void fill_color(int color);
	int remote_key(int key);

	int activate(RemoteHandler *handler=0);
	int deactivate();
	bool is_active() { return handler != 0; }

	RemoteControl(MWindowGUI *gui);
	~RemoteControl();
};

#endif
