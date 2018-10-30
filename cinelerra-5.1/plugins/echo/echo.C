
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "bcdisplayinfo.h"
#include "clip.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "bccolors.h"
#include "samples.h"
#include "echo.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"


#include <string.h>


REGISTER_PLUGIN(Echo)


EchoConfig::EchoConfig()
{
	level = 0.0;
	atten = -6.0;
	offset = 100;
}

int EchoConfig::equivalent(EchoConfig &that)
{
	return EQUIV(level, that.level) &&
		atten == that.atten &&
		offset == that.offset;
}

void EchoConfig::copy_from(EchoConfig &that)
{
	level = that.level;
	atten = that.atten;
	offset = that.offset;
}

void EchoConfig::interpolate(EchoConfig &prev, EchoConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	double frames = next_frame - prev_frame;
        double next_scale = (current_frame - prev_frame) / frames;
        double prev_scale = (next_frame - current_frame) / frames;
        level  = prev.level  * prev_scale + next.level  * next_scale;
        atten   = prev.atten   * prev_scale + next.atten   * next_scale;
        offset = prev.offset * prev_scale + next.offset * next_scale;
}




EchoWindow::EchoWindow(Echo *plugin)
 : PluginClientWindow(plugin, 250, 100, 250, 100, 0)
{
	this->plugin = plugin;
}

EchoWindow::~EchoWindow()
{
}


EchoLevel::EchoLevel(EchoWindow *window, int x, int y)
 : BC_FPot(x, y, window->plugin->config.level, INFINITYGAIN, MAXGAIN)
{
        this->window = window;
}
EchoLevel::~EchoLevel()
{
}
int EchoLevel::handle_event()
{
	Echo *plugin = (Echo *)window->plugin;
	plugin->config.level = get_value();
	plugin->send_configure_change();
	double l = plugin->db.fromdb(plugin->config.level);
	window->level_title->update(l);
	return 1;
}

EchoAtten::EchoAtten(EchoWindow *window, int x, int y)
 : BC_FPot(x, y, window->plugin->config.atten,INFINITYGAIN,-0.1)
{
        this->window = window;
}
EchoAtten::~EchoAtten()
{
}
int EchoAtten::handle_event()
{
	Echo *plugin = (Echo *)window->plugin;
	plugin->config.atten = get_value();
	plugin->send_configure_change();
	double g = plugin->db.fromdb(plugin->config.atten);
	window->atten_title->update(g);
	return 1;
}

EchoOffset::EchoOffset(EchoWindow *window, int x, int y)
 : BC_FPot(x, y, window->plugin->config.offset, 1, 999)
{
        this->window = window;
}
EchoOffset::~EchoOffset()
{
}
int EchoOffset::handle_event()
{
	Echo *plugin = (Echo *)window->plugin;
	plugin->config.offset = get_value();
	plugin->send_configure_change();
	window->offset_title->update((int)plugin->config.offset);
	return 1;
}

void EchoWindow::create_objects()
{
	int x = 170, y = 10;
	add_subwindow(level_title=new EchoTitle(5, y + 10, _("Level: "),
		plugin->db.fromdb(plugin->config.level)));
	add_subwindow(level = new EchoLevel(this, x, y)); y += 25;
	add_subwindow(atten_title=new EchoTitle(5, y + 10, _("Atten: "),
		plugin->db.fromdb(plugin->config.atten)));
	add_subwindow(atten = new EchoAtten(this, x + 35, y)); y += 25;
	add_subwindow(offset_title=new EchoTitle(5, y + 10, _("Offset: "),
		(int)plugin->config.offset));
	add_subwindow(offset = new EchoOffset(this, x, y)); y += 25;
	show_window();
}

int EchoWindow::resize_event(int w, int h)
{
	return 0;
}

void EchoWindow::update_gui()
{
	level->update(plugin->config.level);
	double l = plugin->db.fromdb(plugin->config.level);
	level_title->update(l);
	atten->update(plugin->config.atten);
	double g = plugin->db.fromdb(plugin->config.atten);
	atten->update(g);
	offset->update(plugin->config.offset);
	int ofs = plugin->config.offset;
	offset_title->update(ofs);
}


