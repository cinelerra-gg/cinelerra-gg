
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

#include "clip.h"
#include "bchash.h"
#include "dcoffset.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "transportque.h"

#include "vframe.h"

#include <string.h>


#define WINDOW_SIZE 65536

REGISTER_PLUGIN(DCOffset)









DCOffset::DCOffset(PluginServer *server)
 : PluginAClient(server)
{
	need_collection = 1;
	reference = 0;
}

DCOffset::~DCOffset()
{
	delete reference;
}

const char* DCOffset::plugin_title() { return N_("DC Offset"); }
int DCOffset::is_realtime() { return 1; }



int DCOffset::process_buffer(int64_t size,
	Samples *buffer,
	int64_t start_position,
	int sample_rate)
{
	load_configuration();
	double *buffer_samples = buffer->get_data();
	double *reference_samples = !reference ? 0 : reference->get_data();

	if(need_collection)
	{
		if(!reference) {
			reference = new Samples(WINDOW_SIZE);
			reference_samples = reference->get_data();
		}
		int64_t collection_start = 0;
		if(get_direction() == PLAY_REVERSE)
			collection_start += WINDOW_SIZE;
		read_samples(reference,
			0,
			sample_rate,
			collection_start,
			WINDOW_SIZE);
		offset = 0;
		for(int i = 0; i < WINDOW_SIZE; i++)
		{
			offset += reference_samples[i];
		}
		offset /= WINDOW_SIZE;
		need_collection = 0;
	}

	read_samples(buffer,
		0,
		sample_rate,
		start_position,
		size);

	for(int i = 0; i < size; i++)
	{
		buffer_samples[i] -= offset;
	}

	return 0;
}



