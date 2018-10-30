
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

#ifndef BCRESOURCES_H
#define BCRESOURCES_H



// Global objects for the user interface




#include "bcdisplayinfo.inc"
#include "bcfilebox.h"
#include "bcfontentry.inc"
#include "bcresources.inc"
#include "bcsignals.inc"
#include "bcsynchronous.inc"
#include "bcwindowbase.inc"
#include "vframe.inc"

#include <X11/Xlib.h>

typedef struct
{
	const char *suffix;
	int icon_type;
} suffix_to_type_t;

typedef struct
{
	char path[BCTEXTLEN];
	int id;
} filebox_history_t;

class BC_Resources
{
public:
	BC_Resources(); // The window parameter is used to get the display information initially
	~BC_Resources();

        friend class BC_WindowBase;

	int initialize_display(BC_WindowBase *window);

// Get unique ID
	int get_id();
// Get ID for filebox history.  Persistent.
	int get_filebox_id();
	int get_bg_color();          // window backgrounds
	int get_bg_shadow1();        // border for windows
	int get_bg_shadow2();
	int get_bg_light1();
	int get_bg_light2();
// Get synchronous thread for OpenGL
	BC_Synchronous* get_synchronous();
// Called by user after synchronous thread is created.
	void set_synchronous(BC_Synchronous *synchronous);
// Set signal handler
	static void set_signals(BC_Signals *signal_handler);
	static BC_Signals* get_signals();


// These values should be changed before the first window is created.
// colors
	int bg_color;          // window backgrounds
	int border_light1;
	int border_light2;
	int border_shadow1;
	int border_shadow2;

	int bg_shadow1;        // border for windows
	int bg_shadow2;
	int bg_light1;
	int bg_light2;
	int default_text_color;
	int disabled_text_color;


// beveled box colors
	int button_light;
	int button_highlighted;
	int button_down;
	int button_up;
	int button_shadow;
	int button_uphighlighted;

// highlighting
	int highlight_inverse;

// 3D box colors for menus
	int menu_light;
	int menu_highlighted;
	int menu_down;
	int menu_up;
	int menu_shadow;
// If these are nonzero, they override the menu backgrounds.
	VFrame *menu_popup_bg;
	VFrame **menu_title_bg;
	VFrame *menu_bar_bg;
	VFrame **popupmenu_images;

// Minimum menu width
	int min_menu_w;
// Menu bar text color
	int menu_title_text;
// color of popup title
	int popup_title_text;
// Right and left margin for text not including triangle space.
	int popupmenu_margin;
// post popup on button release event
	int popupmenu_btnup;
// Right margin for triangle not including text margin.
	int popupmenu_triangle_margin;
// color for item text
	int menu_item_text;
// Override the menu item background if nonzero.
	VFrame **menu_item_bg;


// color for progress text
	int progress_text;
// set focus on enter
	int grab_input_focus;

	int menu_highlighted_fontcolor;

// ms for double click
	long double_click;
// ms for cursor flash
	int blink_rate;
// ms for scroll repeats
	int scroll_repeat;
// ms before tooltip
	int tooltip_delay;
	int tooltip_bg_color;
	int tooltips_enabled;
	int textbox_focus_policy;

	int audiovideo_color;

// default color of text
	int text_default;
// background color of textboxes and list boxes
	int text_border1;
	int text_border2;
	int text_border2_hi;
	int text_background;
	int text_background_disarmed;
	int text_background_hi;
	int text_background_noborder_hi;
	int text_border3;
	int text_border3_hi;
	int text_border4;
	int text_highlight;
	int text_inactive_highlight;
	int text_selected_highlight;
// Not used
	int text_background_noborder;

// Optional background for highlighted text in toggle
	VFrame *toggle_highlight_bg;
	int toggle_text_margin;

// Background images
	static VFrame *bg_image;
	static VFrame *menu_bg;

// default icon
	VFrame *default_icon;
// Buttons
	VFrame **ok_images;
	VFrame **cancel_images;
	VFrame **filebox_text_images;
	VFrame **filebox_icons_images;
	VFrame **filebox_updir_images;
	VFrame **filebox_newfolder_images;
	VFrame **filebox_rename_images;
	VFrame **filebox_descend_images;
	VFrame **filebox_delete_images;
	VFrame **filebox_reload_images;
	VFrame **filebox_szfmt_images;

// Generic button images
	VFrame **generic_button_images;
// Generic button text margin
	int generic_button_margin;
	VFrame **usethis_button_images;

// Toggles
	VFrame **checkbox_images;
	VFrame **radial_images;
	VFrame **label_images;
// menu check
	VFrame *check;

