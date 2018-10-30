
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "cursors.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "bccolors.h"
#include "samples.h"
#include "echocancel.h"
#include "theme.h"
#include "transportque.inc"
#include "units.h"
#include "vframe.h"


#include <string.h>
#define TMPDIR "/tmp/echocancel/"


REGISTER_PLUGIN(EchoCancel)


EchoCancelConfig::EchoCancelConfig()
{
	level = 0.0;
	normalize = 0;
	xzoom = MIN_XZOOM;
	peaks = MIN_PEAKS;
	damp = MIN_DAMP;
	cutoff = (MIN_CUTOFF+MAX_CUTOFF)/2;
	gain = 0;
	offset = 0;
	window_size = 0;
	mode = CANCEL_OFF;
	history_size = 4;
}

int EchoCancelConfig::equivalent(EchoCancelConfig &that)
{
	return EQUIV(level, that.level) &&
		normalize == that.normalize &&
		xzoom == that.xzoom &&
		damp == that.damp &&
		cutoff == that.cutoff &&
		window_size == that.window_size &&
		mode == that.mode &&
		history_size == that.history_size;
}

void EchoCancelConfig::copy_from(EchoCancelConfig &that)
{
	level = that.level;
	normalize = that.normalize;
	xzoom = that.xzoom;
	peaks = that.peaks;
	damp = that.damp;
	cutoff = that.cutoff;
	gain = that.gain;
	offset = that.offset;
	window_size = that.window_size;
	mode = that.mode;
	history_size = that.history_size;

	if( window_size ) CLAMP(window_size, MIN_WINDOW, MAX_WINDOW);
	CLAMP(history_size, MIN_HISTORY, MAX_HISTORY);
	CLAMP(xzoom, MIN_XZOOM, MAX_XZOOM);
	CLAMP(peaks, MIN_PEAKS, MAX_PEAKS);
	CLAMP(damp, MIN_DAMP, MAX_DAMP);
	CLAMP(cutoff, MIN_CUTOFF, MAX_CUTOFF);
}

void EchoCancelConfig::interpolate(EchoCancelConfig &prev,
	EchoCancelConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	copy_from(prev);
}



EchoCancelFrame::EchoCancelFrame(int n)
{
	this->len = n;
	data = new float[n + 1];
	data[0] = 1;
}

EchoCancelFrame::~EchoCancelFrame()
{
	delete [] data;
}



EchoCancelLevel::EchoCancelLevel(EchoCancel *plugin, int x, int y)
 : BC_FPot(x, y, plugin->config.level, INFINITYGAIN, MAXGAIN)
{
	this->plugin = plugin;
}

int EchoCancelLevel::handle_event()
{
	plugin->config.level = get_value();
	plugin->send_configure_change();
	return 1;
}





EchoCancelMode::EchoCancelMode(EchoCancel *plugin, int x, int y)
 : BC_PopupMenu(x, y, 120, to_text(plugin->config.mode))
{
	this->plugin = plugin;
}

void EchoCancelMode::create_objects()
{
	add_item(new BC_MenuItem(to_text(CANCEL_OFF)));
	add_item(new BC_MenuItem(to_text(CANCEL_ON)));
	add_item(new BC_MenuItem(to_text(CANCEL_MAN)));
}

int EchoCancelMode::handle_event()
{
	int new_mode = to_mode(get_text());
	if( plugin->config.mode != new_mode ) {
		plugin->config.mode = new_mode;
		plugin->send_configure_change();
	}
	return 1;
}

const char* EchoCancelMode::to_text(int mode)
{
	switch(mode) {
	case CANCEL_ON: return _("ON");
	case CANCEL_MAN: return _("MAN");
	}
	return _("OFF");
}

int EchoCancelMode::to_mode(const char *text)
{
	if(!strcmp(to_text(CANCEL_ON), text)) return CANCEL_ON;
	if(!strcmp(to_text(CANCEL_MAN), text)) return CANCEL_MAN;
	return CANCEL_OFF;
}




EchoCancelHistory::EchoCancelHistory(EchoCancel *plugin,
	int x,
	int y)
 : BC_IPot(x, y, plugin->config.history_size, MIN_HISTORY, MAX_HISTORY)
{
	this->plugin = plugin;
}

int EchoCancelHistory::handle_event()
{
	plugin->config.history_size = get_value();
	plugin->send_configure_change();
	return 1;
}






EchoCancelWindowSize::EchoCancelWindowSize(EchoCancel *plugin, int x, int y, const char *text)
 : BC_PopupMenu(x, y, 80, text)
{
	this->plugin = plugin;
}

int EchoCancelWindowSize::handle_event()
{
	plugin->config.window_size = atoi(get_text());
	plugin->send_configure_change();
	return 1;
}

const char *EchoCancelWindowSize::to_text(int size)
{
	if( !size ) return _("default");
	static char string[BCSTRLEN];
	sprintf(string, "%d", size);
	return string;
}

int EchoCancelWindowSize::to_size(const char *text)
{
	return !strcmp(to_text(0),text) ? 0 : strtol(text,0,0);
}



EchoCancelWindowSizeTumbler::EchoCancelWindowSizeTumbler(EchoCancel *plugin, int x, int y)
 : BC_Tumbler(x, y)
{
	this->plugin = plugin;
}

