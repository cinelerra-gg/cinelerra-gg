
#include "bchash.h"
#include "bctimer.h"
#include "condition.h"
#include "cstrdup.h"
#include "edl.h"
#include "filesystem.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainmenu.h"
#include "mainsession.h"
#include "mainundo.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "mutex.h"
#include "strack.h"
#include "swindow.h"
#include "theme.h"
#include "track.h"
#include "tracks.h"

#include<ctype.h>
#include<errno.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

SWindowOK::SWindowOK(SWindowGUI *gui, int x, int y)
 : BC_OKButton(x, y)
{
	this->gui = gui;
}

SWindowOK::~SWindowOK()
{
}

int SWindowOK::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->stop(0);
		return 1;
	}
	return 0;
}

int SWindowOK::keypress_event()
{
	return 0;
}


SWindowCancel::SWindowCancel(SWindowGUI *gui, int x, int y)
 : BC_CancelButton(x, y)
{
	this->gui = gui;
}

SWindowCancel::~SWindowCancel()
{
}

int SWindowCancel::button_press_event()
{
	if(get_buttonpress() == 1 && is_event_win() && cursor_inside()) {
		gui->stop(1);
		return 1;
	}
	return 0;
}


SWindowLoadPath::SWindowLoadPath(SWindowGUI *gui, int x, int y, char *path)
 : BC_TextBox(x, y, 200, 1, path)
{
	this->sw_gui = gui;
}

SWindowLoadPath::~SWindowLoadPath()
{
}

int SWindowLoadPath::handle_event()
{
	calculate_suggestions();
	strcpy(sw_gui->script_path, get_text());
	return 1;
}


