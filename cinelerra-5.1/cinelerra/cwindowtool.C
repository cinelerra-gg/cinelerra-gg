/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include <stdio.h>
#include <stdint.h>

#include "automation.h"
#include "bccolors.h"
#include "bctimer.h"
#include "clip.h"
#include "condition.h"
#include "cpanel.h"
#include "cplayback.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "edlsession.h"
#include "floatauto.h"
#include "floatautos.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mainsession.h"
#include "mainundo.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "transportque.h"


CWindowTool::CWindowTool(MWindow *mwindow, CWindowGUI *gui)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	tool_gui = 0;
	done = 0;
	current_tool = CWINDOW_NONE;
	set_synchronous(1);
	input_lock = new Condition(0, "CWindowTool::input_lock");
	output_lock = new Condition(1, "CWindowTool::output_lock");
	tool_gui_lock = new Mutex("CWindowTool::tool_gui_lock");
}

CWindowTool::~CWindowTool()
{
	done = 1;
	stop_tool();
	input_lock->unlock();
	Thread::join();
	delete input_lock;
	delete output_lock;
	delete tool_gui_lock;
}

void CWindowTool::start_tool(int operation)
{
	CWindowToolGUI *new_gui = 0;
	int result = 0;

//printf("CWindowTool::start_tool 1\n");
	if(current_tool != operation)
	{
		int previous_tool = current_tool;
		current_tool = operation;
		switch(operation)
		{
			case CWINDOW_EYEDROP:
				new_gui = new CWindowEyedropGUI(mwindow, this);
				break;
			case CWINDOW_CROP:
				new_gui = new CWindowCropGUI(mwindow, this);
				break;
			case CWINDOW_CAMERA:
				new_gui = new CWindowCameraGUI(mwindow, this);
				break;
			case CWINDOW_PROJECTOR:
				new_gui = new CWindowProjectorGUI(mwindow, this);
				break;
			case CWINDOW_MASK:
				new_gui = new CWindowMaskGUI(mwindow, this);
				break;
			case CWINDOW_RULER:
				new_gui = new CWindowRulerGUI(mwindow, this);
				break;
			case CWINDOW_PROTECT:
				mwindow->edl->session->tool_window = 0;
				gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]->update(0);
				// fall thru
			default:
				result = 1;
				stop_tool();
				break;
		}

//printf("CWindowTool::start_tool 1\n");


		if(!result)
		{
			stop_tool();
// Wait for previous tool GUI to finish
			output_lock->lock("CWindowTool::start_tool");
			this->tool_gui = new_gui;
			tool_gui->create_objects();
			if( previous_tool == CWINDOW_PROTECT || previous_tool == CWINDOW_NONE ) {
				mwindow->edl->session->tool_window = 1;
				gui->composite_panel->operation[CWINDOW_TOOL_WINDOW]->update(1);
			}
			update_show_window();

// Signal thread to run next tool GUI
			input_lock->unlock();
		}
//printf("CWindowTool::start_tool 1\n");
	}
	else
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::start_tool");
		tool_gui->update();
		tool_gui->unlock_window();
	}

//printf("CWindowTool::start_tool 2\n");

}


void CWindowTool::stop_tool()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::stop_tool");
		tool_gui->set_done(0);
		tool_gui->unlock_window();
	}
}

void CWindowTool::show_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->show_window();
		tool_gui->unlock_window();
	}
}

void CWindowTool::hide_tool()
{
	if(tool_gui && mwindow->edl->session->tool_window)
	{
		tool_gui->lock_window("CWindowTool::show_tool");
		tool_gui->hide_window();
		tool_gui->unlock_window();
	}
}


void CWindowTool::run()
{
	while(!done)
	{
		input_lock->lock("CWindowTool::run");
		if(!done)
		{
			tool_gui->run_window();
			tool_gui_lock->lock("CWindowTool::run");
			delete tool_gui;
			tool_gui = 0;
			tool_gui_lock->unlock();
		}
		output_lock->unlock();
	}
}

void CWindowTool::update_show_window()
{
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_show_window");

		if(mwindow->edl->session->tool_window)
		{
			tool_gui->update();
			tool_gui->show_window();
		}
		else
			tool_gui->hide_window();
		tool_gui->flush();

		tool_gui->unlock_window();
	}
}

void CWindowTool::raise_window()
{
	if(tool_gui)
	{
		gui->unlock_window();
		tool_gui->lock_window("CWindowTool::raise_window");
		tool_gui->raise_window();
		tool_gui->unlock_window();
		gui->lock_window("CWindowTool::raise_window");
	}
}

void CWindowTool::update_values()
{
	tool_gui_lock->lock("CWindowTool::update_values");
	if(tool_gui)
	{
		tool_gui->lock_window("CWindowTool::update_values");
		tool_gui->update();
		tool_gui->flush();
		tool_gui->unlock_window();
	}
	tool_gui_lock->unlock();
}







CWindowToolGUI::CWindowToolGUI(MWindow *mwindow,
	CWindowTool *thread,
	const char *title,
	int w,
	int h)
 : BC_Window(title,
 	mwindow->session->ctool_x,
	mwindow->session->ctool_y,
	w,
	h,
	w,
	h,
	0,
	0,
	1)
{
	this->mwindow = mwindow;
	this->thread = thread;
	current_operation = 0;
}

CWindowToolGUI::~CWindowToolGUI()
{
}

int CWindowToolGUI::close_event()
{
	hide_window();
	flush();
	mwindow->edl->session->tool_window = 0;
	unlock_window();



	thread->gui->lock_window("CWindowToolGUI::close_event");
	thread->gui->composite_panel->set_operation(mwindow->edl->session->cwindow_operation);
	thread->gui->flush();
	thread->gui->unlock_window();

	lock_window("CWindowToolGUI::close_event");
	return 1;
;}

int CWindowToolGUI::keypress_event()
{
	int result = 0;

	switch( get_keypress() ) {
	case 'w':
	case 'W':
		return close_event();
	case KEY_F1:
	case KEY_F2:
	case KEY_F3:
	case KEY_F4:
	case KEY_F5:
	case KEY_F6:
	case KEY_F7:
	case KEY_F8:
	case KEY_F9:
	case KEY_F10:
	case KEY_F11:
	case KEY_F12:
		resend_event(thread->gui);
		result = 1;
	}

	return result;
}

int CWindowToolGUI::translation_event()
{
	mwindow->session->ctool_x = get_x();
	mwindow->session->ctool_y = get_y();
	return 0;
}






CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, float value, int log_increment = 0)
 : BC_TumbleTextBox(gui, (float)value, (float)-65536, (float)65536, x, y, 100, 3)
{
	this->gui = gui;
	set_log_floatincrement(log_increment);
}

