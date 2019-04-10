
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

#include "edl.h"
#include "keys.h"
#include "language.h"
#include "localsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playbackengine.h"
#include "playtransport.h"
#include "preferences.h"
#include "shuttle.h"
#include "theme.h"
#include "transportque.h"
#include "vframe.h"



PlayTransport::PlayTransport(MWindow *mwindow,
	BC_WindowBase *subwindow,
	int x,
	int y)
{
	this->subwindow = subwindow;
	this->mwindow = mwindow;
	this->x = x;
	this->y = y;
	this->engine = 0;
	this->status = 0;
	this->using_inout = 0;
}


PlayTransport::~PlayTransport()
{
	delete forward_play;
	delete frame_forward_play;
	delete reverse_play;
	delete frame_reverse_play;
	delete fast_reverse;
	delete fast_play;
	delete rewind_button;
	delete stop_button;
	delete end_button;
}

void PlayTransport::set_engine(PlaybackEngine *engine)
{
	this->engine = engine;
}

int PlayTransport::get_transport_width(MWindow *mwindow)
{
	return mwindow->theme->get_image_set("stop")[0]->get_w() * 7 +
		mwindow->theme->get_image_set("rewind")[0]->get_w() * 2;
}

void PlayTransport::create_objects()
{
	int x = this->x, y = this->y;
	subwindow->add_subwindow(rewind_button = new RewindButton(mwindow, this, x, y));
	x += rewind_button->get_w();
	subwindow->add_subwindow(fast_reverse = new FastReverseButton(mwindow, this, x, y));
	x += fast_reverse->get_w();
	subwindow->add_subwindow(reverse_play = new ReverseButton(mwindow, this, x, y));
	x += reverse_play->get_w();
	subwindow->add_subwindow(frame_reverse_play = new FrameReverseButton(mwindow, this, x, y));
	x += frame_reverse_play->get_w();
	subwindow->add_subwindow(stop_button = new StopButton(mwindow, this, x, y));
	x += stop_button->get_w();
	subwindow->add_subwindow(frame_forward_play = new FramePlayButton(mwindow, this, x, y));
	x += frame_forward_play->get_w();
	subwindow->add_subwindow(forward_play = new PlayButton(mwindow, this, x, y));
	x += forward_play->get_w();
	subwindow->add_subwindow(fast_play = new FastPlayButton(mwindow, this, x, y));
	x += fast_play->get_w();
	subwindow->add_subwindow(end_button = new EndButton(mwindow, this, x, y));
	x += end_button->get_w();

	reverse = 0;
	speed = 0;

}

void PlayTransport::reposition_buttons(int x, int y)
{
	this->x = x;
	this->y = y;
	rewind_button->reposition_window(x, y);
	x += rewind_button->get_w();
	fast_reverse->reposition_window(x, y);
	x += fast_reverse->get_w();
	reverse_play->reposition_window(x, y);
	x += reverse_play->get_w();
	frame_reverse_play->reposition_window(x, y);
	x += frame_reverse_play->get_w();
	stop_button->reposition_window(x, y);
	x += stop_button->get_w();
	frame_forward_play->reposition_window(x, y);
	x += frame_forward_play->get_w();
	forward_play->reposition_window(x, y);
	x += forward_play->get_w();
	fast_play->reposition_window(x, y);
	x += fast_play->get_w();
	end_button->reposition_window(x, y);
	x += end_button->get_w();
}

int PlayTransport::get_w()
{
	return end_button->get_x() + end_button->get_w() - rewind_button->get_x();
}

int PlayTransport::is_stopped()
{
	return engine->is_playing_back ? 0 : 1;
}

int PlayTransport::flip_vertical(int vertical, int &x, int &y)
{
	if(vertical)
	{
		rewind_button->reposition_window(x, y);
		y += rewind_button->get_h();
		fast_reverse->reposition_window(x, y);
		y += fast_reverse->get_h();
		reverse_play->reposition_window(x, y);
		y += reverse_play->get_h();
		frame_reverse_play->reposition_window(x, y);
		y += frame_reverse_play->get_h();
		stop_button->reposition_window(x, y);
		y += stop_button->get_h();
		frame_forward_play->reposition_window(x, y);
		y += frame_forward_play->get_h();
		forward_play->reposition_window(x, y);
		y += forward_play->get_h();
		fast_play->reposition_window(x, y);
		y += fast_play->get_h();
		end_button->reposition_window(x, y);
		y += end_button->get_h();
	}
	else
	{
		rewind_button->reposition_window(x, y - 2);
		x += rewind_button->get_w();
		fast_reverse->reposition_window(x, y - 2);
		x += fast_reverse->get_w();
		reverse_play->reposition_window(x, y - 2);
		x += reverse_play->get_w();
		frame_reverse_play->reposition_window(x, y - 2);
		x += frame_reverse_play->get_w();
		stop_button->reposition_window(x, y - 2);
		x += stop_button->get_w();
		frame_forward_play->reposition_window(x, y - 2);
		x += frame_forward_play->get_w();
		forward_play->reposition_window(x, y - 2);
		x += forward_play->get_w();
		fast_play->reposition_window(x, y - 2);
		x += fast_play->get_w();
		end_button->reposition_window(x, y - 2);
		x += end_button->get_w();
	}

	return 0;
}

