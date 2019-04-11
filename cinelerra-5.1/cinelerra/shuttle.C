#ifdef HAVE_SHUTTLE
// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// reworked 2019 for cinelerra-gg by William Morrow

#include "arraylist.h"
#include "cstrdup.h"
#include "file.h"
#include "guicast.h"
#include "keys.h"
#include "linklist.h"
#include "loadfile.h"
#include "mainmenu.h"
#include "shuttle.h"
#include "thread.h"

#include "mwindow.h"
#include "mwindowgui.h"
#include "awindow.h"
#include "awindowgui.h"
#include "cwindow.h"
#include "cwindowgui.h"
#include "vwindow.h"
#include "vwindowgui.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <X11/Xlib.h>
#include <X11/keysym.h>

static Time milliTimeClock()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

KeySymMapping KeySymMapping::key_sym_mapping[] = {
// button keycodes
	{ "XK_Button_1", XK_Button_1 },
	{ "XK_Button_2", XK_Button_2 },
	{ "XK_Button_3", XK_Button_3 },
	{ "XK_Scroll_Up", XK_Scroll_Up },
	{ "XK_Scroll_Down", XK_Scroll_Down },
#include "shuttle_keys.h"
	{ NULL, 0 }
};

int KeySymMapping::get_mask(const char *&str)
{
	int mask = 0;
	while( *str ) {
		if( !strncmp("Shift-",str,6) ) {
			mask |= ShiftMask;
			str += 6;  continue;
		}
		if( !strncmp("Ctrl-",str,5) ) {
			mask |= ControlMask;
			str += 5;  continue;
		}
		else if( !strncmp("Alt-",str,4) ) {
			mask |= Mod1Mask;
			str += 4;  continue;
		}
		break;
	}
	return mask;
}

SKeySym KeySymMapping::to_keysym(const char *str)
{
	if( !strncmp("FWD_",str, 4) ) {
		float speed = atof(str+4) / SHUTTLE_MAX_SPEED;
		if( speed > SHUTTLE_MAX_SPEED ) return 0;
		int key_code = (SKEY_MAX+SKEY_MIN)/2. +
			(SKEY_MAX-SKEY_MIN)/2. * speed;
		if( key_code > SKEY_MAX ) key_code = SKEY_MAX;
		return key_code;
	}
	if( !strncmp("REV_",str, 4) ) {
		float speed = atof(str+4) / SHUTTLE_MAX_SPEED;
		if( speed > SHUTTLE_MAX_SPEED ) return 0;
		int key_code = (SKEY_MAX+SKEY_MIN)/2. -
			(SKEY_MAX-SKEY_MIN)/2. * speed;
		if( key_code < SKEY_MIN ) key_code = SKEY_MIN;
		return key_code;
	}
	int mask = get_mask(str);
	for( KeySymMapping *ksp = &key_sym_mapping[0]; ksp->str; ++ksp )
		if( !strcmp(str, ksp->str) )
			return SKeySym(ksp->sym, mask);
	return 0;
}

const char *KeySymMapping::to_string(SKeySym ks)
{
	for( KeySymMapping *ksp = &key_sym_mapping[0]; ksp->sym.key; ++ksp ) {
		if( ksp->sym.key == ks.key ) {
			static char string[BCSTRLEN];
			char *sp = string, *ep = sp+sizeof(string);
			if( ks.msk & Mod1Mask ) sp += snprintf(sp, ep-sp, "Alt-");
			if( ks.msk & ControlMask ) sp += snprintf(sp, ep-sp, "Ctrl-");
			if( ks.msk & ShiftMask ) sp += snprintf(sp, ep-sp, "Shift-");
			snprintf(sp, ep-sp, "%s", ksp->str);
			return string;
		}
	}
	if( ks >= SKEY_MIN && ks <= SKEY_MAX ) {
		double speed = SHUTTLE_MAX_SPEED *
			(ks-(SKEY_MAX+SKEY_MIN)/2.) / ((SKEY_MAX-SKEY_MIN)/2.);
		static char text[BCSTRLEN];
		sprintf(text, "%s_%0.3f", speed>=0 ? "FWD" : "REV", fabs(speed));
		char *bp = strchr(text,'.');
		if( bp ) {
			char *cp = bp+strlen(bp);
			while( --cp>bp && *cp=='0' ) *cp=0;
			if( cp == bp ) *cp = 0;
		}
		return text;
	}
	return 0;
}

