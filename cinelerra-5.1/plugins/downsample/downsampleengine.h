
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




#ifndef DOWNSAMPLEENGINE_H
#define DOWNSAMPLEENGINE_H

#include "loadbalance.h"
#include "vframe.inc"

class DownSampleServer;

class DownSamplePackage : public LoadPackage
{
public:
	DownSamplePackage();
	int y1, y2;
};

class DownSampleUnit : public LoadClient
{
public:
	DownSampleUnit(DownSampleServer *server);
	void process_package(LoadPackage *package);
	DownSampleServer *server;
};

class DownSampleServer : public LoadServer
{
public:
	DownSampleServer(int total_clients,
		int total_packages);
	void process_frame(VFrame *output,
		VFrame *input,
		int r,
		int g,
		int b,
		int a,
		int vertical,
		int horizontal,
		int vertical_y,
		int horizontal_x);
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();

	VFrame *output;
	VFrame *input;
	int r;
	int g;
	int b;
	int a;
	int vertical;
	int horizontal;
	int vertical_y;
	int horizontal_x;
};


#endif

