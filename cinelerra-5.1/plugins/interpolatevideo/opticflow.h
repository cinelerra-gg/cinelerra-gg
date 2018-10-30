/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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
 * *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef OPTICFLOW_H
#define OPTICFLOW_H


#include "interpolatevideo.inc"
#include "loadbalance.h"
#include "motioncache-hv.inc"
#include "motionscan-hv.inc"
#include "opticflow.inc"

// Need a 2nd table if a large number of packages
class OpticFlowMacroblock
{
public:
	OpticFlowMacroblock();
	void copy_from(OpticFlowMacroblock *src);

	int x, y;
	int dx, dy;
	int is_valid;
// Temporaries for blending macroblocks
	float angle1, angle2;
	float dist;
	int visible;
};

class OpticFlowPackage : public LoadPackage
{
public:
	OpticFlowPackage();
	int macroblock0;
	int macroblock1;
};

class OpticFlowUnit : public LoadClient
{
public:
	OpticFlowUnit(OpticFlow *server);
	~OpticFlowUnit();
	void process_package(LoadPackage *package);
	MotionHVScan *motion;
	OpticFlow *server;
};


class OpticFlow : public LoadServer
{
public:
	OpticFlow(InterpolateVideo *plugin,
		int total_clients,
		int total_packages);
	~OpticFlow();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	InterpolateVideo *plugin;
	MotionHVCache *downsample_cache;
};









class WarpPackage : public LoadPackage
{
public:
	WarpPackage();
	int y1, y2;
};

class WarpUnit : public LoadClient
{
public:
	WarpUnit(Warp *server);
	~WarpUnit();
	void process_package(LoadPackage *package);
	Warp *server;
};


class Warp : public LoadServer
{
public:
	Warp(InterpolateVideo *plugin,
		int total_clients,
		int total_packages);
	~Warp();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	InterpolateVideo *plugin;
};






class BlendPackage : public LoadPackage
{
public:
	BlendPackage();
	int number0, number1;
};



class BlendMacroblockUnit : public LoadClient
{
public:
	BlendMacroblockUnit(BlendMacroblock *server);
	~BlendMacroblockUnit();
	void process_package(LoadPackage *package);
	BlendMacroblock *server;
};


class BlendMacroblock : public LoadServer
{
public:
	BlendMacroblock(InterpolateVideo *plugin,
		int total_clients,
		int total_packages);
	~BlendMacroblock();
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	InterpolateVideo *plugin;
};





#endif


