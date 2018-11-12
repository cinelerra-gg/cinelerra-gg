#include "bcsignals.h"
#include "guicast.h"
#include "pys_icon_png.h"

class TestList : public BC_ListBox
{
public:
	TestList(int x, int y, int w, int h, 
		ArrayList<BC_ListBoxItem*> *items);
	int handle_event();
	int selection_changed();
};

TestList::TestList(int x, int y, int w, int h, 
		ArrayList<BC_ListBoxItem*> *items)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, items,
	0, 0, 1, 0, 0, LISTBOX_SINGLE, ICON_LEFT, 0)
{
}

int TestList::handle_event()
{
	printf("handle_event\n");
	return 1;
}

int TestList::selection_changed()
{
	BC_ListBoxItem *item = get_selection(0, 0);
	printf("selection_changed %s\n", !item ? "<nul>" : item->get_text());
	return 1;
}

class TestWindow : public BC_Window
{
public:
	TestWindow() : BC_Window("Test9", 0, 0, 320, 240) {};
	void create_objects();
	int keypress_event();
	BC_ListBox *list;
	ArrayList<BC_ListBoxItem*> items;
};

void TestWindow::create_objects()
{
	lock_window("AWindowRemovePluginGUI::create_objects");
	set_color(BLACK);
	set_font(LARGEFONT);
	int x = 10, y = 20;
	draw_text(x, y, "Hello world");
	y += 25;
	BC_Button *ok_button = new BC_OKButton(this);
	add_subwindow(ok_button);
	BC_Button *cancel_button = new BC_CancelButton(this);
	add_subwindow(cancel_button);
	BC_ListBoxItem *thing;
	ArrayList<BC_ListBoxItem*> *sublist;
	items.append(thing = new BC_ListBoxItem("thing 1"));
	VFrame *pys_icon = new VFramePng(pys_icon_png);
	thing->set_icon_vframe(pys_icon);
	int pw = pys_icon->get_w(), ph = pys_icon->get_h();
	BC_Pixmap *pys_picon = new BC_Pixmap(this, pw, ph);
	pys_picon->draw_vframe(pys_icon, 0, 0, pw, pw, 0, 0);
	thing->set_icon(pys_picon);
	sublist = thing->new_sublist(1);
	BC_ListBoxItem *fish, *cat, *hat;
	sublist->append(fish = new BC_ListBoxItem("fish"));
	ArrayList<BC_ListBoxItem*> *fish_list = fish->new_sublist(1);
	fish_list->append(new BC_ListBoxItem("green"));
	fish_list->append(new BC_ListBoxItem("eggs"));
	fish_list->append(new BC_ListBoxItem("ham"));
	sublist->append(cat = new BC_ListBoxItem("cat"));
	ArrayList<BC_ListBoxItem*> *cat_list = cat->new_sublist(1);
	cat_list->append(new BC_ListBoxItem("videos"));
	sublist->append(hat = new BC_ListBoxItem("hat"));
	ArrayList<BC_ListBoxItem*> *hat_list = hat->new_sublist(1);
	hat_list->append(new BC_ListBoxItem("bonnet"));
	hat_list->append(new BC_ListBoxItem("cap"));
	hat_list->append(new BC_ListBoxItem("sombrero"));
	items.append(thing = new BC_ListBoxItem("thing 2"));
	int lw = get_w()-x-10, lh = ok_button->get_y() - y - 5;
	add_subwindow(list = new TestList(x, y, lw, lh, &items));
	show_window();
	unlock_window();
}

int TestWindow::keypress_event()
{
	switch( get_keypress() ) {
	case 'v':
		switch( list->get_format() ) {
		case LISTBOX_TEXT:
			list->update_format(LISTBOX_ICONS, 1);
			break;
		case LISTBOX_ICONS:
			list->update_format(LISTBOX_ICONS_PACKED, 1);
			break;
		case LISTBOX_ICONS_PACKED:
			list->update_format(LISTBOX_ICON_LIST, 1);
			break;
		case LISTBOX_ICON_LIST:
			list->update_format(LISTBOX_TEXT, 1);
			break;
		}
		break;
	}
	return 1;
}

int main()
{
	new BC_Signals;
	TestWindow window;
	window.create_objects();
	window.run_window();
}

