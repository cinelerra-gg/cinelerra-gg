
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

#ifndef __PLUGINLV2UI_H__
#define __PLUGINLV2UI_H__

#include "forkbase.h"
#include "pluginlv2.h"
#include "pluginlv2client.h"
#include "pluginlv2gui.inc"
#include "pluginlv2ui.inc"

//=== lv2_external_ui.h
#define LV2_EXTERNAL_UI_URI "http://lv2plug.in/ns/extensions/ui#external"
#define LV2_EXTERNAL_UI_URI__KX__Host "http://kxstudio.sf.net/ns/lv2ext/external-ui#Host"

typedef struct _lv2_external_ui lv2_external_ui;
struct _lv2_external_ui {
        void (*run)(lv2_external_ui *ui);
        void (*show)(lv2_external_ui *ui);
        void (*hide)(lv2_external_ui *ui);
        void *self;
};

typedef struct _lv2_external_ui_host lv2_external_ui_host;
struct _lv2_external_ui_host {
        void (*ui_closed)(void* controller);
        const char *plugin_human_id;
};

#define LV2_EXTERNAL_UI_RUN(ptr) (ptr)->run(ptr)
#define LV2_EXTERNAL_UI_SHOW(ptr) (ptr)->show(ptr)
#define LV2_EXTERNAL_UI_HIDE(ptr) (ptr)->hide(ptr)
//===

typedef struct _GtkWidget GtkWidget;

#define UPDATE_HOST 1
#define UPDATE_PORTS 2

class PluginLV2UI : public PluginLV2
{
public:
	PluginLV2UI();
	~PluginLV2UI();

	const LilvUI *lilv_ui;
	const LilvNode *lilv_type;

	LV2_Extension_Data_Feature *ext_data;
	lv2_external_ui_host extui_host;
	const char *wgt_type;
	const char *gtk_type;
	const char *ui_type;

	char title[BCSTRLEN];
	PluginLV2ClientConfig config;
	int done, running;
	int hidden, updates;

	void start();
	void stop();
	int update_lv2_input(float *vals, int force);
	void update_lv2_output();
	virtual int send_host(int64_t token, const void *data, int bytes) = 0;
	virtual int child_iteration(int64_t usec) { return -1; }

	void run_lilv(int samples);
	void update_value(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	void update_control(int idx, uint32_t bfrsz, uint32_t typ, const void *bfr);
	static void write_from_ui(void *the, uint32_t idx,
		uint32_t bfrsz, uint32_t typ, const void *bfr);
	static uint32_t port_index(void* obj, const char* sym);

//	static void touch(void *obj,uint32_t pidx,bool grabbed);
	static uint32_t uri_to_id(LV2_URID_Map_Handle handle, const char *map, const char *uri);
	static LV2_URID uri_table_map(LV2_URID_Map_Handle handle, const char *uri);
	static const char *uri_table_unmap(LV2_URID_Map_Handle handle, LV2_URID urid);
	void lv2ui_instantiate(void *parent);
	bool lv2ui_resizable();
	void update_host();
};

class PluginLV2GUI
{
public:
	PluginLV2GUI(PluginLV2ChildUI *child_ui);
	virtual ~PluginLV2GUI();
	virtual void reset_gui() = 0;
	virtual void start_gui() = 0;
	virtual int handle_child() = 0;

	PluginLV2ChildUI *child_ui;
	void *top_level;
};

class PluginLV2ChildGtkUI : public PluginLV2GUI
{
public:
	PluginLV2ChildGtkUI(PluginLV2ChildUI *child_ui);
	~PluginLV2ChildGtkUI();

	void reset_gui();
	void start_gui();
	int handle_child();
};

class PluginLV2ChildWgtUI : public PluginLV2GUI
{
public:
	PluginLV2ChildWgtUI(PluginLV2ChildUI *child_ui);
	~PluginLV2ChildWgtUI();

	void reset_gui();
	void start_gui();
	int handle_child();
};

class PluginLV2ChildUI : public ForkChild, public PluginLV2UI
{
public:
	PluginLV2ChildUI();
	~PluginLV2ChildUI();

	int init_ui(const char *path, int sample_rate, int bfrsz);
	void reset_gui();
	void start_gui();
	int handle_child();

	int is_forked() { return ppid; }
	int send_host(int64_t token, const void *data, int bytes);
	int child_iteration(int64_t usec);
	void run();
	int run(int ac, char **av);
	int run_ui();
	void run_buffer(int shmid);

	int ac;
	char **av;
	PluginLV2GUI *gui;
};

#endif
