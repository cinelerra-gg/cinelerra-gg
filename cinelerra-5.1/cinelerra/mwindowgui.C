
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

#include "androidcontrol.h"
#include "awindowgui.h"
#include "awindow.h"
#include "bcdisplayinfo.h"
#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "cwindowgui.h"
#include "cwindow.h"
#include "channelinfo.h"
#include "dbwindow.h"
#include "edit.h"
#include "editpopup.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "keyframepopup.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "maincursor.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mbuttons.h"
#include "mtimebar.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "panedividers.h"
#include "patchbay.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginset.h"
#include "preferences.h"
#include "record.h"
#include "recordgui.h"
#include "renderengine.h"
#include "resourcethread.h"
#include "samplescroll.h"
#include "shbtnprefs.h"
#include "statusbar.h"
#include "swindow.h"
#include "theme.h"
#include "trackcanvas.h"
#include "trackpopup.h"
#include "trackscroll.h"
#include "tracks.h"
#include "transitionpopup.h"
#include "vwindowgui.h"
#include "vwindow.h"
#include "zoombar.h"

#define PANE_DRAG_MARGIN MAX(mwindow->theme->pane_w, mwindow->theme->pane_h)


// the main window uses its own private colormap for video
MWindowGUI::MWindowGUI(MWindow *mwindow)
 : BC_Window(_(PROGRAM_NAME ": Program"),
 		mwindow->session->mwindow_x,
		mwindow->session->mwindow_y,
		mwindow->session->mwindow_w,
		mwindow->session->mwindow_h,
		100,
		100,
		1,
		1,
		1)
{
	this->mwindow = mwindow;
//	samplescroll = 0;
//	trackscroll = 0;
//	cursor = 0;
//	patchbay = 0;
//	timebar = 0;
//	canvas = 0;
	focused_pane = TOP_LEFT_PANE;
	x_divider = 0;
	y_divider = 0;
	x_pane_drag = 0;
	y_pane_drag = 0;
	dragging_pane = 0;
	drag_popup = 0;

	render_engine = 0;
	for(int i = 0; i < TOTAL_PANES; i++)
		pane[i] = 0;

	record = 0;
	channel_info = 0;
	swindow = 0;
	db_window = 0;
// subwindows
	mbuttons = 0;
	statusbar = 0;
	zoombar = 0;
	mainclock = 0;
	track_menu = 0;
	edit_menu = 0;
	plugin_menu = 0;
	keyframe_menu = 0;
	keyframe_hide = 0;
	keyvalue_popup = 0;
 	transition_menu = 0;
	remote_control = 0;
	cwindow_remote_handler = 0;
	record_remote_handler = 0;
	android_control = 0;
}


MWindowGUI::~MWindowGUI()
{
	delete android_control;
	delete cwindow_remote_handler;
	delete record_remote_handler;
	delete remote_control;
	delete keyvalue_popup;
//	delete samplescroll;
//	delete trackscroll;
	for(int i = 0; i < TOTAL_PANES; i++)
		if(pane[i]) delete pane[i];
//	delete cursor;
	delete render_engine;
	delete resource_thread;
	resource_pixmaps.remove_all_objects();
	delete swindow;
#ifdef HAVE_DVB
	delete channel_info;
#endif
	delete db_window;
	delete x_divider;
	delete y_divider;
}

#if 0
void MWindowGUI::get_scrollbars(int flush)
{
	//int64_t h_needed = mwindow->edl->get_tracks_height(mwindow->theme);
	//int64_t w_needed = mwindow->edl->get_tracks_width();
	int need_xscroll = 0;
	int need_yscroll = 0;
	view_w = mwindow->theme->mcanvas_w;
	view_h = mwindow->theme->mcanvas_h;

// Scrollbars are constitutive
	need_xscroll = need_yscroll = 1;
	view_h = mwindow->theme->mcanvas_h;
	view_w = mwindow->theme->mcanvas_w;

// 	for(int i = 0; i < 2; i++)
// 	{
// 		if(w_needed > view_w)
// 		{
// 			need_xscroll = 1;
// 			view_h = mwindow->theme->mcanvas_h - SCROLL_SPAN;
// 		}
// 		else
// 			need_xscroll = 0;
//
// 		if(h_needed > view_h)
// 		{
// 			need_yscroll = 1;
// 			view_w = mwindow->theme->mcanvas_w - SCROLL_SPAN;
// 		}
// 		else
// 			need_yscroll = 0;
// 	}
//printf("MWindowGUI::get_scrollbars 1\n");

	if(canvas && (view_w != canvas->get_w() || view_h != canvas->get_h()))
	{
		canvas->reposition_window(mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			view_w,
			view_h);
	}

	if(need_xscroll)
	{
		if(!samplescroll)
			add_subwindow(samplescroll = new SampleScroll(mwindow,
				this,
				mwindow->theme->mhscroll_x,
				mwindow->theme->mhscroll_y,
				mwindow->theme->mhscroll_w));
		else
			samplescroll->resize_event();

		samplescroll->set_position(0);
	}
	else
	{
		if(samplescroll) delete samplescroll;
		samplescroll = 0;
		mwindow->edl->local_session->view_start = 0;
	}


	if(need_yscroll)
	{
//printf("MWindowGUI::get_scrollbars 1.1 %p %p\n", this, canvas);
		if(!trackscroll)
			add_subwindow(trackscroll = new TrackScroll(mwindow,
				this,
				mwindow->theme->mvscroll_x,
				mwindow->theme->mvscroll_y,
				mwindow->theme->mvscroll_h));
		else
			trackscroll->resize_event();


//printf("MWindowGUI::get_scrollbars 1.2\n");
		trackscroll->update_length(mwindow->edl->get_tracks_height(mwindow->theme),
			mwindow->edl->local_session->track_start,
			view_h,
			0);
//printf("MWindowGUI::get_scrollbars 1.3\n");
	}
	else
	{
		if(trackscroll) delete trackscroll;
		trackscroll = 0;
		mwindow->edl->local_session->track_start = 0;
	}

	if(flush) this->flush();

}
#endif // 0

void MWindowGUI::create_objects()
{
	lock_window("MWindowGUI::create_objects");
	const int debug = 0;

	resource_thread = new ResourceThread(mwindow, this);
	resource_thread->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	set_icon(mwindow->theme->get_image("mwindow_icon"));
	remote_control = new RemoteControl(this);
	cwindow_remote_handler = new CWindowRemoteHandler(remote_control);
	record_remote_handler = new RecordRemoteHandler(remote_control);
	mwindow->reset_android_remote();

	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);

	int x = get_w() - MainShBtns::calculate_w(-1, 0, -1);
	add_subwindow(mainmenu = new MainMenu(mwindow, this, x));
	mainmenu->create_objects();
	add_subwindow(mainshbtns = new MainShBtns(mwindow, x, -1));
	mainshbtns->load(mwindow->preferences);
	mwindow->theme->get_mwindow_sizes(this, get_w(), get_h());
	mwindow->theme->draw_mwindow_bg(this);
	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);

	add_subwindow(mbuttons = new MButtons(mwindow, this));
	mbuttons->create_objects();
	int x1 = mbuttons->get_x() + mbuttons->get_w(), y1 = mbuttons->get_y()+2;
	add_subwindow(proxy_toggle = new ProxyToggle(mwindow, mbuttons, x1, y1));
	x1 += proxy_toggle->get_w() + 3;
	add_subwindow(ffmpeg_toggle = new FFMpegToggle(mwindow, mbuttons, x1, y1));

	pane[TOP_LEFT_PANE] = new TimelinePane(mwindow,
		TOP_LEFT_PANE,
		mwindow->theme->mcanvas_x,
		mwindow->theme->mcanvas_y,
		mwindow->theme->mcanvas_w,
		mwindow->theme->mcanvas_h);
	pane[TOP_LEFT_PANE]->create_objects();

// 	add_subwindow(timebar = new MTimeBar(mwindow,
// 		this,
// 		mwindow->theme->mtimebar_x,
//  		mwindow->theme->mtimebar_y,
// 		mwindow->theme->mtimebar_w,
// 		mwindow->theme->mtimebar_h));
// 	timebar->create_objects();

//	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
//	add_subwindow(patchbay = new PatchBay(mwindow, this));
//	patchbay->create_objects();

//	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
//	get_scrollbars(0);

//	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
//	mwindow->gui->add_subwindow(canvas = new TrackCanvas(mwindow, this));
//	canvas->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(zoombar = new ZoomBar(mwindow, this));
	zoombar->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(statusbar = new StatusBar(mwindow, this));
	statusbar->create_objects();



	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(mainclock = new MainClock(mwindow,
		mwindow->theme->mclock_x, mwindow->theme->mclock_y,
		mwindow->theme->mclock_w));
	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	mainclock->update(0);



