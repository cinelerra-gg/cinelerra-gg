
/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcsignals.h"
#include "channeldb.h"
#include "channelpicker.h"
#include "clip.h"
#include "edl.h"
#include "edlsession.h"
#include "formattools.h"
#include "language.h"
#include "mwindow.h"
#include "mainsession.h"
#include "vdeviceprefs.h"
#include "videoconfig.h"
#include "videodevice.inc"
#include "overlayframe.inc"
#include "playbackconfig.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "recordconfig.h"
#include "recordprefs.h"
#include "theme.h"
#include <string.h>


VDevicePrefs::VDevicePrefs(int x,
	int y,
	PreferencesWindow *pwindow,
	PreferencesDialog *dialog,
	VideoOutConfig *out_config,
	VideoInConfig *in_config,
	int mode)
{
	this->pwindow = pwindow;
	this->dialog = dialog;
	this->driver = DEV_UNKNOWN;
	this->mode = mode;
	this->out_config = out_config;
	this->in_config = in_config;
	this->x = x;
	this->y = y;
	menu = 0;
	reset_objects();

}

VDevicePrefs::~VDevicePrefs()
{
	delete_objects();
	if(menu) delete menu;
	int config = -1;
	switch( pwindow->thread->current_dialog ) {
	case PreferencesThread::PLAYBACK_A:  config = 0;  break;
	case PreferencesThread::PLAYBACK_B:  config = 1;  break;
	}
	if( config >= 0 )
		pwindow->mwindow->session->save_x11_host(config, out_config->x11_host);
}


void VDevicePrefs::reset_objects()
{
	device_title = 0;
	port_title = 0;
	follow_video_config = 0;
	dvb_adapter_title = 0;
	dvb_adapter_device = 0;
	channel_title = 0;
	output_title = 0;
	syt_title = 0;

	device_text = 0;
	firewire_port = 0;
	firewire_channel = 0;
	firewire_channels = 0;
	firewire_syt = 0;
	firewire_path = 0;
	fields_title = 0;
	device_fields = 0;
	use_direct_x11 = 0;

	channel_picker = 0;
}

int VDevicePrefs::initialize(int creation)
{
	int *driver = 0;
	delete_objects();

	switch(mode)
	{
		case MODEPLAY:
			driver = &out_config->driver;
			break;

		case MODERECORD:
			driver = &in_config->driver;
			break;
	}
	this->driver = *driver;

	if(!menu)
	{
		dialog->add_subwindow(menu = new VDriverMenu(x,
			y + 10,
			this,
			(mode == MODERECORD),
			driver));
		menu->create_objects();
	}

	switch(this->driver)
	{
		case DEV_UNKNOWN:
			break;
		case VIDEO4LINUX2:
		case CAPTURE_JPEG_WEBCAM:
		case CAPTURE_YUYV_WEBCAM:
			create_v4l2_objs();
			break;
		case VIDEO4LINUX2JPEG:
			create_v4l2jpeg_objs();
			break;
		case VIDEO4LINUX2MPEG:
			create_v4l2mpeg_objs();
			break;
		case SCREENCAPTURE:
			create_screencap_objs();
			break;
		case PLAYBACK_X11:
		case PLAYBACK_X11_XV:
		case PLAYBACK_X11_GL:
			create_x11_objs();
			break;
		case PLAYBACK_DV1394:
		case PLAYBACK_FIREWIRE:
		case PLAYBACK_IEC61883:
		case CAPTURE_FIREWIRE:
		case CAPTURE_IEC61883:
			create_firewire_objs();
			break;
		case CAPTURE_DVB:
			create_dvb_objs();
			break;
	}



// Update driver dependancies in file format
	if(mode == MODERECORD && dialog && !creation)
	{
		RecordPrefs *record_prefs = (RecordPrefs*)dialog;
		record_prefs->recording_format->update_driver(this->driver);
	}

	return 0;
}

int VDevicePrefs::delete_objects()
{
	delete output_title;
	delete channel_picker;
	delete device_title;
	delete device_text;
	delete dvb_adapter_device;
	delete follow_video_config;
	delete dvb_adapter_title;
	delete use_direct_x11;

	delete port_title;

	if(firewire_port) delete firewire_port;
	if(channel_title) delete channel_title;
	if(firewire_channel) delete firewire_channel;
	if(firewire_path) delete firewire_path;
	if(syt_title) delete syt_title;
	if(firewire_syt) delete firewire_syt;
	if(fields_title) delete fields_title;
	if(device_fields) delete device_fields;

	reset_objects();
	driver = -1;
	return 0;
}

int VDevicePrefs::get_h()
{
	int margin = pwindow->mwindow->theme->widget_border;
	return BC_Title::calculate_h(dialog, "X", MEDIUMFONT) + margin +
		BC_TextBox::calculate_h(dialog, MEDIUMFONT, 1, 1);
}