CWindowCoord::CWindowCoord(CWindowToolGUI *gui, int x, int y, int value)
 : BC_TumbleTextBox(gui, (int64_t)value, (int64_t)-65536, (int64_t)65536, x, y, 100, 3)
{
	this->gui = gui;
}
int CWindowCoord::handle_event()
{
	gui->event_caller = this;
	gui->handle_event();
	return 1;
}


CWindowCropOK::CWindowCropOK(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Do it"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}
int CWindowCropOK::handle_event()
{
	mwindow->crop_video();
	return 1;
}


int CWindowCropOK::keypress_event()
{
	if(get_keypress() == 0xd)
	{
		handle_event();
		return 1;
	}
	return 0;
}







CWindowCropGUI::CWindowCropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow,
 	thread,
	_(PROGRAM_NAME ": Crop"),
	330,
	100)
{
}


CWindowCropGUI::~CWindowCropGUI()
{
}

void CWindowCropGUI::create_objects()
{
	int x = 10, y = 10;
	BC_Title *title;

	lock_window("CWindowCropGUI::create_objects");
	int column1 = 0;
	int pad = MAX(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1),
		BC_Title::calculate_h(this, "X")) + 5;
	add_subwindow(title = new BC_Title(x, y, "X1:"));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("W:")));
	column1 = MAX(column1, title->get_w());
	y += pad;
	add_subwindow(new CWindowCropOK(mwindow, thread->tool_gui, x, y));

	x += column1 + 5;
	y = 10;
	x1 = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_x1);
	x1->create_objects();
	y += pad;
	width = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_x2 - mwindow->edl->session->crop_x1);
	width->create_objects();


	x += x1->get_w() + 10;
	y = 10;
	int column2 = 0;
	add_subwindow(title = new BC_Title(x, y, "Y1:"));
	column2 = MAX(column2, title->get_w());
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("H:")));
	column2 = MAX(column2, title->get_w());
	y += pad;

	y = 10;
	x += column2 + 5;
	y1 = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_y1);
	y1->create_objects();
	y += pad;
	height = new CWindowCoord(thread->tool_gui, x, y,
		mwindow->edl->session->crop_y2 - mwindow->edl->session->crop_y1);
	height->create_objects();
	unlock_window();
}

void CWindowCropGUI::handle_event()
{
	int new_x1, new_y1;
	new_x1 = atol(x1->get_text());
	new_y1 = atol(y1->get_text());
	if(new_x1 != mwindow->edl->session->crop_x1)
	{
		mwindow->edl->session->crop_x2 = new_x1 +
			mwindow->edl->session->crop_x2 -
			mwindow->edl->session->crop_x1;
		mwindow->edl->session->crop_x1 = new_x1;
	}
	if(new_y1 != mwindow->edl->session->crop_y1)
	{
		mwindow->edl->session->crop_y2 = new_y1 +
			mwindow->edl->session->crop_y2 -
			mwindow->edl->session->crop_y1;
		mwindow->edl->session->crop_y1 = atol(y1->get_text());
	}
 	mwindow->edl->session->crop_x2 = atol(width->get_text()) +
 		mwindow->edl->session->crop_x1;
 	mwindow->edl->session->crop_y2 = atol(height->get_text()) +
 		mwindow->edl->session->crop_y1;
	update();
	mwindow->cwindow->gui->canvas->redraw(1);
}

void CWindowCropGUI::update()
{
	x1->update((int64_t)mwindow->edl->session->crop_x1);
	y1->update((int64_t)mwindow->edl->session->crop_y1);
	width->update((int64_t)mwindow->edl->session->crop_x2 -
		mwindow->edl->session->crop_x1);
	height->update((int64_t)mwindow->edl->session->crop_y2 -
		mwindow->edl->session->crop_y1);
}


CWindowEyedropGUI::CWindowEyedropGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread, _(PROGRAM_NAME ": Color"), 220, 290)
{
}

CWindowEyedropGUI::~CWindowEyedropGUI()
{
}

void CWindowEyedropGUI::create_objects()
{
	int margin = mwindow->theme->widget_border;
	int x = 10 + margin;
	int y = 10 + margin;
	int x2 = 70, x3 = x2 + 60;
	lock_window("CWindowEyedropGUI::create_objects");
	BC_Title *title0, *title1, *title2, *title3, *title4, *title5, *title6, *title7;
	add_subwindow(title0 = new BC_Title(x, y,_("X,Y:")));
	y += title0->get_h() + margin;
	add_subwindow(title7 = new BC_Title(x, y, _("Radius:")));
	y += BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1) + margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Red:")));
	y += title1->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("Green:")));
	y += title2->get_h() + margin;
	add_subwindow(title3 = new BC_Title(x, y, _("Blue:")));
	y += title3->get_h() + margin;

	add_subwindow(title4 = new BC_Title(x, y, "Y:"));
	y += title4->get_h() + margin;
	add_subwindow(title5 = new BC_Title(x, y, "U:"));
	y += title5->get_h() + margin;
	add_subwindow(title6 = new BC_Title(x, y, "V:"));

	add_subwindow(current = new BC_Title(x2, title0->get_y(), ""));

	radius = new CWindowCoord(this, x2, title7->get_y(),
		mwindow->edl->session->eyedrop_radius);
	radius->create_objects();
	radius->set_boundaries((int64_t)0, (int64_t)255);

	add_subwindow(red = new BC_Title(x2, title1->get_y(), "0"));
	add_subwindow(green = new BC_Title(x2, title2->get_y(), "0"));
	add_subwindow(blue = new BC_Title(x2, title3->get_y(), "0"));
	add_subwindow(rgb_hex = new BC_Title(x3, red->get_y(), "#000000"));

	add_subwindow(this->y = new BC_Title(x2, title4->get_y(), "0"));
	add_subwindow(this->u = new BC_Title(x2, title5->get_y(), "0"));
	add_subwindow(this->v = new BC_Title(x2, title6->get_y(), "0"));
	add_subwindow(yuv_hex = new BC_Title(x3, this->y->get_y(), "#000000"));

	y = title6->get_y() + this->v->get_h() + 2*margin;
	add_subwindow(sample = new BC_SubWindow(x, y, 50, 50));
	y += sample->get_h() + margin;
	add_subwindow(use_max = new CWindowEyedropCheckBox(mwindow, this, x, y));
	update();
	unlock_window();
}

