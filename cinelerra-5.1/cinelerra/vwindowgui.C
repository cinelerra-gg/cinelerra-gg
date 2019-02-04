
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "arender.h"
#include "asset.h"
#include "assets.h"
#include "awindowgui.h"
#include "awindow.h"
#include "cache.h"
#include "canvas.h"
#include "clip.h"
#include "clipedit.h"
#include "edl.h"
#include "edlsession.h"
#include "filesystem.h"
#include "filexml.h"
#include "fonts.h"
#include "keys.h"
#include "labels.h"
#include "language.h"
#include "localsession.h"
#include "mainclock.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "meterpanel.h"
#include "mwindowgui.h"
#include "mwindow.h"
#include "playtransport.h"
#include "preferences.h"
#include "renderengine.h"
#include "samples.h"
#include "theme.h"
#include "timebar.h"
#include "tracks.h"
#include "transportque.h"
#include "vframe.h"
#include "vplayback.h"
#include "vtimebar.h"
#include "vwindowgui.h"
#include "vwindow.h"




VWindowGUI::VWindowGUI(MWindow *mwindow, VWindow *vwindow)
 : BC_Window(_(PROGRAM_NAME ": Viewer"),
 	mwindow->session->vwindow_x,
	mwindow->session->vwindow_y,
	mwindow->session->vwindow_w,
	mwindow->session->vwindow_h,
	100,
	100,
	1,
	1,
	0) // Hide it
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
	canvas = 0;
	transport = 0;
	edit_panel = 0;
	meters = 0;
//	source = 0;
	strcpy(loaded_title, "");
	highlighted = 0;
}

VWindowGUI::~VWindowGUI()
{
	vwindow->playback_engine->interrupt_playback(1);
	sources.remove_all_objects();
	labels.remove_all_objects();
	delete canvas;
	delete transport;
	delete edit_panel;
	delete meters;
//	delete source;
}

void VWindowGUI::draw_wave()
{
	TransportCommand command;
	command.command = NORMAL_FWD;
	command.get_edl()->copy_all(vwindow->get_edl());
	command.change_type = CHANGE_ALL;
	command.realtime = 0;
	RenderEngine *render_engine = new RenderEngine(0, mwindow->preferences, 0, 0);
	CICache *cache = new CICache(mwindow->preferences);
	render_engine->set_acache(cache);
	render_engine->arm_command(&command);

	double duration = 1.;
	int w = mwindow->edl->session->output_w;
	int h = mwindow->edl->session->output_h;
	VFrame *vframe = new VFrame(w, h, BC_RGB888);
	vframe->clear_frame();
	int sample_rate = mwindow->edl->get_sample_rate();
	int channels = mwindow->edl->session->audio_channels;
	if( channels > 2 ) channels = 2;
	int64_t bfrsz = sample_rate * duration;
	Samples *samples[MAXCHANNELS];
	int ch = 0;
	while( ch < channels ) samples[ch++] = new Samples(bfrsz);
	while( ch < MAXCHANNELS ) samples[ch++] = 0;
	render_engine->arender->process_buffer(samples, bfrsz, 0);

	static int line_colors[2] = { GREEN, YELLOW };
	static int base_colors[2] = { RED, PINK };
	for( int i=channels; --i>=0; ) {
		AssetPicon::draw_wave(vframe, samples[i]->get_data(), bfrsz,
			base_colors[i], line_colors[i]);
	}

	for( int i=channels; --i>=0; ) delete samples[i];
	delete render_engine;
	delete cache;
	delete canvas->refresh_frame;
	canvas->refresh_frame = vframe;
	canvas->draw_refresh(1);
}

void VWindowGUI::change_source(EDL *edl, const char *title)
{
//printf("VWindowGUI::change_source %d\n", __LINE__);

	update_sources(title);
	strcpy(loaded_title, title);
	char string[BCTEXTLEN];
	if(title[0])
		sprintf(string, _(PROGRAM_NAME ": %s"), title);
	else
		sprintf(string, _(PROGRAM_NAME ": Viewer"));

	lock_window("VWindowGUI::change_source");
	canvas->clear();
	if( edl &&
	    !edl->tracks->playable_video_tracks() &&
	    edl->tracks->playable_audio_tracks() )
		draw_wave();
	timebar->update(0);
	set_title(string);
	unlock_window();
}


