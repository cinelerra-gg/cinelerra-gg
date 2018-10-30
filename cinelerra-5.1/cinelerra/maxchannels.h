
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

#ifndef MAXCHANNELS
#define MAXCHANNELS 32
#define MAX_CHANNELS 32

static inline int default_audio_channel_position(int channel, int total_channels)
{
	int default_position = 0;
	switch( total_channels ) {
	case 0: break;
	case 1: switch( channel ) {
		case 0: default_position = 90;   break;
		}
		break;
	case 2: switch( channel ) {
		case 0: default_position = 180;  break;
		case 1: default_position = 0;    break;
		}
		break;
	case 6: switch( channel ) {
		case 0: default_position = 150;  break;
		case 1: default_position = 30;   break;
		case 2: default_position = 90;   break;
		case 3: default_position = 270;  break;
		case 4: default_position = 210;  break;
		case 5: default_position = 330;  break;
		}
		break;
	default:
		default_position = 180 - (360 * channel / total_channels);
		while( default_position < 0 ) default_position += 360;
		break;
	}
	return default_position;
}

#endif
