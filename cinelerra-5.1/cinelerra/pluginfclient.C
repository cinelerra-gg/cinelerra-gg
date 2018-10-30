#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "bcwindowbase.h"
#include "bctitle.h"
#include "cstrdup.h"
#include "language.h"
#include "mwindow.h"
#include "pluginfclient.h"
#include "pluginserver.h"
#include "samples.h"
#include "vframe.h"
#include "filexml.h"

#ifdef FFMPEG3
#define av_filter_iterate(p) ((*(const AVFilter**)(p))=avfilter_next(*(const AVFilter **)(p)))
#endif

static void ff_err(int ret, const char *fmt, ...)
{
	char msg[BCTEXTLEN];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	char errmsg[BCSTRLEN];
	av_strerror(ret, errmsg, sizeof(errmsg));
	fprintf(stderr,_("%s  err: %s\n"),msg, errmsg);
}

PluginFClientConfig::PluginFClientConfig()
{
	ffilt = 0;
}

PluginFClientConfig::~PluginFClientConfig()
{
	delete ffilt;
	remove_all_objects();
}

int PluginFClientConfig::equivalent(PluginFClientConfig &that)
{
	PluginFClientConfig &conf = *this;
	for( int i=0; i<that.size(); ++i ) {
		PluginFClient_Opt *topt = conf[i], *vopt = that[i];
		char tval[BCTEXTLEN], *tp = topt->get(tval, sizeof(tval));
		char vval[BCTEXTLEN], *vp = vopt->get(vval, sizeof(vval));
		int ret = tp && vp ? strcmp(tp, vp) : tp || vp ? 1 : 0;
		if( ret ) return 0;
	}
	return 1;
}

void PluginFClientConfig::copy_from(PluginFClientConfig &that)
{
	PluginFClientConfig &conf = *this;
	for( int i=0; i<that.size(); ++i ) {
		PluginFClient_Opt *vp = that[i];
		const char *nm = vp->opt->name;
		int k = size();
		while( --k >= 0 && strcmp(nm, conf[k]->opt->name) );
		if( k < 0 ) continue;
		PluginFClient_Opt *fopt = conf[k];
		char str[BCTEXTLEN], *sp = vp->get(str, sizeof(str));
		if( sp ) fopt->set(sp);
	}
}

