
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

#include "automation.h"
#include "awindowgui.h"
#include "clip.h"
#include "bchash.h"
#include "edl.h"
#include "filesystem.h"
#include "filexml.h"
#include "floatauto.h"
#include "language.h"
#include "localsession.h"


static const char *xml_autogrouptypes_titlesmax[] =
{
	"AUTOGROUPTYPE_AUDIO_FADE_MAX",
	"AUTOGROUPTYPE_VIDEO_FADE_MAX",
	"AUTOGROUPTYPE_ZOOM_MAX",
	"AUTOGROUPTYPE_SPEED_MAX",
	"AUTOGROUPTYPE_X_MAX",
	"AUTOGROUPTYPE_Y_MAX",
	"AUTOGROUPTYPE_INT255_MAX"
};

static const char *xml_autogrouptypes_titlesmin[] =
{
	"AUTOGROUPTYPE_AUDIO_FADE_MIN",
	"AUTOGROUPTYPE_VIDEO_FADE_MIN",
	"AUTOGROUPTYPE_ZOOM_MIN",
	"AUTOGROUPTYPE_SPEED_MIN",
	"AUTOGROUPTYPE_X_MIN",
	"AUTOGROUPTYPE_Y_MIN",
	"AUTOGROUPTYPE_INT255_MIN"
};


LocalSession::LocalSession(EDL *edl)
{
	this->edl = edl;

	selectionstart = selectionend = 0;
	in_point = out_point = -1;
	sprintf(clip_title, _("Program"));
	strcpy(clip_notes, _("Hello world"));
	strcpy(clip_icon, "");
	clipboard_length = 0;
	loop_playback = 0;
	loop_start = loop_end = 0;
	playback_start = -1;
	playback_end = 0;
	preview_start = 0;  preview_end = -1;
	zoom_sample = DEFAULT_ZOOM_TIME;
	zoom_y = 0;
	zoom_track = 0;
	x_pane = y_pane = -1;

	for(int i = 0; i < TOTAL_PANES; i++) {
		view_start[i] = 0;
		track_start[i] = 0;
	}

	automation_mins[AUTOGROUPTYPE_AUDIO_FADE] = -80;
	automation_maxs[AUTOGROUPTYPE_AUDIO_FADE] = 6;

	automation_mins[AUTOGROUPTYPE_VIDEO_FADE] = 0;
	automation_maxs[AUTOGROUPTYPE_VIDEO_FADE] = 100;

	automation_mins[AUTOGROUPTYPE_ZOOM] = 0.005;
	automation_maxs[AUTOGROUPTYPE_ZOOM] = 5.000;

	automation_mins[AUTOGROUPTYPE_SPEED] = 0.005;
	automation_maxs[AUTOGROUPTYPE_SPEED] = 5.000;

	automation_mins[AUTOGROUPTYPE_X] = -100;
	automation_maxs[AUTOGROUPTYPE_X] = 100;

	automation_mins[AUTOGROUPTYPE_Y] = -100;
	automation_maxs[AUTOGROUPTYPE_Y] = 100;

	automation_mins[AUTOGROUPTYPE_INT255] = 0;
	automation_maxs[AUTOGROUPTYPE_INT255] = 255;

	zoombar_showautotype = AUTOGROUPTYPE_AUDIO_FADE;
	zoombar_showautocolor = -1;

	floatauto_type = FloatAuto::SMOOTH;

	red = green = blue = 0;
	red_max = green_max = blue_max = 0;
	use_max = 0;
}

LocalSession::~LocalSession()
{
}

void LocalSession::copy_from(LocalSession *that)
{
	strcpy(clip_title, that->clip_title);
	strcpy(clip_notes, that->clip_notes);
	strcpy(clip_icon, that->clip_icon);
	in_point = that->in_point;
	loop_playback = that->loop_playback;
	loop_start = that->loop_start;
	loop_end = that->loop_end;
	out_point = that->out_point;
	selectionend = that->selectionend;
	selectionstart = that->selectionstart;
	x_pane = that->x_pane;
	y_pane = that->y_pane;

	for(int i = 0; i < TOTAL_PANES; i++)
	{
		view_start[i] = that->view_start[i];
		track_start[i] = that->track_start[i];
	}

	zoom_sample = that->zoom_sample;
	zoom_y = that->zoom_y;
	zoom_track = that->zoom_track;
	preview_start = that->preview_start;
	preview_end = that->preview_end;
	red = that->red;
	green = that->green;
	blue = that->blue;
	red_max = that->red_max;
	green_max = that->green_max;
	blue_max = that->blue_max;
	use_max = that->use_max;

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		automation_mins[i] = that->automation_mins[i];
		automation_maxs[i] = that->automation_maxs[i];
	}
	floatauto_type = that->floatauto_type;
}

