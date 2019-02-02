
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILE_INC
#define FILE_INC

// Normally, these are sourced from the compiler command line
//   as: c++ -Dvar=value defines
#ifndef CIN_CONFIG
#define CIN_CONFIG "~/.bcast5"
#endif
#ifndef LOCALE_DIR
#define LOCALE_DIR "$CIN_LIB/locale"
#endif
#ifndef CINDAT_DIR
#define CINDAT_DIR "$CIN_PATH"
#endif
#ifndef CINLIB_DIR
#define CINLIB_DIR "$CIN_PATH"
#endif
#ifndef PLUGIN_DIR
#define PLUGIN_DIR "$CIN_LIB/plugins"
#endif
#ifndef LADSPA_DIR
#define LADSPA_DIR "$CIN_LIB/ladspa"
#endif
#ifndef CIN_BROWSER
#define CIN_BROWSER "firefox"
#endif

#include "language.h"

// Return values for open_file
#define FILE_OK 0
#define FILE_NOT_FOUND 1
#define FILE_UNRECOGNIZED_CODEC 2
#define FILE_IS_XML 3

// Numeric codes for each file format
// They're stored in XML files, so they have to be fixed.
#define FILE_PLUGIN       	0x8000
#define FILE_UNKNOWN    	0
// Libzmpeg3 decoding and transport stream capture only
#define FILE_PCM  	    	1
#define FILE_WAV  	    	2
#define FILE_PNG  	    	4
#define FILE_JPEG 	    	5
#define FILE_TIFF 	    	6
#define FILE_GIF  	    	7
#define FILE_JPEG_LIST  	8
#define FILE_AU         	9
#define FILE_AIFF       	10
#define FILE_SND        	11
#define FILE_TGA_LIST     	13
#define FILE_TGA          	14
#define FILE_MPEG         	15
#define FILE_AMPEG        	16    // For encoding only
#define FILE_VMPEG        	17    // For encoding only
#define FILE_RAWDV		18
#define FILE_TIFF_LIST  	22
#define FILE_PNG_LIST   	23
#define FILE_AC3                25    // AC3 encoding
#define FILE_EXR                26
#define FILE_EXR_LIST           27
#define FILE_CR2                28
#define FILE_FLAC               32
#define FILE_FFMPEG             33
#define FILE_SCENE              34
#define FILE_CR2_LIST           35
#define FILE_GIF_LIST   	36
#define FILE_DB                 37
#define FILE_PPM                38
#define FILE_PPM_LIST           39

// For formats supported by plugins, the format number is the plugin number in the
// plugin list ORed with 0x8000.

#if 0
N_("AC3")
N_("Apple/SGI AIFF")
N_("AVI Arne Type 1")
N_("AVI Avifile")
N_("AVI DV Type 2")
N_("AVI Lavtools")
N_("EXR")
N_("EXR Sequence")
N_("FFMPEG")
N_("FLAC")
N_("JPEG")
N_("JPEG Sequence")
N_("Microsoft WAV")
N_("MPEG Audio")    // For encoding only
N_("MPEG")          // For decoding only
N_("MPEG Video")    // For encoding only
N_("PNG")
N_("PNG Sequence")
N_("PPM")
N_("PPM Sequence")
N_("Raw DV")
N_("Raw PCM")
N_("Sun/NeXT AU")
N_("TGA")
N_("TGA Sequence")
N_("TIFF")
N_("TIFF Sequence")
N_("Unknown sound")
#endif

#define AC3_NAME		"AC3"
#define AIFF_NAME		"Apple/SGI AIFF"
#define AMPEG_NAME		"MPEG Audio"    // For encoding only
#define AU_NAME			"Sun/NeXT AU"
#define AVI_ARNE1_NAME		"AVI Arne Type 1"
#define AVI_ARNE2_NAME		"AVI DV Type 2"
#define AVI_AVIFILE_NAME	"AVI Avifile"
#define AVI_LAVTOOLS_NAME	"AVI Lavtools"
#define CR2_LIST_NAME		"CR2 Sequence"
#define CR2_NAME		"Raw Camera"
#define DBASE_NAME		"DB"
#define EXR_LIST_NAME		"EXR Sequence"
#define EXR_NAME		"EXR"
#define FFMPEG_NAME		"FFMPEG"
#define FLAC_NAME		"FLAC"
#define JPEG_LIST_NAME		"JPEG Sequence"
#define JPEG_NAME		"JPEG"
#define MPEG_NAME		"MPEG Stream"	// For capture only
#define PCM_NAME		"Raw PCM"
#define PNG_LIST_NAME		"PNG Sequence"
#define PNG_NAME		"PNG"
#define PPM_LIST_NAME		"PPM Sequence"
#define PPM_NAME		"PPM"
#define RAWDV_NAME		"Raw DV"
#define SCENE_NAME		"Text To Movie"
#define SND_NAME		"Unknown sound"
#define TGA_LIST_NAME		"TGA Sequence"
#define TGA_NAME		"TGA"
#define TIFF_LIST_NAME		"TIFF Sequence"
#define TIFF_NAME		"TIFF"
#define VMPEG_NAME		"MPEG Video"    // For encoding only
#define WAV_NAME		"Microsoft WAV"

#define BITSLINEAR8    8
#define BITSLINEAR16   16
#define BITSLINEAR24   24
#define BITSLINEAR32   32
#define BITS_ADPCM     252
#define BITSFLOAT      253
#define BITSULAW       254

#if 0
N_("8 Bit Linear")
N_("16 Bit Linear")
N_("24 Bit Linear")
N_("32 Bit Linear")
N_("u Law")
N_("IMA 4")
N_("ADPCM")
N_("Float")

N_("RGB ALPHA")
N_("PNG ALPHA")
#endif

#define NAME_8BIT "8 Bit Linear"
#define NAME_16BIT "16 Bit Linear"
#define NAME_24BIT "24 Bit Linear"
#define NAME_32BIT "32 Bit Linear"
#define NAME_ULAW "u Law"
#define NAME_ADPCM "ADPCM"
#define NAME_FLOAT "Float"


class File;
//result<0: continue skimming, result>=0: exit return code
typedef int (*skim_fn)(void *vp, int track);

#endif
