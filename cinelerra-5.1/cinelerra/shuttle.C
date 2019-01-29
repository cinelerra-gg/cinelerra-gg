#ifdef HAVE_SHUTTLE
// Copyright 2013 Eric Messick (FixedImagePhoto.com/Contact)
// reworked 2019 for cinelerra-gg by William Morrow

// keys.h collides with linux/input_events.h
#define KEYS_H

#include "arraylist.h"
#include "cstrdup.h"
#include "file.h"
#include "guicast.h"
#include "linklist.h"
#include "loadfile.h"
#include "mainmenu.h"
#include "mwindow.h"
#include "shuttle.h"
#include "thread.h"

#include <sys/time.h>
#include <sys/types.h>
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
	{ "XK_Button_1", XK_Button_1 },
	{ "XK_Button_2", XK_Button_2 },
	{ "XK_Button_3", XK_Button_3 },
	{ "XK_Scroll_Up", XK_Scroll_Up },
	{ "XK_Scroll_Down", XK_Scroll_Down },
#include "shuttle_keys.h"
	{ NULL, 0 }
};

KeySym KeySymMapping::to_keysym(const char *str)
{
	for( KeySymMapping *ksp = &key_sym_mapping[0]; ksp->str; ++ksp )
		if( !strcmp(str, ksp->str) ) return ksp->sym;
	return 0;
}

const char *KeySymMapping::to_string(KeySym ks)
{
	for( KeySymMapping *ksp = &key_sym_mapping[0]; ksp->sym; ++ksp )
		if( ksp->sym == ks ) return ksp->str;
	return 0;
}

TransName::TransName(int cin, const char *nm, const char *re)
{
	this->cin = cin;
	this->name = cstrdup(nm);
	this->err = regcomp(&this->regex, re, REG_NOSUB);
	if( err ) {
      		fprintf(stderr, "error compiling regex for [%s]: %s\n", name, re);
		char emsg[BCTEXTLEN];
		regerror(err, &regex, emsg, sizeof(emsg));
      		fprintf(stderr, "regerror: %s\n", emsg);
	}
}
TransName::~TransName()
{
	delete [] name;
	regfree(&regex);
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

void Translation::append_stroke(KeySym sym, int press)
{
	Stroke *s = pressed_strokes->append();
	s->keysym = sym;
	s->press = press;
}

void Translation::add_keysym(KeySym sym, int press_release)
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
	if( !all_keys ) {
		pressed_strokes = released_strokes;
	}
	if( keysym_down ) {
		append_stroke(keysym_down, 0);
		keysym_down = 0;
	}
	first_release_stroke = 1;
}

void Translation::add_keystroke(const char *keySymName, int press_release)
{
	KeySym sym;

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

void Translation::add_string(const char *str)
{
	while( str && *str ) {
		if( *str >= ' ' && *str <= '~' )
			add_keysym((KeySym)(*str), PRESS_RELEASE);
		++str;
	}
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
		print_strokes(key, "D", pressed_strokes);
		print_strokes(key, "U", released_strokes);
	}
	else {
		print_strokes(key, "", pressed_strokes);
	}
	printf("\n");
}

// press values in Modifiers:
// PRESS -> down
// HOLD -> held
// PRESS_RELEASE -> released, but to be re-pressed if necessary
// RELEASE -> up

void Modifiers::mark_as_down(KeySym sym, int hold)
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

void Modifiers::mark_as_up(KeySym sym)
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
 : Thread(0, 0, 0)
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
	last_shuttle.tv_sec = 0;
	last_shuttle.tv_usec = 0;
	need_synthetic_shuttle = 0;
	dev_name = 0;

	done = -1;
	failed = 0;
	first_time = 1;
	tr = 0;
	last_translation = 0;
	last_focused = 0;

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
int Shuttle::send_key(KeySym keysym, int press)
{
	KeyCode keycode = XKeysymToKeycode(wdw->top_level->display, keysym);
	if( debug )
		printf("key: %04x %d\n", (unsigned)keycode, press);
	XKeyEvent *k = new XKeyEvent();
	memset(k, 0, sizeof(*k));
	k->type = press ? KeyPress : KeyRelease;
	k->time = milliTimeClock();
	k->display = wdw->top_level->display;
	k->root = wdw->top_level->rootwin;
	k->window = win;
	k->x_root = rx;
	k->y_root = ry;
	k->x = wx;
	k->y = wy;
	k->state = msk;
	k->keycode = keycode;
	k->same_screen = 1;
	wdw->top_level->put_event((XEvent *) k);
	return 0;
}

int Shuttle::send_keysym(KeySym keysym, int press)
{
	return keysym >= XK_Button_1 && keysym <= XK_Scroll_Down ?
		send_button((unsigned int)keysym - XK_Button_0, press) :
		send_key(keysym, press ? True : False);
}


