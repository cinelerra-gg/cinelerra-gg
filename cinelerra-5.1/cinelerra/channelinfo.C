#ifdef HAVE_DVB

#include "asset.h"
#include "batch.h"
#include "bctimer.h"
#include "channelinfo.h"
#include "channel.h"
#include "channeldb.h"
#include "condition.h"
#include "cstrdup.h"
#include "devicedvbinput.h"
#include "edl.h"
#include "edlsession.h"
#include "keys.h"
#include "mainerror.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mutex.h"
#include "bcmenuitem.h"
#include "record.h"
#include "recordconfig.h"
#include "theme.h"
#include "videodevice.h"
#include "videoconfig.h"
#include "videodevice.h"
#include "libzmpeg3.h"

#include <ctype.h>

static inline int min(int a,int b) { return a<b ? a : b; }

static inline int max(int a,int b) { return a>b ? a : b; }


ChanSearch::ChanSearch(ChannelInfo *iwindow)
 : Thread(1, 0, 0)
{
	this->iwindow = iwindow;
	window_lock = new Mutex("ChanSearch::window_lock");
	gui = 0;
}

ChanSearch::~ChanSearch()
{
	stop();
	delete gui;
	delete window_lock;
}

void ChanSearch::start()
{
	window_lock->lock("ChanSearch::start1");
	if( Thread::running() ) {
		gui->lock_window("ChanSearch::start2");
		gui->raise_window();
		gui->unlock_window();
	}
	else {
		delete gui;
		gui = new ChanSearchGUI(this);
		Thread::start();
	}
	window_lock->unlock();
}

void ChanSearch::stop()
{
	if( Thread::running() ) {
		window_lock->lock("ChanSearch::stop");
		if( gui ) gui->set_done(1);
		window_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
}

void ChanSearch::run()
{
	gui->lock_window("ChanSearch::run");
	gui->create_objects();
	gui->unlock_window();
	gui->run_window();
	window_lock->lock("ChanSearch::stop");
	delete gui;  gui = 0;
	window_lock->unlock();
}


ChanSearchTitleText::ChanSearchTitleText(ChanSearchGUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->title_text_enable, _("titles"))
{
	this->gui = gui;
}

ChanSearchTitleText::~ChanSearchTitleText()
{
}

int ChanSearchTitleText::handle_event()
{
	gui->title_text_enable = get_value();
	if( !gui->title_text_enable ) gui->info_text->update(1);
	return 1;
}


ChanSearchInfoText::ChanSearchInfoText(ChanSearchGUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->info_text_enable, _("info"))
{
	this->gui = gui;
}

ChanSearchInfoText::~ChanSearchInfoText()
{
}

int ChanSearchInfoText::handle_event()
{
	gui->info_text_enable = get_value();
	if( !gui->info_text_enable ) gui->title_text->update(1);
	return 1;
}


ChanSearchMatchCase::ChanSearchMatchCase(ChanSearchGUI *gui, int x, int y)
 : BC_CheckBox(x, y, &gui->match_case_enable, _("match case"))
{
	this->gui = gui;
}

ChanSearchMatchCase::~ChanSearchMatchCase()
{
}

int ChanSearchMatchCase::handle_event()
{
	gui->match_case_enable = get_value();
	return 1;
}


ChanSearchText::ChanSearchText(ChanSearchGUI *gui, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, "")
{
	this->gui = gui;
}

ChanSearchText::~ChanSearchText()
{
}


int ChanSearchText::handle_event()
{
	return 1;
}

int ChanSearchText::keypress_event()
{
	switch(get_keypress())
	{
	case RETURN:
		gui->search();
		return 1;
	}

	return BC_TextBox::keypress_event();
}


ChanSearchStart::ChanSearchStart(ChanSearchGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Search"))
{
	this->gui = gui;
}

ChanSearchStart::~ChanSearchStart()
{
}

int ChanSearchStart::handle_event()
{
	gui->search();
	return 1;
}


ChanSearchCancel::ChanSearchCancel(ChanSearchGUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
	this->gui = gui;
}

ChanSearchCancel::~ChanSearchCancel()
{
}

int ChanSearchCancel::handle_event()
{
	gui->set_done(1);
	return 1;
}


ChanSearchList::ChanSearchList(ChanSearchGUI *gui, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, &gui->search_items[0],
		&gui->search_column_titles[0], &gui->search_column_widths[0],
		lengthof(gui->search_items))
{
	this->gui = gui;
	set_sort_column(gui->sort_column);
	set_sort_order(gui->sort_order);
	set_allow_drag_column(1);
}

ChanSearchList::~ChanSearchList()
{
}

int ChanSearchList::handle_event()
{
	if( get_double_click() ) {
		ChannelPanel *panel = gui->panel;
		panel->lock_window("ChanSearchList::handle_event");
		ChannelEvent *item = gui->highlighted_event;
		if( item ) {
			item->text_color(-1);
			item->draw_face();
		}
		int i = get_highlighted_item();
		item = gui->search_results.get(i);
		int x = item->x0-panel->x0 - panel->frame_w/2 + item->get_w()/2;
		if( x < 0 ) x = 0;
		int w = panel->x1 - panel->x0;
		if( x > w ) x = w;
		panel->time_line_scroll->update_value(x);
		panel->set_x_scroll(x);
		item->text_color(YELLOW);
		item->draw_face();
		gui->highlighted_event = item;
		panel->unlock_window();
	}
	return 1;
}

int ChanSearchList::sort_order_event()
{
	gui->sort_events(get_sort_column(), get_sort_order());
        return 1;
}

int ChanSearchList::move_column_event()
{
	gui->move_column(get_from_column(), get_to_column());
	return 1;
}

void ChanSearchGUI::create_objects()
{
	lock_window("ChanSearchGUI::create_objects");
	int pady = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1) + 5;
	int padx = BC_Title::calculate_w(this, (char*)"X", MEDIUMFONT);
	int x = padx/2, y = pady/4;
	BC_Title *title = new BC_Title(text_x, text_y, _("Text:"), MEDIUMFONT, YELLOW);
	add_subwindow(title);  x += title->get_w();
	text_x = x;  text_y = y;
	search_text = new ChanSearchText(this, x, y, get_w()-x-10);
	add_subwindow(search_text);
	x = padx;  y += pady + 5;
	title_text = new ChanSearchTitleText(this, x, y);
	add_subwindow(title_text);  x += title_text->get_w() + padx;
	info_text = new ChanSearchInfoText(this, x, y);
	add_subwindow(info_text);  x += title_text->get_w() + padx;
	match_case = new ChanSearchMatchCase(this, x, y);
	add_subwindow(match_case); x += match_case->get_w() + padx;
	results_x = x + 20;  results_y = y + 5;
	results = new BC_Title(results_x, results_y, " ", MEDIUMFONT, YELLOW);
	add_subwindow(results);
	x = padx;  y += pady + 5;
	search_x = 10;
	search_y = get_h() - BC_GenericButton::calculate_h() - 10;
	search_start = new ChanSearchStart(this, search_x, search_y);
	add_subwindow(search_start);
	cancel_w = ChanSearchCancel::calculate_w();
	cancel_h = ChanSearchCancel::calculate_h();
	cancel_x = get_w() - cancel_w - 10;
	cancel_y = get_h() - cancel_h - 10;
	cancel = new ChanSearchCancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	list_x = x;  list_y = y;
	int list_w = get_w()-10 - list_x;
	int list_h = min(search_y, cancel_y)-10 - list_y;
	search_list = new ChanSearchList(this, list_x, list_y, list_w, list_h);
	add_subwindow(search_list);
	search_list->show_window();
	int clktip_x = list_x;
	int clktip_y = list_y + list_h + 5;
	click_tip = new BC_Title(clktip_x, clktip_y, _("dbl clk row to find title"));
	add_subwindow(click_tip);

	set_icon(iwindow->mwindow->theme->get_image("record_icon"));
	search_text->activate();
	unlock_window();
}

ChanSearchGUI::ChanSearchGUI(ChanSearch *cswindow)
 : BC_Window(_(PROGRAM_NAME ": ChanSearch"),
	cswindow->iwindow->gui->get_abs_cursor_x(1) - 500 / 2,
	cswindow->iwindow->gui->get_abs_cursor_y(1) - 300 / 2,
	500, 300, 400, 300)
{
	this->cswindow = cswindow;
	this->iwindow = cswindow->iwindow;
	this->panel = cswindow->iwindow->gui->panel;

	search_text = 0;
	title_text = 0;
	info_text = 0;
	match_case = 0;
	search_start = 0;
	search_list = 0;
	cancel = 0;
	results = 0;

	search_x = search_y = text_x = text_y = 0;
	cancel_x = cancel_y = cancel_w = cancel_h = 0;
	list_x = list_y = list_w = list_h = 0;
	results_x = results_y = 0;
	sort_column = sort_order = 0;

	title_text_enable = 1;
	info_text_enable = 0;
	match_case_enable = 0;
	highlighted_event = 0;

	search_columns[0] = 0;
	search_columns[1] = 1;
	search_columns[2] = 2;
	search_column_titles[0] = _("Source");
	search_column_titles[1] = C_("Title");
	search_column_titles[2] = _("Start time");
	search_column_widths[0] = 120;
	search_column_widths[2] = 120;
	search_column_widths[1] = get_w()-search_column_widths[0]-search_column_widths[2]-32;
}

ChanSearchGUI::~ChanSearchGUI()
{

	ChannelEvent *item = highlighted_event;
	if( item ) {
		panel->lock_window("ChanSearchGUI::~ChanSearchGUI");
		item->text_color(-1);
		item->draw_face();
		panel->unlock_window();
	}
	for( int i=0; i<lengthof(search_items); ++i )
		search_items[i].remove_all_objects();
}

int ChanSearchGUI::resize_event(int w, int h)
{
	search_text->reposition_window(text_x, text_y, w-text_x-10);
	int cancel_x = w - BC_CancelButton::calculate_w() - 10;
	int cancel_y = h - BC_CancelButton::calculate_h() - 10;
	cancel->reposition_window(cancel_x, cancel_y);
	search_x = 10;
	search_y = h - BC_GenericButton::calculate_h() - 10;
	search_start->reposition_window(search_x, search_y);
	int list_w = w-10 - list_x;
	int list_h = min(search_y, cancel_y)-10 - list_y;
	search_column_widths[1] = w-search_column_widths[0]-search_column_widths[2]-32;
	search_list->reposition_window(list_x, list_y, list_w, list_h);
	int clktip_x = list_x;
	int clktip_y = list_y + list_h + 5;
	click_tip->reposition_window(clktip_x, clktip_y);
	update();
	return 1;
}

int ChanSearchGUI::close_event()
{
	set_done(1);
	return 1;
}

int ChanSearchGUI::search(const char *sp)
{
	char const *tp = search_text->get_text();
	int n = strlen(tp);
	for( const char *cp=sp; *cp!=0; ++cp ) {
		if( match_case_enable ? !strncmp(tp, cp, n) :
			!strncasecmp(tp, cp, n) ) return 1;
	}
	return 0;
}

void ChanSearchGUI::search()
{
	search_results.remove_all();

	int n = panel->channel_event_items.size();
	for( int i=0; i<n; ++i ) {
		ChannelEvent *item = panel->channel_event_items.get(i);
		if( (title_text_enable && search(item->get_text())) ||
		    (info_text_enable  && search(item->get_tooltip()) ) )
			search_results.append(item);
	}

	update();
}

void ChanSearchGUI::update()
{

	for( int i=0; i<lengthof(search_items); ++i )
		search_items[i].remove_all_objects();

	char text[BCTEXTLEN];
	int n = search_results.size();
	for( int i=0; i<n; ++i ) {
		ChannelEvent *item = search_results.get(i);
		for( int k=0; k<lengthof(search_columns); ++k ) {
			const char *cp = 0;
			switch( search_columns[k] ) {
			case 0:  cp = item->channel->title;  break;
			case 1:  cp = item->get_text();      break;
			case 2: {
				struct tm stm;  localtime_r(&item->start_time,&stm);
				double seconds = stm.tm_hour*3600 + stm.tm_min*60 + stm.tm_sec;
				Units::totext(text, seconds, TIME_HMS3);
				cp = text;                   break; }
			}
			if( cp ) search_items[k].append(new BC_ListBoxItem(cp, LTYELLOW));
		}
	}

	search_list->update(search_items, &search_column_titles[0],
			&search_column_widths[0], lengthof(search_items));
	sprintf(text, _("%d found"), n);
	results->update(text);
}


#define CmprFn(nm,key) int ChanSearchGUI:: \
cmpr_##nm(const void *a, const void *b) { \
  ChannelEvent *&ap = *(ChannelEvent **)a; \
  ChannelEvent *&bp = *(ChannelEvent **)b; \
  int n = key; if( !n ) n = ap->no-bp->no; \
  return n; \
}