void LocalSession::save_xml(FileXML *file, double start)
{
	file->tag.set_title("LOCALSESSION");

	file->tag.set_property("IN_POINT", in_point - start);
	file->tag.set_property("LOOP_PLAYBACK", loop_playback);
	file->tag.set_property("LOOP_START", loop_start - start);
	file->tag.set_property("LOOP_END", loop_end - start);
	file->tag.set_property("OUT_POINT", out_point - start);
	file->tag.set_property("SELECTION_START", selectionstart - start);
	file->tag.set_property("SELECTION_END", selectionend - start);
	file->tag.set_property("CLIP_TITLE", clip_title);
	file->tag.set_property("CLIP_ICON", clip_icon);
	file->tag.set_property("X_PANE", x_pane);
	file->tag.set_property("Y_PANE", y_pane);

	char string[BCTEXTLEN];
	for(int i = 0; i < TOTAL_PANES; i++)
	{
		sprintf(string, "TRACK_START%d", i);
		file->tag.set_property(string, track_start[i]);
		sprintf(string, "VIEW_START%d", i);
		file->tag.set_property(string, view_start[i]);
	}

	file->tag.set_property("ZOOM_SAMPLE", zoom_sample);
//printf("EDLSession::save_session 1\n");
	file->tag.set_property("ZOOMY", zoom_y);
//printf("EDLSession::save_session 1 %d\n", zoom_track);
	file->tag.set_property("ZOOM_TRACK", zoom_track);

	double preview_start = this->preview_start - start;
	if( preview_start < 0 ) preview_start = 0;
	double preview_end = this->preview_end - start;
	if( preview_end < preview_start ) preview_end = -1;
	file->tag.set_property("PREVIEW_START", preview_start);
	file->tag.set_property("PREVIEW_END", preview_end);
	file->tag.set_property("FLOATAUTO_TYPE", floatauto_type);

	file->tag.set_property("RED", red);
	file->tag.set_property("GREEN", green);
	file->tag.set_property("BLUE", blue);
	file->tag.set_property("RED_MAX", red_max);
	file->tag.set_property("GREEN_MAX", green_max);
	file->tag.set_property("BLUE_MAX", blue_max);
	file->tag.set_property("USE_MAX", use_max);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			file->tag.set_property(xml_autogrouptypes_titlesmin[i],automation_mins[i]);
			file->tag.set_property(xml_autogrouptypes_titlesmax[i],automation_maxs[i]);
		}
	}
	file->append_tag();
	file->append_newline();

//this used to be a property, now used as tag member
//	file->tag.set_property("CLIP_NOTES", clip_notes);
	file->tag.set_title("CLIP_NOTES");   file->append_tag();
	file->append_text(clip_notes);
	file->tag.set_title("/CLIP_NOTES");  file->append_tag();
	file->append_newline();

	file->tag.set_title("/LOCALSESSION");
	file->append_tag();
	file->append_newline();
	file->append_newline();
}

void LocalSession::synchronize_params(LocalSession *that)
{
	loop_playback = that->loop_playback;
	loop_start = that->loop_start;
	loop_end = that->loop_end;
	preview_start = that->preview_start;
	preview_end = that->preview_end;
	red = that->red;
	green = that->green;
	blue = that->blue;
	red_max = that->red_max;
	green_max = that->green_max;
	blue_max = that->blue_max;
}


