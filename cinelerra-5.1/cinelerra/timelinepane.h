#ifndef TIMELINEPANE_H
#define TIMELINEPANE_H

#include "maincursor.inc"
#include "mtimebar.inc"
#include "mwindow.inc"
#include "mwindowgui.inc"
#include "patchbay.inc"
#include "samplescroll.inc"
#include "track.inc"
#include "trackcanvas.inc"
#include "trackscroll.inc"



class TimelinePane
{
public:
// coordinates are relative to the main window
	TimelinePane(MWindow *mwindow,
		int number,
		int x,
		int y,
		int w,
		int h);
	~TimelinePane();
	void create_objects();
	void resize_event(int x, int y, int w, int h);
	void update(int scrollbars,
		int do_canvas,
		int timebar,
		int patchbay);
	void activate();
	void create_track_scroll(int view_x, int view_y, int view_w, int view_h);
	void create_sample_scroll(int view_x, int view_y, int view_w, int view_h);
	Track *over_track();
	Track *over_patchbay();

	MWindow *mwindow;
	MWindowGUI *gui;

	MainCursor *cursor;
	PatchBay *patchbay;
	MTimeBar *timebar;
	SampleScroll *samplescroll;
	TrackScroll *trackscroll;
	TrackCanvas *canvas;
// number of the name
	int number;
// total area including widgets
	int x, y, w, h;
// area for drawing tracks, excluding the widgets
	int view_x, view_y, view_w, view_h;
};



#endif