int EchoCancelWindowSizeTumbler::handle_up_event()
{
	if( !plugin->config.window_size )
		plugin->config.window_size = MIN_WINDOW;
	else if( (plugin->config.window_size *= 2) > MAX_WINDOW )
		plugin->config.window_size = MAX_WINDOW;
	EchoCancelWindowSize *window_size =
		((EchoCancelWindow *)plugin->get_thread()->get_window())->window_size;
	const char *wsp = EchoCancelWindowSize::to_text(plugin->config.window_size);
	window_size->set_text(wsp);
	plugin->send_configure_change();
	return 0;
}

int EchoCancelWindowSizeTumbler::handle_down_event()
{
	plugin->config.window_size /= 2;
	if(plugin->config.window_size < MIN_WINDOW)
		plugin->config.window_size = 0;
	EchoCancelWindowSize *window_size =
		((EchoCancelWindow *)plugin->get_thread()->get_window())->window_size;
	const char *wsp = EchoCancelWindowSize::to_text(plugin->config.window_size);
	window_size->set_text(wsp);
	plugin->send_configure_change();
	return 1;
}





EchoCancelNormalize::EchoCancelNormalize(EchoCancel *plugin, int x, int y)
 : BC_CheckBox(x, y, plugin->config.normalize, _("Normalize"))
{
	this->plugin = plugin;
}

int EchoCancelNormalize::handle_event()
{
	plugin->config.normalize = get_value();
	plugin->send_configure_change();
	return 1;
}




EchoCancelXZoom::EchoCancelXZoom(EchoCancel *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.xzoom, MIN_XZOOM, MAX_XZOOM)
{
	this->plugin = plugin;
}

int EchoCancelXZoom::handle_event()
{
	plugin->config.xzoom = get_value();
	plugin->send_configure_change();
	return 1;
}



EchoCancelPeaks::EchoCancelPeaks(EchoCancel *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.peaks, MIN_PEAKS, MAX_PEAKS)
{
	this->plugin = plugin;
}

int EchoCancelPeaks::handle_event()
{
	plugin->config.peaks = get_value();
	plugin->send_configure_change();
	return 1;
}



EchoCancelDamp::EchoCancelDamp(EchoCancel *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.damp, MIN_DAMP, MAX_DAMP)
{
	this->plugin = plugin;
}

int EchoCancelDamp::handle_event()
{
	plugin->config.damp = get_value();
	plugin->send_configure_change();
	return 1;
}



EchoCancelCutoff::EchoCancelCutoff(EchoCancel *plugin, int x, int y)
 : BC_IPot(x, y, plugin->config.cutoff, MIN_CUTOFF, MAX_CUTOFF)
{
	this->plugin = plugin;
}

int EchoCancelCutoff::handle_event()
{
	plugin->config.cutoff = get_value();
	plugin->send_configure_change();
	return 1;
}



EchoCancelCanvas::EchoCancelCanvas(EchoCancel *plugin, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h, BLACK)
{
	this->plugin = plugin;
	current_operation = NONE;
}

int EchoCancelCanvas::button_press_event()
{
	if(is_event_win() && cursor_inside())
	{
		calculate_point(1);
		current_operation = DRAG;
		plugin->send_configure_change();
		return 1;
	}
	return 0;
}

int EchoCancelCanvas::button_release_event()
{
	if(current_operation == DRAG)
	{
		calculate_point(2);
		current_operation = NONE;
		return 1;
	}
	return 0;
}

int EchoCancelCanvas::cursor_motion_event()
{
	if(current_operation == DRAG)
	{
		calculate_point(3);
	}
	return 0;
}


void EchoCancelCanvas::calculate_point(int do_overlay)
{
	int x = get_cursor_x();
	int y = get_cursor_y();
	CLAMP(x, 0, get_w() - 1);
	CLAMP(y, 0, get_h() - 1);

	EchoCancelWindow *window = (EchoCancelWindow *)plugin->thread->window;
	window->calculate_frequency(x, y, do_overlay);
}

void EchoCancelCanvas::draw_overlay()
{
	EchoCancelWindow *window = (EchoCancelWindow*)plugin->thread->window;
	if(window->probe_x >= 0 || window->probe_y >= 0)
	{
		set_color(GREEN);
		set_inverse();
		char string[BCTEXTLEN];
		sprintf(string, "%d, %f", plugin->config.offset, plugin->config.gain);
		draw_text(window->probe_x, window->probe_y, string);
		draw_line(0, window->probe_y, get_w(), window->probe_y);
		draw_line(window->probe_x, 0, window->probe_x, get_h());
		set_opaque();
	}
}








EchoCancelWindow::EchoCancelWindow(EchoCancel *plugin)
 : PluginClientWindow(plugin, plugin->w, plugin->h, 320, 320, 1)
{
	this->plugin = plugin;
	probe_x = probe_y = -1;
}

EchoCancelWindow::~EchoCancelWindow()
{
}