void CWindowEyedropGUI::update()
{
	char string[BCTEXTLEN];
	sprintf(string, "%d, %d",
		thread->gui->eyedrop_x,
		thread->gui->eyedrop_y);
	current->update(string);

	radius->update((int64_t)mwindow->edl->session->eyedrop_radius);

	LocalSession *local_session = mwindow->edl->local_session;
	int use_max = local_session->use_max;
	float r = use_max ? local_session->red_max : local_session->red;
	float g = use_max ? local_session->green_max : local_session->green;
	float b = use_max ? local_session->blue_max : local_session->blue;
	this->red->update(r);
	this->green->update(g);
	this->blue->update(b);

	int rx = 255*r + 0.5;  bclamp(rx,0,255);
	int gx = 255*g + 0.5;  bclamp(gx,0,255);
	int bx = 255*b + 0.5;  bclamp(bx,0,255);
	char rgb_text[BCSTRLEN];
	sprintf(rgb_text, "#%02x%02x%02x", rx, gx, bx);
	rgb_hex->update(rgb_text);
	
	float y, u, v;
	YUV::yuv.rgb_to_yuv_f(r, g, b, y, u, v);
	this->y->update(y);
	this->u->update(u);  u += 0.5;
	this->v->update(v);  v += 0.5;

	int yx = 255*y + 0.5;  bclamp(yx,0,255);
	int ux = 255*u + 0.5;  bclamp(ux,0,255);
	int vx = 255*v + 0.5;  bclamp(vx,0,255);
	char yuv_text[BCSTRLEN];
	sprintf(yuv_text, "#%02x%02x%02x", yx, ux, vx);
	yuv_hex->update(yuv_text);

	int rgb = (rx << 16) | (gx << 8) | (bx << 0);
	sample->set_color(rgb);
	sample->draw_box(0, 0, sample->get_w(), sample->get_h());
	sample->set_color(BLACK);
	sample->draw_rectangle(0, 0, sample->get_w(), sample->get_h());
	sample->flash();
}

void CWindowEyedropGUI::handle_event()
{
	int new_radius = atoi(radius->get_text());
	if(new_radius != mwindow->edl->session->eyedrop_radius)
	{
		CWindowGUI *gui = mwindow->cwindow->gui;
		if(gui->eyedrop_visible)
		{
			gui->lock_window("CWindowEyedropGUI::handle_event");
// hide it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
		}

		mwindow->edl->session->eyedrop_radius = new_radius;

		if(gui->eyedrop_visible)
		{
// draw it
			int rerender;
			gui->canvas->do_eyedrop(rerender, 0, 1);
			gui->unlock_window();
		}
	}
}



/* Buttons to control Keyframe-Curve-Mode for Projector or Camera */

// Configuration for all possible Keyframe Curve Mode toggles
struct _CVD {
	FloatAuto::t_mode mode;
	bool use_camera;
	const char* icon_id;
	const char* tooltip;
};

const _CVD Camera_Crv_Smooth =
	{	FloatAuto::SMOOTH,
		true,
		"tan_smooth",
		N_("\"smooth\" Curve on current Camera Keyframes")
	};
const _CVD Camera_Crv_Linear =
	{	FloatAuto::LINEAR,
		true,
		"tan_linear",
		N_("\"linear\" Curve on current Camera Keyframes")
	};
const _CVD Projector_Crv_Smooth =
	{	FloatAuto::SMOOTH,
		false,
		"tan_smooth",
		N_("\"smooth\" Curve on current Projector Keyframes")
	};
const _CVD Projector_Crv_Linear =
	{	FloatAuto::LINEAR,
		false,
		"tan_linear",
		N_("\"linear\" Curve on current Projector Keyframes")
	};

// Implementation Class für Keyframe Curve Mode buttons
//
// This button reflects the state of the "current" keyframe
// (the nearest keyframe on the left) for all three automation
// lines together. Clicking on this button (re)sets the curve
// mode for the three "current" keyframes simultanously, but
// never creates a new keyframe.
//
class CWindowCurveToggle : public BC_Toggle
{
public:
	CWindowCurveToggle(_CVD mode, MWindow *mwindow, CWindowToolGUI *gui, int x, int y);
	void check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z);
	int handle_event();
private:
	_CVD cfg;
	MWindow *mwindow;
	CWindowToolGUI *gui;
};


CWindowCurveToggle::CWindowCurveToggle(_CVD mode, MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->get_image_set(mode.icon_id), false),
   cfg(mode)
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_(cfg.tooltip));
}

void CWindowCurveToggle::check_toggle_state(FloatAuto *x, FloatAuto *y, FloatAuto *z)
{
// the toggle state is only set to ON if all
// three automation lines have the same curve mode.
// For mixed states the toggle stays off.
	set_value( x->curve_mode == this->cfg.mode &&
	           y->curve_mode == this->cfg.mode &&
	           z->curve_mode == this->cfg.mode
	         ,true // redraw to show new state
	         );
}

int CWindowCurveToggle::handle_event()
{
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track) {
		FloatAuto *x=0, *y=0, *z=0;
		mwindow->cwindow->calculate_affected_autos(track,
			&x, &y, &z, cfg.use_camera, 0,0,0); // don't create new keyframe
		if( x ) x->change_curve_mode(cfg.mode);
		if( y ) y->change_curve_mode(cfg.mode);
		if( z ) z->change_curve_mode(cfg.mode);

		gui->update();
		gui->update_preview();
	}

	return 1;
}


CWindowEyedropCheckBox::CWindowEyedropCheckBox(MWindow *mwindow, 
	CWindowEyedropGUI *gui, int x, int y)
 : BC_CheckBox(x, y, mwindow->edl->local_session->use_max, _("Use maximum"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

int CWindowEyedropCheckBox::handle_event()
{
	mwindow->edl->local_session->use_max = get_value();
	
	gui->update();
	return 1;
}


CWindowCameraGUI::CWindowCameraGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow,
 	thread,
	_(PROGRAM_NAME ": Camera"),
	170,
	170)
{
}
CWindowCameraGUI::~CWindowCameraGUI()
{
}

void CWindowCameraGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0, *y_auto = 0, *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	lock_window("CWindowCameraGUI::create_objects");
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 1, 0, 0, 0);
	}

	add_subwindow(title = new BC_Title(x, y, "X:"));
	x += title->get_w();
	this->x = new CWindowCoord(this, x, y,
		x_auto ? x_auto->get_value() : (float)0);
	this->x->create_objects();


	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	x += title->get_w();
	this->y = new CWindowCoord(this, x, y,
		y_auto ? y_auto->get_value() : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, "Z:"));
	x += title->get_w();
	this->z = new CWindowCoord(this, x, y,
		z_auto ? z_auto->get_value() : (float)1);
	this->z->create_objects();
	this->z->set_increment(0.01);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowCameraLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowCameraTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowCameraBottom(mwindow, this, x1, y));