	VFrame **tumble_data;
	int tumble_duration;

// Horizontal bar
	VFrame *bar_data;

// Listbox
	VFrame *listbox_bg;
	VFrame **listbox_button;
	VFrame **listbox_expand;
	VFrame **listbox_column;
	VFrame *listbox_up;
	VFrame *listbox_dn;
	int listbox_title_overlap;
// Margin for titles in addition to listbox border
	int listbox_title_margin;
	int listbox_title_color;
	int listbox_title_hotspot;
	int listbox_border1;
	int listbox_border2_hi;
	int listbox_border2;
	int listbox_border3_hi;
	int listbox_border3;
	int listbox_border4;
// Selected row color
	int listbox_selected;
// Highlighted row color
	int listbox_highlighted;
// Inactive row color
	int listbox_inactive;
// Default text color
	int listbox_text;


// Sliders
	VFrame **horizontal_slider_data;
	VFrame **vertical_slider_data;
	VFrame **hscroll_data;

	VFrame **vscroll_data;
// Minimum pixels in handle
	int scroll_minhandle;

// Pans
	VFrame **pan_data;
	int pan_text_color;

// Pots
	VFrame **pot_images;
	int pot_x1, pot_y1, pot_r;
// Amount of deflection of pot when down
	int pot_offset;
	int pot_needle_color;

// Meters
	VFrame **xmeter_images, **ymeter_images;
	int meter_font;
	int meter_font_color;
	int meter_title_w;
	int meter_3d;

// Progress bar
	VFrame **progress_images;

// Motion required to start a drag
	int drag_radius;

// Filebox
	static suffix_to_type_t suffix_to_type[];
	VFrame **type_to_icon;
// Display mode for fileboxes
	int filebox_mode;
// Filter currently used in filebox
	char filebox_filter[BCTEXTLEN];
// History of submitted files
	filebox_history_t filebox_history[FILEBOX_HISTORY_SIZE];
// filebox size
	int filebox_w;
	int filebox_h;
// Column types for filebox
	int filebox_columntype[FILEBOX_COLUMNS];
	int filebox_columnwidth[FILEBOX_COLUMNS];
	int filebox_sortcolumn;
	int filebox_sortorder;
// Column types for filebox in directory mode
	int dirbox_columntype[FILEBOX_COLUMNS];
	int dirbox_columnwidth[FILEBOX_COLUMNS];
	int dirbox_sortcolumn;
	int dirbox_sortorder;
// Bottom margin between list and window
	int filebox_margin;
	int dirbox_margin;
	int directory_color;
	int file_color;
	double font_scale, icon_scale;
// fonts
	static const char *small_font, *small_font2;
	static const char *medium_font, *medium_font2;
	static const char *large_font, *large_font2;
	static const char *big_font, *big_font2;
	static const char *clock_font, *clock_font2;

	static const char *small_fontset;
	static const char *medium_fontset;
	static const char *large_fontset;
	static const char *big_fontset;
	static const char *clock_fontset;

	static const char *small_font_xft, *small_b_font_xft;
	static const char *medium_font_xft, *medium_b_font_xft;
	static const char *large_font_xft, *large_b_font_xft;
	static const char *big_font_xft, *big_b_font_xft;
	static const char *clock_font_xft, *clock_b_font_xft;

// Backup of fonts in case the first choices don't exist
	static const char *small_font_xft2;
	static const char *medium_font_xft2;
	static const char *large_font_xft2;
	static const char *big_font_xft2;
	static const char *clock_font_xft2;