CmprFn(Text_dn, strcasecmp(ap->get_text(), bp->get_text()))
CmprFn(text_dn, strcmp(ap->get_text(), bp->get_text()))
CmprFn(Text_up, strcasecmp(bp->get_text(), ap->get_text()))
CmprFn(text_up, strcmp(bp->get_text(), ap->get_text()))
CmprFn(time_dn, ap->start_time - bp->start_time)
CmprFn(time_up, bp->start_time - ap->start_time)
CmprFn(Title_dn, strcasecmp(ap->channel->title, bp->channel->title))
CmprFn(title_dn, strcmp(ap->channel->title, bp->channel->title))
CmprFn(Title_up, strcasecmp(bp->channel->title, ap->channel->title))
CmprFn(title_up, strcmp(bp->channel->title, ap->channel->title))


void ChanSearchGUI::sort_events(int column, int order)
{
        sort_column = column;  sort_order = order;
	int n = search_results.size();
	if( !n ) return;
	ChannelEvent **events = &search_results.values[0];
	for( int i=0; i<n; ++i ) events[i]->no = i;

	int(*cmpr)(const void *, const void *) = 0;
	switch( search_columns[sort_column] ) {
	case 0: cmpr = match_case_enable ?
			(sort_order ? cmpr_Title_up : cmpr_Title_dn) :
			(sort_order ? cmpr_title_up : cmpr_title_dn) ;
		break;
	case 1: cmpr = match_case_enable ?
			(sort_order ? cmpr_Text_up : cmpr_Text_dn) :
			(sort_order ? cmpr_text_up : cmpr_text_dn) ;
		break;
	case 2: cmpr = sort_order ? cmpr_time_up : cmpr_time_dn;
		break;
	}

	if( cmpr ) {
		ChannelEvent **events = &search_results.values[0];
		qsort(events, n, sizeof(*events), cmpr);
	}
        update();
}