TransName::TransName(int cin, const char *nm, const char *re)
{
	this->cin = cin;
	this->name = cstrdup(nm);
}
TransName::~TransName()
{
	delete [] name;
}

void Translation::init(int def)
{
	is_default = def;
	is_key = 0;
	first_release_stroke = 0;
	pressed = 0;
	released = 0;
	keysym_down = 0;
}

Translation::Translation(Shuttle *shuttle)
 : modifiers(this)
{ // initial default translation
	init(1);
	this->shuttle = shuttle;
	this->name = cstrdup("Default");
	names.append(new TransName(FOCUS_DEFAULT, name, ""));
	key_down[K6].add_stroke(XK_Button_1, 1);
	key_up[K6].add_stroke(XK_Button_1, 0);
	key_down[K7].add_stroke(XK_Button_2, 1);
	key_up[K7].add_stroke(XK_Button_2, 0);
	key_down[K8].add_stroke(XK_Button_3, 1);
	key_up[K8].add_stroke(XK_Button_3, 0);
	jog[JL].add_stroke(XK_Scroll_Up, 1);
	jog[JL].add_stroke(XK_Scroll_Up, 0);
	jog[JR].add_stroke(XK_Scroll_Down, 0);
	jog[JR].add_stroke(XK_Scroll_Down, 1);
}

Translation::Translation(Shuttle *shuttle, const char *name)
 : modifiers(this)
{
	init(0);
	this->shuttle = shuttle;
	this->name = cstrdup(name);
}

Translation::~Translation()
{
	delete [] name;
}

void Translation::clear()
{
	names.remove_all_objects();
	init(0);
	for( int i=0; i<NUM_KEYS; ++i ) key_down[i].clear();
	for( int i=0; i<NUM_KEYS; ++i ) key_up[i].clear();
	for( int i=0; i<NUM_SHUTTLES; ++i ) shuttles[i].clear();
	for( int i=0; i<NUM_JOGS; ++i ) jog[i].clear();
}

void Translation::append_stroke(SKeySym sym, int press)
{
	Stroke *s = pressed_strokes->append();
	s->keysym = sym;
	s->press = press;
}

void Translation::add_keysym(SKeySym sym, int press_release)
{
//printf("add_keysym(0x%x, %d)\n", (int)sym, press_release);
	switch( press_release ) {
	case PRESS:
		append_stroke(sym, 1);
		modifiers.mark_as_down(sym, 0);
		break;
	case RELEASE:
		append_stroke(sym, 0);
		modifiers.mark_as_up(sym);
		break;
	case HOLD:
		append_stroke(sym, 1);
		modifiers.mark_as_down(sym, 1);
		break;
	case PRESS_RELEASE:
	default:
		if( first_release_stroke ) {
			modifiers.re_press();
			first_release_stroke = 0;
		}
		if( keysym_down ) {
			append_stroke(keysym_down, 0);
		}
		append_stroke(sym, 1);
		keysym_down = sym;
		break;
	}
}

void Translation::add_release(int all_keys)
{
//printf("add_release(%d)\n", all_keys);
	modifiers.release(all_keys);
	if( !all_keys )
		pressed_strokes = released_strokes;
	if( keysym_down ) {
		append_stroke(keysym_down, 0);
		keysym_down = 0;
	}
	first_release_stroke = 1;
}

void Translation::add_keystroke(const char *keySymName, int press_release)
{
	SKeySym sym;

	if( is_key && !strncmp(keySymName, "RELEASE", 8) ) {
		add_release(0);
		return;
	}
	sym = KeySymMapping::to_keysym(keySymName);
	if( sym != 0 ) {
		add_keysym(sym, press_release);
	}
	else
		fprintf(stderr, "unrecognized KeySym: %s\n", keySymName);
}

void Translation::add_string(char *&str)
{
	int delim = *str++;
	if( !delim ) return;
	while( str && *str && *str!=delim ) {
		if( *str < ' ' || *str > '~' ) continue;
		int mask = KeySymMapping::get_mask((const char *&)str);
		if( str[0] == '\\' && str[1] ) ++str;
		add_keysym(SKeySym(*str++, mask), PRESS_RELEASE);
		mask = 0;
	}
	if( *str == delim ) ++str;
}

