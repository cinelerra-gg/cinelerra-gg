#include "bcwindowbase.h"
#include "bcdisplayinfo.h"
#include "bcdialog.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "shbtnprefs.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "theme.h"

#include <sys/wait.h>

ShBtnRun::ShBtnRun(const char *nm, const char *cmds, int warn)
 : Thread(0, 0, 1)
{
	strncpy(name, nm, sizeof(name)-1);
	name[sizeof(name)-1] = 0;
	strncpy(commands, cmds, sizeof(commands)-1);
	commands[sizeof(commands)-1] = 0;
	this->warn = warn;
	start();
}

void ShBtnRun::run()
{
	pid_t pid = vfork();
	if( pid < 0 ) {
		perror("fork");
		return;
	}
	if( pid > 0 ) {
		int stat;  waitpid(pid, &stat, 0);
		if( warn && stat ) {
			char msg[BCTEXTLEN];
			sprintf(msg, "%s: error exit status %d", name, stat);
			MainError::show_error(msg);
		}
		return;
	}
	char *const argv[4] = { (char*) "/bin/bash", (char*) "-c", commands, 0 };
	execvp(argv[0], &argv[0]);
}

ShBtnPref::ShBtnPref(const char *nm, const char *cmds, int warn)
{
	strncpy(name, nm, sizeof(name));
	strncpy(commands, cmds, sizeof(commands));
	this->warn = warn;
}

ShBtnPref::~ShBtnPref()
{
}

void ShBtnPref::execute()
{
	new ShBtnRun(name, commands, warn);
}

ShBtnEditDialog::ShBtnEditDialog(PreferencesWindow *pwindow)
 : BC_DialogThread()
{
	this->pwindow = pwindow;
}

ShBtnEditDialog::~ShBtnEditDialog()
{
	close_window();
}

BC_Window* ShBtnEditDialog::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();

	sb_window = new ShBtnEditWindow(this, x, y);
	sb_window->create_objects();
	return sb_window;
}

void ShBtnEditDialog::handle_close_event(int result)
{
	sb_window = 0;
}


ShBtnEditWindow::ShBtnEditWindow(ShBtnEditDialog *shbtn_edit, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Shell"), x, y, 300, 200, 300, 200, 0, 0, 1)
{
	this->shbtn_edit = shbtn_edit;
	sb_dialog = 0;
}

ShBtnEditWindow::~ShBtnEditWindow()
{
	delete sb_dialog;
}

int ShBtnEditWindow::list_update()
{
	shbtn_items.remove_all_objects();
	Preferences *preferences = shbtn_edit->pwindow->thread->preferences;
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i ) {
		shbtn_items.append(new ShBtnPrefItem(preferences->shbtn_prefs[i]));
	}
	return op_list->update(&shbtn_items, 0, 0, 1);
}

ShBtnAddButton::ShBtnAddButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->sb_window = sb_window;
}

ShBtnAddButton::~ShBtnAddButton()
{
}

int ShBtnAddButton::handle_event()
{

	Preferences *preferences = sb_window->shbtn_edit->pwindow->thread->preferences;
	ShBtnPref *pref = new ShBtnPref(_("new"), "", 0);
	preferences->shbtn_prefs.append(pref);
	sb_window->list_update();
	return sb_window->start_edit(pref);
}

ShBtnDelButton::ShBtnDelButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Del"))
{
	this->sb_window = sb_window;
}

ShBtnDelButton::~ShBtnDelButton()
{
}

int ShBtnDelButton::handle_event()
{
	ShBtnPrefItem *sp = (ShBtnPrefItem *)sb_window->op_list->get_selection(0,0);
	if( !sp ) return 0;
	Preferences *preferences = sb_window->shbtn_edit->pwindow->thread->preferences;
	preferences->shbtn_prefs.remove(sp->pref);
	sb_window->list_update();
	return 1;
}

ShBtnEditButton::ShBtnEditButton(ShBtnEditWindow *sb_window, int x, int y)
 : BC_GenericButton(x, y, _("Edit"))
{
	this->sb_window = sb_window;
}

ShBtnEditButton::~ShBtnEditButton()
{
}

int ShBtnEditButton::handle_event()
{
	ShBtnPrefItem *sp = (ShBtnPrefItem *)sb_window->op_list->get_selection(0,0);
	if( !sp ) return 0;
	return sb_window->start_edit(sp->pref);
}

ShBtnTextDialog::ShBtnTextDialog(ShBtnEditWindow *sb_window)
 : BC_DialogThread()
{
	this->sb_window = sb_window;
	this->pref = 0;
}

ShBtnTextDialog::~ShBtnTextDialog()
{
	close_window();
}

