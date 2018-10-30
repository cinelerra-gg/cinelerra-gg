#ifndef PANEDIVIDERS_H
#define PANEDIVIDERS_H

#include "guicast.h"
#include "mwindow.inc"

class PaneDivider : public BC_SubWindow
{
public:
	PaneDivider(MWindow *mwindow, int x, int y, int length, int is_x);
	~PaneDivider();
	void create_objects();
	void reposition_window(int x, int y, int length);
	int button_press_event();
	int button_release_event();
	int cursor_motion_event();
	int cursor_enter_event();
	int cursor_leave_event();
	void draw(int flush);

// set if it is a vertical line dividing horizontal panes
	int is_x;
	int button_down;
	int is_dragging;
	int origin_x;
	int origin_y;
	int status;
	MWindow *mwindow;
	BC_Pixmap *images[3];
};



#endif
