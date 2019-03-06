
#include "bccolors.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "record.h"
#include "remotecontrol.h"


RemoteWindow::RemoteWindow(RemoteControl *remote_control)
 : BC_Window(_(PROGRAM_NAME ": RemoteWindow"),
		0, 0, 16, 16, -1, -1, 1, 0, 1)
{
	this->remote_control = remote_control;
}

RemoteWindow::~RemoteWindow()
{
}


RemoteControl::RemoteControl(MWindowGUI *mwindow_gui)
 : Thread(1, 0, 0)
{
	this->mwindow_gui = mwindow_gui;
	this->handler = 0;
	active_lock = new Mutex("RemoteControl::active_lock");

	remote_window = new RemoteWindow(this);
	Thread::start();
	remote_window->init_wait();
	gui = new RemoteGUI(remote_window, this);
}

void RemoteControl::run()
{
	remote_window->run_window();
}

RemoteControl::~RemoteControl()
{
	if( Thread::running() ) {
		remote_window->close(1);
	}
	Thread::join();
	delete gui;
	delete remote_window;
	delete active_lock;
}



int RemoteControl::activate(RemoteHandler *handler)
{
	int result = 0;
	active_lock->lock("RemoteControl::activate");
	if( !is_active() ) {
		if( !handler ) handler = !mwindow_gui->record->running() ?
			(RemoteHandler *)mwindow_gui->cwindow_remote_handler :
			(RemoteHandler *)mwindow_gui->record_remote_handler ;
		gui->lock_window("RemoteControl::activate");
		gui->set_active(handler);
		gui->set_color(handler->color);
		gui->fill_color(handler->color);
		gui->unlock_window();
		result = 1;
	}
	active_lock->unlock();
	return result;
}

int RemoteControl::deactivate()
{
	int result = 0;
	active_lock->lock("RemoteControl::deactivate");
	if( is_active() ) {
		gui->set_inactive();
		result = 1;
	}
	active_lock->unlock();
	return result;
}

int RemoteControl::remote_key(int key)
{
	if( !is_active() ) return 0;
	return handler->remote_process_key(this, key);
}

void RemoteControl::set_color(int color)
{
	gui->lock_window("RemoteControl::fill_color");
	gui->set_color(color);
	gui->unlock_window();
}

void RemoteControl::fill_color(int color)
{
	gui->lock_window("RemoteControl::fill_color");
	gui->fill_color(color);
	gui->unlock_window();
}

RemoteGUI::RemoteGUI(BC_WindowBase *wdw, RemoteControl *remote_control)
 : BC_Popup(wdw, remote_control->mwindow_gui->mwindow->session->mwindow_x,0,16,16, -1, 1)
{
	this->remote_control = remote_control;
}

RemoteGUI::~RemoteGUI()
{
}

void RemoteGUI::set_active(RemoteHandler *handler)
{
	remote_control->handler = handler;
	show_window();
	grab_keyboard();
}

void RemoteGUI::set_inactive()
{
	remote_control->handler = 0;
	ungrab_keyboard();
	hide_window();
}

void RemoteGUI::
fill_color(int color)
{
	set_color(color);
	draw_box(0, 0, get_w(), get_h());
	flash();
}

void RemoteGUI::tile_windows(int config)
{
	MWindow *mwindow = remote_control->mwindow_gui->mwindow;
	if( config == mwindow->session->window_config ) return;
	lock_window("RemoteGUI::tile_windows");
	ungrab_keyboard();  hide_window();
	mwindow->gui->lock_window("RemoteGUI::tile_windows 1");
	int need_reload = mwindow->tile_windows(config);
	mwindow->gui->unlock_window();
	if( !need_reload ) {
		show_window();  raise_window();  grab_keyboard();
		reposition_window(mwindow->session->mwindow_x,0);
		unlock_window();
	}
	else {
		unlock_window();
		mwindow->restart_status = 1;
		mwindow->quit();
	}
}

int RemoteGUI::button_press_event()
{
	remote_control->deactivate();
	return 1;
}

int RemoteGUI::keypress_event()
{
	int key = get_keypress();
	int result = remote_control->remote_key(key);
	if( result < 0 ) {
		remote_control->deactivate();
		result = 0;
	}
	return result;
}

RemoteHandler::
RemoteHandler(RemoteGUI *gui, int color)
{
	this->gui = gui;
	this->color = color;
}

RemoteHandler::~RemoteHandler()
{
}