void LocalSession::load_xml(FileXML *file, unsigned long load_flags)
{
	if(load_flags & LOAD_SESSION)
	{
// moved to EDL::load_xml for paste to fill silence.
//		clipboard_length = 0;
// Overwritten by MWindow::load_filenames
		file->tag.get_property("CLIP_TITLE", clip_title);
		clip_notes[0] = 0;
		file->tag.get_property("CLIP_NOTES", clip_notes);
		clip_icon[0] = 0;
		file->tag.get_property("CLIP_ICON", clip_icon);
// kludge if possible
		if( !clip_icon[0] ) {
			char *cp = clip_notes;
			int year, mon, mday, hour, min, sec;
			while( *cp && *cp++ != ':' );
			if( *cp && sscanf(cp, "%d/%02d/%02d %02d:%02d:%02d,",
					&year, &mon, &mday, &hour, &min, &sec) == 6 ) {
				sprintf(clip_icon, "clip_%02d%02d%02d-%02d%02d%02d.png",
					year, mon, mday, hour, min, sec);
			}
		}
		loop_playback = file->tag.get_property("LOOP_PLAYBACK", 0);
		loop_start = file->tag.get_property("LOOP_START", (double)0);
		loop_end = file->tag.get_property("LOOP_END", (double)0);
		selectionstart = file->tag.get_property("SELECTION_START", (double)0);
		selectionend = file->tag.get_property("SELECTION_END", (double)0);
		x_pane = file->tag.get_property("X_PANE", -1);
		y_pane = file->tag.get_property("Y_PANE", -1);


		char string[BCTEXTLEN];
		for(int i = 0; i < TOTAL_PANES; i++)
		{
			sprintf(string, "TRACK_START%d", i);
			track_start[i] = file->tag.get_property(string, track_start[i]);
			sprintf(string, "VIEW_START%d", i);
			view_start[i] = file->tag.get_property(string, view_start[i]);
		}

		zoom_sample = file->tag.get_property("ZOOM_SAMPLE", zoom_sample);
		zoom_y = file->tag.get_property("ZOOMY", zoom_y);
		zoom_track = file->tag.get_property("ZOOM_TRACK", zoom_track);
		preview_start = file->tag.get_property("PREVIEW_START", preview_start);
		preview_end = file->tag.get_property("PREVIEW_END", preview_end);
		red = file->tag.get_property("RED", red);
		green = file->tag.get_property("GREEN", green);
		blue = file->tag.get_property("BLUE", blue);
		red_max = file->tag.get_property("RED_MAX", red_max);
		green_max = file->tag.get_property("GREEN_MAX", green_max);
		blue_max = file->tag.get_property("BLUE_MAX", blue_max);
		use_max = file->tag.get_property("USE_MAX", use_max);

		for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
			if (!Automation::autogrouptypes_fixedrange[i]) {
				automation_mins[i] = file->tag.get_property(xml_autogrouptypes_titlesmin[i],automation_mins[i]);
				automation_maxs[i] = file->tag.get_property(xml_autogrouptypes_titlesmax[i],automation_maxs[i]);
			}
		}
		floatauto_type = file->tag.get_property("FLOATAUTO_TYPE", floatauto_type);
	}


// on operations like cut, paste, slice, clear... we should also undo the cursor position as users
// expect - this is additionally important in keyboard-only editing in viewer window
	if(load_flags & LOAD_SESSION || load_flags & LOAD_TIMEBAR)
	{
		selectionstart = file->tag.get_property("SELECTION_START", (double)0);
		selectionend = file->tag.get_property("SELECTION_END", (double)0);
	}



	if(load_flags & LOAD_TIMEBAR)
	{
		in_point = file->tag.get_property("IN_POINT", (double)-1);
		out_point = file->tag.get_property("OUT_POINT", (double)-1);
	}

	while( !file->read_tag() ) {
		if( file->tag.title_is("/LOCALSESSION") ) break;
		if( file->tag.title_is("CLIP_NOTES") ) {
			XMLBuffer notes;
			file->read_text_until("/CLIP_NOTES", &notes, 1);
			memset(clip_notes, 0, sizeof(clip_notes));
			strncpy(clip_notes, notes.cstr(), sizeof(clip_notes)-1);
		}
	}
}

void LocalSession::boundaries()
{
	zoom_sample = MAX(1, zoom_sample);
}

