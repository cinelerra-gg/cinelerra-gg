
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "adeviceprefs.h"
#include "audioconfig.h"
#include "audiodevice.inc"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "language.h"
#include "mainsession.h"
#include "mwindow.h"
#include "overlayframe.inc"
#include "playbackprefs.h"
#include "preferences.h"
#include "theme.h"
#include "vdeviceprefs.h"
#include "videodevice.inc"



PlaybackPrefs::PlaybackPrefs(MWindow *mwindow, PreferencesWindow *pwindow, int config_number)
 : PreferencesDialog(mwindow, pwindow)
{
	this->config_number = config_number;
	audio_device = 0;
	video_device = 0;
	audio_offset = 0;
	play_gain = 0;
}

PlaybackPrefs::~PlaybackPrefs()
{
	delete audio_device;
	delete video_device;
	delete audio_offset;
	delete play_gain;
}

void PlaybackPrefs::create_objects()
{
	int x, y, x2, y2;
	char string[BCTEXTLEN];
	BC_WindowBase *window;


	playback_config = pwindow->thread->edl->session->playback_config;

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;
	int margin = mwindow->theme->widget_border;

// Audio
	BC_Title *title1, *title2;
	add_subwindow(title1 = new BC_Title(x, y, _("Audio Out"), LARGEFONT));
	y += title1->get_h() + margin;
	add_subwindow(title2 = new BC_Title(x, y, _("Playback buffer samples:"), MEDIUMFONT));
	x2 = title2->get_x() + title2->get_w() + margin;

SET_TRACE
	sprintf(string, "%d", playback_config->aconfig->fragment_size);
	PlaybackModuleFragment *menu;
	add_subwindow(menu = new PlaybackModuleFragment(x2, y, pwindow, this, string));
	menu->add_item(new BC_MenuItem("1024"));
	menu->add_item(new BC_MenuItem("2048"));
	menu->add_item(new BC_MenuItem("4096"));
	menu->add_item(new BC_MenuItem("8192"));
	menu->add_item(new BC_MenuItem("16384"));
	menu->add_item(new BC_MenuItem("32768"));
	menu->add_item(new BC_MenuItem("65536"));
	menu->add_item(new BC_MenuItem("131072"));
	menu->add_item(new BC_MenuItem("262144"));

	y += menu->get_h() + 5;
	x2 = x;
	add_subwindow(title1 = new BC_Title(x2, y, _("Audio offset (sec):")));
	x2 += title1->get_w() + 5;
	audio_offset = new PlaybackAudioOffset(pwindow, this, x2, y);
	audio_offset->create_objects();
	y += audio_offset->get_h() + 5;

SET_TRACE
	add_subwindow(new PlaybackViewFollows(pwindow,
		pwindow->thread->edl->session->view_follows_playback, y));
	y += 30;
	add_subwindow(new PlaybackSoftwareTimer(pwindow,
		pwindow->thread->edl->session->playback_software_position, y));
	y += 30;
	add_subwindow(new PlaybackRealTime(pwindow,
		pwindow->thread->edl->session->real_time_playback, y));
	y += 30;
	PlaybackMap51_2 *map51_2 = new PlaybackMap51_2(pwindow, this,
		playback_config->aconfig->map51_2, y);
	add_subwindow(map51_2);
	x2 =  map51_2->get_x() + map51_2->get_w() + 15;
	y2 = y + BC_TextBox::calculate_h(this,MEDIUMFONT,1,1) - get_text_height(MEDIUMFONT);

	add_subwindow(title2 = new BC_Title(x2, y2, _("Gain:")));
	x2 += title2->get_w() + 8;
	play_gain = new PlaybackGain(x2, y, pwindow, this);
	play_gain->create_objects();
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Audio Driver:")));
	audio_device = new ADevicePrefs(x + 100, y, pwindow,
		this, playback_config->aconfig, 0, MODEPLAY);
	audio_device->initialize(0);

SET_TRACE



// Video
 	y += audio_device->get_h(0) + margin;

SET_TRACE
	add_subwindow(new BC_Bar(x, y, 	get_w() - x * 2));
	y += 5;

SET_TRACE
	add_subwindow(title1 = new BC_Title(x, y, _("Video Out"), LARGEFONT));
	y += title1->get_h() + margin;

SET_TRACE
	add_subwindow(window = new VideoEveryFrame(pwindow, this, x, y));
	int x1 = x + window->get_w() + 30;
	const char *txt = _("Framerate achieved:");
	int y1 = y + (window->get_h() - BC_Title::calculate_h(this, txt)) / 2;
	add_subwindow(title1 = new BC_Title(x1, y1, txt));
	x1 += title1->get_w() + margin;
	add_subwindow(framerate_title = new BC_Title(x1, y1, "--", MEDIUMFONT, RED));
	draw_framerate(0);
	y += window->get_h() + 2*margin;

//	add_subwindow(asynchronous = new VideoAsynchronous(pwindow, x, y));
//	y += asynchronous->get_h() + 10;

SET_TRACE
 	add_subwindow(title1 = new BC_Title(x, y, _("Scaling equation: Enlarge / Reduce ")));
	VScalingEquation *vscaling_equation =
		new VScalingEquation(x + title1->get_w() + 65, y,
			&pwindow->thread->edl->session->interpolation_type);
	add_subwindow(vscaling_equation);
	vscaling_equation->create_objects();
SET_TRACE
	y += 35;

	add_subwindow(title1 = new BC_Title(x, y, _("DVD Subtitle to display:")));
	PlaybackSubtitleNumber *subtitle_number;
	x1 = x + title1->get_w() + margin;
	subtitle_number = new PlaybackSubtitleNumber(x1, y, pwindow, this);
	subtitle_number->create_objects();

	x2 = x + title1->get_w() + 10 + subtitle_number->get_w() + 85;
	PlaybackSubtitle *subtitle_toggle;
	x1 += subtitle_number->get_w() + margin;
	add_subwindow(subtitle_toggle = new PlaybackSubtitle(x2, y, pwindow, this));
	y += subtitle_toggle->get_h();

	PlaybackLabelCells *label_cells_toggle;
	add_subwindow(label_cells_toggle = new PlaybackLabelCells(x2, y, pwindow, this));
	y2 = y + label_cells_toggle->get_h();

	add_subwindow(title1=new BC_Title(x2, y2, _("TOC Program No:"), MEDIUMFONT));
	PlaybackProgramNumber *program_number;
	program_number = new PlaybackProgramNumber(
		x2 + title1->get_w() + 10, y2, pwindow, this);
	program_number->create_objects();

	add_subwindow(interpolate_raw = new PlaybackInterpolateRaw( x, y,
		pwindow, this));
	y += interpolate_raw->get_h() + margin;

 	add_subwindow(white_balance_raw = new PlaybackWhiteBalanceRaw(x, y,
		pwindow, this));
 	if(!pwindow->thread->edl->session->interpolate_raw) 
 		white_balance_raw->disable();
	y += white_balance_raw->get_h() + margin;

	add_subwindow(vdevice_title = new BC_Title(x, y, _("Video Driver:")));
	y += vdevice_title->get_h() + margin;
	video_device = new VDevicePrefs(x, y, pwindow, this,
		playback_config->vconfig, 0, MODEPLAY);
	video_device->initialize(0);
}


