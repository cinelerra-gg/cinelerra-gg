
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

#ifndef EDL_INC
#define EDL_INC

class EDL;

// Load flags for undo
#define LOAD_ALL        0xffffffff
#define LOAD_EDITS      0x00000001
#define LOAD_AUTOMATION 0x00000002
#define LOAD_PATCHES 	0x00000004
#define LOAD_CONSOLE 	0x00000008
#define LOAD_TIMEBAR 	0x00000010
#define LOAD_ACONFIG 	0x00000020
#define LOAD_VCONFIG 	0x00000040
#define LOAD_ASSETS 	0x00000080
#define LOAD_SESSION	0x00000100

// Copy flags
#define COPY_ALL		0x00000001
#define COPY_LENGTH		0x00000002
#define COPY_LOCAL_SESSION	0x00000004
#define COPY_SESSION		0x00000008
#define COPY_VIDEO_CONFIG	0x00000010
#define COPY_AUDIO_CONFIG	0x00000020
#define COPY_FOLDERS		0x00000040
#define COPY_ALL_ASSETS		0x00000080
#define COPY_USED_ASSETS	0x00000100
#define COPY_NESTED_EDL		0x00000200
#define COPY_CLIPS		0x00000400
#define COPY_VWINDOWS		0x00000800
#define COPY_MIXERS		0x00001000
#define COPY_LABELS		0x00002000
#define COPY_EDITS		0x00004000
#define COPY_AUTOS		0x00008000
#define COPY_PLUGINS		0x00010000

#define COPY_ALWAYS ( \
 COPY_LOCAL_SESSION | \
 COPY_SESSION | \
 COPY_VIDEO_CONFIG | \
 COPY_AUDIO_CONFIG | \
 COPY_FOLDERS | \
 COPY_NESTED_EDL )

#define COPY_TRACKS ( \
 COPY_EDITS | \
 COPY_AUTOS | \
 COPY_PLUGINS )

#define COPY_EDL ( \
 COPY_ALL | \
 COPY_ALWAYS | \
 COPY_ALL_ASSETS | \
 COPY_CLIPS | \
 COPY_VWINDOWS | \
 COPY_MIXERS | \
 COPY_LABELS | \
 COPY_TRACKS )

#define COPY_CLIPBOARD ( \
 COPY_LENGTH | \
 COPY_ALWAYS | \
 COPY_USED_ASSETS | \
 COPY_LABELS | \
 COPY_TRACKS )

#define EDITING_MODES 2

// Editing modes
#define EDITING_ARROW 0
#define EDITING_IBEAM 1


// default for left button
// default for middle button
// default for right button
#define MOVE_RIPPLE 0
#define MOVE_ROLL   1
#define MOVE_SLIP   2
#define MOVE_SLIDE  3
#define MOVE_EDGE   4
#define MOVE_EDITS_DISABLED 5
#define EDIT_HANDLE_MODES 5

// AWindow folders
#define AW_NO_FOLDER         -1
#define AW_AEFFECT_FOLDER     0
#define AW_VEFFECT_FOLDER     1
#define AW_ATRANSITION_FOLDER 2
#define AW_VTRANSITION_FOLDER 3
#define AW_LABEL_FOLDER       4
#define AW_CLIP_FOLDER        5
#define AW_MEDIA_FOLDER       6
#define AW_PROXY_FOLDER       7
#define AWINDOW_FOLDERS       8
#define AWINDOW_USER_FOLDERS  8


#define AWINDOW_MODES 2
// Modes for AWindow views
#define ASSETS_TEXT 0
#define ASSETS_ICONS 1
#define ASSETS_ICONS_PACKED 2
#define ASSETS_ICON_LIST 3
#define FOLDERS_TEXT 0
#define FOLDERS_ICONS 1

#define ASSET_COLUMNS 2

// Active tool in CWindow corresponds to a member of the CPanel array
// and the current drag operation

#define CPANEL_OPERATIONS 10


#define CWINDOW_NONE -1
#define CWINDOW_PROTECT     0
#define CWINDOW_ZOOM        1
#define CWINDOW_MASK        2
#define CWINDOW_RULER       3
#define CWINDOW_CAMERA      4
#define CWINDOW_PROJECTOR   5
#define CWINDOW_CROP        6
#define CWINDOW_EYEDROP     7
#define CWINDOW_TOOL_WINDOW 8
#define CWINDOW_TITLESAFE   9

// Current dragging operations not available in the CPanel
#define CWINDOW_SCROLL 0x10
#define CWINDOW_MASK_CONTROL_IN 0x15
#define CWINDOW_MASK_CONTROL_OUT 0x16
#define CWINDOW_MASK_TRANSLATE 0x17

// Inclusive limits for zoom panel
#define MIN_ZOOM_TIME 1
#define MAX_ZOOM_TIME 0x100000
#define DEFAULT_ZOOM_TIME 1024
#define MIN_AMP_ZOOM 8
#define MAX_AMP_ZOOM 262144
#define MIN_TRACK_ZOOM 32
#define MAX_TRACK_ZOOM 1024

#define MAX_PAN 1.0
#define PAN_RADIUS 50

#endif
