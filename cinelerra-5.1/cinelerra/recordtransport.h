
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

#ifndef RECORDTRANSPORT_H
#define RECORDTRANSPORT_H

#include "guicast.h"
#include "record.inc"

class RecordGUIRewind;
class RecordGUIRec;
class RecordGUIRecFrame;
class RecordGUIStop;
class RecordGUIPause;

class RecordGUIBegin;
class RecordGUIBack;
class RecordGUIHalt;
class RecordGUIPlay;
class RecordGUIFwd;
class RecordGUIEnd;


class RecordTransport
{
public:
	RecordTransport(MWindow *mwindow, Record *record,
		BC_WindowBase *window, int x, int y);
	~RecordTransport();

	void create_objects();
	void reposition_window(int x, int y);
	int keypress_event();
	int get_h();
	int get_w();
	int max(int a,int b) { return a>b ? a : b; }
	void start_writing_file(int single_frame=0);
	void stop_writing();

 	MWindow *mwindow;
	BC_WindowBase *window;
	Record *record;
	int x, y, x_end, y_end;
	int record_active;
	int play_active;

	RecordGUIRewind *rewind_button;
	RecordGUIRec *record_button;
	RecordGUIRecFrame *record_frame;
	RecordGUIStop *stop_button;
	RecordGUIPause *pause_button;

	RecordGUIBegin *begin_button;
	RecordGUIBack *back_button;
	RecordGUIHalt *halt_button;
	RecordGUIPlay *play_button;
	RecordGUIFwd *fwd_button;
	RecordGUIEnd *end_button;
};


class RecordGUIRewind : public BC_Button
{
public:
	RecordGUIRewind(RecordTransport *record_transport, int x, int y);
	~RecordGUIRewind();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIRec : public BC_Button
{
public:
	RecordGUIRec(RecordTransport *record_transport, int x, int y);
	~RecordGUIRec();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIRecFrame : public BC_Button
{
public:
	RecordGUIRecFrame(RecordTransport *record_transport, int x, int y);
	~RecordGUIRecFrame();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIStop : public BC_Button
{
public:
	RecordGUIStop(RecordTransport *record_transport, int x, int y);
	~RecordGUIStop();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIPause : public BC_Button
{
public:
	RecordGUIPause(RecordTransport *record_transport, int x, int y);
	~RecordGUIPause();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIBegin : public BC_Button
{
public:
	RecordGUIBegin(RecordTransport *record_transport, int x, int y);
	~RecordGUIBegin();

	int handle_event();
	int keypress_event();
	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIBack : public BC_Button
{
public:
	RecordGUIBack(RecordTransport *record_transport, int x, int y);
	~RecordGUIBack();

	int handle_event();
	int button_press();
	int button_release();
	int repeat_event();
	int keypress_event();
	long count;
	long repeat_id;

	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIHalt : public BC_Button
{
public:
	RecordGUIHalt(RecordTransport *record_transport, int x, int y);
	~RecordGUIHalt();

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIPlay : public BC_Button
{
public:
	RecordGUIPlay(RecordTransport *record_transport, int x, int y);
	~RecordGUIPlay();

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIFwd : public BC_Button
{
public:
	RecordGUIFwd(RecordTransport *record_transport, int x, int y);
	~RecordGUIFwd();

	int handle_event();
	int button_press();
	int button_release();
	int repeat_event();
	int keypress_event();

	long count;
	long repeat_id;

	MWindow *mwindow;
	RecordTransport *record_transport;
};


class RecordGUIEnd : public BC_Button
{
public:
	RecordGUIEnd(RecordTransport *record_transport, int x, int y);
	~RecordGUIEnd();

	int handle_event();
	int keypress_event();

	MWindow *mwindow;
	RecordTransport *record_transport;
};

#endif