void ChanSearchGUI::move_column(int src, int dst)
{
	if( src == dst ) return;
	int src_column = search_columns[src];
	const char *src_column_title = search_column_titles[src];
	int src_column_width = search_column_widths[src];
	if( src < dst ) {
		for( int i=src; i<dst; ++i ) {
			search_columns[i] = search_columns[i+1];
			search_column_titles[i] = search_column_titles[i+1];
			search_column_widths[i] = search_column_widths[i+1];
		}
	}
	else {
		for( int i=src; i>dst; --i  ) {
			search_columns[i] = search_columns[i-1];
			search_column_titles[i] = search_column_titles[i-1];
			search_column_widths[i] = search_column_widths[i-1];
		}
	}
	search_columns[dst] = src_column;
	search_column_titles[dst] = src_column_title;
	search_column_widths[dst] = src_column_width;
	update();
}


ChannelProgress::ChannelProgress(ChannelInfoGUI *gui,
		int x, int y, int w, int h, int len)
 : Thread(1, 0, 0),
   BC_SubWindow(x, y, w, h)
{
	this->gui = gui;
	eta = 0;
	bar = 0;
	done = 0;
	length = len;
	value = 0.;

	eta_timer = new Timer;
}

ChannelProgress::~ChannelProgress()
{
	stop();
	delete eta_timer;
}

void ChannelProgress::create_objects()
{
	int w = BC_Title::calculate_w(gui, (char*)"XXXXX", MEDIUMFONT);
	eta = new BC_Title(0, 0, "+0:00", MEDIUMFONT, -1, 0, w);
	add_subwindow(eta);
	int x = eta->get_w();
	bar = new BC_ProgressBar(x, 0, get_w()-x, length);
	add_subwindow(bar);
	start();
}

void ChannelProgress::start()
{
	done = 0;
	eta_timer->update();
	Thread::start();
}

void ChannelProgress::stop()
{
	if( Thread::running() ) {
		done = 1;
		Thread::cancel();
	}
	Thread::join();
}

void ChannelProgress::run()
{
	gui->init_wait();
	while( !done ) {
		if( update() ) break;
		enable_cancel();
		Timer::delay(500);
		disable_cancel();
	}
}

int ChannelProgress::update()
{
	double elapsed = (double)eta_timer->get_scaled_difference(1000) / 1000.;
	double estimate = value < 1 ? 0 : (length/value-1)*elapsed;
	char text[BCTEXTLEN], *cp = &text[0];
 	Units::totext(cp, estimate, TIME_MS1);
	gui->lock_window("ChannelProgress::update");
	eta->update(cp);
	bar->update(value);
	gui->unlock_window();
	return value < length ? 0 : 1;
}



TimeLineItem::TimeLineItem(ChannelPanel *panel, int x, int y, char *text)
 : BC_Title(x-panel->x_scroll, 0, text, MEDIUMFONT,
		x==panel->x_now ? GREEN : YELLOW)
{
	this->panel = panel;
	x0 = x;  y0 = y;
}

TimeLineItem::~TimeLineItem()
{
}


TimeLine::TimeLine(ChannelPanel *panel)
 : BC_SubWindow(panel->frame_x, 0, panel->frame_w, panel->path_h)
{
	this->panel = panel;
}

TimeLine::~TimeLine()
{
}

int TimeLine::resize_event(int w, int h)
{
	clear_box(0,0, w,h);
	return 1;
}


ChannelDataItem::ChannelDataItem(ChannelPanel *panel, int x, int y, int w,
	int color, const char *text)
 : BC_Title(x, y-panel->y_scroll, text, MEDIUMFONT, color)
{
	this->panel = panel;
	x0 = x;  y0 = y;
	tip_info = 0;
	set_force_tooltip(1);
}

ChannelDataItem::~ChannelDataItem()
{
	delete [] tip_info;
}

int ChannelDataItem::repeat_event(int64_t duration)
{
	if( tip_info && cursor_above() &&
		duration == get_resources()->tooltip_delay ) {
		show_tooltip();
		return 1;
	}
	return 0;
}

void ChannelDataItem::set_tooltip(const char *tip)
{
	BC_Title::set_tooltip(tip_info = tip ? cstrdup(tip) : 0);
}


ChannelData::ChannelData(ChannelPanel *panel, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, LTBLACK)
{
	this->panel = panel;
}

ChannelData::~ChannelData()
{
}

int ChannelData::resize_event(int w, int h)
{
	clear_box(0,0, w,h);
	return 1;
}


ChannelScroll::ChannelScroll(ChannelPanel *panel, int x, int y, int h)
 : BC_ScrollBar(x, y, SCROLL_VERT, h, 0, 0, 0)
{
	this->panel = panel;
}

ChannelScroll::~ChannelScroll()
{
}

int ChannelScroll::handle_event()
{
	panel->set_y_scroll(get_value());
	return 1;
}


TimeLineScroll::TimeLineScroll(ChannelPanel *panel, int x, int y, int w)
 : BC_ScrollBar(x, y, SCROLL_HORIZ, w, 0, 0, 0)
{
	this->panel = panel;
}

TimeLineScroll::~TimeLineScroll()
{
}

int TimeLineScroll::handle_event()
{
	panel->set_x_scroll(get_value());
	return 1;
}


ChannelEventLine::ChannelEventLine(ChannelPanel *panel,
	int x, int y, int w, int h, int color)
 : BC_SubWindow(0, y, w, h, color)
{
	this->panel = panel;
	x0 = x;  y0 = y;
}

ChannelEventLine::~ChannelEventLine()
{
}

void ChannelEventLine::resize(int w, int h)
{
	resize_window(w, h);
}


ChannelEvent::ChannelEvent(ChannelEventLine *channel_line, Channel *channel,
	time_t start_time, time_t end_time, int x, int y, int w, const char *text)
 : BC_GenericButton( x-channel_line->panel->x_scroll,
		y-channel_line->panel->y_scroll, w, text)
{
	this->channel_line = channel_line;
	this->channel = channel;
	this->start_time = start_time;
	this->end_time = end_time;
	x0 = x;  y0 = y;  no = 0;
	tip_info = 0;
	set_force_tooltip(1);
}

ChannelEvent::~ChannelEvent()
{
	delete [] tip_info;
}

int ChannelEvent::handle_event()
{
	ChannelInfoGUI *gui = channel_line->panel->gui;
	Batch *batch = gui->batch_bay->get_editing_batch();
	char *path = batch->asset->path;
	int len = sizeof(batch->asset->path)-1;
	const char *text = get_text();
	const char *dir = gui->channel_dir->get_directory();
	int i = 0;
	while( i<len && *dir ) path[i++] = *dir++;
	if( i > 0 && dir[i-1] != '/' ) path[i++] = '/';
	while( i<len && *text ) {
		int ch = *text++;
		if( !isalnum(ch) ) ch = '_';
		path[i++] = ch;
	}
	if( i < len ) path[i++] = '.';
	if( i < len ) path[i++] = 't';
	if( i < len ) path[i++] = 's';
	path[i] = 0;
	int early = (int)gui->early_time->get_time();
	int late = (int)gui->late_time->get_time();
	time_t st = start_time + early;
	struct tm stm;  localtime_r(&st,&stm);
	batch->record_mode = RECORD_TIMED;
	batch->enabled = 1;
	batch->start_day = stm.tm_wday;
	batch->start_time = stm.tm_hour*3600 + stm.tm_min*60 + stm.tm_sec;
	batch->duration = end_time - start_time - early + late;
	batch->channel = channel;
	gui->batch_bay->update_batches();
	gui->update_channel_tools();
	return 1;
}

void ChannelEvent::set_tooltip(const char *tip)
{
	BC_GenericButton::set_tooltip(tip_info = tip ? cstrdup(tip) : 0);
}


