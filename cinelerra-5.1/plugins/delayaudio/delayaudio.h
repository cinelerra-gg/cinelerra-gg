
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

#ifndef DELAYAUDIO_H
#define DELAYAUDIO_H

#include "bchash.inc"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"



class DelayAudio;
class DelayAudioWindow;
class DelayAudioTextBox;




class DelayAudioConfig
{
public:
	DelayAudioConfig();

	int equivalent(DelayAudioConfig &that);
	void copy_from(DelayAudioConfig &that);
	double length;
};






class DelayAudioWindow : public PluginClientWindow
{
public:
	DelayAudioWindow(DelayAudio *plugin);
	~DelayAudioWindow();

	void create_objects();
	void update_gui();

	DelayAudio *plugin;
	DelayAudioTextBox *length;
};




class DelayAudioTextBox : public BC_TumbleTextBox
{
public:
	DelayAudioTextBox(DelayAudio *plugin, DelayAudioWindow *window, int x, int y);
	~DelayAudioTextBox();

	int handle_event();

	DelayAudio *plugin;
};




class DelayAudio : public PluginAClient
{
public:
	DelayAudio(PluginServer *server);
	~DelayAudio();

	int is_realtime();
	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	int process_realtime(int64_t size, Samples *input_ptr, Samples *output_ptr);




	PLUGIN_CLASS_MEMBERS(DelayAudioConfig);
	void reset();
	void reconfigure();
	void update_gui();



	Samples *buffer;
	int64_t allocation;
	int64_t input_start;
	int need_reconfigure;
};




#endif
