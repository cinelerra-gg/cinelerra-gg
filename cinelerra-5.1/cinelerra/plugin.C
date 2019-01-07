
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

#include "bcsignals.h"
#include "edl.h"
#include "edlsession.h"
#include "filexml.h"
#include "keyframe.h"
#include "keyframes.h"
#include "localsession.h"
#include "mwindow.h"
#include "messages.h"
#include "plugin.h"
#include "pluginpopup.h"
#include "pluginset.h"
#include "pluginserver.h"
#include "track.h"
#include "tracks.h"
#include "virtualnode.h"


Plugin::Plugin(EDL *edl, Track *track, const char *title)
 : Edit(edl, track)
{
	is_plugin = 1;
	this->track = track;
	this->plugin_set = 0;
	strcpy(this->title, title);
	plugin_type = PLUGIN_NONE;
	in = 1;
	out = 1;
	show = 0;
	on = 1;
	keyframes = new KeyFrames(edl, track);
	keyframes->create_objects();
}


Plugin::Plugin(EDL *edl, PluginSet *plugin_set, const char *title)
 : Edit(edl, plugin_set)
{
	is_plugin = 1;
	this->track = plugin_set->track;
	this->plugin_set = plugin_set;
	strcpy(this->title, title);
	plugin_type = PLUGIN_NONE;
	in = 1;
	out = 1;
	show = 0;
	on = 1;
	keyframes = new KeyFrames(edl, track);
	keyframes->create_objects();
}

Plugin::~Plugin()
{
	while(keyframes->last) delete keyframes->last;
	delete keyframes;
}

Edit& Plugin::operator=(Edit& edit)
{
	copy_from(&edit);
	return *this;
}

Plugin& Plugin::operator=(Plugin& edit)
{
	copy_from(&edit);
	return *this;
}

int Plugin::operator==(Plugin& that)
{
	return identical(&that);
}

int Plugin::operator==(Edit& that)
{
	return identical((Plugin*)&that);
}

int Plugin::silence()
{
	return plugin_type == PLUGIN_NONE ? 1 : 0;
}

void Plugin::clear_keyframes(int64_t start, int64_t end)
{
	keyframes->clear(start, end, 0);
}


void Plugin::init(const char *title,
	int64_t unit_position, int64_t unit_length, int plugin_type,
	SharedLocation *shared_location, KeyFrame *default_keyframe)
{
	if( title ) strcpy(this->title, title);
	if( shared_location ) this->shared_location = *shared_location;
	this->plugin_type = plugin_type;
	if( default_keyframe )
		*this->keyframes->default_auto = *default_keyframe;
	this->keyframes->default_auto->position = unit_position;
	this->startproject = unit_position;
	this->length = unit_length;
}

void Plugin::copy_base(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;

	this->startsource = edit->startsource;
	this->startproject = edit->startproject;
	this->length = edit->length;


	this->plugin_type = plugin->plugin_type;
	this->in = plugin->in;
	this->out = plugin->out;
	this->show = plugin->show;
	this->on = plugin->on;
// Should reconfigure this based on where the first track is now.
	this->shared_location = plugin->shared_location;
	strcpy(this->title, plugin->title);
}

void Plugin::copy_from(Edit *edit)
{
	copy_base(edit);
	copy_keyframes((Plugin*)edit);
}

void Plugin::copy_keyframes(Plugin *plugin)
{

	keyframes->copy_from(plugin->keyframes);
}

void Plugin::copy_keyframes(int64_t start,
	int64_t end,
	FileXML *file,
	int default_only,
	int active_only)
{
// Only 1 default is copied from where the start position is
	int64_t endproject = startproject + length;
	if(!default_only ||
		(default_only &&
			start < endproject &&
			start >= startproject))
		keyframes->copy(start, end, file, default_only, active_only);
}

void Plugin::synchronize_params(Edit *edit)
{
	Plugin *plugin = (Plugin*)edit;
	this->in = plugin->in;
	this->out = plugin->out;
	this->show = plugin->show;
	this->on = plugin->on;
	strcpy(this->title, plugin->title);
	copy_keyframes(plugin);
}

void Plugin::shift_keyframes(int64_t position)
{
	for(KeyFrame *keyframe = (KeyFrame*)keyframes->first;
		keyframe;
		keyframe = (KeyFrame*)keyframe->next)
	{
		keyframe->position += position;
	}
}


void Plugin::equivalent_output(Edit *edit, int64_t *result)
{
	Plugin *plugin = (Plugin*)edit;
// End of plugin changed
	if(startproject + length != plugin->startproject + plugin->length)
	{
		if(*result < 0 || startproject + length < *result)
			*result = startproject + length;
	}

// Start of plugin changed
	if( startproject != plugin->startproject || plugin_type != plugin->plugin_type ||
	    on != plugin->on || !(shared_location == plugin->shared_location) ||
		strcmp(title, plugin->title) ) {
		if( *result < 0 || startproject < *result )
			*result = startproject;
	}

// Test keyframes
	keyframes->equivalent_output(plugin->keyframes, startproject, result);
}