void EchoCancelWindow::create_objects()
{
	int pad = plugin->get_theme()->widget_border;
	add_subwindow(canvas = new EchoCancelCanvas(plugin, 0, 0,
		get_w(), get_h() - 2*BC_Pot::calculate_h() - 3*pad));
	canvas->set_cursor(CROSS_CURSOR, 0, 0);

	int x = pad, y = canvas->get_y() + canvas->get_h() + pad;
	int x1 = x, y1 = y;

	add_subwindow(level_title = new BC_Title(x, y, _("Level:")));
	x += level_title->get_w() + pad;
	add_subwindow(level = new EchoCancelLevel(plugin, x, y));
	x += level->get_w() + pad;
	y += level->get_h() + pad;

	x = x1;
	add_subwindow(normalize = new EchoCancelNormalize(plugin, x, y));
	x += normalize->get_w() + 3*pad;
	y += normalize->get_h() - BC_Title::calculate_h(this,_("Gain: "));
	add_subwindow(gain_title = new EchoCancelTitle(x, y, _("Gain: "), 0.));
	x += gain_title->get_w() + 2*pad;
	add_subwindow(offset_title = new EchoCancelTitle(x, y, _("Offset: "), 0));

	x = x1 + level_title->get_w() + level->get_w() + 2*pad;
	x1 = x;
	y = y1;

	add_subwindow(mode_title = new BC_Title(x, y, _("Mode:")));
	x += mode_title->get_w() + pad;
	add_subwindow(mode = new EchoCancelMode(plugin, x, y));
	mode->create_objects();

	x = x1;
	y += mode->get_h() + pad;

	add_subwindow(window_size_title = new BC_Title(x, y, _("Window size:")));
	x += window_size_title->get_w() + pad;
	const char *wsp = EchoCancelWindowSize::to_text(plugin->config.window_size);
	add_subwindow(window_size = new EchoCancelWindowSize(plugin, x, y, wsp));
	x += window_size->get_w();
	add_subwindow(window_size_tumbler = new EchoCancelWindowSizeTumbler(plugin, x, y));

	window_size->add_item(new BC_MenuItem(EchoCancelWindowSize::to_text(0)));
	for( int i=MIN_WINDOW; i<=MAX_WINDOW; i*=2 ) {
		window_size->add_item(new BC_MenuItem(EchoCancelWindowSize::to_text(i)));
	}

	x = x1;
	y += window_size->get_h() + pad;

	x = x1 = window_size_tumbler->get_x() + window_size_tumbler->get_w() + pad;
	y = y1;
	add_subwindow(history_title = new BC_Title(x, y, _("History:")));
	x += history_title->get_w() + pad;
	add_subwindow(history = new EchoCancelHistory(plugin, x, y));

	x = x1;
	int y2 = y;
	y += history->get_h() + pad;
	add_subwindow(xzoom_title = new BC_Title(x, y, _("X Zoom:")));
	x += xzoom_title->get_w() + pad;
	add_subwindow(xzoom = new EchoCancelXZoom(plugin, x, y));
	x += xzoom->get_w() + pad;
	x1 = x;
	add_subwindow(damp_title = new BC_Title(x, y, _("Damp:")));
	x += damp_title->get_w() + pad;
	add_subwindow(damp = new EchoCancelDamp(plugin, x, y));
	x += damp->get_w() + pad;
	add_subwindow(cutoff_title = new BC_Title(x, y, _("Cutoff Hz:")));
	x += cutoff_title->get_w() + pad;
	add_subwindow(cutoff = new EchoCancelCutoff(plugin, x, y));
	int x2 = x - BC_Title::calculate_w(this, _("Peaks:")) - pad;
	add_subwindow(peaks_title = new BC_Title(x2, y2, _("Peaks:")));
	add_subwindow(peaks = new EchoCancelPeaks(plugin, x, y2));

	x = x1 + 3*pad;
	y = y1;
	add_subwindow(freq_title = new BC_Title(x, y, _("0 Hz")));
	y += freq_title->get_h() + pad;
	add_subwindow(amplitude_title = new BC_Title(x, y, _("Amplitude: 0 dB")));

	show_window();
}

int EchoCancelWindow::resize_event(int w, int h)
{
	int canvas_h = canvas->get_h();
	int canvas_difference = get_h() - canvas_h;

	canvas->reposition_window(0, 0, w, h - canvas_difference);
	canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
	probe_x = probe_y = -1;

	int dx = 0, dy = canvas->get_h() - canvas_h;

// Remove all columns which may be a different size.
	plugin->frame_buffer.remove_all_objects();
	plugin->frame_history.remove_all_objects();

	level_title->reposition_window_relative(dx, dy);
	level->reposition_window_relative(dx, dy);
	mode_title->reposition_window_relative(dx, dy);
	mode->reposition_window_relative(dx, dy);
	window_size_title->reposition_window_relative(dx, dy);
	window_size->reposition_window_relative(dx, dy);
	window_size_tumbler->reposition_window_relative(dx, dy);
	gain_title->reposition_window_relative(dx, dy);
	offset_title->reposition_window_relative(dx, dy);
	normalize->reposition_window_relative(dx, dy);
	history_title->reposition_window_relative(dx, dy);
	history->reposition_window_relative(dx, dy);
	xzoom_title->reposition_window_relative(dx, dy);
	xzoom->reposition_window_relative(dx, dy);
	peaks_title->reposition_window_relative(dx, dy);
	peaks->reposition_window_relative(dx, dy);
	damp_title->reposition_window_relative(dx, dy);
	damp->reposition_window_relative(dx, dy);
	cutoff_title->reposition_window_relative(dx, dy);
	cutoff->reposition_window_relative(dx, dy);
	freq_title->reposition_window_relative(dx, dy);
	amplitude_title->reposition_window_relative(dx, dy);

	flush();
	plugin->w = w;
	plugin->h = h;
	return 0;
}


