
/*
 * CINELERRA
 * Copyright (C) 2008-2013 Adam Williams <broadcast at earthling dot net>
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
#include "asset.h"
#include "audioconfig.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "new.h"
#include "language.h"
#include "mwindow.h"
#include "preferences.h"
#include "recordconfig.h"
#include "recordprefs.h"
#include "theme.h"
#include "vdeviceprefs.h"




RecordPrefs::RecordPrefs(MWindow *mwindow, PreferencesWindow *pwindow)
 : PreferencesDialog(mwindow, pwindow)
{
	this->mwindow = mwindow;
}

RecordPrefs::~RecordPrefs()
{
	delete audio_in_device;
	delete video_in_device;
	delete recording_format;
//	delete duplex_device;
}

void RecordPrefs::create_objects()
{
	int x, y, x1, x2;
	char string[BCTEXTLEN];
	BC_Resources *resources = BC_WindowBase::get_resources();
	BC_Title *title;
	BC_Title *title0, *title1, *title2, *title3;
	BC_TextBox *textbox;

	x = mwindow->theme->preferencesoptions_x;
	y = mwindow->theme->preferencesoptions_y;
	int margin = mwindow->theme->widget_border;
	int x0 = x, y0 = y;

	add_subwindow(title = new BC_Title(x, y, _("File Format:"),
		LARGEFONT, resources->text_default));
	y += title->get_h() + margin;

	recording_format = new FormatTools(mwindow, this,
			pwindow->thread->edl->session->recording_format);

	recording_format->create_objects(x, y,
		1,  // Include tools for audio
		1,  // Include tools for video
		1,  // Include checkbox for audio
		1,  // Include checkbox for video
		0,
		1,
		0,  // Select compressors to be offered
		1,  // Prompt for recording options
		0,  // prompt for use labels
		0); // Supply file formats for background rendering

	realtime_toc = new RecordRealtimeTOC(mwindow, pwindow,
		x0+400, y0, pwindow->thread->edl->session->record_realtime_toc);
	add_subwindow(realtime_toc);

// Audio hardware
	add_subwindow(new BC_Bar(x, y, 	get_w() - x * 2));
	y += margin;


	add_subwindow(title = new BC_Title(x, y,
		_("Audio In"), LARGEFONT,
		resources->text_default));

	y += title->get_h() + margin;

	add_subwindow(new BC_Title(x, y, _("Record Driver:"),
		MEDIUMFONT, resources->text_default));
	audio_in_device = new ADevicePrefs(x + 110, y, pwindow, this, 0,
		pwindow->thread->edl->session->aconfig_in, MODERECORD);
	audio_in_device->initialize(1);
	y += audio_in_device->get_h(1) + margin;


	int pad = RecordWriteLength::calculate_h(this,
		MEDIUMFONT,
		1,
		1) +
		mwindow->theme->widget_border;
	add_subwindow(title0 = new BC_Title(x, y, _("Samples read from device:")));
	add_subwindow(title1 = new BC_Title(x, y + pad, _("Samples to write to disk:")));
	add_subwindow(title2 = new BC_Title(x, y + pad * 2, _("Sample rate for recording:")));
	add_subwindow(title3 = new BC_Title(x, y + pad * 3, _("Channels to record:")));
	x2 = MAX(title0->get_w(), title1->get_w()) + margin;
	x2 = MAX(x2, title2->get_w() + margin);
	x2 = MAX(x2, title3->get_w() + margin);


	sprintf(string, "%ld", (long)pwindow->thread->edl->session->record_fragment_size);
	RecordFragment *menu;
	add_subwindow(menu = new RecordFragment(x2,
		y,
		pwindow,
		this,
		string));
	y += menu->get_h() + mwindow->theme->widget_border;
	menu->add_item(new BC_MenuItem("1024"));
	menu->add_item(new BC_MenuItem("2048"));
	menu->add_item(new BC_MenuItem("4096"));
	menu->add_item(new BC_MenuItem("8192"));
	menu->add_item(new BC_MenuItem("16384"));
	menu->add_item(new BC_MenuItem("32768"));
	menu->add_item(new BC_MenuItem("65536"));
	menu->add_item(new BC_MenuItem("131072"));
	menu->add_item(new BC_MenuItem("262144"));

	sprintf(string, "%jd", pwindow->thread->edl->session->record_write_length);
	add_subwindow(textbox = new RecordWriteLength(mwindow, pwindow, x2, y, string));
	y += textbox->get_h() + mwindow->theme->widget_border;
	add_subwindow(textbox = new RecordSampleRate(pwindow, x2, y));
	add_subwindow(new SampleRatePulldown(mwindow,
		textbox,
		x2 + textbox->get_w(),
		y));
	y += textbox->get_h() + mwindow->theme->widget_border;

	RecordChannels *channels = new RecordChannels(pwindow, this, x2, y);
	channels->create_objects();
	y += channels->get_h() + mwindow->theme->widget_border;

	RecordMap51_2 *record_map51_2 = new RecordMap51_2(mwindow, pwindow, x, y,
		pwindow->thread->edl->session->aconfig_in->map51_2);
	add_subwindow(record_map51_2);

	x2 = x + record_map51_2->get_w() + 30;
	int y2 = y + BC_TextBox::calculate_h(this,MEDIUMFONT,1,1) - get_text_height(MEDIUMFONT);
	add_subwindow(title = new BC_Title(x2, y2, _("Gain:")));
	x2 += title->get_w() + 8;
	RecordGain *rec_gain = new RecordGain(pwindow, this, x2, y);
	rec_gain->create_objects();

	x2 += rec_gain->get_w() + 30;
	add_subwindow(new RecordRealTime(mwindow, pwindow, x2, y,
		pwindow->thread->edl->session->real_time_record));
	y += 30;
	x = 5;


// Video hardware
	add_subwindow(new BC_Bar(5, y, 	get_w() - 10));
	y += margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Video In"), LARGEFONT,
		resources->text_default));
	y += title1->get_h() + margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Record Driver:"), MEDIUMFONT,
		resources->text_default));
	video_in_device = new VDevicePrefs(x + title1->get_w() + margin, y,
		pwindow, this, 0, pwindow->thread->edl->session->vconfig_in, MODERECORD);
	video_in_device->initialize(1);
	y += video_in_device->get_h() + margin;

	add_subwindow(title1 = new BC_Title(x, y, _("Frames to record to disk at a time:")));
	x1 = x + title1->get_w() + margin;
	sprintf(string, "%d", pwindow->thread->edl->session->video_write_length);
	add_subwindow(textbox = new VideoWriteLength(pwindow, string, x1, y));
	x1 += textbox->get_w() + margin;
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, x1, y));
	y += 27;

	add_subwindow(title1 = new BC_Title(x, y, _("Frames to buffer in device:")));
	x1 = x + title1->get_w() + margin;
	sprintf(string, "%d", pwindow->thread->edl->session->vconfig_in->capture_length);
	add_subwindow(textbox = new VideoCaptureLength(pwindow, string, x1, y));
	x1 += textbox->get_w() + margin;
	add_subwindow(new CaptureLengthTumbler(pwindow, textbox, x1, y));
	y += 27;

	x1 = x;
	add_subwindow(new BC_Title(x1, y, _("Positioning:")));
	x1 += 100;
	add_subwindow(textbox = new BC_TextBox(x1, y, 200, 1, ""));
	RecordPositioning *positioning = new RecordPositioning(pwindow,textbox);
	add_subwindow(positioning);
	positioning->create_objects();
	y += positioning->get_h() + 5;

	add_subwindow(new RecordSyncDrives(pwindow,
		pwindow->thread->edl->session->record_sync_drives,
		x, y));
	y += 35;

	BC_TextBox *w_text, *h_text;
	add_subwindow(title1 = new BC_Title(x, y, _("Size of captured frame:")));
	x += title1->get_w() + margin;
	add_subwindow(w_text = new RecordW(pwindow, x, y));
	x += w_text->get_w() + margin;
	add_subwindow(title1 = new BC_Title(x, y, "x"));
	x += title1->get_w() + margin;
	add_subwindow(h_text = new RecordH(pwindow, x, y));
	x += h_text->get_w() + margin;
	FrameSizePulldown *tumbler;
	add_subwindow(tumbler = new FrameSizePulldown(mwindow->theme, 
		w_text, h_text, x, y));
	y += tumbler->get_h() + margin;

	x = mwindow->theme->preferencesoptions_x;
	add_subwindow(title1 = new BC_Title(x, y, _("Frame rate for recording:")));
	x += title1->get_w() + margin;
	add_subwindow(textbox = new RecordFrameRate(pwindow, x, y));
	x += textbox->get_w() + margin;
	add_subwindow(new FrameRatePulldown(mwindow, textbox, x, y));

}


int RecordPrefs::show_window(int flush)
{
	PreferencesDialog::show_window(flush);
	if( pwindow->thread->edl->session->recording_format->format == FILE_MPEG &&
	    pwindow->thread->edl->session->vconfig_in->driver == CAPTURE_DVB &&
	    pwindow->thread->edl->session->aconfig_in->driver == AUDIO_DVB )
		return realtime_toc->show_window(flush);
	return realtime_toc->hide_window(flush);
}





RecordFragment::RecordFragment(int x,
	int y,
	PreferencesWindow *pwindow,
	RecordPrefs *record,
	char *text)
 : BC_PopupMenu(x,
 	y,
	100,
	text,
	1)
{
	this->pwindow = pwindow;
	this->record = record;
}

int RecordFragment::handle_event()
{
	pwindow->thread->edl->session->record_fragment_size = atol(get_text());
	return 1;
}







RecordWriteLength::RecordWriteLength(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, char *text)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->pwindow = pwindow;
}

int RecordWriteLength::handle_event()
{
	pwindow->thread->edl->session->record_write_length = atol(get_text());
	return 1;
}


RecordRealTime::RecordRealTime(MWindow *mwindow,
	PreferencesWindow *pwindow, int x, int y, int value)
 : BC_CheckBox(x, y, value,
	_("Record in realtime priority (root only)"))
{
	this->pwindow = pwindow;
}

int RecordRealTime::handle_event()
{
	pwindow->thread->edl->session->real_time_record = get_value();
	return 1;
}


RecordMap51_2::RecordMap51_2(MWindow *mwindow,
	PreferencesWindow *pwindow, int x, int y, int value)
 : BC_CheckBox(x, y, value, _("Map 5.1->2"))
{
	this->pwindow = pwindow;
}

int RecordMap51_2::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->map51_2 = get_value();
	return 1;
}


RecordSampleRate::RecordSampleRate(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->aconfig_in->in_samplerate)
{
	this->pwindow = pwindow;
}
int RecordSampleRate::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->in_samplerate = atol(get_text());
	return 1;
}


RecordRealtimeTOC::RecordRealtimeTOC(MWindow *mwindow,
	PreferencesWindow *pwindow, int x, int y, int value)
 : BC_CheckBox(x, y, value, _("Realtime TOC"))
{
	this->pwindow = pwindow;
}

int RecordRealtimeTOC::handle_event()
{
	pwindow->thread->edl->session->record_realtime_toc = get_value();
	return 1;
}


// DuplexEnable::DuplexEnable(MWindow *mwindow, PreferencesWindow *pwindow, int x, int y, int value)
//  : BC_CheckBox(x, y, value, _("Enable full duplex"))
// { this->pwindow = pwindow; }
//
// int DuplexEnable::handle_event()
// {
// 	pwindow->thread->edl->session->enable_duplex = get_value();
// }
//


RecordW::RecordW(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->vconfig_in->w)
{
	this->pwindow = pwindow;
}
int RecordW::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->w = atol(get_text());
	return 1;
}

RecordH::RecordH(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 70, 1, pwindow->thread->edl->session->vconfig_in->h)
{
	this->pwindow = pwindow;
}
int RecordH::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->h = atol(get_text());
	return 1;
}

RecordFrameRate::RecordFrameRate(PreferencesWindow *pwindow, int x, int y)
 : BC_TextBox(x, y, 140, 1, pwindow->thread->edl->session->vconfig_in->in_framerate)
{
	this->pwindow = pwindow;
}
int RecordFrameRate::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->in_framerate = atof(get_text());
	return 1;
}



RecordChannels::RecordChannels(PreferencesWindow *pwindow, BC_SubWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui,
		pwindow->thread->edl->session->aconfig_in->channels,
		1, MAX_CHANNELS, x, y, 100)
{
	this->pwindow = pwindow;
}

int RecordChannels::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->channels = atoi(get_text());
	return 1;
}

RecordGain::RecordGain(PreferencesWindow *pwindow, BC_SubWindow *gui, int x, int y)
 : BC_TumbleTextBox(gui,
		pwindow->thread->edl->session->aconfig_in->rec_gain,
		0.0001f, 10000.0f, x, y, 72)
{
	this->pwindow = pwindow;
	this->set_increment(0.1);
}

int RecordGain::handle_event()
{
	pwindow->thread->edl->session->aconfig_in->rec_gain = atof(get_text());
	return 1;
}



VideoWriteLength::VideoWriteLength(PreferencesWindow *pwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->pwindow = pwindow;
}

int VideoWriteLength::handle_event()
{
	pwindow->thread->edl->session->video_write_length = atol(get_text());
	return 1;
}


VideoCaptureLength::VideoCaptureLength(PreferencesWindow *pwindow, char *text, int x, int y)
 : BC_TextBox(x, y, 100, 1, text)
{
	this->pwindow = pwindow;
}

int VideoCaptureLength::handle_event()
{
	pwindow->thread->edl->session->vconfig_in->capture_length = atol(get_text());
	return 1;
}

CaptureLengthTumbler::CaptureLengthTumbler(PreferencesWindow *pwindow, BC_TextBox *text, int x, int y)
 : BC_Tumbler(x, y)
{
	this->pwindow = pwindow;
	this->text = text;
}

int CaptureLengthTumbler::handle_up_event()
{
	int value = atol(text->get_text());
	value++;
	char string[BCTEXTLEN];
	sprintf(string, "%d", value);
	text->update(string);
	text->handle_event();
	return 1;
}

int CaptureLengthTumbler::handle_down_event()
{
	int value = atol(text->get_text());
	value--;
	value = MAX(1, value);
	char string[BCTEXTLEN];
	sprintf(string, "%d", value);
	text->update(string);
	text->handle_event();
	return 1;
}

RecordPositioning::RecordPositioning(PreferencesWindow *pwindow, BC_TextBox *textbox)
 : BC_ListBox(textbox->get_x() + textbox->get_w(), textbox->get_y(),
		 200, 100, LISTBOX_TEXT, &position_type, 0, 0, 1, 0, 1)
{
	this->pwindow = pwindow;
	this->textbox = textbox;
}

RecordPositioning::~RecordPositioning()
{
	for( int i=0; i<position_type.total; ++i )
		delete position_type.values[i];
}

void RecordPositioning::create_objects()
{
	position_type.append(new BC_ListBoxItem(_("Presentation Timestamps")));
	position_type.append(new BC_ListBoxItem(_("Software timing")));
	position_type.append(new BC_ListBoxItem(_("Device Position")));
	position_type.append(new BC_ListBoxItem(_("Sample Position")));
	int value = pwindow->thread->edl->session->record_positioning;
	textbox->update(position_type.values[value]->get_text());
}

int RecordPositioning::handle_event()
{
	int v = get_selection_number(0, 0);
	pwindow->thread->edl->session->record_positioning = v;
	textbox->update(position_type.values[v]->get_text());
	textbox->handle_event();
	return 1;
}


RecordSyncDrives::RecordSyncDrives(PreferencesWindow *pwindow, int value, int x, int y)
 : BC_CheckBox(x, y, value, _("Sync drives automatically"))
{
	this->pwindow = pwindow;
}

int RecordSyncDrives::handle_event()
{
	pwindow->thread->edl->session->record_sync_drives = get_value();
	return 1;
}