SWindowLoadFile::SWindowLoadFile(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Load"))
{
	this->sw_gui = gui;
}

SWindowLoadFile::~SWindowLoadFile()
{
}

int SWindowLoadFile::handle_event()
{
	if( sw_gui->script_path[0] ) {
		sw_gui->load_path->set_suggestions(0,0);
		sw_gui->load_script();
		sw_gui->set_script_pos(0);
	}
	else {
		eprintf(_("script text file path required"));
	}
	return 1;
}

SWindowSaveFile::SWindowSaveFile(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Save"))
{
	this->sw_gui = gui;
}

SWindowSaveFile::~SWindowSaveFile()
{
}

int SWindowSaveFile::handle_event()
{
	if( sw_gui->script_path[0] ) {
		sw_gui->save_spumux_data();
	}
	else {
		eprintf(_("script microdvd file path required"));
	}
	return 1;
}




void SWindowGUI::create_objects()
{
	lock_window("SWindowGUI::create_objects");
	int x = 10, y = 10;
	BC_Title *title = new BC_Title(x, y, _("Path:"));
	add_subwindow(title);
	int x1 = x + title->get_w() + pad, y1 = y;
	load_path = new SWindowLoadPath(this, x1, y1, script_path);
	add_subwindow(load_path);
	x1 += load_path->get_w() + 2*pad;
	add_subwindow(load_file = new SWindowLoadFile(this, x1, y1));
	x1 += load_file->get_w() + 2*pad;
	add_subwindow(save_file = new SWindowSaveFile(this, x1, y1));
	y += max(load_path->get_h(), load_file->get_h()) + pad;
	x1 = x + pad, y1 = y;
	BC_Title *title1, *title2;
	add_subwindow(title1 = new BC_Title(x1, y1, _("File Size:")));
	y += title1->get_h() + pad;
	int y2 = y;
	add_subwindow(title2 = new BC_Title(x1, y2, _("Entries:")));
	int x2 = x1 + max(title1->get_w(), title2->get_w()) + pad;
	add_subwindow(script_filesz = new BC_Title(x2, y1, "0", MEDIUMFONT, YELLOW));
	add_subwindow(script_entries = new BC_Title(x2, y2, "0", MEDIUMFONT, YELLOW));
	int x3 = x2 + max(script_entries->get_w()*8, script_filesz->get_w()*8) + pad;
	add_subwindow(title1 = new BC_Title(x3, y1, _("Lines:")));
	add_subwindow(title2 = new BC_Title(x3, y2, _("Texts:")));
	int x4 = x3 + max(title1->get_w(), title2->get_w()) + pad;
	add_subwindow(script_lines = new BC_Title(x4, y1, "0", MEDIUMFONT, YELLOW));
	add_subwindow(script_texts = new BC_Title(x4, y2, "0", MEDIUMFONT, YELLOW));
	int x5 = x4 + max(script_lines->get_w()*8, script_texts->get_w()*8) + 2*pad;
	add_subwindow(prev_script = new ScriptPrev(this, x5, y1));
	add_subwindow(next_script = new ScriptNext(this, x5, y2));
	int x6 = x5 + max(prev_script->get_w(), next_script->get_w()) + 2*pad;
	add_subwindow(paste_script = new ScriptPaste(this, x6, y1));
	add_subwindow(clear_script = new ScriptClear(this, x6, y2));
	y += max(title1->get_h(), title2->get_h()) + 2*pad;

	script_position = new ScriptPosition(this, x, y, 100);
	script_position->create_objects();
	x1 = x + script_position->get_w() + pad;
	add_subwindow(script_scroll = new ScriptScroll(this, x1, y, get_w()-x1-pad));
	y += script_scroll->get_h() + 2*pad;
	x1 = x + pad;
	blank_line = new char[2];
	blank_line[0] = ' ';  blank_line[1] = 0;
	add_subwindow(script_title = new BC_Title(x1, y, _("Script Text:")));
	y += script_title->get_h() + pad;
	int rows = (ok_y - y - BC_Title::calculate_h(this,_("Line Text:")) -
		4*pad) / text_rowsz - 4;
	int w1 = get_w() - x1 - pad;
	script_entry = new ScriptEntry(this, x1, y, w1, rows, blank_line);
	script_entry->create_objects();
	y += script_entry->get_h() + pad;
	add_subwindow(line_title = new BC_Title(x1, y, _("Line Text:")));
	y += line_title->get_h() + pad;
	line_entry = new ScriptEntry(this, x1, y, w1, 4);
	line_entry->create_objects();
	ok = new SWindowOK(this, ok_x, ok_y);
	add_subwindow(ok);
	cancel = new SWindowCancel(this, cancel_x, cancel_y);
	add_subwindow(cancel);
	unlock_window();
}

void SWindowGUI::load()
{
	const char *script_text =
	_("Adding Subtitles: quick \"How To\" (= or * indicates comment)\n"
	"*2345678901234567890123456789012345678901234567890123456789\n"
	"For regular DVD subtitles, put script in a text file. "
	"Lines can be any length but they will be broken up "
	"to fit according to some criteria below.\n"
	"Running text used as script lines will be broken into multilple lines.\n"
	"The target line length is 60 characters.\n"
	"Punctuation may be flagged to create an early line break.\n"
	"Single carriage return ends an individual script line.\n"
	"Double carriage return indicates the end of an entry.\n"
	"Whitespace at beginning or end of line is removed.\n"
	"You can edit the active line in the Line Text box.\n"
	"\n"
	"== A new entry is here for illustration purposes.\n"
	"*  Entry 2\n"
	"This is the second entry.\n");

	if( script_path[0] && !access(script_path,R_OK) ) {
		load_script();
	}
	else {
		script_path[0] = 0;
		load_path->update(script_path);
		FILE *fp = fmemopen((void *)script_text, strlen(script_text), "r");
		load_script(fp);
	}
	int text_no = script_text_no;
	script_text_no = -1;
	load_selection(script_entry_no, text_no);
}

SWindowGUI::SWindowGUI(SWindow *swindow, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Subtitle"), x, y, w, h, 600, 500,
	1, 0, 0 , -1, swindow->mwindow->get_cwindow_display())
{
	this->swindow = swindow;
	pad = 8;

	ok = 0;
	ok_w = BC_OKButton::calculate_w();
	ok_h = BC_OKButton::calculate_h();
	ok_x = 10;
	ok_y = h - ok_h - 10;
	cancel = 0;
	cancel_w = BC_CancelButton::calculate_w();
	cancel_h = BC_CancelButton::calculate_h();
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;

	load_path = 0;
	load_file = 0;
	save_file = 0;
	script_filesz = 0;
	script_lines = 0;
	script_entries = 0;
	script_texts = 0;
	script_entry_no = 0;
	script_text_no = 0;
	script_line_no = 0;
	script_text_lines = 0;
	prev_script = 0;
	next_script = 0;
	paste_script = 0;
	clear_script = 0;
	script_position = 0;
	script_entry = 0;
	line_entry = 0;
	script_scroll = 0;
	blank_line = 0;
	text_font = MEDIUMFONT;
	text_rowsz = get_text_ascent(text_font)+1 + get_text_descent(text_font)+1;
}

SWindowGUI::~SWindowGUI()
{
	delete script_entry;
	delete line_entry;
	delete script_position;
	delete [] blank_line;
}

void SWindowGUI::stop(int v)
{
	if( !swindow->gui_done ) {
		swindow->gui_done = 1;
		set_done(v);
	}
}

int SWindowGUI::translation_event()
{
	swindow->mwindow->session->swindow_x = get_x();
	swindow->mwindow->session->swindow_y = get_y();
	return 0;
}

int SWindowGUI::resize_event(int w, int h)
{
	swindow->mwindow->session->swindow_w = w;
	swindow->mwindow->session->swindow_h = h;

	ok_x = 10;
	ok_y = h - ok_h - 10;
	ok->reposition_window(ok_x, ok_y);
	cancel_x = w - cancel_w - 10,
	cancel_y = h - cancel_h - 10;
	cancel->reposition_window(cancel_x, cancel_y);

	int x = script_position->get_x();
	int y = script_position->get_y();
	int hh = script_position->get_h();
	int ww = script_position->get_w();
	int x1 = x + ww + pad;
	int w1 = w - x1 - pad;
	script_scroll->reposition_window(x1, y, w1);
	y += hh + 2*pad;
	script_title->reposition_window(x, y);
	y += script_title->get_h() + pad;
	w1 = w - x - pad;
	int rows = (ok_y - y - line_title->get_h() - 4*pad) / text_rowsz - 4;
	script_entry->reposition_window(x, y, w1, rows);
	y += script_entry->get_h() + 2*pad;
	line_title->reposition_window(x, y);
	y += line_title->get_h() + pad;
	line_entry->reposition_window(x, y, w1, 4);
	return 0;
}

void SWindowGUI::load_defaults()
{
	BC_Hash *defaults = swindow->mwindow->defaults;
	defaults->get("SUBTTL_SCRIPT_PATH", script_path);
	script_entry_no = defaults->get("SUBTTL_SCRIPT_ENTRY_NO", script_entry_no);
	script_text_no = defaults->get("SUBTTL_SCRIPT_TEXT_NO", script_text_no);
}

void SWindowGUI::save_defaults()
{
	BC_Hash *defaults = swindow->mwindow->defaults;
	defaults->update("SUBTTL_SCRIPT_PATH", script_path);
	defaults->update("SUBTTL_SCRIPT_ENTRY_NO", script_entry_no);
	defaults->update("SUBTTL_SCRIPT_TEXT_NO", script_text_no);
}

void SWindowGUI::set_script_pos(int64_t entry_no, int text_no)
{
	script_entry_no = entry_no<0 ? 0 :
		entry_no>script.size()-1 ? script.size()-1 : entry_no;
	script_text_no = text_no-1;
	load_next_selection();
}

int SWindowGUI::load_selection(int pos, int row)
{
	if( pos < 0 || pos >= script.size() ) return 1;
	ScriptLines *texts = script[pos];
	char *rp = texts->get_text_row(row);
	if( !rp || *rp == '=' || *rp == '*' || *rp=='\n' ) return 1;
	if( pos != script_entry_no || script_text_no < 0 ) {
		script_entry_no = pos;
		script_scroll->update_value(script_entry_no);
		script_position->update(script_entry_no);
		script_entry->set_text(texts->text);
		script_entry->set_text_row(0);
	}
	script_text_no = row;
	char line[BCTEXTLEN], *bp = line;
	char *ep = bp+sizeof(line)-1, *cp = rp;
	while( bp < ep && *cp && *cp!='\n' ) *bp++ = *cp++;
	*bp = 0;  bp = texts->text;
	int char1 = rp-bp, char2 = cp-bp;
	script_entry->set_selection(char1, char2, char2);
	int rows = script_entry->get_rows();
	int rows2 = rows / 2;
	int rows1 = texts->lines+1 - rows;
	if( (row-=rows2) > rows1 ) row = rows1;
	if( row < 0 ) row = 0;
	script_entry->set_text_row(row);
	line_entry->update(line);
	line_entry->set_text_row(0);
	return 0;
}

int SWindowGUI::load_prev_selection()
{
	int pos = script_entry_no, row = script_text_no;
	if( pos < 0 || pos >= script.size() ) return 1;
	for(;;) {
		if( --row < 0 ) {
			if( --pos < 0 ) break;
			row = script[pos]->lines;
			continue;
		}
		if( !load_selection(pos, row) )
			return 0;
	}
	return 1;
}

int SWindowGUI::load_next_selection()
{
	int pos = script_entry_no, row = script_text_no;
	if( pos < 0 || pos >= script.size() ) return 1;
	for(;;) {
		if( ++row >= script[pos]->lines ) {
			if( ++pos >= script.size() ) break;
			row = -1;
			continue;
		}
		if( !load_selection(pos, row) )
			return 0;
	}
	return 1;
}

int SWindowGUI::update_selection()
{
	EDL *edl = swindow->mwindow->edl;
	LocalSession *lsn = edl->local_session;
	double position = lsn->get_selectionstart();
	Edit *edit = 0;
	Tracks *tracks = edl->tracks;
	for( Track *track=tracks->first; track && !edit; track=track->next ) {
		if( !track->record ) continue;
		if( track->data_type != TRACK_SUBTITLE ) continue;
		int64_t pos = track->to_units(position,0);
		edit = track->edits->editof(pos, PLAY_FORWARD, 0);
	}
	if( !edit ) return 1;
	SEdit *sedit = (SEdit *)edit;
	line_entry->update(sedit->get_text());
	line_entry->set_text_row(0);
	return 0;
}

int MWindow::paste_subtitle_text(char *text, double start, double end)
{
	gui->lock_window("MWindow::paste_subtitle_text 1");
	undo->update_undo_before();

	Tracks *tracks = edl->tracks;
	for( Track *track=tracks->first; track; track=track->next ) {
		if( track->data_type != TRACK_SUBTITLE ) continue;
		if( !track->record ) continue;
		int64_t start_i = track->to_units(start, 0);
		int64_t end_i = track->to_units(end, 1);
		track->edits->clear(start_i,end_i);
		SEdit *sedit = (SEdit *)track->edits->create_silence(start_i,end_i);
		strcpy(sedit->get_text(),text);
		track->edits->optimize();
	}

	save_backup();
	undo->update_undo_after(_("paste subttl"), LOAD_EDITS | LOAD_PATCHES);

	sync_parameters(CHANGE_EDL);
	restart_brender();
	gui->update(0, 1, 1, 0, 0, 0, 0);
	gui->unlock_window();

	return 0;
}

int SWindowGUI::paste_text(const char *text, double start, double end)
{
	char stext[BCTEXTLEN], *cp = stext;
	if( text ) { // remap charset, reformat if needed
		for( const char *bp=text; *bp!=0; ++bp,++cp ) {
			switch( *bp ) {
			case '\n':  *cp = '|';  break;
			default:    *cp = *bp;  break;
			}
		}
	}
	if( cp > stext && *(cp-1) == '|' ) --cp;
	*cp = 0;
	return swindow->mwindow->paste_subtitle_text(stext, start, end);
}

int SWindowGUI::paste_selection()
{
	EDL *edl = swindow->mwindow->edl;
	LocalSession *lsn = edl->local_session;
	double start = lsn->get_selectionstart();
        double end = lsn->get_selectionend();
	if( start >= end ) return 1;
	if( lsn->inpoint_valid() && lsn->outpoint_valid() ) {
		lsn->set_inpoint(lsn->get_outpoint());
		lsn->unset_outpoint();
	}
	paste_text(line_entry->get_text(), start, end);
	load_next_selection();
	return 0;
}

int SWindowGUI::clear_selection()
{
	EDL *edl = swindow->mwindow->edl;
	double start = edl->local_session->get_selectionstart();
        double end = edl->local_session->get_selectionend();
	if( end > start )
		paste_text(0, start, end);
	return 0;
}


ScriptPrev::ScriptPrev(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Prev"))
{
	sw_gui = gui;
}

ScriptPrev::~ScriptPrev()
{
}

int ScriptPrev::handle_event()
{
	sw_gui->load_prev_selection();
	return 1;
}

ScriptNext::ScriptNext(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Next"))
{
	sw_gui = gui;
}

ScriptNext::~ScriptNext()
{
}

int ScriptNext::handle_event()
{
	sw_gui->load_next_selection();
	return 1;
}

ScriptPaste::ScriptPaste(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Paste"))
{
	sw_gui = gui;
}

ScriptPaste::~ScriptPaste()
{
}

int ScriptPaste::handle_event()
{
	sw_gui->paste_selection();
	return 1;
}

ScriptClear::ScriptClear(SWindowGUI *gui, int x, int y)
 : BC_GenericButton(x, y, _("Clear"))
{
	sw_gui = gui;
}

ScriptClear::~ScriptClear()
{
}

int ScriptClear::handle_event()
{
	sw_gui->clear_selection();
	return 1;
}


ScriptLines::ScriptLines()
{
	used = 0;
	lines = 0;
	text = new char[allocated = 256];
	*text = 0;
}

ScriptLines::~ScriptLines()
{
	delete [] text;
}

void ScriptLines::append(char *cp)
{
	int len = strlen(cp);
	if( allocated-used < len+1 ) {
		int isz = allocated + used + len;
		char *new_text = new char[isz];
		allocated = isz;
		memcpy(new_text, text, used);
		delete [] text;  text = new_text;
	}
	memcpy(text+used, cp, len);
	text[used += len] = 0;
	++lines;
}

int ScriptLines::break_lines()
{
	int line_limit = 60;
	int limit2 = line_limit/2;
	int limit4 = line_limit/4-2;
	char *cp = text, *dp = cp+used;
	int n;  char *bp, *ep, *pp, *sp;
	for( lines=0; cp<dp; ++lines ) {
		// find end of line/buffer
		for( ep=cp; ep<dp && *ep!='\n'; ++ep );
		// delete trailing spaces
		for( sp=ep; sp>=cp && (!*sp || isspace(*sp)); --sp);
		++sp;
		if( (n=ep-sp) > 0 ) {
			memmove(sp,ep,dp+1-ep);
			used -= n;  dp -= n;  ep -= n;
		}
		++ep;
		// skip, if comment or title line
		if( *cp == '*' || *cp == '=' ) {
			cp = ep;  continue;
		}
		// delete leading spaces
		for( sp=cp; sp<ep && isspace(*sp); ++sp);
		if( (n=sp-cp) > 0 ) {
			memmove(cp,sp,dp+1-sp);
			used -= n;  dp -= n;  ep -= n;
		}
		// target about half remaining line, constrain line_limit
		if( (n=(ep-1-cp)/2) < limit2 || n > line_limit )
			n = line_limit;
		// search for last punct, last space before line_limit
		for( bp=cp, pp=sp=0; --n>=0 && cp<ep; ++cp ) {
			if( ispunct(*cp) && isspace(*(cp+1)) ) pp = cp;
			else if( isspace(*cp) ) sp = cp;
		}
		// line not empty
		if( cp < ep ) {
			// first, after punctuation
			if( pp && pp-bp >= limit4 )
				cp = pp+1;
			// then, on spaces
			else if( sp ) {
				cp = sp;
			}
			// last, on next space
			else {
				while( cp<dp && !isspace(*cp) ) ++cp;
			}
			// add new line
			if( !*cp ) break;
			*cp++ = '\n';
		}
	}
	return lines;
}

int ScriptLines::get_text_rows()
{
	int ret = 1, i=used;
	for( char *cp=text; --i>=0; ++cp )
		if( *cp == '\n' ) ++ret;
	return ret;
}

char *ScriptLines::get_text_row(int n)
{
	char *cp = text;
	if( !n ) return cp;
	for( int i=used; --i>=0; ) {
		if( *cp++ != '\n' ) continue;
		if( --n <= 0 ) return cp;
	}
	return 0;
}

ScriptScroll::ScriptScroll(SWindowGUI *gui, int x, int y, int w)
 : BC_ScrollBar(x, y, SCROLL_HORIZ + SCROLL_STRETCH, w, 0, 0, 0)
{
	this->sw_gui = gui;
}

ScriptScroll::~ScriptScroll()
{
}

int ScriptScroll::handle_event()
{
	int64_t pos = get_value();
	sw_gui->set_script_pos(pos);
	sw_gui->script_position->update(pos);
	return 1;
}


ScriptPosition::ScriptPosition(SWindowGUI *gui, int x, int y, int w, int v, int mn, int mx)
 : BC_TumbleTextBox(gui, v, mn, mx, x, y, w-BC_Tumbler::calculate_w())
{
	this->sw_gui = gui;
}

ScriptPosition::~ScriptPosition()
{
}

int ScriptPosition::handle_event()
{
	int64_t pos = atol(get_text());
	sw_gui->set_script_pos(pos);
	sw_gui->script_scroll->update_value(pos);
	return 1;
}


ScriptEntry::ScriptEntry(SWindowGUI *gui, int x, int y, int w, int rows, char *text)
 : BC_ScrollTextBox(gui, x, y, w, rows, text, -strlen(text))
{
	this->sw_gui = gui;
	this->ttext = text;
}

ScriptEntry::ScriptEntry(SWindowGUI *gui, int x, int y, int w, int rows)
 : BC_ScrollTextBox(gui, x, y, w, rows,(char*)0, BCTEXTLEN)
{
	this->sw_gui = gui;
	this->ttext = 0;
}

ScriptEntry::~ScriptEntry()
{
}

void ScriptEntry::set_text(char *text, int isz)
{
	if( !text || !*text ) return;
	if( isz < 0 ) isz = strlen(text)+1;
	BC_ScrollTextBox::set_text(text, isz);
	ttext = text;
}

int ScriptEntry::handle_event()
{
	if( ttext && sw_gui->get_button_down() &&
	    sw_gui->get_buttonpress() == 1 &&
	    sw_gui->get_triple_click() ) {
		int ibeam = get_ibeam_letter(), row = 0;
		const char *tp = ttext;
		while( *tp && tp-ttext < ibeam ) {
			if( *tp++ == '\n' ) ++row;
		}
		int pos = sw_gui->script_entry_no;
		sw_gui->load_selection(pos, row);
	}
	return 1;
}

int SWindowGUI::load_script_line(FILE *fp)
{
	char line[8192];
	for(;;) { // skip blank lines
		char *cp = fgets(line,sizeof(line),fp);
		if( !cp ) return 1;
		++script_line_no;
		while( *cp && isspace(*cp) ) ++cp;
		if( *cp ) break;
	}

	ScriptLines *entry = new ScriptLines();
	script.append(entry);

	for(;;) { // load non-blank lines
		//int len = strlen(line);
		//if( line[len-1] == '\n' ) line[len-1] = ' ';
		entry->append(line);
		char *cp = fgets(line,sizeof(line),fp);
		if( !cp ) break;
		++script_line_no;
		while( *cp && isspace(*cp) ) ++cp;
		if( !*cp ) break;
	}
	script_text_lines += entry->break_lines();
	return 0;
}

void SWindowGUI::load_script()
{
	FILE *fp = fopen(script_path,"r");
	if( !fp ) {
		char string[BCTEXTLEN];
		sprintf(string,_("cannot open: \"%s\"\n%s"),script_path,strerror(errno));
		MainError::show_error(string);
		return;
	}
	load_script(fp);
	script_text_no = -1;
	load_selection(script_entry_no=0, 0);
}

void SWindowGUI::load_script(FILE *fp)
{
	script.remove_all_objects();
	script_line_no = 0;
	script_text_lines = 0;
	while( !load_script_line(fp) );
	char value[64];
	sprintf(value,"%ld",ftell(fp));
	script_filesz->update(value);
	sprintf(value,"%jd",script_line_no);
	script_lines->update(value);
	sprintf(value,"%d",script.size());
	script_entries->update(value);
	sprintf(value,"%jd",script_text_lines);
	script_texts->update(value);
	int hw = script_scroll->get_h();
	script_scroll->update_length(script.size(), script_entry_no, hw, 0);
	script_position->update(script_entry_no);
	script_position->set_boundaries((int64_t)0, (int64_t)script.size()-1);
	fclose(fp);
}

void SWindowGUI::save_spumux_data()
{
	char filename[BCTEXTLEN], track_title[BCTEXTLEN];
	snprintf(filename,sizeof(filename),"%s",script_path);
	filename[sizeof(filename)-1] = 0;
	char *bp = strrchr(filename,'/');
	if( !bp ) bp = filename; else ++bp;
	char *ext = strrchr(bp, '.');
	if( !ext ) ext = bp + strlen(bp);
	int len = sizeof(filename)-1 - (ext-filename);

	Tracks *tracks = swindow->mwindow->edl->tracks;
	for( Track *track=tracks->first; track; track=track->next ) {
		if( track->data_type != TRACK_SUBTITLE ) continue;
		if( !track->record ) continue;
		char *cp = track_title, *ep = cp+sizeof(track_title)-6;
		for( const char *bp=track->title; cp<ep && *bp!=0; ) {
			int b = butf8(bp), c = !iswalnum(b) ? '_' : b;
			butf8(c, cp);
		}
		*cp = 0;
		snprintf(ext,len,"-%s.udvd",track_title);
		FILE *fp = fopen(filename, "w");
		if( !fp ) {
			eprintf(_("Unable to open %s:\n%m"), filename);
			continue;
		}
		int64_t start = 0;
		for( Edit *edit=track->edits->first; edit; edit=edit->next ) {
			SEdit *sedit = (SEdit *)edit;
			if( sedit->length > 0 ) {
				int64_t end = start + sedit->length;
				char *text = sedit->get_text();
				if( *text ) {
					fprintf(fp, "{%jd}{%jd}%s\n", start, end-1, text);
				}
				start = end;
			}
		}
		fclose(fp);
	}
}



SWindow::SWindow(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;
	window_lock = new Mutex("SWindow::window_lock");
	swin_lock = new Condition(0,"SWindow::swin_lock");
	gui = 0;
	done = 1;
	gui_done = 1;

	start();
}

SWindow::~SWindow()
{
	stop();
	delete gui;
	delete swin_lock;
	delete window_lock;
}


void SWindow::start()
{
	if( !Thread::running() ) {
		done = 0;
		Thread::start();
	}
}

void SWindow::stop()
{
	if( Thread::running() ) {
		done = 1;
		swin_lock->unlock();
		window_lock->lock("SWindow::stop");
		if( gui ) gui->stop(1);
		window_lock->unlock();
		Thread::cancel();
	}
	Thread::join();
}

void SWindow::run()
{
	int root_w = mwindow->gui->get_root_w(1);
	int root_h = mwindow->gui->get_root_h(1);

	while( !done ) {
		swin_lock->reset();
		swin_lock->lock();
		if( done ) break;
		int x = mwindow->session->swindow_x;
		int y = mwindow->session->swindow_y;
		int w = mwindow->session->swindow_w;
		int h = mwindow->session->swindow_h;
		if( w < 600 ) w = 600;
		if( h < 500 ) h = 500;
		int scr_x = mwindow->gui->get_screen_x(1, -1);
		int scr_w = mwindow->gui->get_screen_w(1, -1);
		if( x < scr_x ) x = scr_x;
		if( x > scr_x+scr_w ) x = scr_x+scr_w;
		if( x+w > root_w ) x = root_w - w;
		if( x < 0 ) { x = 0;  w = scr_w; }
		if( y+h > root_h ) y = root_h - h;
		if( y < 0 ) { y = 0;  h = root_h; }
		if( y+h > root_h ) h = root_h-y;
		mwindow->session->swindow_x = x;
		mwindow->session->swindow_y = y;
		mwindow->session->swindow_w = w;
		mwindow->session->swindow_h = h;

		gui_done = 0;
		gui = new SWindowGUI(this, x, y, w, h);
		gui->lock_window("ChannelInfo::gui_create_objects");
		gui->load_defaults();
		gui->create_objects();
		gui->set_icon(mwindow->theme->get_image("record_icon"));
		gui->reposition_window(x, y);
		gui->resize_event(w, h);
		gui->load();
		gui->show_window();
		gui->unlock_window();

		int result = gui->run_window();
		if( !result ) {
			gui->save_spumux_data();
		}

		window_lock->lock("ChannelInfo::run 1");
		gui->save_defaults();
		delete gui;  gui = 0;
		window_lock->unlock();
	}
}




void SWindow::run_swin()
{
	window_lock->lock("SWindow::run_swin");
	if( gui ) {
		gui->lock_window("SWindow::run_swin");
		gui->raise_window();
		gui->unlock_window();
	}
	else
		swin_lock->unlock();
	window_lock->unlock();
}

void SWindow::paste_subttl()
{
	window_lock->lock("SWindow::paste_subttl 1");
	if( gui ) {
		gui->lock_window("SWindow::paste_subttl 2");
		gui->paste_selection();
		gui->unlock_window();
	}
	window_lock->unlock();
}

int SWindow::update_selection()
{
	window_lock->lock("SWindow::update_selection 1");
	if( gui ) {
		gui->lock_window("SWindow::update_selection 2");
		gui->update_selection();
		gui->unlock_window();
	}
	window_lock->unlock();
	return 0;
}



SubttlSWin::SubttlSWin(MWindow *mwindow)
 : BC_MenuItem(_("SubTitle..."), _("Alt-y"), 'y')
{
	set_alt();
	this->mwindow = mwindow;
}

SubttlSWin::~SubttlSWin()
{
}

int SubttlSWin::handle_event()
{
	mwindow->gui->swindow->run_swin();
	return 1;
};