// additional Buttons to control the curve mode of the "current" keyframe
	x1 += button->get_w() + 15;
	add_subwindow(this->t_smooth = new CWindowCurveToggle(Camera_Crv_Smooth, mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(this->t_linear = new CWindowCurveToggle(Camera_Crv_Linear, mwindow, this, x1, y));

// fill in current auto keyframe values, set toggle states.
	this->update();
	unlock_window();
}

void CWindowCameraGUI::update_preview()
{
	CWindowGUI *cgui = mwindow->cwindow->gui;
	cgui->lock_window("CWindowCameraGUI::update_preview");
	cgui->sync_parameters(CHANGE_PARAMS, 0, 1);
	cgui->unlock_window();
}


void CWindowCameraGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		mwindow->undo->update_undo_before(_("camera"), this);
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_X],
				1);
			if(x_auto)
			{
				x_auto->set_value(atof(x->get_text()));
				update();
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Y],
				1);
			if(y_auto)
			{
				y_auto->set_value(atof(y->get_text()));
				update();
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_CAMERA_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 100.) zoom = 100.;
				else
				if(zoom < 0.01) zoom = 0.01;
	// Doesn't allow user to enter from scratch
	// 		if(zoom != atof(z->get_text()))
	// 			z->update(zoom);

				z_auto->set_value(zoom);
				mwindow->gui->lock_window("CWindowCameraGUI::handle_event");
				mwindow->gui->draw_overlays(1);
				mwindow->gui->unlock_window();
				update();
				update_preview();
			}
		}

		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}
}

void CWindowCameraGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 1, 0, 0, 0);
	}

	if(x_auto)
		x->update(x_auto->get_value());
	if(y_auto)
		y->update(y_auto->get_value());
	if(z_auto) {
		float value = z_auto->get_value();
		z->update(value);
		thread->gui->lock_window("CWindowCameraGUI::update");
		thread->gui->composite_panel->cpanel_zoom->update(value);
		thread->gui->unlock_window();
	}

	if( x_auto && y_auto && z_auto )
	{
		t_smooth->check_toggle_state(x_auto, y_auto, z_auto);
		t_linear->check_toggle_state(x_auto, y_auto, z_auto);
	}
}




CWindowCameraLeft::CWindowCameraLeft(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowCameraLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 1, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->set_value(
				(double)track->track_w / z_auto->get_value() / 2 -
				(double)w / 2);
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
			gui->update();
			gui->update_preview();
		}
	}

	return 1;
}


CWindowCameraCenter::CWindowCameraCenter(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowCameraCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_X],
			1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		x_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraRight::CWindowCameraRight(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowCameraRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 1, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			x_auto->set_value( -((double)track->track_w / z_auto->get_value() / 2 -
				(double)w / 2));
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraTop::CWindowCameraTop(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowCameraTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 1, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->set_value((double)track->track_h / z_auto->get_value() / 2 -
				(double)h / 2);
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowCameraMiddle::CWindowCameraMiddle(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowCameraMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_CAMERA_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("camera"), 0);
		y_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
	}

	return 1;
}


CWindowCameraBottom::CWindowCameraBottom(MWindow *mwindow, CWindowCameraGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowCameraBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 1, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		int w = 0, h = 0;
		track->get_source_dimensions(
			mwindow->edl->local_session->get_selectionstart(1),
			w,
			h);

		if(w && h)
		{
			mwindow->undo->update_undo_before(_("camera"), 0);
			y_auto->set_value(-((double)track->track_h / z_auto->get_value() / 2 -
				(double)h / 2));
			gui->update();
			gui->update_preview();
			mwindow->undo->update_undo_after(_("camera"), LOAD_ALL);
		}
	}

	return 1;
}


CWindowProjectorGUI::CWindowProjectorGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow,
 	thread,
	_(PROGRAM_NAME ": Projector"),
	170,
	170)
{
}
CWindowProjectorGUI::~CWindowProjectorGUI()
{
}
void CWindowProjectorGUI::create_objects()
{
	int x = 10, y = 10, x1;
	Track *track = mwindow->cwindow->calculate_affected_track();
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	BC_Title *title;
	BC_Button *button;

	lock_window("CWindowProjectorGUI::create_objects");
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 0, 0, 0, 0);
	}

	add_subwindow(title = new BC_Title(x, y, "X:"));
	x += title->get_w();
	this->x = new CWindowCoord(this, x, y,
		x_auto ? x_auto->get_value() : (float)0);
	this->x->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	x += title->get_w();
	this->y = new CWindowCoord(this, x, y,
		y_auto ? y_auto->get_value() : (float)0);
	this->y->create_objects();
	y += 30;
	x = 10;
	add_subwindow(title = new BC_Title(x, y, "Z:"));
	x += title->get_w();
	this->z = new CWindowCoord(this, x, y,
		z_auto ? z_auto->get_value() : (float)1);
	this->z->create_objects();
	this->z->set_increment(0.01);

	y += 30;
	x1 = 10;
	add_subwindow(button = new CWindowProjectorLeft(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorCenter(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorRight(mwindow, this, x1, y));

	y += button->get_h();
	x1 = 10;
	add_subwindow(button = new CWindowProjectorTop(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorMiddle(mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(button = new CWindowProjectorBottom(mwindow, this, x1, y));

// additional Buttons to control the curve mode of the "current" keyframe
	x1 += button->get_w() + 15;
	add_subwindow(this->t_smooth = new CWindowCurveToggle(Projector_Crv_Smooth, mwindow, this, x1, y));
	x1 += button->get_w();
	add_subwindow(this->t_linear = new CWindowCurveToggle(Projector_Crv_Linear, mwindow, this, x1, y));

// fill in current auto keyframe values, set toggle states.
	this->update();
	unlock_window();
}

void CWindowProjectorGUI::update_preview()
{
	CWindowGUI *cgui = mwindow->cwindow->gui;
	cgui->lock_window("CWindowProjectorGUI::update_preview");
	cgui->sync_parameters(CHANGE_PARAMS, 0, 1);
	cgui->unlock_window();
}

void CWindowProjectorGUI::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();

	if(track)
	{
		mwindow->undo->update_undo_before(_("projector"), this);
		if(event_caller == x)
		{
			x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_X],
				1);
			if(x_auto)
			{
				x_auto->set_value(atof(x->get_text()));
				update();
				update_preview();
			}
		}
		else
		if(event_caller == y)
		{
			y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Y],
				1);
			if(y_auto)
			{
				y_auto->set_value(atof(y->get_text()));
				update();
				update_preview();
			}
		}
		else
		if(event_caller == z)
		{
			z_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
				track->automation->autos[AUTOMATION_PROJECTOR_Z],
				1);
			if(z_auto)
			{
				float zoom = atof(z->get_text());
				if(zoom > 100.) zoom = 100.;
				else if(zoom < 0.01) zoom = 0.01;
// 			if (zoom != atof(z->get_text()))
// 				z->update(zoom);
				z_auto->set_value(zoom);

				mwindow->gui->lock_window("CWindowProjectorGUI::handle_event");
				mwindow->gui->draw_overlays(1);
				mwindow->gui->unlock_window();

				update();
				update_preview();
			}
		}
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}
}