ChannelFrame::ChannelFrame(ChannelPanel *panel)
 : BC_SubWindow(panel->frame_x, panel->frame_y,
		panel->frame_w, panel->frame_h, BLACK)
{
	this->panel = panel;
}

ChannelFrame::~ChannelFrame()
{
}


void ChannelPanel::create_objects()
{
	iwd = BC_ScrollBar::get_span(SCROLL_VERT);
	iht = BC_ScrollBar::get_span(SCROLL_HORIZ);

	frame_x = path_w;  frame_y = path_h;
	frame_w = iwindow_w-frame_x - iwd;
	frame_h = iwindow_h-frame_y - iht;

	time_line = new TimeLine(this);
	add_subwindow(time_line);
	time_line_scroll = new TimeLineScroll(this, frame_x, iwindow_h-iht, frame_w);
	add_subwindow(time_line_scroll);
	channel_data = new ChannelData(this, 0, frame_y, path_w, frame_h);
	add_subwindow(channel_data);
	channel_frame = new ChannelFrame(this);
	add_subwindow(channel_frame);
	channel_scroll = new ChannelScroll(this, iwindow_w-iwd, frame_y, frame_h);
	add_subwindow(channel_scroll);
}

ChannelPanel::ChannelPanel(ChannelInfoGUI *gui,
	int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->gui = gui;
	gettimeofday(&tv, &tz);
	st_org = ((tv.tv_sec+1800-1) / 1800) * 1800 - 1800;
	x_now = (tv.tv_sec-st_org)/1800 * hhr_w;
	path_w = gui->path_w;  path_h = gui->path_h;
	x0 = y0 = x1 = y1 = t0 = t1 = 0;
	x_scroll = y_scroll = 0;
	x_moved = y_moved = 0;
	iwindow_w = w;  iwindow_h = h;
	hhr_w = BC_GenericButton::calculate_w(gui, (char*)"XXXXXXXX") + 5;
}

ChannelPanel::~ChannelPanel()
{
}

ChannelEventLine *ChannelPanel::NewChannelLine(int y, int h, int color)
{
	ChannelEventLine *channel_line =
		 new ChannelEventLine(this, 0, y, frame_w, h, color);
	channel_frame->add_subwindow(channel_line);
	channel_line_items.append(channel_line);
	return channel_line;
}

void ChannelPanel::resize(int w, int h)
{
	frame_w = (iwindow_w=w) - frame_x - iwd;
	frame_h = (iwindow_h=h) - frame_y - iht;
	time_line->resize_window(frame_w, path_h);
	time_line_scroll->reposition_window(frame_x, frame_y+frame_h, frame_w);
	time_line_scroll->update_length(x1-x0, x_scroll-x0, frame_w,0);
	time_line_scroll->update_value(-x0);
	channel_data->resize_window(path_w, frame_h);
	channel_frame->resize_window(frame_w, frame_h);
	int nitems = channel_line_items.size();
	for( int i=0; i<nitems; ++i )
		channel_line_items.values[i]->resize_window(frame_w, path_h);
	channel_scroll->reposition_window(frame_x+frame_w, frame_y, frame_h);
	channel_scroll->update_length(y1-y0, y_scroll-y0, frame_h,0);
	flush();
}

int ChannelPanel::button_press_event()
{
	if(get_buttonpress() == 3 && cursor_inside()) {
		gui->lock_window("ChannelPanel::button_press_event");
		gui->channel_search->start();
		gui->unlock_window();
		return 1;
	}
	return 0;
}

void ChannelPanel::bounding_box(int ix0, int iy0, int ix1, int iy1)
{
	int x_changed = 0, y_changed = 0;
	if( ix0 < x0 ) { x0 = ix0;  x_changed = 1; }
	if( iy0 < y0 ) { y0 = iy0;  y_changed = 1; }
	if( ix1 > x1 ) { x1 = ix1;  x_changed = 1; }
	if( iy1 > y1 ) { y1 = iy1;  y_changed = 1; }
	if( x_changed ) {
		time_line_update(x0, x1);
		time_line_scroll->update_length(x1-x0, x_scroll-x0, frame_w, 0);
		if( !x_moved ) time_line_scroll->update_value(-x0);
	}
	if( y_changed )
		channel_scroll->update_length(y1-y0, y_scroll-y0, frame_h, 0);
	if( x_changed || y_changed ) flush();
}

void ChannelPanel::set_x_scroll(int v)
{
	x_moved = 1;
	x_scroll = v + x0;
	reposition();
}

void ChannelPanel::set_y_scroll(int v)
{
	y_moved = 1;
	y_scroll = v + y0;
	reposition();
}

void ChannelPanel::reposition()
{
	int n, i;
	n = time_line_items.size();
	for( i=0; i<n; ++i ) {
		TimeLineItem *item = time_line_items[i];
		if( item->get_x() == item->x0-x_scroll &&
		    item->get_y() == item->y0-y_scroll) continue;
		item->reposition_window(item->x0-x_scroll, item->y0);
	}
	n = channel_data_items.size();
	for( i=0; i<n; ++i ) {
		ChannelDataItem *item = channel_data_items[i];
		if( item->get_x() == item->x0-x_scroll &&
		    item->get_y() == item->y0-y_scroll) continue;
		item->reposition_window(item->x0, item->y0-y_scroll);
	}
	n = channel_line_items.size();
	for( i=0; i<n; ++i ) {
		ChannelEventLine *item = channel_line_items[i];
		if( item->get_x() == item->x0-x_scroll &&
		    item->get_y() == item->y0-y_scroll) continue;
		item->reposition_window(item->x0, item->y0-y_scroll);
	}
	n = channel_event_items.size();
	for( i=0; i<n; ++i ) {
		ChannelEvent *item = channel_event_items[i];
		if( item->get_x() == item->x0-x_scroll &&
		    item->get_y() == item->y0-y_scroll) continue;
		item->reposition_window(item->x0-x_scroll, item->y0);
	}
}

void ChannelPanel::get_xtime(int x, char *cp)
{
	time_t xt = st_org + (x/hhr_w) * 1800;
	if( !strcmp(tzname[0],"UTC") ) xt -= tz.tz_minuteswest*60;
	struct tm xtm;  localtime_r(&xt,&xtm);
	cp += sprintf(cp,"%02d:%02d",xtm.tm_hour, xtm.tm_min);
	if( !xtm.tm_hour && !xtm.tm_min ) {
		sprintf(cp,"(%3.3s) ",&_("sunmontuewedthufrisat")[xtm.tm_wday*3]);
	}
}

void ChannelPanel::time_line_update(int ix0, int ix1)
{
	int x;  char text[BCTEXTLEN];
	for( x=t0; x>=ix0; x-=hhr_w ) {
		get_xtime(x, text);  t0 = x;
		TimeLineItem *time_line_item = new TimeLineItem(this, t0, 0, text);
		time_line->add_subwindow(time_line_item);
		time_line_items.insert(time_line_item,0);
	}
	for( x=t1; x<ix1; x+=hhr_w ) {
		get_xtime(x, text);  t1 = x;
		TimeLineItem *time_line_item = new TimeLineItem(this, t1, 0, text);
		time_line->add_subwindow(time_line_item);
		time_line_items.append(time_line_item);
	}
}

ChannelInfoCron::
ChannelInfoCron(ChannelInfoGUI *gui, int x, int y, int *value)
 : BC_CheckBox(x, y, value, gui->cron_caption)
{
	this->gui = gui;
	set_tooltip(_("activate batch record when ok pressed"));
}