// Get source list from master EDL
void VWindowGUI::update_sources(const char *title)
{
	lock_window("VWindowGUI::update_sources");
	sources.remove_all_objects();

	for( int i=0; i<mwindow->edl->clips.size(); ++i ) {
		char *clip_title = mwindow->edl->clips.values[i]->local_session->clip_title;
		int exists = 0;

		for( int j=0; !exists && j<sources.size(); ++j ) {
			if( !strcasecmp(sources.values[j]->get_text(), clip_title) )
				exists = 1;
		}

		if( !exists )
			sources.append(new BC_ListBoxItem(clip_title));
	}

	FileSystem fs;
	for( Asset *current=mwindow->edl->assets->first; current; current=NEXT ) {
		char clip_title[BCTEXTLEN];
		fs.extract_name(clip_title, current->path);
		int exists = 0;

		for( int j=0; !exists && j<sources.size(); ++j ) {
			if( !strcasecmp(sources.values[j]->get_text(), clip_title) )
				exists = 1;
		}

		if( !exists )
			sources.append(new BC_ListBoxItem(clip_title));
	}

	unlock_window();
}

void VWindowGUI::create_objects()
{
	lock_window("VWindowGUI::create_objects");
	in_point = 0;
	out_point = 0;
	set_icon(mwindow->theme->get_image("vwindow_icon"));

//printf("VWindowGUI::create_objects 1\n");
	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash(0);

	meters = new VWindowMeters(mwindow,
		this,
		mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		mwindow->theme->vmeter_h);
	meters->create_objects();

//printf("VWindowGUI::create_objects 1\n");
// Requires meters to build
	edit_panel = new VWindowEditing(mwindow, vwindow);
	edit_panel->set_meters(meters);
	edit_panel->create_objects();

//printf("VWindowGUI::create_objects 1\n");
	transport = new VWindowTransport(mwindow,
		this,
		mwindow->theme->vtransport_x,
		mwindow->theme->vtransport_y);
	transport->create_objects();

//printf("VWindowGUI::create_objects 1\n");
//	add_subwindow(fps_title = new BC_Title(mwindow->theme->vedit_x, y, ""));
    add_subwindow(clock = new MainClock(mwindow,
		mwindow->theme->vtime_x,
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w));

	canvas = new VWindowCanvas(mwindow, this);
	canvas->create_objects(mwindow->edl);
	canvas->use_vwindow();


//printf("VWindowGUI::create_objects 1\n");
	add_subwindow(timebar = new VTimeBar(mwindow,
		this,
		mwindow->theme->vtimebar_x,
		mwindow->theme->vtimebar_y,
		mwindow->theme->vtimebar_w,
		mwindow->theme->vtimebar_h));
	timebar->create_objects();
//printf("VWindowGUI::create_objects 2\n");


//printf("VWindowGUI::create_objects 1\n");
// 	source = new VWindowSource(mwindow,
// 		this,
// 		mwindow->theme->vsource_x,
// 		mwindow->theme->vsource_y);
// 	source->create_objects();
	update_sources(_("None"));

//printf("VWindowGUI::create_objects 2\n");
	deactivate();

	show_window();
	unlock_window();
}

int VWindowGUI::resize_event(int w, int h)
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
	mwindow->session->vwindow_w = w;
	mwindow->session->vwindow_h = h;

	mwindow->theme->get_vwindow_sizes(this);
	mwindow->theme->draw_vwindow_bg(this);
	flash(0);

//printf("VWindowGUI::resize_event %d %d\n", __LINE__, mwindow->theme->vedit_y);
	edit_panel->reposition_buttons(mwindow->theme->vedit_x,
		mwindow->theme->vedit_y);

	timebar->resize_event();
	transport->reposition_buttons(mwindow->theme->vtransport_x,
		mwindow->theme->vtransport_y);
	clock->reposition_window(mwindow->theme->vtime_x,
		mwindow->theme->vtime_y,
		mwindow->theme->vtime_w,
		clock->get_h());
	canvas->reposition_window(0,
		mwindow->theme->vcanvas_x,
		mwindow->theme->vcanvas_y,
		mwindow->theme->vcanvas_w,
		mwindow->theme->vcanvas_h);
