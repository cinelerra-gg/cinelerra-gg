#ifndef __PROXY_H__
#define __PROXY_H__

/*
 * CINELERRA
 * Copyright (C) 2015 Adam Williams <broadcast at earthling dot net>
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

// functions for handling proxies

#include "arraylist.h"
#include "audiodevice.inc"
#include "asset.h"
#include "bcdialog.h"
#include "cache.inc"
#include "file.inc"
#include "formattools.inc"
#include "loadbalance.h"
#include "mutex.inc"
#include "mwindow.inc"
#include "proxy.inc"
#include "renderengine.inc"

#define MAX_SIZES 16

class ProxyMenuItem : public BC_MenuItem
{
public:
	ProxyMenuItem(MWindow *mwindow);
	~ProxyMenuItem();

	int handle_event();
	void create_objects();

	MWindow *mwindow;
	ProxyDialog *dialog;
};

class FromProxyMenuItem : public BC_MenuItem
{
public:
	FromProxyMenuItem(MWindow *mwindow);

	int handle_event();
	MWindow *mwindow;
};


class ProxyRender
{
public:
	ProxyRender(MWindow *mwindow, Asset *format_asset);
	~ProxyRender();
	void to_proxy_path(char *new_path, Indexable *indexable, int scale);
	static int from_proxy_path(char *new_path, Indexable *indexable, int scale);

	ArrayList<Indexable *> orig_idxbls;   // originals which match the proxy assets
	ArrayList<Indexable *> orig_proxies;  // proxy assets
	Asset *add_original(Indexable *idxbl, int new_scale);
	ArrayList<Indexable *> needed_idxbls; // originals which match the needed_assets
	ArrayList<Asset *> needed_proxies;    // assets which must be created
	void add_needed(Indexable *idxbl, Asset *proxy);

	int create_needed_proxies(int new_scale);
// increment the frame count by 1
	void update_progress();
// if user canceled progress bar
	int is_canceled();

	MWindow *mwindow;
	Asset *format_asset;
	MainProgressBar *progress;
	Mutex *counter_lock;
	int total_rendered;
	int failed, canceled;
};

class ProxyDialog : public BC_DialogThread
{
public:
	ProxyDialog(MWindow *mwindow);
	~ProxyDialog();
	BC_Window* new_gui();
	void handle_close_event(int result);

	void from_proxy();
// calculate possible sizes based on the original size
	void calculate_sizes();
	void scale_to_text(char *string, int scale);

	MWindow *mwindow;
	ProxyWindow *gui;
	Asset *asset;
	ProxyRender *proxy_render;

	int new_scale;
	int orig_scale;
	int use_scaler;
	int auto_scale;
	int beep;
	char *size_text[MAX_SIZES];
	int size_factors[MAX_SIZES];
	int total_sizes;
};

class ProxyUseScaler : public BC_CheckBox
{
public:
	ProxyUseScaler(ProxyWindow *pwindow, int x, int y);
	void update();
	int handle_event();

	ProxyWindow *pwindow;
};

class ProxyAutoScale : public BC_CheckBox
{
public:
	ProxyAutoScale(ProxyWindow *pwindow, int x, int y);
	void update();
	int handle_event();

	ProxyWindow *pwindow;
};

class ProxyBeepOnDone : public BC_CheckBox
{
public:
	ProxyBeepOnDone(ProxyWindow *pwindow, int x, int y);
	void update();
	int handle_event();

	ProxyWindow *pwindow;
};

class ProxyFormatTools : public FormatTools
{
public:
	ProxyFormatTools(MWindow *mwindow, ProxyWindow *window, Asset *asset);

	void update_format();
	ProxyWindow *pwindow;
};

class ProxyMenu : public BC_PopupMenu
{
public:
	ProxyMenu(MWindow *mwindow, ProxyWindow *pwindow,
		int x, int y, int w, const char *text);
	void update_sizes();
	int handle_event();
	MWindow *mwindow;
	ProxyWindow *pwindow;
};


class ProxyTumbler : public BC_Tumbler
{
public:
	ProxyTumbler(MWindow *mwindow, ProxyWindow *pwindow, int x, int y);

	int handle_up_event();
	int handle_down_event();
	
	ProxyWindow *pwindow;
	MWindow *mwindow;
};


class ProxyWindow : public BC_Window
{
public:
	ProxyWindow(MWindow *mwindow, ProxyDialog *dialog,
		int x, int y);
	~ProxyWindow();

	void create_objects();
	void update();

	MWindow *mwindow;
	ProxyDialog *dialog;
	FormatTools *format_tools;
	BC_Title *new_dimensions;
	BC_Title *active_scale;
	ProxyMenu *scale_factor;
	ProxyUseScaler *use_scaler;
	ProxyAutoScale *auto_scale;
	ProxyBeepOnDone *beep_on_done;
};

class ProxyFarm;

class ProxyPackage : public LoadPackage
{
public:
	ProxyPackage();
	Indexable *orig_idxbl;
	Asset *proxy_asset;
};

class ProxyClient : public LoadClient
{
public:
	ProxyClient(MWindow *mwindow, ProxyRender *proxy_render, ProxyFarm *server);
	~ProxyClient();
	void process_package(LoadPackage *package);

	MWindow *mwindow;
	ProxyRender *proxy_render;
	RenderEngine *render_engine;
	CICache *video_cache;
	File *src_file;
};


class ProxyFarm : public LoadServer
{
public:
	ProxyFarm(MWindow *mwindow, ProxyRender *proxy_render, 
		ArrayList<Indexable*> *orig_idxbls, ArrayList<Asset*> *proxy_assets);
	
	void init_packages();
	LoadClient* new_client();
	LoadPackage* new_package();
	
	MWindow *mwindow;
	ProxyRender *proxy_render;
	ArrayList<Indexable*> *orig_idxbls;
	ArrayList<Asset*> *proxy_assets;
};

class ProxyBeep : public Thread
{
public:
	enum { BEEP_SAMPLE_RATE=48000 };
	typedef int16_t audio_data_t;
	ProxyBeep(MWindow *mwindow);
	~ProxyBeep();

	void run();
	void start();
	void stop(int wait);
	void tone(double freq, double secs, double gain);

	MWindow *mwindow;
	double freq, secs, gain;
	AudioDevice *audio;
	int playing_audio, interrupted;
	int audio_pos;
};

#endif