ChannelInfoCron::
~ChannelInfoCron()
{
}

int ChannelInfoCron::
handle_event()
{
	gui->iwindow->cron_enable = get_value();
	return 1;
}


ChannelInfoPowerOff::ChannelInfoPowerOff(ChannelInfoGUI *gui, int x, int y, int *value)
 : BC_CheckBox(x, y , value, gui->power_caption)
{
	this->gui = gui;
	set_tooltip(_("poweroff system when batch record done"));
}

ChannelInfoPowerOff::~ChannelInfoPowerOff()
{
}

int ChannelInfoPowerOff::handle_event()
{
	gui->iwindow->poweroff_enable = get_value();
	return 1;
}


ChannelInfoFind::ChannelInfoFind(ChannelInfoGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Find"))
{
	this->gui = gui;
	set_tooltip(_("search event titles/info"));
}

ChannelInfoFind::~ChannelInfoFind()
{
}

int ChannelInfoFind::handle_event()
{
	gui->lock_window("ChannelInfoFind::handle_event");
	gui->channel_search->start();
	gui->unlock_window();
	return 1;
}

void ChannelThread::start()
{
	if( !Thread::running() ) {
		done = 0;
		Thread::start();
	}
}

void ChannelThread::stop()
{
	if( Thread::running() ) {
		done = 1;
		Thread::cancel();
	}
	Thread::join();
}

ChannelThread::ChannelThread(ChannelInfoGUI *gui)
 : Thread(1, 0, 0)
{
	this->gui = gui;
	this->iwindow = gui->iwindow;
	this->panel = gui->panel;

	fd = 0;
	done = 0;
}

ChannelThread::~ChannelThread()
{
	stop();
}

int ChannelThread::load_ident(int n, int y, char *ident)
{
	ChannelDataItem *data_item = new ChannelDataItem(panel, 0, y,
		panel->path_w, !n ? YELLOW : LTYELLOW, ident);
	panel->channel_data->add_subwindow(data_item);
	panel->channel_data_items.append(data_item);
	if( !n ) {
		char info[BCTEXTLEN];
		int i = mpeg3_dvb_get_chan_info(fd, n, -1, 0, info, sizeof(info)-1);
		while( --i >= 0 && (info[i]=='\n' || info[i]==' ') ) info[i] = 0;
		if( info[0] ) data_item->set_tooltip(info);
	}
	return 0;
}

int ChannelThread::load_info(Channel *channel, ChannelEventLine *channel_line)
{
	int n = channel->element;
	int ord = 0, i = 0;

	while( ord < 0x80 ) {
		char info[65536], *cp = &info[0];
		int len = mpeg3_dvb_get_chan_info(fd,n,ord,i++,cp,sizeof(info)-1);
		if( len < 0 ) { i = 0;  ++ord;  continue; }
		char *bp = cp;  cp += len;
		struct tm stm;  memset(&stm,0,sizeof(stm));
		struct tm etm;  memset(&etm,0,sizeof(etm));
		int k, l = sscanf(bp,"%d:%d:%d-%d:%d:%d %n",
			&stm.tm_hour, &stm.tm_min, &stm.tm_sec,
			&etm.tm_hour, &etm.tm_min, &etm.tm_sec,
			&k);
		if( l != 6 ) {
			printf(_("bad scan time: %s\n"),info);
			continue;
		}
		char *title = (bp += k);
		while( bp<cp && *bp!='\n' ) ++bp;
		char *dp = bp;  *bp++ = 0;
		if( !*title ) {
			printf(_("bad title: %s\n"),info);
			continue;
		}
		char stm_wday[4];  memset(&stm_wday,0,sizeof(stm_wday));
		l = sscanf(bp, "(%3s) %d/%d/%d", &stm_wday[0],
			&stm.tm_year, &stm.tm_mon, &stm.tm_mday);
		if( l != 4 ) {
			printf(_("bad scan date: %s\n"),info);
			continue;
		}
		while( bp<cp && *bp!='\n' ) ++bp;
		*bp = 0;
		etm.tm_year = (stm.tm_year -= 1900);
		etm.tm_mon = --stm.tm_mon;
		etm.tm_mday = stm.tm_mday;
		stm.tm_isdst = etm.tm_isdst = -1;
		time_t st = mktime(&stm);
		time_t et = mktime(&etm);
		if( et < st ) et += 24*3600;
		if( et < st ) {
			printf(_("end before start: %s\n"),info);
			continue;
		}
		if( panel->st_org - et > 24*3600*2) {
			printf(_("end time early: %s\n"),info);
			continue;
		}
		if( st - panel->st_org > 24*3600*12 ) {
			printf(_("start time late: %s\n"),info);
			continue;
		}
		time_t st_min = (st - panel->st_org)/60;
		time_t et_min = (et - panel->st_org)/60;
		int dt = et_min - st_min;
		if( dt <= 0 ) {
			printf(_("zero duration: %s\n"),info);
			continue;
		}

		int w = (dt * panel->hhr_w) / 30;
		int x = (st_min * panel->hhr_w) / 30;
		int y = channel_line->y0;
		panel->bounding_box(x, y, x+w, y+panel->path_h);
		ChannelEvent *channel_event =
			new ChannelEvent(channel_line, channel,
					st, et, x, 0, w, title);
		channel_line->add_subwindow(channel_event);
		panel->channel_event_items.append(channel_event);

		*dp = '\n';  *bp++ = '\n';
		for( char *lp=bp; bp<cp; ++bp ) {
			if( *bp == '\n' || ((bp-lp)>=60 && *bp==' ') )
				*(lp=bp) = '\n';
		}
		*cp = 0;
		for( bp=&info[0]; --cp>=bp && (*cp=='\n' || *cp==' '); *cp=0 );
		if( info[0] ) channel_event->set_tooltip(info);
	}
	return 0;
}

void ChannelThread::run()
{
	gui->init_wait();
	Channel *channel = 0;
	int y = 0;
	int nchannels = total_channels();
	enable_cancel();

	for(int ch=0; !done && ch<nchannels; gui->update_progress(++ch) ) {
		Channel *chan = get_channel(ch);
		if( !chan ) continue;
		disable_cancel();
		iwindow->vdevice_lock->lock("ChannelThread::run");
		DeviceDVBInput *dvb_input = iwindow->dvb_input;
		if( !channel || chan->entry != channel->entry ) {
			int retry = 3;
			while( dvb_input->set_channel(chan) && --retry >= 0 );
			if( retry >= 0 ) {
				channel = chan;
				if( y ) {
					gui->lock_window();
					y += panel->separator(y);
					gui->unlock_window();
				}
			}
			else
				channel = 0;
		}
		else if( chan->element == channel->element )
			chan = 0;
		else
			channel = chan;
		if( !done && chan && channel && (fd=dvb_input->get_src()) ) {
			gui->lock_window();
			load_ident(chan->element, y, chan->title);
			ChannelEventLine *channel_line = panel->NewChannelLine(y);
			load_info(chan, channel_line);
			channel_line->show_window();
			gui->unlock_window();
			dvb_input->put_src();
			y += panel->path_h;
		}
		iwindow->vdevice_lock->unlock();
		enable_cancel();
	}

	gui->lock_window("ChannelProgress::run");
	gui->channel_status->hide_window();
	gui->unlock_window();

	disable_cancel();
	iwindow->close_vdevice();
}


ChannelInfoOK::ChannelInfoOK(ChannelInfoGUI *gui, int x, int y)
 : BC_OKButton(x, y)
{
	this->gui = gui;
	set_tooltip(_("end channel info, start record"));
}

ChannelInfoOK::~ChannelInfoOK()
{
}

int ChannelInfoOK::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->stop(0);
		return 1;
	}
	return 0;
}

