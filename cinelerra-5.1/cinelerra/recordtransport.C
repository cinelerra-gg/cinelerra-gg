
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

#include "asset.h"
#include "file.h"
#include "language.h"
#include "mwindow.h"
#include "record.h"
#include "recordgui.h"
#include "recordtransport.h"
#include "theme.h"
#include "units.h"


RecordTransport::RecordTransport(MWindow *mwindow, Record *record,
		BC_WindowBase *window, int x, int y)
{
	this->mwindow = mwindow;
	this->record = record;
	this->window = window;
	this->x = x;
	this->y = y;
	record_active = 0;
	play_active = 0;
}

void RecordTransport::create_objects()
{
	int x = this->x, y = this->y;

	window->add_subwindow(rewind_button = new RecordGUIRewind(this, x, y));
	x += rewind_button->get_w();  y_end = y+rewind_button->get_h();
	window->add_subwindow(record_button = new RecordGUIRec(this, x, y));
	x += record_button->get_w();  y_end = max(y_end,y+record_button->get_h());
	record_frame = 0;
	if(record->default_asset->video_data) {
		window->add_subwindow(
			record_frame = new RecordGUIRecFrame(this, x, y));
		x += record_frame->get_w();  y_end = max(y_end,y+record_button->get_h());
	}
	window->add_subwindow(stop_button = new RecordGUIStop(this, x, y));
	x += stop_button->get_w();  y_end = max(y_end,y+record_button->get_h());
//	window->add_subwindow(pause_button = new RecordGUIPause(this, x, y));
//	x += pause_button->get_w();  y_end = max(y_end,y+record_button->get_h());
	x_end = x;

#if 0
	x = this->x;  y = y_end;
	window->add_subwindow(back_button = new RecordGUIBack(this, x, y));
	x += back_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + back_button->get_h(), y_end);
	window->add_subwindow(play_button = new RecordGUIPlay(this, x, y));
	x += play_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + play_button->get_h(), y_end);
	window->add_subwindow(fwd_button = new RecordGUIFwd(this, x, y));
	x += fwd_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + fwd_button->get_h(), y_end);
	window->add_subwindow(end_button = new RecordGUIEnd(this, x, y));
	x += end_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + end_button->get_h(), y_end);
#endif
        x_end += 10;  y_end += 10;
}

void RecordTransport::reposition_window(int x, int y)
{
	this->x = x;  this->y = y;
	rewind_button->reposition_window(x, y);
	x += rewind_button->get_w();  y_end = y+rewind_button->get_h();
	record_button->reposition_window(x, y);
	x += record_button->get_w();  y_end = max(y_end,y+record_button->get_h());
	if(record->default_asset->video_data) {
		record_frame->reposition_window(x, y);
		x += record_frame->get_w();  y_end = max(y_end,y+record_button->get_h());
	}
	stop_button->reposition_window(x, y);
	x += stop_button->get_w();  y_end = max(y_end,y+record_button->get_h());
//	pause_button->reposition_window(x, y);
//	x += pause_button->get_w();  y_end = max(y_end,y+record_button->get_h());
	x_end = x;

#if 0
	x = this->x;  y = y_end;
	back_button->reposition_window(x, y);
	x += back_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + back_button->get_h(), y_end);
	play_button->reposition_window(x, y);
	x += play_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + play_button->get_h(), y_end);
	fwd_button->reposition_window(x, y);
	x += fwd_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + fwd_button->get_h(), y_end);
	end_button->reposition_window(x, y);
	x += end_button->get_w();  x_end = max(x, x_end);
	y_end = max(y + end_button->get_h(), y_end);
#endif
        x_end += 10;  y_end += 10;
}

int RecordTransport::get_h()
{
	return y_end - y;
}

int RecordTransport::get_w()
{
	return x_end - x;
}


RecordTransport::~RecordTransport()
{
}
void RecordTransport::
start_writing_file(int single_frame)
{
	if( !record->writing_file ) {
		record->pause_input_threads();
		record->update_position();
		record->single_frame = single_frame;
		record->start_writing_file();
		record->resume_input_threads();
	}
}

void RecordTransport::
stop_writing()
{
        record->stop_cron_thread(_("Interrupted"));
	record->stop_writing();
}

