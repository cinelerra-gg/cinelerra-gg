#ifdef HAVE_LV2

/*
 * CINELERRA
 * Copyright (C) 2018 GG
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

#include "arraylist.h"
#include "cstrdup.h"
#include "language.h"
#include "pluginlv2.h"
#include "pluginlv2config.h"

#include <ctype.h>
#include <string.h>

PluginLV2UriTable::PluginLV2UriTable()
 : Mutex("PluginLV2UriTable::PluginLV2UriTable")
{
	set_array_delete();
}

PluginLV2UriTable::~PluginLV2UriTable()
{
	remove_all_objects();
}

LV2_URID PluginLV2UriTable::map(const char *uri)
{
	lock("PluginLV2UriTable::map");
	int i = 0, n = size();
	while( i<n && strcmp(uri, get(i)) ) ++i;
	if( i >= n ) append(cstrdup(uri));
	unlock();
	return i+1;
}

const char *PluginLV2UriTable::unmap(LV2_URID urid)
{
	lock("PluginLV2UriTable::unmap");
	int idx = urid - 1;
	const char *ret = idx>=0 && idx<size() ? get(idx) : 0;
	unlock();
	return ret;
}

PluginLV2Client_OptName:: PluginLV2Client_OptName(PluginLV2Client_Opt *opt)
{
	this->opt = opt;
	set_text(opt->get_name());
}

PluginLV2Client_OptValue::PluginLV2Client_OptValue(PluginLV2Client_Opt *opt)
{
	this->opt = opt;
	update();
}

int PluginLV2Client_OptValue::update()
{
	char val[BCSTRLEN];
	sprintf(val, "%f", opt->get_value());
	if( !strcmp(val, get_text()) ) return 0;
	set_text(val);
	return 1;
}


PluginLV2Client_Opt::PluginLV2Client_Opt(PluginLV2ClientConfig *conf, int idx)
{
	this->conf = conf;
	this->idx = idx;
	item_name = new PluginLV2Client_OptName(this);
	item_value = new PluginLV2Client_OptValue(this);
}

PluginLV2Client_Opt::~PluginLV2Client_Opt()
{
	delete item_name;
	delete item_value;
}

float PluginLV2Client_Opt::get_value()
{
	return conf->ctls[idx];
}

const char *PluginLV2Client_Opt::get_symbol()
{
	return conf->syms[idx];
}

void PluginLV2Client_Opt::set_value(float v)
{
	conf->ctls[idx] = v;
}

int PluginLV2Client_Opt::update(float v)
{
	set_value(v);
	return item_value->update();
}

const char *PluginLV2Client_Opt::get_name()
{
	return conf->names[idx];
}

PluginLV2ClientConfig::PluginLV2ClientConfig()
{
	names = 0;
	syms = 0;
	mins = 0;
	maxs = 0;
	ctls = 0;
	ports = 0;
	nb_ports = 0;
}

PluginLV2ClientConfig::~PluginLV2ClientConfig()
{
	reset();
	remove_all_objects();
}

void PluginLV2ClientConfig::reset()
{
	for( int i=0; i<nb_ports; ++i ) {
		delete [] names[i];
		delete [] syms[i];
	}
	delete [] names;  names = 0;
	delete [] syms;   syms = 0;
	delete [] mins;   mins = 0;
	delete [] maxs;   maxs = 0;
	delete [] ctls;   ctls = 0;
	delete [] ports;  ports = 0;
	nb_ports = 0;
}


int PluginLV2ClientConfig::equivalent(PluginLV2ClientConfig &that)
{
	PluginLV2ClientConfig &conf = *this;
	for( int i=0; i<that.size(); ++i ) {
		PluginLV2Client_Opt *topt = conf[i], *vopt = that[i];
		if( !EQUIV(topt->get_value(), vopt->get_value()) ) return 0;
	}
	return 1;
}

void PluginLV2ClientConfig::copy_from(PluginLV2ClientConfig &that)
{
	if( nb_ports != that.nb_ports ) {
		reset();
		nb_ports = that.nb_ports;
		names = new const char *[nb_ports];
		syms = new const char *[nb_ports];
		for( int i=0; i<nb_ports; ++i ) names[i] = syms[i] = 0;
		mins  = new float[nb_ports];
		maxs  = new float[nb_ports];
		ctls  = new float[nb_ports];
		ports = new int[nb_ports];
	}
	for( int i=0; i<nb_ports; ++i ) {
		delete [] names[i];  names[i] = cstrdup(that.names[i]);
		delete [] syms[i];   syms[i] = cstrdup(that.syms[i]);
		mins[i] = that.mins[i];
		maxs[i] = that.maxs[i];
		ctls[i] = that.ctls[i];
		ports[i] = ports[i];
	}
	remove_all_objects();
	for( int i=0; i<that.size(); ++i ) {
		append(new PluginLV2Client_Opt(this, that[i]->idx));
	}
}

void PluginLV2ClientConfig::interpolate(PluginLV2ClientConfig &prev, PluginLV2ClientConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

void PluginLV2ClientConfig::init_lv2(const LilvPlugin *lilv, PluginLV2 *lv2)
{
	reset();
	nb_ports = lilv_plugin_get_num_ports(lilv);
	names = new const char *[nb_ports];
	syms = new const char *[nb_ports];
	mins  = new float[nb_ports];
	maxs  = new float[nb_ports];
	ctls  = new float[nb_ports];
	ports = new int[nb_ports];
	lilv_plugin_get_port_ranges_float(lilv, mins, maxs, ctls);
	for( int i=0; i<nb_ports; ++i ) {
		const LilvPort *lp = lilv_plugin_get_port_by_index(lilv, i);
		LilvNode *pnm = lilv_port_get_name(lilv, lp);
		names[i] = cstrdup(lilv_node_as_string(pnm));
		lilv_node_free(pnm);
		const LilvNode *sym = lilv_port_get_symbol(lilv, lp);
		syms[i] = cstrdup(lilv_node_as_string(sym));
		int port = 0;
		if( lilv_port_is_a(lilv, lp, lv2->lv2_AudioPort) )   port |= PORTS_AUDIO;
		if( lilv_port_is_a(lilv, lp, lv2->lv2_ControlPort) ) port |= PORTS_CONTROL;
		if( lilv_port_is_a(lilv, lp, lv2->lv2_InputPort) )   port |= PORTS_INPUT;
		if( lilv_port_is_a(lilv, lp, lv2->lv2_OutputPort) )  port |= PORTS_OUTPUT;
		if( lilv_port_is_a(lilv, lp, lv2->atom_AtomPort) )   port |= PORTS_ATOM;
		ports[i] = port;
	}
}

int PluginLV2ClientConfig::update()
{
	int ret = 0;
	PluginLV2ClientConfig &conf = *this;
	for( int i=0; i<size(); ++i ) {
		if( conf[i]->item_value->update() ) ++ret;
	}
	return ret;
}

void PluginLV2ClientConfig::dump(FILE *fp)
{
	fprintf(fp, "Controls:\n");
	for( int i=0; i<size(); ++i ) {
		fprintf(fp, " %3d. (%3d)  %-24s  %f\n",
			i, get(i)->idx, get(i)->get_symbol(), get(i)->get_value());
	}
	fprintf(fp, "Ports:\n");
	for( int i=0; i<nb_ports; ++i ) {
		fprintf(fp, " %3d.  %-24s  %f  (%f - %f) %s\n",
			i, syms[i], ctls[i], mins[i], maxs[i], names[i]);
	}
}

#endif /* HAVE_LV2 */