	void init_font_defs(double scale);
	void finit_font_defs();

	VFrame **medium_7segment;

//clock
	int draw_clock_background;

	int use_fontset;
	int use_xft;

// Current locale uses utf8
	static int locale_utf8;
// Byte order is little_endian
	static int little_endian;
// Language and region
	static char language[LEN_LANG];
	static char region[LEN_LANG];
	static char encoding[LEN_ENCOD];
	static const char *wide_encoding;
	static ArrayList<BC_FontEntry*> *fontlist;
	static int init_fontconfig(const char *search_path);
	static BC_FontEntry *find_fontentry(const char *displayname, int style,
		int mask, int preferred_style = 0);
	static void encode_to_utf8(char *buffer, int buflen);
	static FcPattern* find_similar_font(FT_ULong char_code, FcPattern *oldfont);
	static size_t encode(const char *from_enc, const char *to_enc,
		char *input, int input_length, char *output, int output_length);
	static int find_font_by_char(FT_ULong char_code, char *path_new, const FT_Face oldface);
	static int font_debug;
	static void dump_fonts(FILE *fp = stdout);
	static void dump_font_entry(FILE *fp, const char *cp,  BC_FontEntry *ep);

	static void new_vframes(int n, VFrame *vframes[], ...);
	static void del_vframes(VFrame *vframes[], int n);
// default images
	static VFrame *default_type_to_icon[6];
	static VFrame *default_bar;
	static VFrame *default_cancel_images[3];
	static VFrame *default_ok_images[3];
	static VFrame *default_usethis_images[3];
#if 0
	static VFrame *default_checkbox_images[5];
	static VFrame *default_radial_images[5];
	static VFrame *default_label_images[5];
#endif
	static VFrame *default_menuitem_data[3];
	static VFrame *default_menubar_data[3];
	static VFrame *default_menu_popup_bg;
	static VFrame *default_menu_bar_bg;
	static VFrame *default_check_image;
	static VFrame *default_filebox_text_images[3];
	static VFrame *default_filebox_icons_images[3];
	static VFrame *default_filebox_updir_images[3];
	static VFrame *default_filebox_newfolder_images[3];
	static VFrame *default_filebox_rename_images[3];
	static VFrame *default_filebox_delete_images[3];
	static VFrame *default_filebox_reload_images[3];
	static VFrame *default_filebox_szfmt_images[12];
	static VFrame *default_listbox_button[4];
	static VFrame *default_listbox_bg;
	static VFrame *default_listbox_expand[5];
	static VFrame *default_listbox_column[3];
	static VFrame *default_listbox_up;
	static VFrame *default_listbox_dn;
	static VFrame *default_pot_images[3];
	static VFrame *default_progress_images[2];
	static VFrame *default_medium_7segment[20];
	static VFrame *default_vscroll_data[10];
	static VFrame *default_hscroll_data[10];
	static VFrame *default_icon_img;

// Make VFrame use shm
	int vframe_shm;
	int use_vframe_shm() { return use_shm && vframe_shm ? 1 : 0; }

	static int get_machine_cpus();
	static int machine_cpus;
// Available display extensions
	int use_shm;
	int shm_reply;
	static int error;
// If the program uses recursive_resizing
	int recursive_resizing;
// Work around X server bugs
	int use_xvideo;
// Seems to help if only 1 window is created at a time.
	Mutex *create_window_lock;
// size raw, 1000, 1024, thou
	int filebox_size_format;
private:
// Test for availability of shared memory pixmaps
	void init_shm(BC_WindowBase *window);
	void init_sizes(BC_WindowBase *window);
	static int x_error_handler(Display *display, XErrorEvent *event);
 	VFrame **list_pointers[100];
 	int list_lengths[100];
 	int list_total;
	static const char *fc_properties[];


	Mutex *id_lock;
	static Mutex fontconfig_lock;

// Pointer to signal handler class to run after ipc
	static BC_Signals *signal_handler;
	BC_Synchronous *synchronous;

	int id;
	int filebox_id;
};


#endif
