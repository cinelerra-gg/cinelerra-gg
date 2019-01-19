
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

#ifndef CHANNELTABLES_H
#define CHANNELTABLES_H

typedef struct
{
	const char *name;
	int   freq;
} CHANLIST;


typedef struct
{
    const char             *name;
    CHANLIST 		 *list;
    int               count;
} CHANLISTS;

#define CHAN_COUNT(x) (sizeof(x)/sizeof(CHANLIST))

#define NTSC_AUDIO_CARRIER	4500
#define PAL_AUDIO_CARRIER_I	6000
#define PAL_AUDIO_CARRIER_BGHN	5500
#define PAL_AUDIO_CARRIER_MN	4500
#define PAL_AUDIO_CARRIER_D	6500
#define SEACAM_AUDIO_DKK1L	6500
#define SEACAM_AUDIO_BG		5500
#define NICAM728_PAL_BGH	5850
#define NICAM728_PAL_I		6552

// Norms
#define NTSC 0
#define PAL 1
#define SECAM 2

// Frequencies
#define NTSC_DVB 0
#define CATV_DVB 1
#define NTSC_BCAST 2
#define NTSC_CABLE 3
#define NTSC_HRC 4
#define NTSC_BCAST_JP 5
#define NTSC_CABLE_JP 6
#define PAL_EUROPE 7
#define PAL_E_EUROPE 8
#define PAL_ITALY 9
#define PAL_NEWZEALAND 10
#define PAL_AUSTRALIA 11
#define PAL_IRELAND 12
#define CHANLIST_SIZE 13

extern CHANLISTS chanlists[CHANLIST_SIZE];

#endif
