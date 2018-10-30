#ifndef __DRAGCHECKBOX_H__
#define __DRAGCHECKBOX_H__

#include "bctoggle.h"
#include "mwindow.inc"
#include "track.inc"

class DragCheckBox : public BC_CheckBox
{
public:
	DragCheckBox(MWindow *mwindow, int x, int y, const char *text, int *value,
		float drag_x, float drag_y, float drag_w, float drag_h);
	~DragCheckBox();
	virtual Track *get_drag_track() = 0;
	virtual int64_t get_drag_position() = 0;
	virtual void update_gui() { return; };
	void create_objects();
	void bound();
	static void draw_boundary(VFrame *out, int x, int y, int w, int h);

	int check_pending();
	int drag_activate();
	void drag_deactivate();

	int handle_event();
	int grab_event(XEvent *event);
	
	MWindow *mwindow;
	int grabbed, dragging, pending;
	float drag_x, drag_y;
	float drag_w, drag_h;
	float drag_dx, drag_dy;
};

#endif

