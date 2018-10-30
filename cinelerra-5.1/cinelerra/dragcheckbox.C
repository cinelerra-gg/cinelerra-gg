#include "automation.h"
#include "bctoggle.h"
#include "canvas.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "dragcheckbox.h"
#include "edl.h"
#include "edlsession.h"
#include "mwindow.h"
#include "theme.h"
#include "track.h"
#include "vframe.h"

DragCheckBox::DragCheckBox(MWindow *mwindow,
	int x, int y, const char *text, int *value,
	float drag_x, float drag_y, float drag_w, float drag_h)
 : BC_CheckBox(x, y, value, text)
{
	this->mwindow = mwindow;
	this->drag_x = drag_x;  this->drag_y = drag_y;
	this->drag_w = drag_w;  this->drag_h = drag_h;
	drag_dx = drag_dy = 0;
	grabbed = 0;
	dragging = 0;
	pending = 0;
}

DragCheckBox::~DragCheckBox()
{
	drag_deactivate();
}

void DragCheckBox::create_objects()
{
	if( !drag_w ) drag_w = get_drag_track()->track_w;
	if( !drag_h ) drag_h = get_drag_track()->track_h;
	if( get_value() )
		drag_activate();
}

int DragCheckBox::handle_event()
{
	int ret = BC_CheckBox::handle_event();
	if( get_value() ) {
		if( drag_activate() ) {
			update(*value=0);
			flicker(10,50);
		}
	}
	else
		drag_deactivate();
	return ret;
}

int DragCheckBox::drag_activate()
{
	int ret = 0;
	if( !grabbed && !(grabbed = grab(mwindow->cwindow->gui)) ) {
		ret = 1;
	}
	pending = 0;
	dragging = 0;
	return ret;
}

void DragCheckBox::drag_deactivate()
{
	if( grabbed ) {
		ungrab(mwindow->cwindow->gui);
		grabbed = 0;
	}
	pending = 0;
	dragging = 0;
}

int DragCheckBox::check_pending()
{
	if( pending && !grab_event_count() ) {
		pending = 0;
		update_gui();
	}
	return 0;
}

int DragCheckBox::grab_event(XEvent *event)
{
	switch( event->type ) {
	case ButtonPress: break;
	case ButtonRelease: break;
	case MotionNotify: break;
	default:
		return check_pending();
	}

	CWindowGUI *cwindow_gui = mwindow->cwindow->gui;
	CWindowCanvas *canvas = cwindow_gui->canvas;
	int cx, cy;  cwindow_gui->get_relative_cursor(cx, cy);
	cx -= mwindow->theme->ccanvas_x;
	cy -= mwindow->theme->ccanvas_y;

	if( !dragging ) {
		if( cx < 0 || cx >= mwindow->theme->ccanvas_w ||
		    cy < 0 || cy >= mwindow->theme->ccanvas_h )
			return check_pending();
	}

	switch( event->type ) {
	case ButtonPress:
		if( !dragging ) break;
		return 1;
	case ButtonRelease:
		if( !dragging ) return check_pending();
		dragging = 0;
		return 1;
	case MotionNotify:
		if( !dragging ) return check_pending();
		break;
	default:
		return check_pending();
	}

	Track *track = get_drag_track();
	if( !track ) return 0;
	if( !drag_w ) drag_w = track->track_w;
	if( !drag_h ) drag_h = track->track_h;

	int64_t position = get_drag_position();

	float cursor_x = cx, cursor_y = cy;
	canvas->canvas_to_output(mwindow->edl, 0, cursor_x, cursor_y);
	float projector_x, projector_y, projector_z;
	int track_w = track->track_w, track_h = track->track_h;
	track->automation->get_projector(
		&projector_x, &projector_y, &projector_z,
		position, PLAY_FORWARD);
	projector_x += mwindow->edl->session->output_w / 2;
	projector_y += mwindow->edl->session->output_h / 2;
	cursor_x = (cursor_x - projector_x) / projector_z + track_w / 2;
	cursor_y = (cursor_y - projector_y) / projector_z + track_h / 2;
	float r = MIN(track_w, track_h)/100.f + 2;
	float x0 = drag_x, x1 = drag_x+(drag_w+1)/2, x2 = drag_x+drag_w;
	float y0 = drag_y, y1 = drag_y+(drag_h+1)/2, y2 = drag_y+drag_h;
	if( !dragging ) {  // clockwise
		     if( fabs(drag_dx = cursor_x-x0) < r &&  // x0,y0
			 fabs(drag_dy = cursor_y-y0) < r ) dragging = 1;
		else if( fabs(drag_dx = cursor_x-x1) < r &&  // x1,y0
			 fabs(drag_dy = cursor_y-y0) < r ) dragging = 2;
		else if( fabs(drag_dx = cursor_x-x2) < r &&  // x2,y0
			 fabs(drag_dy = cursor_y-y0) < r ) dragging = 3;
		else if( fabs(drag_dx = cursor_x-x2) < r &&  // x2,y1
			 fabs(drag_dy = cursor_y-y1) < r ) dragging = 4;
		else if( fabs(drag_dx = cursor_x-x2) < r &&  // x2,y2
			 fabs(drag_dy = cursor_y-y2) < r ) dragging = 5;
		else if( fabs(drag_dx = cursor_x-x1) < r &&  // x1,y2
			 fabs(drag_dy = cursor_y-y2) < r ) dragging = 6;
		else if( fabs(drag_dx = cursor_x-x0) < r &&  // x0,y2
			 fabs(drag_dy = cursor_y-y2) < r ) dragging = 7;
		else if( fabs(drag_dx = cursor_x-x0) < r &&  // x0,y1
			 fabs(drag_dy = cursor_y-y1) < r ) dragging = 8;
		else if( fabs(drag_dx = cursor_x-x1) < r &&  // x1,y1
			 fabs(drag_dy = cursor_y-y1) < r ) dragging = 9;
		else
			return 0;
		return 1;
	}
	int cur_x = cursor_x - drag_dx;
	int cur_y = cursor_y - drag_dy;
	switch( dragging ) {
	case 1: { // x0,y0
		float dx = cur_x - x0;
		float dy = cur_y - y0;
		if( !dx && !dy ) return 1;
		if( (drag_w-=dx) < 1 ) drag_w = 1;
		if( (drag_h-=dy) < 1 ) drag_h = 1;
		drag_x = cur_x;   drag_y = cur_y;
		break; }
	case 2: { // x1,y0
		float dy = cur_y - y0;
		if( !dy ) return 1;
		drag_y = cur_y;
		if( (drag_h-=dy) < 1 ) drag_h = 1;
		break; }
	case 3: { // x2,y0
		float dx = cur_x - x2;
		float dy = cur_y - y0;
		if( (drag_w+=dx) < 1 ) drag_w = 1;
		if( (drag_h-=dy) < 1 ) drag_h = 1;
		drag_y = cur_y;
		break; }
	case 4: { // x2,y1
		float dx = cur_x - x2;
		if( !dx ) return 1;
		if( (drag_w+=dx) < 1 ) drag_w = 1;
		break; }
	case 5: { // x2,y2
		float dx = cur_x - x2;
		float dy = cur_y - y2;
		if( (drag_w+=dx) < 1 ) drag_w = 1;
		 if( (drag_h+=dy) < 1 ) drag_h = 1;
		break; }
	case 6: { // x1,y2
		float dy = cur_y - y2;
		if( !dy ) return 1;
		if( (drag_h+=dy) < 1 ) drag_h = 1;
		break; }
	case 7: { // x0,y2
		float dx = cur_x - x0;
		float dy = cur_y - y2;
		if( (drag_w-=dx) < 1 ) drag_w = 1;
		if( (drag_h+=dy) < 1 ) drag_h = 1;
		drag_x = cur_x;
		break; }
	case 8: { // x0,y1
		float dx = cur_x - x0;
		if( !dx ) return 1;
		if( (drag_w-=dx) < 1 ) drag_w = 1;
		drag_x = cur_x;
		break; }
	case 9: { // x1,y1
		float dx = cur_x - x1;
		float dy = cur_y - y1;
		if( !dx && !dy ) return 1;
		drag_x += dx;
		drag_y += dy;
		}
	}
	if( grab_event_count() )
		pending = 1;
	else if( dragging ) {
		pending = 0;
		update_gui();
	}
	return 1;
}