int PlayTransport::keypress_event()
{
	int key = subwindow->get_keypress();
	return do_keypress(key);
}

int PlayTransport::do_keypress(int key)
{
	int result = 1;
// unqualified keys, still holding lock
	switch( key ) {
	case HOME:
		goto_start();
		return result;
	case END:
		goto_end();
		return result;
	}

	int ctrl_key = subwindow->ctrl_down() ? 1 : 0;
	int shft_key = subwindow->shift_down() ? 1 : 0;
	int alt_key = subwindow->alt_down() ? 1 : 0;
	int use_inout = ctrl_key;
	int toggle_audio = shft_key & ~ctrl_key;
	int loop_play = shft_key & ctrl_key;
	float speed = 0;
	int command = -1;
	int curr_command = engine->is_playing_back ? engine->command->command : STOP;
	subwindow->unlock_window();

	result = 0;

	if( key >= SKEY_MIN && key <= SKEY_MAX ) {
		speed = SHUTTLE_MAX_SPEED *
			 (key - (SKEY_MAX + SKEY_MIN)/2.f) /
				((SKEY_MAX - SKEY_MIN) / 2.f);
		if( speed < 0 ) {
			speed = -speed;
			command = speed < 1 ? SLOW_REWIND :
				speed > 1 ? FAST_REWIND : NORMAL_REWIND;
		}
		else if( speed > 0 ) {
			command = speed < 1 ? SLOW_FWD :
				speed > 1 ? FAST_FWD : NORMAL_FWD;
		}
		else
			command = STOP;
		use_inout = 0;
		loop_play = 0;
	}
	else switch( key ) {
	case KPINS:	command = STOP;			break;
	case KPPLUS:	command = FAST_REWIND;		break;
	case KP6:	command = NORMAL_REWIND;	break;
	case KP5:	command = SLOW_REWIND;		break;
	case KP4:	command = SINGLE_FRAME_REWIND;	break;
	case KP1:	command = SINGLE_FRAME_FWD;	break;
	case KP2:	command = SLOW_FWD;		break;
	case KP3:	command = NORMAL_FWD;		break;
	case KPENTER:	command = FAST_FWD;		break;

	case ' ':
		switch( curr_command ) {
		case COMMAND_NONE:
		case CURRENT_FRAME:
		case LAST_FRAME:
		case PAUSE:
		case STOP:
			command = NORMAL_FWD;
			break;
		default:
			command = STOP;
			break;
		}
		break;
	case 'u': case 'U':
		if( alt_key ) command = SINGLE_FRAME_REWIND;
		break;
	case 'i': case 'I':
		if( alt_key ) command = SLOW_REWIND;
		break;
	case 'o': case 'O':
		if( alt_key ) command = NORMAL_REWIND;
		break;
	case 'p': case 'P':
		if( alt_key ) command = FAST_REWIND;
		break;
	case 'j': case 'J':
		if( alt_key ) command = SINGLE_FRAME_FWD;
		break;
	case 'k': case 'K':
		if( alt_key ) command = SLOW_FWD;
		break;
	case 'l': case 'L':
		if( alt_key ) command = NORMAL_FWD;
		break;
	case ':': case ';':
		if( alt_key ) command = FAST_FWD;
		break;
	case 'm': case 'M':
		if( alt_key ) command = STOP;
		break;
	}
	if( command >= 0 ) {
		handle_transport(command, 0, use_inout, toggle_audio, loop_play, speed);
		result = 1;
	}

	subwindow->lock_window("PlayTransport::keypress_event 5");
	return result;
}


void PlayTransport::goto_start()
{
	handle_transport(REWIND, 1);
}

void PlayTransport::goto_end()
{
	handle_transport(GOTO_END, 1);
}



void PlayTransport::handle_transport(int command, int wait_tracking,
		int use_inout, int toggle_audio, int loop_play, float speed)
{
	EDL *edl = get_edl();
	if( !edl ) return;
	using_inout = use_inout;

	if( !is_vwindow() )
		mwindow->handle_mixers(edl, command, wait_tracking,
				use_inout, toggle_audio, 0, speed);
	engine->next_command->toggle_audio = toggle_audio;
	engine->next_command->loop_play = loop_play;
	engine->next_command->speed = speed;
	engine->send_command(command, edl, wait_tracking, use_inout);
}

EDL* PlayTransport::get_edl()
{
	return mwindow->edl;
}