int RecordTransport::keypress_event()
{
	if( record->cron_active() > 0 ) return 0;
	if( window->get_keypress() == ' ' ) {
//printf("RecordTransport::keypress_event 1\n");
		if( record->writing_file ) {
			window->unlock_window();
			stop_writing();
			window->lock_window("RecordTransport::keypress_event 1");
		}
		else {
			window->unlock_window();
			start_writing_file();
			window->lock_window("RecordTransport::keypress_event 2");
		}
//printf("RecordTransport::keypress_event 2\n");
		return 1;
	}
	return 0;
}


RecordGUIRec::RecordGUIRec(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("record"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Start recording\nfrom current position"));
}

RecordGUIRec::~RecordGUIRec()
{
}

int RecordGUIRec::handle_event()
{
	if( record_transport->record->cron_active() > 0 ) return 0;
	unlock_window();
	record_transport->start_writing_file();
	lock_window("RecordGUIRec::handle_event");
	return 1;
}

int RecordGUIRec::keypress_event()
{
	return 0;
}

RecordGUIRecFrame::RecordGUIRecFrame(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("recframe"))
{
	this->record_transport = record_transport;
	set_tooltip(_("RecordTransport single frame"));
}

RecordGUIRecFrame::~RecordGUIRecFrame()
{
}

int RecordGUIRecFrame::handle_event()
{
	if( record_transport->record->cron_active() > 0 ) return 0;
	unlock_window();
	record_transport->start_writing_file(1);
	lock_window("RecordGUIRecFrame::handle_event");
	return 1;
}

int RecordGUIRecFrame::keypress_event()
{
	return 0;
}

RecordGUIPlay::RecordGUIPlay(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("play"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Preview recording"));
}

RecordGUIPlay::~RecordGUIPlay()
{
}

int RecordGUIPlay::handle_event()
{
	unlock_window();
	lock_window();
	return 1;
}

int RecordGUIPlay::keypress_event()
{
	return 0;
}


RecordGUIStop::RecordGUIStop(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("stoprec"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Stop operation"));
}

RecordGUIStop::~RecordGUIStop()
{
}

int RecordGUIStop::handle_event()
{
	unlock_window();
	record_transport->stop_writing();
	lock_window("RecordGUIStop::handle_event");
	return 1;
}

int RecordGUIStop::keypress_event()
{
	return 0;
}



RecordGUIPause::RecordGUIPause(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("pause"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Pause"));
}

RecordGUIPause::~RecordGUIPause()
{
}

int RecordGUIPause::handle_event()
{
	return 1;
}

int RecordGUIPause::keypress_event()
{
	return 0;
}



RecordGUIRewind::RecordGUIRewind(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("rewind"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Start over"));
}

RecordGUIRewind::~RecordGUIRewind()
{
}

int RecordGUIRewind::handle_event()
{
	RecordGUI *record_gui = record_transport->record->record_gui;
	if( !record_gui->startover_thread->running() )
		record_gui->startover_thread->start();
	return 1;
}

int RecordGUIRewind::keypress_event()
{
	return 0;
}



RecordGUIBack::RecordGUIBack(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("fastrev"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Fast rewind"));
	repeat_id = 257;
}

RecordGUIBack::~RecordGUIBack()
{
}

int RecordGUIBack::handle_event()
{
	return 1;
}

int RecordGUIBack::button_press()
{
	return 1;
}

int RecordGUIBack::button_release()
{
	unset_repeat(repeat_id);
	return 1;
}

int RecordGUIBack::repeat_event()
{
return 0;
	return 1;
}

int RecordGUIBack::keypress_event()
{
	return 0;
}



RecordGUIFwd::RecordGUIFwd(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("fastfwd"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Fast forward"));
	repeat_id = 255;
}

RecordGUIFwd::~RecordGUIFwd()
{
}

int RecordGUIFwd::handle_event()
{
	return 1;
}

int RecordGUIFwd::button_press()
{
	return 1;
}

int RecordGUIFwd::button_release()
{
	unset_repeat(repeat_id);
	return 1;
}

int RecordGUIFwd::repeat_event()
{
	return 0;
}

int RecordGUIFwd::keypress_event()
{
	return 0;
}



RecordGUIEnd::RecordGUIEnd(RecordTransport *record_transport, int x, int y)
 : BC_Button(x, y, record_transport->mwindow->theme->get_image_set("end"))
{
	this->record_transport = record_transport;
	set_tooltip(_("Seek to end of recording"));
}

RecordGUIEnd::~RecordGUIEnd()
{
}

int RecordGUIEnd::handle_event()
{
	return 1;
}

int RecordGUIEnd::keypress_event()
{
	return 0;
}

