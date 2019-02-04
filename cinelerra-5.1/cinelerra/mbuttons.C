
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

#include "cplayback.h"
#include "cwindow.h"
#include "editpanel.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keys.h"
#include "localsession.h"
#include "mbuttons.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "record.h"
#include "mainsession.h"
#include "theme.h"
#include "tracks.h"

MButtons::MButtons(MWindow *mwindow, MWindowGUI *gui)
 : BC_SubWindow(mwindow->theme->mbuttons_x,
 	mwindow->theme->mbuttons_y,
	mwindow->theme->mbuttons_w,
	mwindow->theme->mbuttons_h)
{
	this->gui = gui;
	this->mwindow = mwindow;
}

MButtons::~MButtons()
{
	delete transport;
	delete edit_panel;
}

void MButtons::create_objects()
{
	int x = 3, y = 0;
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	transport = new MainTransport(mwindow, this, x, y);
	transport->create_objects();
	transport->set_engine(mwindow->cwindow->playback_engine);
	x += transport->get_w();
	x += mwindow->theme->mtransport_margin;

	edit_panel = new MainEditing(mwindow, this, x, y);

	edit_panel->create_objects();

	x += edit_panel->get_w();
	flash(0);
}

int MButtons::resize_event()
{
	reposition_window(mwindow->theme->mbuttons_x,
 		mwindow->theme->mbuttons_y,
		mwindow->theme->mbuttons_w,
		mwindow->theme->mbuttons_h);
	draw_top_background(get_parent(), 0, 0, get_w(), get_h());
	flash(0);
	return 1;
}

int MButtons::keypress_event()
{
	int result = 0;

	if(!result)
	{
		result = transport->keypress_event();
	}

	return result;
}

void MButtons::update()
{
	edit_panel->update();
}


MainTransport::MainTransport(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : PlayTransport(mwindow, mbuttons, x, y)
{
}

void MainTransport::goto_start()
{
	mwindow->gui->unlock_window();
	handle_transport(REWIND, 1);
	mwindow->gui->lock_window();
	mwindow->goto_start();
}


void MainTransport::goto_end()
{
	mwindow->gui->unlock_window();
	handle_transport(GOTO_END, 1);
	mwindow->gui->lock_window();
	mwindow->goto_end();
}

MainEditing::MainEditing(MWindow *mwindow, MButtons *mbuttons, int x, int y)
 : EditPanel(mwindow, mbuttons, MWINDOW_ID, x, y,
		mwindow->edl->session->editing_mode,
		1, // use_editing_mode
		1, // use_keyframe
		0, // use_splice
		0, // use_overwrite
		1, // use_copy
		1, // use_paste
		1, // use_undo
		1, // use_fit
		1, // locklabels
		1, // use_labels
		1, // use_toclip
		0, // use_meters
		1, // use_cut
		mwindow->has_commercials(), // use_commerical
		1, // use_goto
		0) // use_clk2play
{
	this->mwindow = mwindow;
	this->mbuttons = mbuttons;
}

double MainEditing::get_position()
{
	return mwindow->get_position();
}

void MainEditing::set_position(double position)
{
	mwindow->set_position(position);
}

void MainEditing::set_click_to_play(int v)
{
// not used
}

void MainEditing::panel_stop_transport()
{
	mwindow->gui->stop_transport("MainEditing::stop_transport");
}

void MainEditing::panel_toggle_label()
{
	mwindow->toggle_label();
}

void MainEditing::panel_next_label(int cut)
{
	int shift_down = mwindow->gui->shift_down();
	panel_stop_transport();
	if( cut )
		mwindow->cut_right_label();
	else
		mwindow->next_label(shift_down);
}

void MainEditing::panel_prev_label(int cut)
{
	int shift_down = mwindow->gui->shift_down();
	panel_stop_transport();
	if( cut )
		mwindow->cut_left_label();
	else
		mwindow->prev_label(shift_down);
}

void MainEditing::panel_prev_edit(int cut)
{
	int shift_down = subwindow->shift_down();
	panel_stop_transport();
	if( cut )
		mwindow->cut_left_edit();
	else
		mwindow->prev_edit_handle(shift_down);
}

void MainEditing::panel_next_edit(int cut)
{
	int shift_down = subwindow->shift_down();
	panel_stop_transport();
	if( cut )
		mwindow->cut_right_edit();
	else
		mwindow->next_edit_handle(shift_down);
}

void MainEditing::panel_copy_selection()
{
	mwindow->copy();
}

void MainEditing::panel_overwrite_selection() {} // not used
void MainEditing::panel_splice_selection() {} // not used

void MainEditing::panel_set_inpoint()
{
	mwindow->set_inpoint();
}

void MainEditing::panel_set_outpoint()
{
	mwindow->set_outpoint();
}

void MainEditing::panel_unset_inoutpoint()
{
	mwindow->unset_inoutpoint();
}

void MainEditing::panel_to_clip()
{
	MWindowGUI *gui = mwindow->gui;
	gui->unlock_window();
	mwindow->to_clip(mwindow->edl, _("main window: "), 0);
	gui->lock_window("MainEditing::to_clip");
}


void MainEditing::panel_cut()
{
	 mwindow->cut();
}

void MainEditing::panel_paste()
{
	mwindow->paste();
}

void MainEditing::panel_fit_selection()
{
	mwindow->fit_selection();
}

void MainEditing::panel_fit_autos(int all)
{
	mwindow->fit_autos(all);
}

void MainEditing::panel_set_editing_mode(int mode)
{
	mwindow->set_editing_mode(mode);
}

void MainEditing::panel_set_auto_keyframes(int v)
{
	mwindow->set_auto_keyframes(v);
}

void MainEditing::panel_set_labels_follow_edits(int v)
{
        mwindow->set_labels_follow_edits(v);
}