void CWindowProjectorGUI::update()
{
	FloatAuto *x_auto = 0;
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, &y_auto, &z_auto, 0, 0, 0, 0);
	}

	if(x_auto)
		x->update(x_auto->get_value());
	if(y_auto)
		y->update(y_auto->get_value());
	if(z_auto) {
		float value = z_auto->get_value();
		z->update(value);
		thread->gui->lock_window("CWindowProjectorGUI::update");
		thread->gui->composite_panel->cpanel_zoom->update(value);
		thread->gui->unlock_window();
	}

	if( x_auto && y_auto && z_auto )
	{
		t_smooth->check_toggle_state(x_auto, y_auto, z_auto);
		t_linear->check_toggle_state(x_auto, y_auto, z_auto);
	}
}

CWindowProjectorLeft::CWindowProjectorLeft(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("left_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Left justify"));
}
int CWindowProjectorLeft::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 0, 1, 0, 0);
	}
	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value( (double)track->track_w * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_w / 2 );
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorCenter::CWindowProjectorCenter(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("center_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center horizontal"));
}
int CWindowProjectorCenter::handle_event()
{
	FloatAuto *x_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		x_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_X],
			1);

	if(x_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorRight::CWindowProjectorRight(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("right_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Right justify"));
}
int CWindowProjectorRight::handle_event()
{
	FloatAuto *x_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			&x_auto, 0, &z_auto, 0, 1, 0, 0);
	}

	if(x_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		x_auto->set_value( -((double)track->track_w * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_w / 2));
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorTop::CWindowProjectorTop(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("top_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Top justify"));
}
int CWindowProjectorTop::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 0, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value( (double)track->track_h * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_h / 2 );
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorMiddle::CWindowProjectorMiddle(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("middle_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Center vertical"));
}
int CWindowProjectorMiddle::handle_event()
{
	FloatAuto *y_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track)
		y_auto = (FloatAuto*)mwindow->cwindow->calculate_affected_auto(
			track->automation->autos[AUTOMATION_PROJECTOR_Y], 1);

	if(y_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value(0);
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowProjectorBottom::CWindowProjectorBottom(MWindow *mwindow, CWindowProjectorGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("bottom_justify"))
{
	this->gui = gui;
	this->mwindow = mwindow;
	set_tooltip(_("Bottom justify"));
}
int CWindowProjectorBottom::handle_event()
{
	FloatAuto *y_auto = 0;
	FloatAuto *z_auto = 0;
	Track *track = mwindow->cwindow->calculate_affected_track();
	if( track ) {
		mwindow->cwindow->calculate_affected_autos(track,
			0, &y_auto, &z_auto, 0, 0, 1, 0);
	}

	if(y_auto && z_auto)
	{
		mwindow->undo->update_undo_before(_("projector"), 0);
		y_auto->set_value( -((double)track->track_h * z_auto->get_value() / 2 -
			(double)mwindow->edl->session->output_h / 2));
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("projector"), LOAD_ALL);
	}

	return 1;
}


CWindowMaskName::CWindowMaskName(MWindow *mwindow,
	CWindowToolGUI *gui, int x, int y, const char *text)
 : BC_PopupTextBox(gui, 0, text, x, y, 100, 160)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskName::~CWindowMaskName()
{
}

int CWindowMaskName::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
//printf("CWindowMaskGUI::update 1\n");
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		int k = get_number();
		if( k < 0 ) k = mwindow->edl->session->cwindow_mask;
		else mwindow->edl->session->cwindow_mask = k;
		if( k >= 0 && k < mask_items.size() ) {
			mask_items[k]->set_text(get_text());
			update_list(&mask_items);
		}
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		memset(submask->name, 0, sizeof(submask->name));
		strncpy(submask->name, get_text(), sizeof(submask->name)-1);
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		for(MaskAuto *current = (MaskAuto*)autos->default_auto; current; ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			memset(submask->name, 0, sizeof(submask->name));
			strncpy(submask->name, get_text(), sizeof(submask->name));
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
#endif
	        gui->update();
		gui->update_preview();
	}
	return 1;
}

void CWindowMaskName::update_items(MaskAuto *keyframe)
{
	mask_items.remove_all_objects();
	int sz = keyframe->masks.size();
	for( int i=0; i<sz; ++i ) {
		SubMask *sub_mask = keyframe->masks.get(i);
		char *text = sub_mask->name;
		mask_items.append(new BC_ListBoxItem(text));
	}
	update_list(&mask_items);
}


CWindowMaskDelMask::CWindowMaskDelMask(MWindow *mwindow,
	CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete mask"));
}

int CWindowMaskDelMask::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	int total_points;

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 0);

	if( track ) {
		mwindow->undo->update_undo_before(_("mask delete"), 0);

#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		submask->points.remove_all_objects();
		total_points = 0;
// Commit change to span of keyframes
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		for(MaskAuto *current = (MaskAuto*)autos->default_auto; current; ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			submask->points.clear();
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
		total_points = 0;
#endif
		if( mwindow->cwindow->gui->affected_point >= total_points )
			mwindow->cwindow->gui->affected_point =
				total_points > 0 ? total_points-1 : 0;

		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("mask delete"), LOAD_AUTOMATION);
	}

	return 1;
}

CWindowMaskDelPoint::CWindowMaskDelPoint(MWindow *mwindow,
	CWindowToolGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete point"));
}

int CWindowMaskDelPoint::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
	int total_points;

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		mwindow->undo->update_undo_before(_("point delete"), 0);

#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		SubMask *submask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		int i = mwindow->cwindow->gui->affected_point;
		for( ; i<submask->points.total-1; ++i )
			*submask->points.values[i] = *submask->points.values[i+1];
		if( submask->points.total > 0 ) {
			point = submask->points.values[submask->points.total-1];
			submask->points.remove_object(point);
		}
		total_points = submask->points.total;

// Commit change to span of keyframes
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->update_parameter(&temp_keyframe);
#else
		MaskAuto *current = (MaskAuto*)autos->default_auto;
		while( current ) {
			SubMask *submask = current->get_submask(mwindow->edl->session->cwindow_mask);
			int i = mwindow->cwindow->gui->affected_point;
			for( ; i<submask->points.total-1; ++i ) {
				*submask->points.values[i] = *submask->points.values[i+1];
			if( submask->points.total > 0 ) {
				point = submask->points.values[submask->points.total-1];
				submask->points.remove_object(point);
			}
			total_points = submask->points.total;
			current = current == (MaskAuto*)autos->default_auto ?
				(MaskAuto*)autos->first : (MaskAuto*)NEXT;
		}
#endif
		if( mwindow->cwindow->gui->affected_point >= total_points )
			mwindow->cwindow->gui->affected_point =
				total_points > 0 ? total_points-1 : 0;

		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("mask delete"), LOAD_AUTOMATION);
	}

	return 1;
}

int CWindowMaskDelPoint::keypress_event()
{
	if( get_keypress() == BACKSPACE ||
	    get_keypress() == DELETE )
		return handle_event();
	return 0;
}




