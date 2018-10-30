/*
DeScratch - Scratches Removing Filter
Plugin for Avisynth 2.5
Copyright (c)2003-2016 Alexander G. Balakhnin aka Fizick
bag@hotmail.ru  http://avisynth.org.ru

This program is FREE software under GPL licence v2.

This plugin removes vertical scratches from digitized films.
Reworked for cin5 by GG. 03/2018, from the laws of Fizick's
Adapted strategy to mark, test, draw during port.
*/


#include "clip.h"
#include "filexml.h"
#include "language.h"
#include "descratch.h"

#include <math.h>

REGISTER_PLUGIN(DeScratchMain)

DeScratchMain::DeScratchMain(PluginServer *server)
 : PluginVClient(server)
{
	inf = 0;  sz_inf = 0;
	src = 0;  dst = 0;
	tmpy = 0; blury = 0;
	overlay_frame = 0;
}

DeScratchMain::~DeScratchMain()
{
	delete [] inf;
	delete src;
	delete dst;
	delete blury;
	delete tmpy;
	delete overlay_frame;
}

const char* DeScratchMain::plugin_title() { return N_("DeScratch"); }
int DeScratchMain::is_realtime() { return 1; }

void DeScratchConfig::reset()
{
	threshold = 12;
	asymmetry = 25;
	min_width = 1;
	max_width = 2;
	min_len = 10;
	max_len = 100;
	max_angle = 5;
	blur_len = 2;
	gap_len = 0;
	mode_y = MODE_ALL;
	mode_u = MODE_NONE;
	mode_v = MODE_NONE;
	mark = 0;
	ffade = 100;
	border = 2;
	edge_only = 0;
}

DeScratchConfig::DeScratchConfig()
{
	reset();
}

DeScratchConfig::~DeScratchConfig()
{
}

int DeScratchConfig::equivalent(DeScratchConfig &that)
{
	return threshold == that.threshold &&
		asymmetry == that.asymmetry &&
		min_width == that.min_width &&
		max_width == that.max_width &&
		min_len == that.min_len &&
		max_len == that.max_len &&
		max_angle == that.max_angle &&
		blur_len == that.blur_len &&
		gap_len == that.gap_len &&
		mode_y == that.mode_y &&
		mode_u == that.mode_u &&
		mode_v == that.mode_v &&
		mark == that.mark &&
		ffade == that.ffade &&
		border == that.border &&
		edge_only == that.edge_only;
}
void DeScratchConfig::copy_from(DeScratchConfig &that)
{
	threshold = that.threshold;
	asymmetry = that.asymmetry;
	min_width = that.min_width;
	max_width = that.max_width;
	min_len = that.min_len;
	max_len = that.max_len;
	max_angle = that.max_angle;
	blur_len = that.blur_len;
	gap_len = that.gap_len;
	mode_y = that.mode_y;
	mode_u = that.mode_u;
	mode_v = that.mode_v;
	mark = that.mark;
	ffade = that.ffade;
	border = that.border;
	edge_only = that.edge_only;
}

void DeScratchConfig::interpolate(DeScratchConfig &prev, DeScratchConfig &next,
		int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

LOAD_CONFIGURATION_MACRO(DeScratchMain, DeScratchConfig)

void DeScratchMain::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
// Store data
	output.tag.set_title("DESCRATCH");
	output.tag.set_property("THRESHOLD", config.threshold);
	output.tag.set_property("ASYMMETRY", config.asymmetry);
	output.tag.set_property("MIN_WIDTH", config.min_width);
	output.tag.set_property("MAX_WIDTH", config.max_width);
	output.tag.set_property("MIN_LEN", config.min_len);
	output.tag.set_property("MAX_LEN", config.max_len);
	output.tag.set_property("MAX_ANGLE", config.max_angle);
	output.tag.set_property("BLUR_LEN", config.blur_len);
	output.tag.set_property("GAP_LEN", config.gap_len);
	output.tag.set_property("MODE_Y", config.mode_y);
	output.tag.set_property("MODE_U", config.mode_u);
	output.tag.set_property("MODE_V", config.mode_v);
	output.tag.set_property("MARK", config.mark);
	output.tag.set_property("FFADE", config.ffade);
	output.tag.set_property("BORDER", config.border);
	output.tag.set_property("EDGE_ONLY", config.edge_only);
	output.append_tag();
	output.tag.set_title("/DESCRATCH");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
}

