
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

#ifndef STRACK_H
#define STRACK_H

#include "arraylist.h"
#include "automation.h"
#include "autoconf.inc"
#include "bcwindowbase.inc"
#include "edl.inc"
#include "edit.h"
#include "edits.h"
#include "filexml.inc"
#include "floatautos.inc"
#include "linklist.h"
#include "maxchannels.h"
#include "mwindow.h"
#include "panautos.inc"
#include "patchbay.h"
#include "patchgui.h"
#include "track.h"




class STrack : public Track
{
public:
	STrack(EDL *edl, Tracks *tracks);
	STrack() {};
	~STrack();

	int load_defaults(BC_Hash *defaults);
	void synchronize_params(Track *track);
	int copy_settings(Track *track);
	int save_header(FileXML *file);
	int save_derived(FileXML *file);
	int load_header(FileXML *file, uint32_t load_flags);
	int load_derived(FileXML *file, uint32_t load_flags);
	void create_objects();
	void set_default_title();
	int vertical_span(Theme *theme);
	int64_t to_units(double position, int round);
	double to_doubleunits(double position);
	double from_units(int64_t position);
	int get_dimensions(int pane_number,
		double &view_start,
		double &view_units,
		double &zoom_units);
	int identical(int64_t sample1, int64_t sample2);
	int64_t length();
};

class SEdits;

class SEdit : public Edit
{
	char text[BCTEXTLEN];
public:
	SEdit(EDL *edl, Edits *edits);
	~SEdit();

	void copy_from(Edit *edit);
	int load_properties_derived(FileXML *xml);
	int copy_properties_derived(FileXML *xml, int64_t length_in_selection);
	int dump_derived();
	int64_t get_source_end(int64_t default_);
	char *get_text() { return text; }
};

class SEdits : public Edits {
public:
	SEdits(EDL *edl, Track *track) : Edits(edl, track) {}
	Edit* create_edit() { return new SEdit(edl, this); }
	Edit* append_new_edit() { return append(create_edit()); }
	Edit* insert_edit_after(Edit* previous_edit) {
		return insert_after(previous_edit, create_edit());
	}
        //int optimize() { return 0; }
};


class SAutomation : public Automation {
public:
	SAutomation(EDL *edl, Track *track) : Automation(edl, track) {}
	~SAutomation() {}
	void create_objects() { Automation::create_objects(); }
};

class SPatchGUI : public PatchGUI
{
public:
	SPatchGUI(MWindow *mwindow, PatchBay *patchbay, STrack *track, int x, int y) :
 		PatchGUI(mwindow, patchbay, track, x, y) { data_type = TRACK_SUBTITLE; }
	~SPatchGUI() {}

	void create_objects() { update(x, y); }
	int reposition(int x, int y) { return PatchGUI::reposition(x, y); }
	int update(int x, int y) {
		return track->vertical_span(mwindow->theme) + PatchGUI::update(x, y);
	}
};

#endif
