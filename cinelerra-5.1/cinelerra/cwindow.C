
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

#include "autos.h"
#include "bchash.h"
#include "bcsignals.h"
#include "channelinfo.h"
#include "clip.h"
#include "cplayback.h"
#include "ctimebar.h"
#include "ctracking.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "cwindowtool.h"
#include "edl.h"
#include "keys.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "localsession.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mbuttons.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "remotecontrol.h"
#include "theme.h"
#include "track.h"
#include "trackcanvas.h"
#include "tracks.h"
#include "transportque.h"

#include <unistd.h>



CWindow::CWindow(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->playback_engine = 0;
	this->playback_cursor = 0;
	this->gui = 0;
}


CWindow::~CWindow()
{
	if(gui && running()) {
		gui->set_done(0);
	}
	join();
	delete gui;  gui = 0;
	delete playback_engine;
	delete playback_cursor;
}

void CWindow::create_objects()
{
	destination = mwindow->defaults->get("CWINDOW_DESTINATION", 0);


	gui = new CWindowGUI(mwindow, this);

	gui->create_objects();


	playback_engine = new CPlayback(mwindow, this, gui->canvas);


// Start command loop
	playback_engine->create_objects();

	gui->transport->set_engine(playback_engine);

	playback_cursor = new CTracking(mwindow, this);

	playback_cursor->create_objects();

}


void CWindow::show_window()
{
	gui->lock_window("CWindow::show_cwindow");
	gui->show_window();
	gui->raise_window();
	gui->flush();
	gui->unlock_window();

	gui->tool_panel->show_tool();
}

void CWindow::hide_window()
{
	gui->hide_window();
	gui->mwindow->session->show_cwindow = 0;
// Unlock in case MWindow is waiting for it.
	gui->unlock_window();

	gui->tool_panel->hide_tool();

	mwindow->gui->lock_window("CWindowGUI::close_event");
	mwindow->gui->mainmenu->show_cwindow->set_checked(0);
	mwindow->gui->unlock_window();
	mwindow->save_defaults();

	gui->lock_window("CWindow::hide_window");
}


Track* CWindow::calculate_affected_track()
{
	Track* affected_track = 0;
	for(Track *track = mwindow->edl->tracks->first;
		track;
		track = track->next)
	{
		if(track->data_type == TRACK_VIDEO &&
			track->record)
		{
			affected_track = track;
			break;
		}
	}
	return affected_track;
}

Auto* CWindow::calculate_affected_auto(Autos *autos,
		int create, int *created, int redraw)
{
	Auto* affected_auto = 0;
	if( created ) *created = 0;

	if( create ) {
		int total = autos->total();
		affected_auto = autos->get_auto_for_editing(-1, create);

// Got created
		if( total != autos->total() ) {
			if( created ) *created = 1;
			if( redraw ) {
// May have to unlock CWindowGUI here.
				mwindow->gui->lock_window("CWindow::calculate_affected_auto");
				mwindow->gui->draw_overlays(1);
				mwindow->gui->unlock_window();
			}
		}
	}
	else {
		affected_auto = autos->get_prev_auto(PLAY_FORWARD, affected_auto);
	}

	return affected_auto;
}



void CWindow::calculate_affected_autos(Track *track,
		FloatAuto **x_auto, FloatAuto **y_auto, FloatAuto **z_auto,
		int use_camera, int create_x, int create_y, int create_z,
		int redraw)
{
	if( x_auto ) *x_auto = 0;
	if( y_auto ) *y_auto = 0;
	if( z_auto ) *z_auto = 0;
	if( !track ) return;

	int ix = use_camera ? AUTOMATION_CAMERA_X : AUTOMATION_PROJECTOR_X, x_created = 0;
	int iy = use_camera ? AUTOMATION_CAMERA_Y : AUTOMATION_PROJECTOR_Y, y_created = 0;
	int iz = use_camera ? AUTOMATION_CAMERA_Z : AUTOMATION_PROJECTOR_Z, z_created = 0;

	if( x_auto )
		*x_auto = (FloatAuto*) calculate_affected_auto(track->automation->autos[ix],
				create_x, &x_created, redraw);
	if( y_auto )
		*y_auto = (FloatAuto*) calculate_affected_auto(track->automation->autos[iy],
				create_y, &y_created, redraw);
	if( z_auto ) *z_auto = (FloatAuto*) calculate_affected_auto(track->automation->autos[iz],
				create_z, &z_created, redraw);
}

void CWindow::stop_playback(int wait)
{
	playback_engine->stop_playback(wait);
}

void CWindow::run()
{
	gui->run_window();
}