int Translation::start_line(const char *key)
{
	pressed_strokes = 0;
	released_strokes = 0;
	pressed = released = 0;
	is_key = 0;
	if( !strcasecmp("JL", key) ) {
		pressed = &jog[0];
	}
	else if( !strcasecmp("JR", key) ) {
		pressed = &jog[1];
	}
	else {
		char c = 0;  int k = -1, n = 0;
		if( sscanf(key, "%c%d%n", &c, &k, &n) != 2 ) return 1;
		switch( c ) {
		case 'K': case 'k':
			--k;
			if( k >= K1 && k <= K15 ) {
				pressed = &key_down[k];
				released = &key_up[k];
				is_key = 1;
			}
			break;
		case 'S': case 's':
			if( k >= S_7 && k <= S7 ) {
				pressed = &shuttles[k-S_7];
			}
			break;
		}
		if( !pressed ) {
			fprintf(stderr, "bad key name: [%s]%s\n", name, key);
			return 1;
		}
		if( pressed->first ) {
			fprintf(stderr, "dupl key name: [%s]%s\n", name, key);
			return 1;
		}
	}
	pressed_strokes = pressed;
	released_strokes = released;
	return 0;
}

void Translation::print_stroke(Stroke *s)
{
	if( !s ) return;
	const char *cp = KeySymMapping::to_string(s->keysym);
	if( !cp ) { printf("0x%x", (int)s->keysym); cp = "???"; }
	printf("%s/%c ", cp, s->press ? 'D' : 'U');
}

void Translation::print_strokes(const char *name, const char *up_dn, Strokes *strokes)
{
	printf("%s[%s]: ", name, up_dn);
	for( Stroke *s=strokes->first; s; s=s->next )
		print_stroke(s);
	printf("\n");
}

void Translation::finish_line()
{
//printf("finish_line()\n");
	if( is_key ) {
		add_release(0);
	}
	add_release(1);
}

void Translation::print_line(const char *key)
{
	if( is_key ) {
		print_strokes(key, "D", pressed);
		print_strokes(key, "U", released);
	}
	else {
		print_strokes(key, "", pressed);
	}
	printf("\n");
}

// press values in Modifiers:
// PRESS -> down
// HOLD -> held
// PRESS_RELEASE -> released, but to be re-pressed if necessary
// RELEASE -> up

void Modifiers::mark_as_down(SKeySym sym, int hold)
{
	Modifiers &modifiers = *this;
	for( int i=0; i<size(); ++i ) {
		Stroke &s = modifiers[i];
		if( s.keysym == sym ) {
			s.press = hold ? HOLD : PRESS;
			return;
		}
	}
	Stroke &s = append();
	s.keysym = sym;
	s.press = hold ? HOLD : PRESS;
}

void Modifiers::mark_as_up(SKeySym sym)
{
	Modifiers &modifiers = *this;
	for( int i=0; i<size(); ++i ) {
		Stroke &s = modifiers[i];
		if( s.keysym == sym ) {
			s.press = RELEASE;
			return;
		}
	}
}

void Modifiers::release(int allkeys)
{
	Modifiers &modifiers = *this;
	for( int i=0; i<size(); ++i ) {
		Stroke &s = modifiers[i];
		if( s.press == PRESS ) {
			trans->append_stroke(s.keysym, 0);
			s.press = PRESS_RELEASE;
		}
		else if( allkeys && s.press == HOLD ) {
			trans->append_stroke(s.keysym, 0);
			s.press = RELEASE;
		}
	}
}

void Modifiers::re_press()
{
	Modifiers &modifiers = *this;
	for( int i=0; i<size(); ++i ) {
		Stroke &s = modifiers[i];
		if( s.press == PRESS_RELEASE ) {
			trans->append_stroke(s.keysym, 1);
			s.press = PRESS;
		}
	}
}


Shuttle::Shuttle(MWindow *mwindow)
 : Thread(1, 0, 0)
{
	this->mwindow = mwindow;

	fd = -1;
	wdw = 0;
	win = 0;
	msk = 0;
	rx = ry = 0;
	wx = wy = 0;
	jogvalue = 0xffff;
	shuttlevalue = 0xffff;
	dev_index = -1;

	done = -1;
	failed = 0;
	first_time = 1;
	tr = 0;
	debug = 0;
	usb_direct = 0;

	last_translation = 0;
	last_focused = 0;

#ifdef HAVE_SHUTTLE_USB
	devsh = 0;
	claimed = -1;
	last_jog = 0;
	last_shuttle = 0;
	last_btns = 0;
#endif

	default_translation = new Translation(this);
	config_path = 0;
	config_mtime = 0;
	ev.type = ~0;
}

