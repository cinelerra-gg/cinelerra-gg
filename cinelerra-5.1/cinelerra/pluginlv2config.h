
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

#ifndef PLUGINLV2CONFIG_H
#define PLUGINLV2CONFIG_H

#include "arraylist.h"
#include "cstrdup.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "pluginlv2.inc"
#include "pluginlv2config.inc"
#include "samples.inc"

#ifdef HAVE_LV2
#include <lilv/lilv.h>
#include <suil/suil.h>

#define NS_UI "http://lv2plug.in/ns/extensions/ui#"
#include <lv2/lv2plug.in/ns/extensions/ui/ui.h>
#include <lv2/lv2plug.in/ns/ext/atom/atom.h>
#include <lv2/lv2plug.in/ns/ext/urid/urid.h>
#include <lv2/lv2plug.in/ns/ext/state/state.h>
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
#include <lv2/lv2plug.in/ns/ext/log/log.h>
#include <lv2/lv2plug.in/ns/ext/options/options.h>
#include <lv2/lv2plug.in/ns/ext/parameters/parameters.h>
#include <lv2/lv2plug.in/ns/ext/buf-size/buf-size.h>
#include <lv2/lv2plug.in/ns/ext/uri-map/uri-map.h>
#include <lv2/lv2plug.in/ns/ext/data-access/data-access.h>


class Lv2Feature : public LV2_Feature
{
public:
	Lv2Feature(const char *uri, void *dp) {
		this->URI = cstrdup(uri);  this->data = dp;
	}
	~Lv2Feature() { delete [] URI; }
};
class Lv2Features : public ArrayList<Lv2Feature *>
{
public:
	Lv2Features() {}
	~Lv2Features() { remove_all_objects(); }
	operator LV2_Feature **() { return (LV2_Feature **)&values[0]; }
};

class PluginLV2UriTable : public ArrayList<const char *>, public Mutex
{
public:
	PluginLV2UriTable();
	~PluginLV2UriTable();
	LV2_URID map(const char *uri);
	const char *unmap(LV2_URID urid);
	operator LV2_URID_Map_Handle() { return (LV2_URID_Map_Handle)this; }
};

#endif

class PluginLV2Client_OptName : public BC_ListBoxItem
{
public:
	PluginLV2Client_Opt *opt;
	PluginLV2Client_OptName(PluginLV2Client_Opt *opt);
};

class PluginLV2Client_OptValue : public BC_ListBoxItem
{
public:
	PluginLV2Client_Opt *opt;
	PluginLV2Client_OptValue(PluginLV2Client_Opt *opt);
	int update();
};

class PluginLV2Client_Opt
{
public:
	int idx;
	const char *sym;
	PluginLV2ClientConfig *conf;
	PluginLV2Client_OptName *item_name;
	PluginLV2Client_OptValue *item_value;
	float get_value();
	const char *get_symbol();
	void set_value(float v);
	int update(float v);
	const char *get_name();

	PluginLV2Client_Opt(PluginLV2ClientConfig *conf, int idx);
	~PluginLV2Client_Opt();
};


class PluginLV2ClientConfig : public ArrayList<PluginLV2Client_Opt *>
{
public:
	PluginLV2ClientConfig();
	~PluginLV2ClientConfig();

	int equivalent(PluginLV2ClientConfig &that);
	void copy_from(PluginLV2ClientConfig &that);
	void interpolate(PluginLV2ClientConfig &prev, PluginLV2ClientConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	void reset();
	void init_lv2(const LilvPlugin *lilv, PluginLV2 *lv2);
	int update();
	void dump(FILE *fp);

	int nb_ports, *ports;
	const char **names, **syms;
	float *mins, *maxs, *ctls;
};

#endif