static Stroke *fetch_stroke(Translation *translation, int kjs, int index)
{
	Stroke *ret = 0;
	if( translation ) {
		switch( kjs ) {
		default:
		case KJS_KEY_DOWN:  ret = translation->key_down[index].first;	break;
		case KJS_KEY_UP:    ret = translation->key_up[index].first;	break;
		case KJS_JOG:	    ret = translation->jog[index].first;	break;
		case KJS_SHUTTLE:   ret = translation->shuttles[index].first;	break;
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


void Shuttle:: shuttle(int value)
{
	if( value < S_7 || value > S7 ) {
		fprintf(stderr, "shuttle(%d) out of range\n", value);
		return;
	}
	gettimeofday(&last_shuttle, 0);
	need_synthetic_shuttle = value != 0;
	if( value != shuttlevalue ) {
		shuttlevalue = value;
		send_stroke_sequence(KJS_SHUTTLE, value+7);
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
	int direction;
	struct timeval now;
	struct timeval delta;

	// We should generate a synthetic event for the shuttle going
	// to the home position if we have not seen one recently
	if( need_synthetic_shuttle ) {
		gettimeofday( &now, 0 );
		timersub( &now, &last_shuttle, &delta );

		if( delta.tv_sec >= 1 || delta.tv_usec >= 5000 ) {
			shuttle(0);
			need_synthetic_shuttle = 0;
		}
	}

	if( jogvalue != 0xffff ) {
		value = value & 0xff;
		direction = ((value - jogvalue) & 0x80) ? -1 : 1;
		while( jogvalue != value ) {
			// driver fails to send an event when jogvalue == 0
			if( jogvalue != 0 ) {
	send_stroke_sequence(KJS_JOG, direction > 0 ? 1 : 0);
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
	default:
		fprintf(stderr, "jogshuttle(%d, %d) invalid code\n", code, value);
		break;
	}
}

const char *Shuttle::probe()
{
	struct stat st;
	static const char *shuttle_devs[] = {
		"/dev/input/by-id/usb-Contour_Design_ShuttleXpress-event-if00",
		"/dev/input/by-id/usb-Contour_Design_ShuttlePRO_v2-event-if00",
	};
	int ret = sizeof(shuttle_devs) / sizeof(shuttle_devs[0]);
	while( --ret >= 0 && stat(shuttle_devs[ret] , &st) );
	return ret >= 0 ? shuttle_devs[ret] : 0;
}

void Shuttle::start(const char *dev_name)
{
	this->dev_name = dev_name;
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


int Shuttle::get_focused_window_translation()
{
	MWindowGUI *gui = mwindow->gui;
	Display *dpy = gui->display;
	Window focus = 0;
	int ret = 0, revert = 0;
	char win_title[BCTEXTLEN];
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
				printf("new focus: %08x\n", (unsigned)focus);
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
	if( (wdw=mwindow->gui) && wdw->win == focus )
		cin = FOCUS_MWINDOW;
	else if( (wdw=mwindow->awindow->gui) && wdw->win == focus )
		cin = FOCUS_AWINDOW;
	else if( (wdw=mwindow->cwindow->gui) && wdw->win == focus )
		cin = FOCUS_CWINDOW;
	else if( (wdw=mwindow->gui->mainmenu->load_file->thread->window) &&
		 wdw->win == focus )
		cin = FOCUS_LOAD;
	else {
		int i = mwindow->vwindows.size();
		while( --i >= 0 ) {
			VWindow *vwdw =  mwindow->vwindows[i];
			if( !vwdw->is_running() ) continue;
			if( (wdw=vwdw->gui) && wdw->win == focus ) {
				cin = FOCUS_VIEWER;  break;
			}
		}
	}
	if( cin < 0 ) return -1;
	Window root = 0, child = 0;
	int root_x = 0, root_y = 0, win_x = 0, win_y = 0, x = 0, y = 0;
	unsigned int mask = 0, width = 0, height = 0, border_width = 0, depth = 0;
	wdw->lock_window("Shuttle::get_focused_window_translation 1");
	if( XQueryPointer(wdw->display, focus, &root, &child,
			&root_x, &root_y, &win_x, &win_y, &mask) ) {
		if( !child ) {
			if( wdw->active_menubar )
				child = wdw->active_menubar->win;
			else if( wdw->active_popup_menu )
				child = wdw->active_popup_menu->win;
			else if( wdw->active_subwindow )
				child = wdw->active_subwindow->win;
		}
		if( child )
			XGetGeometry(wdw->display, child, &root, &x, &y,
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
			if( name->cin != cin ) continue;
			if( regexec(&name->regex, win_title, 0, NULL, 0) )
				return 1;
		}
	}
	tr = default_translation;
	return 0;
}

void Shuttle::handle_event()
{
	if( read_config_file() > 0 ) {
		done = 1;
		return;
	}
	if( get_focused_window_translation() < 0 )
		return;
	if( last_translation != tr ) {
		last_translation = tr;
		if( debug )
			printf("new translation: %s\n", tr->name);
	}
//fprintf(stderr, "event: (%d, %d, 0x%x)\n", ev.type, ev.code, ev.value);
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
	for( enable_cancel(); !done; sleep(1) ) {
		fd = open(dev_name, O_RDONLY);
		if( fd < 0 ) {
			perror(dev_name);
			if( first_time ) break;
			continue;
		}
		if( !ioctl(fd, EVIOCGRAB, 1) ) { // exclusive access
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
// regex in TransName constructor
			trans->names.append(new TransName(cin, name, cp));
			if( trans->names.last()->err ) { ret = 1;  break; }
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
					tok = ++cp;
					while( *cp && *cp != '"' && *cp != '\n' ) {
						if( *cp != '\\' ) { ++cp;  continue; }
						for( char *bp=cp; *bp; ++bp ) bp[0] = bp[1];
					}
					*cp++ = 0;
					trans->add_string(tok);
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