void DeScratchMain::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	int result = 0;

	while( !(result = input.read_tag()) ) {
		if(input.tag.title_is("DESCRATCH")) {
			config.threshold = input.tag.get_property("THRESHOLD", config.threshold);
			config.asymmetry = input.tag.get_property("ASYMMETRY", config.asymmetry);
			config.min_width = input.tag.get_property("MIN_WIDTH", config.min_width);
			config.max_width = input.tag.get_property("MAX_WIDTH", config.max_width);
			config.min_len	 = input.tag.get_property("MIN_LEN", config.min_len);
			config.max_len	 = input.tag.get_property("MAX_LEN", config.max_len);
			config.max_angle = input.tag.get_property("MAX_ANGLE", config.max_angle);
			config.blur_len	 = input.tag.get_property("BLUR_LEN", config.blur_len);
			config.gap_len	 = input.tag.get_property("GAP_LEN", config.gap_len);
			config.mode_y	 = input.tag.get_property("MODE_Y", config.mode_y);
			config.mode_u	 = input.tag.get_property("MODE_U", config.mode_u);
			config.mode_v	 = input.tag.get_property("MODE_V", config.mode_v);
			config.mark	 = input.tag.get_property("MARK", config.mark);
			config.ffade	 = input.tag.get_property("FFADE", config.ffade);
			config.border	 = input.tag.get_property("BORDER", config.border);
			config.edge_only = input.tag.get_property("EDGE_ONLY", config.edge_only);
		}
	}
}

void DeScratchMain::set_extrems_plane(int width, int comp, int thresh)
{
	uint8_t **rows = blury->get_rows();
	int r = width, r1 = r+1, wd = src_w - r1;
	int bpp = 3, dsz = r * bpp;
	int asym = config.asymmetry * 256 / 100;
	if( thresh > 0 ) {	// black (low value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w + r1;
			uint8_t *dp = rows[y] + r1*bpp + comp;
			for( int x=r1; x<wd; ++x,++ip,dp+=bpp ) {
				if( *ip != SD_NULL ) continue;
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				if( (lp[0]-*dp) > thresh && (rp[0]-*dp) > thresh &&
				    (abs(lp[-bpp]-rp[+bpp]) <= asym) &&
				    ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) >
				     (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) ) {
					*ip = SD_EXTREM;
					for( int i=1; i<r; ++i ) ip[i] = ip[-i] = SD_EXTREM;
				}
			}
		}
	}
	else {			// white (high value) scratches
		for( int y=0; y<src_h; ++y ) {
			uint8_t *ip = inf + y*src_w + r1;
			uint8_t *dp = rows[y] + r1*bpp + comp;
			for( int x=r1; x<wd; ++x,++ip,dp+=bpp ) {
				if( *ip != SD_NULL ) continue;
				uint8_t *lp = dp-dsz, *rp = dp+dsz;
				if( (lp[0]-*dp) < thresh && (rp[0]-*dp) < thresh &&
				    (abs(lp[-bpp]-rp[+bpp]) <= asym) &&
				    ((lp[0]-lp[+bpp]) + (rp[0]-rp[-bpp]) <
				     (lp[-bpp]-lp[0]) + (rp[bpp]-rp[0])) ) {
					*ip = SD_EXTREM;
					for( int i=1; i<r; ++i ) ip[i] = ip[-i] = SD_EXTREM;
				}
			}
		}
	}
}

//
void DeScratchMain::close_gaps()
{
	int len = config.gap_len * src_h / 100;
	for( int y=0; y<src_h; ++y ) {
		uint8_t *ip = inf + y*src_w;
		for( int x=0; x<src_w; ++x,++ip ) {
			if( !(*ip & SD_EXTREM) ) continue;
			uint8_t *bp = ip, b = *bp;		// expand to previous lines in range
			int i = len < y ? len : y;
			while( --i>=0 ) *(bp-=src_w) = b;
		}
	}
}

