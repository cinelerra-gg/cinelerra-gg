
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

#ifndef RESIZETRACKTHREAD_H
#define RESIZETRACKTHREAD_H





#include "asset.inc"
#include "assetedit.inc"
#include "guicast.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "thread.h"
#include "track.inc"


class ResizeVTrackWindow;

class ResizeVTrackThread : public Thread
{
public:
	ResizeVTrackThread(MWindow *mwindow);
	~ResizeVTrackThread();

	void start_window(int w, int h, int w1, int h1);
	void run();
	virtual void update() {}

	ResizeVTrackWindow *window;
	MWindow *mwindow;
	int w, h;
	int w1, h1;
	double w_scale, h_scale;
};



class ResizeVTrackWindow;


class ResizeVTrackWidth : public BC_TextBox
{
public:
	ResizeVTrackWidth(ResizeVTrackWindow *gui,
		ResizeVTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeVTrackWindow *gui;
	ResizeVTrackThread *thread;
};

class ResizeVTrackSwap : public BC_Button
{
public:
	ResizeVTrackSwap(ResizeVTrackWindow *gui,
		ResizeVTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeVTrackWindow *gui;
	ResizeVTrackThread *thread;
};

class ResizeVTrackHeight : public BC_TextBox
{
public:
	ResizeVTrackHeight(ResizeVTrackWindow *gui,
		ResizeVTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeVTrackWindow *gui;
	ResizeVTrackThread *thread;
};


class ResizeVTrackScaleW : public BC_TextBox
{
public:
	ResizeVTrackScaleW(ResizeVTrackWindow *gui,
		ResizeVTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeVTrackWindow *gui;
	ResizeVTrackThread *thread;
};

class ResizeVTrackScaleH : public BC_TextBox
{
public:
	ResizeVTrackScaleH(ResizeVTrackWindow *gui,
		ResizeVTrackThread *thread,
		int x,
		int y);
	int handle_event();
	ResizeVTrackWindow *gui;
	ResizeVTrackThread *thread;
};


class ResizeVTrackWindow : public BC_Window
{
public:
	ResizeVTrackWindow(MWindow *mwindow,
		ResizeVTrackThread *thread,
		int x,
		int y);
	~ResizeVTrackWindow();

	void create_objects();
	void update(int changed_scale,
		int changed_size,
		int changed_all);

	MWindow *mwindow;
	ResizeVTrackThread *thread;
	ResizeVTrackWidth *w;
	ResizeVTrackHeight *h;
	ResizeVTrackScaleW *w_scale;
	ResizeVTrackScaleH *h_scale;
};


class ResizeTrackThread : public ResizeVTrackThread
{
public:
	ResizeTrackThread(MWindow *mwindow);
	~ResizeTrackThread();

	void start_window(Track *track);
	void update();

	Track *track;
};

class ResizeAssetThread : public ResizeVTrackThread
{
public:
	ResizeAssetThread(AssetEditWindow *fwindow);
	~ResizeAssetThread();

	void start_window(Asset *asset);
	void update();

	AssetEditWindow *fwindow;
	Asset *asset;
};

class ResizeAssetButton : public BC_GenericButton
{
public:
	ResizeAssetButton(AssetEditWindow *fwindow, int x, int y);
	~ResizeAssetButton();

	int handle_event();

	ResizeAssetThread *resize_asset_thread;
	AssetEditWindow *fwindow;
};

#endif
