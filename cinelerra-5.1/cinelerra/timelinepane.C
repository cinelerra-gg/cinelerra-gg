#include "bcsignals.h"
#include "edl.h"
#include "timelinepane.h"
#include "localsession.h"
#include "maincursor.h"
#include "mtimebar.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "patchbay.h"
#include "samplescroll.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"
#include "trackcanvas.h"
#include "trackscroll.h"


// coordinates are relative to the main window
TimelinePane::TimelinePane(MWindow *mwindow,
	int number,
	int x,
	int y,
	int w,
	int h)
{
// printf("TimelinePane::TimelinePane %d number=%d %d %d %d %d\n",
// __LINE__,
// number,
// x,
// y,
// w,
// h);
	this->mwindow = mwindow;
	this->number = number;
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	patchbay = 0;
	timebar = 0;
	samplescroll = 0;
	trackscroll = 0;
	cursor = 0;
}

TimelinePane::~TimelinePane()
{
	delete canvas;
	delete cursor;
	delete patchbay;
	delete timebar;
	delete samplescroll;
	delete trackscroll;
}

void TimelinePane::create_objects()
{
	this->gui = mwindow->gui;
	mwindow->theme->get_pane_sizes(gui,
		&view_x,
		&view_y,
		&view_w,
		&view_h,
		number,
		x,
		y,
		w,
		h);
	cursor = new MainCursor(mwindow, this);
	cursor->create_objects();
// printf("TimelinePane::create_objects %d number=%d x=%d y=%d w=%d h=%d view_x=%d view_w=%d\n",
// __LINE__,
// number,
// x,
// y,
// w,
// h,
// view_x,
// view_w);


	gui->add_subwindow(canvas = new TrackCanvas(mwindow,
		this,
		view_x,
		view_y,
		view_w,
		view_h));
	canvas->create_objects();

	if(number == TOP_LEFT_PANE ||
		number == BOTTOM_LEFT_PANE)
	{
		int patchbay_y = y;
		int patchbay_h = h;

		if(number == TOP_LEFT_PANE)
		{
			patchbay_y += mwindow->theme->mtimebar_h;
			patchbay_h -= mwindow->theme->mtimebar_h;
		}

		gui->add_subwindow(patchbay = new PatchBay(mwindow,
			this,
			x,
			patchbay_y,
			mwindow->theme->patchbay_w,
			patchbay_h));
		patchbay->create_objects();
	}

	if(number == TOP_LEFT_PANE ||
		number == TOP_RIGHT_PANE)
	{
		int timebar_w = view_w;
// Overlap right scrollbar
		if(gui->total_panes() == 1 ||
			number == TOP_RIGHT_PANE)
			timebar_w += BC_ScrollBar::get_span(SCROLL_VERT);

		gui->add_subwindow(timebar = new MTimeBar(mwindow,
			this,
			view_x,
 			y,
			timebar_w,
			mwindow->theme->mtimebar_h));
		timebar->create_objects();
	}

	create_sample_scroll(view_x, view_y, view_w, view_h);

	create_track_scroll(view_x, view_y, view_w, view_h);
}



void TimelinePane::resize_event(int x, int y, int w, int h)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
	mwindow->theme->get_pane_sizes(
		gui,
		&view_x,
		&view_y,
		&view_w,
		&view_h,
		number,
		x,
		y,
		w,
		h);
// printf("TimelinePane::resize_event %d number=%d x=%d y=%d w=%d h=%d view_x=%d view_y=%d view_w=%d view_h=%d\n",
// __LINE__,
// number,
// x,
// y,
// w,
// h,
// view_x,
// view_y,
// view_w,
// view_h);

	if(timebar)
	{
		int timebar_w = view_w;
// Overlap right scrollbar
		if(gui->total_panes() == 1 ||
			(gui->total_panes() == 2 &&
			(number == TOP_LEFT_PANE ||
			number == BOTTOM_LEFT_PANE)) ||
			number == TOP_RIGHT_PANE ||
			number == BOTTOM_RIGHT_PANE)
			timebar_w += BC_ScrollBar::get_span(SCROLL_VERT);
		timebar->resize_event(view_x,
			y,
			timebar_w,
			mwindow->theme->mtimebar_h);
	}

	if(patchbay)
	{
		int patchbay_y = y;
		int patchbay_h = h;

		if(number == TOP_LEFT_PANE)
		{
			patchbay_y += mwindow->theme->mtimebar_h;
			patchbay_h -= mwindow->theme->mtimebar_h;
		}
		else
		{
		}

		patchbay->resize_event(x,
			patchbay_y,
			mwindow->theme->patchbay_w,
			patchbay_h);
	}

	if(samplescroll)
	{
		if(gui->pane[TOP_LEFT_PANE] &&
			gui->pane[BOTTOM_LEFT_PANE] &&
			(number == TOP_LEFT_PANE ||
				number == TOP_RIGHT_PANE))
		{
			delete samplescroll;
			samplescroll = 0;
		}
		else
		{
			samplescroll->resize_event(view_x,
				view_y + view_h,
				view_w);
			samplescroll->set_position();
		}
	}
	else
		create_sample_scroll(view_x, view_y, view_w, view_h);

	if(trackscroll)
	{
		if(gui->pane[TOP_LEFT_PANE] &&
			gui->pane[TOP_RIGHT_PANE] &&
			(number == TOP_LEFT_PANE ||
			number == BOTTOM_LEFT_PANE))
		{
			delete trackscroll;
			trackscroll = 0;
		}
		else
		{
			trackscroll->resize_event(view_x + view_w,
				view_y,
				view_h);
			trackscroll->set_position();
		}
	}
	else
		create_track_scroll(view_x, view_y, view_w, view_h);

	canvas->reposition_window(view_x, view_y, view_w, view_h);
	canvas->resize_event();
}