void DeScratchMain::test_scratches()
{
	int w2 = src_w - 2;
	int min_len = config.min_len * src_h / 100;
	int max_len = config.max_len * src_h / 100;
	int maxwidth = config.max_width;
	float sin_mxa = sin(config.max_angle * M_PI/180.);

	for( int y=0; y<src_h; ++y ) {
		for( int x=2; x<w2; ++x ) {
			int ofs = y*src_w + x;			// first candidate
			if( !(inf[ofs] & SD_EXTREM) ) continue;
			int ctr = ofs, nctr = ctr;
			int hy = src_h - y, len;
			for( len=0; len<hy; ++len ) {		// check vertical aspect
				uint8_t *ip = inf + ctr;
				int n = 0;			// number good points in row
				if( ip[-1] & SD_EXTREM ) { ip[-1] |= SD_TESTED; nctr = ctr-1; ++n; }
				if( ip[+1] & SD_EXTREM ) { ip[+1] |= SD_TESTED; nctr = ctr+1; ++n; }
				if( ip[+0] & SD_EXTREM ) { ip[+0] |= SD_TESTED; nctr = ctr+0; ++n; }
				if( !n ) break;
				ctr = nctr + src_w;		// new center for next row test
			}
			int v = (!config.edge_only || y == 0 || y+len>=src_h) &&
				abs(nctr%src_w - x) < maxwidth + len*sin_mxa &&
				len >= min_len && len <= max_len ? SD_GOOD : SD_REJECT;
			ctr = ofs; nctr = ctr;			// pass2
			for( len=0; len<hy; ++len ) {		// cycle along inf
				uint8_t *ip = inf + ctr;
				int n = 0;			// number good points in row
				if( ip[-1] & SD_TESTED ) { ip[-1] = v; nctr = ctr-1; ++n; }
				if( ip[+1] & SD_TESTED ) { ip[+1] = v; nctr = ctr+1; ++n; }
				if( ip[+0] & SD_TESTED ) { ip[+0] = v; nctr = ctr+0; ++n; }
				if( !n ) break;
				ctr = nctr + src_w;		// new center for next row test
			}
		}
	}
}

void DeScratchMain::mark_scratches_plane()
{
	int bpp = 3, dst_w = dst->get_w(), dst_h = dst->get_h();
	uint8_t **rows = dst->get_rows();
	for( int y=0; y<dst_h; ++y ) {
		uint8_t *ip = inf + y*src_w;
		for( int x=0; x<dst_w; ++x,++ip ) {
			if( *ip == SD_NULL ) continue;
			static uint8_t grn_yuv[3] = { 0xad, 0x28, 0x19, };
			static uint8_t ylw_yuv[3] = { 0xdb, 0x0f, 0x89, };
			static uint8_t red_yuv[3] = { 0x40, 0x66, 0xef, };
			for( int comp=0; comp<3; ++comp ) {
				uint8_t *dp = rows[y] + comp + x*bpp;
				if( *ip & SD_GOOD )
					*dp = config.threshold > 0 ? grn_yuv[comp] : red_yuv[comp];
				else if( *ip & SD_REJECT )
					*dp = ylw_yuv[comp];
			}
		}
	}
}

void DeScratchMain::remove_scratches_plane(int comp)
{
	int bpp = 3, w1 = src_w-1;
	int border = config.border;
	uint8_t *ip = inf;
	uint8_t **dst_rows = dst->get_rows();
	uint8_t **blur_rows = blury->get_rows();
	float a = config.ffade / 100, b = 1 - a;

	for( int y=0; y<src_h; ++y ) {
		int left = -1;
		uint8_t *out = dst_rows[y] + comp;
		uint8_t *blur = blur_rows[y] + comp;
		for( int x=1; x<w1; ++x ) {
			uint8_t *dp = ip + x;
			if( !(dp[-1]&SD_GOOD) && (dp[0]&SD_GOOD) ) left = x;
			if( left < 0 || !(dp[0]&SD_GOOD) || (dp[1]&SD_GOOD) ) continue;
			int right = x;
			int ctr = (left + right) / 2;			// scratch center
			int r = (right - left + border) / 2 + 1;
			left = 0;
			int ls = ctr - r, rs = ctr + r;			// scratch edges
			int lt = ls - border, rt = rs + border;		// border edges
			if( ls < 0 ) ls = 0;
			if( rs > w1 ) rs = w1;
			if( lt < 0 ) lt = 0;
			if( rt > w1 ) rt = w1;
			ls *= bpp;  rs *= bpp;
			lt *= bpp;  rt *= bpp;
			if( rs > ls ) {
				float s = 1. / (rs - ls);
				for( int i=ls; (i+=bpp)<rs; ) {		// across the scratch
					int lv = a * blur[ls] + b * out[i];
					int rv = a * blur[rs] + b * out[i];
					int v = s * (lv*(rs-i) + rv*(i-ls));
					out[i] = CLIP(v, 0, 255);
				}
			}
			if( !border ) continue;
			if( ls > lt ) {
				float s = 1. / (ls - lt);
				for( int i=lt; (i+=bpp)<=ls; ) {	// at left border
					int lv = a * out[lt] + b * out[i];
					int rv = a * blur[i] + b * out[i];
					int v = s * (lv*(ls-i) + rv*(i-lt));
					out[i] = CLIP(v, 0, 255);
				}
			}
			if( rt > rs ) {
				float s = 1. / (rt - rs);
				for( int i=rt; (i-=bpp)>=rs; ) {	// at right border
					int lv = a * blur[i] + b * out[i];
					int rv = a * out[rt] + b * out[i];
					int v = s * (rv*(i-rs) + lv*(rt-i));
					out[i] = CLIP(v, 0, 255);
				}
			}
		}
		ip += src_w;
	}
}