CWindowMaskAffectedPoint::CWindowMaskAffectedPoint(MWindow *mwindow,
	CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui,
		(int64_t)mwindow->cwindow->gui->affected_point,
		(int64_t)0, INT64_MAX, x, y, 100)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskAffectedPoint::~CWindowMaskAffectedPoint()
{
}

int CWindowMaskAffectedPoint::handle_event()
{
	int total_points = 0;
	int affected_point = atol(get_text());
	Track *track = mwindow->cwindow->calculate_affected_track();
	if(track) {
		MaskAutos *autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		MaskAuto *keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(autos, 0);
		if( keyframe ) {
			SubMask *mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
			total_points = mask->points.size();
		}
	}
	int active_point = affected_point;
	if( affected_point >= total_points )
		affected_point = total_points - 1;
	else if( affected_point < 0 )
		affected_point = 0;
	if( active_point != affected_point )
		update((int64_t)affected_point);
	mwindow->cwindow->gui->affected_point = affected_point;
	gui->update();
	gui->update_preview();
	return 1;
}


CWindowMaskFocus::CWindowMaskFocus(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_CheckBox(x, y, ((CWindowMaskGUI*)gui)->focused, _("Focus"))
{
	this->mwindow = mwindow;
	this->gui = gui;
}

CWindowMaskFocus::~CWindowMaskFocus()
{
}

int CWindowMaskFocus::handle_event()
{
 	((CWindowMaskGUI*)gui)->focused = get_value();
	gui->update();
	gui->update_preview();
	return 1;
}

CWindowMaskFeather::CWindowMaskFeather(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 0, 0, 0xff, x, y, 64, 2)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFeather::~CWindowMaskFeather()
{
}

int CWindowMaskFeather::update(float v)
{
	CWindowMaskGUI *mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->feather_slider->update(v);
	return BC_TumbleTextBox::update(v);
}

int CWindowMaskFeather::update_value(float v)
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask feather"), this);

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.feather = v;
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->feather = v;
#endif
		gui->update_preview();
	}

	mwindow->undo->update_undo_after(_("mask feather"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskFeather::handle_event()
{
	float v = atof(get_text());
	CWindowMaskGUI * mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->feather_slider->update(v);
	return mask_gui->feather->update_value(v);
}

CWindowMaskFeatherSlider::CWindowMaskFeatherSlider(MWindow *mwindow,
		CWindowToolGUI *gui, int x, int y, int w, float v)
 : BC_FSlider(x, y, 0, w, w, 0.f, 255.f, v)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_precision(0.01);
}

CWindowMaskFeatherSlider::~CWindowMaskFeatherSlider()
{
}

int CWindowMaskFeatherSlider::handle_event()
{
	float v = get_value();
	CWindowMaskGUI * mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->feather->update(v);
	return mask_gui->feather->update_value(v);
}

int CWindowMaskFeatherSlider::update(float v)
{
	return BC_FSlider::update(v);
}

CWindowMaskFade::CWindowMaskFade(MWindow *mwindow, CWindowToolGUI *gui, int x, int y)
 : BC_TumbleTextBox(gui, 0, -100.f, 100.f, x, y, 64, 2)
{
	this->mwindow = mwindow;
	this->gui = gui;
}
CWindowMaskFade::~CWindowMaskFade()
{
}

int CWindowMaskFade::update(float v)
{
	CWindowMaskGUI *mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->fade_slider->update(v);
	return BC_TumbleTextBox::update(v);
}

int CWindowMaskFade::update_value(float v)
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
#ifdef USE_KEYFRAME_SPANNING
	int create_it = 0;
#else
	int create_it = 1;
#endif

	mwindow->undo->update_undo_before(_("mask fade"), this);

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe,
			mask, point, create_it);
	if( track ) {
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.value = v;
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->value = v;
#endif

		gui->update_preview();
	}

	mwindow->undo->update_undo_after(_("mask fade"), LOAD_AUTOMATION);
	return 1;
}

int CWindowMaskFade::handle_event()
{
	float v = atof(get_text());
	CWindowMaskGUI * mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->fade_slider->update(v);
	return mask_gui->fade->update_value(v);
}

CWindowMaskFadeSlider::CWindowMaskFadeSlider(MWindow *mwindow, CWindowToolGUI *gui,
		int x, int y, int w)
 : BC_ISlider(x, y, 0, w, w, -200, 200, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	timer = new Timer();
	stick = 0;
}

CWindowMaskFadeSlider::~CWindowMaskFadeSlider()
{
	delete timer;
}

int CWindowMaskFadeSlider::handle_event()
{
	float v = 100*get_value()/200;
	if( !v && !stick )
		stick = 16;
	else if( stick > 0 ) {
		int64_t ms = timer->get_difference();
		if( ms < 1000 ) {
			--stick;
			if( get_value() == 0 ) return 1;
			update(v = 0);
		}
		else
			stick = 0;
	}
	timer->update();
	CWindowMaskGUI *mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->fade->BC_TumbleTextBox::update(v);
	return mask_gui->fade->update_value(v);
}

int CWindowMaskFadeSlider::update(int64_t v)
{
	return BC_ISlider::update(200*v/100);
}

CWindowMaskMode::CWindowMaskMode(MWindow *mwindow,
	CWindowToolGUI *gui, int x, int y)
 : BC_Toggle(x, y, mwindow->theme->mask_mode_toggle, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Subtract/Multiply Alpha"));
}

CWindowMaskMode::~CWindowMaskMode()
{
}

int CWindowMaskMode::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;
// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 0);
	if( track ) {
		mwindow->undo->update_undo_before(_("mask mode"), 0);
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Update parameter
		temp_keyframe.mode = get_value();
// Commit change to span of keyframes
		autos->update_parameter(&temp_keyframe);
#else
		((MaskAuto*)autos->default_auto)->mode = get_value();
#endif
		mwindow->undo->update_undo_after(_("mask mode"), LOAD_AUTOMATION);
	}

//printf("CWindowMaskMode::handle_event 1\n");
	gui->update_preview();
	return 1;
}

CWindowMaskBeforePlugins::CWindowMaskBeforePlugins(CWindowToolGUI *gui, int x, int y)
 : BC_CheckBox(x,
 	y,
	1,
	_("Apply mask before plugins"))
{
	this->gui = gui;
}

int CWindowMaskBeforePlugins::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 1);

	if (keyframe) {
		int v = get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(gui->mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		temp_keyframe.apply_before_plugins = v;
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->apply_before_plugins = v;
#endif
		gui->update_preview();
	}
	return 1;
}


CWindowDisableOpenGLMasking::CWindowDisableOpenGLMasking(CWindowToolGUI *gui, int x, int y)
 : BC_CheckBox(x, y, 1, _("Disable OpenGL masking"))
{
	this->gui = gui;
}