void VDevicePrefs::create_dvb_objs()
{
	int x1 = x + menu->get_w() + 30;
	int y1 = y + 10;
	char *output_char = in_config->dvb_in_adapter;
	int y2 = y1 - BC_Title::calculate_h(dialog, _("DVB Adapter:"), MEDIUMFONT) - 5;
	BC_Resources *resources = BC_WindowBase::get_resources();
	dvb_adapter_title = new BC_Title(x1, y2, _("DVB Adapter:"),
			MEDIUMFONT, resources->text_default);
	dialog->add_subwindow(dvb_adapter_title);
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y1, output_char));
	int x2 = x1 + device_text->get_w() + 5;
	device_title = new BC_Title(x2, y2, _("dev:"),
			MEDIUMFONT, resources->text_default);
	dialog->add_subwindow(device_title);
	int *output_int = &in_config->dvb_in_device;
	dvb_adapter_device = new VDeviceTumbleBox(this, x2, y1, output_int, 0, 9, 20);
	dvb_adapter_device->create_objects();
	x1 += 64;  y1 += device_text->get_h() + 5;
	follow_video_config = new BC_CheckBox(x1, y1,
			&in_config->follow_video, _("Follow video config"));
	dialog->add_subwindow(follow_video_config);
}

int VDevicePrefs::create_firewire_objs()
{
	int *output_int = 0;
	char *output_char = 0;
	int x1 = x + menu->get_w() + 5;
	BC_Resources *resources = BC_WindowBase::get_resources();

// Firewire path
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_char = out_config->dv1394_path;
			else
			if(driver == PLAYBACK_FIREWIRE)
				output_char = out_config->firewire_path;
			break;
		case MODERECORD:
			if(driver == CAPTURE_FIREWIRE)
				output_char = in_config->firewire_path;
			break;
	}

	if(output_char)
	{
		dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device Path:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_path = new VDeviceTextBox(x1, y + 20, output_char));
		x1 += firewire_path->get_w() + 5;
	}

// Firewire port
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_port;
			else
				output_int = &out_config->firewire_port;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_port;
			break;
	}
	dialog->add_subwindow(port_title = new BC_Title(x1, y, _("Port:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_port = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_port->get_w() + 5;

// Firewire channel
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_channel;
			else
				output_int = &out_config->firewire_channel;
			break;
		case MODERECORD:
			output_int = &in_config->firewire_channel;
			break;
	}

	dialog->add_subwindow(channel_title = new BC_Title(x1, y, _("Channel:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(firewire_channel = new VDeviceIntBox(x1, y + 20, output_int));
	x1 += firewire_channel->get_w() + 5;


// Firewire syt
	switch(mode)
	{
		case MODEPLAY:
			if(driver == PLAYBACK_DV1394)
				output_int = &out_config->dv1394_syt;
			else
			if(driver == PLAYBACK_FIREWIRE)
				output_int = &out_config->firewire_syt;
			else
				output_int = 0;
			break;
		case MODERECORD:
			output_int = 0;
			break;
	}
	if(output_int)
	{
		dialog->add_subwindow(syt_title = new BC_Title(x1, y, _("Syt Offset:"), MEDIUMFONT, resources->text_default));
		dialog->add_subwindow(firewire_syt = new VDeviceIntBox(x1, y + 20, output_int));
	}

	return 0;
}

int VDevicePrefs::create_v4l2_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));

	return 0;
}

int VDevicePrefs::create_v4l2jpeg_objs()
{
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	char *output_char = &pwindow->thread->edl->session->vconfig_in->v4l2jpeg_in_device[0];
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	x1 += bmax(device_title->get_w(),device_text->get_w()) + 5;
	int *output_int = &pwindow->thread->edl->session->vconfig_in->v4l2jpeg_in_fields;
	fields_title = new BC_Title(x1, y, _("Fields:"), MEDIUMFONT, resources->text_default);
	dialog->add_subwindow(fields_title);
	device_fields = new VDeviceTumbleBox(this, x1, y + 20, output_int, 1, 2, 20);
	device_fields->create_objects();
	return 0;
}

int VDevicePrefs::create_v4l2mpeg_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->v4l2mpeg_in_device;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Device path:"), MEDIUMFONT, resources->text_default));
	int y1 = y + 20;
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y1, output_char));
	x1 += 64;  y1 += device_text->get_h() + 5;
	follow_video_config = new BC_CheckBox(x1, y1,
			&in_config->follow_video, _("Follow video config"));
	dialog->add_subwindow(follow_video_config);
	return 0;
}


int VDevicePrefs::create_screencap_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	int x1 = x + menu->get_w() + 5;
	output_char = pwindow->thread->edl->session->vconfig_in->screencapture_display;
	dialog->add_subwindow(device_title = new BC_Title(x1, y, _("Display:"), MEDIUMFONT, resources->text_default));
	dialog->add_subwindow(device_text = new VDeviceTextBox(x1, y + 20, output_char));
	return 0;
}

