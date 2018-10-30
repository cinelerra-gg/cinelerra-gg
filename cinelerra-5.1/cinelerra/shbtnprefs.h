#ifndef __SHBTNPREFS_H__
#define __SHBTNPREFS_H__

#include "bcwindowbase.h"
#include "bcbutton.h"
#include "bcdialog.h"
#include "bclistbox.h"
#include "bclistboxitem.h"
#include "preferences.inc"
#include "preferencesthread.inc"
#include "thread.h"
#include "shbtnprefs.inc"


class ShBtnRun : public Thread
{
public:
	int warn;
	char name[BCSTRLEN];
	char commands[BCTEXTLEN];

	ShBtnRun(const char *name, const char *cmds, int warn=0);
	void run();
};

class ShBtnPref
{
public:
	char name[BCSTRLEN];
	char commands[BCTEXTLEN];
	int warn;
	void execute();

	ShBtnPref(const char *nm, const char *cmds, int warn);
	~ShBtnPref();
};

class ShBtnEditDialog : public BC_DialogThread
{
public:
	PreferencesWindow *pwindow;

        ShBtnEditWindow *sb_window;
	BC_Window* new_gui();
	void handle_close_event(int result);

	ShBtnEditDialog(PreferencesWindow *pwindow);
	~ShBtnEditDialog();
};


class ShBtnAddButton : public BC_GenericButton {
public:
	ShBtnEditWindow *sb_window;
	int handle_event();

	ShBtnAddButton(ShBtnEditWindow *sb_window, int x, int y);
	~ShBtnAddButton();
};

class ShBtnDelButton : public BC_GenericButton {
public:
	ShBtnEditWindow *sb_window;
	int handle_event();

	ShBtnDelButton(ShBtnEditWindow *sb_window, int x, int y);
	~ShBtnDelButton();
};

class ShBtnEditButton : public BC_GenericButton {
public:
	ShBtnEditWindow *sb_window;

	int handle_event();

	ShBtnEditButton(ShBtnEditWindow *sb_window, int x, int y);
	~ShBtnEditButton();
};

class ShBtnTextDialog : public BC_DialogThread
{
public:
        ShBtnEditWindow *sb_window;
	ShBtnTextWindow *st_window;
	ShBtnPref *pref;

	BC_Window* new_gui();
	void handle_close_event(int result);
	int start_edit(ShBtnPref *pref);

	ShBtnTextDialog(ShBtnEditWindow *sb_window);
	~ShBtnTextDialog();
};

class ShBtnTextOK : public BC_OKButton
{
public:
	ShBtnTextWindow *st_window;
	int handle_event();

	ShBtnTextOK(ShBtnTextWindow *st_window, int x, int y);
	~ShBtnTextOK();
};

class ShBtnErrWarn : public BC_CheckBox
{
public:
	ShBtnErrWarn(ShBtnTextWindow *st_window, int x, int y);
	~ShBtnErrWarn();

	ShBtnTextWindow *st_window;
};

class ShBtnTextWindow : public BC_Window
{
public:
	BC_TextBox *cmd_name;
	BC_ScrollTextBox *cmd_text;
	ShBtnEditWindow *sb_window;
	ShBtnErrWarn *st_err_warn;
	int warn;

	void create_objects();

	ShBtnTextWindow(ShBtnEditWindow *sb_window, int x, int y);
	~ShBtnTextWindow();
};

class ShBtnPrefItem : public BC_ListBoxItem {
public:
	ShBtnPref *pref;

	ShBtnPrefItem(ShBtnPref *item);
	~ShBtnPrefItem();
};

class ShBtnPrefList : public BC_ListBox
{
public:
	ShBtnEditWindow *sb_window;
	int handle_event();

	ShBtnPrefList(ShBtnEditWindow *sb_window, int x, int y);
	~ShBtnPrefList();
};

class ShBtnEditWindow : public BC_Window
{
public:
	ShBtnAddButton *add_button;
	ShBtnDelButton *del_button;
	ShBtnEditButton *edit_button;
	ShBtnTextDialog *sb_dialog;
	ArrayList<BC_ListBoxItem *> shbtn_items;
	ShBtnPrefList *op_list;

	void create_objects();
	int list_update();
	int start_edit(ShBtnPref *pref);

	ShBtnEditWindow(ShBtnEditDialog *shbtn_edit, int x, int y);
	~ShBtnEditWindow();

	ShBtnEditDialog *shbtn_edit;
};

class MainShBtnItem : public BC_MenuItem
{
public:
	MainShBtnItem(MainShBtns *shbtns, ShBtnPref *pref);
	int handle_event();

	MainShBtns *shbtns;
	ShBtnPref *pref;
};

class MainShBtns : public BC_PopupMenu
{
public:
	MainShBtns(MWindow *mwindow, int x, int y);
	int load(Preferences *preferences);
	int handle_event();

	MWindow *mwindow;
};

#endif