int CWindowDisableOpenGLMasking::handle_event()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 1);

	if( keyframe ) {
		int v = get_value();
#ifdef USE_KEYFRAME_SPANNING
		MaskAuto temp_keyframe(gui->mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
		temp_keyframe.disable_opengl_masking = v;
		autos->update_parameter(&temp_keyframe);
#else
		keyframe->disable_opengl_masking = v;
#endif
		gui->update_preview();
	}
	return 1;
}


CWindowMaskClrMask::CWindowMaskClrMask(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("reset_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Delete all masks"));
}

CWindowMaskClrMask::~CWindowMaskClrMask()
{
}

int CWindowMaskClrMask::calculate_w(MWindow *mwindow)
{
	VFrame *vfrm = *mwindow->theme->get_image_set("reset_button");
	return vfrm->get_w();
}

int CWindowMaskClrMask::handle_event()
{
	MaskAutos *autos;
	MaskAuto *keyframe;
	Track *track;
	MaskPoint *point;
	SubMask *mask;

// Get existing keyframe
	((CWindowMaskGUI*)gui)->get_keyframe(track, autos, keyframe, mask, point, 0);

	if( track ) {
		mwindow->undo->update_undo_before(_("del masks"), 0);
		((MaskAutos*)track->automation->autos[AUTOMATION_MASK])->clear_all();
		gui->update();
		gui->update_preview();
		mwindow->undo->update_undo_after(_("del masks"), LOAD_AUTOMATION);
	}

	return 1;
}

CWindowMaskClrFeather::CWindowMaskClrFeather(MWindow *mwindow,
		CWindowMaskGUI *gui, int x, int y)
 : BC_Button(x, y, mwindow->theme->get_image_set("reset_button"))
{
	this->mwindow = mwindow;
	this->gui = gui;
	set_tooltip(_("Zero Feather"));
}
CWindowMaskClrFeather::~CWindowMaskClrFeather()
{
}

int CWindowMaskClrFeather::handle_event()
{
	float v = 0;
	CWindowMaskGUI * mask_gui = (CWindowMaskGUI*)gui;
	mask_gui->feather->update(v);
	mask_gui->feather_slider->update(v);
	return mask_gui->feather->update_value(v);
}


CWindowMaskGUI::CWindowMaskGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow, thread,
	_(PROGRAM_NAME ": Mask"), 360, 440)
{
	this->mwindow = mwindow;
	this->thread = thread;
	active_point = 0;
	fade = 0;
	feather = 0;
	focused = 0;
}
CWindowMaskGUI::~CWindowMaskGUI()
{
	lock_window("CWindowMaskGUI::~CWindowMaskGUI");
	delete active_point;
	delete fade;
	delete feather;
	unlock_window();
}

void CWindowMaskGUI::create_objects()
{
	int x = 10, y = 10, margin = mwindow->theme->widget_border;
	int clr_x = get_w()-x - CWindowMaskClrMask::calculate_w(mwindow);
	int del_x = clr_x-margin - CWindowMaskDelMask::calculate_w(this,_("Delete"));
	//MaskAuto *keyframe = 0;
	//Track *track = mwindow->cwindow->calculate_affected_track();
	//if(track)
	//	keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(track->automation->autos[AUTOMATION_MASK], 0);

	lock_window("CWindowMaskGUI::create_objects");
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Mask:")));
	int x1 = x + 70;
	name = new CWindowMaskName(mwindow, this, x1, y, "");
	name->create_objects();
	add_subwindow(clr_mask = new CWindowMaskClrMask(mwindow, this, clr_x, y));
	add_subwindow(del_mask = new CWindowMaskDelMask(mwindow, this, del_x, y));
	y += name->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Fade:")));
	fade = new CWindowMaskFade(mwindow, this, x1, y);
	fade->create_objects();
	int x2 = x1 + fade->get_w() + 2*margin;
	int w2 = clr_x-2*margin - x2;
	add_subwindow(fade_slider = new CWindowMaskFadeSlider(mwindow, this, x2, y, w2));
	add_subwindow(mode = new CWindowMaskMode(mwindow, this, clr_x, y));
	y += fade->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, _("Feather:")));
	feather = new CWindowMaskFeather(mwindow, this, x1, y);
	feather->create_objects();
	x2 = x1 + feather->get_w() + margin;
	w2 = clr_x - 2*margin - x2;
	feather_slider = new CWindowMaskFeatherSlider(mwindow, this, x2, y, w2, 0);
	add_subwindow(feather_slider);
	add_subwindow(new CWindowMaskClrFeather(mwindow, this, clr_x, y));
	y += feather->get_h() + 3*margin;

	add_subwindow(title = new BC_Title(x, y, _("Point:")));
	active_point = new CWindowMaskAffectedPoint(mwindow, this, x1, y);
	active_point->create_objects();
	add_subwindow(del_point = new CWindowMaskDelPoint(mwindow, this, del_x, y));
	y += active_point->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "X:"));
	this->x = new CWindowCoord(this, x1, y, (float)0.0);
	this->x->create_objects();
	y += this->x->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	this->y = new CWindowCoord(this, x1, y, (float)0.0);
	this->y->create_objects();
	y += this->y->get_h() + margin;
	BC_Bar *bar;
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += bar->get_h() + margin;

	add_subwindow(title = new BC_Title(x, y, "X:"));
	focus_x = new CWindowCoord(this, x1, y, (float)0.0);
	focus_x->create_objects();
	add_subwindow(focus = new CWindowMaskFocus(mwindow, this, del_x, y));
	y += focus_x->get_h() + margin;
	add_subwindow(title = new BC_Title(x, y, "Y:"));
	focus_y = new CWindowCoord(this, x1, y, (float)0.0);
	focus_y->create_objects();
	y += focus_x->get_h() + margin;
	add_subwindow(this->apply_before_plugins = new CWindowMaskBeforePlugins(this, 10, y));
	y += this->apply_before_plugins->get_h();
	add_subwindow(this->disable_opengl_masking = new CWindowDisableOpenGLMasking(this, 10, y));
	y += this->disable_opengl_masking->get_h() + margin;
	add_subwindow(bar = new BC_Bar(x, y, get_w()-2*x));
	y += bar->get_h() + margin;

	y += margin;
	add_subwindow(title = new BC_Title(x, y, _(
		"Shift+LMB: move an end point\n"
		"Ctrl+LMB: move a control point\n"
		"Alt+LMB: to drag translate the mask\n"
		"Shift+Key Delete to delete the mask\n"
		"Wheel Up/Dn: rotate around pointer\n"
		"Shift+Wheel Up/Dn: scale around pointer\n"
		"Shift+MMB: Toggle focus center at pointer")));
	update();
	unlock_window();
}