Shuttle::~Shuttle()
{
	stop();
	delete default_translation;
	delete [] config_path;
}

int Shuttle::send_button(unsigned int button, int press)
{
	if( debug )
		printf("btn: %u %d\n", button, press);
	XButtonEvent *b = new XButtonEvent();
	memset(b, 0, sizeof(*b));
	b->type = press ? ButtonPress : ButtonRelease;
	b->time = milliTimeClock();
	b->send_event = 1;
	b->display = wdw->top_level->display;
	b->root = wdw->top_level->rootwin;
	b->window = win;
	b->x_root = rx;
	b->y_root = ry;
	b->x = wx;
	b->y = wy;
	b->state = msk;
	b->button = button;
	b->same_screen = 1;
	wdw->top_level->put_event((XEvent *) b);
	return 0;
}
int Shuttle::send_keycode(unsigned key, unsigned msk, int press, int send)
{
	if( debug ) {
		const char *cp = !send ? 0 :
			KeySymMapping::to_string(SKeySym(key, msk));
		if( cp )
			printf("key: %s %d\n", cp, press);
		else
			printf("key: %04x/%04x %d\n", key, msk, press);
	}
	XKeyEvent *k = new XKeyEvent();
	memset(k, 0, sizeof(*k));
	k->type = press ? KeyPress : KeyRelease;
	k->time = milliTimeClock();
	k->send_event = send;
	k->display = wdw->top_level->display;
	k->root = wdw->top_level->rootwin;
	k->window = win;
	k->x_root = rx;
	k->y_root = ry;
	k->x = wx;
	k->y = wy;
	k->state = msk;
	k->keycode = key;
	k->same_screen = 1;
	wdw->top_level->put_event((XEvent *) k);
	return 0;
}

int Shuttle::send_keysym(SKeySym keysym, int press)
{
	return keysym >= XK_Button_1 && keysym <= XK_Scroll_Down ?
		send_button((unsigned int)keysym - XK_Button_0, press) :
		send_keycode(keysym.key, keysym.msk, press, 1);
//	unsigned int keycode = XKeysymToKeycode(wdw->top_level->display, keysym);
//	return send_keycode(keycode, press, 0);
}


static Stroke *fetch_stroke(Translation *translation, int kjs, int index)
{
	Stroke *ret = 0;
	if( translation ) {
		switch( kjs ) {
		default:
		case KJS_KEY_DOWN:  ret = translation->key_down[index].first;     break;
		case KJS_KEY_UP:    ret = translation->key_up[index].first;       break;
		case KJS_JOG:	    ret = translation->jog[index].first;          break;
		case KJS_SHUTTLE:   ret = translation->shuttles[index-S_7].first; break;
		}
	}
	return ret;
}

void Shuttle::send_stroke_sequence(int kjs, int index)
{
	if( !wdw ) return;
	Stroke *s = fetch_stroke(tr, kjs, index);
	if( !s ) s = fetch_stroke(default_translation, kjs, index);
	while( s ) {
		send_keysym(s->keysym, s->press);
		s = s->next;
	}
}

void Shuttle::key(unsigned short code, unsigned int value)
{
	code -= EVENT_CODE_KEY1;
	if( code >= NUM_KEYS ) {
		fprintf(stderr, "key(%d, %d) out of range\n", code + EVENT_CODE_KEY1, value);
		return;
	}
	send_stroke_sequence(value ? KJS_KEY_DOWN : KJS_KEY_UP, code);
}


void Shuttle::shuttle(int value)
{
	if( value < S_7 || value > S7 ) {
		fprintf(stderr, "shuttle(%d) out of range\n", value);
		return;
	}
	if( value != (int)shuttlevalue ) {
		shuttlevalue = value;
		send_stroke_sequence(KJS_SHUTTLE, value);
	}
}

