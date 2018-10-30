
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

#include "asset.h"
#include "batch.h"
#include "bcsignals.h"
#include "browsebutton.h"
#include "channel.h"
#include "channelpicker.h"
#include "clip.h"
#include "condition.h"
#include "bchash.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "keys.h"
#include "language.h"
#include "loadmode.h"
#include "meterpanel.h"
#include "mutex.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "preferences.h"
#include "question.h"
#include "recconfirmdelete.h"
#include "recordgui.h"
#include "record.h"
#include "recordlabel.h"
#include "recordmonitor.h"
#include "recordtransport.h"
#include "recordvideo.h"
#include "mainsession.h"
#include "theme.h"
#include "units.h"
#include "videodevice.h"

#include <time.h>




RecordGUI::RecordGUI(MWindow *mwindow, Record *record)
 : BC_Window(_(PROGRAM_NAME ": Recording"),
 	mwindow->session->rwindow_x, mwindow->session->rwindow_y,
	mwindow->session->rwindow_w, mwindow->session->rwindow_h,
	10, 10, 1, 0, 1)
{
	this->mwindow = mwindow;
	this->record = record;

	current_operation = 0;
	position_title = 0;
	prev_label_title = 0;
	frames_behind = 0;
	frames_dropped = 0;
        framerate = 0;
	samples_clipped = 0;
	cron_status = 0;
	batch_bay = 0;
	batch_path = 0;
	status_thread = 0;
	batch_start = 0;
	batch_duration = 0;
	record_transport = 0;
	batch_browse = 0;
	batch_source = 0;
	batch_mode = 0;
	new_batch = 0;
	delete_batch = 0;
	start_batches = 0;
	stop_batches = 0;
	activate_batch = 0;
	label_button = 0;
	drop_frames = 0;
	fill_frames = 0;
	monitor_video = 0;
	monitor_audio = 0;
	meter_audio = 0;
	batch_flash = 0;
	startover_thread = 0;
	interrupt_thread = 0;
	load_mode = 0;
	flash_color = 0;
	loop_hr = 0;
	loop_min = 0;
	loop_sec = 0;
	reset = 0;
	monitor_video_window = 0;
	dc_offset_button = 0;
	for( int i=0; i<MAXCHANNELS; ++i ) {
		dc_offset_text[i] = 0;
		meter[i] = 0;
	}
	total_dropped_frames = 0;
	total_clipped_samples = 0;
}

RecordGUI::~RecordGUI()
{
	delete status_thread;
	delete record_transport;
	delete batch_source;
	delete batch_mode;
	delete batch_flash;
	delete startover_thread;
	delete interrupt_thread;
	delete batch_start;
	delete batch_duration;
	delete load_mode;
}