void EchoCancelWindow::calculate_frequency(int x, int y, int do_overlay)
{
	if( !do_overlay && probe_x == x && probe_y == y ) return;
	if( do_overlay & 2 ) canvas->draw_overlay();
	probe_x = x;  probe_y = y;

// Convert to coordinates in frame history
	int need_config_update = 0;
	double gain = plugin->config.gain;
	double offset = plugin->config.offset;

	if( x >= 0 && plugin->frame_history.size() ) {
		int ww = canvas->get_w();
		int freq_pixel = x;
		if( freq_pixel < 0 ) freq_pixel = 0;
		if( freq_pixel > ww ) freq_pixel = ww;
		int time_pixel = 0;
		int idx = plugin->frame_history.size()-1 - time_pixel;
		EchoCancelFrame *frm = plugin->frame_history[idx];
		int pixels = canvas->get_w();
		int half_window = plugin->header.window_size / 2;
		offset = (double)half_window * freq_pixel/(pixels * plugin->config.xzoom);
		if( offset <= 1e-10 ) offset = 1;
		int freq = plugin->header.sample_rate / offset;
		int msecs = 1000. / freq;
		char string[BCTEXTLEN];
		sprintf(string, _("%d Hz, %d ms (%d))"), freq, msecs, (int)offset);
		freq_title->update(string);
		int frm_sz1 = frm->size()-1;
		if( freq_pixel > frm_sz1 ) freq_pixel = frm_sz1;
		float *frame_data = frm->samples();
		double level = frame_data[freq_pixel];
		double scale = frm->scale();
		sprintf(string, _("Amplitude: %.3f (%.6g)"), level, scale);
		amplitude_title->update(string);
	}
	if( y >= 0 ) {
		int hh = canvas->get_h()/2;
		int gain_pixel = hh - y;
		if( gain_pixel < 0 ) gain_pixel = 0;
		if( gain_pixel > hh ) gain_pixel = hh;
		gain = (double)gain_pixel / hh;
	}
	if( plugin->config.gain != gain ) {
		gain_title->update(plugin->config.gain = gain);
		need_config_update = 1;
	}
	if( offset > 0 && plugin->config.offset != offset ) {
		offset_title->update(plugin->config.offset = offset);
		need_config_update = 1;
	}
	if( need_config_update )
		plugin->send_configure_change();

	if( do_overlay & 1 ) canvas->draw_overlay();
        if( do_overlay ) canvas->flash();
}

void EchoCancelWindow::update_gui()
{
	level->update(plugin->config.level);
        char string[BCTEXTLEN];
        sprintf(string, "%d", plugin->config.window_size);
        window_size->set_text(string);
	mode->set_text(EchoCancelMode::to_text(plugin->config.mode));
	history->update(plugin->config.history_size);
	normalize->set_value(plugin->config.normalize);
	gain_title->update(plugin->config.gain);
	offset_title->update(plugin->config.offset);
}

EchoCancel::EchoCancel(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	timer = new Timer;
	w = 640;
	h = 480;
}

EchoCancel::~EchoCancel()
{
	delete fft;
	delete audio_buffer;
	delete data;
	delete_buffers();
	frame_buffer.remove_all_objects();
	frame_history.remove_all_objects();
	delete timer;
}


void EchoCancel::reset()
{
	thread = 0;
	window_size = 0;
	half_window = 0;
	interrupted = 0;
	fft = 0;
	audio_buffer = 0;
	data = 0;
	cor = 0;
	aud_real = 0;  aud_imag = 0;
	env_real = 0;  env_imag = 0;
	envelope = 0;  env_data = 0;
	time_frame = 0;
	memset(&header, 0, sizeof(header));
}

void DataBuffer::
set_buffer(int ofs, int len)
{
	int needed = ofs + len;
	if( needed > allocated ) {
		DataHeader *new_header = new_data_header(needed);
		if( data_header ) {
			int old_size = sizeof(DataHeader) + allocated*sizeof(float);
			memcpy(new_header, data_header, old_size);
			delete [] (char *)data_header;
		}
		else
			memset(new_header, 0, sizeof(*new_header));
		data_header = new_header;
		allocated = needed;
	}
	sample_data = &data_header->samples[ofs];
	data_len = len;
}

void AudioBuffer::
set_buffer(int ofs, int len)
{
	int needed = ofs + len;
	if( needed > allocated ) {
		Samples *new_samples = new Samples(needed);
		if( samples ) {
			double *old_data = samples->get_data();
			double *new_data = new_samples->get_data();
			memcpy(new_data, old_data, allocated*sizeof(old_data[0]));
			delete samples;
		}
		samples = new_samples;
		allocated = needed;
	}
	sample_data = samples->get_data() + ofs;
	data_len = len;
}

