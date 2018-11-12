
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

#ifndef TITLEWINDOW_H
#define TITLEWINDOW_H

#include "guicast.h"

class TitleThread;
class TitleWindow;
class TitleInterlace;

#include "colorpicker.h"
#include "dragcheckbox.h"
#include "filexml.h"
#include "mutex.h"
#include "titler.h"

#include <stdarg.h>

class TitleFontTumble;
class TitleSizeTumble;
class TitleItalic;
class TitleAlias;
class TitleBold;
class TitleDrag;
class TitleSize;
class TitlePitch;
class TitleEncoding;
class TitleColorButton;
class TitleOutlineColorButton;
class TitleDropShadow;
class TitleMotion;
class TitleLoop;
class TitleLinePitch;
class TitleFade;
class TitleFont;
class TitleText;
class TitleTextChars;
class TitleX;
class TitleY;
class TitleW;
class TitleH;
class TitleLeft;
class TitleCenter;
class TitleRight;class TitleTop;
class TitleMid;
class TitleBottom;
class TitleColorThread;
class TitleSpeed;
class TitleTimecode;
class TitleTimecodeFormat;
class TitleOutline;
class TitleStroker;
class TitleBackground;
class TitleBackgroundPath;
class TitleLoopPlayback;
class TitleCurPopup;
class TitleCurItem;
class TitleCurSubMenu;
class TitleCurSubMenuItem;
class TitleFontsPopup;
class TitleColorPopup;
class TitlePngPopup;

class TitleWindow : public PluginClientWindow
{
public:
	TitleWindow(TitleMain *client);
	~TitleWindow();

	void create_objects();
	int resize_event(int w, int h);
	void update_drag();
	void update_color();
	void update_justification();
	void update_stats();
	void update();
	void previous_font();
	void next_font();
	void check_style(const char *font_name, int update);
	int insert_ibeam(const char *txt, int ofs=0);
	void done_event(int result);
	void send_configure_change();
	int check_configure_change(int ret);

	TitleMain *client;

	BC_Title *font_title;
	TitleFont *font;
	TitleFontTumble *font_tumbler;
	BC_Title *x_title;
	TitleX *title_x;
	BC_Title *y_title;
	TitleY *title_y;
	BC_Title *w_title;
	TitleW *title_w;
	BC_Title *h_title;
	TitleH *title_h;
	BC_Title *dropshadow_title;
	TitleDropShadow *dropshadow;
	BC_Title *outline_title;
	TitleOutline *outline;
	BC_Title *stroker_title;
	TitleStroker *stroker;
	BC_Title *style_title;
	TitleItalic *italic;
	TitleAlias *alias;
	TitleBold *bold;
	TitleDrag *drag;
	TitleCurPopup *cur_popup;
	TitleFontsPopup *fonts_popup;
	TitleColorPopup *color_popup;
	TitlePngPopup *png_popup;

	int color_x, color_y;
	int outline_color_x, outline_color_y;
	int drag_dx, drag_dy, dragging;
	int cur_ibeam, pending_config;

	BC_Title *size_title;
	TitleSize *size;
	TitleSizeTumble *size_tumbler;
	BC_Title *pitch_title;
	TitlePitch *pitch;
	BC_Title *encoding_title;
	TitleEncoding *encoding;
	TitleColorButton *color_button;
	TitleColorThread *color_thread;
	TitleOutlineColorButton *outline_color_button;
	TitleColorThread *outline_color_thread;
	BC_Title *motion_title;
	TitleMotion *motion;
	TitleLinePitch *line_pitch;
	TitleLoop *loop;
	BC_Title *fadein_title;
	TitleFade *fade_in;
	BC_Title *fadeout_title;
	TitleFade *fade_out;
	BC_Title *text_title;
	TitleText *text;
	TitleTextChars *text_chars;
	BC_Title *justify_title;
	TitleLeft *left;
	TitleCenter *center;
	TitleRight *right;
	TitleTop *top;
	TitleMid *mid;
	TitleBottom *bottom;
	BC_Title *speed_title;
	TitleSpeed *speed;
	TitleTimecode *timecode;
	TitleTimecodeFormat *timecode_format;
	TitleBackground *background;
	TitleBackgroundPath *background_path;
	BrowseButton *background_browse;
	TitleLoopPlayback *loop_playback;

// Color preview
	ArrayList<BC_ListBoxItem*> sizes;
	ArrayList<BC_ListBoxItem*> encodings;
	ArrayList<BC_ListBoxItem*> paths;
	ArrayList<BC_ListBoxItem*> fonts;
};


