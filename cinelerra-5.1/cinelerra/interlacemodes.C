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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifndef HAVE_STDINT_H
#define HAVE_STDINT_H
#endif /* HAVE_STDINT_H */

#define Y4M_UNKNOWN -1
#define Y4M_ILACE_NONE          0   /* non-interlaced, progressive frame */
#define Y4M_ILACE_TOP_FIRST     1   /* interlaced, top-field first       */
#define Y4M_ILACE_BOTTOM_FIRST  2   /* interlaced, bottom-field first    */
#define Y4M_ILACE_MIXED         3   /* mixed, "refer to frame header"    */

#include "interlacemodes.h"

// AUTO FIX METHOD ====================

void ilaceautofixoption_to_text(char *string, int autofixoption)
{
	const char *cp = 0;
	switch(autofixoption) {
	case ILACE_AUTOFIXOPTION_AUTO:		cp = ILACE_AUTOFIXOPTION_AUTO_T;	break;
	case ILACE_AUTOFIXOPTION_MANUAL:	cp = ILACE_AUTOFIXOPTION_MANUAL_T;	break;
	default: cp = ILACE_UNKNOWN_T;  break;
	}
	strcpy(string, _(cp));
}

int ilaceautofixoption_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, _(ILACE_AUTOFIXOPTION_AUTO_T)))  	return ILACE_AUTOFIXOPTION_AUTO;
	if(!strcasecmp(text, _(ILACE_AUTOFIXOPTION_MANUAL_T)))	return ILACE_AUTOFIXOPTION_MANUAL;
	return thedefault;
}

// INTERLACE MODE ====================

void ilacemode_to_text(char *string, int ilacemode)
{
	const char *cp = 0;
	switch(ilacemode) {
	case ILACE_MODE_UNDETECTED:     cp = ILACE_MODE_UNDETECTED_T;      break;
	case ILACE_MODE_TOP_FIRST:      cp = ILACE_MODE_TOP_FIRST_T;       break;
	case ILACE_MODE_BOTTOM_FIRST:   cp = ILACE_MODE_BOTTOM_FIRST_T;    break;
	case ILACE_MODE_NOTINTERLACED:  cp = ILACE_MODE_NOTINTERLACED_T;   break;
 	default: cp = ILACE_UNKNOWN_T;  break;
	}
	strcpy(string, _(cp));
}

int ilacemode_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, _(ILACE_MODE_UNDETECTED_T)))     return ILACE_MODE_UNDETECTED;
	if(!strcasecmp(text, _(ILACE_MODE_TOP_FIRST_T)))      return ILACE_MODE_TOP_FIRST;
	if(!strcasecmp(text, _(ILACE_MODE_BOTTOM_FIRST_T)))   return ILACE_MODE_BOTTOM_FIRST;
	if(!strcasecmp(text, _(ILACE_MODE_NOTINTERLACED_T)))  return ILACE_MODE_NOTINTERLACED;
	return thedefault;
}

void ilacemode_to_xmltext(char *string, int ilacemode)
{
	switch(ilacemode) {
	case ILACE_MODE_UNDETECTED:     strcpy(string, ILACE_MODE_UNDETECTED_XMLT);      return;
	case ILACE_MODE_TOP_FIRST:      strcpy(string, ILACE_MODE_TOP_FIRST_XMLT);       return;
	case ILACE_MODE_BOTTOM_FIRST:   strcpy(string, ILACE_MODE_BOTTOM_FIRST_XMLT);    return;
	case ILACE_MODE_NOTINTERLACED:  strcpy(string, ILACE_MODE_NOTINTERLACED_XMLT);   return;
	}
	strcpy(string, ILACE_UNKNOWN_T);
}

int ilacemode_from_xmltext(const char *text, int thedefault)
{
	if( text ) {
		if(!strcasecmp(text, ILACE_MODE_UNDETECTED_XMLT))     return ILACE_MODE_UNDETECTED;
		if(!strcasecmp(text, ILACE_MODE_TOP_FIRST_XMLT))      return ILACE_MODE_TOP_FIRST;
		if(!strcasecmp(text, ILACE_MODE_BOTTOM_FIRST_XMLT))   return ILACE_MODE_BOTTOM_FIRST;
		if(!strcasecmp(text, ILACE_MODE_NOTINTERLACED_XMLT))  return ILACE_MODE_NOTINTERLACED;
	}
	return thedefault;
}

// INTERLACE FIX METHOD ====================

void ilacefixmethod_to_text(char *string, int fixmethod)
{
	const char *cp = 0;
	switch(fixmethod) {
	case ILACE_FIXMETHOD_NONE:   	cp = ILACE_FIXMETHOD_NONE_T;   	break;
	case ILACE_FIXMETHOD_UPONE:  	cp = ILACE_FIXMETHOD_UPONE_T;  	break;
	case ILACE_FIXMETHOD_DOWNONE:	cp = ILACE_FIXMETHOD_DOWNONE_T;	break;
	default: cp = ILACE_UNKNOWN_T;  break;
	}
	strcpy(string, _(cp));
}