int PlaybackPrefs::draw_framerate(int flush)
{
//printf("PlaybackPrefs::draw_framerate 1 %f\n", mwindow->session->actual_frame_rate);
	char string[BCTEXTLEN];
	sprintf(string, "%.4f", mwindow->session->actual_frame_rate);
	framerate_title->update(string, flush);
	return 0;
}



PlaybackAudioOffset::PlaybackAudioOffset(PreferencesWindow *pwindow,
	PlaybackPrefs *playback, int x, int y)
 : BC_TumbleTextBox(playback, playback->playback_config->aconfig->audio_offset,
	-10.0, 10.0, x, y, 100)
{
	this->pwindow = pwindow;
	this->playback = playback;
	set_precision(2);
	set_increment(0.1);
}

int PlaybackAudioOffset::handle_event()
{
	playback->playback_config->aconfig->audio_offset = atof(get_text());
	return 1;
}




PlaybackModuleFragment::PlaybackModuleFragment(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback, char *text)
 : BC_PopupMenu(x, y, 100, text, 1)
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackModuleFragment::handle_event()
{
	playback->playback_config->aconfig->fragment_size = atol(get_text());
	return 1;
}


PlaybackViewFollows::PlaybackViewFollows(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("View follows playback"))
{
	this->pwindow = pwindow;
}

int PlaybackViewFollows::handle_event()
{
	pwindow->thread->edl->session->view_follows_playback = get_value();
	return 1;
}