int PlayTransport::pause_transport()
{
	if(active_button) active_button->set_mode(PLAY_MODE);
	return 0;
}


int PlayTransport::reset_transport()
{
	fast_reverse->set_mode(PLAY_MODE);
	reverse_play->set_mode(PLAY_MODE);
	forward_play->set_mode(PLAY_MODE);
	frame_reverse_play->set_mode(PLAY_MODE);
	frame_forward_play->set_mode(PLAY_MODE);
	fast_play->set_mode(PLAY_MODE);
	return 0;
}

PTransportButton::PTransportButton(MWindow *mwindow, PlayTransport *transport, int x, int y, VFrame **data)
 : BC_Button(x, y, data)
{
	this->mwindow = mwindow;
	this->transport = transport;
	mode = PLAY_MODE;
}
PTransportButton::~PTransportButton()
{
}

int PTransportButton::set_mode(int mode)
{
	this->mode = mode;
	return 0;
}

int PTransportButton::play_command(const char *lock_msg, int command)
{
	int ctrl_key = transport->subwindow->ctrl_down() ? 1 : 0;
	int shft_key = transport->subwindow->shift_down() ? 1 : 0;
	int use_inout = ctrl_key;
	int toggle_audio = shft_key & ~ctrl_key;
	int loop_play = shft_key & ctrl_key;
	unlock_window();
	transport->handle_transport(command, 0, use_inout, toggle_audio, loop_play, 0);
	lock_window(lock_msg);
	return 1;
}


RewindButton::RewindButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("rewind"))
{
	set_tooltip(_("Rewind ( Home )"));
}
int RewindButton::handle_event()
{
//	unlock_window();
	transport->goto_start();
//	lock_window();
	return 1;
}

FastReverseButton::FastReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("fastrev"))
{
	set_tooltip(_("Fast reverse ( + or Alt-p )"));
}
int FastReverseButton::handle_event()
{
	return play_command("FastReverseButton::handle_event", FAST_REWIND);
}

// Reverse playback normal speed

ReverseButton::ReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("reverse"))
{
	set_tooltip(_("Normal reverse ( 6 or Alt-o )"));
}
int ReverseButton::handle_event()
{
	return play_command("ReverseButton::handle_event", NORMAL_REWIND);
}

// Reverse playback one frame

FrameReverseButton::FrameReverseButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("framerev"))
{
	set_tooltip(_("Frame reverse ( 4 or Alt-u )"));
}
int FrameReverseButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_REWIND, 0, ctrl_down());
	lock_window("FrameReverseButton::handle_event");
	return 1;
}

// forward playback normal speed

PlayButton::PlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("play"))
{
	set_tooltip(_("Normal forward ( 3 or Alt-l )"));
}
int PlayButton::handle_event()
{
	return play_command("PlayButton::handle_event", NORMAL_FWD);
}



// forward playback one frame

FramePlayButton::FramePlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("framefwd"))
{
	set_tooltip(_("Frame forward ( 1 or Alt-j )"));
}
int FramePlayButton::handle_event()
{
	unlock_window();
	transport->handle_transport(SINGLE_FRAME_FWD, 0, ctrl_down());
	lock_window("FramePlayButton::handle_event");
	return 1;
}



FastPlayButton::FastPlayButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("fastfwd"))
{
	set_tooltip(_("Fast forward ( Enter or Alt-; )"));
}
int FastPlayButton::handle_event()
{
	return play_command("FastPlayButton::handle_event", FAST_FWD);
}

EndButton::EndButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("end"))
{
	set_tooltip(_("Jump to end ( End )"));
}
int EndButton::handle_event()
{
//	unlock_window();
	transport->goto_end();
//	lock_window();
	return 1;
}

StopButton::StopButton(MWindow *mwindow, PlayTransport *transport, int x, int y)
 : PTransportButton(mwindow, transport, x, y, mwindow->theme->get_image_set("stop"))
{
	set_tooltip(_("Stop ( 0 or Alt-m )"));
}
int StopButton::handle_event()
{
	unlock_window();
	transport->handle_transport(STOP);
	lock_window("StopButton::handle_event");
	return 1;
}



void PlayTransport::change_position(double position)
{
	if( !get_edl() ) return;
	int command = engine->command->command;
// stop transport
	engine->stop_playback(0);
	mwindow->gui->lock_window("PlayTransport::change_position");
	mwindow->goto_position(position);
	mwindow->gui->unlock_window();
// restart command
	switch( command ) {
	case FAST_REWIND:
	case NORMAL_REWIND:
	case SLOW_REWIND:
	case SLOW_FWD:
	case NORMAL_FWD:
	case FAST_FWD:
		engine->next_command->realtime = 1;
		engine->next_command->resume = 1;
		engine->transport_command(command, CHANGE_NONE, get_edl(), using_inout);
	}
}