//	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
//	cursor = new MainCursor(mwindow, this);
//	cursor->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(track_menu = new TrackPopup(mwindow, this));
	track_menu->create_objects();
	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(edit_menu = new EditPopup(mwindow, this));
	edit_menu->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(plugin_menu = new PluginPopup(mwindow, this));
	plugin_menu->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(keyframe_menu = new KeyframePopup(mwindow, this));
	keyframe_menu->create_objects();
	add_subwindow(keyframe_hide = new KeyframeHidePopup(mwindow, this));
	keyframe_hide->create_objects();


	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	add_subwindow(transition_menu = new TransitionPopup(mwindow, this));
	transition_menu->create_objects();

#ifdef HAVE_DVB
	channel_info = new ChannelInfo(mwindow);
#endif
#ifdef HAVE_COMMERCIAL
	db_window = new DbWindow(mwindow);
#endif
	swindow = new SWindow(mwindow);

	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);

	add_subwindow(pane_button = new PaneButton(mwindow,
		get_w() - mwindow->theme->get_image_set("pane")[0]->get_w(),
		mwindow->theme->mzoom_y + 1 - mwindow->theme->get_image_set("pane")[0]->get_h()));

	pane[TOP_LEFT_PANE]->canvas->activate();

	if(debug) printf("MWindowGUI::create_objects %d\n", __LINE__);
	unlock_window();
}

void MWindowGUI::redraw_time_dependancies()
{
	zoombar->redraw_time_dependancies();
	for(int i = 0; i < TOTAL_PANES; i++)
		if(pane[i] && pane[i]->timebar) pane[i]->timebar->update(0);
	mainclock->update(mwindow->edl->local_session->get_selectionstart(1));
}

int MWindowGUI::focus_in_event()
{
	for(int i = 0; i < TOTAL_PANES; i++)
		if(pane[i]) pane[i]->cursor->focus_in_event();
	return 1;
}

int MWindowGUI::focus_out_event()
{
	for(int i = 0; i < TOTAL_PANES; i++)
		if(pane[i]) pane[i]->cursor->focus_out_event();
	return 1;
}


int MWindowGUI::resize_event(int w, int h)
{
//printf("MWindowGUI::resize_event %d\n", __LINE__);
	mwindow->session->mwindow_w = w;
	mwindow->session->mwindow_h = h;
	int x = w - MainShBtns::calculate_w(-1, 0, -1);
	mainmenu->resize_event(x, mainmenu->get_h());
	mainshbtns->reposition_window(x, -1);
	mwindow->theme->get_mwindow_sizes(this, w, h);
	mwindow->theme->draw_mwindow_bg(this);
	mbuttons->resize_event();
	int x1 = mbuttons->get_x() + mbuttons->get_w(), y1 = mbuttons->get_y()+2;
	proxy_toggle->reposition_window(x1, y1);
	x1 += proxy_toggle->get_w() + 3;
	ffmpeg_toggle->reposition_window(x1, y1);
	statusbar->resize_event();
	zoombar->resize_event();

	resource_thread->stop_draw(1);

	if(total_panes() > 1)
	{
		if(horizontal_panes())
		{
// 			printf("MWindowGUI::resize_event %d %d %d\n",
// 				__LINE__,
// 				pane[TOP_RIGHT_PANE]->x,
// 				mwindow->theme->mcanvas_w -
// 					BC_ScrollBar::get_span(SCROLL_VERT) -
// 					PANE_DRAG_MARGIN);
			if(pane[TOP_RIGHT_PANE]->x >= mwindow->theme->mcanvas_w -
				BC_ScrollBar::get_span(SCROLL_VERT) -
				PANE_DRAG_MARGIN)
			{
				delete_x_pane(pane[TOP_RIGHT_PANE]->x);
				mwindow->edl->local_session->x_pane = -1;
			}
		}
		else
		if(vertical_panes())
		{
			if(pane[BOTTOM_LEFT_PANE]->y >= mwindow->theme->mzoom_y -
				BC_ScrollBar::get_span(SCROLL_HORIZ) -
				PANE_DRAG_MARGIN)
			{
				delete_y_pane(pane[BOTTOM_LEFT_PANE]->y);
				mwindow->edl->local_session->y_pane = -1;
			}
		}
		else
		{
			if(pane[TOP_RIGHT_PANE]->x >= mwindow->theme->mcanvas_w -
					BC_ScrollBar::get_span(SCROLL_VERT) -
					PANE_DRAG_MARGIN)
			{
				delete_x_pane(pane[TOP_RIGHT_PANE]->x);
				mwindow->edl->local_session->x_pane = -1;
			}

			if(pane[BOTTOM_LEFT_PANE]->y >= mwindow->theme->mzoom_y -
				BC_ScrollBar::get_span(SCROLL_HORIZ) -
				PANE_DRAG_MARGIN)
			{
				delete_y_pane(pane[BOTTOM_LEFT_PANE]->y);
				mwindow->edl->local_session->y_pane = -1;
			}
		}
	}

	if(total_panes() == 1)
	{
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_w,
			mwindow->theme->mcanvas_h);
	}
	else
	if(horizontal_panes())
	{
		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			pane[TOP_LEFT_PANE]->w,
			mwindow->theme->mcanvas_h);
		pane[TOP_RIGHT_PANE]->resize_event(
			pane[TOP_RIGHT_PANE]->x,
			pane[TOP_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_w - pane[TOP_RIGHT_PANE]->x,
			mwindow->theme->mcanvas_h);
	}
	else
	if(vertical_panes())
	{
		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			mwindow->theme->mcanvas_w,
			pane[TOP_LEFT_PANE]->h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			pane[BOTTOM_LEFT_PANE]->x,
			pane[BOTTOM_LEFT_PANE]->y,
			mwindow->theme->mcanvas_w,
			mwindow->theme->mcanvas_y +
				mwindow->theme->mcanvas_h -
				pane[BOTTOM_LEFT_PANE]->y);
	}
	else
	{
		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			pane[TOP_LEFT_PANE]->w,
			pane[TOP_LEFT_PANE]->h);
		pane[TOP_RIGHT_PANE]->resize_event(
			pane[TOP_RIGHT_PANE]->x,
			pane[TOP_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_w - pane[TOP_RIGHT_PANE]->x,
			pane[TOP_RIGHT_PANE]->h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			pane[BOTTOM_LEFT_PANE]->x,
			pane[BOTTOM_LEFT_PANE]->y,
			pane[BOTTOM_LEFT_PANE]->w,
			mwindow->theme->mcanvas_y +
				mwindow->theme->mcanvas_h -
				pane[BOTTOM_LEFT_PANE]->y);
		pane[BOTTOM_RIGHT_PANE]->resize_event(
			pane[BOTTOM_RIGHT_PANE]->x,
			pane[BOTTOM_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_w -
				pane[BOTTOM_RIGHT_PANE]->x,
			mwindow->theme->mcanvas_y +
				mwindow->theme->mcanvas_h -
				pane[BOTTOM_RIGHT_PANE]->y);
	}

	update_pane_dividers();
	pane_button->reposition_window(w - mwindow->theme->get_image_set("pane")[0]->get_w(),
		mwindow->theme->mzoom_y + 1 - mwindow->theme->get_image_set("pane")[0]->get_h());
	resource_thread->start_draw();

	flash(1);
	return 0;
}

int MWindowGUI::total_panes()
{
	int total = 0;
	for(int i = 0; i < TOTAL_PANES; i++)
		if(pane[i]) total++;
	return total;
}

int MWindowGUI::vertical_panes()
{
	return total_panes() == 2 &&
		pane[TOP_LEFT_PANE] &&
		pane[BOTTOM_LEFT_PANE];
}

int MWindowGUI::horizontal_panes()
{
	return total_panes() == 2 &&
		pane[TOP_LEFT_PANE] &&
		pane[TOP_RIGHT_PANE];
}

TimelinePane* MWindowGUI::get_focused_pane()
{
	if(pane[focused_pane]) return pane[focused_pane];
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i]) return pane[i];
	}
	return 0;
}

void MWindowGUI::activate_timeline()
{
	if(pane[focused_pane])
	{
		pane[focused_pane]->activate();
	}
	else
	{
		for(int i = 0; i < TOTAL_PANES; i++)
		{
			if(pane[i])
			{
				pane[i]->activate();
				return;
			}
		}
	}
}

void MWindowGUI::deactivate_timeline()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->deactivate();
		}
	}
}