int VDevicePrefs::create_x11_objs()
{
	char *output_char;
	BC_Resources *resources = BC_WindowBase::get_resources();
	output_char = out_config->x11_host;
	const char *x11_display;
	switch( pwindow->thread->current_dialog ) {
	default:
	case PreferencesThread::PLAYBACK_A:
		x11_display = _("Default A Display:");  break;
		break;
	case PreferencesThread::PLAYBACK_B:
		x11_display = _("Default B Display:");  break;
		break;
	}
	int x1 = menu->get_x() + menu->get_w() + 10;
	int y1 = menu->get_y();
	if( driver == PLAYBACK_X11 ) y1 -= 10;
	dialog->add_subwindow(device_title = new BC_Title(x1, y1+4, x11_display,
			MEDIUMFONT, resources->text_default));
	int x2 = x1 + device_title->get_w() + 10, dy = device_title->get_h();
	dialog->add_subwindow(device_text = new VDeviceTextBox(x2, y1, output_char));
	if( driver == PLAYBACK_X11 ) {
		int y2 = device_text->get_h();
		if( dy < y2 ) dy = y2;
		y1 += dy + 5;
		use_direct_x11 = new BC_CheckBox(x1, y1,
			&out_config->use_direct_x11, _("use direct x11 render if possible"));
		dialog->add_subwindow(use_direct_x11);
	}
	return 0;
}




VDriverMenu::VDriverMenu(int x,
	int y,
	VDevicePrefs *device_prefs,
	int do_input,
	int *output)
 : BC_PopupMenu(x, y, 200, driver_to_string(*output))
{
	this->output = output;
	this->do_input = do_input;
	this->device_prefs = device_prefs;
}

VDriverMenu::~VDriverMenu()
{
}

char* VDriverMenu::driver_to_string(int driver)
{
	switch(driver)
	{
		case DEV_UNKNOWN:
			sprintf(string, DEV_UNKNOWN_TITLE);
			break;
		case VIDEO4LINUX2:
			sprintf(string, VIDEO4LINUX2_TITLE);
			break;
		case CAPTURE_JPEG_WEBCAM:
			sprintf(string, CAPTURE_JPEG_WEBCAM_TITLE);
			break;
		case CAPTURE_YUYV_WEBCAM:
			sprintf(string, CAPTURE_YUYV_WEBCAM_TITLE);
			break;
		case VIDEO4LINUX2JPEG:
			sprintf(string, VIDEO4LINUX2JPEG_TITLE);
			break;
		case VIDEO4LINUX2MPEG:
			sprintf(string, VIDEO4LINUX2MPEG_TITLE);
			break;
		case SCREENCAPTURE:
			sprintf(string, SCREENCAPTURE_TITLE);
			break;
#ifdef HAVE_FIREWIRE
		case CAPTURE_FIREWIRE:
			sprintf(string, CAPTURE_FIREWIRE_TITLE);
			break;
		case CAPTURE_IEC61883:
			sprintf(string, CAPTURE_IEC61883_TITLE);
			break;
#endif
		case CAPTURE_DVB:
			sprintf(string, CAPTURE_DVB_TITLE);
			break;
		case PLAYBACK_X11:
			sprintf(string, PLAYBACK_X11_TITLE);
			break;
		case PLAYBACK_X11_XV:
			sprintf(string, PLAYBACK_X11_XV_TITLE);
			break;
		case PLAYBACK_X11_GL:
			sprintf(string, PLAYBACK_X11_GL_TITLE);
			break;
#ifdef HAVE_FIREWIRE
		case PLAYBACK_FIREWIRE:
			sprintf(string, PLAYBACK_FIREWIRE_TITLE);
			break;
		case PLAYBACK_DV1394:
			sprintf(string, PLAYBACK_DV1394_TITLE);
			break;
		case PLAYBACK_IEC61883:
			sprintf(string, PLAYBACK_IEC61883_TITLE);
			break;
#endif
		default:
			string[0] = 0;
	}
	return string;
}