void AudioBuffer::
append(double *bfr, int len)
{
	set_buffer(buffer_size(), len);
	memcpy(get_data(), bfr, len*sizeof(sample_data[0]));
	data_len = len;
}

void AudioBuffer::
remove(int len)
{
	double *data = samples->get_data();
	int move_len = buffer_size() - len;
	memmove(data, data+len, move_len*sizeof(double));
	if( move_len < data_len ) {
		sample_data = data;
		data_len = move_len;
	}
	else
		sample_data -= len;
}

const char* EchoCancel::plugin_title() { return N_("EchoCancel"); }
int EchoCancel::is_realtime() { return 1; }

static inline double sqr(double v) { return v*v; }

static inline void cx_product(int n, int sf, double *rp, double *ip,
		double *arp, double *aip, double *brp, double *bip)
{
	int m = !sf ? n : n/2, i = 0;
	while( i <= m ) {
		double ar = arp[i], ai = aip[i];
		double br = brp[i], bi = bip[i];
		rp[i] = ar*br - ai*bi;  // complex a*ib
		ip[i] = ar*bi + ai*br;
		++i;
	}
	if( !sf ) return;
	while( --m > 0 ) { rp[i] = rp[m];  ip[i] = -ip[m];  ++i; }
}

static inline void cj_product(int n, int sf, double *rp, double *ip,
		double *arp, double *aip, double *brp, double *bip)
{
	int m = !sf ? n-1 : n/2, i = 0;
	while( i <= m ) {
		double ar = arp[i], ai = aip[i];
		double br = brp[i], bi = -bip[i];
		rp[i] = ar*br - ai*bi;  // complex a*ib'
		ip[i] = ar*bi + ai*br;
		++i;
	}
	if( !sf ) return;
	while( --m > 0 ) { rp[i] = rp[m];  ip[i] = -ip[m];  ++i; }
}

static inline void log_power(int n, int sf, double *rp, double *arp, double *aip)
{
	int m = !sf ? n : n/2, i = 0;
	while( i <= m ) {
		double sr = arp[i], si = aip[i];
		double ss = sqr(sr) + sqr(si);
		rp[i] = ss > 1e-20 ? log(ss) : -46;
		++i;
	}
	if( !sf ) return;
	while( --m > 0 ) { rp[i] = rp[m];  ++i; }
}

/* Adapted from Marple: Digital Spectral Analysis with Applications
 *   Solves linear simultaneous equations by the Levinson algorithm.
 *      TX = Z
 * Input:
 *   T[m,m] a nonsymmetric Toeplitz matrix,
 *     tc[0..m-1] left column, tr[0..m-1] top row
 *   Z[m]   known right-hand-side column,
 * Output:
 *   X[m]   solution vector
 * Returns: 1 if singular, 0 on success
 */

static inline int toeplitz(int m, double *tc, double *tr, double *z, double *x)
{
	double a[m], b[m], p = tc[0];
	if( p != tr[0] || fabs(p) < 1e-20 ) return 1;
	x[0] = z[0] / p;
	for( int k0=0,k=1; k<m; k0=k++ ) {
		double sc = tc[k], sr = tr[k], xc = x[0] * sc;
		for( int i=1,j=k0; i<k; ++i,--j ) {
			sc += a[i] * tc[j];
			sr += b[i] * tr[j];
			xc += x[i] * tc[j];
		}
		double ac = -sc / p, br = -sr / p;
		p *= 1 - ac * br;
		if( fabs(p) < 1e-20 ) return 1;
		a[k] = ac;	b[k] = br;
		x[k] = (z[k] - xc) / p;
		for( int i=1,j=k0; j>0; ++i,--j ) {
			sc = a[i];	sr = b[j];
			a[i] += ac * sr;
			b[j] += br * sc;
		}
		double xk = x[k];
		for( int i=0,j=k; j>0; ++i,--j )
			x[i] += xk * b[j];
	}
	return 0;
}


void EchoCancel::cepstrum(double *audio)
{
//dfile_dump(TMPDIR "audio",audio,window_size);
	fft->do_fft(window_size, 0, audio, 0, aud_real, aud_imag);
//dfile_dump(TMPDIR "aud_real",aud_real,window_size);
//dfile_dump(TMPDIR "aud_imag",aud_imag,window_size);
	// half zero, half window of audio
	int k = 0;
	for( ; k<half_window; ++k ) cor[k] = 0;
	for( ; k<window_size; ++k ) cor[k] = audio[k];
	fft->do_fft(window_size, 0, cor, 0, env_real, env_imag);
//dfile_dump(TMPDIR "env_real",env_real,window_size);
//dfile_dump(TMPDIR "env_imag",env_imag,window_size);
	cj_product(window_size, 1, env_real, env_imag,
		env_real, env_imag, aud_real, aud_imag);
//dfile_dump(TMPDIR "prd_real",env_real,window_size);
//dfile_dump(TMPDIR "prd_imag",env_imag,window_size);
	log_power(window_size, 1, aud_real, env_real, env_imag);
	// a = zeros, last half audio buffer, b = audio buffer
	// COR = fft(a)*conj(fft(b)), cor = IFFT(COR)
	// cepstrum = IFFT(log(mag(COR)**2))
	double *cept_real = aud_real, *cept_imag = aud_imag; // do in place
	fft->do_fft(window_size, 1, aud_real, 0, cept_real, cept_imag);
	fft->do_fft(window_size, 1, env_real, env_imag, cor, aud_imag);
//dfile_dump(TMPDIR "cor",cor,half_window);
	int damp = config.damp; // apply dampening
	if( ++time_frames < damp ) damp = time_frames;
	float wt = 1./damp, wt1 = 1. - wt;
	float *dp = data->get_buffer();
	for( int i=0; i<half_window; ++i ) {
		double v = cept_real[i] * cor[i];
		dp[i] = time_frame[i] = (wt1*time_frame[i] + wt*v);
	}
//ffile_dump(TMPDIR "time_frame",time_frame,half_window);
}