void TimelinePane::create_sample_scroll(int view_x, int view_y, int view_w, int view_h)
{
//printf("TimelinePane::create_sample_scroll %d %d\n", __LINE__, number);
	if(number == BOTTOM_LEFT_PANE ||
		number == BOTTOM_RIGHT_PANE ||
		(gui->total_panes() == 2 &&
			gui->pane[TOP_LEFT_PANE] &&
			gui->pane[TOP_RIGHT_PANE]) ||
		gui->total_panes() == 1)
	{
//printf("TimelinePane::create_sample_scroll %d %d %d\n", __LINE__, y, h);
		gui->add_subwindow(samplescroll = new SampleScroll(mwindow,
			this,
			view_x,
			y + h - BC_ScrollBar::get_span(SCROLL_VERT),
			view_w));
		samplescroll->set_position();
	}
}

void TimelinePane::create_track_scroll(int view_x, int view_y, int view_w, int view_h)
{
	if(number == TOP_RIGHT_PANE ||
		number == BOTTOM_RIGHT_PANE ||
		gui->vertical_panes() ||
		gui->total_panes() == 1)
	{
		gui->add_subwindow(trackscroll = new TrackScroll(mwindow,
			this,
			view_x + view_w,
			view_y,
			view_h));
		trackscroll->set_position();
	}
}


void TimelinePane::update(int scrollbars,
	int do_canvas,
	int timebar,
	int patchbay)
{
	if(timebar && this->timebar) this->timebar->update(0);
	if(scrollbars) {
		if(samplescroll && this->samplescroll) samplescroll->set_position();
		if(trackscroll && this->trackscroll) trackscroll->set_position();
	}
	if(patchbay && this->patchbay) this->patchbay->update();

	if( do_canvas != NO_DRAW ) {
		this->canvas->draw(do_canvas, 1);
		this->cursor->show();
		this->canvas->flash(0);
// Activate causes the menubar to deactivate.  Don't want this for
// picon thread.
//		if(do_canvas != IGNORE_THREAD) this->canvas->activate();
	}
}

void TimelinePane::activate()
{
	canvas->activate();
	gui->focused_pane = number;
}

Track *TimelinePane::over_track()
{
	int canvas_x = canvas->get_relative_cursor_x();
	if( canvas_x < 0 || canvas_x >= canvas->get_w() ) return 0;
	int canvas_y = canvas->get_relative_cursor_y();
	if( canvas_y < 0 || canvas_y >= canvas->get_h() ) return 0;
	int pane_y = canvas_y + mwindow->edl->local_session->track_start[number];
	for( Track *track=mwindow->edl->tracks->first; track; track=track->next ) {
		int track_y = track->y_pixel;
		if( pane_y < track_y ) continue;
		track_y += track->vertical_span(mwindow->theme);
		if( pane_y < track_y )
			return track;
	}

	return 0;
}

Track *TimelinePane::over_patchbay()
{
	if( !patchbay ) return 0;
	int patch_x = patchbay->get_relative_cursor_x() ;
	if( patch_x < 0 || patch_x >= patchbay->get_w() ) return 0;
	int patch_y = patchbay->get_relative_cursor_y();
	if( patch_y < 0 || patch_y >= patchbay->get_h() ) return 0;
//	int canvas_x = patch_x + patchbay->get_x() - canvas->get_x();
	int canvas_y = patch_y + patchbay->get_y() - canvas->get_y();
	int pane_y = canvas_y + mwindow->edl->local_session->track_start[number];
	for( Track *track=mwindow->edl->tracks->first; track; track=track->next ) {
		int track_y = track->y_pixel;
		if( pane_y < track_y ) continue;
		track_y += track->vertical_span(mwindow->theme);
		if( pane_y < track_y )
			return track;
	}

	return 0;
}