void MWindowGUI::update_title(char *path)
{
	FileSystem fs;
	char filename[BCTEXTLEN], string[BCTEXTLEN];
	fs.extract_name(filename, path);
	sprintf(string, _(PROGRAM_NAME ": %s"), filename);
	set_title(string);
//printf("MWindowGUI::update_title %s\n", string);
	flush();
}

void MWindowGUI::draw_overlays(int flash_it)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->draw_overlays();
			if(flash_it) pane[i]->canvas->flash();
		}
	}
}

void MWindowGUI::update_timebar(int flush_it)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->timebar)
		{
			pane[i]->timebar->update(flush_it);
		}
	}
}

void MWindowGUI::update_timebar_highlights()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->timebar)
		{
			pane[i]->timebar->update_highlights();
		}
	}
}


void MWindowGUI::update_patchbay()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->patchbay)
		{
			pane[i]->patchbay->update();
		}
	}
}

void MWindowGUI::update_proxy_toggle()
{
	int value = mwindow->edl->session->proxy_scale == 1 ? 1 : 0;
	proxy_toggle->set_value(value);
	if( mwindow->edl->session->proxy_scale == 1 &&
	    mwindow->edl->session->proxy_disabled_scale == 1 )
		proxy_toggle->hide();
	else
		proxy_toggle->show();
}

void MWindowGUI::update_plugintoggles()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->refresh_plugintoggles();
		}
	}

}

void MWindowGUI::draw_indexes(Indexable *indexable)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->draw_indexes(indexable);
		}
	}
}

void MWindowGUI::draw_canvas(int redraw, int hide_cursor)
{
	resource_thread->stop_draw(0);

	int mode = redraw ? FORCE_REDRAW : NORMAL_DRAW;
	for(int i = 0; i < TOTAL_PANES; i++) {
		if( pane[i] )
			pane[i]->canvas->draw(mode, hide_cursor);
	}

	resource_thread->start_draw();
}

void MWindowGUI::flash_canvas(int flush)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->flash(flush);
		}
	}
}

int MWindowGUI::show_window(int flush)
{
	int ret = BC_WindowBase::show_window(flush);
	update_proxy_toggle();
	return ret;
}

void MWindowGUI::draw_cursor(int do_plugintoggles)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->cursor->draw(do_plugintoggles);
		}
	}
}

void MWindowGUI::show_cursor(int do_plugintoggles)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->cursor->show(do_plugintoggles);
		}
	}
}

void MWindowGUI::hide_cursor(int do_plugintoggles)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->cursor->hide(do_plugintoggles);
		}
	}
}

void MWindowGUI::update_cursor()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->cursor->update();
		}
	}
}

void MWindowGUI::set_playing_back(int value)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->cursor->playing_back = value;
		}
	}
}

void MWindowGUI::update_scrollbars(int flush)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->update(1, NO_DRAW, 0, 0);
		}
	}
	if(flush) this->flush();
}

void MWindowGUI::reset_meters()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->patchbay)
		{
			pane[i]->patchbay->reset_meters();
		}
	}
}

void MWindowGUI::stop_meters()
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->patchbay)
		{
			pane[i]->patchbay->stop_meters();
		}
	}
}

void MWindowGUI::update_meters(ArrayList<double> *module_levels)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->patchbay)
		{
			pane[i]->patchbay->update_meters(module_levels);
		}
	}
}

void MWindowGUI::set_editing_mode(int flush)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i])
		{
			pane[i]->canvas->update_cursor(flush);
		}
	}
}

void MWindowGUI::set_meter_format(int mode, int min, int max)
{
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i] && pane[i]->patchbay)
		{
			pane[i]->patchbay->set_meter_format(mode, min, max);
		}
	}
}

void MWindowGUI::update(int scrollbars,
	int do_canvas,
	int timebar,
	int zoombar,
	int patchbay,
	int clock,
	int buttonbar)
{
	const int debug = 0;
	if(debug) PRINT_TRACE



	mwindow->edl->tracks->update_y_pixels(mwindow->theme);

	if( do_canvas != NO_DRAW && do_canvas != IGNORE_THREAD )
		resource_thread->stop_draw(1);

	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i]) pane[i]->update(scrollbars,
			do_canvas,
			timebar,
			patchbay);
	}

	if( do_canvas != NO_DRAW && do_canvas != IGNORE_THREAD )
		resource_thread->start_draw();

//	if(scrollbars) this->get_scrollbars(0);
//	if(timebar) this->timebar->update(0);
	if(zoombar) this->zoombar->update();
//	if(patchbay) this->patchbay->update();
	if(clock) this->mainclock->update(
		mwindow->edl->local_session->get_selectionstart(1));
	if(debug) PRINT_TRACE



//	if(do_canvas)
//	{
//		this->canvas->draw(do_canvas);
//		this->cursor->show();
//		this->canvas->flash(0);
// Activate causes the menubar to deactivate.  Don't want this for
// picon thread.
//		if(canvas != IGNORE_THREAD) this->canvas->activate();
//	}
	if(debug) PRINT_TRACE



	if(buttonbar) mbuttons->update();
	if(debug) PRINT_TRACE

// Can't age if the cache called this to draw missing picons
// or the GUI is updating the status of the draw toggle.
	if( do_canvas != FORCE_REDRAW && do_canvas != IGNORE_THREAD ) {
		unlock_window();
		mwindow->age_caches();
		lock_window("MWindowGUI::update");
	}

	flush();
	if(debug) PRINT_TRACE
}

int MWindowGUI::visible(int64_t x1, int64_t x2, int64_t view_x1, int64_t view_x2)
{
	return (x1 >= view_x1 && x1 < view_x2) ||
		(x2 > view_x1 && x2 <= view_x2) ||
		(x1 <= view_x1 && x2 >= view_x2);
}


void MWindowGUI::show_message(const char *message, int msg_color, int bar_color)
{
	statusbar->show_message(message, msg_color, bar_color);
}

void MWindowGUI::update_default_message()
{
	statusbar->update_default_message();
}

void MWindowGUI::reset_default_message()
{
	statusbar->reset_default_message();
}

void MWindowGUI::default_message()
{
	statusbar->default_message();
}

// Drag motion called from other window
int MWindowGUI::drag_motion()
{
	if(get_hidden()) return 0;

	Track *over_track = 0;
	Edit *over_edit = 0;
	PluginSet *over_pluginset = 0;
	Plugin *over_plugin = 0;
	int redraw = 0;

	if(drag_popup)
	{
		drag_popup->cursor_motion_event();
	}


// there's no point in drawing highlights has until drag operation has been set
	if (!mwindow->session->current_operation)
		return 0;

	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i]) pane[i]->canvas->drag_motion(
			&over_track,
			&over_edit,
			&over_pluginset,
			&over_plugin);
	}

	if(mwindow->session->track_highlighted != over_track)
	{
		mwindow->session->track_highlighted = over_track;
		redraw = 1;
	}

	if(mwindow->session->edit_highlighted != over_edit)
	{
		mwindow->session->edit_highlighted = over_edit;
		redraw = 1;
	}

	if(mwindow->session->pluginset_highlighted != over_pluginset)
	{
		mwindow->session->pluginset_highlighted = over_pluginset;
		redraw = 1;
	}

	if(mwindow->session->plugin_highlighted != over_plugin)
	{
		mwindow->session->plugin_highlighted = over_plugin;
		redraw = 1;
	}

	if( mwindow->session->current_operation == DRAG_ASSET ||
	    mwindow->session->current_operation == DRAG_EDIT ||
	    mwindow->session->current_operation == DRAG_GROUP ||
	    mwindow->session->current_operation == DRAG_AEFFECT_COPY ||
	    mwindow->session->current_operation == DRAG_VEFFECT_COPY )
        {
                redraw = 1;
        }


// printf("drag_motion %d %d over_track=%p over_edit=%p\n",
// __LINE__,
// redraw,
// over_track,
// over_edit);
	if(redraw)
	{
		lock_window("MWindowGUI::drag_motion");
		draw_overlays(1);
		unlock_window();
	}
	return 0;
}

int MWindowGUI::drag_stop()
{
	if(get_hidden()) return 0;
	int result = 0, redraw = 0;

	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i]) result |= pane[i]->canvas->drag_stop(
			&redraw);
	}
	mwindow->edl->optimize();

// since we don't have subwindows we have to terminate any drag operation
	if(result)
	{
		if (mwindow->session->track_highlighted
			|| mwindow->session->edit_highlighted
			|| mwindow->session->plugin_highlighted
			|| mwindow->session->pluginset_highlighted)
			redraw = 1;
		mwindow->session->track_highlighted = 0;
		mwindow->session->edit_highlighted = 0;
		mwindow->session->plugin_highlighted = 0;
		mwindow->session->pluginset_highlighted = 0;
		mwindow->session->current_operation = NO_OPERATION;
	}