class TitleFontTumble : public BC_Tumbler
{
public:
	TitleFontTumble(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_up_event();
	int handle_down_event();

	TitleMain *client;
	TitleWindow *window;
};


class TitleSizeTumble : public BC_Tumbler
{
public:
	TitleSizeTumble(TitleMain *client, TitleWindow *window, int x, int y);

	int handle_up_event();
	int handle_down_event();

	TitleMain *client;
	TitleWindow *window;
};


class TitleAlias : public BC_CheckBox
{
public:
	TitleAlias(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleItalic : public BC_CheckBox
{
public:
	TitleItalic(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBold : public BC_CheckBox
{
public:
	TitleBold(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleDrag : public DragCheckBox
{
public:
	TitleDrag(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	void update_gui();
	Track *get_drag_track();
	int64_t get_drag_position();

	TitleMain *client;
	TitleWindow *window;
};


class TitleSize : public BC_PopupTextBox
{
public:
	TitleSize(TitleMain *client, TitleWindow *window, int x, int y, char *text);
	~TitleSize();
	int handle_event();
	void update(int size);
	TitleMain *client;
	TitleWindow *window;
};

class TitlePitch : public BC_TumbleTextBox
{
public:
	TitlePitch(TitleMain *client, TitleWindow *window, int x, int y, int *value);
	~TitlePitch();
	int handle_event();

	int *value;
	TitleMain *client;
	TitleWindow *window;
};

class TitleEncoding : public BC_PopupTextBox
{
public:
	TitleEncoding(TitleMain *client, TitleWindow *window, int x, int y, char *text);
	~TitleEncoding();
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleColorButton : public BC_GenericButton
{
public:
	TitleColorButton(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleOutlineColorButton : public BC_GenericButton
{
public:
	TitleOutlineColorButton(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleMotion : public BC_PopupTextBox
{
public:
	TitleMotion(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleLoop : public BC_CheckBox
{
public:
	TitleLoop(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleLinePitch : public BC_CheckBox
{
public:
	TitleLinePitch(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTimecode : public BC_CheckBox
{
public:
	TitleTimecode(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTimecodeFormat : public BC_PopupMenu
{
public:
	TitleTimecodeFormat(TitleMain *client, TitleWindow *window, int x, int y, const char *text);
	void create_objects();
	int update(int timecode_format);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleFade : public BC_TextBox
{
public:
	TitleFade(TitleMain *client, TitleWindow *window, double *value, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
	double *value;
};
class TitleFont : public BC_PopupTextBox
{
public:
	TitleFont(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleText : public BC_ScrollTextBox
{
public:
	TitleText(TitleMain *client,
		TitleWindow *window,
		int x,
		int y,
		int w,
		int h);
	int handle_event();
	int button_press_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleTextChars : public BC_Title
{
public:
	int update(int n);
	TitleTextChars(int x, int y, int w);
	~TitleTextChars();
};

class TitleX : public BC_TumbleTextBox
{
public:
	TitleX(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleY : public BC_TumbleTextBox
{
public:
	TitleY(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleW : public BC_TumbleTextBox
{
public:
	TitleW(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleH : public BC_TumbleTextBox
{
public:
	TitleH(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleDropShadow : public BC_TumbleTextBox
{
public:
	TitleDropShadow(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleOutline : public BC_TumbleTextBox
{
public:
	TitleOutline(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleStroker : public BC_TumbleTextBox
{
public:
	TitleStroker(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleSpeed : public BC_TumbleTextBox
{
public:
	TitleSpeed(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleLeft : public BC_Radial
{
public:
	TitleLeft(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleCenter : public BC_Radial
{
public:
	TitleCenter(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleRight : public BC_Radial
{
public:
	TitleRight(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};

class TitleTop : public BC_Radial
{
public:
	TitleTop(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleMid : public BC_Radial
{
public:
	TitleMid(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBottom : public BC_Radial
{
public:
	TitleBottom(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleColorThread : public ColorPicker
{
public:
	TitleColorThread(TitleMain *client, TitleWindow *window, int is_outline);
	virtual int handle_new_color(int output, int alpha);
	void handle_done_event(int result);
	TitleMain *client;
	TitleWindow *window;
	int is_outline;
};
class TitleBackground : public BC_CheckBox
{
public:
	TitleBackground(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleBackgroundPath : public BC_TextBox
{
public:
	TitleBackgroundPath(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};
class TitleLoopPlayback : public BC_CheckBox
{
public:
	TitleLoopPlayback(TitleMain *client, TitleWindow *window, int x, int y);
	int handle_event();
	TitleMain *client;
	TitleWindow *window;
};


class TitleCurPopup : public BC_PopupMenu
{
public:
        TitleCurPopup(TitleMain *client, TitleWindow *window);

        int handle_event();
        void create_objects();

	TitleMain *client;
	TitleWindow *window;
};

class TitleCurItem : public BC_MenuItem
{
public:
        TitleCurItem(TitleCurPopup *popup, const char *text);

        int handle_event();
        TitleCurPopup *popup;
};

enum { POPUP_OFFSET=-1, POPUP_NONE=0, POPUP_FONT, POPUP_COLOR, POPUP_PNG, };

class TitleCurSubMenu : public BC_SubMenu
{
public:
        TitleCurSubMenu(TitleCurItem *cur_item);
        ~TitleCurSubMenu();
	void add_subitemx(int popup_type, va_list ap, const char *fmt);
	void add_subitem(int popup_type, const char *fmt,...) { va_list ap;
		va_start(ap, fmt); add_subitemx(popup_type, ap, fmt); va_end(ap);
	}
	void add_subitem(const char *fmt,...) { va_list ap;
		va_start(ap, fmt); add_subitemx(POPUP_NONE, ap, fmt); va_end(ap);
	}
        TitleCurItem *cur_item;
};

class TitleCurSubMenuItem : public BC_MenuItem
{
public:
        TitleCurSubMenuItem(TitleCurSubMenu *submenu, const char *text, int popup_type);
        ~TitleCurSubMenuItem();
        int handle_event();

        TitleCurSubMenu *submenu;
	int popup_type;
};

class TitleFontsPopup : public BC_ListBox
{
public:
	TitleFontsPopup(TitleMain *client, TitleWindow *window);
	~TitleFontsPopup();
	int keypress_event();
	int handle_event();
	int show_tooltip(const char *text, int x,int y, int w,int h) {
		return gui_tooltip(text);
	}

	TitleMain *client;
	TitleWindow *window;
};

class TitleColorPopup : public ColorPicker
{
public:
	TitleColorPopup(TitleMain *client, TitleWindow *window);
	~TitleColorPopup();
	int handle_new_color(int output, int alpha);
	void handle_done_event(int result);
	int activate();

	TitleMain *client;
	TitleWindow *window;
	int color_value;
};

class TitlePngPopup : public BC_DialogThread
{
public:
	TitlePngPopup(TitleMain *client, TitleWindow *window);
	~TitlePngPopup();

	void handle_done_event(int result);
	BC_Window* new_gui();
	int activate();

	TitleMain *client;
	TitleWindow *window;
};

#endif