int ilacefixmethod_from_text(const char *text, int thedefault)
{
	if(!strcasecmp(text, _(ILACE_FIXMETHOD_NONE_T)))   	return ILACE_FIXMETHOD_NONE;
	if(!strcasecmp(text, _(ILACE_FIXMETHOD_UPONE_T)))  	return ILACE_FIXMETHOD_UPONE;
	if(!strcasecmp(text, _(ILACE_FIXMETHOD_DOWNONE_T)))	return ILACE_FIXMETHOD_DOWNONE;
	return thedefault;
}

void ilacefixmethod_to_xmltext(char *string, int fixmethod)
{
	switch(fixmethod) {
	case ILACE_FIXMETHOD_NONE:   	strcpy(string, ILACE_FIXMETHOD_NONE_XMLT);   	return;
	case ILACE_FIXMETHOD_UPONE:  	strcpy(string, ILACE_FIXMETHOD_UPONE_XMLT);  	return;
	case ILACE_FIXMETHOD_DOWNONE:	strcpy(string, ILACE_FIXMETHOD_DOWNONE_XMLT);	return;
	}
	strcpy(string, ILACE_UNKNOWN_T);
}

int ilacefixmethod_from_xmltext(const char *text, int thedefault)
{
	if(!strcasecmp(text, ILACE_FIXMETHOD_NONE_XMLT))   	return ILACE_FIXMETHOD_NONE;
	if(!strcasecmp(text, ILACE_FIXMETHOD_UPONE_XMLT))  	return ILACE_FIXMETHOD_UPONE;
	if(!strcasecmp(text, ILACE_FIXMETHOD_DOWNONE_XMLT))	return ILACE_FIXMETHOD_DOWNONE;
	return thedefault;
}

int  ilaceautofixmethod(int projectmode, int assetmode)
{
	if (projectmode == assetmode)
		return ILACE_FIXMETHOD_NONE;
	if( (projectmode == ILACE_MODE_BOTTOM_FIRST && assetmode == ILACE_MODE_TOP_FIRST ) ||
	    (projectmode == ILACE_MODE_TOP_FIRST  && assetmode == ILACE_MODE_BOTTOM_FIRST) )
		return ILACE_FIXDEFAULT;
	// still to implement anything else...
	return ILACE_FIXMETHOD_NONE;
}

int  ilaceautofixmethod2(int projectilacemode, int assetautofixoption, int assetilacemode, int assetfixmethod)
{
	if (assetautofixoption == ILACE_AUTOFIXOPTION_AUTO)
		return (ilaceautofixmethod(projectilacemode, assetilacemode));
	return (assetfixmethod);
}

int ilace_bc_to_yuv4mpeg(int ilacemode)
{
	switch (ilacemode) {
	case ILACE_MODE_UNDETECTED:	return(Y4M_UNKNOWN);
	case ILACE_MODE_TOP_FIRST:	return(Y4M_ILACE_TOP_FIRST);
	case ILACE_MODE_BOTTOM_FIRST: return(Y4M_ILACE_BOTTOM_FIRST);
	case ILACE_MODE_NOTINTERLACED: return(Y4M_ILACE_NONE);
	}
	return(Y4M_UNKNOWN);
}

int ilace_yuv4mpeg_to_bc(int ilacemode)
{
	switch (ilacemode) {
	case Y4M_UNKNOWN:		return (ILACE_MODE_UNDETECTED);
	case Y4M_ILACE_NONE:		return (ILACE_MODE_NOTINTERLACED);
	case Y4M_ILACE_TOP_FIRST:	return (ILACE_MODE_TOP_FIRST);
	case Y4M_ILACE_BOTTOM_FIRST:	return (ILACE_MODE_BOTTOM_FIRST);
//	case Y4M_ILACE_MIXED:		return (ILACE_MODE_UNDETECTED);  // fixme!!
	}
	return (ILACE_MODE_UNDETECTED);
}


void ilace_yuv4mpeg_mode_to_text(char *string, int ilacemode)
{
	const char *cp = 0;
	switch(ilacemode) {
	case Y4M_UNKNOWN:             cp = ILACE_Y4M_UKNOWN_T;       break;
	case Y4M_ILACE_NONE:          cp = ILACE_Y4M_NONE_T;         break;
	case Y4M_ILACE_TOP_FIRST:     cp = ILACE_Y4M_TOP_FIRST_T;    break;
	case Y4M_ILACE_BOTTOM_FIRST:  cp = ILACE_Y4M_BOTTOM_FIRST_T; break;
//	case Y4M_ILACE_MIXED:         cp = ILACE_Y4M_MIXED_T;        break;
	default: cp = ILACE_UNKNOWN_T;  break;
	}
	strcpy(string, _(cp));
}

