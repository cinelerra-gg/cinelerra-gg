
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

#ifndef AMODULE_H
#define AMODULE_H

class AModuleGUI;
class AModuleTitle;
class AModulePan;
class AModuleFade;
class AModuleInv;
class AModuleMute;
class AModuleReset;

#include "aedit.inc"
#include "amodule.inc"
#include "aplugin.inc"
#include "asset.inc"
#include "datatype.h"
#include "edl.inc"
#include "file.inc"
#include "filexml.inc"
#include "floatautos.inc"
#include "maxchannels.h"
#include "meterhistory.h"
#include "module.h"
#include "resample.h"
#include "samples.inc"
#include "sharedlocation.inc"
#include "track.inc"
#include "units.h"

class AModuleResample : public Resample
{
public:
	AModuleResample(AModule *module);
	~AModuleResample();

// All positions are in source sample rate
	int read_samples(Samples *buffer, int64_t start, int64_t len);

	AModule *module;
// output for nested EDL if resampled
	Samples *nested_output[MAX_CHANNELS];
// number of samples allocated
	int nested_allocation;
};

class AModule : public Module
{
public:
	AModule(RenderEngine *renderengine,
		CommonRender *commonrender,
		PluginArray *plugin_array,
	Track *track);
	virtual ~AModule();

	void create_objects();
	CICache* get_cache();

	int import_samples(AEdit *playable_edit,
		int64_t start_project,
		int64_t edit_startproject,
		int64_t edit_startsource,
		int direction,
		int sample_rate,
		Samples *buffer,
		int64_t fragment_len);


	int render(Samples *buffer,
		int64_t input_len,
		int64_t input_position,
		int direction,
		int sample_rate,
		int use_nudge);
	int get_buffer_size();

	AttachmentPoint* new_attachment(Plugin *plugin);




// synchronization with tracks
	FloatAutos* get_pan_automation(int channel);  // get pan automation
	FloatAutos* get_fade_automation();       // get the fade automation for this module

	MeterHistory *meter_history;

// Temporary buffer for rendering transitions
	Samples *transition_temp;
// Temporary buffer for rendering speed curve
	Samples *speed_temp;
// Previous buffers for rendering speed curve
#define SPEED_OVERLAP 4
	double prev_head[SPEED_OVERLAP];
	double prev_tail[SPEED_OVERLAP];

// Pointer to an asset for the resampler
	Asset *asset;
// Channel to import from the source
	int channel;
// File being read if source is a file.
	File *file;

// Temporary buffer for rendering a nested audio EDL.
// Sharing nested output between modules wouldn't work because if the
// segment was cut inside input_len, the shared channels would have to
// initialize their EDL's at different starting points and botch their
// starting states.
	Samples *nested_output[MAX_CHANNELS];
// number of samples allocated
	int nested_allocation;
// resampling for nested output
	AModuleResample *resample;
};


#endif