void PluginFClientConfig::interpolate(PluginFClientConfig &prev, PluginFClientConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

void PluginFClientConfig::initialize(const char *name)
{
	delete ffilt;
	ffilt = PluginFFilter::new_ffilter(name);
	const AVOption *opt = 0;
	void *obj = ffilt->filter_config();

	const AVClass *filt_class = ffilt->filter_class();
	if( filt_class && filt_class->option ) {
		PluginFClientConfig &conf = *this;
		while( (opt=av_opt_next(obj, opt)) != 0 ) {
			if( opt->type == AV_OPT_TYPE_CONST ) continue;
			int dupl = 0;
			for( int i=0; !dupl && i<size(); ++i ) {
				PluginFClient_Opt *fp = conf[i];
				const AVOption *op = fp->opt;
				if( op->offset != opt->offset ) continue;
				if( op->type != opt->type ) continue;
				dupl = 1;
				if( strlen(op->name) < strlen(opt->name) )
					fp->opt = opt;
			}
			if( dupl ) continue;
			PluginFClient_Opt *fopt = new PluginFClient_Opt(this, opt);
			append(fopt);
		}
	}
	if( (ffilt->filter->flags & AVFILTER_FLAG_SLICE_THREADS) != 0 ) {
		opt = av_opt_find(ffilt->fctx, "threads", NULL, 0, 0);
		if( opt ) {
			PluginFClient_Opt *fopt = new PluginFClient_Opt(this, opt);
			append(fopt);
		}
	}
}

int PluginFClientConfig::update()
{
	int ret = 0;
	PluginFClientConfig &conf = *this;

	for( int i=0; i<size(); ++i ) {
		PluginFClient_Opt *fopt = conf[i];
		char val[BCTEXTLEN], *vp = fopt->get(val, sizeof(val));
		if( !vp || !strcmp(val, fopt->item_value->get_text()) ) continue;
		fopt->item_value->update();
		++ret;
	}
	return ret;
}

void PluginFClientConfig::dump(FILE *fp)
{
	const AVOption *opt = 0;
	const AVClass *obj = filter_class();
	PluginFClientConfig &conf = *this;

	while( (opt=av_opt_next(&obj, opt)) != 0 ) {
		if( opt->type == AV_OPT_TYPE_CONST ) continue;
		int k = size();
		while( --k >= 0 && strcmp(opt->name, conf[k]->opt->name) );
		if( k < 0 ) continue;
		PluginFClient_Opt *fopt = conf[k];
		char val[BCTEXTLEN], *vp = fopt->get(val,sizeof(val));
		fprintf(fp, "  %s:=%s", opt->name, vp);
		if( opt->unit ) {
			char unt[BCTEXTLEN], *up = unt;
			fopt->units(up);
			fprintf(fp, "%s", unt);
		}
		fprintf(fp, "\n");
	}
}

PluginFClientReset::
PluginFClientReset(PluginFClientWindow *fwin, int x, int y)
 : BC_GenericButton(x, y, _("Reset"))
{
	this->fwin = fwin;
}

PluginFClientReset::
~PluginFClientReset()
{
}

int PluginFClientReset::handle_event()
{
	fwin->ffmpeg->config.initialize(fwin->ffmpeg->name);
	if( fwin->ffmpeg->config.update() > 0 )
		fwin->draw();
	fwin->ffmpeg->plugin->send_configure_change();
	return 1;
}

PluginFClientText::
PluginFClientText(PluginFClientWindow *fwin, int x, int y, int w)
 : BC_TextBox(x, y, w, 1, (char *)"")
{
	this->fwin = fwin;
}

PluginFClientText::
~PluginFClientText()
{
}

int PluginFClientText::handle_event()
{
	return 0;
}

PluginFClientUnits::
PluginFClientUnits(PluginFClientWindow *fwin, int x, int y, int w)
 : BC_PopupMenu(x, y, w, "")
{
	this->fwin = fwin;
}

PluginFClientUnits::
~PluginFClientUnits()
{
}

int PluginFClientUnits::handle_event()
{
	const char *text = get_text();
	if( text && fwin->selected ) {
		if( text ) fwin->selected->set(text);
		fwin->selected->item_value->update();
		fwin->draw();
		fwin->ffmpeg->plugin->send_configure_change();
	}
	return 1;
}

PluginFClientApply::
PluginFClientApply(PluginFClientWindow *fwin, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->fwin = fwin;
}

PluginFClientApply::
~PluginFClientApply()
{
}

int PluginFClientApply::handle_event()
{
	return fwin->update();
}

PluginFClientPot::PluginFClientPot(PluginFClientWindow *fwin, int x, int y)
 : BC_FPot(x, y, 0.f, 0.f, 0.f)
{
	this->fwin = fwin;
}

int PluginFClient_Opt::get_range(float &fmn, float &fmx)
{
	switch( opt->type ) {
	case AV_OPT_TYPE_INT:
	case AV_OPT_TYPE_INT64:
	case AV_OPT_TYPE_DOUBLE:
	case AV_OPT_TYPE_FLOAT: break;
	default: return 1;
	}
	const AVClass *filt_class = filter_class();
	if( !filt_class || !filt_class->option ) return 1;
	AVOptionRanges *r = 0;
	void *obj = &filt_class;
	if( av_opt_query_ranges(&r, obj, opt->name, AV_OPT_SEARCH_FAKE_OBJ) < 0 ) return 1;
	int ret = 1;
	if( r->nb_ranges == 1 ) {
		fmn = r->range[0]->value_min;
		fmx = r->range[0]->value_max;
		ret = 0;
	}
	av_opt_freep_ranges(&r);
	return ret;
}

float PluginFClient_Opt::get_float()
{
	char val[BCTEXTLEN];  val[0] = 0;
	get(val, sizeof(val));
	return atof(val);
}

void PluginFClient_Opt::set_float(float v)
{
	char val[BCTEXTLEN];  val[0] = 0;
	sprintf(val, "%f", v);
	set(val);
}

int PluginFClientPot::handle_event()
{
	if( fwin->selected ) {
		fwin->selected->set_float(get_value());
		fwin->update_selected();
		return fwin->update();
	}
	return 1;
}

PluginFClientSlider::PluginFClientSlider(PluginFClientWindow *fwin, int x, int y)
 : BC_FSlider(x, y, 0, fwin->get_w()-x-20, fwin->get_w()-x-20, 0.f, 0.f, 0.f)
{
	this->fwin = fwin;
}

int PluginFClientSlider::handle_event()
{
	if( fwin->selected ) {
		fwin->selected->set_float(get_value());
		fwin->update_selected();
		return fwin->update();
	}
	return 1;
}


PluginFClient_OptPanel::
PluginFClient_OptPanel(PluginFClientWindow *fwin, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT), opts(items[0]), vals(items[1])
{
	this->fwin = fwin;
	update();  // init col/wid/columns
}

PluginFClient_OptPanel::
~PluginFClient_OptPanel()
{
}

void PluginFClient_OptPanel::create_objects()
{
	PluginFClientConfig &fconfig = fwin->ffmpeg->config;
	for( int i=0; i<fconfig.size(); ++i ) {
		PluginFClient_Opt *opt = fconfig[i];
		opts.append(opt->item_name);
		vals.append(opt->item_value);
	}
}

int PluginFClient_OptPanel::cursor_leave_event()
{
	hide_tooltip();
	return 0;
}

void PluginFClientWindow::update(PluginFClient_Opt *opt)
{
	if( selected != opt ) {
		if( selected ) selected->item_name->set_selected(0);
		selected = opt;
		if( selected ) selected->item_name->set_selected(1);
	}
	clear_box(0,0, 0,panel->get_y());
	char str[BCTEXTLEN], *sp;
	*(sp=str) = 0;
	if( opt ) opt->types(sp);
	type->update(str);
	*(sp=str) = 0;
	if( opt ) opt->ranges(sp);
	range->update(str);
	while( units->total_items() ) units->del_item(0);
	ArrayList<const AVOption *> opts;
	int n = !opt ? 0 : opt->units(opts);
	for( int i=0; i<n; ++i )
		units->add_item(new BC_MenuItem(opts[i]->name, 0));
	char unit[BCSTRLEN];  strcpy(unit, "()");
	if( units->total_items() && opt && opt->opt->unit )
		strcpy(unit, opt->opt->unit);
	units->set_text(unit);
	char val[BCTEXTLEN];  val[0] = 0;
	if( opt ) opt->get(val, sizeof(val));
	text->update(val);

	float v = 0, fmn = 0, fmx = 0;
	if( opt && !opt->get_range(fmn, fmx) ) v = atof(val);
	float p = (fmx-fmn) / slider->get_w();
	slider->set_precision(p);
	slider->update(slider->get_w(), v, fmn, fmx);
	pot->set_precision(p);
	pot->update(v, fmn, fmx);
	panel->update();
}

int PluginFClientWindow::update()
{
	const char *txt = text->get_text();
	if( txt && selected ) {
		selected->set(txt);
		selected->item_value->update();
		draw();
		ffmpeg->plugin->send_configure_change();
	}
	return 1;
}

void PluginFClientWindow::update_selected()
{
	update(selected);
}

int PluginFClient_OptPanel::selection_changed()
{
	PluginFClient_Opt *opt = 0;
	BC_ListBoxItem *item = get_selection(0, 0);
	if( item ) {
		PluginFClient_OptName *opt_name = (PluginFClient_OptName *)item;
		opt = opt_name->opt;
	}
	fwin->update(opt);
	fwin->panel->set_tooltip(!opt ? 0 : opt->tip());
	fwin->panel->show_tooltip();
	return 1;
}

void *PluginFClient_Opt::filter_config()
{
	return conf->filter_config();
}
const AVClass *PluginFClient_Opt::filter_class()
{
	return conf->filter_class();
}

int PluginFClient_Opt::types(char *rp)
{
	const char *cp;
	switch (opt->type) {
	case AV_OPT_TYPE_FLAGS: cp = "<flags>";  break;
	case AV_OPT_TYPE_INT: cp = "<int>"; break;
	case AV_OPT_TYPE_INT64: cp = "<int64>"; break;
	case AV_OPT_TYPE_DOUBLE: cp = "<double>"; break;
	case AV_OPT_TYPE_FLOAT: cp = "<float>"; break;
	case AV_OPT_TYPE_STRING: cp = "<string>"; break;
	case AV_OPT_TYPE_RATIONAL: cp = "<rational>"; break;
	case AV_OPT_TYPE_BINARY: cp = "<binary>"; break;
	case AV_OPT_TYPE_IMAGE_SIZE: cp = "<image_size>"; break;
	case AV_OPT_TYPE_VIDEO_RATE: cp = "<video_rate>"; break;
	case AV_OPT_TYPE_PIXEL_FMT: cp = "<pix_fmt>"; break;
	case AV_OPT_TYPE_SAMPLE_FMT: cp = "<sample_fmt>"; break;
	case AV_OPT_TYPE_DURATION: cp = "<duration>"; break;
	case AV_OPT_TYPE_COLOR: cp = "<color>"; break;
	case AV_OPT_TYPE_CHANNEL_LAYOUT: cp = "<channel_layout>";  break;
	default: cp = "<undef>";  break;
	}
	return sprintf(rp, "%s", cp);
}
int PluginFClient_Opt::scalar(double d, char *rp)
{
	const char *cp = 0;
	     if( d == INT_MAX ) cp = "INT_MAX";
	else if( d == INT_MIN ) cp = "INT_MIN";
	else if( d == UINT32_MAX ) cp = "UINT32_MAX";
	else if( d == (double)INT64_MAX ) cp = "I64_MAX";
	else if( d == INT64_MIN ) cp = "I64_MIN";
	else if( d == FLT_MAX ) cp = "FLT_MAX";
	else if( d == FLT_MIN ) cp = "FLT_MIN";
	else if( d == -FLT_MAX ) cp = "-FLT_MAX";
	else if( d == -FLT_MIN ) cp = "-FLT_MIN";
	else if( d == DBL_MAX ) cp = "DBL_MAX";
	else if( d == DBL_MIN ) cp = "DBL_MIN";
	else if( d == -DBL_MAX ) cp = "-DBL_MAX";
	else if( d == -DBL_MIN ) cp = "-DBL_MIN";
	else if( d == 0 ) cp = signbit(d) ? "-0" : "0";
	else if( isnan(d) ) cp = signbit(d) ? "-NAN" : "NAN";
	else if( isinf(d) ) cp = signbit(d) ? "-INF" : "INF";
	else return sprintf(rp, "%g", d);
	return sprintf(rp, "%s", cp);
}

int PluginFClient_Opt::ranges(char *rp)
{
	const AVClass *filt_class = filter_class();
	if( !filt_class || !filt_class->option ) return 0;
	switch (opt->type) {
	case AV_OPT_TYPE_INT:
	case AV_OPT_TYPE_INT64:
	case AV_OPT_TYPE_DOUBLE:
	case AV_OPT_TYPE_FLOAT: break;
	default: return 0;;
	}
	AVOptionRanges *r = 0;
	void *obj = &filt_class;
	char *cp = rp;
	if( av_opt_query_ranges(&r, obj, opt->name, AV_OPT_SEARCH_FAKE_OBJ) < 0 ) return 0;
	for( int i=0; i<r->nb_ranges; ++i ) {
		cp += sprintf(cp, " (");  cp += scalar(r->range[i]->value_min, cp);
		cp += sprintf(cp, "..");  cp += scalar(r->range[i]->value_max, cp);
		cp += sprintf(cp, ")");
	}
	av_opt_freep_ranges(&r);
	return cp - rp;
}

int PluginFClient_Opt::units(ArrayList<const AVOption *> &opts)
{
	if( !this->opt->unit ) return 0;
	const AVClass *filt_class = filter_class();
	if( !filt_class || !filt_class->option ) return 0;
	void *obj = &filt_class;
	const AVOption *opt = NULL;
	while( (opt=av_opt_next(obj, opt)) != 0 ) {
		if( !opt->unit ) continue;
		if( opt->type != AV_OPT_TYPE_CONST ) continue;
		if( strcmp(this->opt->unit, opt->unit) ) continue;
		int i = opts.size();
		while( --i >= 0 ) {
			if( opts[i]->default_val.i64 != opt->default_val.i64 ) continue;
			if( strlen(opts[i]->name) < strlen(opt->name) ) opts[i] = opt;
			break;
		}
		if( i < 0 )
			opts.append(opt);
	}
	return opts.size();
}

int PluginFClient_Opt::units(char *rp)
{
	ArrayList<const AVOption *> opts;
	int n = units(opts);
	if( !n ) return 0;
	char *cp = rp;
	cp += sprintf(cp, " [%s:", this->opt->unit);
	for( int i=0; i<n; ++i )
		cp += sprintf(cp, " %s", opts[i]->name);
	cp += sprintf(cp, "]:");
	return cp - rp;
}

const char *PluginFClient_Opt::tip()
{
	return opt->help;
}

int PluginFClient_OptPanel::update()
{
	const char *cols[] = { "option", "value", };
	const int col1_w = 150;
	int wids[] = { col1_w, get_w()-col1_w };
	BC_ListBox::update(&items[0], &cols[0], &wids[0], sizeof(items)/sizeof(items[0]),
		get_xposition(), get_yposition(), get_highlighted_item());
	return 0;
}


PluginFClientWindow::PluginFClientWindow(PluginFClient *ffmpeg)
 : PluginClientWindow(ffmpeg->plugin, 600, 300, 600, 300, 1)
{
	this->ffmpeg = ffmpeg;
	this->selected = 0;
}

PluginFClientWindow::~PluginFClientWindow()
{
}

void PluginFClientWindow::create_objects()
{
	char string[BCTEXTLEN];
	BC_Title *title;
	int x = 10, y = 10;
	const char *descr = ffmpeg->config.ffilt->description();
	if( !descr ) descr = ffmpeg->config.ffilt->filter_name();
	add_subwindow(title = new BC_Title(x, y, descr));
	y += title->get_h() + 10;
	int x0 = x;
	sprintf(string, _("Type: "));
	add_subwindow(title = new BC_Title(x0, y, string));
	x0 += title->get_w() + 8;
	add_subwindow(type = new BC_Title(x0, y, (char *)""));
	x0 = x + 150;
	sprintf(string, _("Range: "));
	add_subwindow(title = new BC_Title(x0, y, string));
	x0 += title->get_w() + 8;
	add_subwindow(range = new BC_Title(x0, y, (char *)""));
	int x1 = get_w() - BC_GenericButton::calculate_w(this, _("Reset")) - 8;
	add_subwindow(reset = new PluginFClientReset(this, x1, y));
	y += title->get_h() + 10;
	x0 = x;
	add_subwindow(units = new PluginFClientUnits(this, x0, y, 120));
	x0 += units->get_w() + 8;
	x1 = get_w() - BC_GenericButton::calculate_w(this, _("Apply")) - 8;
	add_subwindow(apply = new PluginFClientApply(this, x1, y));
	add_subwindow(text = new PluginFClientText(this, x0, y, x1-x0 - 8));
	y += title->get_h() + 10;
	add_subwindow(pot = new PluginFClientPot(this, x, y));
	x1 = x + pot->get_w() + 10;
	add_subwindow(slider = new PluginFClientSlider(this, x1, y+10));
	y += pot->get_h() + 10;

	panel_x = x;  panel_y = y;
	panel_w = get_w()-10 - panel_x;
	panel_h = get_h()-10 - panel_y;
	panel = new PluginFClient_OptPanel(this, panel_x, panel_y, panel_w, panel_h);
	add_subwindow(panel);
	panel->create_objects();
	ffmpeg->config.update();
	draw();
	show_window(1);
}

void PluginFClientWindow::draw()
{
	update(selected);
}

int PluginFClientWindow::resize_event(int w, int h)
{
	int x = get_w() - BC_GenericButton::calculate_w(this, _("Reset")) - 8;
	int y = reset->get_y();
	reset->reposition_window(x, y);
	int x1 = get_w() - BC_GenericButton::calculate_w(this, _("Apply")) - 8;
	int y1 = units->get_y();
	apply->reposition_window(x1, y1);
	int x0 = units->get_x() + units->get_w() + 8;
	int y0 = units->get_y();
	text->reposition_window(x0,y0, x1-x0-8);
	x1 = pot->get_x() + pot->get_w() + 10;
	int w1 = w - slider->get_x() - 20;
	slider->set_pointer_motion_range(w1);
	slider->reposition_window(x1, slider->get_y(), w1, slider->get_h());
	panel_w = get_w()-10 - panel_x;
	panel_h = get_h()-10 - panel_y;
	panel->reposition_window(panel_x,panel_y, panel_w, panel_h);
	return 1;
}


PluginFClient::PluginFClient(PluginClient *plugin, const char *name)
{
	this->plugin = plugin;
	this->name = name;
	ffilt = 0;
	fsrc = fsink = 0;
	plugin_position = -1;
	filter_position = -1;
	activated = 0;
	sprintf(title, "F_%s", name);
	config.initialize(name);
	curr_config.initialize(name);
}

PluginFClient::~PluginFClient()
{
	delete ffilt;
}

bool PluginFClient::is_audio(const AVFilter *fp)
{
	if( !fp->outputs ) return 0;
	if( avfilter_pad_count(fp->outputs) > 1 ) return 0;
	if( !avfilter_pad_get_name(fp->outputs, 0) ) return 0;
	if( avfilter_pad_get_type(fp->outputs, 0) != AVMEDIA_TYPE_AUDIO ) return 0;
	if( !fp->inputs ) return 1;
	if( avfilter_pad_count(fp->inputs) > 1 ) return 0;
	if( !avfilter_pad_get_name(fp->inputs, 0) ) return 0;
	if( avfilter_pad_get_type(fp->inputs, 0) != AVMEDIA_TYPE_AUDIO ) return 0;
	return 1;
}
bool PluginFClient::is_video(const AVFilter *fp)
{
	if( !fp->outputs ) return 0;
	if( avfilter_pad_count(fp->outputs) > 1 ) return 0;
	if( !avfilter_pad_get_name(fp->outputs, 0) ) return 0;
	if( avfilter_pad_get_type(fp->outputs, 0) != AVMEDIA_TYPE_VIDEO ) return 0;
	if( !fp->inputs ) return 1;
	if( avfilter_pad_count(fp->inputs) > 1 ) return 0;
	if( !avfilter_pad_get_name(fp->inputs, 0) ) return 0;
	if( avfilter_pad_get_type(fp->inputs, 0) != AVMEDIA_TYPE_VIDEO ) return 0;
	return 1;
}

NEW_WINDOW_MACRO(PluginFClient, PluginFClientWindow)

int PluginFClient::load_configuration()
{
	int64_t src_position =  get_source_position();
	KeyFrame *prev_keyframe = get_prev_keyframe(src_position);
	config.copy_from(curr_config);
	read_data(prev_keyframe);
	int ret = !config.equivalent(curr_config) ? 1 : 0;
	return ret;
}


void PluginFClient::render_gui(void *data, int size)
{
	PluginFClientConfig *conf = (PluginFClientConfig *)data;
	config.copy_from(*conf);
	KeyFrame *keyframe = plugin->server->get_keyframe();
	save_data(keyframe);
	update_gui();
}

void PluginFClient::update_gui()
{
	PluginClientThread *thread = plugin->get_thread();
	if( !thread ) return;
	PluginFClientWindow *window = (PluginFClientWindow*)thread->get_window();
	window->lock_window("PluginFClient::update_gui");
	load_configuration();
	if( config.update() > 0 )
		window->draw();
	window->unlock_window();
}

const char *PluginFClient::plugin_title()
{
	return title;
}

char *PluginFClient::to_upper(char *cp, const char *sp)
{
	char *bp = cp;
	while( *sp != 0 ) *bp++ = toupper(*sp++);
	*bp = 0;
	return cp;
}

void PluginFClient::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	output.tag.set_title(to_upper(string, plugin_title()));
	const AVClass *filt_class = config.filter_class();
	if( filt_class && filt_class->option ) {
		void *obj = config.filter_config();
		const AVOption *opt = NULL;
		while( (opt=av_opt_next(obj, opt)) != 0 ) {
			uint8_t *buf = 0;
			if( av_opt_get(obj, opt->name, 0, &buf) < 0 ) continue;
			output.tag.set_property(opt->name, (const char *)buf);
			av_freep(&buf);
		}
	}
	if( (config.ffilt->filter->flags & AVFILTER_FLAG_SLICE_THREADS) != 0 ) {
		uint8_t *buf = 0;
		if( av_opt_get(config.ffilt->fctx, "threads", 0, &buf) >= 0 && buf ) {
			output.tag.set_property("threads", (const char *)buf);
			av_freep(&buf);
		}
	}

	output.append_tag();
	output.terminate_string();
}