//printf("MWindowGUI::drag_stop %d %d\n", redraw, mwindow->session->current_operation);
	if(redraw)
	{
		mwindow->edl->tracks->update_y_pixels(mwindow->theme);
		update_scrollbars(0);
		update_patchbay();
		draw_canvas(1, 1);
		update_cursor();
		flash_canvas(1);
	}

	if(drag_popup)
	{
		delete drag_popup;
		drag_popup = 0;
	}
	return result;
}

void MWindowGUI::default_positions()
{
//printf("MWindowGUI::default_positions 1\n");
	VWindow *vwindow = mwindow->vwindows.size() > DEFAULT_VWINDOW ?
		mwindow->vwindows.get(DEFAULT_VWINDOW) : 0;
	if( vwindow && !vwindow->is_running() ) vwindow = 0;
	if( vwindow ) vwindow->gui->lock_window("MWindowGUI::default_positions");
	mwindow->cwindow->gui->lock_window("MWindowGUI::default_positions");
	mwindow->awindow->gui->lock_window("MWindowGUI::default_positions");

// printf("MWindowGUI::default_positions 1 %d %d %d %d\n", mwindow->session->vwindow_x,
// mwindow->session->vwindow_y,
// mwindow->session->vwindow_w,
// mwindow->session->vwindow_h);
	reposition_window(mwindow->session->mwindow_x,
		mwindow->session->mwindow_y,
		mwindow->session->mwindow_w,
		mwindow->session->mwindow_h);
	if( vwindow ) vwindow->gui->reposition_window(mwindow->session->vwindow_x,
		mwindow->session->vwindow_y,
		mwindow->session->vwindow_w,
		mwindow->session->vwindow_h);
	mwindow->cwindow->gui->reposition_window(mwindow->session->cwindow_x,
		mwindow->session->cwindow_y,
		mwindow->session->cwindow_w,
		mwindow->session->cwindow_h);
	mwindow->awindow->gui->reposition_window(mwindow->session->awindow_x,
		mwindow->session->awindow_y,
		mwindow->session->awindow_w,
		mwindow->session->awindow_h);
//printf("MWindowGUI::default_positions 1\n");

	resize_event(mwindow->session->mwindow_w,
		mwindow->session->mwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	if( vwindow ) vwindow->gui->resize_event(mwindow->session->vwindow_w,
		mwindow->session->vwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	mwindow->cwindow->gui->resize_event(mwindow->session->cwindow_w,
		mwindow->session->cwindow_h);
//printf("MWindowGUI::default_positions 1\n");
	mwindow->awindow->gui->resize_event(mwindow->session->awindow_w,
		mwindow->session->awindow_h);

//printf("MWindowGUI::default_positions 1\n");

	flush();
	if( vwindow ) vwindow->gui->flush();
	mwindow->cwindow->gui->flush();
	mwindow->awindow->gui->flush();

	if( vwindow ) vwindow->gui->unlock_window();
	mwindow->cwindow->gui->unlock_window();
	mwindow->awindow->gui->unlock_window();
//printf("MWindowGUI::default_positions 2\n");
}


int MWindowGUI::repeat_event(int64_t duration)
{
// if(duration == 100)
// mwindow->sync_parameters(CHANGE_ALL);
	int result = 0;
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		if(pane[i]) result = pane[i]->cursor->repeat_event(duration);
	}
	return result;
}


int MWindowGUI::translation_event()
{
//printf("MWindowGUI::translation_event 1 %d %d\n", get_x(), get_y());
	mwindow->session->mwindow_x = get_x();
	mwindow->session->mwindow_y = get_y();
	return 0;
}


int MWindowGUI::save_defaults(BC_Hash *defaults)
{
	defaults->update("MWINDOWWIDTH", get_w());
	defaults->update("MWINDOWHEIGHT", get_h());
	mainmenu->save_defaults(defaults);
	BC_WindowBase::save_defaults(defaults);
	return 0;
}

int MWindowGUI::keypress_event()
{
//printf("MWindowGUI::keypress_event 1 %d\n", get_keypress());
	int result = mbuttons->keypress_event();
	if( result ) return result;

	Track *this_track = 0, *first_track = 0;
	int collapse = 0, packed = 0, overwrite = 0, plugins = 0;
	double position = 0;

	switch( get_keypress() ) {
	case 'A':
		if( !ctrl_down() || !shift_down() || alt_down() ) break;
		mwindow->edl->tracks->clear_selected_edits();
		draw_overlays(1);
		result = 1;
		break;
	case 'e':
		mwindow->toggle_editing_mode();
		result = 1;
		break;

	case 'k': case 'K':
		if( alt_down() ) break;
		stop_transport("MWindowGUI::keypress_event 1");
		mwindow->nearest_plugin_keyframe(shift_down(),
			!ctrl_down() ? PLAY_FORWARD : PLAY_REVERSE);
		result = 1;
		break;

	case 'C':
		packed = 1;
	case 'c':
		if( !ctrl_down() || alt_down() ) break;
		mwindow->selected_edits_to_clipboard(packed);
		result = 1;
		break;
	case 'P':
		plugins = 1;
	case 'b':
		overwrite = -1; // fall thru
	case 'v':
		if( !ctrl_down() || alt_down() ) break;
		if( mwindow->session->current_operation == DROP_TARGETING ) {
			mwindow->session->current_operation = NO_OPERATION;
			mwindow->gui->set_editing_mode(1);
			int pane_no = 0;
			for( ; pane_no<TOTAL_PANES; ++pane_no  ) {
				if( !pane[pane_no] ) continue;
				first_track = pane[pane_no]->over_track();
				if( first_track ) break;
			}
			if( first_track ) {
				int cursor_x = pane[pane_no]->canvas->get_relative_cursor_x();
				position = mwindow->edl->get_cursor_position(cursor_x, pane_no);
			}
		}
		else
			position = mwindow->edl->local_session->get_selectionstart();
		if( !plugins )
			mwindow->paste(position, first_track, 0, overwrite);
		else
			mwindow->paste_clipboard(first_track, position, 1, 0, 1, 1, 1);
		mwindow->edl->tracks->clear_selected_edits();
		draw_overlays(1);
		result = 1;
		break;
	case 'M':
		collapse = 1;
	case 'm':
		mwindow->cut_selected_edits(0, collapse);
		result = 1;
		break;
	case 'z':
		collapse = 1;
	case 'x':
		if( !ctrl_down() || alt_down() ) break;
		mwindow->cut_selected_edits(1, collapse);
		result = 1;
		break;

	case '1' ... '8':
		if( !alt_down() || shift_down() ) break;
		if( !mwindow->select_asset(get_keypress()-'1',1) )
			result = 1;
		break;

	case LEFT:
		if( !ctrl_down() ) {
			if( alt_down() ) {
				stop_transport("MWindowGUI::keypress_event 1");
				mwindow->prev_edit_handle(shift_down());
			}
			else
				mwindow->move_left();
			result = 1;
		}
		break;

	case ',':
		if( !ctrl_down() && !alt_down() ) {
			mwindow->move_left();
			result = 1;
		}
		break;

	case RIGHT:
		if( !ctrl_down() ) {
			if( alt_down() ) {
				stop_transport("MWindowGUI::keypress_event 2");
				mwindow->next_edit_handle(shift_down());
			}
			else
				mwindow->move_right();
			result = 1;
		}
		break;

	case '.':
		if( !ctrl_down() && !alt_down() ) {
			mwindow->move_right();
			result = 1;
		}
		break;

	case UP:
		if( ctrl_down() && !alt_down() )
			mwindow->expand_y();
		else if( !ctrl_down() && alt_down() )
			mwindow->expand_autos(0,1,1);
		else if( ctrl_down() && alt_down() )
			mwindow->expand_autos(1,1,1);
		else
			mwindow->expand_sample();
		result = 1;
		break;

	case DOWN:
		if( ctrl_down() && !alt_down() )
			mwindow->zoom_in_y();
		else if( !ctrl_down() && alt_down() )
			mwindow->shrink_autos(0,1,1);
		else if( ctrl_down() && alt_down() )
			mwindow->shrink_autos(1,1,1);
		else
			mwindow->zoom_in_sample();
		result = 1;
		break;

	case PGUP:
		if( !ctrl_down() )
			mwindow->move_up();
		else
			mwindow->expand_t();
		result = 1;
		break;

	case PGDN:
		if( !ctrl_down() )
			mwindow->move_down();
		else
			mwindow->zoom_in_t();
		result = 1;
		break;

	case TAB:
	case LEFTTAB:
		for( int i=0; i<TOTAL_PANES; ++i ) {
			if( !pane[i] ) continue;
			if( (this_track = pane[i]->over_track()) != 0 ) break;
			if( (this_track = pane[i]->over_patchbay()) != 0 ) break;
		}

		if( get_keypress() == TAB ) { // Switch the record button
			if( this_track )
				this_track->record = !this_track->record ? 1 : 0;
		}
		else {
			int total_selected = mwindow->edl->tracks->total_of(Tracks::RECORD);
			// all selected if nothing previously selected or
			// if this patch was previously the only one selected and armed
			int selected = !total_selected || (total_selected == 1 &&
				this_track && this_track->record ) ? 1 : 0;
			mwindow->edl->tracks->select_all(Tracks::RECORD, selected);
			if( !selected && this_track ) this_track->record = 1;
		}

		update(0, NORMAL_DRAW, 0, 0, 1, 0, 1);
		unlock_window();
		mwindow->cwindow->update(0, 1, 1);
		lock_window("MWindowGUI::keypress_event 3");

		result = 1;
		break;

	case KEY_F1 ... KEY_F12:
		resend_event(mwindow->cwindow->gui);
		return 1;
	}

// since things under cursor have changed...
	if(result)
		cursor_motion_event();

	return result;
}

int MWindowGUI::keyboard_listener(BC_WindowBase *wp)
{
	return key_listener(wp->get_keypress());
}

int MWindowGUI::key_listener(int key)
{
	int result = 1;
	switch( key ) {
	case KPTV:
		if( !record->running() )
			record->start();
		else
			record->record_gui->interrupt_thread->start(0);
		break;
	case KPHAND:
		mwindow->quit();
		break;
#ifdef HAVE_DVB
	case KPBOOK:
		channel_info->toggle_scan();
		break;
#endif
	case KPMENU:
		if( !remote_control->deactivate() )
			remote_control->activate();
		break;
	default:
		result = 0;
		break;
	}
	return result;
}


void MWindowGUI::use_android_remote(int on)
{
	if( !on ) {
		delete android_control;
		android_control = 0;
		return;
	}
	if( android_control ) return;
	android_control = new AndroidControl(this);
}

int MWindowGUI::close_event()
{
	mainmenu->quit();
	return 0;
}

void MWindowGUI::stop_drawing()
{
	resource_thread->stop_draw(1);
}

int MWindowGUI::menu_w()
{
	return mainmenu->get_w();
}

int MWindowGUI::menu_h()
{
	return mainmenu->get_h();
}

void MWindowGUI::start_x_pane_drag()
{
	if(!x_pane_drag)
	{
		x_pane_drag = new BC_Popup(this,
			get_abs_cursor_x(0) - mwindow->theme->pane_w,
			BC_DisplayInfo::get_top_border() +
				get_y() +
				mwindow->theme->mcanvas_y,
			mwindow->theme->pane_w,
			mwindow->theme->mcanvas_h,
			mwindow->theme->drag_pane_color);
		x_pane_drag->draw_3segmentv(0,
			0,
			x_pane_drag->get_h(),
			mwindow->theme->get_image_set("xpane")[BUTTON_DOWNHI]);
		x_pane_drag->flash(1);
	}
	dragging_pane = 1;
}

void MWindowGUI::start_y_pane_drag()
{
	if(!y_pane_drag)
	{
//printf("MWindowGUI::start_y_pane_drag %d %d %d\n", __LINE__, get_x(), get_y());
		y_pane_drag = new BC_Popup(this,
			BC_DisplayInfo::get_left_border() +
				get_x() +
				mwindow->theme->mcanvas_x,
			get_abs_cursor_y(0) - mwindow->theme->pane_h,
			mwindow->theme->mcanvas_w,
			mwindow->theme->pane_h,
			mwindow->theme->drag_pane_color);
		y_pane_drag->draw_3segmenth(0,
			0,
			y_pane_drag->get_w(),
			mwindow->theme->get_image_set("ypane")[BUTTON_DOWNHI]);
		y_pane_drag->flash(1);
	}
	dragging_pane = 1;
}

void MWindowGUI::handle_pane_drag()
{
	if(dragging_pane)
	{
		if(x_pane_drag)
		{
			x_pane_drag->reposition_window(
				get_abs_cursor_x(0) - mwindow->theme->pane_w,
				x_pane_drag->get_y());
		}

		if(y_pane_drag)
		{
			y_pane_drag->reposition_window(
				y_pane_drag->get_x(),
				get_abs_cursor_y(0) - mwindow->theme->pane_h);
		}
	}
}


void MWindowGUI::create_x_pane(int cursor_x)
{
	if(total_panes() == 1)
	{
// create a horizontal pane
// do this 1st so the resize_event knows there are 2 panes
		mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE] +
			cursor_x -
			mwindow->theme->patchbay_w;
		pane[TOP_RIGHT_PANE] = new TimelinePane(mwindow,
			TOP_RIGHT_PANE,
			mwindow->theme->mcanvas_x +
				cursor_x,
			mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			mwindow->theme->mcanvas_h);
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			cursor_x - mwindow->theme->pane_w,
			mwindow->theme->mcanvas_h);
		pane[TOP_RIGHT_PANE]->create_objects();
	}
	else
	if(vertical_panes())
	{
// create 2 horizontal panes
		mwindow->edl->local_session->track_start[TOP_RIGHT_PANE] =
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE];
		mwindow->edl->local_session->track_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE];
		mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE] +
			cursor_x -
			mwindow->theme->patchbay_w;
		pane[TOP_RIGHT_PANE] = new TimelinePane(mwindow,
			TOP_RIGHT_PANE,
			mwindow->theme->mcanvas_x +
				cursor_x,
			pane[TOP_LEFT_PANE]->y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			pane[TOP_LEFT_PANE]->h);
		pane[BOTTOM_RIGHT_PANE] = new TimelinePane(mwindow,
			BOTTOM_RIGHT_PANE,
			mwindow->theme->mcanvas_x +
				cursor_x,
			pane[BOTTOM_LEFT_PANE]->y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			pane[BOTTOM_LEFT_PANE]->h);
		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			cursor_x - mwindow->theme->pane_w,
			pane[TOP_LEFT_PANE]->h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			pane[BOTTOM_LEFT_PANE]->x,
			pane[BOTTOM_LEFT_PANE]->y,
			cursor_x - mwindow->theme->pane_w,
			pane[BOTTOM_LEFT_PANE]->h);
		pane[TOP_RIGHT_PANE]->create_objects();
		pane[BOTTOM_RIGHT_PANE]->create_objects();
	}
	else
	if(horizontal_panes())
	{
// resize a horizontal pane
		mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] +=
			cursor_x -
			pane[TOP_RIGHT_PANE]->x;
		if(mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] < 0)
			mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] = 0;
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			cursor_x - mwindow->theme->pane_w,
			mwindow->theme->mcanvas_h);
		pane[TOP_RIGHT_PANE]->resize_event(
			mwindow->theme->mcanvas_x +
				cursor_x,
			pane[TOP_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			mwindow->theme->mcanvas_h);
	}
	else
	{
// resize 2 horizontal panes
		mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] +=
			cursor_x -
			pane[TOP_RIGHT_PANE]->x;
		if(mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] < 0)
			mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] = 0;
		mwindow->edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[TOP_RIGHT_PANE];

		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			pane[TOP_LEFT_PANE]->y,
			cursor_x - mwindow->theme->pane_w,
			pane[TOP_LEFT_PANE]->h);
		pane[TOP_RIGHT_PANE]->resize_event(
			mwindow->theme->mcanvas_x +
				cursor_x,
			pane[TOP_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			pane[TOP_RIGHT_PANE]->h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			pane[BOTTOM_LEFT_PANE]->y,
			cursor_x - mwindow->theme->pane_w,
			pane[BOTTOM_LEFT_PANE]->h);
		pane[BOTTOM_RIGHT_PANE]->resize_event(
			mwindow->theme->mcanvas_x +
				cursor_x,
			pane[BOTTOM_RIGHT_PANE]->y,
			mwindow->theme->mcanvas_x +
				mwindow->theme->mcanvas_w -
				cursor_x,
			pane[BOTTOM_RIGHT_PANE]->h);

	}
}