void DeScratchMain::blur(int scale)
{
	int th = (src_h / scale) & ~1;
	if( tmpy&& (tmpy->get_w() != src_w || tmpy->get_h() != th) ) {
		delete tmpy;  tmpy= 0;
	}
	if( !tmpy ) tmpy = new VFrame(src_w, th, BC_YUV888);
	if( blury && (blury->get_w() != src_w || blury->get_h() != src_h) ) {
		delete blury;  blury= 0;
	}
	if( !blury ) blury = new VFrame(src_w, src_h, BC_YUV888);
	overlay_frame->overlay(tmpy, src,
		0,0,src_w,src_h, 0,0,src_w,th, 1.f, TRANSFER_NORMAL, LINEAR_LINEAR);
	overlay_frame->overlay(blury, tmpy,
		0,0,src_w,th, 0,0,src_w,src_h, 1.f, TRANSFER_NORMAL, CUBIC_CUBIC);
}

void DeScratchMain::copy(int comp)
{
	uint8_t **src_rows = src->get_rows();
	uint8_t **dst_rows = dst->get_rows();
	for( int y=0; y<src_h; ++y ) {
		uint8_t *sp = src_rows[y] + comp, *dp = dst_rows[y] + comp;
		for( int x=0; x<src_w; ++x,sp+=3,dp+=3 ) *sp = *dp;
	}
}

void DeScratchMain::pass(int comp, int thresh)
{
// pass for current plane and current sign
	int w0 = config.min_width, w1 = config.max_width;
	if( w1 < w0 ) w1 = w0;
	for( int iw=w0; iw<=w1; ++iw )
		set_extrems_plane(iw, comp, thresh);
}

void DeScratchMain::plane_pass(int comp, int mode)
{
	int threshold = config.threshold;
	if( comp != 0 ) threshold /= 2;  // fakey UV scaling
	switch( mode ) {
	case MODE_ALL:
		pass(comp, threshold);
		copy(comp);
	case MODE_HIGH:		// fall thru
		threshold = -threshold;
	case MODE_LOW:		// fall thru
		pass(comp, threshold);
		break;
	}
}

void DeScratchMain::plane_proc(int comp, int mode)
{
	if( mode == MODE_NONE ) return;
	remove_scratches_plane(comp);
}

int DeScratchMain::process_realtime(VFrame *input, VFrame *output)
{
	load_configuration();
	src_w = input->get_w();
	src_h = input->get_h();
	if( src_w >= 2*config.max_width+3 ) {
		if( !overlay_frame ) {
			int cpus = PluginClient::smp + 1;
			if( cpus > 8 ) cpus = 8;
			overlay_frame = new OverlayFrame(cpus);
		}
		if( src && (src->get_w() != src_w || src->get_h() != src_h) ) {
			delete src;  src = 0;
		}
		if( !src ) src = new VFrame(src_w, src_h, BC_YUV888);
		src->transfer_from(input);
		if( dst && (dst->get_w() != src_w || dst->get_h() != src_h) ) {
			delete dst;  dst = 0;
		}
		if( !dst ) dst = new VFrame(src_w, src_h, BC_YUV888);
		dst->copy_from(src);
		int sz = src_w * src_h;
		if( sz_inf != sz ) {  delete [] inf;  inf = 0; }
		if( !inf ) inf = new uint8_t[sz_inf=sz];
		blur(config.blur_len + 1);
		memset(inf, SD_NULL, sz_inf);
		plane_pass(0, config.mode_y);
		plane_pass(1, config.mode_u);
		plane_pass(2, config.mode_v);
		close_gaps();
		test_scratches();
		if( !config.mark ) {
			plane_proc(0, config.mode_y);
			plane_proc(1, config.mode_u);
			plane_proc(2, config.mode_v);
		}
		else
			mark_scratches_plane();
		output->transfer_from(dst);
	}
	return 0;
}