void EchoCancel::dfile_dump(const char *fn, double *dp, int n)
{
	FILE *fp = fopen(fn,"w");
	if( !fp ) return;
	for( int i=0; i<n; ++i ) fprintf(fp,"%f\n",dp[i]);
	fclose(fp);
}

void EchoCancel::ffile_dump(const char *fn, float *dp, int n)
{
	FILE *fp = fopen(fn,"w");
	if( !fp ) return;
	for( int i=0; i<n; ++i ) fprintf(fp,"%f\n",dp[i]);
	fclose(fp);
}

void EchoCancel::calculate_envelope(int sample_rate, int peaks, int bsz)
{
	int cutoff = sample_rate/config.cutoff;
	bsz |= 1; // should be odd
	int bsz2 = bsz/2+1;
	double power = 0;
	for( int i=bsz2; --i>0; ) power += 2*cor[i];
	power += cor[0];
	int damp = config.damp; // apply dampening
	if( ++time_frames < damp ) damp = time_frames;
	float wt = 1./damp, wt1 = 1. - wt;
	for( int i=0; i<half_window; ++i ) envelope[i] *= wt1;
	if( power < 1e-12 ) return;
	double *dp = env_real, *ep = dp;
        for( int i=0; i<half_window; ++i ) *ep++ = time_frame[i];
//printf("envelope %.4f:\n", power);
	while( --peaks >= 0 ) {
		int x = cutoff;
       		double *sp = dp+x, *mp = sp, *xp = sp;
		double sx = 0;  // first bsz band
		for( int i=bsz; --i>=0; ) sx += *xp++;
		double mx = sx;
		while( xp < ep ) {
			sx += *xp++ - *sp++;
			if( sx > mx ) { mx = sx;  mp = sp; }
		}
		x = mp - dp;
		double echo_signal = 0, *cp = cor+x;
		for( int i=0; i<bsz; ++i ) echo_signal += cp[i];
//printf("   %d(%.3f/%.3f):\n", x, mx, echo_signal/power);
		// positive reflections only, not too small
		if( echo_signal/power < 0.001 ) break;
		double gain[bsz];
		int ret = toeplitz(bsz, cor, cor, cp, gain);
		if( !ret ) {
			double ss = 0;
			for( int i=0; i<bsz; ++i ) ss += sqr(gain[i]);
			double rms = sqrt(ss / bsz);
//printf("rms=%f ",rms);
			if( rms > 2 ) ret = 1;   // too wacky
		}
		if( ret ) { // misbehaved, use single pulse in band center
//printf("** ");
			memset(gain, 0, bsz*sizeof(gain[0]));
			gain[bsz2] = echo_signal / power;
		}
//double g=0;
		for( int i=0; i<bsz; ++i,++x ) {
			envelope[x] += wt * gain[i];
//printf("[%d]=%.3f,", x, envelope[x]);  g += envelope[x];
		}
//printf(" : %.4f\n",g);
		if( !peaks ) break;
		// remove echo from search, prevent periodic reverbs
		for(int k=x; k<half_window; k+=x ) {
			// clear this band and adj bands
			int j = k - bsz, n = 3*bsz;
			if( j < 0 ) { n += j;  j = 0; }
			if( j+n > half_window ) n = half_window - j;
			for( int i=n; --i>=0; ++j ) dp[j] = 0;
		}
	}
//printf(" ==\n");
}

void EchoCancel::create_envelope(int sample_rate)
{
	memset(envelope,0,half_window*sizeof(envelope[0]));
	int offset = config.offset;
	if( offset < 0 || offset >= half_window ) return;
	double gain = config.gain;
	if( gain < 0 || gain >= 1 ) return;
	envelope[offset] = gain;
}

int EchoCancel::cancel_echo(double *bp, int size)
{
	int n = half_window + size;
	if( n < window_size ) n = window_size;
	if( audio_buffer->buffer_size() < n ) return 1;
	int k = 0;
	// a = audio data, t = env data: convolution = ifft(fft(a)*conj(fft(t))),
	// but ifft(fft(a)*conj(fft(t)))==ifft(fft(a)*fft([t[0]]+t[:0:-1]))
	// removing conj reverses t, as echos are from the past
	if( config.mode == CANCEL_MAN ) create_envelope(sample_rate);
	for( int i=0; i<half_window; ++i ) env_data[k++] = envelope[i];
	while( k < window_size ) env_data[k++] = 0;
	fft->do_fft(window_size, 0, env_data, 0, env_real, env_imag);

	n = half_window + size;
	if( n < window_size ) n = window_size;
	double *ap = audio_buffer->get_buffer(-n);
	fft->do_fft(window_size, 0, ap, 0, aud_real, aud_imag);
	cx_product(window_size, 1, aud_real, aud_imag,
		aud_real, aud_imag, env_real, env_imag);
	fft->do_fft(window_size, 1, aud_real, aud_imag);
	n = size;
	if( n > half_window ) n = half_window;
	double *sp = &aud_real[window_size - n];
	for( int i=n; --i>=0; ++bp,++sp ) *bp -= *sp;

	return 0;
}