int ChannelInfoOK::keypress_event()
{
	return 0;
}


ChannelInfoCancel::ChannelInfoCancel(ChannelInfoGUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
	this->gui = gui;
}

ChannelInfoCancel::~ChannelInfoCancel()
{
}

int ChannelInfoCancel::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->stop(1);
		return 1;
	}
	return 0;
}


ChannelInfoGUIBatches::ChannelInfoGUIBatches(ChannelInfoGUI *gui,
	int x, int y, int w, int h)
 : RecordBatchesGUI(gui->iwindow->record_batches, x, y, w, h)
{
	this->gui = gui;
}

ChannelInfoGUIBatches::~ChannelInfoGUIBatches()
{
}

int ChannelInfoGUIBatches::selection_changed()
{
	return RecordBatchesGUI::selection_changed();
}

int ChannelInfoGUIBatches::handle_event()
{
	if( get_double_click() )
		gui->update_channel_tools();
	return 1;
}


void ChannelInfoGUI::create_objects()
{
	lock_window("ChannelInfoGUI::create_objects");
	panel = new ChannelPanel(this,0,0,panel_w,panel_h);
	add_subwindow(panel);
	panel->create_objects();
	int items = iwindow->channeldb->size();
	if( items < 1 ) items = 1;
	progress = new ChannelProgress(this, 0, 0, path_w, path_h, items);
	add_subwindow(progress);
	progress->create_objects();
	ok = new ChannelInfoOK(this, ok_x, ok_y);
	add_subwindow(ok);
	channel_cron = new ChannelInfoCron(this, cron_x, cron_y, &iwindow->cron_enable);
	add_subwindow(channel_cron);
	channel_poweroff = new ChannelInfoPowerOff(this,
		 power_x, power_y, &iwindow->poweroff_enable);
	add_subwindow(channel_poweroff);
	channel_find = new ChannelInfoFind(this, find_x, find_y);
	add_subwindow(channel_find);
	cancel = new ChannelInfoCancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	batch_bay = new ChannelInfoGUIBatches(this,bay_x, bay_y, bay_w, bay_h);
	add_subwindow(batch_bay);
	iwindow->record_batches.gui = batch_bay;
	batch_bay->set_current_batch(-1);
	batch_bay->update_batches(-1);

	pad = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1) + 5;
	x0 = bay_x+bay_w + 10;
	y0 = bay_y+10;
	int ww = 0;
	int x = x0;
	int y = y0;

	add_subwindow(directory_title = new BC_Title(x, y, _("Directory:")));
	ww = max(directory_title->get_w(), ww);   y += pad;
	add_subwindow(path_title = new BC_Title(x, y, _("Path:")));
	ww = max(path_title->get_w(), ww);       y += pad;
	add_subwindow(start_title = new BC_Title(x, y, _("Start:")));
	ww = max(start_title->get_w(), ww);      y += pad;
	add_subwindow(duration_title = new BC_Title(x, y, _("Duration:")));
	ww = max(duration_title->get_w(), ww);   y += pad;
	add_subwindow(source_title = new BC_Title(x, y, _("Source:")));
	ww = max(source_title->get_w(), ww);     y += pad;
	title_w = ww;

	int x1 = x0 + title_w + pad;
	x = x1;  y = y0;  ww = 0;

	char *dir = iwindow->record_batches.get_default_directory();
	add_subwindow(channel_dir = new ChannelDir(this, dir, x, y));
	ww = max(channel_dir->get_w(), ww);     y += pad;
	add_subwindow(channel_path = new ChannelPath(this, x, y));
	ww = max(channel_path->get_w(), ww);   y += pad;
	channel_start = new ChannelStart(this, x, y);
	channel_start->create_objects();
	ww = max(channel_start->get_w(), ww);   y += pad;
	int w = BC_Title::calculate_w(this, (char*)"+00:00:00+", MEDIUMFONT);
	channel_duration = new ChannelDuration(this, x, y, w);
	channel_duration->create_objects();
	int x2 = x + channel_duration->get_w();
	double *early_margin = iwindow->record_batches.get_early_margin();
	early_time = new ChannelEarlyTime(this, x2, y, early_margin);
	early_time->create_objects();
	x2 += early_time->get_w();
	double *late_margin = iwindow->record_batches.get_late_margin();
	late_time = new ChannelLateTime(this, x2, y, late_margin);
	late_time->create_objects();
	x2 += late_time->get_w();
	ww = max(x2-x1, ww);                    y += pad;
	channel_source = new ChannelSource(this, x, y);
	channel_source->create_objects();
	ww = max(channel_source->get_w(), ww);  y += pad;
	data_w = x1-x0 + ww;

	x = x0 + pad;
	add_subwindow(channel_clear_batch = new ChannelClearBatch(this, x, y));
	x += channel_clear_batch->get_w() + 10;
	add_subwindow(channel_new_batch = new ChannelNewBatch(this, x, y));
	x += channel_new_batch->get_w() + 10;
	add_subwindow(channel_delete_batch = new ChannelDeleteBatch(this, x, y));
	x += channel_delete_batch->get_w();
	ww = max(x-x0, ww);
	data_w = max(ww, data_w);

	channel_status = new ChannelStatus(this, x0+data_w, y0);
	add_subwindow(channel_status);
	channel_status->create_objects();
	status_w = channel_status->get_w();

	channel_search = new ChanSearch(iwindow);
	show_window();
	unlock_window();
}

ChannelInfoGUI::ChannelInfoGUI(ChannelInfo *iwindow,
	int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Channel Info"), x, y, w, h, 600, 400,
	1, 0, 0 , -1, iwindow->mwindow->get_cwindow_display())
{
	this->iwindow = iwindow;
	panel = 0;
	batch_bay =0;
	channel_dir = 0;
	channel_path = 0;
	channel_start = 0;
	channel_duration = 0;
	channel_source = 0;
	channel_clear_batch = 0;
	channel_new_batch = 0;
	channel_delete_batch = 0;
	early_time = late_time = 0;
	directory_title = 0;
	path_title = 0;
	start_title = 0;
	duration_title = 0;
	source_title = 0;
	cron_caption = _("Start Cron");
	power_caption = _("Poweroff");
	ok = 0;
	cancel = 0;

	path_w = 16*BC_Title::calculate_w(this, (char*)"X", MEDIUMFONT);
	path_h = BC_TextBox::calculate_h(this, MEDIUMFONT, 0, 1);
	x0 = y0 = title_w = data_w = status_w = pad = 0;
	ok_w = BC_OKButton::calculate_w();
	ok_h = BC_OKButton::calculate_h();
	ok_x = 10;
	ok_y = h - ok_h - 10;
	BC_CheckBox::calculate_extents(this, &cron_w, &cron_h, cron_caption);
	cron_x = ok_x;
	cron_y = ok_y - cron_h - 10;
	BC_CheckBox::calculate_extents(this, &power_w, &power_h, power_caption);
	power_x = cron_x;
	power_y = cron_y - power_h - 5;
	find_h = BC_GenericButton::calculate_h();
	find_x = power_x;
	find_y = power_y - find_h - 10;
	cancel_w = BC_CancelButton::calculate_w();
	cancel_h = BC_CancelButton::calculate_h();
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;
	max_bay_w = 700;
	bay_h = 150;
	bay_x = ok_w + 20;
	int x1 = cron_x+cron_w + 10;
	if( x1 > bay_x ) bay_x = x1;
	x1 = power_x+power_w + 10;
	if( x1 > bay_x ) bay_x = x1;
	bay_y = h - bay_h;
	// data_w,status_w zero, updated in create_objects
	bay_w = (w-bay_x) - (data_w+10) - (max(cancel_w, status_w)+20);
	if( bay_w > max_bay_w ) bay_w = max_bay_w;
	panel_w = w;
	panel_h = h - bay_h;
}

