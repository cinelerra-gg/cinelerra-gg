
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

#ifndef BCFONTENTRY_H
#define BCFONTENTRY_H

#include "bcfontentry.inc"
#include "vframe.h"

class BC_FontEntry
{
public:
	BC_FontEntry();
	~BC_FontEntry();

	void dump();

	VFrame *image;
	char *displayname;
	char *path;
	char *foundry;
	char *family;
	char *weight;
	char *slant;
	char *swidth;
	char *adstyle;
	int pixelsize;
	int pointsize;
	int xres;
	int yres;
	int style;
	char *spacing;
	int avg_width;
	char *registry;
	char *encoding;
	int fixed_style;
};

#endif
