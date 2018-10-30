
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
#include "assetedit.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainerror.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "new.h"
#include "resizetrackthread.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"






ResizeVTrackThread::ResizeVTrackThread(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	window = 0;
}

ResizeVTrackThread::~ResizeVTrackThread()
{
	if(window)
	{
		window->lock_window();
		window->set_done(1);
		window->unlock_window();
	}

	Thread::join();
}

void ResizeVTrackThread::start_window(int w, int h, int w1, int h1)
{
	if( window && running() ) {
		window->lock_window();
		window->raise_window();
		window->unlock_window();
		return;
	}
	this->w = w;    this->h = h;
	this->w1 = w1;  this->h1 = h1;
	w_scale = h_scale = 1;
	start();
}


void ResizeVTrackThread::run()
{
	window = new ResizeVTrackWindow(mwindow, this,
			mwindow->gui->get_abs_cursor_x(1),
			mwindow->gui->get_abs_cursor_y(1));
	window->create_objects();
	int result = window->run_window();

	delete window;  window = 0;
	if(!result) {
		update();
	}

	if(((w % 4) || (h % 4)) &&
		mwindow->edl->session->playback_config->vconfig->driver == PLAYBACK_X11_GL)
	{
		MainError::show_error(
			_("This track's dimensions are not multiples of 4 so\n"
			"it can't be rendered by OpenGL."));
	}
}




ResizeVTrackWindow::ResizeVTrackWindow(MWindow *mwindow,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_Window(_(PROGRAM_NAME ": Resize Track"),
				x - 320 / 2, y - get_resources()->ok_images[0]->get_h() + 100 / 2,
				400, get_resources()->ok_images[0]->get_h() + 100,
				400, get_resources()->ok_images[0]->get_h() + 100,
				0, 0, 1)
{
	this->mwindow = mwindow;
	this->thread = thread;
}

ResizeVTrackWindow::~ResizeVTrackWindow()
{
}

void ResizeVTrackWindow::create_objects()
{
	lock_window("ResizeVTrackWindow::create_objects");
	int x = 10, y = 10;
	BC_Title *size_title = new BC_Title(x, y, _("Size:"));
	add_subwindow(size_title);
	int x1 = x + size_title->get_w();
	int y1 = y + size_title->get_h() + 10;
	BC_Title *scale_title = new BC_Title(x, y1, _("Scale:"));
	add_subwindow(scale_title);
	int x2 = x + scale_title->get_w();
	if( x2 > x1 ) x1 = x2;
	x1 += 10;
	add_subwindow(w = new ResizeVTrackWidth(this, thread, x1, y));
	x2 = x1 + w->get_w() + 5;
	BC_Title *xy = new BC_Title(x2, y, _("x"));
	add_subwindow(xy);
	int x3 = x2 + xy->get_w() + 5;
	add_subwindow(h = new ResizeVTrackHeight(this, thread, x3, y));
	x = x3 + h->get_w() + 5;
	FrameSizePulldown *pulldown;
	add_subwindow(pulldown = new FrameSizePulldown(mwindow->theme, w, h, x, y));
	x += pulldown->get_w() + 5;
	add_subwindow(new ResizeVTrackSwap(this, thread, x, y));

	add_subwindow(w_scale = new ResizeVTrackScaleW(this, thread, x1, y1));
	add_subwindow(new BC_Title(x2, y1, _("x")));
	add_subwindow(h_scale = new ResizeVTrackScaleH(this, thread, x3, y1));

	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));

	show_window();
	flush();
	unlock_window();
}

void ResizeVTrackWindow::update(int changed_scale,
	int changed_size,
	int changed_all)
{
//printf("ResizeVTrackWindow::update %d %d %d\n", changed_scale, changed_size, changed_all);
	if(changed_scale || changed_all)
	{
		thread->w = (int)(thread->w1 * thread->w_scale);
		w->update((int64_t)thread->w);
		thread->h = (int)(thread->h1 * thread->h_scale);
		h->update((int64_t)thread->h);
	}
	else
	if(changed_size || changed_all)
	{
		thread->w_scale = thread->w1 ? (double)thread->w / thread->w1 : 1.;
		w_scale->update((float)thread->w_scale);
		thread->h_scale = thread->h1 ? (double)thread->h / thread->h1 : 1.;
		h_scale->update((float)thread->h_scale);
	}
}