void EchoCancel::delete_buffers()
{
	delete [] cor;		cor = 0;
	delete [] aud_real;	aud_real = 0;
	delete [] aud_imag;	aud_imag = 0;
	delete [] envelope;	envelope = 0;
	delete [] env_real;	env_real = 0;
	delete [] env_imag;	env_real = 0;
	delete [] env_data;	env_data = 0;
	delete [] time_frame;	time_frame = 0;
}

void EchoCancel::alloc_buffers(int fft_sz)
{
	window_size = fft_sz;
	half_window = window_size / 2;
	cor = new double[window_size];
	aud_real = new double[window_size];
	aud_imag = new double[window_size];
	envelope = new double[half_window];
	env_real = new double[window_size];
	env_imag = new double[window_size];
	env_data = new double[window_size];
	time_frame = new float[half_window];
}

int EchoCancel::process_buffer(int64_t size, Samples *buffer,
		int64_t start_position, int sample_rate)
{
// Pass through
	read_samples(buffer, 0, sample_rate, start_position, size);
// Reset audio buffer
	load_configuration();
	int fft_sz = config.window_size;
	if( !fft_sz ) fft_sz = 1<<FFT::samples_to_bits(sample_rate);
	if( window_size != fft_sz ) {
		render_stop();
		delete_buffers();
		alloc_buffers(fft_sz);
		time_frames = 0;
	}
	if( !time_frames ) {
		memset(envelope, 0, half_window*sizeof(envelope[0]));
		memset(time_frame, 0, half_window*sizeof(time_frame[0]));
	}
	if( !data ) data = new DataBuffer(window_size);
	if( !audio_buffer ) audio_buffer = new AudioBuffer(window_size+2*size);
	if( !fft ) fft = new FFT;

// Accumulate audio
	int past_audio = audio_buffer->buffer_size() - window_size;
	if( past_audio > 0 ) audio_buffer->remove(past_audio);
	audio_buffer->append(buffer->get_data(), size);

// process half_windows
	double *bp = buffer->get_data();
	int total_windows = 0, audio_size = audio_buffer->buffer_size();
	while( audio_size >= window_size ) {
		int sample_offset = half_window * total_windows++;
		data->set_buffer(sample_offset, half_window);
		double *audio = audio_buffer->get_buffer(-audio_size);
		cepstrum(audio);
		if( config.mode == CANCEL_ON )
			calculate_envelope(sample_rate, config.peaks, 7);
		if( config.mode != CANCEL_OFF && size > 0 )
			cancel_echo(bp, size);
		bp += half_window;
		size -= half_window;
		audio_size -= half_window;
	}

	DataHeader *data_header = data->get_header();
	data_header->window_size = window_size;
	data_header->interrupted = interrupted;
	data_header->sample_rate = sample_rate;
	data_header->total_windows = total_windows;
	data_header->level = DB::fromdb(config.level); // Linear output level
	send_render_gui(data_header, data_header->size());
	interrupted = 0;
	return 0;
}

void EchoCancel::render_stop()
{
	interrupted = 1;
	if( audio_buffer ) audio_buffer->set_buffer(0, 0);
	if( data ) { delete data;  data = 0; }
}



NEW_WINDOW_MACRO(EchoCancel, EchoCancelWindow)


void EchoCancelCanvas::
draw_frame(float *data, int len, int hh)
{
	int w = get_w(), h = get_h();
	float y = data[0];
	int h1 = h-1;
	int y1 = hh - hh*y;
	for(int x1=0,x2=1; x2<w; ++x2 ) {
		if( x2 < len ) y = data[x2];
		int y2 = hh - hh*y;
		CLAMP(y2, 0, h1);
		draw_line(x1, y1, x2, y2);
		x1 = x2;  y1 = y2;
	}
}