// Due to a bug (?) in the way Linux HID handles the ShuttlePro, the
// center position is not reported for the shuttle wheel.	Instead,
// a jog event is generated immediately when it returns.	We check to
// see if the time since the last shuttle was more than a few ms ago
// and generate a shuttle of 0 if so.
//
// Note, this fails if jogvalue happens to be 0, as we don't see that
// event either!
void Shuttle::jog(unsigned int value)
{
	if( jogvalue != 0xffff ) {
		value = value & 0xff;
		int direction = ((value - jogvalue) & 0x80) ? -1 : 1;
		int index = direction > 0 ? 1 : 0;
		while( jogvalue != value ) {
			// driver fails to send an event when jogvalue == 0
			if( jogvalue != 0 ) {
				send_stroke_sequence(KJS_JOG, index);
			}
			jogvalue = (jogvalue + direction) & 0xff;
		}
	}
	jogvalue = value;
}

void Shuttle::jogshuttle(unsigned short code, unsigned int value)
{
	switch( code ) {
	case EVENT_CODE_JOG:
		jog(value);
		break;
	case EVENT_CODE_SHUTTLE:
		shuttle(value);
		break;
	case EVENT_CODE_HI_JOG:  // redundant report of JOG value*120
		break;
	default:
		fprintf(stderr, "jogshuttle(%d, %d) invalid code\n", code, value);
		break;
	}
}

static const struct shuttle_dev {
	const char *path;
	unsigned vendor, product;
} shuttle_devs[] = {
	{ "/dev/input/by-id/usb-Contour_Design_ShuttleXpress-event-if00",
		0x0b33, 0x0020 },
	{ "/dev/input/by-id/usb-Contour_Design_ShuttlePRO_v2-event-if00",
		0x0b33, 0x0030 },
	{ "/dev/input/by-id/usb-Contour_Design_ShuttlePro-event-if00",
		0x0b33, 0x0030 },
	{ "/dev/input/by-id/usb-Contour_Design_ShuttlePRO_v2-event-joystick",
		0x0b33, 0x0030 },
};

#ifdef HAVE_SHUTTLE_USB
void Shuttle::usb_probe(int idx)
{
	int ret = libusb_init(0);
	if( ret < 0 ) return;
	claimed = 0;
	const struct shuttle_dev *s = &shuttle_devs[idx];
	devsh = libusb_open_device_with_vid_pid(0, s->vendor, s->product);
	if( devsh ) {
		int sh_iface = SHUTTLE_INTERFACE;
		libusb_detach_kernel_driver(devsh, sh_iface);
		ret = libusb_claim_interface(devsh, sh_iface);
		if( ret >= 0 ) claimed = 1;
	}
	if( !claimed )
		usb_done();
}

void Shuttle::usb_done()
{
	if( devsh ) {
		if( claimed > 0 ) {
			int sh_iface = SHUTTLE_INTERFACE;
			libusb_release_interface(devsh, sh_iface);
			libusb_attach_kernel_driver(devsh, sh_iface);
			claimed = 0;
		}
		libusb_close(devsh);
		devsh = 0;
	}
	if( claimed >= 0 ) {
		libusb_exit(0);
		claimed = -1;
	}
}
#endif

int Shuttle::probe()
{
	struct stat st;
	int ret = sizeof(shuttle_devs) / sizeof(shuttle_devs[0]);
	while( --ret >= 0 && stat(shuttle_devs[ret].path , &st) );
	return ret;
}

void Shuttle::start(int idx)
{
	this->dev_index = idx;
	first_time = 1;
	done = 0;
	Thread::start();
}

void Shuttle::stop()
{
	if( running() && !done ) {
		done = 1;
		cancel();
		join();
	}
}

BC_WindowBase *Shuttle::owns(BC_WindowBase *wdw, Window win)
{
	if( wdw->win == win ) return wdw;
	if( (wdw=wdw->top_level)->win == win ) return wdw;
	for( int i=wdw->popups.size(); --i>=0; )
		if( wdw->popups[i]->win == win ) return wdw;
	return 0;
}

