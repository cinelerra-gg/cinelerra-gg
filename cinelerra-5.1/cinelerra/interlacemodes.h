/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef INTERLACEMODES_H
#define INTERLACEMODES_H
#include "language.h"

#define ILACE_UNKNOWN_T      N_("Error!")

//Interlace Automatic fixing options
#define ILACE_AUTOFIXOPTION_MANUAL  	0
#define ILACE_AUTOFIXOPTION_MANUAL_T	N_("Manual compensation using selection")
#define ILACE_AUTOFIXOPTION_AUTO    	1
#define ILACE_AUTOFIXOPTION_AUTO_T  	N_("Automatic compensation using modes")
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

//Interlace Modes
#define ILACE_MODE_UNDETECTED         0
#define ILACE_MODE_UNDETECTED_XMLT    "UNKNOWN"
#define ILACE_MODE_UNDETECTED_T       N_("Unknown")
#define ILACE_MODE_TOP_FIRST          1
#define ILACE_MODE_TOP_FIRST_XMLT     "TOP_FIELD_FIRST"
#define ILACE_MODE_TOP_FIRST_T        N_("Top Fields First")
#define ILACE_MODE_BOTTOM_FIRST       2
#define ILACE_MODE_BOTTOM_FIRST_XMLT  "BOTTOM_FIELD_FIRST"
#define ILACE_MODE_BOTTOM_FIRST_T     N_("Bottom Fields First")
#define ILACE_MODE_NOTINTERLACED      3
#define ILACE_MODE_NOTINTERLACED_XMLT "NOTINTERLACED"
#define ILACE_MODE_NOTINTERLACED_T    N_("Not Interlaced")

#define ILACE_ASSET_MODEDEFAULT  	ILACE_MODE_UNDETECTED
#define ILACE_PROJECT_MODEDEFAULT	ILACE_MODE_NOTINTERLACED_T
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

//Interlace Compensation Methods
#define ILACE_FIXMETHOD_NONE     	0
#define ILACE_FIXMETHOD_NONE_XMLT   	"DO_NOTHING"
#define ILACE_FIXMETHOD_NONE_T   	N_("Do Nothing")
#define ILACE_FIXMETHOD_UPONE    	1
#define ILACE_FIXMETHOD_UPONE_XMLT  	"SHIFT_UPONE"
#define ILACE_FIXMETHOD_UPONE_T  	N_("Shift Up 1 pixel")
#define ILACE_FIXMETHOD_DOWNONE  	2
#define ILACE_FIXMETHOD_DOWNONE_XMLT	"SHIFT_DOWNONE"
#define ILACE_FIXMETHOD_DOWNONE_T	N_("Shift Down 1 pixel")

// the following is for project/asset having odd/even, or even/odd
#define ILACE_FIXDEFAULT         	ILACE_FIXMETHOD_UPONE
//Note: Do not change what the numbers mean as this will make backward-compatability have erroraneous settings.

// Refer to <mjpegtools/yuv4mpeg.h> (descriptions were cut-and-pasted!)
#define ILACE_Y4M_UKNOWN_T         N_("unknown")
#define ILACE_Y4M_NONE_T           N_("non-interlaced, progressive frame")
#define ILACE_Y4M_TOP_FIRST_T      N_("interlaced, top-field first")
#define ILACE_Y4M_BOTTOM_FIRST_T   N_("interlaced, bottom-field first")
#define ILACE_Y4M_MIXED_T          N_("mixed, \"refer to frame header\"")

void ilaceautofixoption_to_text(char *string, int autofixoption);
int  ilaceautofixoption_from_text(const char *text, int thedefault);

void ilacemode_to_text(char *string, int ilacemode);
int  ilacemode_from_text(const char *text, int thedefault);
void ilacemode_to_xmltext(char *string, int ilacemode);
int  ilacemode_from_xmltext(const char *text, int thedefault);

void ilacefixmethod_to_text(char *string, int fixmethod);
int  ilacefixmethod_from_text(const char *text, int thedefault);
void ilacefixmethod_to_xmltext(char *string, int fixmethod);
int  ilacefixmethod_from_xmltext(const char *text, int thedefault);


int  ilaceautofixmethod(int projectilacemode, int assetilacemode);
int  ilaceautofixmethod2(int projectilacemode, int assetautofixoption, int assetilacemode, int assetfixmethod);

int ilace_bc_to_yuv4mpeg(int ilacemode);
int ilace_yuv4mpeg_to_bc(int ilacemode);

void ilace_yuv4mpeg_mode_to_text(char *string, int ilacemode);

#endif // INTERLACEMODES_H