void PluginFClient::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];
	input.set_shared_input(keyframe->xbuf);

	while( !input.read_tag() ) {
		to_upper(string, plugin_title());
		if( !input.tag.title_is(string) ) continue;
		const AVClass *filt_class = config.filter_class();
		if( filt_class && filt_class->option ) {
			void *obj = config.filter_config();
			const AVOption *opt = NULL;
			while( (opt=av_opt_next(obj, opt)) != 0 ) {
				const char *v = input.tag.get_property(opt->name);
				if( v ) av_opt_set(obj, opt->name, v, 0);
			}
			if( (config.ffilt->filter->flags & AVFILTER_FLAG_SLICE_THREADS) != 0 ) {
				const char *v = input.tag.get_property("threads");
				if( v ) av_opt_set(config.ffilt->fctx, "threads", v, 0);
			}
		}
	}
}

int PluginFClient::activate()
{
	if( fsrc )
		avfilter_link(fsrc, 0, ffilt->fctx, 0);
	avfilter_link(ffilt->fctx, 0, fsink, 0);
	int ret = avfilter_graph_config(ffilt->graph, NULL);
	if( ret >= 0 ) {
		curr_config.copy_from(config);
		activated = 1;
	}
	return ret;
}

void PluginFClient::reactivate()
{
	delete ffilt;  ffilt = 0;
	activated = 0;
}