int Shuttle::get_focused_window_translation()
{
	MWindowGUI *gui = mwindow->gui;
	Display *dpy = gui->display;
	Window focus = 0;
	int ret = 0, revert = 0;
	char win_title[BCTEXTLEN];  win_title[0] = 0;
	gui->lock_window("Shuttle::get_focused_window_translation");
	XGetInputFocus(dpy, &focus, &revert);
	if( last_focused != focus ) {
		last_focused = focus;
		Atom prop = XInternAtom(dpy, "WM_NAME", False);
		Atom type;  int form;
		unsigned long remain, len;
		unsigned char *list;
		if( XGetWindowProperty(dpy, focus, prop, 0, sizeof(win_title)-1, False,
			AnyPropertyType, &type, &form, &len, &remain, &list) == Success ) {
			len = len*(form/8) - remain;
			memcpy(win_title, list, len);
			win_title[len] = 0;
			XFree(list);
			if( debug )
				printf("new focus: %08x %s\n", (unsigned)focus, win_title);
		}
		else {
			last_focused = 0;
			fprintf(stderr, "XGetWindowProperty failed for window 0x%x\n",
					(int)focus);
			ret = 1;
		}
	}
	gui->unlock_window();
	if( ret ) return -1;

	this->wdw = 0;  this->win = 0;
	this->wx = 0;   this->wy = 0;
	this->rx = 0;   this->ry = 0;
	this->msk = 0;
	BC_WindowBase *wdw = 0;
	int cin = -1;
	if( (wdw=owns(mwindow->gui, focus)) != 0 )
		cin = FOCUS_MWINDOW;
	else if( (wdw=owns(mwindow->awindow->gui, focus)) != 0 )
		cin = FOCUS_AWINDOW;
	else if( (wdw=owns(mwindow->cwindow->gui, focus)) != 0 ) {
		if( mwindow->cwindow->gui->canvas->get_fullscreen() )
			wdw = mwindow->cwindow->gui->canvas->get_canvas();
		cin = FOCUS_CWINDOW;
	}
	else if( mwindow->gui->mainmenu->load_file->thread->running() &&
		 (wdw=mwindow->gui->mainmenu->load_file->thread->window) != 0 &&
		 wdw->win == focus )
		cin = FOCUS_LOAD;
	else {
		int i = mwindow->vwindows.size();
		while( --i >= 0 ) {
			VWindow *vwdw =  mwindow->vwindows[i];
			if( !vwdw->is_running() ) continue;
			if( (wdw=owns(vwdw->gui, focus)) != 0 ) {
				if( vwdw->gui->canvas->get_fullscreen() )
					wdw = vwdw->gui->canvas->get_canvas();
				cin = FOCUS_VIEWER;  break;
			}
		}
	}
	if( cin < 0 ) return -1;
	Window root = 0, child = 0;
	int root_x = 0, root_y = 0, win_x = 0, win_y = 0, x = 0, y = 0;
	unsigned int mask = 0, width = 0, height = 0, border_width = 0, depth = 0;
	wdw->lock_window("Shuttle::get_focused_window_translation 1");
	if( XQueryPointer(wdw->top_level->display, focus, &root, &child,
			&root_x, &root_y, &win_x, &win_y, &mask) ) {
		if( !child ) {
			if( wdw->active_menubar )
				child = wdw->active_menubar->win;
			else if( wdw->active_popup_menu )
				child = wdw->active_popup_menu->win;
			else if( wdw->active_subwindow )
				child = wdw->active_subwindow->win;
			else
				child = wdw->win;
		}
		else
			XGetGeometry(wdw->top_level->display, child, &root, &x, &y,
				&width, &height, &border_width, &depth);
	}
	wdw->unlock_window();
	if( !child || !wdw->match_window(child) ) return -1;
// success
	this->wdw = wdw;
	this->win = child;
	this->msk = mask;
	this->rx = root_x;
	this->ry = root_y;
	this->wx = win_x - x;
	this->wy = win_y - y;
	for( tr=translations.first; tr; tr=tr->next ) {
		if( tr->is_default ) return 1;
		for( int i=0; i<tr->names.size(); ++i ) {
			TransName *name = tr->names[i];
			if( name->cin == cin ) return 1;
		}
	}
	tr = default_translation;
	return 0;
}

int Shuttle::load_translation()
{
	if( read_config_file() > 0 )
		return done = 1;
	if( get_focused_window_translation() < 0 )
		return 0;
	if( last_translation != tr ) {
		last_translation = tr;
		if( debug )
			printf("new translation: %s\n", tr->name);
	}
	return 0;
}

void Shuttle::handle_event()
{
	if( load_translation() ) return;
//	if( debug )
//		printf("event: (%d, %d, 0x%x)\n", ev.type, ev.code, ev.value);
	switch( ev.type ) {
	case EVENT_TYPE_DONE:
	case EVENT_TYPE_ACTIVE_KEY:
		break;
	case EVENT_TYPE_KEY:
		key(ev.code, ev.value);
		break;
	case EVENT_TYPE_JOGSHUTTLE:
		jogshuttle(ev.code, ev.value);
		break;
	default:
		fprintf(stderr, "handle_event() invalid type code\n");
		break;
	}
}