void RecordGUI::create_objects()
{
	char string[BCTEXTLEN];
	flash_color = RED;
	Asset *asset = record->default_asset;
	lock_window("RecordGUI::create_objects");
	status_thread = new RecordStatusThread(mwindow, this);
	status_thread->start();
	set_icon(mwindow->theme->get_image("record_icon"));

	mwindow->theme->get_recordgui_sizes(this, get_w(), get_h());
//printf("RecordGUI::create_objects 1\n");
	mwindow->theme->draw_rwindow_bg(this);

	int x = 10;
	int y = 10;
	int x1 = 0;
	BC_Title *title;
	int pad = max(BC_TextBox::calculate_h(this, MEDIUMFONT, 1, 1),
		BC_Title::calculate_h(this, "X")) + 5;
	int button_y = 0;

// Current batch
	add_subwindow(title = new BC_Title(x, y, _("Path:")));
	x1 = max(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Start time:")));
	x1 = max(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Duration time:")));
	x1 = max(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Source:")));
	x1 = max(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Mode:")));
	x1 = max(title->get_w(), x1);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Transport:")));
	x1 = max(title->get_w(), x1);
	y += pad;

	button_y = max(y, button_y);



	int x2 = 0;
	y = 10;
	x = x1 + 20;
	add_subwindow(batch_path = new RecordPath(this, x, y));
	add_subwindow(batch_browse = new BrowseButton(mwindow->theme,
		this,
		batch_path,
		batch_path->get_x() + batch_path->get_w(),
		y,
		asset->path,
		_(PROGRAM_NAME ": Record path"),
		_("Select a file to record to:"),
		0));
	x2 = max(x2, batch_path->get_w() + batch_browse->get_w());
	y += pad;
	batch_start = new RecordStart(this, x, y);
	batch_start->create_objects();
	x2 = max(x2, batch_start->get_w());
	y += pad;
	batch_duration = new RecordDuration(this, x, y);
	batch_duration->create_objects();
	x2 = max(x2, batch_duration->get_w());
	y += pad;
	batch_source = new RecordSource(this, x, y);
	batch_source->create_objects();
	x2 = max(x2, batch_source->get_w());
	y += pad;
	batch_mode = new RecordGUIModeMenu(this, x, y, 200, "");
	batch_mode->create_objects();
	x2 = max(x2, batch_mode->get_w());
	y += pad;
	record_transport = new RecordTransport(mwindow,
		record, this, x, y);
	record_transport->create_objects();
	x2 = max(x2, record_transport->get_w());




// Compression settings
	x = x2 + x1 + 30;
	y = 10;
	int x3 = 0;
	pad = BC_Title::calculate_h(this, "X") + 5;
	add_subwindow(title = new BC_Title(x, y, _("Format:")));
	x3 = max(title->get_w(), x3);
	y += pad;

	if(asset->audio_data)
	{
		add_subwindow(title = new BC_Title(x, y, _("Audio compression:")));
		x3 = max(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Samplerate:")));
		x3 = max(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Clipped samples:")));
		x3 = max(title->get_w(), x3);
		y += pad;
	}

	if(asset->video_data)
	{
		add_subwindow(title = new BC_Title(x, y, _("Video compression:")));
		x3 = max(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Framerate:")));
		x3 = max(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Frames dropped:")));
		x3 = max(title->get_w(), x3);
		y += pad;
		add_subwindow(title = new BC_Title(x, y, _("Frames behind:")));
		x3 = max(title->get_w(), x3);
		y += pad;
	}

	add_subwindow(title = new BC_Title(x, y, _("Position:")));
	x3 = max(title->get_w(), x3);
	y += pad;
	add_subwindow(title = new BC_Title(x, y, _("Prev label:")));
	x3 = max(title->get_w(), x3);
	y += pad;

	button_y = max(y, button_y);
	y = 10;
	x = x3 + x2 + x1 + 40;

	add_subwindow(new BC_Title(x, y,
		File::formattostr(asset->format),
		MEDIUMFONT,
		mwindow->theme->recordgui_fixed_color));
	y += pad;

	if(asset->audio_data) {
		add_subwindow(new BC_Title(x, y,
			File::bitstostr(asset->bits),
			MEDIUMFONT,
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		sprintf(string, "%d", asset->sample_rate);
		add_subwindow(new BC_Title(x, y,
			string, MEDIUMFONT,
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		add_subwindow(samples_clipped = new BC_Title(x, y,
			"0", MEDIUMFONT,
			mwindow->theme->recordgui_variable_color));
		y += pad;
	}

	if(asset->video_data) {
		add_subwindow(new BC_Title(x, y,
			asset->format == FILE_MPEG ? _("File Capture") :
				File::compressiontostr(asset->vcodec),
			MEDIUMFONT,
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		sprintf(string, "%0.2f", asset->frame_rate);
		add_subwindow(framerate = new BC_Title(x, y,
			string, MEDIUMFONT,
			mwindow->theme->recordgui_fixed_color));

		y += pad;
		add_subwindow(frames_dropped = new BC_Title(x, y,
			"0", MEDIUMFONT,
			mwindow->theme->recordgui_variable_color));
		y += pad;
		add_subwindow(frames_behind = new BC_Title(x, y,
			"0", MEDIUMFONT,
			mwindow->theme->recordgui_variable_color));
		y += pad;
	}

	add_subwindow(position_title = new BC_Title(x, y,
		"", MEDIUMFONT,
		mwindow->theme->recordgui_variable_color));

	y += pad;
	add_subwindow(prev_label_title = new BC_Title(x, y,
		_("None"), MEDIUMFONT,
		mwindow->theme->recordgui_variable_color));

	y += pad + 10;
	button_y = max(y, button_y);

// Buttons
	x = 10;
	y = button_y;

	add_subwindow(title = new BC_Title(x,y, _("Batches:")));
	int y_max = y + title->get_h();  x1 = x;
	x += title->get_w() + 5;
	add_subwindow(activate_batch = new RecordGUIActivateBatch(this, x, y));
	x += activate_batch->get_w();
	y_max = max(y_max, y + activate_batch->get_h());
	add_subwindow(start_batches = new RecordGUIStartBatches(this, x, y));
	y_max = max(y_max, y + start_batches->get_h());  x2 = x;
	x += start_batches->get_w();
	add_subwindow(new_batch = new RecordGUINewBatch(this, x, y));
	y_max = max(y_max, y + new_batch->get_h());  x3 = x;
	x += new_batch->get_w();  int x4 = x;
	add_subwindow(label_button = new RecordGUILabel(this, x, y));
	y_max = max(y_max, y + label_button->get_h());

	int y1 = y_max, y2 = y1 + 5;
	add_subwindow(title = new BC_Title(x1,y2, _("Cron:")));
	y_max = max(y_max, y2 + title->get_h());
	x1 += title->get_w() + 5;
	add_subwindow(cron_status = new BC_Title(x1,y2, _("Idle"), MEDIUMFONT,
		mwindow->theme->recordgui_variable_color));
	y_max = max(y_max, y2 + cron_status->get_h());
	add_subwindow(stop_batches = new RecordGUIStopBatches(this, x2, y1));
	y_max = max(y_max, y1 + stop_batches->get_h());
	add_subwindow(delete_batch = new RecordGUIDeleteBatch(this, x3, y1));
	y_max = max(y_max, y1 + delete_batch->get_h());
	add_subwindow(clrlbls_button = new RecordGUIClearLabels(this, x4, y1));
	y_max = max(y_max, y1 + clrlbls_button->get_h());

	x = x1 = 10;
	y = y_max + pad;
	y1 = y + pad + 5;

	fill_frames = 0;
	monitor_video = 0;
	monitor_audio = 0;
	meter_audio = 0;
	if(asset->video_data) {
		add_subwindow(drop_frames = new RecordGUIDropFrames(this, x, y));
		add_subwindow(fill_frames = new RecordGUIFillFrames(this, x, y1));
		x += drop_frames->get_w() + 5;  x1 = x;
		add_subwindow(monitor_video = new RecordGUIMonitorVideo(this, x, y));
		x += monitor_video->get_w() + 5;
	}

	if(asset->audio_data) {
		add_subwindow(monitor_audio = new RecordGUIMonitorAudio(this, x, y));
		x += monitor_audio->get_w() + 5;
		add_subwindow(meter_audio = new RecordGUIAudioMeters(this, x, y));
		x += meter_audio->get_w() + 5;
	}

	add_subwindow(power_off = new RecordGUIPowerOff(this, x1, y1));
	x1 += power_off->get_w() + 10;
	add_subwindow(commercial_check = new RecordGUICommCheck(this, x1, y1));

// Batches
	x = 10;
	y += 5;
	if( fill_frames )
		y = y1 + fill_frames->get_h();
	else if( monitor_audio )
		y += monitor_audio->get_h();

	int bottom_margin = max(BC_OKButton::calculate_h(),
		LoadMode::calculate_h(this, mwindow->theme)) + 5;


	add_subwindow(batch_bay = new RecordGUIBatches(this, x, y,
		get_w() - 20, get_h() - y - bottom_margin - 10));
	y += batch_bay->get_h() + 5;
	record->record_batches.gui = batch_bay;
	batch_bay->update_batches(-1);

// Controls
	int loadmode_w = LoadMode::calculate_w(this, mwindow->theme);
	load_mode = new LoadMode(mwindow, this, get_w() / 2 - loadmode_w / 2, y,
		&record->load_mode, 1);
	load_mode->create_objects();
	y += load_mode->get_h() + 5;

	add_subwindow(new RecordGUIOK(this));

	interrupt_thread = new EndRecordThread(this);
//	add_subwindow(new RecordGUISave(record, this));
	add_subwindow(new RecordGUICancel(this));

	startover_thread = new RecordStartoverThread(this);

	enable_batch_buttons();
	if( batch_mode->value == RECORD_TIMED )
		batch_duration->enable();
	else
		batch_duration->disable();
	unlock_window();
}

void RecordGUI::update_batches()
{
	lock_window("void RecordGUI::Update_batches");
	batch_bay->update_batches();
	unlock_window();
}


Batch *RecordGUI::get_current_batch()
{
	return record->get_editing_batch();
}

Batch *RecordGUI::get_editing_batch()
{
	return record->get_editing_batch();
}

void RecordGUI::update_batch_sources()
{
//printf("RecordGUI::update_batch_sources 1\n");
	ChannelPicker *channel_picker =
		record->record_monitor->window->channel_picker;
	if(channel_picker)
		batch_source->update_list(&channel_picker->channel_listitems);
//printf("RecordGUI::update_batch_sources 2\n");
}

int RecordGUI::translation_event()
{
	mwindow->session->rwindow_x = get_x();
	mwindow->session->rwindow_y = get_y();
	return 0;
}


int RecordGUI::resize_event(int w, int h)
{
// Recompute batch list based on previous extents
	int bottom_margin = mwindow->session->rwindow_h -
		batch_bay->get_y() -
		batch_bay->get_h();
	int mode_margin = mwindow->session->rwindow_h - load_mode->get_y();
	mwindow->session->rwindow_x = get_x();
	mwindow->session->rwindow_y = get_y();
	mwindow->session->rwindow_w = w;
	mwindow->session->rwindow_h = h;
	mwindow->theme->get_recordgui_sizes(this, w, h);
	mwindow->theme->draw_rwindow_bg(this);

	int new_h = mwindow->session->rwindow_h - bottom_margin - batch_bay->get_y();
	if(new_h < 10) new_h = 10;
	batch_bay->reposition_window(batch_bay->get_x(),
		batch_bay->get_y(),
		mwindow->session->rwindow_w - 20,
		mwindow->session->rwindow_h - bottom_margin - batch_bay->get_y());

	load_mode->reposition_window(mwindow->session->rwindow_w / 2 -
			mwindow->theme->loadmode_w / 2,
		mwindow->session->rwindow_h - mode_margin);

	flash();
	return 1;
}

void RecordGUI::update_batch_tools()
{
	lock_window("RecordGUI::update_batch_tools");
//printf("RecordGUI::update_batch_tools 1\n");
	Batch *batch = get_editing_batch();
	batch_path->update(batch->asset->path);

// File is open in editing batch
// 	if(current_batch() == editing_batch() && record->file)
// 		batch_path->disable();
// 	else
// 		batch_path->enable();

	batch_start->update(&batch->start_day, &batch->start_time);
	batch_duration->update(0, &batch->duration);
	batch_source->update(batch->get_source_text());
	batch_mode->update(batch->record_mode);
	if( batch_mode->value == RECORD_TIMED )
		batch_duration->enable();
	else
		batch_duration->disable();
	flush();
	unlock_window();
}

void RecordGUI::enable_batch_buttons()
{
	lock_window("RecordGUI::enable_batch_buttons");
	new_batch->enable();
	delete_batch->enable();
	start_batches->enable();
	stop_batches->disable();
	activate_batch->enable();
	unlock_window();
}

void RecordGUI::disable_batch_buttons()
{
	lock_window("RecordGUI::disable_batch_buttons");
	new_batch->disable();
	delete_batch->disable();
	start_batches->disable();
	stop_batches->enable();
	activate_batch->disable();
	unlock_window();
}

RecordGUIBatches::RecordGUIBatches(RecordGUI *gui, int x, int y, int w, int h)
 : RecordBatchesGUI(gui->record->record_batches, x, y, w, h)
{
	this->gui = gui;
}

// Do nothing for double clicks to protect active batch
int RecordGUIBatches::handle_event()
{
	return 1;
}

int RecordGUIBatches::selection_changed()
{
	RecordBatchesGUI::selection_changed();
	gui->update_batch_tools();
	return 1;
}


RecordGUISave::RecordGUISave(RecordGUI *gui)
 : BC_Button(10,
	gui->get_h() - BC_WindowBase::get_resources()->ok_images[0]->get_h() - 10,
	BC_WindowBase::get_resources()->ok_images)
{
	set_tooltip(_("Save the recording and quit."));
	this->gui = gui;
}

int RecordGUISave::handle_event()
{
	gui->set_done(0);
	return 1;
}

int RecordGUISave::keypress_event()
{
// 	if(get_keypress() == RETURN)
// 	{
// 		handle_event();
// 		return 1;
// 	}
	return 0;
}

RecordGUICancel::RecordGUICancel(RecordGUI *gui)
 : BC_CancelButton(gui)
{
	set_tooltip(_("Quit without pasting into project."));
	this->gui = gui;
}

int RecordGUICancel::handle_event()
{
	gui->interrupt_thread->start(0);
	return 1;
}

int RecordGUICancel::keypress_event()
{
	if(get_keypress() == ESC)
	{
		handle_event();
		return 1;
	}

	return 0;
}


RecordGUIOK::RecordGUIOK(RecordGUI *gui)
 : BC_OKButton(gui)
{
	set_tooltip(_("Quit and paste into project."));
	this->gui = gui;
}

int RecordGUIOK::handle_event()
{
	gui->interrupt_thread->start(1);
	return 1;
}


RecordGUIStartOver::RecordGUIStartOver(RecordGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Start Over"))
{
	set_tooltip(_("Rewind the current file and erase."));
	this->gui = gui;
}
RecordGUIStartOver::~RecordGUIStartOver()
{
}

int RecordGUIStartOver::handle_event()
{
	if(!gui->startover_thread->running())
		gui->startover_thread->start();
	return 1;
}


RecordGUIDropFrames::RecordGUIDropFrames(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->drop_overrun_frames, _("drop overrun frames"))
{
	this->set_underline(0);
	this->gui = gui;
	set_tooltip(_("Drop input frames when behind."));
}

int RecordGUIDropFrames::handle_event()
{
	gui->record->drop_overrun_frames = get_value();
	return 1;
}

int RecordGUIDropFrames::keypress_event()
{
	if( get_keypress() == caption[0] ) {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}

RecordGUIFillFrames::RecordGUIFillFrames(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->fill_underrun_frames, _("fill underrun frames"))
{
	this->set_underline(0);
	this->gui = gui;
	set_tooltip(_("Write extra frames when behind."));
}

int RecordGUIFillFrames::handle_event()
{
	gui->record->fill_underrun_frames = get_value();
	return 1;
}

int RecordGUIFillFrames::keypress_event()
{
	if( get_keypress() == caption[0] ) {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}

RecordGUIPowerOff::RecordGUIPowerOff(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->power_off, _("poweroff when done"))
{
	this->set_underline(0);
	this->gui = gui;
	set_tooltip(_("poweroff system when batch record done."));
}

int RecordGUIPowerOff::handle_event()
{
	gui->record->power_off = get_value();
	return 1;
}

int RecordGUIPowerOff::keypress_event()
{
	if( get_keypress() == caption[0] ) {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}


RecordGUICommCheck::RecordGUICommCheck(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->commercial_check, _("check for ads"))
{
	this->set_underline(0);
	this->gui = gui;
	set_tooltip(_("check for commercials."));
}

int RecordGUICommCheck::handle_event()
{
	gui->record->update_skimming(get_value());
	return 1;
}


int RecordGUICommCheck::keypress_event()
{
	if( get_keypress() == caption[0] ) {
		set_value(get_value() ? 0 : 1);
		gui->record->update_skimming(get_value());
		handle_event();
		return 1;
	}
	return 0;
}


RecordGUIMonitorVideo::RecordGUIMonitorVideo(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->monitor_video, _("Monitor video"))
{
	this->set_underline(8);
	this->gui = gui;
}

int RecordGUIMonitorVideo::handle_event()
{
// Video capture constitutively, just like audio, but only flash on screen if 1
	int mode = get_value();
	Record *record = gui->record;
	record->set_video_monitoring(mode);
	if(record->monitor_video) {
		unlock_window();
		BC_Window *window = record->record_monitor->window;
		window->lock_window("RecordGUIMonitorVideo::handle_event");
		window->show_window();
		window->raise_window();
		window->flush();
		window->unlock_window();
		lock_window("RecordGUIMonitorVideo::handle_event");
		record->video_window_open = 1;
	}
	return 1;
}


int RecordGUIMonitorVideo::keypress_event()
{
	if(get_keypress() == 'v') {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}



RecordGUIMonitorAudio::RecordGUIMonitorAudio(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->monitor_audio, _("Monitor audio"))
{
	this->set_underline(8);
	this->gui = gui;
}

int RecordGUIMonitorAudio::handle_event()
{
	int mode = get_value();
	Record *record = gui->record;
	record->set_audio_monitoring(mode);
	if(record->monitor_audio) {
		unlock_window();
		BC_Window *window = record->record_monitor->window;
		window->lock_window("RecordGUIMonitorAudio::handle_event");
		window->show_window();
		window->raise_window();
		window->flush();
		window->unlock_window();
		lock_window("RecordGUIMonitorAudio::handle_event");
		record->video_window_open = 1;
	}
	return 1;
}

int RecordGUIMonitorAudio::keypress_event()
{
	if(get_keypress() == 'a') {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}


RecordGUIAudioMeters::RecordGUIAudioMeters(RecordGUI *gui, int x, int y)
 : BC_CheckBox(x, y, gui->record->metering_audio, _("Audio meters"))
{
	this->set_underline(6);
	this->gui = gui;
}

int RecordGUIAudioMeters::handle_event()
{
	int mode = get_value();
	Record *record = gui->record;
	record->set_audio_metering(mode);
	if(record->metering_audio) {
		unlock_window();
		BC_Window *window = record->record_monitor->window;
		window->lock_window("RecordGUIAudioMeters::handle_event");
		window->show_window();
		window->raise_window();
		window->flush();
		window->unlock_window();
		lock_window("RecordGUIAudioMeters::handle_event");
		record->video_window_open = 1;
	}
	return 1;
}

int RecordGUIAudioMeters::keypress_event()
{
	if(get_keypress() == 'm') {
		set_value(get_value() ? 0 : 1);
		handle_event();
		return 1;
	}
	return 0;
}

RecordPath::RecordPath(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::Path(gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordPath::handle_event()
{
	return RecordBatchesGUI::Path::handle_event();
}


RecordStart::RecordStart(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::StartTime(gui, gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordStart::handle_event()
{
	return RecordBatchesGUI::StartTime::handle_event();
}

RecordDuration::RecordDuration(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::Duration(gui, gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordDuration::handle_event()
{
	return RecordBatchesGUI::Duration::handle_event();
}


RecordSource::RecordSource(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::Source(gui, gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordSource::handle_event()
{
	gui->record->set_batch_channel_no(get_number());
	return RecordBatchesGUI::Source::handle_event();
}


RecordNews::RecordNews(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::News(gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordNews::handle_event()
{
	return RecordBatchesGUI::News::handle_event();
}


RecordGUINewBatch::RecordGUINewBatch(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::NewBatch(gui->record->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Create new clip."));
}
int RecordGUINewBatch::handle_event()
{
	gui->record->new_batch();
	return RecordBatchesGUI::NewBatch::handle_event();
}


RecordGUIDeleteBatch::RecordGUIDeleteBatch(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::DeleteBatch(gui->record->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Delete clip."));
}

int RecordGUIDeleteBatch::handle_event()
{
	gui->record->delete_batch();
	return RecordBatchesGUI::DeleteBatch::handle_event();
}


RecordGUIStartBatches::RecordGUIStartBatches(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::StartBatches(gui->record->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Start batch recording\nfrom the current position."));
}

int RecordGUIStartBatches::handle_event()
{
	Record *record = gui->record;
	record->start_cron_thread();
	return RecordBatchesGUI::StartBatches::handle_event();
}


RecordGUIStopBatches::RecordGUIStopBatches(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::StopBatches(gui->record->record_batches, x, y)
{
	this->gui = gui;
}

int RecordGUIStopBatches::handle_event()
{
	Record *record = gui->record;
	unlock_window();
	record->stop_cron_thread(_("Stopped"));
	lock_window();
	return RecordBatchesGUI::StopBatches::handle_event();
}


RecordGUIActivateBatch::RecordGUIActivateBatch(RecordGUI *gui, int x, int y)
 : RecordBatchesGUI::ActivateBatch(gui->record->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Make the highlighted\nclip active."));
}
int RecordGUIActivateBatch::handle_event()
{
	gui->record->activate_batch(gui->record->editing_batch());
	gui->update_cron_status(_("Idle"));
	return RecordBatchesGUI::ActivateBatch::handle_event();
}


RecordGUILabel::RecordGUILabel(RecordGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Label"))
{
	this->gui = gui;
	set_underline(0);
}


RecordGUILabel::~RecordGUILabel()
{
}

int RecordGUILabel::handle_event()
{
	gui->record->toggle_label();
	return 1;
}

int RecordGUILabel::keypress_event()
{
	if( get_keypress() == *get_text() ) {
		handle_event();
		return 1;
	}
	return 0;
}


RecordGUIClearLabels::RecordGUIClearLabels(RecordGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("ClrLbls"))
{
	this->gui = gui;
}


RecordGUIClearLabels::~RecordGUIClearLabels()
{
}

int RecordGUIClearLabels::handle_event()
{
	gui->record->clear_labels();
	return 1;
}


EndRecordThread::EndRecordThread(RecordGUI *gui)
 : Thread(1, 0, 0)
{
	this->gui = gui;
	is_ok = 0;
}

EndRecordThread::~EndRecordThread()
{
	if(Thread::running()) {
		window->lock_window("EndRecordThread::~EndRecordThread");
		window->set_done(1);
		window->unlock_window();
	}
	Thread::join();
}

void EndRecordThread::start(int is_ok)
{
	this->is_ok = is_ok;
	if( gui->record->writing_file ) {
		if(!running())
			Thread::start();
	}
	else {
		gui->set_done(!is_ok);
	}
}

void EndRecordThread::run()
{
	window = new QuestionWindow(gui->record->mwindow);
	window->create_objects(_("Interrupt recording in progress?"), 0);
	int result = window->run_window();
	delete window;
	if(result == 2) gui->set_done(!is_ok);
}


RecordStartoverThread::RecordStartoverThread(RecordGUI *gui)
 : Thread(1, 0, 0)
{
	this->gui = gui;
}
RecordStartoverThread::~RecordStartoverThread()
{
	if(Thread::running()) {
		window->lock_window("RecordStartoverThread::~RecordStartoverThread");
		window->set_done(1);
		window->unlock_window();
	}
	Thread::join();
}

void RecordStartoverThread::run()
{
	Record *record = gui->record;
	window = new QuestionWindow(record->mwindow);
	window->create_objects(_("Rewind batch and overwrite?"), 0);
	int result = window->run_window();
	if(result == 2) record->start_over();
	delete window;
}


int RecordGUI::set_translation(int x, int y, float z)
{
	record->video_x = x;
	record->video_y = y;
	record->video_zoom = z;
	return 0;
}

void RecordGUI::reset_video()
{
	total_dropped_frames = 0;
	status_thread->reset_video();
	update_framerate(record->default_asset->frame_rate);
}

void RecordGUI::update_dropped_frames(long value)
{
	status_thread->update_dropped_frames(value);
}

void RecordGUI::update_frames_behind(long value)
{
	status_thread->update_frames_behind(value);
}

void RecordGUI::update_position(double value)
{
	status_thread->update_position(value);
}

void RecordGUI::update_framerate(double value)
{
	status_thread->update_framerate(value);
}

void RecordGUI::update_video(int dropped, int behind)
{
	total_dropped_frames += dropped;
	update_dropped_frames(total_dropped_frames);
	update_frames_behind(behind);
	status_thread->update_position(record->current_display_position());
}

void RecordGUI::reset_audio()
{
//	gui->lock_window("RecordAudio::run 2");
// reset meter
	total_clipped_samples = 0;
	status_thread->reset_audio();
	AudioDevice *adevice = record->adevice;
	RecordMonitorGUI *window = record->record_monitor->window;
	window->lock_window("RecordAudio::run 2");
	MeterPanel *meters = window->meters;
	if( meters ) {
		int dmix = adevice && (adevice->get_idmix() || adevice->get_odmix());
		meters->init_meters(dmix);
	}
	window->unlock_window();
//	gui->unlock_window();
}

void RecordGUI::update_clipped_samples(long value)
{
	status_thread->update_clipped_samples(value);
}

void RecordGUI::update_audio(int channels, double *max, int *over)
{
// Get clipping status
	int clipped = 0;
	for( int ch=0; ch<channels && !clipped; ++ch )
		if( over[ch] ) clipped = 1;
	if( clipped ) {
		update_clipped_samples(++total_clipped_samples);
	}
// Update meters if monitoring
	if( record->metering_audio ) {
		RecordMonitorGUI *window = record->record_monitor->window;
		window->lock_window("RecordAudio::run 1");
		MeterPanel *meters = window->meters;
		int nmeters = meters->meters.total;
		for( int ch=0; ch<nmeters; ++ch ) {
			double vmax = ch < channels ? max[ch] : 0.;
			int   vover = ch < channels ? over[ch] : 0;
			meters->meters.values[ch]->update(vmax, vover);
		}
		window->unlock_window();
	}
// update position, if no video
	if( !record->default_asset->video_data )
		update_position(record->current_display_position());
}


int RecordGUI::keypress_event()
{
	return record_transport->keypress_event();
}

void RecordGUI::update_labels(double new_position)
{
	RecordLabel *prev, *next;

	for(prev = record->get_current_batch()->labels->last;
		prev;
		prev = prev->previous) {
		if(prev->position <= new_position) break;
	}

	for(next = record->get_current_batch()->labels->first;
		next;
		next = next->next)
	{
		if(next->position > new_position) break;
	}

	if(prev)
		update_title(prev_label_title, prev->position);
	else
		update_title(prev_label_title, -1);

// 	if(next)
// 		update_title(next_label_title, (double)next->position / record->default_asset->sample_rate);
// 	else
// 		update_title(next_label_title, -1);
}


int RecordGUI::update_prev_label(long new_position)
{
	update_title(prev_label_title, new_position);
	return 0;
}

// int RecordGUI::update_next_label(long new_position)
// {
// 	update_title(next_label_title, new_position);
// }
//
int RecordGUI::update_title(BC_Title *title, double position)
{
	static char string[256];

	if(position >= 0) {
		Units::totext(string,
				position,
				mwindow->edl->session->time_format,
				record->default_asset->sample_rate,
				record->default_asset->frame_rate,
				mwindow->edl->session->frames_per_foot);
	}
	else {
		sprintf(string, "-");
	}
	title->update(string);
	return 0;
}


void RecordGUI::update_cron_status(const char *status)
{
	lock_window("RecordGUI::update_cron_status");
	cron_status->update(status);
	unlock_window();
}

void RecordGUI::update_power_off(int value)
{
	lock_window("RecordGUI::update_power_off");
	power_off->update(value);
	unlock_window();
}




// ===================================== GUI
// ================================================== modes

RecordGUIModeTextBox::RecordGUIModeTextBox(RecordGUIModeMenu *mode_menu,
		int x, int y, int w,const char *text)
 : BC_TextBox(x, y, w, 1, text)
{
	this->mode_menu = mode_menu;
}

RecordGUIModeTextBox::~RecordGUIModeTextBox()
{
}

int RecordGUIModeTextBox::handle_event()
{
	return 0;
}

RecordGUIModeListBox::RecordGUIModeListBox(RecordGUIModeMenu *mode_menu)
 : BC_ListBox(mode_menu->textbox->get_x() + mode_menu->textbox->get_w(),
		mode_menu->textbox->get_y(), 100, 50, LISTBOX_TEXT,
		&mode_menu->modes, 0, 0, 1, 0, 1)
{
	this->mode_menu = mode_menu;
}

RecordGUIModeListBox::~RecordGUIModeListBox()
{
}

int RecordGUIModeListBox::handle_event()
{
	return mode_menu->handle_event();
}

RecordGUIModeMenu::RecordGUIModeMenu(RecordGUI *record_gui,
		int x, int y, int w,const char *text)
{
	this->record_gui = record_gui;
	textbox = new RecordGUIModeTextBox(this,x, y, w, "");
	record_gui->add_subwindow(textbox);
	listbox = new RecordGUIModeListBox(this);
	record_gui->add_subwindow(listbox);
}

RecordGUIModeMenu::~RecordGUIModeMenu()
{
	for( int i=0; i<modes.total; ++i )
		delete modes.values[i];
	delete listbox;
	delete textbox;
}

void RecordGUIModeMenu::create_objects()
{
	value = RECORD_UNTIMED;
	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_UNTIMED)));
	modes.append(new BC_ListBoxItem(Batch::mode_to_text(RECORD_TIMED)));
        textbox->update(modes.values[value]->get_text());
}

int RecordGUIModeMenu::handle_event()
{
	value = listbox->get_selection_number(0, 0);
	textbox->update(modes.values[value]->get_text());
	textbox->handle_event();
	record_gui->record->set_record_mode(value);
	if( value == RECORD_TIMED )
		record_gui->batch_duration->enable();
	else
		record_gui->batch_duration->disable();
	return 0;
}

void RecordGUIModeMenu::update(int value)
{
	this->value = value;
        textbox->update(modes.values[value]->get_text());
}

int RecordGUIModeMenu::get_w()
{
	return textbox->get_w() + listbox->get_w();
}

int RecordGUIModeMenu::get_h()
{
	return MAX(textbox->get_h(), listbox->get_h());
}


RecordStatusThread::RecordStatusThread(MWindow *mwindow, RecordGUI *gui)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	this->gui = gui;
	input_lock = new Condition(0, "RecordStatusThread::input_lock");
	reset_video();
	reset_audio();
	window_locked = 0;
	done = 0;
}

RecordStatusThread::~RecordStatusThread()
{
	if(Thread::running())
	{
		done = 1;
		input_lock->unlock();
	}
	Thread::join();
	delete input_lock;
}

void RecordStatusThread::reset_video()
{
	new_dropped_frames = 0;
	displayed_dropped_frames = -1;
	new_frames_behind = 0;
	displayed_frames_behind = -1;
	new_position = 0;
	displayed_framerate = -1.;
	new_framerate = 0.;
}

void RecordStatusThread::reset_audio()
{
	new_clipped_samples = 0;
	displayed_clipped_samples = -1;
}

void RecordStatusThread::update_frames_behind(long value)
{
	if( value != displayed_frames_behind ) {
		new_frames_behind = value;
		input_lock->unlock();
	}
}

void RecordStatusThread::update_dropped_frames(long value)
{
	if( value != displayed_dropped_frames ) {
		new_dropped_frames = value;
		input_lock->unlock();
	}
}

void RecordStatusThread::update_clipped_samples(long value)
{
	if( value != displayed_clipped_samples ) {
		new_clipped_samples = value;
		input_lock->unlock();
	}
}

void RecordStatusThread::update_position(double value)
{
	this->new_position = value;
	input_lock->unlock();
}

void RecordStatusThread::update_framerate(double value)
{
	if( value != displayed_framerate ) {
		new_framerate = value;
		input_lock->unlock();
	}
}

void RecordStatusThread::get_window_lock()
{
	if( !window_locked ) {
		gui->lock_window("RecordStatusThread::run 1");
		window_locked = 1;
	}
}

template<class T>
void RecordStatusThread::
update_status_string(const char *fmt, T &new_value, T &displayed_value, BC_Title *widgit)
{
	if( new_value >= 0 ) {
		if( displayed_value != new_value ) {
			displayed_value = new_value;
			if( widgit ) {
				char string[64];
				sprintf(string, fmt, displayed_value);
				get_window_lock();
				widgit->update(string);
			}
		}
		new_value = -1;
	}
}

void RecordStatusThread::run()
{
	while(!done) {
		input_lock->lock("RecordStatusThread::run");
		if(done) break;
		update_status_string("%d", new_dropped_frames, displayed_dropped_frames,
					gui->frames_dropped);
		update_status_string("%d", new_frames_behind, displayed_frames_behind,
					gui->frames_behind);
		update_status_string("%d", new_clipped_samples, displayed_clipped_samples,
					gui->samples_clipped);
		update_status_string("%0.2f", new_framerate, displayed_framerate,
					gui->framerate);
		if( new_position >= 0 ) {
			get_window_lock();
			gui->update_title(gui->position_title, new_position);
			gui->update_labels(new_position);
			new_position = -1;
		}
		if( window_locked ) {
			gui->unlock_window();
			window_locked = 0;
		}
	}
}


void RecordGUI::start_flash_batch()
{
	if( batch_flash ) return;
	batch_flash = new RecordGUIFlash(this);
}

void RecordGUI::stop_flash_batch()
{
	if( !batch_flash ) return;
	delete batch_flash;
	batch_flash = 0;
}

void RecordGUI::flash_batch()
{
	lock_window("void RecordGUI::flash_batch");
	int cur_batch = record->current_batch();
	if( cur_batch >= 0 && cur_batch < batch_bay->count()) {
		flash_color = flash_color == GREEN ? RED : GREEN;
//printf("RecordGUI::flash_batch %x\n", flash_color);
		batch_bay->set_row_color(cur_batch, flash_color);
		batch_bay->update_batch_news(cur_batch);
	}
	unlock_window();
}

RecordGUIFlash::
RecordGUIFlash(RecordGUI *record_gui)
 : Thread(1, 0, 0)
{
	this->record_gui = record_gui;
	flash_lock = new Condition(0,"RecordGUIFlash::flash_lock");
	done = 0;
	Thread::start();
}

RecordGUIFlash::
~RecordGUIFlash()
{
	if( Thread::running() ) {
		done = 1;
		flash_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
	delete flash_lock;
}

void RecordGUIFlash::run()
{
	while( !done ) {
		record_gui->flash_batch();
		enable_cancel();
		Timer::delay(500);
		disable_cancel();
	}
}



RecordGUIDCOffset::RecordGUIDCOffset(MWindow *mwindow, int y)
 : BC_Button(230, y, mwindow->theme->calibrate_data)
{
}

RecordGUIDCOffset::~RecordGUIDCOffset() {}

int RecordGUIDCOffset::handle_event()
{
	return 1;
}

int RecordGUIDCOffset::keypress_event() { return 0; }

RecordGUIDCOffsetText::RecordGUIDCOffsetText(char *text, int y, int number)
 : BC_TextBox(30, y+1, 67, 1, text, 0)
{
	this->number = number;
}

RecordGUIDCOffsetText::~RecordGUIDCOffsetText()
{
}

int RecordGUIDCOffsetText::handle_event()
{
	return 1;
}

RecordGUIReset::RecordGUIReset(MWindow *mwindow, RecordGUI *gui, int y)
 : BC_Button(400, y, mwindow->theme->over_button)
{ this->gui = gui; }

RecordGUIReset::~RecordGUIReset()
{
}

int RecordGUIReset::handle_event()
{
	return 1;
}

RecordGUIResetTranslation::RecordGUIResetTranslation(MWindow *mwindow, RecordGUI *gui, int y)
 : BC_Button(250, y, mwindow->theme->reset_data)
{
	this->gui = gui;
}

RecordGUIResetTranslation::~RecordGUIResetTranslation()
{
}

int RecordGUIResetTranslation::handle_event()
{
	gui->set_translation(0, 0, 1);
	return 1;
}