PlaybackSoftwareTimer::PlaybackSoftwareTimer(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Disable hardware synchronization"))
{
	this->pwindow = pwindow;
}

int PlaybackSoftwareTimer::handle_event()
{
	pwindow->thread->edl->session->playback_software_position = get_value();
	return 1;
}


PlaybackRealTime::PlaybackRealTime(PreferencesWindow *pwindow, int value, int y)
 : BC_CheckBox(10, y, value, _("Audio playback in real time priority (root only)"))
{
	this->pwindow = pwindow;
}

int PlaybackRealTime::handle_event()
{
	pwindow->thread->edl->session->real_time_playback = get_value();
	return 1;
}


PlaybackMap51_2::PlaybackMap51_2(PreferencesWindow *pwindow,
		PlaybackPrefs *playback_prefs, int value, int y)
 : BC_CheckBox(10, y, value, _("Map 5.1->2"))
{
	this->pwindow = pwindow;
	this->playback_prefs = playback_prefs;
}

int PlaybackMap51_2::handle_event()
{
	playback_prefs->playback_config->aconfig->map51_2 = get_value();
	return 1;
}


PlaybackInterpolateRaw::PlaybackInterpolateRaw( int x, int y,
		PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->edl->session->interpolate_raw,
	_("Interpolate CR2 images"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackInterpolateRaw::handle_event()
{
	pwindow->thread->edl->session->interpolate_raw = get_value();
	if( !pwindow->thread->edl->session->interpolate_raw ) {
		playback->white_balance_raw->update(0, 0);
		playback->white_balance_raw->disable();
	}
	else {
		playback->white_balance_raw->update(pwindow->thread->edl->session->white_balance_raw, 0);
		playback->white_balance_raw->enable();
	}
	return 1;
}

PlaybackWhiteBalanceRaw::PlaybackWhiteBalanceRaw( int x, int y,
		PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_CheckBox(x,
 	y,
	pwindow->thread->edl->session->interpolate_raw &&
		pwindow->thread->edl->session->white_balance_raw,
	_("White balance CR2 images"))
{
	this->pwindow = pwindow;
	this->playback = playback;
	if(!pwindow->thread->edl->session->interpolate_raw) disable();
}

int PlaybackWhiteBalanceRaw::handle_event()
{
	pwindow->thread->edl->session->white_balance_raw = get_value();
	return 1;
}

// VideoAsynchronous::VideoAsynchronous(PreferencesWindow *pwindow, int x, int y)
//  : BC_CheckBox(x, y,
// 	pwindow->thread->edl->session->video_every_frame &&
// 		pwindow->thread->edl->session->video_asynchronous,
// 	_("Decode frames asynchronously"))
// {
// 	this->pwindow = pwindow;
// 	if(!pwindow->thread->edl->session->video_every_frame)
// 		disable();
// }
// 
// int VideoAsynchronous::handle_event()
// {
// 	pwindow->thread->edl->session->video_asynchronous = get_value();
// 	return 1;
// }


VideoEveryFrame::VideoEveryFrame(PreferencesWindow *pwindow,
	PlaybackPrefs *playback_prefs, int x, int y)
 : BC_CheckBox(x, y, pwindow->thread->edl->session->video_every_frame, _("Play every frame"))
{
	this->pwindow = pwindow;
	this->playback_prefs = playback_prefs;
}

int VideoEveryFrame::handle_event()
{
	pwindow->thread->edl->session->video_every_frame = get_value();
//	if(!pwindow->thread->edl->session->video_every_frame) {
//		playback_prefs->asynchronous->update(0, 0);
//		playback_prefs->asynchronous->disable();
//	}
//	else {
//		playback_prefs->asynchronous->update(pwindow->thread->edl->session->video_asynchronous, 0);
//		playback_prefs->asynchronous->enable();
//	}
	return 1;
}


PlaybackSubtitle::PlaybackSubtitle(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_CheckBox(x, y,
	pwindow->thread->edl->session->decode_subtitles,
	_("Enable subtitles/captioning"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackSubtitle::handle_event()
{
	pwindow->thread->edl->session->decode_subtitles = get_value();
	return 1;
}


PlaybackSubtitleNumber::PlaybackSubtitleNumber(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_TumbleTextBox(playback, pwindow->thread->edl->session->subtitle_number,
 	0, 31, x, y, 50)
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackSubtitleNumber::handle_event()
{
	pwindow->thread->edl->session->subtitle_number = atoi(get_text());
	return 1;
}


PlaybackLabelCells::PlaybackLabelCells(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_CheckBox(x, y,
	pwindow->thread->edl->session->label_cells,
	_("Label cells"))
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackLabelCells::handle_event()
{
	pwindow->thread->edl->session->label_cells = get_value();
	return 1;
}


PlaybackProgramNumber::PlaybackProgramNumber(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_TumbleTextBox(playback,
	pwindow->thread->edl->session->program_no,
 	0, 31, x, y, 50)
{
	this->pwindow = pwindow;
	this->playback = playback;
}

int PlaybackProgramNumber::handle_event()
{
	pwindow->thread->edl->session->program_no = atoi(get_text());
	return 1;
}

PlaybackGain::PlaybackGain(int x, int y,
	PreferencesWindow *pwindow, PlaybackPrefs *playback)
 : BC_TumbleTextBox(playback,
		pwindow->thread->edl->session->playback_config->aconfig->play_gain,
		0.0001f, 10000.0f, x, y, 72)
{
	this->pwindow = pwindow;
	this->set_increment(0.1);
}

int PlaybackGain::handle_event()
{
	pwindow->thread->edl->session->playback_config->
		aconfig->play_gain = atof(get_text());
	return 1;
}