int Plugin::is_synthesis(int64_t position,
		int direction)
{
	switch(plugin_type)
	{
		case PLUGIN_STANDALONE:
		{
			if(!track)
			{
				printf("Plugin::is_synthesis track not defined\n");
				return 0;
			}


			PluginServer *plugin_server = MWindow::scan_plugindb(title,
				track->data_type);
//printf("Plugin::is_synthesis %d %p %d\n", __LINE__, plugin_server, plugin_server->get_synthesis());
//plugin_server->dump();
			return plugin_server->get_synthesis();
			break;
		}

// Dereference real plugin and descend another level
		case PLUGIN_SHAREDPLUGIN:
		{
			int real_module_number = shared_location.module;
			int real_plugin_number = shared_location.plugin;
			Track *track = edl->tracks->number(real_module_number);
// Get shared plugin from master track
			Plugin *plugin = track->get_current_plugin(position,
				real_plugin_number,
				direction,
				0,
				0);

			if(plugin)
				return plugin->is_synthesis(position, direction);
			break;
		}

// Dereference the real track and descend
		case PLUGIN_SHAREDMODULE:
		{
			int real_module_number = shared_location.module;
			Track *track = edl->tracks->number(real_module_number);
			return track->is_synthesis(position, direction);
			break;
		}
	}
	return 0;
}



int Plugin::identical(Plugin *that)
{
// Test type
	if(plugin_type != that->plugin_type) return 0;

// Test title or location
	switch(plugin_type)
	{
		case PLUGIN_STANDALONE:
			if(strcmp(title, that->title)) return 0;
			break;
		case PLUGIN_SHAREDPLUGIN:
			if(shared_location.module != that->shared_location.module ||
				shared_location.plugin != that->shared_location.plugin) return 0;
			break;
		case PLUGIN_SHAREDMODULE:
			if(shared_location.module != that->shared_location.module) return 0;
			break;
	}

// Test remaining fields
	return (this->on == that->on &&
		((KeyFrame*)keyframes->default_auto)->identical(
			((KeyFrame*)that->keyframes->default_auto)));
}

int Plugin::identical_location(Plugin *that)
{
	if(!plugin_set || !plugin_set->track) return 0;
	if(!that->plugin_set || !that->plugin_set->track) return 0;

	if(plugin_set->track->number_of() == that->plugin_set->track->number_of() &&
		plugin_set->get_number() == that->plugin_set->get_number() &&
		startproject == that->startproject) return 1;

	return 0;

}

int Plugin::keyframe_exists(KeyFrame *ptr)
{
	for(KeyFrame *current = (KeyFrame*)keyframes->first;
		current;
		current = (KeyFrame*)NEXT)
	{
		if(current == ptr) return 1;
	}
	return 0;
}


void Plugin::change_plugin(char *title,
		SharedLocation *shared_location,
		int plugin_type)
{
	strcpy(this->title, title);
	this->shared_location = *shared_location;
	this->plugin_type = plugin_type;
}



KeyFrame* Plugin::get_prev_keyframe(int64_t position,
	int direction)
{
	return keyframes->get_prev_keyframe(position, direction);
}

KeyFrame* Plugin::get_next_keyframe(int64_t position,
	int direction)
{
	KeyFrame *current;

// This doesn't work for playback because edl->selectionstart doesn't
// change during playback at the same rate as PluginClient::source_position.
	if(position < 0)
	{
//printf("Plugin::get_next_keyframe position < 0\n");
		position = track->to_units(edl->local_session->get_selectionstart(1), 0);
	}

// Get keyframe after current position
	for(current = (KeyFrame*)keyframes->first;
		current;
		current = (KeyFrame*)NEXT)
	{
		if(direction == PLAY_FORWARD && current->position > position) break;
		else
		if(direction == PLAY_REVERSE && current->position >= position) break;
	}

// Nothing after current position
	if(!current && keyframes->last)
	{
		current =  (KeyFrame*)keyframes->last;
	}
	else
// No keyframes
	if(!current)
	{
		current = (KeyFrame*)keyframes->default_auto;
	}

	return current;
}

KeyFrame* Plugin::get_keyframe()
{
	return keyframes->get_keyframe();
}

