
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

#ifndef PRESETS_H
#define PRESETS_H


#include "arraylist.h"
#include "filexml.inc"
#include "keyframe.inc"


// Front end for a DB of presets for all plugins

// A single preset
class PresetsDBKeyframe
{
public:
	PresetsDBKeyframe(const char *title, int is_factory);
	~PresetsDBKeyframe();

	void set_data(char *data);

	char *title;
	char *data;
// is a factory preset
	int is_factory;
};

// Presets for a single plugin
class PresetsDBPlugin
{
public:
	PresetsDBPlugin(const char *title);
	~PresetsDBPlugin();
	
	
	void load(FileXML *file, int is_factory);
	void save(FileXML *file);

// Get a preset by name
	PresetsDBKeyframe* get_keyframe(const char *title, int is_factory);
// Create a new keyframe
	PresetsDBKeyframe* new_keyframe(const char *title);
	void delete_keyframe(const char *title);
// Load a preset into the keyframe
	void load_preset(const char *preset_title, 
		KeyFrame *keyframe,
		int is_factory);
	int preset_exists(const char *preset_title, int is_factory);
	int get_total_presets(int user_only);

	ArrayList<PresetsDBKeyframe*> keyframes;
	char *title;
};

class PresetsDB
{
public:
	PresetsDB();

// Load the database from the file.
	void load_from_file(char *path, int is_factory, int clear_it);
// load the database from a string
	void load_from_string(char *string, int is_factory, int clear_it);
	void load_common(FileXML *file, int is_factory);
	
// Save the database to the file.
	void save();
	void sort(char *plugin_title);

// Get the total number of presets for a plugin
	int get_total_presets(char *plugin_title, int user_only);
// Get the title of a preset
	char* get_preset_title(char *plugin_title, int number);
	int get_is_factory(char *plugin_title, int number);
// Get the data for a preset
	char* get_preset_data(char *plugin_title, int number);
// Get a pluginDB by name
	PresetsDBPlugin* get_plugin(const char *plugin_title);
// Create a pluginDB
	PresetsDBPlugin* new_plugin(const char *plugin_title);
	void save_preset(const char *plugin_title, 
		const char *preset_title, 
		char *data);
	void delete_preset(const char *plugin_title, 
		const char *preset_title,
		int is_factory);
// Load a preset into the keyframe
	void load_preset(const char *plugin_title, 
		const char *preset_title, 
		KeyFrame *keyframe,
		int is_factory);
	int preset_exists(const char *plugin_title, 
		const char *preset_title,
		int is_factory);

private:
// Remove all plugin data
	void clear();

	ArrayList<PresetsDBPlugin*> plugins;
};



#endif



