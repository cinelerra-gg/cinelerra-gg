#include "clip.h"
#include "edit.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "datatype.h"
#include "filexml.h"
#include "localsession.h"
#include "mainsession.h"
#include "strack.h"
#include "theme.h"
#include "trackcanvas.h"
#include "tracks.h"

#include <string.h>

STrack::STrack(EDL *edl, Tracks *tracks)
 : Track(edl, tracks)
{
	data_type = TRACK_SUBTITLE;
}

STrack::~STrack()
{
}


// Used by PlaybackEngine
void STrack::synchronize_params(Track *track)
{
	Track::synchronize_params(track);
}

int STrack::copy_settings(Track *track)
{
	Track::copy_settings(track);
	return 0;
}

int STrack::load_defaults(BC_Hash *defaults)
{
	Track::load_defaults(defaults);
	return 0;
}

void STrack::set_default_title()
{
	Track *current = ListItem<Track>::list->first;
	int i;
	for(i = 0; current; current = NEXT)
	{
		if(current->data_type == TRACK_SUBTITLE) i++;
	}
	sprintf(title, _("Subttl %d"), i);
}

void STrack::create_objects()
{
	Track::create_objects();
	automation = new SAutomation(edl, this);
	automation->create_objects();
	edits = new SEdits(edl, this);
}

int STrack::vertical_span(Theme *theme)
{
	int track_h = Track::vertical_span(theme);
	int patch_h = 0;
	return track_h + patch_h;
}


int64_t STrack::to_units(double position, int round)
{
        if(round)
        {
                return Units::round(position * edl->session->frame_rate);
        }
        else
        {
// Kludge for rounding errors, just on a smaller scale than formal rounding
                position *= edl->session->frame_rate;
                return Units::to_int64(position);
        }
}

double STrack::to_doubleunits(double position)
{
        return position * edl->session->frame_rate;
}


double STrack::from_units(int64_t position)
{
        return (double)position / edl->session->frame_rate;
}




int STrack::identical(int64_t sample1, int64_t sample2)
{
// Units of frames
	if(labs(sample1 - sample2) <= 1) return 1; else return 0;
}

int STrack::save_header(FileXML *file)
{
	file->tag.set_property("TYPE", "SUBTTL");
	return 0;
}

int STrack::save_derived(FileXML *file)
{
	file->append_newline();
	return 0;
}

int STrack::load_header(FileXML *file, uint32_t load_flags)
{
	return 0;
}

int STrack::load_derived(FileXML *file, uint32_t load_flags)
{
	return 0;
}


int STrack::get_dimensions(int pane_number,
	double &view_start,
	double &view_units,
	double &zoom_units)
{
	view_start = edl->local_session->view_start[pane_number] * edl->session->frame_rate;
	view_units = 0;
//	view_units = Units::toframes(tracks->view_samples(), mwindow->session->sample_rate, mwindow->session->frame_rate);
	zoom_units = edl->local_session->zoom_sample / edl->session->sample_rate * edl->session->frame_rate;
	return 0;
}

int64_t STrack::length()
{
	return edits->length();
}

SEdit::SEdit(EDL *edl, Edits *edits)
 : Edit(edl, edits)
{
	text[0] = 0;
}

SEdit::~SEdit()
{
}



void SEdit::
copy_from(Edit *edit)
{
	Edit::copy_from(edit);
	SEdit *sedit = (SEdit*)edit;
	strcpy(text,sedit->text);
}

int SEdit::load_properties_derived(FileXML *xml)
{
	xml->tag.get_property("TEXT", text);
	return 0;
}

int SEdit::copy_properties_derived(FileXML *xml, int64_t length_in_selection)
{
	xml->tag.set_property("TEXT", text);
	return 0;
}


int SEdit::dump_derived()
{
	return 0;
}


int64_t SEdit::get_source_end(int64_t default_)
{
	return default_;   // Infinity
}