void DeScratchMain::update_gui()
{
	if( !thread ) return;
	DeScratchWindow *window = (DeScratchWindow *)thread->get_window();
	window->lock_window("DeScratchMain::update_gui");
	if( load_configuration() )
		window->update_gui();
	window->unlock_window();
}

NEW_WINDOW_MACRO(DeScratchMain, DeScratchWindow)


DeScratchWindow::DeScratchWindow(DeScratchMain *plugin)
 : PluginClientWindow(plugin, 512, 270, 512, 270, 0)
{
	this->plugin = plugin;
}

DeScratchWindow::~DeScratchWindow()
{
}

void DeScratchWindow::create_objects()
{
	int x = 10, y = 10;
	plugin->load_configuration();
	DeScratchConfig &config = plugin->config;

	BC_Title *title;
	add_tool(title = new BC_Title(x, y, _("DeScratch:")));

	int w1 = DeScratchReset::calculate_w(this, _("Reset"));
	add_tool(reset = new DeScratchReset(this, get_w()-w1-15, y));

	y += title->get_h() + 15;
	int x1 = x, x2 = get_w()/2;
	add_tool(title = new BC_Title(x1=x, y, _("threshold:")));
	x1 += title->get_w()+16;
	add_tool(threshold = new DeScratchISlider(this, x1, y, x2-x1-10, 0,64, &config.threshold));
	add_tool(title = new BC_Title(x1=x2, y, _("asymmetry:")));
	x1 += title->get_w()+16;
	add_tool(asymmetry = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0,100., &config.asymmetry));
	y += threshold->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("Mode:")));
	x1 += title->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("y:")));
	w1 = title->get_w()+16;
	add_tool(y_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_y));
	y_mode->create_objects();  x1 += y_mode->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("u:")));
	add_tool(u_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_u));
	u_mode->create_objects();  x1 += u_mode->get_w()+16;
	add_tool(title = new BC_Title(x1, y, _("v:")));
	add_tool(v_mode = new DeScratchMode(this, (x1+=w1), y, &config.mode_v));
	v_mode->create_objects();
	y += y_mode->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("width:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("min:")));
	x1 += title->get_w()+16;
	add_tool(min_width = new DeScratchISlider(this, x1, y, x2-x1-10, 1,16, &config.min_width));
	add_tool(title = new BC_Title(x1=x2, y, _("max:")));
	x1 += title->get_w()+16;
	add_tool(max_width = new DeScratchISlider(this, x1, y, get_w()-x1-15, 1,16, &config.max_width));
	y += min_width->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("len:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("min:")));
	x1 += title->get_w()+16;
	add_tool(min_len = new DeScratchFSlider(this, x1, y, x2-x1-10, 0.0,100.0, &config.min_len));
	add_tool(title = new BC_Title(x1=x2, y, _("max:")));
	x1 += title->get_w()+16;
	add_tool(max_len = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.max_len));
	y += min_len->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("len:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(title = new BC_Title(x1, y, _("blur:")));
	x1 += title->get_w()+16;
	add_tool(blur_len = new DeScratchISlider(this, x1, y, x2-x1-10, 0,8, &config.blur_len));
	add_tool(title = new BC_Title(x1=x2, y, _("gap:")));
	x1 += title->get_w()+16;
	add_tool(gap_len = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.gap_len));
	y += blur_len->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("max angle:")));
	w1 = title->get_w()+16;  x1 += w1;
	add_tool(max_angle = new DeScratchFSlider(this, x1, y, x2-x1-10, 0.0,15.0, &config.max_angle));
	add_tool(title = new BC_Title(x1=x2, y, _("fade:")));
	x1 += title->get_w()+16;
	add_tool(ffade = new DeScratchFSlider(this, x1, y, get_w()-x1-15, 0.0,100.0, &config.ffade));
	y += max_angle->get_h() + 10;

	add_tool(title = new BC_Title(x1=x, y, _("border:")));
	x1 += title->get_w()+16;
	add_tool(border = new DeScratchISlider(this, x1, y, x2-x1-10, 0,16, &config.border));
	add_tool(mark = new DeScratchMark(this, x1=x2, y));
	x1 += mark->get_w() + 10;
	add_tool(edge_only = new DeScratchEdgeOnly(this, x1, y));

	show_window();
}