void MWindowGUI::delete_x_pane(int cursor_x)
{
// give left panes coordinates of right pane
	if(cursor_x < mwindow->theme->patchbay_w + PANE_DRAG_MARGIN &&
		pane[TOP_RIGHT_PANE])
	{
		mwindow->edl->local_session->view_start[TOP_LEFT_PANE] =
			mwindow->edl->local_session->view_start[TOP_RIGHT_PANE] -
			pane[TOP_RIGHT_PANE]->x + mwindow->theme->patchbay_w;
		if(mwindow->edl->local_session->view_start[TOP_LEFT_PANE] < 0)
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE] = 0;
		mwindow->edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE];
	}

	switch(total_panes())
	{
		case 2:
			if(pane[TOP_LEFT_PANE] && pane[TOP_RIGHT_PANE])
			{
// delete right pane
				delete pane[TOP_RIGHT_PANE];
				pane[TOP_RIGHT_PANE] = 0;
				pane[TOP_LEFT_PANE]->resize_event(
					mwindow->theme->mcanvas_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_w,
					mwindow->theme->mcanvas_h);

			}
			break;

		case 4:
// delete right panes
			delete pane[TOP_RIGHT_PANE];
			pane[TOP_RIGHT_PANE] = 0;
			delete pane[BOTTOM_RIGHT_PANE];
			pane[BOTTOM_RIGHT_PANE] = 0;
			pane[TOP_LEFT_PANE]->resize_event(
				mwindow->theme->mcanvas_x,
				pane[TOP_LEFT_PANE]->y,
				mwindow->theme->mcanvas_w,
				pane[TOP_LEFT_PANE]->h);
			pane[BOTTOM_LEFT_PANE]->resize_event(
				mwindow->theme->mcanvas_x,
				pane[BOTTOM_LEFT_PANE]->y,
				mwindow->theme->mcanvas_w,
				pane[BOTTOM_LEFT_PANE]->h);
			break;
	}
}