void Shuttle::run()
{
	if( dev_index < 0 ) return;
	const char *dev_name = shuttle_devs[dev_index].path;

#ifdef HAVE_SHUTTLE_USB
	if( usb_direct )
		usb_probe(dev_index);

	disable_cancel();
	while( devsh && !done ) {
		int len = 0;
		static const int IN_ENDPOINT = 0x81;
		unsigned char dat[5];
		int ret = libusb_interrupt_transfer(devsh,
				IN_ENDPOINT, dat, sizeof(dat), &len, 100);
		if( ret != 0 ) {
			if( ret == LIBUSB_ERROR_TIMEOUT ) continue;
			printf("shuttle: %s\n  %s\n",
				dev_name, libusb_strerror((libusb_error)ret));
			sleep(1);  continue;
		}
		if( load_translation() ) break;
		if( debug ) {
			printf("shuttle: ");
			for( int i=0; i<len; ++i ) printf(" %02x", dat[i]);
			printf("\n");
		}
		if( last_shuttle != dat[0] )
			shuttle((char)(last_shuttle = dat[0]));

		if( last_jog != dat[1] )
			jog(last_jog = dat[1]);

		unsigned btns = (dat[4]<<8) | dat[3];
		unsigned dif = last_btns ^ btns;
		if( dif ) {
			last_btns = btns;
			for( int i=0; i<15; ++i ) {
				unsigned msk = 1 << i;
				if( !(dif & msk) ) continue;
				key(i+EVENT_CODE_KEY1, btns & msk ? 1 : 0);
			}
		}
	}
	usb_done();
#endif
	usb_direct = 0;
	enable_cancel();

	for( ; !done; sleep(1) ) {
		fd = open(dev_name, O_RDONLY);
		if( fd < 0 ) {
			perror(dev_name);
			if( first_time ) break;
			continue;
		}
		if( 1 || !ioctl(fd, EVIOCGRAB, 1) ) { // exclusive access
			first_time = 0;
			while( !done ) {
				int ret = read(fd, &ev, sizeof(ev));
				if( done ) break;
				if( ret != sizeof(ev) ) {
					if( ret < 0 ) { perror("read event"); break; }
					fprintf(stderr, "bad read: %d\n", ret);
					break;
				}
				handle_event();
			}
		}
		else
			perror( "evgrab ioctl" );
		close(fd);
	}
	done = 2;
}