void EchoCancel::update_gui()
{
	if( !thread ) return;
	EchoCancelWindow *window = (EchoCancelWindow*)thread->get_window();
	window->lock_window("EchoCancel::update_gui");
	if( load_configuration() )
		window->update_gui();
// Shift frames into history
	int new_frames = 0;
	while( frame_buffer.size() > 0 ) {
		while( frame_history.size() >= config.history_size )
			frame_history.remove_object_number(0);
		frame_history.append(frame_buffer[0]);
		frame_buffer.remove_number(0);
		++new_frames;
	}

	if( new_frames > 0 ) {
		int last_frame = frame_history.size()-1;
// Draw frames from history
		EchoCancelCanvas *canvas = (EchoCancelCanvas*)window->canvas;
		canvas->clear_box(0, 0, canvas->get_w(), canvas->get_h());
		canvas->set_line_width(1);
		for( int frame=0; frame<last_frame; ++frame ) {
			int luma = 0x80 * (frame+1)/last_frame + 0x40;
			canvas->set_color(luma*0x010101);
			EchoCancelFrame *frm = frame_history[frame];
			canvas->draw_frame(frm->samples(), frm->size(), h/2);
		}
		canvas->set_line_width(2);
		canvas->set_color(WHITE);
		EchoCancelFrame *frm = frame_history[last_frame];
		canvas->draw_frame(frm->samples(), frm->size(), h/2);
		canvas->set_color(RED);
		canvas->draw_line(frm->cut_offset,0, frm->cut_offset,10);
// Recompute probe level
		int do_overlay = canvas->is_dragging() ? 1 : 0;
		window->calculate_frequency(window->probe_x, window->probe_y, do_overlay);
		canvas->flash();
	}

	window->unlock_window();
}

void EchoCancel::render_gui(void *data, int size)
{
	if( !thread ) return;
	DataHeader *data_header = (DataHeader *)data;
	memcpy(&header, data_header, sizeof(header));
	if( window_size != data_header->window_size ) {
		window_size = data_header->window_size;
		half_window = window_size / 2;
		frame_buffer.remove_all_objects();
		frame_history.remove_all_objects();
	}
	if( data_header->interrupted )
		interrupted = 1;
	EchoCancelWindow *window = (EchoCancelWindow *)thread->get_window();
	int pixels = window->canvas->get_w();
	float window_scale = (double)half_window / (pixels * config.xzoom);
	int nwin = data_header->total_windows;
	int iwin = nwin - config.history_size;
	if( iwin < 0 ) iwin = 0;
	for( ; iwin < nwin; ++iwin ) {
                float *samples = data_header->samples + half_window*iwin;
		EchoCancelFrame *frm = new EchoCancelFrame(pixels);
		float *frm_data = frm->samples();
		double cut_period = (double)data_header->sample_rate / config.cutoff;
		frm->cut_offset = cut_period / window_scale;

		for(int i = 0; i < pixels; i++) {
			float start = i*window_scale, stop = start+window_scale;
			int istart = (int)start, istop = (int)stop;
			float fstart = start-istart, fstop = stop-istop;
			fstart = istart == istop ? -fstart : 1 - fstart;
			float sum = samples[istart++] * fstart;
			while( istart < istop ) sum += samples[istart++];
			if( fstop > 1e-10 ) sum += samples[istop] * fstop;
			frm_data[i] = sum/window_scale;
		}
// Normalize
		float scale = data_header->level;
		if( config.normalize ) {
			float max = 0;
			for( int i=frm->cut_offset; i<pixels; ++i ) {
				float dat = fabsf(frm_data[i]);
				if( dat > max ) max = dat;
			}
			scale = max > 1e-10 ? scale / max : 1;
		}
		if( scale != 1 ) frm->rescale(scale);
		while( frame_buffer.size() >= config.history_size )
				frame_buffer.remove_object_number(0);
		frame_buffer.append(frm);
	}

	timer->update();
}

int EchoCancel::load_configuration()
{
        KeyFrame *prev_keyframe = get_prev_keyframe(get_source_position());
        EchoCancelConfig old_config;
        old_config.copy_from(config);
        read_data(prev_keyframe);
        return !old_config.equivalent(config) ? 1 : 0;
}

void EchoCancel::read_data(KeyFrame *keyframe)
{
	int result;
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while(!(result = input.read_tag()) ) {
		if( !input.tag.title_is("ECHOCANCEL")) continue;
		config.level = input.tag.get_property("LEVEL", config.level);
		config.normalize = input.tag.get_property("NORMALIZE", config.normalize);
		config.window_size = input.tag.get_property("WINDOW_SIZE", config.window_size);
		config.xzoom = input.tag.get_property("XZOOM", config.xzoom);
		config.peaks = input.tag.get_property("PEAKS", config.peaks);
		config.mode = input.tag.get_property("MODE", config.mode);
		config.damp = input.tag.get_property("DAMP", config.damp);
		config.history_size = input.tag.get_property("HISTORY_SIZE", config.history_size);
		config.gain = input.tag.get_property("GAIN", config.gain);
		config.offset = input.tag.get_property("OFFSET", config.offset);
		if( !is_defaults() ) continue;
		w = input.tag.get_property("W", w);
		h = input.tag.get_property("H", h);
	}
}

void EchoCancel::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("ECHOCANCEL");
	output.tag.set_property("LEVEL", config.level);
	output.tag.set_property("NORMALIZE", config.normalize);
	output.tag.set_property("WINDOW_SIZE", config.window_size);
	output.tag.set_property("MODE", config.mode);
	output.tag.set_property("XZOOM", config.xzoom);
	output.tag.set_property("PEAKS", config.peaks);
	output.tag.set_property("DAMP", config.damp);
	output.tag.set_property("HISTORY_SIZE", config.history_size);
	output.tag.set_property("GAIN", config.gain);
	output.tag.set_property("OFFSET", config.offset);
	output.tag.set_property("W", w);
	output.tag.set_property("H", h);
	output.append_tag();
	output.tag.set_title("/ECHOCANCEL");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}