void MWindowGUI::create_y_pane(int cursor_y)
{
	if(total_panes() == 1)
	{
		mwindow->edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE];
		mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] =
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE] +
			cursor_y -
			mwindow->theme->mtimebar_h;
// do this 1st so the resize_event knows there are 2 panes
		pane[BOTTOM_LEFT_PANE] = new TimelinePane(mwindow,
			BOTTOM_LEFT_PANE,
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y +
				cursor_y,
			mwindow->theme->mcanvas_w,
			mwindow->theme->mcanvas_h -
				cursor_y);
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_w,
			pane[BOTTOM_LEFT_PANE]->y -
				mwindow->theme->mcanvas_y -
				mwindow->theme->pane_h);
		pane[BOTTOM_LEFT_PANE]->create_objects();
	}
	else
	if(horizontal_panes())
	{
// create 2 panes
		mwindow->edl->local_session->view_start[BOTTOM_LEFT_PANE] =
			mwindow->edl->local_session->view_start[TOP_LEFT_PANE];
		mwindow->edl->local_session->view_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->view_start[TOP_RIGHT_PANE];
		mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] =
		mwindow->edl->local_session->track_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE] +
			cursor_y -
			mwindow->theme->mtimebar_h;

		pane[BOTTOM_LEFT_PANE] = new TimelinePane(mwindow,
			BOTTOM_LEFT_PANE,
			pane[TOP_LEFT_PANE]->x,
			mwindow->theme->mcanvas_y +
				cursor_y,
			pane[TOP_LEFT_PANE]->w,
			mwindow->theme->mcanvas_h -
				cursor_y);
		pane[BOTTOM_RIGHT_PANE] = new TimelinePane(mwindow,
			BOTTOM_RIGHT_PANE,
			pane[TOP_RIGHT_PANE]->x,
			mwindow->theme->mcanvas_y +
				cursor_y,
			pane[TOP_RIGHT_PANE]->w,
			mwindow->theme->mcanvas_h -
				cursor_y);

		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			pane[TOP_LEFT_PANE]->w,
			pane[BOTTOM_LEFT_PANE]->y -
				mwindow->theme->mcanvas_y -
				mwindow->theme->pane_h);
		pane[TOP_RIGHT_PANE]->resize_event(
			pane[TOP_RIGHT_PANE]->x,
			pane[TOP_RIGHT_PANE]->y,
			pane[TOP_RIGHT_PANE]->w,
			pane[BOTTOM_RIGHT_PANE]->y -
				mwindow->theme->mcanvas_y -
				mwindow->theme->pane_h);

		pane[BOTTOM_LEFT_PANE]->create_objects();
		pane[BOTTOM_RIGHT_PANE]->create_objects();
	}
	else
	if(vertical_panes())
	{
// resize a pane
		mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] +=
			cursor_y -
			(pane[BOTTOM_LEFT_PANE]->y - mwindow->theme->mcanvas_y);
		if(mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] < 0)
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] = 0;
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_w,
			cursor_y - mwindow->theme->pane_h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			pane[BOTTOM_LEFT_PANE]->x,
			cursor_y +
				mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_w,
			mwindow->theme->mcanvas_h -
				cursor_y);
	}
	else
	{
// resize 2 panes
		mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] +=
			cursor_y -
			(pane[BOTTOM_LEFT_PANE]->y - mwindow->theme->mcanvas_y);
		if(mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] < 0)
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] = 0;
		mwindow->edl->local_session->track_start[BOTTOM_RIGHT_PANE] =
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE];
		pane[TOP_LEFT_PANE]->resize_event(
			pane[TOP_LEFT_PANE]->x,
			pane[TOP_LEFT_PANE]->y,
			pane[TOP_LEFT_PANE]->w,
			cursor_y - mwindow->theme->pane_h);
		pane[BOTTOM_LEFT_PANE]->resize_event(
			pane[BOTTOM_LEFT_PANE]->x,
			cursor_y +
				mwindow->theme->mcanvas_y,
			pane[BOTTOM_LEFT_PANE]->w,
			mwindow->theme->mcanvas_h -
					cursor_y);
		pane[TOP_RIGHT_PANE]->resize_event(
			pane[TOP_RIGHT_PANE]->x,
			pane[TOP_RIGHT_PANE]->y,
			pane[TOP_RIGHT_PANE]->w,
			cursor_y - mwindow->theme->pane_h);
		pane[BOTTOM_RIGHT_PANE]->resize_event(
			pane[BOTTOM_RIGHT_PANE]->x,
			cursor_y +
				mwindow->theme->mcanvas_y,
			pane[BOTTOM_RIGHT_PANE]->w,
			mwindow->theme->mcanvas_h -
					cursor_y);
	}
}

void MWindowGUI::delete_y_pane(int cursor_y)
{
	if(cursor_y < mwindow->theme->mtimebar_h +
		PANE_DRAG_MARGIN &&
		pane[BOTTOM_LEFT_PANE])
	{
// give top pane coordinates of bottom pane
		mwindow->edl->local_session->track_start[TOP_LEFT_PANE] =
			mwindow->edl->local_session->track_start[BOTTOM_LEFT_PANE] -
			pane[BOTTOM_LEFT_PANE]->y;
		if(mwindow->edl->local_session->track_start[TOP_LEFT_PANE] < 0)
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE] = 0;
		mwindow->edl->local_session->track_start[TOP_RIGHT_PANE] =
			mwindow->edl->local_session->track_start[TOP_LEFT_PANE];
	}

// delete a pane
	switch(total_panes())
	{
		case 2:
			delete pane[BOTTOM_LEFT_PANE];
			pane[BOTTOM_LEFT_PANE] = 0;
			pane[TOP_LEFT_PANE]->resize_event(
				mwindow->theme->mcanvas_x,
				mwindow->theme->mcanvas_y,
				mwindow->theme->mcanvas_w,
				mwindow->theme->mcanvas_h);
			break;

		case 4:
// delete bottom 2 panes

			delete pane[BOTTOM_LEFT_PANE];
			pane[BOTTOM_LEFT_PANE] = 0;
			delete pane[BOTTOM_RIGHT_PANE];
			pane[BOTTOM_RIGHT_PANE] = 0;
			pane[TOP_LEFT_PANE]->resize_event(
				pane[TOP_LEFT_PANE]->x,
				mwindow->theme->mcanvas_y,
				pane[TOP_LEFT_PANE]->w,
				mwindow->theme->mcanvas_h);
			pane[TOP_RIGHT_PANE]->resize_event(
				pane[TOP_RIGHT_PANE]->x,
				mwindow->theme->mcanvas_y,
				pane[TOP_RIGHT_PANE]->w,
				mwindow->theme->mcanvas_h);
			break;
	}
}