int Shuttle::read_config_file()
{
	if( !config_path ) {
		const char *env;
		config_path = (env=getenv("SHUTTLE_CONFIG_FILE")) != 0 ? cstrdup(env) :
			(env=getenv("HOME")) != 0 ? cstrcat(2, env, "/.shuttlerc") : 0;
		if( !config_path ) { fprintf(stderr, "no config file\n");  return 1; }
	}
	struct stat st;
	if( stat(config_path, &st) ) {
		perror(config_path);
		char shuttlerc[BCTEXTLEN];
		snprintf(shuttlerc, sizeof(shuttlerc), "%s/shuttlerc",
				File::get_cindat_path());
		if( stat(shuttlerc, &st) ) {
			perror(shuttlerc);
			return 1;
		}
		delete [] config_path;
		config_path = cstrdup(shuttlerc);
	}
	if( config_mtime > 0 &&
	    config_mtime == st.st_mtime ) return 0;
	FILE *fp = fopen(config_path, "r");
	if( !fp ) {
		perror(config_path);
		return 1;
	}

	config_mtime = st.st_mtime;
	translations.clear();
	debug = 0;
	usb_direct = 0;
#define ws(ch) (ch==' ' || ch=='\t')
	char line[BCTEXTLEN], *cp;
	Translation *trans = 0;
	int no = 0;
	int ret = fgets(cp=line,sizeof(line),fp) ? 0 : -1;
	if( !ret ) ++no;
// lines
	while( !ret ) {
// translation names
		while( !ret && *cp == '[' ) {
			char *name = ++cp;
			while( *cp && *cp != ']' ) ++cp;
			*cp++ = 0;
			if( !name || !*name ) { ret = 1;  break; }
			int cin = -1;
			if( !strcasecmp("default", name) )
				cin = FOCUS_DEFAULT;
			else if( !strcasecmp("cinelerra", name) )
				cin = FOCUS_MWINDOW;
			else if( !strcasecmp("resources", name) )
				cin = FOCUS_AWINDOW;
			else if( !strcasecmp("composer", name) )
				cin = FOCUS_CWINDOW;
			else if( !strcasecmp("viewer", name) )
				cin = FOCUS_VIEWER;
			else if( !strcasecmp("load", name) )
				cin = FOCUS_LOAD;
			else {
				fprintf(stderr, "unknown focus target window: %s\n",
					 name);
				ret = 1;  break;
			}
			if( !trans ) {
				if( cin == FOCUS_DEFAULT ) {
					trans = default_translation;
					trans->clear();
				}
				else {
					trans = new Translation(this, name);
				}
			}
			while( ws(*cp) ) ++cp;
			trans->names.append(new TransName(cin, name, cp));
			ret = fgets(cp=line,sizeof(line),fp) ? 0 : 1;
			if( ret ) {
				fprintf(stderr, "hit eof, no translation def for: %s\n",
					trans->names.last()->name);
				ret = 1;  break;
			}
			++no;
		}
		if( ret ) break;
		if( debug && trans ) {
			printf("------------------------\n");
			TransNames &names = trans->names;
			for( int i=0; i<names.size(); ++i ) {
				TransName *tp = names[i];
				printf("[%s] # %d\n\n", tp->name, tp->cin);
			}
		}
// rules lines: "tok <stroke list>\n"
		while( !ret && *cp != '[' ) {
			const char *key = 0, *tok = 0;
			while( ws(*cp) ) ++cp;
			if( !*cp || *cp == '\n' || *cp == '#' ) goto skip;
			tok = cp;
			while( *cp && !ws(*cp) && *cp != '\n' ) ++cp;
			*cp++ = 0;
			if( !strcmp(tok, "DEBUG") ) {
				debug = 1;  goto skip;
			}
			if( !strcmp(tok, "USB_DIRECT") ) {
				usb_direct = 1;  goto skip;
			}
			key = tok;
			if( !trans ) {
				fprintf(stderr, "no translation section defining key: %s\n", key);
				ret = 1;  break;
			}
	
			ret = trans->start_line(key);
			while( !ret && *cp && *cp != '\n' ) {
				while( ws(*cp) ) ++cp;
				if( !*cp || *cp == '#' || *cp == '\n' ) break;
				if( *cp == '"' ) {
					while( *cp ) {
						if( *cp == '"' )
							trans->add_string(cp);
						while( ws(*cp) ) ++cp;
						if( !*cp || *cp == '#' || *cp == '\n' ) break;
						tok = cp;
						while( *cp && !ws(*cp) && *cp != '\n' ) ++cp;
						*cp = 0;
						SKeySym sym = KeySymMapping::to_keysym(tok);
						if( !sym ) {
							fprintf(stderr, "unknown keysym: %s\n", tok);
							ret = 1;  break;
						}
						trans->add_keysym(sym, PRESS_RELEASE);
					}
					continue;
				}
				tok = cp;
				while( *cp && !ws(*cp) && *cp!='/' && *cp != '\n' ) ++cp;
				int dhu = PRESS_RELEASE;
				if( *cp == '/' ) {
					*cp++ = 0;
					switch( *cp ) {
					case 'D':  dhu = PRESS;    break;
					case 'H':  dhu = HOLD;     break;
					case 'U':  dhu = RELEASE;  break;
					default:
						fprintf(stderr, "invalid up/down modifier [%s]%s: '%c'\n",
							trans->name, tok, *cp);
						ret = 1;  break;
					}
					++cp;
				}
				else
					*cp++ = 0;
				trans->add_keystroke(tok, dhu);
			}
			if( ret ) break;
			trans->finish_line();
			if( debug )
				trans->print_line(key);
		skip:	ret = fgets(cp=line,sizeof(line),fp) ? 0 : -1;
			if( !ret ) ++no;
		}
		if( trans ) {
			if( trans != default_translation )
				translations.append(trans);
			trans = 0;
		}
	}
	if( ret > 0 )
		fprintf(stderr, "shuttle config err file: %s, line:%d\n",
			config_path, no);

	fclose(fp);
	return ret;
}
#endif
