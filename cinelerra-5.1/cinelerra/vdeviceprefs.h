
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

#ifndef VDEVICEPREFS_H
#define VDEVICEPREFS_H

// Modes
#ifndef MODEPLAY
#define MODEPLAY 0
#define MODERECORD 1
#define MODEDUPLEX 2
#endif

#include "adeviceprefs.inc"
#include "channelpicker.inc"
#include "guicast.h"
#include "playbackconfig.inc"
#include "preferencesthread.inc"
#include "recordconfig.inc"
#include "vdeviceprefs.inc"

class VDeviceCheckBox;
class VDeviceTextBox;
class VDeviceIntBox;
class VDeviceTumbleBox;
class VDriverMenu;

class VDevicePrefs
{
public:
	VDevicePrefs(int x,
		int y,
		PreferencesWindow *pwindow,
		PreferencesDialog *dialog,
		VideoOutConfig *out_config,
		VideoInConfig *in_config,
		int mode);
	~VDevicePrefs();

// creation - set if this is the first initialize of the object
//            to prevent file format from being overwritten
//            & window from being flushed
	int initialize(int creation /* = 0 */);
	int delete_objects();
	void reset_objects();
	int get_h();

	PreferencesWindow *pwindow;
	PreferencesDialog *dialog;
	VideoOutConfig *out_config;
	VideoInConfig *in_config;
	PrefsChannelPicker *channel_picker;

private:
	int create_firewire_objs();
	int create_dv1394_objs();
	int create_v4l2_objs();
	int create_v4l2jpeg_objs();
	int create_v4l2mpeg_objs();
	int create_screencap_objs();
	int create_x11_objs();
	void create_dvb_objs();

	VDriverMenu *menu;

	BC_Title *device_title;
	BC_Title *port_title;
	BC_CheckBox *follow_video_config;
	BC_CheckBox *use_direct_x11;
	BC_Title *channel_title;
	BC_Title *output_title;
	BC_Title *syt_title;
	BC_Title *fields_title;
	BC_Title *dvb_adapter_title;

	VDeviceTextBox *device_text;
	VDeviceIntBox *firewire_port;
	VDeviceIntBox *firewire_channel;
	VDeviceIntBox *firewire_channels;
	VDeviceIntBox *firewire_syt;
	VDeviceTextBox *firewire_path;
	VDeviceTumbleBox *device_fields;
	VDeviceTumbleBox *dvb_adapter_device;

	int driver, mode;
	int x;
	int y;
};

class VDeviceTextBox : public BC_TextBox
{
public:
	VDeviceTextBox(int x, int y, char *output);

	int handle_event();
	char *output;
};

class VDeviceIntBox : public BC_TextBox
{
public:
	VDeviceIntBox(int x, int y, int *output);

	int handle_event();
	int *output;
};

class VDeviceTumbleBox : public BC_TumbleTextBox
{
public:
	VDeviceTumbleBox(VDevicePrefs *prefs,
		int x, int y, int *output, int min, int max, int text_w=60);

	int handle_event();
	int *output;
};

class VDeviceCheckBox : public BC_CheckBox
{
public:
	VDeviceCheckBox(int x, int y, int *output, char *text);

	int handle_event();
	int *output;
};


class VDriverMenu : public BC_PopupMenu
{
public:
	VDriverMenu(int x,
		int y,
		VDevicePrefs *device_prefs,
		int do_input,
		int *output);
	~VDriverMenu();

	char* driver_to_string(int driver);
	void create_objects();

	VDevicePrefs *device_prefs;
	int do_input;
	int *output;
	char string[BCTEXTLEN];
};


class VDriverItem : public BC_MenuItem
{
public:
	VDriverItem(VDriverMenu *popup, const char *text, int driver);
	~VDriverItem();

	int handle_event();

	VDriverMenu *popup;
	int driver;
};


class VScalingEquation : public BC_PopupMenu
{
public:
	VScalingEquation(int x, int y, int *output);
	~VScalingEquation();

	const char* interpolation_to_string(int type);
	void create_objects();
	int *output;
};

class VScalingItem : public BC_MenuItem
{
public:
	VScalingItem(VScalingEquation *popup, int interpolation);
	~VScalingItem();

	VScalingEquation *popup;
	int handle_event();
	int interpolation;
};


#endif
