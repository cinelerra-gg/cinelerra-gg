#include "bcsignals.h"
#include "cursors.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "panedividers.h"
#include "theme.h"


PaneDivider::PaneDivider(MWindow *mwindow, int x, int y, int length, int is_x)
 : BC_SubWindow(x,
	y,
	is_x ? mwindow->theme->pane_w : length,
	is_x ? length : mwindow->theme->pane_h,
	mwindow->theme->pane_color)
{
	this->mwindow = mwindow;
	this->is_x = is_x;
	button_down = 0;
	is_dragging = 0;
	images[0] = images[1]= images[2] = 0;
	status = BUTTON_UP;
}

PaneDivider::~PaneDivider()
{
	delete images[0];
	delete images[1];
	delete images[2];
}

void PaneDivider::create_objects()
{
	VFrame **image_src;
	if(is_x)
	{
		set_cursor(HSEPARATE_CURSOR, 0, 0);
		image_src = mwindow->theme->get_image_set("xpane");
	}
	else
	{
		set_cursor(VSEPARATE_CURSOR, 0, 0);
		image_src = mwindow->theme->get_image_set("ypane");
	}

	for(int i = 0; i < 3; i++)
	{
		images[i] = new BC_Pixmap(this, image_src[i], PIXMAP_ALPHA);
	}

	draw(0);
}

void PaneDivider::draw(int flush)
{
	if(is_x)
	{
		draw_3segmentv(0,
			0,
			get_h(),
			images[status]);
	}
	else
	{
		draw_3segmenth(0,
			0,
			get_w(),
			images[status]);
	}
	this->flash(flush);
}

void PaneDivider::reposition_window(int x, int y, int length)
{
	BC_SubWindow::reposition_window(x,
		y,
		is_x ? mwindow->theme->pane_w : length,

		is_x ? length : mwindow->theme->pane_h);
}

int PaneDivider::button_press_event()
{
	if(is_event_win())
	{
		origin_x = get_cursor_x();
		origin_y = get_cursor_y();
		button_down = 1;
		status = BUTTON_DOWNHI;
		draw(1);
		return 0;
	}
	return 0;
}

int PaneDivider::button_release_event()
{
	if(button_down)
	{
		button_down = 0;

		if(is_dragging)
		{
			is_dragging = 0;
// might be deleted in here
			mwindow->gui->stop_pane_drag();
			status = BUTTON_UPHI;
			draw(1);
		}

		return 1;
	}
	return 0;
}

int PaneDivider::cursor_motion_event()
{
	if(button_down)
	{
		if(!is_dragging)
		{
			if(is_x && abs(get_cursor_x() - origin_x) > 5)
			{
				is_dragging = 1;
				mwindow->gui->start_x_pane_drag();
			}
			else
			if(!is_x && abs(get_cursor_y() - origin_y) > 5)
			{
				is_dragging = 1;
				mwindow->gui->start_y_pane_drag();
			}
		}
		else
		{
			mwindow->gui->handle_pane_drag();
		}
		return 1;
	}
	return 0;
}


int PaneDivider::cursor_enter_event()
{
	if(is_event_win())
	{
		if(get_button_down())
		{
			status = BUTTON_DOWNHI;
		}
		else
		if(status == BUTTON_UP)
		{
			status = BUTTON_UPHI;
		}

		draw(1);
	}
	return 0;
}

int PaneDivider::cursor_leave_event()
{
	if(status == BUTTON_UPHI)
	{
		status = BUTTON_UP;

		draw(1);


	}
	return 0;
}










