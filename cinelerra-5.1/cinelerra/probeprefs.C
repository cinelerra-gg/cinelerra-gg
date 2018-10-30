#include "bcwindowbase.h"
#include "bcdisplayinfo.h"
#include "bcdialog.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "probeprefs.h"
#include "preferences.h"
#include "preferencesthread.h"
#include "theme.h"

#include <sys/wait.h>

ProbePref::ProbePref(const char *nm, int armed)
{
	strncpy(name, nm, sizeof(name));
	this->armed = armed;
}

ProbePref::~ProbePref()
{
}

FileProbeDialog::FileProbeDialog(PreferencesWindow *pwindow)
 : BC_DialogThread()
{
	this->pwindow = pwindow;
}

FileProbeDialog::~FileProbeDialog()
{
	close_window();
}

BC_Window* FileProbeDialog::new_gui()
{
	BC_DisplayInfo display_info;
	int x = display_info.get_abs_cursor_x();
	int y = display_info.get_abs_cursor_y();

	pb_window = new ProbeEditWindow(this, x, y);
	pb_window->create_objects();
	return pb_window;
}

void FileProbeDialog::handle_close_event(int result)
{
	pb_window = 0;
}


ProbeEditWindow::ProbeEditWindow(FileProbeDialog *pb_dialog, int x, int y)
 : BC_Window(_(PROGRAM_NAME ": Probes"), x, y, 300, 200, 300, 200, 0, 0, 1)
{
	this->pb_dialog = pb_dialog;
	probe_list = 0;
	probe_up_button = 0;
	probe_down_button = 0;
	probe_enabled = 0;
	pb_enabled = 0;
	pb_disabled = 0;
}

ProbeEditWindow::~ProbeEditWindow()
{
	delete pb_enabled;
	delete pb_disabled;
}

void ProbeEditWindow::create_objects()
{
	lock_window("ProbeEditWindow::create_objects");
	pb_enabled = new BC_Pixmap(this,
                BC_WindowBase::get_resources()->listbox_up,
                PIXMAP_ALPHA);
	pb_disabled = new BC_Pixmap(this,
                BC_WindowBase::get_resources()->listbox_dn,
                PIXMAP_ALPHA);
	Preferences *preferences = pb_dialog->pwindow->thread->preferences;
	for( int i=0; i<preferences->file_probes.size(); ++i ) {
		probe_items.append(new ProbePrefItem(this, preferences->file_probes[i]));
	}
	int x = 10, y = 10;
	add_subwindow(probe_list = new ProbePrefList(this, x, y));
	y += probe_list->get_h();
	int x1 = x, y1 = y;
	add_subwindow(probe_up_button = new ProbeUpButton(this, x1, y1));
	x1 += probe_up_button->get_w() + 10;
	add_subwindow(probe_down_button = new ProbeDownButton(this, x1, y1));
	x1 += probe_down_button->get_w() + 10;
	add_subwindow(probe_enabled = new ProbeEnabled(this, x1, y1));
	probe_enabled->disable();

	add_subwindow(new ProbeEditOK(this));
	add_subwindow(new BC_CancelButton(this));

	list_update();
	show_window();
	unlock_window();
}

ProbeEditOK::ProbeEditOK(ProbeEditWindow *pb_window)
 : BC_OKButton(pb_window)
{
        this->pb_window = pb_window;
}

ProbeEditOK::~ProbeEditOK()
{
}

int ProbeEditOK::handle_event()
{
        Preferences *preferences = pb_window->pb_dialog->pwindow->thread->preferences;
	ArrayList<ProbePref *> &file_probes = preferences->file_probes;
	file_probes.remove_all_objects();
	ArrayList<ProbePrefItem *> &probe_items = pb_window->probe_items;
        for( int i=0; i<probe_items.size(); ++i ) {
                ProbePrefItem *item = probe_items[i];
                file_probes.append(new ProbePref(item->get_text(), item->armed));
        }
        return BC_OKButton::handle_event();
}

int ProbeEditWindow::list_update()
{
//for( int i=0; i<probe_items.size(); ++i ) printf("%s, ",probe_items[i]->get_text()); printf("\n");
	probe_list->update((ArrayList<BC_ListBoxItem*> *)&probe_items, 0, 0, 1,
		probe_list->get_xposition(), probe_list->get_yposition(),
		probe_list->get_highlighted_item(), 1, 0);
	probe_list->center_selection();
	return 1;
}