void Echo::update_gui()
{
	if( !thread ) return;
	EchoWindow *window = (EchoWindow*)thread->get_window();
	window->lock_window("Echo::update_gui");
	if( load_configuration() )
                window->update_gui();
        window->unlock_window();
}

Echo::Echo(PluginServer *server)
 : PluginAClient(server)
{
	reset();
}

Echo::~Echo()
{
	delete_buffers();
}


void Echo::reset()
{
	thread = 0;
	rbfr = 0;
	rpos = 0;
	ibfr = 0;
	nch = 0;
	isz = 0;
	rsz = 0;
	bfr_pos = 0;
}

const char* Echo::plugin_title() { return N_("Echo"); }
int Echo::is_realtime() { return 1; }
int Echo::is_multichannel() { return 1; }

void Echo::delete_buffers()
{
       	for( int ch=0; ch<nch; ++ch ) {
		delete [] ibfr[ch];
		delete rbfr[ch];
	}
	delete [] ibfr;  ibfr = 0;
	delete [] rbfr;  rbfr = 0;
	isz = rsz = nch = 0;
}

void Echo::alloc_buffers(int nch, int isz, int rsz)
{
	// allocate reflection buffers
	ibfr = new double *[nch];
	rbfr = new ring_buffer *[nch];
       	for( int ch=0; ch<nch; ++ch ) {
		ibfr[ch] = new double[isz];
		rbfr[ch] = new ring_buffer(rsz);
	}
	this->nch = nch;
	this->rsz = rsz;
	this->isz = isz;
}

int Echo::process_buffer(int64_t size, Samples **buffer, int64_t start_position, int sample_rate)
{
	load_configuration();

	double level = db.fromdb(config.level);
	double atten = db.fromdb(config.atten);
	int ofs = config.offset * sample_rate/1000.;
	int len = size * ((ofs + size-1) / size);

	if( nch != total_in_buffers || isz != size || rsz != len )
		delete_buffers();
	if( !ibfr ) {
		alloc_buffers(total_in_buffers, size, len);
		bfr_pos = -1;
	}
	if( bfr_pos != start_position ) {	// reset reflection
		bfr_pos = start_position;
       		for( int ch=0; ch<nch; ++ch ) rbfr[ch]->clear();
		rpos = 0;
	}

	for( int ch=0; ch<nch; ++ch ) {		// read samples * level
               	read_samples(buffer[ch], ch, sample_rate, start_position, size);
               	double *bp = buffer[ch]->get_data(), *cp = ibfr[ch];
		for( int ch=0; ch<size; ++ch ) *cp++ = *bp++ * level;
	}
	bfr_pos += size;
	int n = len - ofs;

	for( int ch=0; ch<nch; ++ch ) {
		ring_buffer *rp = rbfr[ch];
		rp->iseek(rpos);  rp->oseek(rpos+ofs);
		double *bp = ibfr[ch];
		for( int i=n; --i>=0; )		// add front to end reflection
			rp->onxt() = atten * (rp->inxt() + *bp++);

		double *op = buffer[ch]->get_data(), *ip = ibfr[ch];
		rp->iseek(rpos);
		for( int i=size; --i>=0; )	// output reflection + input
			*op++ = rp->inxt() + *ip++;

		rp->iseek(rpos+n);
		for( int i=size-n; --i>=0; )	// add end to front reflection
			rp->onxt() = atten * (rp->inxt() + *bp++);
	}
	rpos += size;

        return 0;
}



NEW_WINDOW_MACRO(Echo, EchoWindow)


int Echo::load_configuration()
{
        KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
        EchoConfig old_config;
        old_config.copy_from(config);
        read_data(prev_keyframe);
        return !old_config.equivalent(config) ? 1 : 0;
}

void Echo::read_data(KeyFrame *keyframe)
{
	int result;
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while(!(result = input.read_tag()) ) {
		if( !input.tag.title_is("ECHO")) continue;
		config.level = input.tag.get_property("LEVEL", config.level);
		config.atten = input.tag.get_property("ATTEN", config.atten);
		config.offset = input.tag.get_property("OFFSET", config.offset);
		if( !is_defaults() ) continue;
	}
}

void Echo::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("ECHO");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("ATTEN", config.atten);
	output.tag.set_property("OFFSET", config.offset);
	output.append_tag();
	output.tag.set_title("/ECHO");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}



