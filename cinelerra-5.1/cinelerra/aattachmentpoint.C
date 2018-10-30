
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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

#include "aattachmentpoint.h"
#include "bcsignals.h"
#include "datatype.h"
#include "edl.h"
#include "edlsession.h"
#include "playbackconfig.h"
#include "plugin.h"
#include "pluginserver.h"
#include "renderengine.h"
#include "samples.h"
#include "transportque.h"

AAttachmentPoint::AAttachmentPoint(RenderEngine *renderengine, Plugin *plugin)
: AttachmentPoint(renderengine, plugin, TRACK_AUDIO)
{
	buffer_vector = 0;
	buffer_allocation = 0;
}

AAttachmentPoint::~AAttachmentPoint()
{
	delete_buffer_vector();
}

void AAttachmentPoint::delete_buffer_vector()
{
	if(buffer_vector)
	{
		for(int i = 0; i < virtual_plugins.total; i++)
			delete buffer_vector[i];
		delete [] buffer_vector;
	}
	buffer_vector = 0;
	buffer_allocation = 0;
}

void AAttachmentPoint::new_buffer_vector(int total, int size)
{
	if(buffer_vector && size > buffer_allocation)
		delete_buffer_vector();

	if(!buffer_vector)
	{
		buffer_allocation = size;
		buffer_vector = new Samples*[virtual_plugins.total];
		for(int i = 0; i < virtual_plugins.total; i++)
		{
			buffer_vector[i] = new Samples(buffer_allocation);
		}
	}
}

int AAttachmentPoint::get_buffer_size()
{
	return renderengine->config->aconfig->fragment_size;
// must be greater than value audio_read_length, calculated in PackageRenderer::create_engine
// if it is not, plugin's PluginClient::in_buffer_size is below the real maximum and
// we get a crush on rendering of audio plugins!
// 	int audio_read_length = renderengine->command->get_edl()->session->sample_rate;
// 	int fragment_size = renderengine->config->aconfig->fragment_size;
// 	if(audio_read_length > fragment_size)
// 		return audio_read_length;
// 	else
// 		return fragment_size;
}

void AAttachmentPoint::render(Samples *output,
	int buffer_number,
	int64_t start_position,
	int64_t len,
	int64_t sample_rate)
{
	if(!plugin_server || !plugin->on) return;

	if(plugin_server->multichannel)
	{
// Test against previous parameters for reuse of previous data
		if( !is_processed || this->start_position != start_position ||
			this->len != len || this->sample_rate != sample_rate ) {
// Update status
			this->start_position = start_position;
			this->len = len;
			this->sample_rate = sample_rate;
			is_processed = 1;

// Allocate buffer vector
			new_buffer_vector(virtual_plugins.total, len);

// Process plugin
			plugin_servers.values[0]->process_buffer(
				buffer_vector, start_position, len, sample_rate,
				plugin->length * sample_rate /
					renderengine->get_edl()->session->sample_rate,
				renderengine->command->get_direction());
		}
		memcpy(output->get_data(),
			buffer_vector[buffer_number]->get_data(),
			sizeof(double) * len);
	}
	else
	{
// Process plugin
		Samples *output_temp[1];
		output_temp[0] = output;

if(0) printf("AAttachmentPoint::render %d buffer_number=%d renderengine=%p plugin_server=%p\n",
__LINE__,
buffer_number,
renderengine,
plugin_servers.values[buffer_number]);
		plugin_servers.values[buffer_number]->process_buffer(output_temp,
			start_position,
			len,
			sample_rate,
			plugin->length *
				sample_rate /
				renderengine->get_edl()->session->sample_rate,
			renderengine->command->get_direction());
	}
}


