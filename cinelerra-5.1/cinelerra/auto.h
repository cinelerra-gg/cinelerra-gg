
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

#ifndef AUTO_H
#define AUTO_H

#include "auto.inc"
#include "edl.inc"
#include "guicast.h"
#include "filexml.inc"
#include "autos.inc"






// The default constructor is used for menu effects.

class Auto : public ListItem<Auto>
{
public:
	Auto();
	Auto(EDL *edl, Autos *autos);
	virtual ~Auto() {};

	virtual Auto& operator=(Auto &that);
	virtual int operator==(Auto &that);
	virtual void copy_from(Auto *that);
	virtual bool is_floatauto() { return false; }
	/* create an interpolation using a1 and a2, (defaulting to previous and next)  */
	/* if not possible, just fill from a1 (or from template if given) and return 0  */
	virtual int interpolate_from(Auto *a1, Auto *a2, int64_t new_position, Auto *templ=0);
	virtual void copy(int64_t start, int64_t end, FileXML *file, int default_only);

	virtual void load(FileXML *file);

	virtual void get_caption(char *string) {};


	int skip;       // if added by selection event for moves
	EDL *edl;
	Autos *autos;
	int WIDTH, HEIGHT;
// Units native to the track
	int is_default;
	int64_t position;
private:
	virtual int value_to_str(char *string, float value) { return 1; }
};

#endif