ChannelInfoGUI::~ChannelInfoGUI()
{
	progress->stop();
	channel_search->stop();
	flush();  sync();
	delete channel_status;
	delete channel_search;
	delete channel_start;
	delete channel_duration;
	delete early_time;
	delete late_time;
	delete channel_source;
}

void ChannelInfoGUI::stop(int v)
{
	if( !iwindow->gui_done ) {
		iwindow->gui_done = 1;
		set_done(v);
	}
}

int ChannelInfoGUI::translation_event()
{
	iwindow->mwindow->session->cswindow_x = get_x();
	iwindow->mwindow->session->cswindow_y = get_y();
	return 0;
}

int ChannelInfoGUI::resize_event(int w, int h)
{
	iwindow->mwindow->session->cswindow_w = w;
	iwindow->mwindow->session->cswindow_h = h;
	panel_w = w;
	panel_h = h - bay_h;
	panel->resize(panel_w,panel_h);
	panel->reposition_window(0,0,panel_w,panel_h);
	ok_x = 10;
	ok_y = h - ok_h - 10;
	ok->reposition_window(ok_x, ok_y);
	cron_x = ok_x;
	cron_y = ok_y - cron_h - 10;
	channel_cron->reposition_window(cron_x, cron_y);
	power_x = cron_x;
	power_y = cron_y - power_h - 5;
	channel_poweroff->reposition_window(power_x, power_y);
	find_x = power_x;
	find_y = power_y - find_h - 10;
	channel_find->reposition_window(find_x, find_y);
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;
	cancel->reposition_window(cancel_x, cancel_y);
	bay_x = ok_w + 20;
	int x1 = cron_x+cron_w + 10;
	if( x1 > bay_x ) bay_x = x1;
	x1 = power_x+power_w + 10;
	if( x1 > bay_x ) bay_x = x1;
	bay_y = h - bay_h;
	bay_w = (w-bay_x) - (data_w+10) - (max(cancel_w, status_w)+20);
	if( bay_w > max_bay_w ) bay_w = max_bay_w;
	batch_bay->reposition_window(bay_x, bay_y, bay_w, bay_h);

	int x0 = bay_x+bay_w + 10;
	int y0 = bay_y+10;
	int x = x0;
	int y = y0;

	directory_title->reposition_window(x, y);  y += pad;
	path_title->reposition_window(x, y);       y += pad;
	start_title->reposition_window(x, y);      y += pad;
	duration_title->reposition_window(x, y);   y += pad;
	source_title->reposition_window(x, y);

	x = x0 + title_w + pad;
	y = y0;

	channel_dir->reposition_window(x, y);      y += pad;
	channel_path->reposition_window(x, y);     y += pad;
	channel_start->reposition_window(x, y);    y += pad;
	channel_duration->reposition_window(x, y);
	int x2 = x + channel_duration->get_w();
	early_time->reposition_window(x2, y);
	x2 += early_time->get_w();
	late_time->reposition_window(x2, y);       y += pad;
	channel_source->reposition_window(x, y);   y += pad;

	x = x0 + pad;
	channel_clear_batch->reposition_window(x, y);
	x += channel_clear_batch->get_w() + 10;
	channel_new_batch->reposition_window(x, y);
	x += channel_new_batch->get_w() + 10;
	channel_delete_batch->reposition_window(x, y);

	y = y0;
	x = x0 + data_w;
	channel_status->reposition_window(x, y);
	return 1;
}

int ChannelInfoGUI::close_event()
{
	stop(1);
	return 1;
}

void ChannelInfoGUI::update_channel_tools()
{
	Batch *batch = batch_bay->get_editing_batch();
	channel_path->update(batch->asset->path);
	channel_start->update(&batch->start_day, &batch->start_time);
	channel_duration->update(0, &batch->duration);
	channel_source->update(batch->get_source_text());
	flush();
}

void ChannelInfoGUI::incr_event(int start_time_incr, int duration_incr)
{
	Batch *batch = batch_bay->get_editing_batch();
	batch->start_time += start_time_incr;
	batch->duration += duration_incr;
	batch_bay->update_batches();
	update_channel_tools();
}


ChannelInfo::ChannelInfo(MWindow *mwindow)
 : Thread(1, 0, 0),
   record_batches(mwindow)
{
	this->mwindow = mwindow;
	this->record = mwindow->gui->record;
	scan_lock = new Condition(0,"ChannelInfo::scan_lock");
	window_lock = new Mutex("ChannelInfo::window_lock");
	vdevice_lock = new Mutex("ChannelInfo::vdevice_lock");
	progress_lock = new Mutex("ChannelInfo::progress_lock");

	vdevice = 0;
	dvb_input = 0;
	gui = 0;
	thread = 0;
	channeldb = 0;
	cron_enable = 1;
	poweroff_enable = 0;
	item = 0;
	done = 1;
	gui_done = 1;

	start();
}

ChannelInfo::~ChannelInfo()
{
	stop();
	delete thread;
	delete gui;  gui = 0;
	delete vdevice;
	record_batches.clear();
	delete channeldb;
	delete scan_lock;
	delete window_lock;
	delete vdevice_lock;
	delete progress_lock;
}

void ChannelInfo::run_scan()
{
	window_lock->lock("ChannelInfo::run_scan");
	if( gui ) {
		gui->lock_window("ChannelInfo::run_scan");
		gui->raise_window();
		gui->unlock_window();
	}
	else
		scan_lock->unlock();
	window_lock->unlock();
}

void ChannelInfo::toggle_scan()
{
	window_lock->lock("ChannelInfo::toggle_scan");
	if( gui )
		gui->stop(1);
	else
		scan_lock->unlock();
	window_lock->unlock();
}

void ChannelInfo::start()
{
	if( !Thread::running() ) {
		done = 0;
		Thread::start();
	}
}

