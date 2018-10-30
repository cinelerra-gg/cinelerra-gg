
/*
 * CINELERRA
 * Copyright (C) 1997-2016 Adam Williams <broadcast at earthling dot net>
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

#ifndef MWINDOW_INC
#define MWINDOW_INC

#define PROGRAM_NAME "Cinelerra"
#define DEFAULT_THEME "UnFlat"
#define DEFAULT_PICON "cinfinity"
#define CONFIG_FILE "Cinelerra_rc"
// user presets
#define PRESETS_FILE "Cinelerra_presets"
#define EXPANDERS_FILE "expanders.txt"
#define PERPETUAL_FILE "perpetual.dat"
// factory presets
#define FACTORY_FILE "Cinelerra_factory"
#define PICTURE_FILE "Cinelerra_picture"
#define PLUGIN_FILE "Cinelerra_plugins"
#define LADSPA_FILE "ladspa_plugins."
#define PLUGIN_FILE_VERSION 4

// Behavior of region selections
#define SELECTION_SAMPLES 0
#define SELECTION_EDIT 1

// min milliseconds between tweek undos to create new undo entries
#define UNDO_SPAN 1000

// Make tweeks span keyframes if a region is selected
#define USE_KEYFRAME_SPANNING

// Index of vwindow that always exists
#define DEFAULT_VWINDOW 0

class MWindow;

#if 0
// for PROGRAM_NAME xlat strings

N_("Cinelerra: Asset Info")
N_("Cinelerra: Asset path")
N_("Cinelerra: Attach Effect")
N_("Cinelerra: Audio compression")
N_("Cinelerra: Audio Compression")
N_("Cinelerra: Audio Preset")
N_("Cinelerra: Batch Render")
N_("Cinelerra: Camera")
N_("Cinelerra: CD Ripper")
N_("Cinelerra: Change Effect")
N_("Cinelerra: Channel Info")
N_("Cinelerra: Channels")
N_("Cinelerra: ChanSearch")
N_("Cinelerra: Clip Info")
N_("Cinelerra: Color")
N_("Cinelerra: Copy File List")
N_("Cinelerra: Commands")
N_("Cinelerra: Compositor")
N_("Cinelerra: Confirm")
N_("Cinelerra: Confirm Quit")
N_("Cinelerra: Could not set locale.\n")
N_("Cinelerra: Create BD")
N_("Cinelerra: Create DVD")
N_("Cinelerra: Crop")
N_("Cinelerra: DbWindow")
N_("Cinelerra: Delete All Indexes")
N_("Cinelerra: Edit Channel")
N_("Cinelerra: Edit length")
N_("Cinelerra: Effect Prompt")
N_("Cinelerra: Effect Info")
N_("Cinelerra: Error")
N_("Cinelerra: Errors")
N_("Cinelerra: Export EDL")
N_("Cinelerra: Export Project"),
N_("Cinelerra: Feather Edits")
N_("Cinelerra: File Exists")
N_("Cinelerra: File format")
N_("Cinelerra: File Format")
N_("Cinelerra: Goto position")
N_("Cinelerra: Label Info")
N_("Cinelerra: Levels")
N_("Cinelerra: Load")
N_("Cinelerra: Loading")
N_("Cinelerra: Locate file")
N_("Cinelerra: Mask")
N_("Cinelerra: Mixer")
N_("Cinelerra: New folder")
N_("Cinelerra: New Project")
N_("Cinelerra: Append to Project")
N_("Cinelerra: Normalize")
N_("Cinelerra: Options")
N_("Cinelerra: Overlays")
N_("Cinelerra: Paste File List")
N_("Cinelerra: Path")
N_("Cinelerra: Picture")
N_("Cinelerra: Preferences")
N_("Cinelerra: Probes")
N_("Cinelerra: Program")
N_("Cinelerra: Projector")
N_("Cinelerra: Proxy settings")
N_("Cinelerra: Question")
N_("Cinelerra: Record")
N_("Cinelerra: Recording")
N_("Cinelerra: Record path")
N_("Cinelerra: Redraw Indexes")
N_("Cinelerra: RemoteWindow")
N_("Cinelerra: Remove assets")
N_("Cinelerra: Resample")
N_("Cinelerra: Remove plugin")
N_("Cinelerra: Render")
N_("Cinelerra: Render effect")
N_("Cinelerra: Resize Track")
N_("Cinelerra: Resources")
N_("Cinelerra: Ruler")
N_("Cinelerra: %s")
N_("Cinelerra: Save")
N_("Cinelerra: Scale")
N_("Cinelerra: Scan confirm")
N_("Cinelerra: Scopes")
N_("Cinelerra: Set edit title")
N_("Cinelerra: Set Format")
N_("Cinelerra: Shell")
N_("Cinelerra: %s Keyframe")
N_("Cinelerra: %s Presets")
N_("Cinelerra: Subtitle")
N_("Cinelerra: Time stretch")
N_("Cinelerra: Tip of the day")
N_("Cinelerra: Transition")
N_("Cinelerra: Transition length")
N_("Cinelerra: Video Compression")
N_("Cinelerra: Video in")
N_("Cinelerra: Video in %d%%")
N_("Cinelerra: Video out")
N_("Cinelerra: Video out %d%%")
N_("Cinelerra: Video Preset")
N_("Cinelerra: Viewer")
N_("Cinelerra: Warning")

#endif

#endif
