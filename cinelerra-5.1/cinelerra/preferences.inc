
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

#ifndef PREFERENCES_INC
#define PREFERENCES_INC

// Run-time configuration directory
#define DEAMON_PORT 400
#define BACKUP_FILE "backup.xml"
#define BACKUP_FILE1 "backup.prev"
#define LAYOUT_FILE "layout%d_rc"
#define DEFAULT_LV2_PATH CINDAT_DIR "/lv2"
#define FFMPEG_EARLY_TIP _("Currently: Try FFMpeg first\n   Click to: Try FFMpeg last")
#define FFMPEG_LATE_TIP  _("Currently: Try FFMpeg last\n   Click to: Try FFMpeg first")

#define VICON_COLOR_MODE_LOW 0
#define VICON_COLOR_MODE_MED 1
#define VICON_COLOR_MODE_HIGH 2

class Preferences;
class PlaybackConfig;
class AudioOutConfig;
class VideoOutConfig;
class AudioInConfig;
class VideoInConfig;

#endif
