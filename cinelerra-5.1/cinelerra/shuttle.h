#ifndef __SHUTTLE_H__
#define __SHUTTLE_H__

#include "arraylist.h"
#include "bcwindowbase.inc"
#include "linklist.h"
#include "shuttle.inc"
#include "thread.h"

#include <linux/input.h>
#include <sys/types.h>
#include <regex.h>

// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// reworked 2019 for cinelerra-gg by William Morrow (aka goodguy)

// delay in ms before processing each XTest event
// CurrentTime means no delay
#define DELAY CurrentTime
// playback max speed -64x .. 64x
#define SHUTTLE_MAX_SPEED 64.

// protocol for events from the shuttlepro HUD device
//
// ev.type values:
#define EVENT_TYPE_DONE 0
#define EVENT_TYPE_KEY 1
#define EVENT_TYPE_JOGSHUTTLE 2
#define EVENT_TYPE_ACTIVE_KEY 4

// ev.code when ev.type == KEY
#define EVENT_CODE_KEY1 256
// KEY2 257, etc...

enum { K1=0,K2,K3,K4,K5,K6,K7,K8,K9,K10,K11,K12,K13,K14,K15, };
enum { S_7=-7,S_6,S_5,S_4,S_3,S_2,S_1,S0,S1,S2,S3,S4,S5,S6,S7, };
enum { JL=0,JR };

// ev.value when ev.type == KEY
// 1 -> PRESS; 0 -> RELEASE

// ev.code when ev.type == JOGSHUTTLE
#define EVENT_CODE_JOG 7
#define EVENT_CODE_SHUTTLE 8

// ev.value when ev.code == JOG
// 8 bit value changing by one for each jog step

// ev.value when ev.code == SHUTTLE
// -7 .. 7 encoding shuttle position

// we define these as extra KeySyms to represent mouse events
#define XK_Button_0 0x2000000 // just an offset, not a real button
#define XK_Button_1 0x2000001
#define XK_Button_2 0x2000002
#define XK_Button_3 0x2000003
#define XK_Scroll_Up 0x2000004
#define XK_Scroll_Down 0x2000005

#define PRESS 1
#define RELEASE 2
#define PRESS_RELEASE 3
#define HOLD 4

#define NUM_KEYS 15
#define NUM_SHUTTLES 15
#define NUM_JOGS 2

#define KJS_KEY_DOWN 1
#define KJS_KEY_UP 2
#define KJS_SHUTTLE 3
#define KJS_JOG 4

// cinelerra window input targets
#define FOCUS_DEFAULT 0
#define FOCUS_MWINDOW 1
#define FOCUS_AWINDOW 2
#define FOCUS_CWINDOW 3
#define FOCUS_VIEWER  4
#define FOCUS_LOAD    5

class KeySymMapping {
public:
	static KeySym to_keysym(const char *str);
	static const char *to_string(KeySym ks);
	static KeySymMapping key_sym_mapping[];
	const char *str;
	KeySym sym;
};

class Stroke : public ListItem<Stroke>
{
public:
	KeySym keysym;
	int press; // 1:press, 0:release
};

class Strokes : public List<Stroke>
{
public:
	Strokes() {}
	~Strokes() {}
	void clear() { while( last ) delete last; }
	void add_stroke(KeySym keysym, int press=1) {
		Stroke *s = append();
		s->keysym = keysym; s->press = press;
	}
};

class Modifiers : public ArrayList<Stroke>
{
	Translation *trans;
public:
	Modifiers(Translation *trans) { this->trans = trans; }
	~Modifiers() {}

	void mark_as_down(KeySym sym, int hold);
	void mark_as_up(KeySym sym);
	void release(int allkeys);
	void re_press();
};

class TransName
{
public:
	int cin, err;
	const char *name;
	regex_t regex;

	TransName(int cin, const char *nm, const char *re);
	~TransName();
};

class TransNames : public ArrayList<TransName *>
{
public:
	TransNames() {}
	~TransNames() { remove_all_objects(); }
};

class Translation : public ListItem<Translation>
{
public:
	Translation(Shuttle *shuttle);
	Translation(Shuttle *shuttle, const char *name);
	~Translation();
	void init(int def);
	void clear();
	void append_stroke(KeySym sym, int press);
	void add_release(int all_keys);
	void add_keystroke(const char *keySymName, int press_release);
	void add_keysym(KeySym sym, int press_release);
	void add_string(const char *str);
	int start_line(const char *key);
	void print_strokes(const char *name, const char *up_dn, Strokes *strokes);
	void print_stroke(Stroke *s);
	void finish_line();
	void print_line(const char *key);

	Shuttle *shuttle;
	TransNames names;
	const char *name;
	int is_default, is_key;
	int first_release_stroke;
	Strokes *pressed, *released;
	Strokes *pressed_strokes, *released_strokes;
	KeySym keysym_down;

	Strokes key_down[NUM_KEYS];
	Strokes key_up[NUM_KEYS];
	Strokes shuttles[NUM_SHUTTLES];
	Strokes jog[NUM_JOGS];
	Modifiers modifiers;
};

class Translations : public List<Translation>
{
public:
	Translations() {}
	~Translations() {}
	void clear() { while( last ) delete last; }
};

class Shuttle : public Thread
{
	int fd;
	unsigned short jogvalue;
	int shuttlevalue;
	struct timeval last_shuttle;
	int need_synthetic_shuttle;
	const char *dev_name;
	Translation *default_translation;
	Translations translations;
public:
	Shuttle(MWindow *mwindow);
	~Shuttle();

	int send_button(unsigned int button, int press);
	int send_keycode(unsigned int keycode, int press, int send);
	int send_keysym(KeySym keysym, int press);
	void send_stroke_sequence(int kjs, int index);
	void key(unsigned short code, unsigned int value);
	void shuttle(int value);
	void jog(unsigned int value);
	void jogshuttle(unsigned short code, unsigned int value);
	void start(const char *dev_name);
	void stop();
	void handle_event();
	int get_focused_window_translation();
	static const char *probe();
	void run();
	int read_config_file();
	static BC_WindowBase *owns(BC_WindowBase *wdw, Window win);

	int done;
	int failed;
	int first_time;
	int debug;

	MWindow *mwindow;
	Translation *tr, *last_translation;
	BC_WindowBase *wdw;
	Window win;
	unsigned msk;
	int rx, ry, wx, wy;
	Window last_focused;

	const char *config_path;
	time_t config_mtime;
	input_event ev;
};

#endif