//printf("VWindowGUI::resize_event %d %d\n", __LINE__, mwindow->theme->vcanvas_x);
// 	source->reposition_window(mwindow->theme->vsource_x,
// 		mwindow->theme->vsource_y);
	meters->reposition_window(mwindow->theme->vmeter_x,
		mwindow->theme->vmeter_y,
		-1,
		mwindow->theme->vmeter_h);

	BC_WindowBase::resize_event(w, h);
	return 1;
}





int VWindowGUI::translation_event()
{
	mwindow->session->vwindow_x = get_x();
	mwindow->session->vwindow_y = get_y();
	return 0;
}

int VWindowGUI::close_event()
{
	hide_window();
	int i = mwindow->vwindows.size();
	while( --i >= 0 && mwindow->vwindows.get(i)->gui != this );
	if( i > 0 ) {
		set_done(0);
		return 1;
	}

	mwindow->session->show_vwindow = 0;
	unlock_window();

	mwindow->gui->lock_window("VWindowGUI::close_event");
	mwindow->gui->mainmenu->show_vwindow->set_checked(0);
	mwindow->gui->unlock_window();

	lock_window("VWindowGUI::close_event");
	mwindow->save_defaults();
	return 1;
}

int VWindowGUI::keypress_event()
{
	int result = 0;
	switch( get_keypress() ) {
	case 'w':
	case 'W':
		close_event();
		result = 1;
		break;
	case 'z':
		mwindow->undo_entry(this);
		break;
	case 'Z':
		mwindow->redo_entry(this);
		break;
	case 'f':
		unlock_window();
		if(mwindow->session->vwindow_fullscreen)
			canvas->stop_fullscreen();
		else
			canvas->start_fullscreen();
		lock_window("VWindowGUI::keypress_event 1");
		break;
	case ESC:
		unlock_window();
		if(mwindow->session->vwindow_fullscreen)
			canvas->stop_fullscreen();
		lock_window("VWindowGUI::keypress_event 2");
		break;
	case KEY_F1:
	case KEY_F2:
	case KEY_F3:
	case KEY_F4:
		if( ctrl_down() && shift_down() ) {
			resend_event(mwindow->gui);
			result = 1;
			break;
		}
	}
	if( !result )
		result = transport->keypress_event();
	return result;
}