void MWindowGUI::stop_pane_drag()
{
	dragging_pane = 0;
	resource_thread->stop_draw(0);

	if(x_pane_drag)
	{
// cursor position relative to canvas
		int cursor_x = x_pane_drag->get_x() -
			get_x() -
			BC_DisplayInfo::get_left_border() -
			mwindow->theme->mcanvas_x +
			mwindow->theme->pane_w;
		delete x_pane_drag;
		x_pane_drag = 0;


		if(cursor_x >= mwindow->theme->patchbay_w + PANE_DRAG_MARGIN &&
			cursor_x < mwindow->theme->mcanvas_w -
				BC_ScrollBar::get_span(SCROLL_VERT) -
				PANE_DRAG_MARGIN)
		{
			create_x_pane(cursor_x);
			mwindow->edl->local_session->x_pane = cursor_x;
		}
		else
// deleted a pane
		{
			delete_x_pane(cursor_x);
			mwindow->edl->local_session->x_pane = -1;
		}


	}

	if(y_pane_drag)
	{
// cursor position relative to canvas
		int cursor_y = y_pane_drag->get_y() -
			get_y() -
			BC_DisplayInfo::get_top_border() -
			mwindow->theme->mcanvas_y +
			mwindow->theme->pane_h;
		delete y_pane_drag;
		y_pane_drag = 0;



		if(cursor_y >= mwindow->theme->mtimebar_h +
				PANE_DRAG_MARGIN &&
			cursor_y < mwindow->theme->mcanvas_h -
				BC_ScrollBar::get_span(SCROLL_HORIZ) -
				PANE_DRAG_MARGIN)
		{
			create_y_pane(cursor_y);
			mwindow->edl->local_session->y_pane = cursor_y;
		}
		else
		{
			delete_y_pane(cursor_y);
			mwindow->edl->local_session->y_pane = -1;
		}
	}

	update_pane_dividers();
	update_cursor();
// required to get new widgets to appear
	show_window();
	resource_thread->start_draw();
}