PluginFAClient::PluginFAClient(PluginServer *server, const char *name)
 : PluginAClient(server), PluginFClient(this, name)
{
}

PluginFAClient::~PluginFAClient()
{
}

int PluginFAClient::activate()
{
	if( activated ) return activated;
	ffilt = PluginFFilter::new_ffilter(name, &config);
	if( !ffilt ) {
		config.copy_from(curr_config);
		send_configure_change();
		send_render_gui(&config, sizeof(config));
		ffilt = PluginFFilter::new_ffilter(name, &config);
	}
	AVSampleFormat sample_fmt = AV_SAMPLE_FMT_FLTP;
	int channels = PluginClient::total_in_buffers;
	uint64_t layout = (((uint64_t)1)<<channels) - 1;
	AVFilterGraph *graph = !ffilt ? 0 : ffilt->graph;
	int ret = !graph ? -1 : 0;
	if( ret >= 0 && ffilt->filter->inputs ) {
		char args[BCTEXTLEN];
		snprintf(args, sizeof(args),
			"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%jx",
			1, sample_rate, sample_rate, av_get_sample_fmt_name(sample_fmt), layout);
		ret = avfilter_graph_create_filter(&fsrc, avfilter_get_by_name("abuffer"),
			"in", args, NULL, graph);
	}
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&fsink, avfilter_get_by_name("abuffersink"),
			"out", NULL, NULL, graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(fsink, "sample_fmts",
			(uint8_t*)&sample_fmt, sizeof(sample_fmt), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(fsink, "channel_layouts",
			(uint8_t*)&layout, sizeof(layout), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = av_opt_set_bin(fsink, "sample_rates",
			(uint8_t*)&sample_rate, sizeof(sample_rate), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = PluginFClient::activate();
	if( ret < 0 && activated >= 0 ) {
		ff_err(ret, "PluginFAClient::activate: %s failed\n", plugin_title());
		activated = -1;
	}
	return activated;
}


static AVRational best_frame_rate(double frame_rate)
{
	static const int m1 = 1001*12, m2 = 1000*12;
	static const int freqs[] = {
		40*m1, 48*m1, 50*m1, 60*m1, 80*m1,120*m1, 240*m1,
		24*m2, 30*m2, 60*m2, 12*m2, 15*m2, 48*m2, 0,
	};
	double max_err = 1.;
	int freq, best_freq = 0;
	for( int i=0; (freq = i<30*12 ? (i+1)*1001 : freqs[i-30*12]); ++i ) {
		double framerate = (double)freq / m1;
		double err = fabs(frame_rate/framerate - 1.);
		if( err >= max_err ) continue;
		max_err = err;
		best_freq = freq;
	}
	return max_err < 0.0001 ?
		(AVRational) { best_freq, m1 } :
		(AVRational) { 0, 0 };
}

int PluginFVClient::activate(int width, int height, int color_model)
{
	if( activated ) return activated;
	ffilt = PluginFFilter::new_ffilter(name, &config);
	if( !ffilt ) {
		config.copy_from(curr_config);
		send_configure_change();
		send_render_gui(&config, sizeof(config));
		ffilt = PluginFFilter::new_ffilter(name, &config);
	}
	AVPixelFormat pix_fmt = color_model_to_pix_fmt(color_model);
	AVFilterGraph *graph = !ffilt ? 0 : ffilt->graph;
	int ret = !graph ? -1 : 0;
	if( ret >= 0 && ffilt->filter->inputs ) {
		curr_config.copy_from(config);
		if( pix_fmt == AV_PIX_FMT_NB ) {
			int bpp = BC_CModels::calculate_pixelsize(color_model) * 8;
			int bits_per_comp = bpp / BC_CModels::components(color_model);
			int alpha =  BC_CModels::has_alpha(color_model);
			pix_fmt = bits_per_comp > 8 ?
				!alpha ? AV_PIX_FMT_RGB48LE : AV_PIX_FMT_RGBA64LE :
				!alpha ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_RGBA ;
		}
		int aspect_w = 1, aspect_h = 1; // XXX
		AVRational best_rate = best_frame_rate(frame_rate);
		char args[BCTEXTLEN];
		snprintf(args, sizeof(args),
			"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
			width, height, pix_fmt, best_rate.num, best_rate.den, aspect_w, aspect_h);
		ret = avfilter_graph_create_filter(&fsrc, avfilter_get_by_name("buffer"),
			"in", args, NULL, graph);
	}
	if( ret >= 0 )
		ret = avfilter_graph_create_filter(&fsink, avfilter_get_by_name("buffersink"),
			"out", NULL, NULL, graph);
	if( ret >= 0 )
		ret = av_opt_set_bin(fsink, "pix_fmts",
			(uint8_t*)&pix_fmt, sizeof(pix_fmt), AV_OPT_SEARCH_CHILDREN);
	if( ret >= 0 )
		ret = PluginFClient::activate();
	if( ret < 0 && activated >= 0 ) {
		ff_err(ret, "PluginFVClient::activate() %s\n", plugin_title());
		activated = -1;
	}
	return activated;
}

int PluginFAClient::get_inchannels()
{
	AVFilterContext *fctx = ffilt->fctx;
	AVFilterLink **links = !fctx->nb_inputs ? 0 : fctx->inputs;
	return !links ? 0 : links[0]->channels;
}

int PluginFAClient::get_outchannels()
{
	AVFilterContext *fctx = ffilt->fctx;
	AVFilterLink **links = !fctx->nb_outputs ? 0 : fctx->outputs;
	return !links ? 0 : links[0]->channels;
}

int PluginFAClient::process_buffer(int64_t size, Samples **buffer, int64_t start_position, int sample_rate)
{
	int total_in = PluginClient::total_in_buffers;
	int total_out = PluginClient::total_out_buffers;
	int in_channels = 0, out_channels = 0;

	if( load_configuration() )
		reactivate();

	if( plugin_position != start_position )
		filter_position = plugin_position = start_position;

	AVFrame *frame = 0;
	int ret = activate();
	if( ret >= 0 && !(frame = av_frame_alloc()) ) {
		fprintf(stderr, "PluginFAClient::process_buffer: av_frame_alloc failed\n");
		ret = AVERROR(ENOMEM);
	}
	if( ret >= 0 ) {
		in_channels = get_inchannels();
		out_channels = get_outchannels();
		frame->nb_samples = size;
		frame->format = AV_SAMPLE_FMT_FLTP;
		frame->channel_layout = (1<<in_channels)-1;
		frame->sample_rate = sample_rate;
		frame->pts = local_to_edl(filter_position);
	}

	int retry = 10;
	while( ret >= 0 && --retry >= 0 ) {
		ret = av_buffersink_get_frame(fsink, frame);
		if( ret >= 0 || ret != AVERROR(EAGAIN) ) break;
		if( !fsrc ) { ret = AVERROR(EIO);  break; }
		for( int i=0; i<total_in; ++i )
			read_samples(buffer[i], i, sample_rate, filter_position, size);
		filter_position += size;
		ret = av_frame_get_buffer(frame, 0);
		if( ret < 0 ) break;
		float **in_buffers = (float **)&frame->extended_data[0];
		for(int i = 0; i < in_channels; i++) {
			float *in_buffer = in_buffers[i];
			int k = i < total_in ? i : 0;
			double *in_ptr = buffer[k]->get_data();
			for(int j = 0; j < size; j++) in_buffer[j] = in_ptr[j];
		}
		ret = av_buffersrc_add_frame_flags(fsrc, frame, 0);
	}
	if( ret >= 0 && retry < 0 )
		ret = AVERROR(EAGAIN);

	if( ret >= 0 ) {
		int nbfrs = total_out;
		if( out_channels < nbfrs ) nbfrs = out_channels;
		float **out_buffers = (float **)&frame->extended_data[0];
		int i = 0;
		while( i < nbfrs ) {
			float *out_buffer = out_buffers[i];
			double *out_ptr = buffer[i++]->get_data();
			for(int j = 0; j < size; j++) out_ptr[j] = out_buffer[j];
		}
		while( i < total_out ) {
			double *out_ptr = buffer[i++]->get_data();
			bzero(out_ptr, sizeof(double) * size);
		}
	}
	if( ret < 0 ) {
		int64_t position = PluginFClient::get_source_position();
		double t0 = (double)position/sample_rate, dt = 1./sample_rate;
		for( int i=0; i<total_out; ++i ) {
			double t = t0, *out = buffer[i]->get_data();
			for( int k=size; --k>=0; t+=dt ) {
				double w = int(2*t) & 1 ? 2*M_PI*440 : 2*M_PI*697;
				*out++ = sin(t * w);
			}
		}
		ff_err(ret, "PluginFAClient::process_buffer() %s\n", plugin_title());
	}

	av_frame_free(&frame);
	plugin_position += size;
	return size;
}


PluginFVClient::PluginFVClient(PluginServer *server, const char *name)
 : PluginVClient(server), PluginFClient(this, name)
{
}

PluginFVClient::~PluginFVClient()
{
}

int PluginFVClient::process_buffer(VFrame **frames, int64_t position, double frame_rate)
{
	VFrame *vframe = *frames;
	int width = vframe->get_w();
	int height = vframe->get_h();

	if( load_configuration() )
		reactivate();

	if( plugin_position != position )
		filter_position = plugin_position = position;

	int colormodel = vframe->get_color_model();
	int ret = activate(width, height, colormodel);
	AVPixelFormat pix_fmt = fsrc ?
		(AVPixelFormat) fsrc->outputs[0]->format :
		color_model_to_pix_fmt(colormodel);
	if( pix_fmt <= AV_PIX_FMT_NONE || pix_fmt >= AV_PIX_FMT_NB )
		pix_fmt = AV_PIX_FMT_RGBA;

	AVFrame *frame = 0;
	if( ret >= 0 && !(frame = av_frame_alloc()) ) {
		fprintf(stderr, "PluginFVClient::process_buffer: av_frame_alloc failed\n");
		ret = AVERROR(ENOMEM);
	}

	int retry = 10;
	while( ret >= 0 && --retry >= 0 ) {
		ret = av_buffersink_get_frame(fsink, frame);
		if( ret >= 0 || ret != AVERROR(EAGAIN) ) break;
		if( !fsrc ) { ret = AVERROR(EIO);  break; }
		read_frame(vframe, 0, filter_position++, frame_rate, 0);
		frame->format = pix_fmt;
	        frame->width  = width;
		frame->height = height;
		frame->pts = local_to_edl(position);
		ret = av_frame_get_buffer(frame, 32);
		if( ret < 0 ) break;
		ret = transfer_pixfmt(vframe, frame);
		if( ret < 0 ) break;
		ret = av_buffersrc_add_frame_flags(fsrc, frame, 0);
	}
	if( ret >= 0 && retry < 0 )
		ret = AVERROR(EAGAIN);

	if( ret >= 0 ) {
		pix_fmt = (AVPixelFormat) frame->format;
		ret = transfer_cmodel(vframe, frame);
	}
	if( ret < 0 ) {
		ff_err(ret, "PluginFVClient::process_buffer() %s\n", plugin_title());
		if( position & 1 ) vframe->clear_frame();
	}

	++plugin_position;
	av_frame_free(&frame);
	return ret >= 0 ? 0 : 1;
}


PluginFClient_OptName:: PluginFClient_OptName(PluginFClient_Opt *opt)
{
	this->opt = opt;
	set_text(opt->opt->name);
}

PluginFClient_OptValue::PluginFClient_OptValue(PluginFClient_Opt *opt)
{
	this->opt = opt;
	update();
}

void PluginFClient_OptValue::update()
{
	char val[BCTEXTLEN];  val[0] = 0;
	opt->get(val, sizeof(val));
	set_text(val);
}


PluginFClient_Opt::PluginFClient_Opt(PluginFClientConfig *conf, const AVOption *opt)
{
	this->conf = conf;
	this->opt = opt;
	item_name = new PluginFClient_OptName(this);
	item_value = new PluginFClient_OptValue(this);
}

PluginFClient_Opt::~PluginFClient_Opt()
{
	delete item_name;
	delete item_value;
}

const char *PluginFClientConfig::get(const char *name)
{
	uint8_t *bp = 0;
	if( av_opt_get(filter_config(), name, 0, &bp) >= 0 ||
	    av_opt_get(ffilt->fctx, name, 0, &bp) >= 0 )
		return (const char *)bp;
	return 0;
}
char *PluginFClient_Opt::get(char *vp, int sz)
{
	const char *val = conf->get(opt->name);
	if( val ) {
		strncpy(vp, val, sz);
		vp[sz-1] = 0;
	}
	else
		vp = 0;
	av_freep(&val);
	return vp;
}

void PluginFClientConfig::set(const char *name, const char *val)
{
	if( av_opt_set(filter_config(), name, val, 0) < 0 )
		av_opt_set(ffilt->fctx, name, val, 0);
}
void PluginFClient_Opt::set(const char *val)
{
	conf->set(opt->name, val);
}

void PluginFFilter::uninit()
{
}

static int get_defaults(const char *name, char *args)
{
	*args = 0;
	char defaults_path[BCTEXTLEN];
	FFMPEG::set_option_path(defaults_path, "plugin.opts");
	FILE *fp = fopen(defaults_path,"r");
	if( !fp ) return 0;
	char ff_plugin[BCSTRLEN], ff_args[BCTEXTLEN], *ap = 0;
	while( fgets(ff_args, sizeof(ff_args), fp) ) {
		char *cp = ff_args;
		if( *cp == ';' ) continue;
		if( *cp == '#' ) ++cp;
		char *bp = ff_plugin, *ep = bp + sizeof(ff_plugin)-1;
		while( bp < ep && *cp && *cp != '\n' && *cp != ' ' ) *bp++ = *cp++;
		*bp = 0;
		if( !strcmp(ff_plugin, name) ) { ap = cp;  break; }
	}
	fclose(fp);
	if( !ap ) return 0;
	if( ff_args[0] == '#' ) return -1;
	while( *ap == ' ' ) ++ap;
	while( *ap && *ap != '\n' ) *args++ = *ap++;
	*args = 0;
	return 1;
}

bool PluginFFilter::is_audio()
{
	return PluginFClient::is_audio(filter);
}

bool PluginFFilter::is_video()
{
	return PluginFClient::is_video(filter);
}

int PluginFFilter::init(const char *name, PluginFClientConfig *conf)
{
	char args[BCTEXTLEN];
	int ret = get_defaults(name, args);
	if( ret < 0 ) return 0;
	PluginFLogLevel errs(AV_LOG_ERROR);
	this->filter = avfilter_get_by_name(name);
	if( !this->filter ) return AVERROR(ENOENT);
	int flag_mask = AVFILTER_FLAG_DYNAMIC_INPUTS | AVFILTER_FLAG_DYNAMIC_OUTPUTS;
	if( filter->flags & flag_mask ) return AVERROR(EPERM);
	if( !this->is_audio() && !this->is_video() ) return AVERROR(EIO);
	this->graph = avfilter_graph_alloc();
	if( !this->graph ) return AVERROR(ENOMEM);
	static int inst = 0;
	char inst_name[BCSTRLEN];
	sprintf(inst_name,"%s_%d", name, ++inst);
	if( conf && (filter->flags & AVFILTER_FLAG_SLICE_THREADS) != 0 ) {
		graph->thread_type = AVFILTER_THREAD_SLICE;
		graph->nb_threads  = atoi(conf->get("threads"));
	}
	else {
		graph->thread_type = 0;
		graph->nb_threads  = 0;
	}
	fctx = avfilter_graph_alloc_filter(graph, filter, inst_name);
	if( !fctx ) return AVERROR(ENOMEM);
	fctx->thread_type = graph->thread_type; // bug in avfilter
	if( conf ) {
		AVDictionary *opts = 0;
		for( int i=0; i<conf->size(); ++i ) {
			PluginFClient_Opt *op = conf->get(i);
			const char *name = op->opt->name;
			char val[BCTEXTLEN], *vp = op->get(val, sizeof(val));
			if( !vp ) continue;
			uint8_t *bp = 0;
// unspecified opts cause a special behavior in some filters (lut3d)
// so... if opt value is the default, skip it or no special behavior
			if( av_opt_get(filter_config(), name, 0, &bp) >= 0 ) {
				int result = strcmp((const char *)bp, vp);
				av_freep(&bp);
				if( !result ) continue;
			}
			av_dict_set(&opts, name, vp, 0);
		}
		ret = avfilter_init_dict(fctx, &opts);
		av_dict_free(&opts);
	}
	else
		ret = avfilter_init_str(fctx, args);
	return ret < 0 ? ret : 1;
}


PluginFFilter::PluginFFilter()
{
	filter = 0;
	graph = 0;
	fctx = 0;
}

PluginFFilter::~PluginFFilter()
{
	PluginFLogLevel errs(AV_LOG_ERROR);
	avfilter_graph_free(&graph);
	filter = 0;  fctx = 0;
}

PluginFFilter *PluginFFilter::new_ffilter(const char *name, PluginFClientConfig *conf)
{
	PluginFFilter *ffilt = new PluginFFilter;
	int ret = ffilt->init(name, conf);
	if( ret < 0 ) ff_err(ret, "PluginFFilter::new_ffilter(%s)\n", name);
	if( ret <= 0 ) { delete ffilt;  ffilt = 0; }
	return ffilt;
}

PluginClient *PluginServer::new_ffmpeg_plugin()
{
	const AVFilter *filter = avfilter_get_by_name(ff_name);
	if( !filter ) return 0;
	PluginFClient *ffmpeg =
		PluginFClient::is_audio(filter) ?
			(PluginFClient *)new PluginFAClient(this, ff_name) :
		PluginFClient::is_video(filter) ?
			(PluginFClient *)new PluginFVClient(this, ff_name) :
		0;
	if( !ffmpeg ) return 0;
	return ffmpeg->plugin;
}


PluginServer* MWindow::new_ffmpeg_server(MWindow *mwindow, const char *name)
{
	PluginFFilter *ffilt = PluginFFilter::new_ffilter(name);
	if( !ffilt ) return 0;
	delete ffilt;
	return new PluginServer(mwindow, name, PLUGIN_TYPE_FFMPEG);
}

void MWindow::init_ffmpeg_index(MWindow *mwindow, Preferences *preferences, FILE *fp)
{
	PluginFLogLevel errs(AV_LOG_ERROR);
	const AVFilter *filter = 0;  void *idx = 0;
	while( (filter=av_filter_iterate(&idx)) != 0 ) {
//printf("%s\n",filter->name);
		PluginServer *server = new_ffmpeg_server(mwindow, filter->name);
		if( server ) {
			int result = server->open_plugin(1, preferences, 0, 0);
			if( !result ) {
				server->write_table(fp, filter->name, PLUGIN_FFMPEG_ID, 0);
				server->close_plugin();
			}
			delete server;
			if( result ) fprintf(fp, "#%s\n", filter->name);
		}
	}
}

void MWindow::init_ffmpeg()
{
}