void CWindow::update(int dir, int overlays, int tool_window, int operation, int timebar)
{

	if( dir )
		refresh_frame(CHANGE_NONE, dir);

	gui->lock_window("CWindow::update 2");
// Create tool window
	if( operation )
		gui->set_operation(mwindow->edl->session->cwindow_operation);

// Updated by video device.
	if( overlays && !dir )
		gui->canvas->draw_refresh();

// Update tool parameters
// Never updated by someone else
	if( tool_window || dir )
		gui->update_tool();

	if( timebar )
		gui->timebar->update(1);

	double zoom = !mwindow->edl->session->cwindow_scrollbars ?
		0 :mwindow->edl->session->cwindow_zoom;
	gui->zoom_panel->update(zoom);

	gui->canvas->update_zoom(mwindow->edl->session->cwindow_xscroll,
			mwindow->edl->session->cwindow_yscroll,
			mwindow->edl->session->cwindow_zoom);
	gui->canvas->update_geometry(mwindow->edl,
			mwindow->theme->ccanvas_x, mwindow->theme->ccanvas_y,
			mwindow->theme->ccanvas_w, mwindow->theme->ccanvas_h);

	gui->unlock_window();
}

int CWindow::update_position(double position)
{
	gui->unlock_window();

	playback_engine->interrupt_playback(1);

	position = mwindow->edl->align_to_frame(position, 0);
	position = MAX(0, position);

	mwindow->gui->lock_window("CWindowSlider::handle_event 2");
	mwindow->select_point(position);
	mwindow->gui->unlock_window();

	gui->lock_window("CWindow::update_position 1");
	return 1;
}

void CWindow::refresh_frame(int change_type, EDL *edl, int dir)
{
	mwindow->refresh_mixers(dir);
	playback_engine->refresh_frame(change_type, edl, dir);
}

void CWindow::refresh_frame(int change_type, int dir)
{
	refresh_frame(change_type, mwindow->edl, dir);
}

CWindowRemoteHandler::
CWindowRemoteHandler(RemoteControl *remote_control)
 : RemoteHandler(remote_control->gui, RED)
{
	last_key = -1;
}

CWindowRemoteHandler::
~CWindowRemoteHandler()
{
}

int CWindowRemoteHandler::remote_process_key(RemoteControl *remote_control, int key)
{
	MWindowGUI *mwindow_gui = remote_control->mwindow_gui;
	EDL *edl = mwindow_gui->mwindow->edl;
	if( !edl ) return 0;
	PlayTransport *transport = mwindow_gui->mbuttons->transport;
	if( !transport->get_edl() ) return 0;
	PlaybackEngine *engine = transport->engine;
	double position = engine->get_tracking_position();
	double length = edl->tracks->total_length();
	int next_command = -1, lastkey = last_key;
	last_key = key;

	switch( key ) {
	case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8':
		if( lastkey == 'e' ) {
			mwindow_gui->mwindow->select_asset(key-'1', 1);
			break;
		} // fall through
	case '0': case '9':
		position = length * (key-'0')/10.0;
		break;
	case UP:      position += 60.0;			break;
	case DOWN:    position -= 60.0;			break;
	case LEFT:    position -= 10.0;			break;
	case RIGHT:   position += 10.0;			break;
	case KPSTOP:  next_command = STOP;		break;
	case KPREV:   next_command = NORMAL_REWIND;	break;
	case KPPLAY:  next_command = NORMAL_FWD;	break;
	case KPBACK:  next_command = FAST_REWIND;	break;
	case KPFWRD:  next_command = FAST_FWD;		break;
	case KPRECD:  next_command = SLOW_REWIND;	break;
	case KPAUSE:  next_command = SLOW_FWD;		break;
	case ' ':  next_command = NORMAL_FWD;		break;
	case 'a':  gui->tile_windows(0);		return 1;
	case 'b':  gui->tile_windows(1);		return 1;
	case 'c':  gui->tile_windows(2);		return 1;
#ifdef HAVE_DVB
	case 'd':
		mwindow_gui->channel_info->toggle_scan();
		return 1;
#endif
	case 'e':
		break;
	case 'f': {
		Canvas *canvas = mwindow_gui->mwindow->cwindow->gui->canvas;
		if( !canvas->get_fullscreen() )
			canvas->start_fullscreen();
		else
			canvas->stop_fullscreen();
		return 1; }
	default:
		return -1;
	}

	if( next_command < 0 ) {
		if( position < 0 ) position = 0;
		transport->change_position(position);
	}
	else
		transport->handle_transport(next_command);
	return 1;
}

