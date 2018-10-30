
/*
 * CINELERRA
 * Copyright (C) 2008-2015 Adam Williams <broadcast at earthling dot net>
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

#ifndef EDLSESSION_H
#define EDLSESSION_H

#include "asset.inc"
#include "autoconf.inc"
#include "bcwindowbase.inc"
#include "bchash.inc"
#include "edl.inc"
#include "filexml.inc"
#include "maxchannels.h"
#include "playbackconfig.inc"
#include "recordconfig.inc"


// Session shared between all clips


class EDLSession
{
public:
	EDLSession(EDL *edl);
	~EDLSession();

	int load_xml(FileXML *xml, int append_mode, uint32_t load_flags);
	int save_xml(FileXML *xml);
	int copy(EDLSession *session);
	int load_audio_config(FileXML *file, int append_mode, uint32_t load_flags);
	int save_audio_config(FileXML *xml);
	int load_video_config(FileXML *file, int append_mode, uint32_t load_flags);
	int save_video_config(FileXML *xml);
	int load_defaults(BC_Hash *defaults);
	int save_defaults(BC_Hash *defaults);
	void boundaries();

//	PlaybackConfig* get_playback_config(int strategy, int head);
//	ArrayList<PlaybackConfig*>* get_playback_config(int strategy);
//	int get_playback_heads(int strategy);

// Called by PreferencesThread to determine if preference changes need to be
// rendered.
	int need_rerender(EDLSession *ptr);
// Called by BRender to determine if any background rendered frames are valid.
	void equivalent_output(EDLSession *session, double *result);
	void dump();

// Audio
	int achannel_positions[MAXCHANNELS];
// AWindow format
	int assetlist_format;
// AWindow column widths
	int asset_columns[ASSET_COLUMNS];
	AutoConf *auto_conf;
// Aspect ratio for video
	float aspect_w;
	float aspect_h;
	int audio_channels;
	int audio_tracks;
// automation follows edits during editing
 	int autos_follow_edits;
// Generate keyframes for every tweek
	int auto_keyframes;
// Where to do background rendering
	double brender_start;
	double brender_end;
// Length of clipboard if pasting
	double clipboard_length;
// Colormodel for intermediate frames
	int color_model;
// Interlace Mode for entire project
	int interlace_mode;
// Coords for cropping operation
	int crop_x1, crop_x2, crop_y1, crop_y2;
// radius of eyedropper
	int eyedrop_radius;
	float ruler_x1, ruler_y1;
	float ruler_x2, ruler_y2;
// Ruler points relative to the output frame.
// Current folder in resource window
	int awindow_folder;
// align cursor on frame boundaries
	int cursor_on_frames;
// paste keyframes to any track type
	int typeless_keyframes;
// Destination item for CWindow
	int cwindow_dest;
// Current submask being edited in CWindow
	int cwindow_mask;
// Use the cwindow or not
	int cwindow_meter;
// CWindow tool currently selected
	int cwindow_operation;
// Use scrollbars in the CWindow
	int cwindow_scrollbars;
// Scrollbar positions
	int cwindow_xscroll;
	int cwindow_yscroll;
	float cwindow_zoom;
	int cwindow_click2play;
// Transition
	char default_atransition[BCTEXTLEN];
	char default_vtransition[BCTEXTLEN];
// Length in seconds
	double default_transition_length;
// Edit mode to use for each mouse button
	int edit_handle_mode[3];
// Editing mode
	int editing_mode;
	EDL *edl;
	int enable_duplex;
// AWindow format
	int folderlist_format;
	double frame_rate;
	float frames_per_foot;
// Number of highlighted track
	int highlighted_track;
// Enumeration for how to scale from edl.inc.
	int interpolation_type;
// Whether to interpolate CR2 images
	int interpolate_raw;
// Whether to white balance CR2 images
	int white_balance_raw;
// labels follow edits during editing
	int labels_follow_edits;
	int plugins_follow_edits;
// For main menu plugin attaching,
// // attach 1 standalone on the first track and share it with other tracks
	int single_standalone;
	int meter_format;
	int min_meter_db;
	int max_meter_db;
	int output_w;
	int output_h;
	int64_t playback_buffer;
	int playback_cursor_visible;
	int64_t playback_preload;
	int decode_subtitles;
	int subtitle_number;
	int label_cells;
	int program_no;
	int playback_software_position;
// current settings are scaled this much from the original settings
        int proxy_scale;
// original proxy_scale when proxy_enable disables/enables proxy
	int proxy_disabled_scale;
// upscale geom from proxy to session on read in
        int proxy_use_scaler;
// automatically rescale assets when loaded
        int proxy_auto_scale;
// beep when proxy/render done
        int proxy_beep, render_beep;
//	int playback_strategy;
// Play audio in realtime priority
	int real_time_playback;
	int real_time_record;
// Use record positioning selection
	int record_positioning;
// Record undecoded stream data
	int record_raw_stream;
// Sync the drives during recording
	int record_sync_drives;
// Speed of meters when recording
	int record_speed;
// Samples to read from device at a time
	int record_fragment_size;
// Samples to write to disk at a time
	int64_t record_write_length;
// Realtime TOC generation for DVB mpeg recording
	int record_realtime_toc;
// Show title and action safe regions in CWindow
	int safe_regions;
	int64_t sample_rate;
	float scrub_speed;
// Load files as a duration
	int si_useduration;
	float si_duration;
// Show assets in track canvas
	int show_assets;
// Show titles in resources
	int show_titles;
// Test for data before rendering a track
	int test_playback_edits;
// Format to display times in
	int time_format;
// Offset for timecode
	int timecode_offset[4];
// Format to display nudge in, either seconds or track units.
	int nudge_format;
// Show tool window in CWindow
	int tool_window;
// Location of video outs
	int vchannel_x[MAXCHANNELS];
	int vchannel_y[MAXCHANNELS];
// Recording
	int video_channels;
	VideoInConfig *vconfig_in;
	AudioInConfig *aconfig_in;
	Asset *recording_format;
// play every frame
	int video_every_frame;
// decode video asynchronously
//	int video_asynchronous;
	int video_tracks;
// number of frames to write to disk at a time during video recording.
	int video_write_length;
	int view_follows_playback;
// Use the vwindow meter or not
	int vwindow_meter;
	float vwindow_zoom;
	int vwindow_click2play;

// Global ID counter
	static int current_id;
	PlaybackConfig* playback_config;

private:
// Global playback.  This is loaded from defaults but not from XML probably
// because it was discovered to be the most convenient.
// It is part of the EDL probably because the playback setting was
// going to be bound to the EDL.
//	ArrayList<PlaybackConfig*> playback_config[PLAYBACK_STRATEGIES];
};


#endif