void DragCheckBox::bound()
{
	Track *track = get_drag_track();
	int trk_w = track->track_w, trk_h = track->track_h;
	float x1 = drag_x, x2 = x1 + drag_w;
	float y1 = drag_y, y2 = y1 + drag_h;
	bclamp(x1, 0, trk_w);  bclamp(x2, 0, trk_w);
	bclamp(y1, 0, trk_h);  bclamp(y2, 0, trk_h);
	if( x1 >= x2 ) { if( x2 > 0 ) x1 = x2-1; else x2 = (x1=0)+1; }
	if( y1 >= y2 ) { if( x2 > 0 ) y1 = y2-1; else y2 = (y1=0)+1; }
	drag_x = x1;  drag_y = y1;  drag_w = x2-x1;  drag_h = y2-y1;
}

void DragCheckBox::draw_boundary(VFrame *out,
	int x, int y, int w, int h)
{
	int iw = out->get_w(), ih = out->get_h();
	int mr = MIN(iw, ih)/200 + 2, rr = 2*mr;
	int r2 = (rr+1)/2;
	int x0 = x-r2, x1 = x+(w+1)/2, x2 = x+w+r2;
	int y0 = y-r2, y1 = y+(h+1)/2, y2 = y+h+r2;
	int st = 1;
	for( int r=2; r<mr; r<<=1 ) st = r;
	out->set_stiple(st);

	for( int r=mr/2; --r>=0; ) { // bbox
		int lft = x+r, rgt = x+w-1-r;
		int top = y+r, bot = y+h-1-r;
		out->draw_line(lft,top, rgt,top);
		out->draw_line(rgt,top, rgt,bot);
		out->draw_line(rgt,bot, lft,bot);
		out->draw_line(lft,bot, lft,top);
	}

	for( int r=mr; r<rr; ++r ) { // center
		out->draw_smooth(x1-r,y1, x1-r,y1+r, x1,y1+r);
		out->draw_smooth(x1,y1+r, x1+r,y1+r, x1+r,y1);
		out->draw_smooth(x1+r,y1, x1+r,y1-r, x1,y1-r);
		out->draw_smooth(x1,y1-r, x1-r,y1-r, x1-r,y1);
	}

	for( int r=rr; --r>=0; ) { // edge arrows
		out->draw_line(x1-r,y0+r, x1+r,y0+r);
		out->draw_line(x2-r,y1-r, x2-r,y1+r);
		out->draw_line(x1-r,y2-r, x1+r,y2-r);
		out->draw_line(x0+r,y1+r, x0+r,y1-r);
	}
	x0 += r2;  y0 += r2;  x2 -= r2;  y2 -= r2;
	for( int r=2*mr; --r>=0; ) { // corner arrows
		out->draw_line(x0,y0+r, x0+r,y0);
		out->draw_line(x2,y0+r, x2-r,y0);
		out->draw_line(x2,y2-r, x2-r,y2);
		out->draw_line(x0,y2-r, x0+r,y2);
	}
	out->set_stiple(0);
}