// create panes from EDL
void MWindowGUI::load_panes()
{
	int need_x_panes = 0;
	int need_y_panes = 0;
// use names from create functions
	int cursor_x = mwindow->edl->local_session->x_pane;
	int cursor_y = mwindow->edl->local_session->y_pane;

	resource_thread->stop_draw(1);
	if(cursor_x >=
		mwindow->theme->patchbay_w + PANE_DRAG_MARGIN &&
		cursor_x <
		mwindow->theme->mcanvas_w -
				BC_ScrollBar::get_span(SCROLL_VERT) -
				PANE_DRAG_MARGIN)
	{
		need_x_panes = 1;
	}

	if(cursor_y >=
		mwindow->theme->mtimebar_h + PANE_DRAG_MARGIN &&
		cursor_y <
		mwindow->theme->mcanvas_h -
				BC_ScrollBar::get_span(SCROLL_HORIZ) -
				PANE_DRAG_MARGIN)
	{
		need_y_panes = 1;
	}

//printf("MWindowGUI::load_panes %d %d %d\n", __LINE__, need_x_panes, need_y_panes);


	if(need_x_panes)
	{
		if(need_y_panes)
		{
// 4 panes
			if(total_panes() == 1)
			{
// create 4 panes
//printf("MWindowGUI::load_panes %d\n", __LINE__);
				pane[TOP_RIGHT_PANE] = new TimelinePane(mwindow,
					TOP_RIGHT_PANE,
					mwindow->theme->mcanvas_x +
						cursor_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					cursor_y - mwindow->theme->pane_h);
				pane[BOTTOM_LEFT_PANE] = new TimelinePane(mwindow,
					BOTTOM_LEFT_PANE,
					mwindow->theme->mcanvas_x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					cursor_x - mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[BOTTOM_RIGHT_PANE] = new TimelinePane(mwindow,
					BOTTOM_RIGHT_PANE,
					pane[TOP_RIGHT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					pane[TOP_RIGHT_PANE]->w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[TOP_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					pane[TOP_LEFT_PANE]->y,
					cursor_x - mwindow->theme->pane_w,
					cursor_y - mwindow->theme->pane_h);
				pane[TOP_RIGHT_PANE]->create_objects();
				pane[BOTTOM_LEFT_PANE]->create_objects();
				pane[BOTTOM_RIGHT_PANE]->create_objects();
			}
			else
			if(horizontal_panes())
			{
// create vertical panes
//printf("MWindowGUI::load_panes %d\n", __LINE__);
				pane[BOTTOM_LEFT_PANE] = new TimelinePane(mwindow,
					BOTTOM_LEFT_PANE,
					mwindow->theme->mcanvas_x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					cursor_x - mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[BOTTOM_RIGHT_PANE] = new TimelinePane(mwindow,
					BOTTOM_RIGHT_PANE,
					pane[TOP_RIGHT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					pane[TOP_RIGHT_PANE]->w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[TOP_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					pane[TOP_LEFT_PANE]->y,
					cursor_x - mwindow->theme->pane_w,
					cursor_y - mwindow->theme->pane_h);
				pane[TOP_RIGHT_PANE]->resize_event(
					mwindow->theme->mcanvas_x +
						cursor_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					cursor_y - mwindow->theme->pane_h);
				pane[BOTTOM_LEFT_PANE]->create_objects();
				pane[BOTTOM_RIGHT_PANE]->create_objects();
			}
			else
			if(vertical_panes())
			{
// create horizontal panes
//printf("MWindowGUI::load_panes %d\n", __LINE__);
				pane[TOP_RIGHT_PANE] = new TimelinePane(mwindow,
					TOP_RIGHT_PANE,
					mwindow->theme->mcanvas_x +
						cursor_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					cursor_y - mwindow->theme->pane_h);
				pane[BOTTOM_RIGHT_PANE] = new TimelinePane(mwindow,
					BOTTOM_RIGHT_PANE,
					pane[TOP_RIGHT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					pane[TOP_RIGHT_PANE]->w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[TOP_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					pane[TOP_LEFT_PANE]->y,
					cursor_x - mwindow->theme->pane_w,
					cursor_y - mwindow->theme->pane_h);
				pane[BOTTOM_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x -  mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[TOP_RIGHT_PANE]->create_objects();
				pane[BOTTOM_RIGHT_PANE]->create_objects();


			}
			else
			{
// resize all panes
//printf("MWindowGUI::load_panes %d\n", __LINE__);
				pane[TOP_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					pane[TOP_LEFT_PANE]->y,
					cursor_x - mwindow->theme->pane_w,
					cursor_y - mwindow->theme->pane_h);
				pane[TOP_RIGHT_PANE]->resize_event(
					mwindow->theme->mcanvas_x +
						cursor_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					cursor_y - mwindow->theme->pane_h);
				pane[BOTTOM_LEFT_PANE]->resize_event(
					pane[TOP_LEFT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x - mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h -
						cursor_y);
				pane[BOTTOM_RIGHT_PANE]->resize_event(
					pane[TOP_RIGHT_PANE]->x,
					mwindow->theme->mcanvas_y +
						cursor_y,
					pane[TOP_RIGHT_PANE]->w,
					mwindow->theme->mcanvas_h -
						cursor_y);


			}
		}
		else
		{
// 2 X panes
			if(pane[BOTTOM_LEFT_PANE]) delete pane[BOTTOM_LEFT_PANE];
			if(pane[BOTTOM_RIGHT_PANE]) delete pane[BOTTOM_RIGHT_PANE];
			pane[BOTTOM_LEFT_PANE] = 0;
			pane[BOTTOM_RIGHT_PANE] = 0;

			if(!pane[TOP_RIGHT_PANE])
			{
				pane[TOP_RIGHT_PANE] = new TimelinePane(mwindow,
					TOP_RIGHT_PANE,
					mwindow->theme->mcanvas_x +
						cursor_x,
					mwindow->theme->mcanvas_y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					mwindow->theme->mcanvas_h);
				pane[TOP_LEFT_PANE]->resize_event(
					mwindow->theme->mcanvas_x,
					mwindow->theme->mcanvas_y,
					cursor_x - mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h);
				pane[TOP_RIGHT_PANE]->create_objects();
			}
			else
			{
				pane[TOP_LEFT_PANE]->resize_event(
					mwindow->theme->mcanvas_x,
					mwindow->theme->mcanvas_y,
					cursor_x - mwindow->theme->pane_w,
					mwindow->theme->mcanvas_h);
				pane[TOP_RIGHT_PANE]->resize_event(
					mwindow->theme->mcanvas_x +
						cursor_x,
					pane[TOP_RIGHT_PANE]->y,
					mwindow->theme->mcanvas_x +
						mwindow->theme->mcanvas_w -
						cursor_x,
					mwindow->theme->mcanvas_h);
			}
		}
	}
	else
	if(need_y_panes)
	{
// 2 Y panes
		if(pane[TOP_RIGHT_PANE]) delete pane[TOP_RIGHT_PANE];
		if(pane[BOTTOM_RIGHT_PANE]) delete pane[BOTTOM_RIGHT_PANE];
		pane[TOP_RIGHT_PANE] = 0;
		pane[BOTTOM_RIGHT_PANE] = 0;

		if(!pane[BOTTOM_LEFT_PANE])
		{
//printf("MWindowGUI::load_panes %d\n", __LINE__);
			pane[BOTTOM_LEFT_PANE] = new TimelinePane(mwindow,
				BOTTOM_LEFT_PANE,
				mwindow->theme->mcanvas_x,
				mwindow->theme->mcanvas_y +
					cursor_y,
				mwindow->theme->mcanvas_w,
				mwindow->theme->mcanvas_h -
					cursor_y);
			pane[TOP_LEFT_PANE]->resize_event(
				mwindow->theme->mcanvas_x,
				mwindow->theme->mcanvas_y,
				mwindow->theme->mcanvas_w,
				pane[BOTTOM_LEFT_PANE]->y -
					mwindow->theme->mcanvas_y -
					mwindow->theme->pane_h);
			pane[BOTTOM_LEFT_PANE]->create_objects();
		}
		else
		{
			pane[TOP_LEFT_PANE]->resize_event(
				mwindow->theme->mcanvas_x,
				mwindow->theme->mcanvas_y,
				mwindow->theme->mcanvas_w,
				cursor_y - mwindow->theme->pane_h);
			pane[BOTTOM_LEFT_PANE]->resize_event(
				pane[BOTTOM_LEFT_PANE]->x,
				cursor_y +
					mwindow->theme->mcanvas_y,
				mwindow->theme->mcanvas_w,
				mwindow->theme->mcanvas_h -
					cursor_y);
		}
	}
	else
	{
// 1 pane
		if(pane[TOP_RIGHT_PANE]) delete pane[TOP_RIGHT_PANE];
		if(pane[BOTTOM_RIGHT_PANE]) delete pane[BOTTOM_RIGHT_PANE];
		if(pane[BOTTOM_LEFT_PANE]) delete pane[BOTTOM_LEFT_PANE];
		pane[TOP_RIGHT_PANE] = 0;
		pane[BOTTOM_RIGHT_PANE] = 0;
		pane[BOTTOM_LEFT_PANE] = 0;
		pane[TOP_LEFT_PANE]->resize_event(
			mwindow->theme->mcanvas_x,
			mwindow->theme->mcanvas_y,
			mwindow->theme->mcanvas_w,
			mwindow->theme->mcanvas_h);
	}

	update_pane_dividers();
	show_window();

	resource_thread->start_draw();
}

void MWindowGUI::update_pane_dividers()
{

	if(horizontal_panes() || total_panes() == 4)
	{
		int x = pane[TOP_RIGHT_PANE]->x - mwindow->theme->pane_w;
		int y = mwindow->theme->mcanvas_y;
		int h = mwindow->theme->mcanvas_h;

		if(!x_divider)
		{
			add_subwindow(x_divider = new PaneDivider(
				mwindow, x, y, h, 1));
			x_divider->create_objects();
		}
		else
		{
			x_divider->reposition_window(x, y, h);
			x_divider->draw(0);
		}
	}
	else
	{
		if(x_divider)
		{
			delete x_divider;
			x_divider = 0;
		}
	}

	if(vertical_panes() || total_panes() == 4)
	{
		int x = mwindow->theme->mcanvas_x;
		int y = pane[BOTTOM_LEFT_PANE]->y -
			mwindow->theme->pane_h;
		int w = mwindow->theme->mcanvas_w;
		if(!y_divider)
		{
			add_subwindow(y_divider = new PaneDivider(
				mwindow, x, y, w, 0));
			y_divider->create_objects();
		}
		else
		{
			y_divider->reposition_window(x, y, w);
			y_divider->draw(0);
		}
	}
	else
	{
		if(y_divider)
		{
			delete y_divider;
			y_divider = 0;
		}
	}
}

void MWindowGUI::draw_samplemovement()
{
	draw_canvas(0, 1);
	show_cursor(1);
	flash_canvas(0);
	update_timebar(0);
	zoombar->update();
	update_scrollbars(1);
}

void MWindowGUI::draw_trackmovement()
{
	update_scrollbars(0);
	draw_canvas(0, 0);
	update_patchbay();
	flash_canvas(1);
}


void MWindowGUI::update_mixers(Track *track, int v)
{
	for( int i=0; i<TOTAL_PANES;  ++i ) {
		if( !pane[i] ) continue;
		PatchBay *patchbay = pane[i]->patchbay;
		if( !patchbay ) continue;
		for( int j=0; j<patchbay->patches.total; ++j ) {
			PatchGUI *patchgui = patchbay->patches.values[j];
			if( !patchgui->mix ) continue;
			if( !track || patchgui->track == track ) {
				patchgui->mix->update(v>=0 ? v :
					mwindow->mixer_track_active(patchgui->track));
			}
		}
	}
}

void MWindowGUI::stop_transport(const char *lock_msg)
{
	if( !mbuttons->transport->is_stopped() ) {
		if( lock_msg ) unlock_window();
		mbuttons->transport->handle_transport(STOP, 1);
		if( lock_msg ) lock_window(lock_msg);
	}
}

PaneButton::PaneButton(MWindow *mwindow, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("pane"))
{
	this->mwindow = mwindow;
}

int PaneButton::cursor_motion_event()
{
	if(get_top_level()->get_button_down() &&
		is_event_win() &&
		get_status() == BUTTON_DOWNHI &&
		!cursor_inside())
	{
//		printf("PaneButton::cursor_motion_event %d\n", __LINE__);
// create drag bar
		if(get_cursor_x() < 0 && !mwindow->gui->dragging_pane)
		{
			mwindow->gui->start_x_pane_drag();
		}
		else
		if(get_cursor_y() < 0 && !mwindow->gui->dragging_pane)
		{
			mwindow->gui->start_y_pane_drag();
		}
	}

	mwindow->gui->handle_pane_drag();

	int result = BC_Button::cursor_motion_event();
	return result;
}

int PaneButton::button_release_event()
{
	if( get_buttonpress() != WHEEL_DOWN && get_buttonpress() != WHEEL_UP )
		mwindow->gui->stop_pane_drag();
	int result = BC_Button::button_release_event();
	return result;
}


FFMpegToggle::FFMpegToggle(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->ffmpeg_toggle,
	 mwindow->preferences->get_file_probe_armed("FFMPEG_Early") > 0 ? 1 : 0)
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
	set_tooltip(get_value() ? FFMPEG_EARLY_TIP : FFMPEG_LATE_TIP);
}

FFMpegToggle::~FFMpegToggle()
{
}

int FFMpegToggle::handle_event()
{
	int ffmpeg_early_probe = get_value();
	set_tooltip(ffmpeg_early_probe ? FFMPEG_EARLY_TIP : FFMPEG_LATE_TIP);
	mwindow->preferences->set_file_probe_armed("FFMPEG_Early", ffmpeg_early_probe);
	mwindow->preferences->set_file_probe_armed("FFMPEG_Late", !ffmpeg_early_probe);

	mwindow->show_warning(&mwindow->preferences->warn_indexes,
		_("Changing the base codecs may require rebuilding indexes."));
	return 1;
}


ProxyToggle::ProxyToggle(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : BC_Toggle(x, y, ( !mwindow->edl->session->proxy_use_scaler ?
			mwindow->theme->proxy_p_toggle :
			mwindow->theme->proxy_s_toggle ),
		mwindow->edl->session->proxy_disabled_scale != 1)
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
	scaler_images = mwindow->edl->session->proxy_use_scaler;
	set_tooltip(mwindow->edl->session->proxy_disabled_scale==1 ?
		_("Disable proxy") : _("Enable proxy"));
}

void ProxyToggle::show()
{
	int use_scaler = mwindow->edl->session->proxy_use_scaler;
	if( scaler_images != use_scaler )
		set_images(!(scaler_images=use_scaler) ?
			mwindow->theme->proxy_p_toggle :
			mwindow->theme->proxy_s_toggle );
	draw_face(1, 0);
	if( is_hidden() )
		show_window();
}

void ProxyToggle::hide()
{
	if( !is_hidden() )
		hide_window();
}

ProxyToggle::~ProxyToggle()
{
}

int ProxyToggle::handle_event()
{
	int disabled = get_value();
	mwindow->gui->unlock_window();
	if( disabled )
		mwindow->disable_proxy();
	else
		mwindow->enable_proxy();
	mwindow->gui->lock_window("ProxyToggle::handle_event");
	set_tooltip(!disabled ? _("Disable proxy") : _("Enable proxy"));
	return 1;
}

int ProxyToggle::keypress_event()
{
	if( ctrl_down() && !shift_down() && !alt_down() ) {
		int key = get_keypress();
		if( key == 'r' ) {
			int value = get_value() ? 0 : 1;
			set_value(value);
			return handle_event();
		}
	}
	return 0;
}

