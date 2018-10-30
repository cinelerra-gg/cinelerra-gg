
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

#ifndef TIMEBAR_H
#define TIMEBAR_H

#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "labels.inc"
#include "labeledit.inc"
#include "mwindow.inc"
#include "recordlabel.inc"
#include "testobject.h"
#include "timebar.inc"
#include "timelinepane.inc"
#include "vwindowgui.inc"

class TimeBarLeftArrow;
class TimeBarRightArrow;

class LabelGUI;
class InPointGUI;
class OutPointGUI;
class PresentationGUI;

// Operations for cursor
#define TIMEBAR_NONE        0
#define TIMEBAR_DRAG        1
#define TIMEBAR_DRAG_LEFT   2
#define TIMEBAR_DRAG_RIGHT  3
#define TIMEBAR_DRAG_CENTER 4
#define TIMEBAR_DRAG_LABEL  5

class LabelGUI : public BC_Toggle
{
public:
	LabelGUI(MWindow *mwindow,
		TimeBar *timebar,
		int64_t pixel,
		int y,
		double position,
		VFrame **data = 0);
	virtual ~LabelGUI();

	static int translate_pixel(MWindow *mwindow, int pixel);
	virtual int handle_event();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
	void reposition(int flush = 1);
	void update_value();
	int test_drag_label(int press);

	Label *label;
	int button_press_event();
	int button_release_event();
	MWindow *mwindow;
	VWindowGUI *gui;
	TimeBar *timebar;
	int64_t pixel;
	double position;
};

class TestPointGUI : public LabelGUI
{
public:
	TestPointGUI(MWindow *mwindow,
		TimeBar *timebar,
		int64_t pixel,
		double position);
};

class InPointGUI : public LabelGUI
{
public:
	InPointGUI(MWindow *mwindow,
		TimeBar *timebar,
		int64_t pixel,
		double position);
	virtual ~InPointGUI();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
};

class OutPointGUI : public LabelGUI
{
public:
	OutPointGUI(MWindow *mwindow,
		TimeBar *timebar,
		int64_t pixel,
		double position);
	virtual ~OutPointGUI();
	static int get_y(MWindow *mwindow, TimeBar *timebar);
};

class PresentationGUI : public LabelGUI
{
public:
	PresentationGUI(MWindow *mwindow,
		TimeBar *timebar,
		int64_t pixel,
		double position);
	~PresentationGUI();
};

class TimeBar : public BC_SubWindow
{
public:
	TimeBar(MWindow *mwindow,
		BC_WindowBase *gui,
		int x,
		int y,
		int w,
		int h);
	virtual ~TimeBar();

	virtual void create_objects();
	int update_defaults();
	virtual int button_press_event();
	virtual int button_release_event();
	int cursor_motion_event();
	int cursor_leave_event();
	virtual void update_clock(double position);

	LabelEdit *label_edit;

// Synchronize label, in/out, presentation display with master EDL
	virtual void update(int flush);
	virtual void draw_time();
// Called by update and draw_time.
	virtual void draw_range();
	virtual void select_label(double position);
	virtual void stop_playback();
	virtual EDL* get_edl();
	virtual int test_preview(int buttonpress);
	virtual void update_preview();
	virtual int64_t position_to_pixel(double position);
	virtual double pixel_to_position(int pixel);
	virtual void handle_mwindow_drag();
	virtual void activate_timeline();
	int move_preview(int &redraw);
// Get highlight status when the cursor is over the timeline.
	virtual double test_highlight();
	virtual int has_preview() { return 0; }
	virtual int is_vwindow() { return 0; }


	void update_labels();
	void update_points();
// Make sure widgets are highlighted according to selection status
	void update_highlights();
	void draw_inout_highlight();
// Update highlight cursor during a drag
	virtual void update_cursor();

// ================================= file operations

	int save(FileXML *xml);
	int load(FileXML *xml, int undo_type);

	int delete_project();        // clear timebar of labels

	int draw();                  // draw everything over
	int samplemovement();
	int refresh_labels();

// ========================================= editing

	int select_region(double position);
	double get_edl_length();

	MWindow *mwindow;
	BC_WindowBase *gui;
	TimelinePane *pane;
	int flip_vertical(int w, int h);
	int delete_arrows();    // for flipping vertical

// Operation started by a buttonpress
	int current_operation;
	LabelGUI *drag_label;

private:
	int get_preview_pixels(int &x1, int &x2);
	int draw_bevel();
	ArrayList<LabelGUI*> labels;
	InPointGUI *in_point;
	OutPointGUI *out_point;
	ArrayList<PresentationGUI*> presentations;



// Records for dragging operations
	double start_position;
	double starting_start_position;
	double starting_end_position;
	double time_per_pixel;
	double edl_length;
	int start_cursor_x;
	int highlighted;
};


#endif
