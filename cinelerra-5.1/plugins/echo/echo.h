
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

#ifndef ECHO_H
#define ECHO_H
#include "bchash.inc"
#include "bctimer.inc"
#include "fourier.h"
#include "guicast.h"
#include "mutex.h"
#include "pluginaclient.h"
#include "vframe.inc"

#include <string.h>


class Echo;


class EchoPrefix {
public:
	const char *prefix;
	EchoPrefix(const char *p) : prefix(p) {}
	~EchoPrefix() {}
};

class EchoTitle : public EchoPrefix, public BC_Title {
	char string[BCSTRLEN];
	char *preset(int value) {
		sprintf(string, "%s%d", prefix, value);
		return string;
	}
	char *preset(double value) {
		sprintf(string, "%s%.3f", prefix, value);
		return string;
	}
public:
	void update(int value) { BC_Title::update(preset(value)); }
	void update(double value) { BC_Title::update(preset(value)); }

	EchoTitle(int x, int y, const char *pfx, int value)
		: EchoPrefix(pfx), BC_Title(x, y, preset(value)) {}
	EchoTitle(int x, int y, const char *pfx, double value)
		:  EchoPrefix(pfx),BC_Title(x, y, preset(value)) {}
	~EchoTitle() {}
};


class EchoWindow;

class EchoLevel : public BC_FPot
{
public:
	EchoLevel(EchoWindow *window, int x, int y);
	~EchoLevel();
	int handle_event();
	EchoWindow *window;
};

class EchoAtten : public BC_FPot
{
public:
	EchoAtten(EchoWindow *window, int x, int y);
	~EchoAtten();
	int handle_event();
	EchoWindow *window;
};

class EchoOffset : public BC_FPot
{
public:
	EchoOffset(EchoWindow *window, int x, int y);
	~EchoOffset();
	int handle_event();
	EchoWindow *window;
};


class EchoWindow : public PluginClientWindow
{
public:
	EchoWindow(Echo *plugin);
	~EchoWindow();

	void create_objects();
	void update_gui();
	int resize_event(int w, int h);

	EchoTitle *level_title;
	EchoLevel *level;

	EchoTitle *atten_title;
	EchoAtten *atten;
	EchoTitle *offset_title;
	EchoOffset *offset;
	Echo *plugin;
};





class EchoConfig
{
public:
	EchoConfig();
	int equivalent(EchoConfig &that);
	void copy_from(EchoConfig &that);
	void interpolate(EchoConfig &prev, EchoConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame);
	double level;
	double atten;
	double offset;
};


class Echo : public PluginAClient
{
public:
	class ring_buffer {
		int bsz;
		double *ip, *op;	// in ptr, out ptr
		double *bfr, *lmt;	// first, limit
		void reset() { ip = op = bfr;  lmt = bfr + bsz; }
	public:
		ring_buffer(int sz) { bfr = new double[bsz = sz]; reset(); }
		~ring_buffer() { delete [] bfr; }
		void clear() { memset(bfr, 0, bsz*sizeof(*bfr)); }
		double &icur() { return  *ip; }
		double &ocur() { return  *op; }
		double &inxt() { double &v=*ip; if( ++ip>=lmt ) ip=bfr; return v; }
		double &onxt() { double &v=*op; if( ++op>=lmt ) op=bfr; return v; }
		int64_t iseek(int64_t n) { ip = bfr+(n%bsz); return n; }
		int64_t oseek(int64_t n) { op = bfr+(n%bsz); return n; }
	} **rbfr;
	int64_t rpos;

	Echo(PluginServer *server);
	~Echo();

	PLUGIN_CLASS_MEMBERS2(EchoConfig)
	int is_realtime();
	int is_multichannel();
	int process_buffer(int64_t size, Samples **buffer, int64_t start_position, int sample_rate);

	void read_data(KeyFrame *keyframe);
	void save_data(KeyFrame *keyframe);
	void update_gui();

	void reset();
	void alloc_buffers(int nch, int isz, int rsz);
	void delete_buffers();

	int nch, rsz, isz;
	int64_t bfr_pos;
	double **ibfr;
	DB db;
};


#endif