void ChannelInfo::stop()
{
	if( Thread::running() ) {
		done = 1;
		scan_lock->unlock();
		window_lock->lock("ChannelInfo::stop");
		if( gui ) gui->stop(1);
		window_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
}

void ChannelInfo::run()
{
	int root_w = mwindow->gui->get_root_w(1);
	int root_h = mwindow->gui->get_root_h(1);

	while( !done ) {
		scan_lock->reset();
		scan_lock->lock();
		if( done ) break;
		if( record->Thread::running() ) {
			char string[BCTEXTLEN];
			sprintf(string,_("Recording in progress\n"));
			MainError::show_error(string);
			continue;
		}
		EDLSession *session = mwindow->edl->session;
		VideoInConfig *vconfig_in = session->vconfig_in;
		if( vconfig_in->driver != CAPTURE_DVB ) {
			char string[BCTEXTLEN];
			sprintf(string,_("capture driver not dvb\n"));
			MainError::show_error(string);
			continue;
		}
		int x = mwindow->session->cswindow_x;
		int y = mwindow->session->cswindow_y;
		int w = mwindow->session->cswindow_w;
		int h = mwindow->session->cswindow_h;
		if( w < 600 ) w = 600;
		if( h < 400 ) h = 400;
		int scr_x = mwindow->gui->get_screen_x(1, -1);
		int scr_w = mwindow->gui->get_screen_w(1, -1);
		if( x < scr_x ) x = scr_x;
		if( x > scr_x+scr_w ) x = scr_x+scr_w;
		if( x+w > root_w ) x = root_w - w;
		if( x < 0 ) { x = 0;  w = scr_w; }
		if( y+h > root_h ) y = root_h - h;
		if( y < 0 ) { y = 0;  h = root_h; }
		if( y+h > root_h ) h = root_h-y;
		mwindow->session->cswindow_x = x;
		mwindow->session->cswindow_y = y;
		mwindow->session->cswindow_w = w;
		mwindow->session->cswindow_h = h;
		cron_enable = 1;
		poweroff_enable = 0;
		vdevice = new VideoDevice(mwindow);
		int result = vdevice->open_input(vconfig_in, 0, 0, 1., vconfig_in->in_framerate);
		dvb_input = result ? 0 : (DeviceDVBInput *)vdevice->mpeg_device();
		if( dvb_input ) {
			channeldb = new ChannelDB;
			VideoDevice::load_channeldb(channeldb, vconfig_in);
			record_batches.load_defaults(channeldb);
			item = 0;
			window_lock->lock("ChannelInfo::run 0");
			gui_done = 0;
			gui = new ChannelInfoGUI(this, x, y, w, h);
			gui->lock_window("ChannelInfo::gui_create_objects");
			gui->create_objects();
			gui->set_icon(mwindow->theme->get_image("record_icon"));
			gui->reposition_window(x, y);
			gui->resize_event(w, h);
        		dvb_input->set_signal_status(gui->channel_status);
			gui->unlock_window();
			thread = new ChannelThread(gui);
			thread->start();
			window_lock->unlock();
			result = gui->run_window();
			delete thread;  thread = 0;
			close_vdevice();
			gui->lock_window();
			gui->unlock_window();
			window_lock->lock("ChannelInfo::run 1");
			delete gui;  gui = 0;
			window_lock->unlock();
			record_batches.save_defaults(channeldb);
			if( !result ) {
				record_batches.save_default_channel(channeldb);
				record->start();
				record->init_lock->lock();
				if( cron_enable ) {
					record->set_power_off(poweroff_enable);
					record->start_cron_thread();
				}
			}
			record_batches.clear();
			delete channeldb;  channeldb = 0;
		}
		else {
			close_vdevice();
			char string[BCTEXTLEN];
			sprintf(string,_("cannot open dvb video device\n"));
			MainError::show_error(string);
		}
	}
}

void ChannelInfo::close_vdevice()
{
	vdevice_lock->lock("ChannelInfo::close_vdevice");
	progress_lock->lock("ChannelInfo::close_vdevice");
	if( vdevice ) {
		vdevice->close_all();
		delete vdevice;  vdevice = 0;
	}
	progress_lock->unlock();
	vdevice_lock->unlock();
}

Batch *ChannelInfo::new_batch()
{
	Batch *batch = new Batch(gui->iwindow->mwindow, 0);
	batch->create_objects();
	batch->calculate_news();
	gui->iwindow->record_batches.append(batch);
	return batch;
}

void ChannelInfo::delete_batch()
{
// Abort if one batch left
	int edit_batch = editing_batch();
	int total_batches = record_batches.total();
	if( total_batches > 1 && edit_batch < total_batches ) {
		Batch *batch = record_batches[edit_batch];
		record_batches.remove(batch);  delete batch;
	}
}

ChannelScan::ChannelScan(MWindow *mwindow)
 : BC_MenuItem(_("Scan..."), _("Ctrl-Alt-s"), 's')
{
	set_ctrl();
	set_alt();
	this->mwindow = mwindow;
}

ChannelScan::~ChannelScan()
{
}

int ChannelScan::handle_event()
{
        mwindow->gui->channel_info->run_scan();
	return 1;
}


ChannelDir::ChannelDir(ChannelInfoGUI *gui, const char *dir, int x, int y)
 : RecordBatchesGUI::Dir(gui->iwindow->record_batches, dir, x, y)
{
	this->gui = gui;
}


ChannelPath::ChannelPath(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::Path(gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
}


ChannelStart::ChannelStart(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::StartTime(gui, gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
}


ChannelDuration::ChannelDuration(ChannelInfoGUI *gui, int x, int y, int w)
 : RecordBatchesGUI::Duration(gui, gui->iwindow->record_batches, x, y, w)
{
	this->gui = gui;
}


ChannelEarlyTime::ChannelEarlyTime(ChannelInfoGUI *gui, int x, int y,
	double *output_time)
 : TimeEntryTumbler(gui, x, y, 0, output_time, 15, TIME_MS2, 75)
{
	this->gui = gui;
}

int ChannelEarlyTime::handle_up_event()
{
	gui->incr_event(incr,-incr);
	return TimeEntryTumbler::handle_up_event();
}

int ChannelEarlyTime::handle_down_event()
{
	gui->incr_event(-incr,incr);
	return TimeEntryTumbler::handle_down_event();
}


ChannelLateTime::ChannelLateTime(ChannelInfoGUI *gui, int x, int y,
	double *output_time)
 : TimeEntryTumbler(gui, x, y, 0, output_time, 15, TIME_MS2, 75)
{
	this->gui = gui;
}

int ChannelLateTime::handle_up_event()
{
	gui->incr_event(0,incr);
	return TimeEntryTumbler::handle_up_event();
}

int ChannelLateTime::handle_down_event()
{
	gui->incr_event(0,-incr);
	return TimeEntryTumbler::handle_down_event();
}


ChannelSource::ChannelSource(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::Source(gui, gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
}

void ChannelSource::create_objects()
{
	RecordBatchesGUI::Source::create_objects();
	ChannelDB *channeldb = gui->iwindow->channeldb;
	sources.remove_all_objects();
	for(int i = 0; i < channeldb->size(); i++) {
		sources.append(new BC_ListBoxItem(channeldb->get(i)->title));
	}
	update_list(&sources);
}

int ChannelSource::handle_event()
{
	int chan_no = get_number();
	Channel *channel = gui->iwindow->channeldb->get(chan_no);
	if( channel ) {
		Batch *batch = batches.get_editing_batch();
		if( batch ) batch->channel = channel;
		update(channel->title);
	}
	return RecordBatchesGUI::Source::handle_event();
}


ChannelClearBatch::ChannelClearBatch(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::ClearBatch(gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Delete all clips."));
}

int ChannelClearBatch::handle_event()
{
	gui->iwindow->record_batches.clear();
	gui->iwindow->new_batch();
	gui->batch_bay->set_current_batch(-1);
	gui->batch_bay->set_editing_batch(0);
	gui->batch_bay->update_batches(0);
	return RecordBatchesGUI::ClearBatch::handle_event();
}


ChannelNewBatch::ChannelNewBatch(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::NewBatch(gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Create new clip."));
}
int ChannelNewBatch::handle_event()
{
	gui->iwindow->new_batch();
	return RecordBatchesGUI::NewBatch::handle_event();
}


ChannelDeleteBatch::ChannelDeleteBatch(ChannelInfoGUI *gui, int x, int y)
 : RecordBatchesGUI::DeleteBatch(gui->iwindow->record_batches, x, y)
{
	this->gui = gui;
	set_tooltip(_("Delete clip."));
}

int ChannelDeleteBatch::handle_event()
{
	gui->iwindow->delete_batch();
	return RecordBatchesGUI::DeleteBatch::handle_event();
}

#endif