void VDriverMenu::create_objects()
{
	if(do_input)
	{
#ifdef HAVE_VIDEO4LINUX2
		add_item(new VDriverItem(this, VIDEO4LINUX2_TITLE, VIDEO4LINUX2));
		add_item(new VDriverItem(this, CAPTURE_JPEG_WEBCAM_TITLE, CAPTURE_JPEG_WEBCAM));
		add_item(new VDriverItem(this, CAPTURE_YUYV_WEBCAM_TITLE, CAPTURE_YUYV_WEBCAM));
		add_item(new VDriverItem(this, VIDEO4LINUX2JPEG_TITLE, VIDEO4LINUX2JPEG));
		add_item(new VDriverItem(this, VIDEO4LINUX2MPEG_TITLE, VIDEO4LINUX2MPEG));
#endif

		add_item(new VDriverItem(this, SCREENCAPTURE_TITLE, SCREENCAPTURE));
#ifdef HAVE_FIREWIRE
		add_item(new VDriverItem(this, CAPTURE_FIREWIRE_TITLE, CAPTURE_FIREWIRE));
		add_item(new VDriverItem(this, CAPTURE_IEC61883_TITLE, CAPTURE_IEC61883));
#endif
#ifdef HAVE_DVB
		add_item(new VDriverItem(this, CAPTURE_DVB_TITLE, CAPTURE_DVB));
#endif
	}
	else
	{
		add_item(new VDriverItem(this, PLAYBACK_X11_TITLE, PLAYBACK_X11));
		add_item(new VDriverItem(this, PLAYBACK_X11_XV_TITLE, PLAYBACK_X11_XV));
#ifdef HAVE_GL
// Check runtime glx version. pbuffer needs >= 1.3
		if(get_opengl_server_version() >= 103)
			add_item(new VDriverItem(this, PLAYBACK_X11_GL_TITLE, PLAYBACK_X11_GL));
#endif
#ifdef HAVE_FIREWIRE
		add_item(new VDriverItem(this, PLAYBACK_FIREWIRE_TITLE, PLAYBACK_FIREWIRE));
		add_item(new VDriverItem(this, PLAYBACK_DV1394_TITLE, PLAYBACK_DV1394));
		add_item(new VDriverItem(this, PLAYBACK_IEC61883_TITLE, PLAYBACK_IEC61883));
#endif
	}
}


VDriverItem::VDriverItem(VDriverMenu *popup, const char *text, int driver)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->driver = driver;
}

VDriverItem::~VDriverItem()
{
}

int VDriverItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = driver;
	popup->device_prefs->initialize(0);
	popup->device_prefs->pwindow->show_dialog();
	return 1;
}




VDeviceTextBox::VDeviceTextBox(int x, int y, char *output)
 : BC_TextBox(x, y, 200, 1, output)
{
	this->output = output;
}

int VDeviceTextBox::handle_event()
{
// Suggestions
	calculate_suggestions(0);

	strcpy(output, get_text());
	return 1;
}

VDeviceTumbleBox::VDeviceTumbleBox(VDevicePrefs *prefs,
	int x, int y, int *output, int min, int max, int text_w)
 : BC_TumbleTextBox(prefs->dialog, *output, min, max, x, y, text_w)
{
	this->output = output;
}

int VDeviceTumbleBox::handle_event()
{
	*output = atol(get_text());
	return 1;
}






VDeviceIntBox::VDeviceIntBox(int x, int y, int *output)
 : BC_TextBox(x, y, 60, 1, *output)
{
	this->output = output;
}

int VDeviceIntBox::handle_event()
{
	*output = atol(get_text());
	return 1;
}





VDeviceCheckBox::VDeviceCheckBox(int x, int y, int *output, char *text)
 : BC_CheckBox(x, y, *output, text)
{
	this->output = output;
}
int VDeviceCheckBox::handle_event()
{
	*output = get_value();
	return 1;
}




VScalingItem::VScalingItem(VScalingEquation *popup, int interpolation)
 : BC_MenuItem(popup->interpolation_to_string(interpolation))
{
	this->popup = popup;
	this->interpolation = interpolation;
}

VScalingItem::~VScalingItem()
{
}

int VScalingItem::handle_event()
{
	popup->set_text(get_text());
	*(popup->output) = interpolation;
	return 1;
}


VScalingEquation::VScalingEquation(int x, int y, int *output)
 : BC_PopupMenu(x, y, 240, interpolation_to_string(*output))
{
	this->output = output;
}

VScalingEquation::~VScalingEquation()
{
}

const char *VScalingEquation::interpolation_to_string(int type)
{
	switch( type ) {
	case NEAREST_NEIGHBOR:  return _("Nearest Neighbor");
	case CUBIC_CUBIC:	return _("BiCubic / BiCubic");
	case CUBIC_LINEAR:	return _("BiCubic / BiLinear");
	case LINEAR_LINEAR:	return _("BiLinear / BiLinear");
	case LANCZOS_LANCZOS:	return _("Lanczos / Lanczos");
	}
	return _("Unknown");
}

void VScalingEquation::create_objects()
{
	add_item(new VScalingItem(this, NEAREST_NEIGHBOR));
	add_item(new VScalingItem(this, CUBIC_CUBIC));
	add_item(new VScalingItem(this, CUBIC_LINEAR));
	add_item(new VScalingItem(this, LINEAR_LINEAR));
	add_item(new VScalingItem(this, LANCZOS_LANCZOS));
}

