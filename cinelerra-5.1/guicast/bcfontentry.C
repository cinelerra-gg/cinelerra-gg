
/*
 *
 * Copyright (C) 2014 Einar Rünkaru <einarry at smail dot ee>
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

#include "bcfontentry.h"

#include <stdio.h>


BC_FontEntry::BC_FontEntry()
{
	image = 0;
	displayname = 0;
	path = 0;
	foundry = 0;
	family = 0;
	weight = 0;
	slant = 0;
	swidth = 0;
	adstyle = 0;
	spacing = 0;
	pixelsize = 0;
	pointsize = 0;
	xres = 0;
	yres = 0;
	style = 0;
	spacing = 0;
	avg_width = 0;
	registry = 0;
	encoding = 0;
	fixed_style = 0;
}

BC_FontEntry::~BC_FontEntry()
{
	delete image;
	delete [] path;
	delete [] displayname;
	delete [] foundry;
	delete [] family;
	delete [] weight;
	delete [] slant;
	delete [] swidth;
	delete [] adstyle;
	delete [] spacing;
	delete [] registry;
	delete [] encoding;
}

void BC_FontEntry::dump()
{
	printf("%s: %s %s %s %s %s %s %d %d %d %d %s %d %s %s\n",
		path, foundry, family, weight, slant, swidth, adstyle, pixelsize,
		pointsize, xres, yres, spacing, avg_width, registry, encoding);
}