ResizeVTrackSwap::ResizeVTrackSwap(ResizeVTrackWindow *gui,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_Button(x, y, thread->mwindow->theme->get_image_set("swap_extents"))
{
	this->thread = thread;
	this->gui = gui;
	set_tooltip(_("Swap dimensions"));
}

int ResizeVTrackSwap::handle_event()
{
	int w = thread->w;
	int h = thread->h;
	thread->w = h;
	thread->h = w;
	gui->w->update((int64_t)h);
	gui->h->update((int64_t)w);
	gui->update(0, 1, 0);
	return 1;
}






ResizeVTrackWidth::ResizeVTrackWidth(ResizeVTrackWindow *gui,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, thread->w)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeVTrackWidth::handle_event()
{
	thread->w = atol(get_text());
	gui->update(0, 1, 0);
	return 1;
}

ResizeVTrackHeight::ResizeVTrackHeight(ResizeVTrackWindow *gui,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, thread->h)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeVTrackHeight::handle_event()
{
	thread->h = atol(get_text());
	gui->update(0, 1, 0);
	return 1;
}


ResizeVTrackScaleW::ResizeVTrackScaleW(ResizeVTrackWindow *gui,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, (float)thread->w_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeVTrackScaleW::handle_event()
{
	thread->w_scale = atof(get_text());
	gui->update(1, 0, 0);
	return 1;
}

ResizeVTrackScaleH::ResizeVTrackScaleH(ResizeVTrackWindow *gui,
	ResizeVTrackThread *thread,
	int x,
	int y)
 : BC_TextBox(x, y, 90, 1, (float)thread->h_scale)
{
	this->gui = gui;
	this->thread = thread;
}
int ResizeVTrackScaleH::handle_event()
{
	thread->h_scale = atof(get_text());
	gui->update(1, 0, 0);
	return 1;
}





ResizeTrackThread::ResizeTrackThread(MWindow *mwindow)
 : ResizeVTrackThread(mwindow)
{
}

ResizeTrackThread::~ResizeTrackThread()
{
}


void ResizeTrackThread::start_window(Track *track)
{
	this->track= track;
	ResizeVTrackThread::start_window(track->track_w,
		track->track_h,
		track->track_w,
		track->track_h);
}

void ResizeTrackThread::update()
{
	if( mwindow->edl->tracks->track_exists(track) )
		mwindow->resize_track(track, w, h);
}




ResizeAssetThread::ResizeAssetThread(AssetEditWindow *fwindow)
 : ResizeVTrackThread(fwindow->mwindow)
{
	this->fwindow = fwindow;
}

ResizeAssetThread::~ResizeAssetThread()
{
}

void ResizeAssetThread::start_window(Asset *asset)
{
	this->asset = asset;
	ResizeVTrackThread::start_window(asset->get_w(),
		asset->get_h(),
		asset->actual_width,
		asset->actual_height);
}

void ResizeAssetThread::update()
{
	char string[BCTEXTLEN];
	asset->width = w;
	asset->height = h;
	sprintf(string, "%d", asset->width);
	fwindow->win_width->update(string);
	sprintf(string, "%d", asset->height);
	fwindow->win_height->update(string);
}

ResizeAssetButton::ResizeAssetButton(AssetEditWindow *fwindow, int x, int y)
 : BC_GenericButton(x, y, _("Resize"))
{
	resize_asset_thread = 0;
        this->fwindow = fwindow;
}

ResizeAssetButton::~ResizeAssetButton()
{
	delete resize_asset_thread;
}

int ResizeAssetButton::handle_event()
{
	if( !resize_asset_thread )
		resize_asset_thread = new ResizeAssetThread(fwindow);
	resize_asset_thread->start_window(fwindow->asset_edit->changed_params);
        return 1;
}