ProbeUpButton::ProbeUpButton(ProbeEditWindow *pb_window, int x, int y)
 : BC_GenericButton(x, y, _("Up"))
{
	this->pb_window = pb_window;
}

ProbeUpButton::~ProbeUpButton()
{
}

int ProbeUpButton::handle_event()
{
	ArrayList<ProbePrefItem *> &probe_items = pb_window->probe_items;
	int n = probe_items.size();
	if( n > 1 ) {
		ProbePrefItem **pitem = &probe_items[0], *item0 = *pitem;
		for( int i=1; i<n; ++i ) {
			ProbePrefItem *&item = probe_items[i];
			if( item->get_selected() ) {
				ProbePrefItem *t = item;  item = *pitem; *pitem = t;
			}
			pitem = &item;
		}
		if( item0->get_selected() ) {
			ProbePrefItem *t = *pitem;  *pitem = item0;  probe_items[0] = t;
		}
	}
	pb_window->list_update();
	return 1;
}

ProbeDownButton::ProbeDownButton(ProbeEditWindow *pb_window, int x, int y)
 : BC_GenericButton(x, y, _("Down"))
{
	this->pb_window = pb_window;
}

ProbeDownButton::~ProbeDownButton()
{
}

ProbeEnabled::ProbeEnabled(ProbeEditWindow *pb_window, int x, int y)
 : BC_CheckBox(x, y, 0, _("Enabled"))
{
	this->pb_window = pb_window;
}
ProbeEnabled::~ProbeEnabled()
{
}

int ProbeEnabled::handle_event()
{
	ProbePrefItem *item = (ProbePrefItem *)pb_window->probe_list->get_selection(0,0);
	if( item ) {
		item->set_armed(get_value());
		pb_window->list_update();
	}
	return 1;
}

int ProbeDownButton::handle_event()
{
	ArrayList<ProbePrefItem *> &probe_items = pb_window->probe_items;
	int n = probe_items.size();
	if( n > 1 ) {
		ProbePrefItem **pitem = &probe_items[n-1], *item1 = *pitem;
		for( int i=n-1; --i>=0; ) {
			ProbePrefItem *&item = probe_items[i];
			if( item->get_selected() ) {
				ProbePrefItem *t = item;  item = *pitem; *pitem = t;
			}
			pitem = &item;
		}
		if( item1->get_selected() ) {
			ProbePrefItem *t = *pitem;  *pitem = item1;  probe_items[n-1] = t;
		}
	}
	pb_window->list_update();
	return 1;
}

ProbePrefItem::ProbePrefItem(ProbeEditWindow *pb_window, ProbePref *pref)
 : BC_ListBoxItem(pref->name, pref->armed ?
		pb_window->pb_enabled : pb_window->pb_disabled)
{
	this->pb_window = pb_window;
	this->armed = pref->armed;
}

ProbePrefItem::~ProbePrefItem()
{
}

void ProbePrefItem::set_armed(int armed)
{
	this->armed = armed;
	set_icon(armed ? pb_window->pb_enabled : pb_window->pb_disabled);
}

ProbePrefList::ProbePrefList(ProbeEditWindow *pb_window, int x, int y)
 : BC_ListBox(x, y, pb_window->get_w()-x-10, pb_window->get_h()-y-80, LISTBOX_ICON_LIST,
	(ArrayList<BC_ListBoxItem *>*) &pb_window->probe_items, 0, 0)
{
	this->pb_window = pb_window;
}

ProbePrefList::~ProbePrefList()
{
}

int ProbePrefList::selection_changed()
{
	ProbePrefItem *item = (ProbePrefItem *)pb_window->probe_list->get_selection(0,0);
	if( item ) {
		pb_window->probe_enabled->set_value(item->armed);
		pb_window->probe_enabled->enable();
	}
	else {
		pb_window->probe_enabled->set_value(0);
		pb_window->probe_enabled->disable();
	}
	pb_window->list_update();
	return 1;
}

int ProbePrefList::handle_event()
{
	if( get_double_click() && get_buttonpress() == 1 ) {
		ProbePrefItem *item = (ProbePrefItem *)get_selection(0,0);
		int armed = item->armed ? 0 : 1;
		pb_window->probe_enabled->enable();
		pb_window->probe_enabled->set_value(armed);
		item->set_armed(armed);
		pb_window->list_update();
	}
	return 1;
}