void CWindowMaskGUI::get_keyframe(Track* &track,
	MaskAutos* &autos,
	MaskAuto* &keyframe,
	SubMask* &mask,
	MaskPoint* &point,
	int create_it)
{
	autos = 0;
	keyframe = 0;

	track = mwindow->cwindow->calculate_affected_track();
	if(track)
	{
		autos = (MaskAutos*)track->automation->autos[AUTOMATION_MASK];
		keyframe = (MaskAuto*)mwindow->cwindow->calculate_affected_auto(
			autos,
			create_it);
	}

	if(keyframe)
		mask = keyframe->get_submask(mwindow->edl->session->cwindow_mask);
	else
		mask = 0;

	point = 0;
	if(keyframe)
	{
		if(mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0)
		{
			point = mask->points.values[mwindow->cwindow->gui->affected_point];
		}
	}
}

void CWindowMaskGUI::update()
{
	Track *track;
	MaskAutos *autos;
	MaskAuto *keyframe;
	SubMask *mask;
	MaskPoint *point;
//printf("CWindowMaskGUI::update 1\n");
	get_keyframe(track, autos, keyframe, mask, point, 0);

	double position = mwindow->edl->local_session->get_selectionstart(1);
	position = mwindow->edl->align_to_frame(position, 0);
	if(track)
	{
		int64_t position_i = track->to_units(position, 0);

		if(point)
		{
			x->update(point->x);
			y->update(point->y);
		}

		if(mask)
		{
			feather->update(autos->get_feather(position_i, PLAY_FORWARD));
			fade->update(autos->get_value(position_i, PLAY_FORWARD));
			apply_before_plugins->update(keyframe->apply_before_plugins);
			disable_opengl_masking->update(keyframe->disable_opengl_masking);
		}
	}

//printf("CWindowMaskGUI::update 1\n");
	active_point->update((int64_t)mwindow->cwindow->gui->affected_point);
	const char *text = "";
	if( keyframe ) {
		name->update_items(keyframe);
		int k = mwindow->edl->session->cwindow_mask;
		if( k >= 0 && k < keyframe->masks.size() )
			text = keyframe->masks[k]->name;
	}
	name->update(text);

//printf("CWindowMaskGUI::update 1\n");
	if( track ) {
#ifdef USE_KEYFRAME_SPANNING
		mode->update(keyframe->mode);
#else
		mode->set_text(((MaskAuto*)autos->default_auto)->mode);
#endif
	}
//printf("CWindowMaskGUI::update 2\n");
}

void CWindowMaskGUI::handle_event()
{
	Track *track;
	MaskAuto *keyframe;
	MaskAutos *autos;
	SubMask *mask;
	MaskPoint *point;
	get_keyframe(track, autos, keyframe, mask, point, 0);

	mwindow->undo->update_undo_before(_("mask point"), this);

	if(point)
	{
#ifdef USE_KEYFRAME_SPANNING
// Create temp keyframe
		MaskAuto temp_keyframe(mwindow->edl, autos);
		temp_keyframe.copy_data(keyframe);
// Get affected point in temp keyframe
		mask = temp_keyframe.get_submask(mwindow->edl->session->cwindow_mask);
		if(mwindow->cwindow->gui->affected_point < mask->points.total &&
			mwindow->cwindow->gui->affected_point >= 0)
		{
			point = mask->points.values[mwindow->cwindow->gui->affected_point];
		}

		if(point)
		{
			point->x = atof(x->get_text());
			point->y = atof(y->get_text());
// Commit to spanned keyframes
			autos->update_parameter(&temp_keyframe);
		}
#else
		point->x = atof(x->get_text());
		point->y = atof(y->get_text());
#endif
	}

	update_preview();
	mwindow->undo->update_undo_after(_("mask point"), LOAD_AUTOMATION);
}

void CWindowMaskGUI::update_preview()
{
	unlock_window();
	CWindowGUI *cgui = mwindow->cwindow->gui;
	cgui->lock_window("CWindowMaskGUI::update_preview");
	cgui->sync_parameters(CHANGE_PARAMS, 0, 1);
	cgui->unlock_window();
	lock_window("CWindowMaskGUI::update_preview");
}

void CWindowMaskGUI::set_focused(int v, float cx, float cy)
{
	focus_x->update(cx);
	focus_y->update(cy);
	focus->update(focused = v);
}

CWindowRulerGUI::CWindowRulerGUI(MWindow *mwindow, CWindowTool *thread)
 : CWindowToolGUI(mwindow,
 	thread,
	_(PROGRAM_NAME ": Ruler"),
	320,
	240)
{
}

CWindowRulerGUI::~CWindowRulerGUI()
{
}

void CWindowRulerGUI::create_objects()
{
	int x = 10, y = 10, x1 = 100;
	BC_Title *title;

	lock_window("CWindowRulerGUI::create_objects");
	add_subwindow(title = new BC_Title(x, y, _("Current:")));
	add_subwindow(current = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, _("Point 1:")));
	add_subwindow(point1 = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, _("Point 2:")));
	add_subwindow(point2 = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, _("Deltas:")));
	add_subwindow(deltas = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, _("Distance:")));
	add_subwindow(distance = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 5;
	add_subwindow(title = new BC_Title(x, y, _("Angle:")));
	add_subwindow(angle = new BC_TextBox(x1, y, 200, 1, ""));
	y += title->get_h() + 10;
	char string[BCTEXTLEN];
	sprintf(string,
		 _("Press Ctrl to lock ruler to the\nnearest 45%c%c angle."),
		0xc2, 0xb0); // degrees utf
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	y += title->get_h() + 10;
	sprintf(string, _("Press Alt to translate the ruler."));
	add_subwindow(title = new BC_Title(x,
		y,
		string));
	update();
	unlock_window();
}

void CWindowRulerGUI::update()
{
	char string[BCTEXTLEN];
	int cx = mwindow->session->cwindow_output_x;
	int cy = mwindow->session->cwindow_output_y;
	sprintf(string, "%d, %d", cx, cy);
	current->update(string);
	double x1 = mwindow->edl->session->ruler_x1;
	double y1 = mwindow->edl->session->ruler_y1;
	sprintf(string, "%.0f, %.0f", x1, y1);
	point1->update(string);
	double x2 = mwindow->edl->session->ruler_x2;
	double y2 = mwindow->edl->session->ruler_y2;
	sprintf(string, "%.0f, %.0f", x2, y2);
	point2->update(string);
	double dx = x2 - x1, dy = y2 - y1;
	sprintf(string, "%s%.0f, %s%.0f", (dx>=0? "+":""), dx, (dy>=0? "+":""), dy);
	deltas->update(string);
	double d = sqrt(dx*dx + dy*dy);
	sprintf(string, _("%0.01f pixels"), d);
	distance->update(string);
	double a = d > 0 ? (atan2(-dy, dx) * 180/M_PI) : 0.;
	sprintf(string, "%0.02f %c%c", a, 0xc2, 0xb0);
	angle->update(string);
}

void CWindowRulerGUI::handle_event()
{
}