void Plugin::copy(int64_t start, int64_t end, FileXML *file)
{
	int64_t endproject = startproject + length;

	if((startproject >= start && startproject <= end) ||  // startproject in range
		 (endproject <= end && endproject >= start) ||	   // endproject in range
		 (startproject <= start && endproject >= end))    // range in project
	{
// edit is in range
		int64_t startproject_in_selection = startproject; // start of edit in selection in project
		int64_t startsource_in_selection = startsource; // start of source in selection in source
		//int64_t endsource_in_selection = startsource + length; // end of source in selection
		int64_t length_in_selection = length;             // length of edit in selection

		if(startproject < start)
		{         // start is after start of edit in project
			int64_t length_difference = start - startproject;

			startsource_in_selection += length_difference;
			startproject_in_selection += length_difference;
			length_in_selection -= length_difference;
		}

// end is before end of edit in project
		if(endproject > end)
		{
			length_in_selection = end - startproject_in_selection;
		}

// Plugins don't store silence
		file->tag.set_title("PLUGIN");
//		file->tag.set_property("STARTPROJECT", startproject_in_selection - start);
		file->tag.set_property("LENGTH", length_in_selection);
		file->tag.set_property("TYPE", plugin_type);
		file->tag.set_property("TITLE", title);
		file->append_tag();
		file->append_newline();


		if(plugin_type == PLUGIN_SHAREDPLUGIN ||
			plugin_type == PLUGIN_SHAREDMODULE)
		{
			shared_location.save(file);
		}



		if(in)
		{
			file->tag.set_title("IN");
			file->append_tag();
			file->tag.set_title("/IN");
			file->append_tag();
		}
		if(out)
		{
			file->tag.set_title("OUT");
			file->append_tag();
			file->tag.set_title("/OUT");
			file->append_tag();
		}
		if(show)
		{
			file->tag.set_title("SHOW");
			file->append_tag();
			file->tag.set_title("/SHOW");
			file->append_tag();
		}
		if(on)
		{
			file->tag.set_title("ON");
			file->append_tag();
			file->tag.set_title("/ON");
			file->append_tag();
		}
		file->append_newline();

// Keyframes
		keyframes->copy(start, end, file, 0, 0);

		file->tag.set_title("/PLUGIN");
		file->append_tag();
		file->append_newline();
	}
}

void Plugin::load(FileXML *file)
{
	int result = 0;
	int first_keyframe = 1;
 	in = 0;
	out = 0;
// Currently show is ignored when loading
	show = 0;
	on = 0;
	while(keyframes->last) delete keyframes->last;

	do{
		result = file->read_tag();

//printf("Plugin::load 1 %s\n", file->tag.get_title());
		if(!result)
		{
			if(file->tag.title_is("/PLUGIN"))
			{
				result = 1;
			}
			else
			if(file->tag.title_is("SHARED_LOCATION"))
			{
				shared_location.load(file);
			}
			else
			if(file->tag.title_is("IN"))
			{
				in = 1;
			}
			else
			if(file->tag.title_is("OUT"))
			{
				out = 1;
			}
			else
			if(file->tag.title_is("SHOW"))
			{
				show = 1;
			}
			else
			if(file->tag.title_is("ON"))
			{
				on = 1;
			}
			else
			if(file->tag.title_is("KEYFRAME"))
			{
// Default keyframe
				if(first_keyframe)
				{
					keyframes->default_auto->load(file);
					first_keyframe = 0;
				}
				else
// Override default keyframe
				{
					KeyFrame *keyframe = (KeyFrame*)keyframes->append(new KeyFrame(edl, keyframes));
					keyframe->position = file->tag.get_property("POSITION", (int64_t)0);
					keyframe->load(file);
				}
			}
		}
	}while(!result);
}

void Plugin::get_shared_location(SharedLocation *result)
{
	if(plugin_type == PLUGIN_STANDALONE && plugin_set)
	{
		result->module = edl->tracks->number_of(track);
		result->plugin = track->plugin_set.number_of(plugin_set);
	}
	else
	{
		*result = this->shared_location;
	}
}

Track* Plugin::get_shared_track()
{
	return edl->tracks->get_item_number(shared_location.module);
}


void Plugin::calculate_title(char *string, int use_nudge)
{
	switch( plugin_type ) {
	case PLUGIN_STANDALONE:
	case PLUGIN_NONE:
		strcpy(string, _(title));
		break;
	case PLUGIN_SHAREDPLUGIN:
	case PLUGIN_SHAREDMODULE:
		shared_location.calculate_title(string, edl,
			startproject, 0, plugin_type, use_nudge);
		break;
	}
}

void Plugin::fix_plugin_title(char *title)
{
	MWindow::fix_plugin_title(title);
}


void Plugin::paste(FileXML *file)
{
	length = file->tag.get_property("LENGTH", (int64_t)0);
}

void Plugin::resample(double old_rate, double new_rate)
{
// Resample keyframes in here
	keyframes->resample(old_rate, new_rate);
}

void Plugin::shift(int64_t difference)
{
	Edit::shift(difference);

	if(edl->session->autos_follow_edits)
		shift_keyframes(difference);
}

void Plugin::dump(FILE *fp)
{
	fprintf(fp,"    PLUGIN: type=%d title=\"%s\" on=%d track=%d plugin=%d\n",
		plugin_type, title, on, shared_location.module, shared_location.plugin);
	fprintf(fp,"    startproject %jd length %jd\n", startproject, length);

	keyframes->dump(fp);
}


