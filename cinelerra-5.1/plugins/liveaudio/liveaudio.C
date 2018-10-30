
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

#include "asset.h"
#include "audiodevice.h"
#include "bcdisplayinfo.h"
#include "bcsignals.h"
#include "clip.h"
#include "bchash.h"
#include "edlsession.h"
#include "filexml.h"
#include "guicast.h"
#include "language.h"
#include "mwindow.h"
#include "pluginaclient.h"
#include "pluginserver.h"
#include "recordconfig.h"
#include "samples.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>
#include <stdint.h>

class LiveAudio;
class LiveAudioWindow;


class LiveAudioConfig
{
public:
	LiveAudioConfig();
};






class LiveAudioWindow : public PluginClientWindow
{
public:
	LiveAudioWindow(LiveAudio *plugin);
	~LiveAudioWindow();

	void create_objects();

	LiveAudio *plugin;
};






class LiveAudio : public PluginAClient
{
public:
	LiveAudio(PluginServer *server);
	~LiveAudio();


	PLUGIN_CLASS_MEMBERS(LiveAudioConfig);

	int process_buffer(int64_t size,
		Samples **buffer,
		int64_t start_position,
		int sample_rate);
	int is_realtime();
	int is_multichannel();
	int is_synthesis();
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
	void update_gui();
	void render_stop();

	AudioDevice *adevice;

	Samples **history;
	int history_ptr;
	int history_channels;
	int64_t history_position;
	int history_size;
// Fragment size from the EDL session
	int fragment_size;
};












LiveAudioConfig::LiveAudioConfig()
{
}








LiveAudioWindow::LiveAudioWindow(LiveAudio *plugin)
 : PluginClientWindow(plugin,
	300,
	160,
	300,
	160,
	0)
{
	this->plugin = plugin;
}

LiveAudioWindow::~LiveAudioWindow()
{
}

void LiveAudioWindow::create_objects()
{
	int x = 10, y = 10;

	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Live audio")));
	show_window();
	flush();
}






















REGISTER_PLUGIN(LiveAudio)






LiveAudio::LiveAudio(PluginServer *server)
 : PluginAClient(server)
{
	adevice = 0;
	history = 0;
	history_channels = 0;
	history_ptr = 0;
	history_position = 0;
	history_size = 0;

	fragment_size = 0;
}


LiveAudio::~LiveAudio()
{
	if(adevice)
	{
		adevice->interrupt_crash();
		adevice->close_all();
	}
	delete adevice;
	if(history)
	{
		for(int i = 0; i < history_channels; i++)
			delete history[i];
		delete [] history;
	}
}



int LiveAudio::process_buffer(int64_t size,
	Samples **buffer,
	int64_t start_position,
	int sample_rate)
{
	load_configuration();

// printf("LiveAudio::process_buffer 10 size=%lld buffer_size=%d channels=%d size=%d\n",
// size, get_buffer_size(), get_total_buffers(), size);

	int first_buffer = 0;

	if(!adevice)
	{
		EDLSession *session = PluginClient::get_edlsession();
		if(session)
		{
			fragment_size = session->record_fragment_size;
			history_channels = session->aconfig_in->channels;

			adevice = new AudioDevice(server ? server->mwindow : 0);
// Take fragment size & channels from the recording config
			adevice->open_input(session->aconfig_in,
				session->vconfig_in,
				get_project_samplerate(),
				fragment_size,
				session->aconfig_in->channels,
				session->real_time_record);
			adevice->start_recording();
			first_buffer = 1;
			history_position = start_position;
		}
	}

	if(!history || history[0]->get_allocated() < size)
	{
// compute new history which is a multiple of our fragment size
		int new_history_size = fragment_size;
		while(new_history_size < size)
			new_history_size += fragment_size;


		if(!history)
		{
			history = new Samples*[history_channels];
			for(int i = 0; i < history_channels; i++)
				history[i] = 0;
		}

		for(int i = 0; i < history_channels; i++)
		{
			delete history[i];
			history[i] = new Samples(new_history_size);
			bzero(history[i]->get_data(), sizeof(double) * new_history_size);
		}
	}



//	if(get_direction() == PLAY_FORWARD)
	{
// Reset history buffer to current position if before maximum history
		if(start_position < history_position - history[0]->get_allocated())
			history_position = start_position;



// Extend history buffer
		int64_t end_position = start_position + size;
// printf("LiveAudio::process_buffer %lld %lld %lld\n",
// end_position,
// history_position,
// end_position - history_position);
		if(end_position > history_position)
		{
// Reset history buffer to current position if after maximum history
			if(start_position >= history_position + history[0]->get_allocated())
				history_position = start_position;
// A delay seems required because ALSA playback may get ahead of
// ALSA recording and never recover.
			if(first_buffer) end_position += sample_rate;
			int done = 0;
			while(!done && history_position < end_position)
			{
				if(history_ptr + fragment_size  >= history[0]->get_allocated())
				{
					done = 1;
				}

// Read rest of buffer from sound driver
				if(adevice)
				{
					int over[history_channels];
					double max[history_channels];
					int result = adevice->read_buffer(history,
						history_channels,
						fragment_size,
						over,
						max,
						history_ptr);
					if( result && adevice->config_updated() )
						adevice->config_update();
				}

				history_ptr += fragment_size;
// wrap around buffer
				if(history_ptr >= history[0]->get_allocated())
					history_ptr = 0;
				history_position += fragment_size;
			}
		}

// Copy data from history buffer
		int buffer_position = 0;
		int history_buffer_ptr = history_ptr - history_position + start_position;
		while(history_buffer_ptr < 0)
			history_buffer_ptr += history[0]->get_allocated();
		while(buffer_position < size)
		{
			int fragment = size;
			if(history_buffer_ptr + fragment > history[0]->get_allocated())
				fragment = history[0]->get_allocated() - history_buffer_ptr;
			if(buffer_position + fragment > size)
				fragment = size - buffer_position;

			for(int i = 0; i < get_total_buffers(); i++)
			{
				int input_channel = i;
// Clamp channel to total history channels
				if(input_channel >= history_channels)
					input_channel = history_channels - 1;
				memcpy(buffer[i]->get_data() + buffer_position,
					history[input_channel]->get_data() + history_buffer_ptr,
					sizeof(double) * fragment);
			}

			history_buffer_ptr += fragment;
			if(history_buffer_ptr >= history[0]->get_allocated())
				history_buffer_ptr = 0;
			buffer_position += fragment;
		}


	}


	return 0;
}

void LiveAudio::render_stop()
{
	if(adevice)
	{
		adevice->interrupt_crash();
		adevice->close_all();
	}
	delete adevice;
	adevice = 0;
	history_ptr = 0;
	history_position = 0;
	history_size = 0;
}


const char* LiveAudio::plugin_title() { return N_("Live Audio"); }
int LiveAudio::is_realtime() { return 1; }
int LiveAudio::is_multichannel() { return 1; }
int LiveAudio::is_synthesis() { return 1; }



NEW_WINDOW_MACRO(LiveAudio, LiveAudioWindow)



int LiveAudio::load_configuration()
{
	return 0;
}


void LiveAudio::save_data(KeyFrame *keyframe)
{
}

void LiveAudio::read_data(KeyFrame *keyframe)
{
}

void LiveAudio::update_gui()
{
}