int VWindowGUI::button_press_event()
{
	if( vwindow->get_edl() != 0 && canvas->get_canvas() &&
	    mwindow->edl->session->vwindow_click2play &&
	    canvas->get_canvas()->get_cursor_over_window() ) {
		switch( get_buttonpress() ) {
		case LEFT_BUTTON:
			if( !vwindow->playback_engine->is_playing_back ) {
				double length = vwindow->get_edl()->tracks->total_playable_length();
				double position = vwindow->playback_engine->get_tracking_position();
				if( position >= length ) transport->goto_start();
			}
			return transport->forward_play->handle_event();
		case MIDDLE_BUTTON:
			if( !vwindow->playback_engine->is_playing_back ) {
				double position = vwindow->playback_engine->get_tracking_position();
				if( position <= 0 ) transport->goto_end();
			}
			return transport->reverse_play->handle_event();
		case RIGHT_BUTTON:  // activates popup
			break;
		case WHEEL_UP:
			return transport->frame_forward_play->handle_event();
		case WHEEL_DOWN:
			return transport->frame_reverse_play->handle_event();
		}
	}
	if(canvas->get_canvas())
		return canvas->button_press_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::cursor_leave_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_leave_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::cursor_enter_event()
{
	if(canvas->get_canvas())
		return canvas->cursor_enter_event_base(canvas->get_canvas());
	return 0;
}

int VWindowGUI::button_release_event()
{
	if(canvas->get_canvas())
		return canvas->button_release_event();
	return 0;
}

int VWindowGUI::cursor_motion_event()
{
	if(canvas->get_canvas())
	{
		canvas->get_canvas()->unhide_cursor();
		return canvas->cursor_motion_event();
	}
	return 0;
}


void VWindowGUI::drag_motion()
{
// Window hidden
	if(get_hidden()) return;
	if(mwindow->session->current_operation != DRAG_ASSET) return;
	int need_highlight = cursor_above() && get_cursor_over_window() ? 1 : 0;
	if( highlighted == need_highlight ) return;
	highlighted = need_highlight;
	canvas->draw_refresh();
}

int VWindowGUI::drag_stop()
{
	if( get_hidden() ) return 0;

	if( highlighted &&
	    mwindow->session->current_operation == DRAG_ASSET ) {
		highlighted = 0;
		canvas->draw_refresh();
		unlock_window();

		Indexable *indexable =
			mwindow->session->drag_assets->size() > 0 ?
				(Indexable *)mwindow->session->drag_assets->get(0) :
			mwindow->session->drag_clips->size() > 0 ?
				(Indexable *)mwindow->session->drag_clips->get(0) : 0;
		if( indexable )
			vwindow->change_source(indexable);

		lock_window("VWindowGUI::drag_stop");
		return 1;
	}

	return 0;
}

void VWindowGUI::stop_transport()
{
	if( !transport->is_stopped() ) {
		unlock_window();
		transport->handle_transport(STOP, 1, 0, 0);
		lock_window("VWindowGUI::panel_stop_transport");
	}
}

void VWindowGUI::update_meters()
{
	if(mwindow->edl->session->vwindow_meter != meters->visible)
	{
		meters->set_meters(meters->meter_count,
			mwindow->edl->session->vwindow_meter);
		mwindow->theme->get_vwindow_sizes(this);
		resize_event(get_w(), get_h());
	}
}



VWindowMeters::VWindowMeters(MWindow *mwindow,
	VWindowGUI *gui,
	int x,
	int y,
	int h)
 : MeterPanel(mwindow,
		gui,
		x,
		y,
		-1,
		h,
		mwindow->edl->session->audio_channels,
		mwindow->edl->session->vwindow_meter,
		0,
		0)
{
	this->mwindow = mwindow;
	this->gui = gui;
}

VWindowMeters::~VWindowMeters()
{
}

int VWindowMeters::change_status_event(int new_status)
{
	mwindow->edl->session->vwindow_meter = new_status;
	gui->update_meters();
	return 1;
}


VWindowEditing::VWindowEditing(MWindow *mwindow, VWindow *vwindow)
 : EditPanel(mwindow, vwindow->gui, VWINDOW_ID,
		mwindow->theme->vedit_x, mwindow->theme->vedit_y,
		EDITING_ARROW,
		0, // use_editing_mode
		0, // use_keyframe
		1, // use_splice
		1, // use_overwrite
		1, // use_copy
		0, // use_paste
		0, // use_undo
		0, // use_fit
		0, // locklabels
		1, // use_labels
		1, // use_toclip
		1, // use_meters
		0, // use_cut
		0, // use_commerical
		0, // use_goto
		1) // use_clk2play
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

VWindowEditing::~VWindowEditing()
{
}

#define relock_vm(s) \
 vwindow->gui->unlock_window(); \
 mwindow->gui->lock_window("VWindowEditing::" s)
#define relock_mv(s) \
 mwindow->gui->unlock_window(); \
 vwindow->gui->lock_window("VWindowEditing::" s)

double VWindowEditing::get_position()
{
	EDL *edl = vwindow->get_edl();
	double position = !edl ? 0 : edl->local_session->get_selectionstart(1);
	return position;
}

void VWindowEditing::set_position(double position)
{
	EDL *edl = vwindow->get_edl();
	if( !edl ) return;
	if( get_position() != position ) {
		if( position < 0 ) position = 0;
		edl->local_session->set_selectionstart(position);
		edl->local_session->set_selectionend(position);
		vwindow->update_position(CHANGE_NONE, 0, 1);
	}
}

void VWindowEditing::set_click_to_play(int v)
{
	click2play->update(v);
	relock_vm("set_click_to_play");
	mwindow->edl->session->vwindow_click2play = v;
	mwindow->update_vwindow();
	relock_mv("set_click_to_play");
}

void VWindowEditing::panel_stop_transport()
{
	vwindow->gui->stop_transport();
}

void VWindowEditing::panel_toggle_label()
{
	if( !vwindow->get_edl() ) return;
	EDL *edl = vwindow->get_edl();
	edl->labels->toggle_label(edl->local_session->get_selectionstart(1),
		edl->local_session->get_selectionend(1));
	vwindow->gui->timebar->update(1);
}

void VWindowEditing::panel_next_label(int cut)
{
	if( !vwindow->get_edl() ) return;
	vwindow->gui->unlock_window();
	vwindow->playback_engine->interrupt_playback(1);
	vwindow->gui->lock_window("VWindowEditing::next_label");

	EDL *edl = vwindow->get_edl();
	Label *current = edl->labels->next_label(
		edl->local_session->get_selectionstart(1));
	double position = current ? current->position :
		edl->tracks->total_length();
	edl->local_session->set_selectionstart(position);
	edl->local_session->set_selectionend(position);
	vwindow->update_position(CHANGE_NONE, 0, 1, 0);
	vwindow->gui->timebar->update(1);
}

void VWindowEditing::panel_prev_label(int cut)
{
	if( !vwindow->get_edl() ) return;
	vwindow->gui->unlock_window();
	vwindow->playback_engine->interrupt_playback(1);
	vwindow->gui->lock_window("VWindowEditing::prev_label");

	EDL *edl = vwindow->get_edl();
	Label *current = edl->labels->prev_label(
		edl->local_session->get_selectionstart(1));
	double position = !current ? 0 : current->position;
	edl->local_session->set_selectionstart(position);
	edl->local_session->set_selectionend(position);
	vwindow->update_position(CHANGE_NONE, 0, 1, 0);
	vwindow->gui->timebar->update(1);
}

void VWindowEditing::panel_prev_edit(int cut) {} // not used
void VWindowEditing::panel_next_edit(int cut) {} // not used

void VWindowEditing::panel_copy_selection()
{
	vwindow->copy(vwindow->gui->shift_down());
}

void VWindowEditing::panel_overwrite_selection()
{
	if( !vwindow->get_edl() ) return;
	relock_vm("overwrite_selection");
	mwindow->overwrite(vwindow->get_edl(), vwindow->gui->shift_down());
	relock_mv("overwrite_selection");
}

void VWindowEditing::panel_splice_selection()
{
	if( !vwindow->get_edl() ) return;
	relock_vm("splice_selection");
	mwindow->splice(vwindow->get_edl(), vwindow->gui->shift_down());
	relock_mv("splice_selection");
}

void VWindowEditing::panel_set_inpoint()
{
	vwindow->set_inpoint();
}

void VWindowEditing::panel_set_outpoint()
{
	vwindow->set_outpoint();
}

void VWindowEditing::panel_unset_inoutpoint()
{
	vwindow->unset_inoutpoint();
}

void VWindowEditing::panel_to_clip()
{
	EDL *edl = vwindow->get_edl();
	if( !edl ) return;
	mwindow->to_clip(edl, _("viewer window: "), subwindow->shift_down());
}

// not used
void VWindowEditing::panel_cut() {}
void VWindowEditing::panel_paste() {}
void VWindowEditing::panel_fit_selection() {}
void VWindowEditing::panel_fit_autos(int all) {}
void VWindowEditing::panel_set_editing_mode(int mode) {}
void VWindowEditing::panel_set_auto_keyframes(int v) {}
void VWindowEditing::panel_set_labels_follow_edits(int v) {}


VWindowSource::VWindowSource(MWindow *mwindow, VWindowGUI *vwindow, int x, int y)
 : BC_PopupTextBox(vwindow,
 	&vwindow->sources,
	"",
	x,
	y,
	200,
	200)
{
	this->mwindow = mwindow;
	this->vwindow = vwindow;
}

VWindowSource::~VWindowSource()
{
}

int VWindowSource::handle_event()
{
	return 1;
}


VWindowTransport::VWindowTransport(MWindow *mwindow,
	VWindowGUI *gui,
	int x,
	int y)
 : PlayTransport(mwindow,
	gui,
	x,
	y)
{
	this->gui = gui;
}

EDL* VWindowTransport::get_edl()
{
	return gui->vwindow->get_edl();
}


void VWindowTransport::goto_start()
{
	gui->unlock_window();
	handle_transport(REWIND, 1);
	gui->lock_window("VWindowTransport::goto_start");
	gui->vwindow->goto_start();
}

void VWindowTransport::goto_end()
{
	gui->unlock_window();
	handle_transport(GOTO_END, 1);
	gui->lock_window("VWindowTransport::goto_end");
	gui->vwindow->goto_end();
}




VWindowCanvas::VWindowCanvas(MWindow *mwindow, VWindowGUI *gui)
 : Canvas(mwindow,
 	gui,
 	mwindow->theme->vcanvas_x,
	mwindow->theme->vcanvas_y,
	mwindow->theme->vcanvas_w,
	mwindow->theme->vcanvas_h,
	0, 0, 0)
{
//printf("VWindowCanvas::VWindowCanvas %d %d\n", __LINE__, mwindow->theme->vcanvas_x);
	this->mwindow = mwindow;
	this->gui = gui;
}

void VWindowCanvas::zoom_resize_window(float percentage)
{
	EDL *edl = gui->vwindow->get_edl();
	if(!edl) edl = mwindow->edl;

	int canvas_w, canvas_h;
	int new_w, new_h;

// Get required canvas size
	calculate_sizes(edl->get_aspect_ratio(),
		edl->session->output_w,
		edl->session->output_h,
		percentage,
		canvas_w,
		canvas_h);

// Estimate window size from current borders
	new_w = canvas_w + (gui->get_w() - mwindow->theme->vcanvas_w);
	new_h = canvas_h + (gui->get_h() - mwindow->theme->vcanvas_h);

	mwindow->session->vwindow_w = new_w;
	mwindow->session->vwindow_h = new_h;

	mwindow->theme->get_vwindow_sizes(gui);

// Estimate again from new borders
	new_w = canvas_w + (mwindow->session->vwindow_w - mwindow->theme->vcanvas_w);
	new_h = canvas_h + (mwindow->session->vwindow_h - mwindow->theme->vcanvas_h);


	gui->resize_window(new_w, new_h);
	gui->resize_event(new_w, new_h);
}

void VWindowCanvas::close_source()
{
	gui->unlock_window();
	gui->vwindow->playback_engine->interrupt_playback(1);
	gui->lock_window("VWindowCanvas::close_source");
	gui->vwindow->delete_source(1, 1);
}


void VWindowCanvas::draw_refresh(int flush)
{
	EDL *edl = gui->vwindow->get_edl();

	if(!get_canvas()->get_video_on()) get_canvas()->clear_box(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
	if(!get_canvas()->get_video_on() && refresh_frame && edl)
	{
		float in_x1, in_y1, in_x2, in_y2;
		float out_x1, out_y1, out_x2, out_y2;
		get_transfers(edl,
			in_x1, in_y1, in_x2, in_y2,
			out_x1, out_y1, out_x2, out_y2);
// input scaled from session to refresh frame coordinates
		int ow = get_output_w(edl);
		int oh = get_output_h(edl);
		int rw = refresh_frame->get_w();
		int rh = refresh_frame->get_h();
		float xs = (float)rw / ow;
		float ys = (float)rh / oh;
		in_x1 *= xs;  in_x2 *= xs;
		in_y1 *= ys;  in_y2 *= ys;
		get_canvas()->draw_vframe(refresh_frame,
				(int)out_x1,
				(int)out_y1,
				(int)(out_x2 - out_x1),
				(int)(out_y2 - out_y1),
				(int)in_x1,
				(int)in_y1,
				(int)(in_x2 - in_x1),
				(int)(in_y2 - in_y1),
				0);
	}

	if(!get_canvas()->get_video_on())
	{
		draw_overlays();
		get_canvas()->flash(flush);
	}
}

int VWindowCanvas::need_overlays()
{
	if( gui->highlighted ) return 1;
	return 0;
}

void VWindowCanvas::draw_overlays()
{
	if( gui->highlighted )
	{
		get_canvas()->set_color(WHITE);
		get_canvas()->set_inverse();
		get_canvas()->draw_rectangle(0, 0, get_canvas()->get_w(), get_canvas()->get_h());
		get_canvas()->draw_rectangle(1, 1, get_canvas()->get_w() - 2, get_canvas()->get_h() - 2);
		get_canvas()->set_opaque();
	}
}

int VWindowCanvas::get_fullscreen()
{
	return mwindow->session->vwindow_fullscreen;
}

void VWindowCanvas::set_fullscreen(int value)
{
	mwindow->session->vwindow_fullscreen = value;
}