int LocalSession::load_defaults(BC_Hash *defaults)
{
	loop_playback = defaults->get("LOOP_PLAYBACK", 0);
	loop_start = defaults->get("LOOP_START", (double)0);
	loop_end = defaults->get("LOOP_END", (double)0);
	selectionstart = defaults->get("SELECTIONSTART", selectionstart);
	selectionend = defaults->get("SELECTIONEND", selectionend);
//	track_start = defaults->get("TRACK_START", 0);
//	view_start = defaults->get("VIEW_START", 0);
	zoom_sample = defaults->get("ZOOM_SAMPLE", DEFAULT_ZOOM_TIME);
	zoom_y = defaults->get("ZOOMY", 64);
	zoom_track = defaults->get("ZOOM_TRACK", 64);
	red = defaults->get("RED", 0.0);
	green = defaults->get("GREEN", 0.0);
	blue = defaults->get("BLUE", 0.0);
	red_max = defaults->get("RED_MAX", 0.0);
	green_max = defaults->get("GREEN_MAX", 0.0);
	blue_max = defaults->get("BLUE_MAX", 0.0);
	use_max = defaults->get("USE_MAX", 0);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			automation_mins[i] = defaults->get(xml_autogrouptypes_titlesmin[i], automation_mins[i]);
			automation_maxs[i] = defaults->get(xml_autogrouptypes_titlesmax[i], automation_maxs[i]);
		}
	}

	floatauto_type = defaults->get("FLOATAUTO_TYPE", floatauto_type);
	x_pane = defaults->get("X_PANE", x_pane);
	y_pane = defaults->get("Y_PANE", y_pane);

	return 0;
}

int LocalSession::save_defaults(BC_Hash *defaults)
{
	defaults->update("LOOP_PLAYBACK", loop_playback);
	defaults->update("LOOP_START", loop_start);
	defaults->update("LOOP_END", loop_end);
	defaults->update("SELECTIONSTART", selectionstart);
	defaults->update("SELECTIONEND", selectionend);
//	defaults->update("TRACK_START", track_start);
//	defaults->update("VIEW_START", view_start);
	defaults->update("ZOOM_SAMPLE", zoom_sample);
	defaults->update("ZOOMY", zoom_y);
	defaults->update("ZOOM_TRACK", zoom_track);
	defaults->update("RED", red);
	defaults->update("GREEN", green);
	defaults->update("BLUE", blue);
	defaults->update("RED_MAX", red_max);
	defaults->update("GREEN_MAX", green_max);
	defaults->update("BLUE_MAX", blue_max);
	defaults->update("USE_MAX", use_max);

	for (int i = 0; i < AUTOGROUPTYPE_COUNT; i++) {
		if (!Automation::autogrouptypes_fixedrange[i]) {
			defaults->update(xml_autogrouptypes_titlesmin[i], automation_mins[i]);
			defaults->update(xml_autogrouptypes_titlesmax[i], automation_maxs[i]);
		}
	}

	defaults->update("FLOATAUTO_TYPE", floatauto_type);
	defaults->update("X_PANE", x_pane);
	defaults->update("Y_PANE", y_pane);
	return 0;
}

void LocalSession::set_selectionstart(double value)
{
	this->selectionstart = value;
}

void LocalSession::set_selectionend(double value)
{
	this->selectionend = value;
}

void LocalSession::set_inpoint(double value)
{
	in_point = value;
}

void LocalSession::set_outpoint(double value)
{
	out_point = value;
}

void LocalSession::unset_inpoint()
{
	in_point = -1;
}

void LocalSession::unset_outpoint()
{
	out_point = -1;
}

void LocalSession::set_playback_start(double value)
{
	if( playback_end < 0 ) return;
	playback_start = value;
	playback_end = -1;
}

void LocalSession::set_playback_end(double value)
{
	if( value < playback_start ) {
		if( playback_end < 0 )
			playback_end = playback_start;
		playback_start = value;
	}
	else if( playback_end < 0 || value > playback_end )
		playback_end = value;
}


double LocalSession::get_selectionstart(int highlight_only)
{
	if(highlight_only || !EQUIV(selectionstart, selectionend))
		return selectionstart;

	if(in_point >= 0)
		return in_point;
	else
	if(out_point >= 0)
		return out_point;
	else
		return selectionstart;
}

double LocalSession::get_selectionend(int highlight_only)
{
	if(highlight_only || !EQUIV(selectionstart, selectionend))
		return selectionend;

	if(out_point >= 0)
		return out_point;
	else
	if(in_point >= 0)
		return in_point;
	else
		return selectionend;
}

double LocalSession::get_inpoint()
{
	return in_point;
}

double LocalSession::get_outpoint()
{
	return out_point;
}

int LocalSession::inpoint_valid()
{
	return in_point >= 0;
}

int LocalSession::outpoint_valid()
{
	return out_point >= 0;
}

