/*
 * defaultformats.h
 * Copyright (C) 2011 Einar Rünkaru <einarry at smail dot ee>
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
 */

/*
 * Default presets for format
 */

struct formatpresets
{
	const char *name;
	int audio_channels;
	int audio_tracks;
	int sample_rate;
	int video_channels;
	int video_tracks;
	double frame_rate;
	int output_w;
	int output_h;
	int aspect_w;
	int aspect_h;
	int interlace_mode;
	int color_model;
};

static struct formatpresets format_presets[] = {
	{ N_("1080P/60"),		2, 2, 48000,	1, 1, 60000.0 / 1001,
		1920,1080, 16,9, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("1080P/24"),		6, 6, 48000,	1, 1, 24,
		1920,1080, 16,9, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("1080I"),		2, 2, 48000,	1, 1, 30000.0 / 1001,
		1920,1080, 16,9, ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ N_("720P/60"),	2, 2, 48000,	1, 1, 60000.0 / 1001,
		1280,720,  16,9, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("PAL 576I - DV(D)"),	2, 2, 48000,	1, 1, 25,
		720,576,   4,3, ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ N_("NTSC 480P - DV(D)"),	2, 2, 48000,	1, 1, 60000.0 / 1001,
		720,480,   4,3, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("NTSC 480I - DV(D)"),	2, 2, 48000,	1, 1, 30000.0 / 1001,
		720,480,   4,3, ILACE_MODE_BOTTOM_FIRST, BC_YUVA8888 },
	{ N_("YouTube"),		1, 1, 48000,	1, 1, 30000.0 / 1001,
		424,318,   4,3, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("Half D-1 PAL"),		2, 2, 48000,	1, 1, 25,
		360,288,   4,3, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("NTSC Half D-1"),	2, 2, 48000,	1, 1, 30000.0 / 1001,
		360,240,   4,3, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("Internet"),		1, 1, 22050,	1, 1, 15,
		320,240,   4,3, ILACE_MODE_NOTINTERLACED, BC_YUVA8888 },
	{ N_("CD Audio"),		2, 2, 44100,	1, 0, 30000.0 / 1001,
		720,480,   4,3, ILACE_MODE_NOTINTERLACED, BC_RGBA8888 },
	{ N_("DAT Audio"),		2, 2, 48000,	1, 0, 30000.0 / 1001,
		720,480,   4,3, ILACE_MODE_NOTINTERLACED, BC_RGBA8888 },
	{ 0 }
};

#define MAX_NUM_PRESETS (sizeof(format_presets) / sizeof(struct formatpresets))
