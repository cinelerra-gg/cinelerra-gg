#ifndef __PROBEPREFS_H__
#define __PROBEPREFS_H__

#include "bcwindowbase.h"
#include "bcbutton.h"
#include "bcdialog.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "bctoggle.h"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "thread.h"
#include "probeprefs.inc"


class FileProbeDialog : public BC_DialogThread
{
public:
	PreferencesWindow *pwindow;

        ProbeEditWindow *pb_window;
	BC_Window* new_gui();
	void handle_close_event(int result);

	FileProbeDialog(PreferencesWindow *pwindow);
	~FileProbeDialog();
};

class ProbePref
{
public:
	char name[BCSTRLEN];
	int armed;

	ProbePref(const char *nm, int armed);
	~ProbePref();
};

class ProbeUpButton : public BC_GenericButton {
public:
	ProbeEditWindow *pb_window;
	int handle_event();

	ProbeUpButton(ProbeEditWindow *pb_window, int x, int y);
	~ProbeUpButton();
};

class ProbeDownButton : public BC_GenericButton {
public:
	ProbeEditWindow *pb_window;
	int handle_event();

	ProbeDownButton(ProbeEditWindow *pb_window, int x, int y);
	~ProbeDownButton();
};

class ProbeEnabled : public BC_CheckBox
{
public:
	ProbeEditWindow *pb_window;
	int handle_event();

	ProbeEnabled(ProbeEditWindow *pb_window, int x, int y);
	~ProbeEnabled();
};

class ProbePrefItem : public BC_ListBoxItem {
public:
	ProbeEditWindow *pb_window;
	int armed;
	void set_armed(int armed);

	ProbePrefItem(ProbeEditWindow *pb_window, ProbePref *pref);
	~ProbePrefItem();
};

class ProbePrefList : public BC_ListBox
{
public:
	ProbeEditWindow *pb_window;
	int handle_event();
	int selection_changed();

	ProbePrefList(ProbeEditWindow *pb_window, int x, int y);
	~ProbePrefList();
};

class ProbeEditOK : public BC_OKButton
{
public:
	ProbeEditWindow *pb_window;
	int handle_event();

	ProbeEditOK(ProbeEditWindow *pb_window);
	~ProbeEditOK();
};

class ProbeEditWindow : public BC_Window
{
public:
	ProbeUpButton *probe_up_button;
	ProbeDownButton *probe_down_button;
	ProbeEnabled *probe_enabled;
	ArrayList<ProbePrefItem *> probe_items;
	ProbePrefList *probe_list;
	BC_Pixmap *pb_enabled, *pb_disabled;

	void create_objects();
	int list_update();

	ProbeEditWindow(FileProbeDialog *pb_dialog, int x, int y);
	~ProbeEditWindow();

	FileProbeDialog *pb_dialog;
};

#endif