void DeScratchWindow::update_gui()
{
	DeScratchConfig &config = plugin->config;
	threshold->update(config.threshold);
	asymmetry->update(config.asymmetry);
	min_width->update(config.min_width);
	max_width->update(config.max_width);
	min_len->update(config.min_len);
	max_len->update(config.max_len);
	max_angle->update(config.max_angle);
	blur_len->update(config.blur_len);
	gap_len->update(config.gap_len);
	y_mode->update(config.mode_y);
	u_mode->update(config.mode_u);
	v_mode->update(config.mode_v);
	mark->update(config.mark);
	ffade->update(config.ffade);
	border->update(config.border);
	edge_only->update(config.edge_only);
}


DeScratchModeItem::DeScratchModeItem(DeScratchMode *popup, int type, const char *text)
 : BC_MenuItem(text)
{
	this->popup = popup;
	this->type = type;
}

DeScratchModeItem::~DeScratchModeItem()
{
}

int DeScratchModeItem::handle_event()
{
	popup->update(type);
	return popup->handle_event();
}

DeScratchMode::DeScratchMode(DeScratchWindow *win, int x, int y, int *value)
 : BC_PopupMenu(x, y, 64, "", 1)
{
	this->win = win;
	this->value = value;
}

DeScratchMode::~DeScratchMode()
{
}

void DeScratchMode::create_objects()
{
	add_item(new DeScratchModeItem(this, MODE_NONE, _("None")));
	add_item(new DeScratchModeItem(this, MODE_LOW,  _("Low")));
	add_item(new DeScratchModeItem(this, MODE_HIGH, _("High")));
	add_item(new DeScratchModeItem(this, MODE_ALL,  _("All")));
	set_value(*value);
}

int DeScratchMode::handle_event()
{
	win->plugin->send_configure_change();
	return 1;
}

void DeScratchMode::update(int v)
{
	set_value(*value = v);
}

void DeScratchMode::set_value(int v)
{
	int i = total_items();
	while( --i >= 0 && ((DeScratchModeItem*)get_item(i))->type != v );
	if( i >= 0 ) set_text(get_item(i)->get_text());
}

DeScratchISlider::DeScratchISlider(DeScratchWindow *win,
		int x, int y, int w, int min, int max, int *output)
 : BC_ISlider(x, y, 0, w, w, min, max, *output)
{
        this->win = win;
        this->output = output;
}

DeScratchISlider::~DeScratchISlider()
{
}

int DeScratchISlider::handle_event()
{
        *output = get_value();
        win->plugin->send_configure_change();
        return 1;
}

DeScratchFSlider::DeScratchFSlider(DeScratchWindow *win,
		int x, int y, int w, float min, float max, float *output)
 : BC_FSlider(x, y, 0, w, w, min, max, *output)
{
        this->win = win;
        this->output = output;
}

DeScratchFSlider::~DeScratchFSlider()
{
}

int DeScratchFSlider::handle_event()
{
	*output = get_value();
	win->plugin->send_configure_change();
        return 1;
}

DeScratchMark::DeScratchMark(DeScratchWindow *win, int x, int y)
 : BC_CheckBox(x, y, &win->plugin->config.mark, _("Mark"))
{
	this->win = win;
};

DeScratchMark::~DeScratchMark()
{
}

int DeScratchMark::handle_event()
{
	int ret = BC_CheckBox::handle_event();
	win->plugin->send_configure_change();
	return ret;
}

DeScratchEdgeOnly::DeScratchEdgeOnly(DeScratchWindow *win, int x, int y)
 : BC_CheckBox(x, y, &win->plugin->config.edge_only, _("Edge"))
{
	this->win = win;
};

DeScratchEdgeOnly::~DeScratchEdgeOnly()
{
}

int DeScratchEdgeOnly::handle_event()
{
	int ret = BC_CheckBox::handle_event();
	win->plugin->send_configure_change();
	return ret;
}

DeScratchReset::DeScratchReset(DeScratchWindow *win, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->win = win;
}

int DeScratchReset::handle_event()
{
	win->plugin->config.reset();
	win->update_gui();
	win->plugin->send_configure_change();
	return 1;
}

