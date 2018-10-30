#ifndef __PLUGINLV2GUI_H__
#define __PLUGINLV2GUI_H__

#include "guicast.h"
#include "forkbase.h"
#include "pluginlv2gui.inc"
#include "pluginlv2ui.h"
#include "pluginaclient.h"

class PluginLV2ClientUI : public BC_GenericButton {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientUI(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientUI();
	int handle_event();
};

class PluginLV2ClientReset : public BC_GenericButton
{
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientReset(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientReset();
	int handle_event();
};

class PluginLV2ClientText : public BC_TextBox {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientText(PluginLV2ClientWindow *gui, int x, int y, int w);
	~PluginLV2ClientText();
	int handle_event();
};

class PluginLV2ClientApply : public BC_GenericButton {
public:
	PluginLV2ClientWindow *gui;

	PluginLV2ClientApply(PluginLV2ClientWindow *gui, int x, int y);
	~PluginLV2ClientApply();
	int handle_event();
};

class PluginLV2Client_OptPanel : public BC_ListBox
{
public:
	PluginLV2Client_OptPanel(PluginLV2ClientWindow *gui, int x, int y, int w, int h);
	~PluginLV2Client_OptPanel();

	PluginLV2ClientWindow *gui;
	ArrayList<BC_ListBoxItem*> items[2];
	ArrayList<BC_ListBoxItem*> &opts;
	ArrayList<BC_ListBoxItem*> &vals;

	int selection_changed();
	void update();
};

class PluginLV2ClientPot : public BC_FPot
{
public:
	PluginLV2ClientPot(PluginLV2ClientWindow *gui, int x, int y);
	int handle_event();
	PluginLV2ClientWindow *gui;
};

class PluginLV2ClientSlider : public BC_FSlider
{
public:
	PluginLV2ClientSlider(PluginLV2ClientWindow *gui, int x, int y);
	int handle_event();
	PluginLV2ClientWindow *gui;
};

class PluginLV2ClientWindow : public PluginClientWindow
{
public:
	PluginLV2ClientWindow(PluginLV2Client *client);
	~PluginLV2ClientWindow();
	void done_event(int result);

	void create_objects();
	int resize_event(int w, int h);
	void update_selected();
	void update_selected(float v);
	int scalar(float f, char *rp);
	void update(PluginLV2Client_Opt *opt);
	void lv2_update();
	void lv2_update(float *vals);
	void lv2_ui_enable();
	void lv2_set(int idx, float val);
	PluginLV2ParentUI *find_ui();
	PluginLV2ParentUI *get_ui();

	PluginLV2Client *client;
	PluginLV2ClientUI *client_ui;
	PluginLV2ClientReset *reset;
	PluginLV2ClientApply *apply;
	PluginLV2ClientPot *pot;
	PluginLV2ClientSlider *slider;
	PluginLV2ClientText *text;
	BC_Title *varbl, *range;
	PluginLV2Client_OptPanel *panel;
	PluginLV2Client_Opt *selected;

};

#endif