ShBtnTextWindow::ShBtnTextWindow(ShBtnEditWindow *sb_window, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Commands"), x, y, 640, 160, 640, 150, 0, 0, 1)
{
        this->sb_window = sb_window;
	warn = sb_window->sb_dialog->pref->warn;
}

ShBtnTextWindow::~ShBtnTextWindow()
{
}

ShBtnErrWarn::ShBtnErrWarn(ShBtnTextWindow *st_window, int x, int y)
 : BC_CheckBox(x, y, &st_window->warn, _("Warn on err exit"))
{
        this->st_window = st_window;
}

ShBtnErrWarn::~ShBtnErrWarn()
{
}

void ShBtnTextWindow::create_objects()
{
	lock_window("ShBtnTextWindow::create_objects");
        int x = 10, y = 10;
	int x1 = 160;
	BC_Title *title = new BC_Title(x, y, _("Label:"));
	add_subwindow(title);
	title = new BC_Title(x1, y, _("Commands:"));
	add_subwindow(title);
	y += title->get_h() + 8;
	ShBtnPref *pref = sb_window->sb_dialog->pref;
        cmd_name = new BC_TextBox(x, y, 140, 1, pref->name);
        add_subwindow(cmd_name);
        cmd_text = new BC_ScrollTextBox(this, x1, y, get_w()-x1-20, 4, pref->commands);
	cmd_text->create_objects();
	y += cmd_text->get_h() + 16;
        add_subwindow(st_err_warn = new ShBtnErrWarn(this, x1, y));
        y = get_h() - ShBtnTextOK::calculate_h() - 10;
        add_subwindow(new ShBtnTextOK(this, x, y));
        show_window();
	unlock_window();
}

ShBtnTextOK::ShBtnTextOK(ShBtnTextWindow *st_window, int x, int y)
 : BC_OKButton(x, y)
{
	this->st_window = st_window;
}

ShBtnTextOK::~ShBtnTextOK()
{
}

int ShBtnTextOK::handle_event()
{
	ShBtnPref *pref = st_window->sb_window->sb_dialog->pref;
	strcpy(pref->name, st_window->cmd_name->get_text());
	strcpy(pref->commands, st_window->cmd_text->get_text());
	pref->warn = st_window->warn;
	return BC_OKButton::handle_event();
}


BC_Window *ShBtnTextDialog::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();

	st_window = new ShBtnTextWindow(sb_window, x, y);
	st_window->create_objects();
	return st_window;
}

void ShBtnTextDialog::handle_close_event(int result)
{
	if( !result ) {
		sb_window->lock_window("ShBtnTextDialog::handle_close_event");
		sb_window->list_update();
		sb_window->unlock_window();
	}
	st_window = 0;
}

int ShBtnTextDialog::start_edit(ShBtnPref *pref)
{
	this->pref = pref;
	start();
	return 1;
}

void ShBtnEditWindow::create_objects()
{
	lock_window("ShBtnEditWindow::create_objects");
	Preferences *preferences = shbtn_edit->pwindow->thread->preferences;
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i ) {
		shbtn_items.append(new ShBtnPrefItem(preferences->shbtn_prefs[i]));
	}
	int x = 10, y = 10;
	add_subwindow(op_list = new ShBtnPrefList(this, x, y));
	x = 190;
	add_subwindow(add_button = new ShBtnAddButton(this, x, y));
	y += add_button->get_h() + 8;
	add_subwindow(del_button = new ShBtnDelButton(this, x, y));
	y += del_button->get_h() + 8;
	add_subwindow(edit_button = new ShBtnEditButton(this, x, y));
	add_subwindow(new BC_OKButton(this));
	show_window();
	unlock_window();
}

int ShBtnEditWindow::start_edit(ShBtnPref *pref)
{
	if( !sb_dialog )
		sb_dialog = new ShBtnTextDialog(this);
	return sb_dialog->start_edit(pref);
}


ShBtnPrefItem::ShBtnPrefItem(ShBtnPref *pref)
 : BC_ListBoxItem(pref->name)
{
	this->pref = pref;
}

ShBtnPrefItem::~ShBtnPrefItem()
{
}

ShBtnPrefList::ShBtnPrefList(ShBtnEditWindow *sb_window, int x, int y)
 : BC_ListBox(x, y, 140, 100, LISTBOX_TEXT, &sb_window->shbtn_items, 0, 0)
{
	this->sb_window = sb_window;
}

ShBtnPrefList::~ShBtnPrefList()
{
}

int ShBtnPrefList::handle_event()
{
	return 1;
}

MainShBtnItem::MainShBtnItem(MainShBtns *shbtns, ShBtnPref *pref)
 : BC_MenuItem(pref->name)
{
	this->shbtns = shbtns;
	this->pref = pref;
}

int MainShBtnItem::handle_event()
{
	pref->execute();
	return 1;
}

MainShBtns::MainShBtns(MWindow *mwindow, int x, int y)
 : BC_PopupMenu(x, y, 0, "", -1, mwindow->theme->shbtn_data)
{
	this->mwindow = mwindow;
	set_tooltip(_("shell cmds"));
}

int MainShBtns::load(Preferences *preferences)
{
	while( total_items() ) del_item(0);
	for( int i=0; i<preferences->shbtn_prefs.size(); ++i )
		add_item(new MainShBtnItem(this, preferences->shbtn_prefs[i]));
	return 0;
}

int MainShBtns::handle_event()
{
	return 1;
}

