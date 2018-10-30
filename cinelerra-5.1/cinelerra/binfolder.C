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

#include "awindowgui.h"
#include "bchash.h"
#include "binfolder.h"
#include "cstrdup.h"
#include "edl.h"
#include "filexml.h"
#include "guicast.h"
#include "indexable.h"
#include "language.h"
#include "localsession.h"
#include "mainerror.h"
#include "mainsession.h"
#include "mutex.h"
#include "mwindow.h"

#include <time.h>

const char *BinFolderEnabled::types[] = {
	N_("Off"),
	N_("And"),
	N_("Or"),
	N_("And Not"),
	N_("Or Not"),
};

const char *BinFolderTarget::types[] = {
	N_("Patterns"),
	N_("Filesize"),
	N_("Time"),
	N_("Track type"),
	N_("Width"),
	N_("Height"),
	N_("Framerate"),
	N_("Samplerate"),
	N_("Channels"),
	N_("Duration"),
};

const char *BinFolderOp::types[] = {
	N_("Around"),
	N_("Eq  =="),
	N_("Ge  >="),
	N_("Gt  > "),
	N_("Ne  !="),
	N_("Le  <="),
	N_("Lt  < "),
	N_("Matches"),
};

static const struct {
	const char *sfx, *sfxs;
	double radix;
} scan_ranges[] = {
	{ N_("min"),   N_("mins"),   60, },
	{ N_("hour"),  N_("hours"),  60*60, },
	{ N_("day"),   N_("days"),   24*60*60, },
	{ N_("week"),  N_("weeks"),  24*60*60*7, },
	{ N_("month"), N_("months"), 24*60*60*31, },
	{ N_("year"),  N_("years"),  24*60*60*365.25, },
};
#define SCAN_RANGE_DAYS 2
#define DBL_INF 9007199254740991.

static int scan_inf(const char *cp, char *&bp)
{
	bp = (char *)cp;
	const char *inf = _("inf");
	const int linf = strlen(inf);
	if( strncasecmp(cp,inf,linf) ) return 0;
	bp = (char *)(cp + linf);
	return 1;
}

static double scan_no(const char *cp, char *&bp)
{
	while( *cp == ' ' ) ++cp;
	if( scan_inf(cp, bp) ) return DBL_INF;
	if( *cp == '+' ) return 0;
	double v = strtod(cp, &bp);
	switch( *bp ) {
	case 'T': v *= 1099511627776.;	break;
	case 't': v *= 1e12;		break;
	case 'G': v *= 1073741824.;	break;
	case 'g': v *= 1e9;		break;
	case 'M': v *= 1048576.;	break;
	case 'm': v *= 1e6;		break;
	case 'K': v *= 1024.;		break;
	case 'k': v *= 1e3;		break;
	default:  return v;
	}
	++bp;
	return v;
}

static void scan_around(const char *cp, char *&bp, double &v, double &a)
{
	v = 0;  a = -1;
	double sv = scan_no(cp, bp);
	if( bp > cp ) v = sv;
	if( *bp == '+' ) {
		double sa = scan_no(cp=bp+1, bp);
		if( bp > cp ) a = sa;
	}
}

static void show_no(double v, char *&cp, char *ep, const char *fmt="%0.0f")
{
	if( v == DBL_INF ) {
		cp += snprintf(cp, ep-cp, "%s", _("inf"));
		return;
	}
	const char *sfx = 0;
	static const struct { double radix; const char *sfx; } sfxs[] = {
		{ 1024.,          "K" }, { 1e3,  "k" },
		{ 1048576.,       "M" }, { 1e6,  "m" },
		{ 1073741824.,    "G" }, { 1e9,  "g" },
		{ 1099511627776., "T" }, { 1e12, "t" },
	};
	for( int i=sizeof(sfxs)/sizeof(sfxs[0]); --i>=0; ) {
		if( v < sfxs[i].radix ) continue;
		if( fmod(v, sfxs[i].radix) == 0 ) {
			v /= sfxs[i].radix;
			sfx = sfxs[i].sfx;
			break;
		}
	}
	cp += snprintf(cp, ep-cp, fmt, v);
	if( sfx ) cp += snprintf(cp, ep-cp, "%s", sfx);
}

static double scan_duration(const char *cp, char *&bp)
{
	if( scan_inf(cp, bp) ) return DBL_INF;
	double secs = 0;
	while( *cp ) {
		double v = strtod(cp, &bp);
		if( cp >= bp ) break;
		int k = sizeof(scan_ranges)/sizeof(scan_ranges[0]);
		while( --k >= 0 ) {
			const char *tsfx  = _(scan_ranges[k].sfx);
			int lsfx  = strlen(tsfx), msfx  = strncasecmp(bp, tsfx,  lsfx);
			const char *tsfxs = _(scan_ranges[k].sfxs);
			int lsfxs = strlen(tsfxs), msfxs = strncasecmp(bp, tsfxs, lsfxs);
			int len = !msfx && !msfxs ? (lsfx > lsfxs ? lsfx : lsfxs) :
				!msfx ? lsfx : !msfxs ? lsfxs : -1;
			if( len >= 0 ) {
				secs += v * scan_ranges[k].radix;
				bp += len;
				break;
			}
		}
		if( k < 0 ) {
			int hour = 0, mins = 0;
			if( *bp == ':' && v == (int)v ) {
				mins = v;
				v = strtod(cp=bp+1, &bp);
				if( *bp == ':' && v == (int)v ) {
					hour = mins;  mins = v;
					v = strtod(cp=bp+1, &bp);
				}
			}
			secs += hour*3600 + mins*60 + v;
		}
		while( *bp && (*bp<'0' || *bp>'9') ) ++bp;
		cp = bp;
	}
	return secs;
}

static void show_duration(double v, char *&cp, char *ep)
{
	if( v == DBL_INF ) {
		cp += snprintf(cp, ep-cp, "%s", _("inf"));
		return;
	}
	double secs = v;  char *bp = cp;
	int k = sizeof(scan_ranges)/sizeof(scan_ranges[0]);
	while( --k >= SCAN_RANGE_DAYS ) {
		if( secs >= scan_ranges[k].radix ) {
			int v = secs/scan_ranges[k].radix;
			secs -= v * scan_ranges[k].radix;
			cp += snprintf(cp, ep-cp,"%d%s", v,
				v > 1 ? _(scan_ranges[k].sfxs) : _(scan_ranges[k].sfx));
		}
	}
	if( secs > 0 ) {
		if( cp > bp && cp < ep ) *cp++ = ' ';
		int64_t n = secs; int hour = n/3600, min = (n/60)%60, sec = n%60;
		if( hour > 0 ) cp += snprintf(cp, ep-cp, "%d:", hour);
		if( hour > 0 || min > 0 ) cp += snprintf(cp, ep-cp, "%02d:", min);
		cp += snprintf(cp, ep-cp, n>=10 ? "%02d" : "%d", sec);
	}
}

static int64_t scan_date(const char *cp, char *&bp)
{
	double year=0, mon=1, day=1;
	double hour=0, min=0;
	bp = (char *)cp;
	while( *bp == ' ' ) ++bp;
	if( *bp == '+' ) return 0;
	double secs = strtod(cp=bp, &bp);
	if( *bp == '/' && secs == (int)secs ) {
		year = secs;  secs = 0;
		mon = strtod(cp=bp+1, &bp);
		if( *bp == '/' && mon == (int)mon ) {
			day = strtod(cp=bp+1, &bp);
			while( *bp == ' ' ) ++bp;
			secs = *bp != '+' ? strtod(cp=bp, &bp) : 0;
		}
	}
	if( *bp == ':' && secs == (int)secs ) {
		hour = secs;  secs = 0;
		min = strtod(cp=bp+1, &bp);
		if( *bp == ':' && min == (int)min ) {
			secs = strtod(cp=bp+1, &bp);
		}
	}
	struct tm ttm;  memset(&ttm, 0, sizeof(ttm));
	ttm.tm_year = year-1900;  ttm.tm_mon = mon-1;  ttm.tm_mday = day;
	ttm.tm_hour = hour;       ttm.tm_min = min;    ttm.tm_sec = secs;
	time_t t = mktime(&ttm);
	return (int64_t)t;
}

static void show_date(time_t t, char *&cp, char *ep)
{
	struct tm tm;  localtime_r(&t, &tm);
	cp += snprintf(cp, ep-cp, "%04d/%02d/%02d %02d:%02d:%02d",
		tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
}

double BinFolder::matches_indexable(Indexable *idxbl)
{
	double result = -1;
	for( int i=0; i<filters.size(); ++i ) {
		BinFolderFilter *filter = filters[i];
		double ret = filter->op->test(filter->target, idxbl);
		switch( filter->enabled->type ) {
		case FOLDER_ENABLED_OR: {
			if( result < 0 ) result = ret;
			break; }
		case FOLDER_ENABLED_AND: {
			if( ret < 0 ) result = -1;
			break; }
		case FOLDER_ENABLED_OR_NOT: {
			if( ret < 0 ) result = 1;
			break; }
		case FOLDER_ENABLED_AND_NOT: {
			if( ret >= 0 ) result = -1;
			break; }
		}
	}
	return result;
}


BinFolder::BinFolder(int awindow_folder, int is_clips, const char *title)
{
	this->awindow_folder = awindow_folder;
	this->is_clips = is_clips;
	char *bp = this->title;
	int len = sizeof(this->title);
	while( --len>0 && *title ) *bp++ = *title++;
	*bp = 0;
}

BinFolder::BinFolder(BinFolder &that)
{
	copy_from(&that);
}

void BinFolder::copy_from(BinFolder *that)
{
	strcpy(title, that->title);
	awindow_folder = that->awindow_folder;
	is_clips = that->is_clips;
	filters.copy_from(&that->filters);
}

BinFolder::~BinFolder()
{
}

void BinFolder::save_xml(FileXML *file)
{
	file->tag.set_title("FOLDER");
	file->tag.set_property("TITLE", title);
	file->tag.set_property("AWINDOW_FOLDER", awindow_folder);
	file->tag.set_property("IS_CLIPS", is_clips);
	file->append_tag();
	file->append_newline();
	for( int i=0; i<filters.size(); ++i )
		filters[i]->save_xml(file);
	file->tag.set_title("/FOLDER");
	file->append_tag();
	file->append_newline();
}

int BinFolder::load_xml(FileXML *file)
{
	title[0] = 0;
	file->tag.get_property("TITLE", title);
	awindow_folder = file->tag.get_property("AWINDOW_FOLDER", -1);
	is_clips = file->tag.get_property("IS_CLIPS", 0);
	filters.remove_all_objects();

	int ret = 0;
	while( !(ret=file->read_tag()) ) {
		if( file->tag.title_is("/FOLDER") ) break;
		if( file->tag.title_is("FILTER") ) {
			BinFolderFilter *filter = new BinFolderFilter();
			filter->load_xml(file);
			filters.append(filter);
		}
	}
	return ret;
}

int BinFolder::add_patterns(ArrayList<Indexable*> *drag_idxbls, int use_basename)
{
	int n = drag_idxbls->size();
	if( !n ) return 1;
	Indexable *idxbl;
	int len = 0;
	for( int i=0; i<n; ++i ) {
		idxbl = drag_idxbls->get(i);
		if( !idxbl->is_asset &&
		    idxbl->folder_no == AW_PROXY_FOLDER )
			continue;

		const char *tp = idxbl->get_title();
		if( use_basename && idxbl->is_asset ) {
			const char *cp = strrchr(tp, '/');
			if( cp ) tp = cp + 1;
			len += 1;  // "*" + fn
		}
		len += strlen(tp) + 1;
	}
	if( !len ) return 1;
	char *pats = new char[len+1], *bp = pats;
	for( int i=0; i<n; ++i ) {
		idxbl = drag_idxbls->get(i);
		if( !idxbl->is_asset &&
		    idxbl->folder_no == AW_PROXY_FOLDER )
			continue;
		if( i > 0 ) *bp++ = '\n';
		const char *tp = idxbl->get_title();
		if( use_basename && idxbl->is_asset ) {
			const char *cp = strrchr(tp, '/');
			if( cp ) tp = cp + 1;
			*bp++ = '*';
		}
		while( *tp ) *bp++ = *tp++;
	}
	*bp = 0;
// new pattern filter
	BinFolderFilter *filter = new BinFolderFilter();
	filter->update_enabled(FOLDER_ENABLED_OR);
	filter->update_target(FOLDER_TARGET_PATTERNS);
	filter->update_op(FOLDER_OP_MATCHES);
	BinFolderTargetPatterns *patterns = (BinFolderTargetPatterns *)(filter->target);
	patterns->update(pats);
	filters.append(filter);
	return 0;
}


double BinFolders::matches_indexable(int folder, Indexable *idxbl)
{
	int k = size();
	BinFolder *bin_folder = 0;
	while( --k>=0 && (bin_folder=get(k)) && bin_folder->awindow_folder!=folder );
	if( k < 0 ) return -1;
	if( bin_folder->is_clips && idxbl->is_asset ) return -1;
	if( !bin_folder->is_clips && !idxbl->is_asset ) return -1;
	return bin_folder->matches_indexable(idxbl);
}

void BinFolders::save_xml(FileXML *file)
{
	file->tag.set_title("FOLDERS");
	file->append_tag();
	file->append_newline();
	for( int i=0; i<size(); ++i )
		get(i)->save_xml(file);
	file->tag.set_title("/FOLDERS");
	file->append_tag();
	file->append_newline();
}

int BinFolders::load_xml(FileXML *file)
{
	clear();
	int ret = 0;
	while( !(ret=file->read_tag()) ) {
		if( file->tag.title_is("/FOLDERS") ) break;
		if( file->tag.title_is("FOLDER") ) {
			BinFolder *folder = new BinFolder(-1, -1, "folder");
			folder->load_xml(file);
			append(folder);
		}
	}
	return ret;
}

void BinFolders::copy_from(BinFolders *that)
{
	clear();
	for( int i=0; i<that->size(); ++i )
		append(new BinFolder(*that->get(i)));
}


BinFolderFilter::BinFolderFilter()
{
	enabled = 0;
	target = 0;
	op = 0;
	value = 0;
}
BinFolderFilter::~BinFolderFilter()
{
	delete enabled;
	delete target;
	delete op;
	delete value;
}

void BinFolderFilter::update_enabled(int type)
{
	if( !enabled )
		enabled = new BinFolderEnabled(this, type);
	else
		enabled->update(type);
}

void BinFolderFilter::update_target(int type)
{
	if( target ) {
		if( target->type == type ) return;
		delete target;  target = 0;
	}
	switch( type ) {
	case FOLDER_TARGET_PATTERNS:	target = new BinFolderTargetPatterns(this);    break;
	case FOLDER_TARGET_FILE_SIZE:	target = new BinFolderTargetFileSize(this);    break;
	case FOLDER_TARGET_MOD_TIME:	target = new BinFolderTargetTime(this);        break;
	case FOLDER_TARGET_TRACK_TYPE:	target = new BinFolderTargetTrackType(this);   break;
	case FOLDER_TARGET_WIDTH:	target = new BinFolderTargetWidth(this);       break;
	case FOLDER_TARGET_HEIGHT:	target = new BinFolderTargetHeight(this);      break;
	case FOLDER_TARGET_FRAMERATE:	target = new BinFolderTargetFramerate(this);   break;
	case FOLDER_TARGET_SAMPLERATE:	target = new BinFolderTargetSamplerate(this);  break;
	case FOLDER_TARGET_CHANNELS:	target = new BinFolderTargetChannels(this);    break;
	case FOLDER_TARGET_DURATION:	target = new BinFolderTargetDuration(this);    break;
	}
}

void BinFolderFilter::update_op(int type)
{
	if( op ) {
		if( op->type == type ) return;
		delete op;  op = 0;
	}
	switch( type ) {
	case FOLDER_OP_AROUND:	op = new BinFolderOpAround(this);  break;
	case FOLDER_OP_EQ:	op = new BinFolderOpEQ(this);      break;
	case FOLDER_OP_GE:	op = new BinFolderOpGE(this);      break;
	case FOLDER_OP_GT:	op = new BinFolderOpGT(this);      break;
	case FOLDER_OP_NE:	op = new BinFolderOpNE(this);      break;
	case FOLDER_OP_LE:	op = new BinFolderOpLE(this);      break;
	case FOLDER_OP_LT:	op = new BinFolderOpLT(this);      break;
	case FOLDER_OP_MATCHES:	op = new BinFolderOpMatches(this); break;
	}
}

void BinFolderFilter::update_value(const char *text)
{
	if( !value )
		value = new BinFolderValue(this, text);
	else
		value->update(text);
}

void BinFolderFilter::save_xml(FileXML *file)
{
	file->tag.set_title("FILTER");
	file->tag.set_property("ENABLED", enabled->type);
	file->tag.set_property("OP", op->type);
	file->tag.set_property("TARGET", target->type);
	target->save_xml(file);
	file->append_tag();
	if( target->type == FOLDER_TARGET_PATTERNS )
		file->append_text(((BinFolderTargetPatterns *)target)->text);
	file->tag.set_title("/FILTER");
	file->append_tag();
	file->append_newline();
}

int BinFolderFilter::load_xml(FileXML *file)
{
	int enabled_type = file->tag.get_property("ENABLED", FOLDER_ENABLED_AND);
	int op_type = file->tag.get_property("OP", FOLDER_OP_MATCHES);
	int target_type = file->tag.get_property("TARGET", FOLDER_TARGET_PATTERNS);
	XMLBuffer data;
	file->read_text_until("/FILTER", &data, 0);
	update_enabled(enabled_type);
	update_target(target_type);
	update_op(op_type);
	target->load_xml(file);
	if( target->type == FOLDER_TARGET_PATTERNS )
		((BinFolderTargetPatterns *)target)->update(data.cstr());
	return 0;
}

void BinFolderFilters::copy_from(BinFolderFilters *that)
{
	clear();
	for( int i=0; i<that->size(); ++i ) {
		BinFolderFilter *filter = new BinFolderFilter();
		BinFolderFilter *tp = that->get(i);
		filter->update_enabled(tp->enabled->type);
		filter->update_target(tp->target->type);
		filter->update_op(tp->op->type);
		filter->target->copy_from(tp->target);
		filter->op->copy_from(tp->op);
		append(filter);
	}
}

double BinFolderOp::around(double v, double a)
{
	if( type != FOLDER_OP_AROUND ) return v;
	v = fabs(v);
	return a>0 ? v/a : v;
}

// string theory: Feynman, Einstein and Schrodinger string compare
//   Feynman: try all possible matches, weight the outcomes
//   Schrodinger: it may be there several ways at the same time
//   Einstein: the more matches there are, the heavier it is
double BinFolderOp::around(const char *ap, const char *bp)
{
	int64_t v = 0, vmx = 0;
	int alen = strlen(ap), blen = strlen(bp);
	if( alen > blen ) {
		const char *cp = ap;  ap = bp;  bp = cp;
		int clen = alen;  alen = blen;  blen = clen;
	}
// 4 level loop (with strncmp), don't try long strings
	for( int n=0; ++n<=alen; ) {
		int64_t nn = n*n;
		int an = alen-n+1, bn = blen-n+1;
		for( int i=an; --i>=0; ) {
			for( int j=bn; --j>=0; ) {
				if( !strncmp(ap+i, bp+j, n) ) v += nn;
			}
		}
		vmx += an*bn*nn;
	}
	return !vmx ? -1 : 1 - v / (double)vmx;
}


double BinFolderOp::compare(BinFolderTarget *target, Indexable *idxbl)
{
	double v = -1;
	switch( target->type ) {
	case FOLDER_TARGET_PATTERNS: {
		BinFolderTargetPatterns *tgt = (BinFolderTargetPatterns *)target;
		switch( type ) {
		case FOLDER_OP_AROUND: {
			const char *cp = idxbl->path;
			const char *bp = strrchr(cp, '/');
			if( bp ) cp = bp + 1;
			v = around(cp, tgt->text);
			break; }
		case FOLDER_OP_EQ:  case FOLDER_OP_GT:  case FOLDER_OP_GE:
		case FOLDER_OP_NE:  case FOLDER_OP_LT:  case FOLDER_OP_LE: {
			const char *cp = idxbl->path;
			const char *bp = strrchr(cp, '/');
			if( bp ) cp = bp + 1;
			v = strcmp(cp, tgt->text);
			break; }
		case FOLDER_OP_MATCHES: {
			v = -1;
			char *cp = tgt->text;
			while( v < 0 && *cp ) {
				char pattern[BCTEXTLEN], *bp = pattern, ch;
				while( *cp && (ch=*cp++)!='\n' ) *bp++ = ch;
				*bp = 0;
				if( !pattern[0] ) continue;
				const char *title = idxbl->get_title();
				if( !FileSystem::test_filter(title, pattern) )
					v = 1;
			}
			break; }
		}
		break; }
	case FOLDER_TARGET_FILE_SIZE: {
		BinFolderTargetFileSize *tgt = (BinFolderTargetFileSize *)target;
		int64_t file_size = !idxbl->is_asset ? -1 :
			FileSystem::get_size(idxbl->path);
		v = around(file_size - tgt->file_size, tgt->around);
		break; }
	case FOLDER_TARGET_MOD_TIME: {
		BinFolderTargetTime *tgt = (BinFolderTargetTime *)target;
		struct stat st;
		if( stat(idxbl->path, &st) ) break;
		v = around(st.st_mtime - tgt->mtime, tgt->around);
		break; }
	case FOLDER_TARGET_TRACK_TYPE: {
		BinFolderTargetTrackType *tgt = (BinFolderTargetTrackType *)target;
		int want_audio = (tgt->data_types&(1<<TRACK_AUDIO)) ? 1 : 0;
		int has_audio = idxbl->have_audio();
		if( want_audio != has_audio ) break;
		int want_video = (tgt->data_types&(1<<TRACK_VIDEO)) ? 1 : 0;
		int has_video = idxbl->have_video();
		if( want_video != has_video ) break;
		v = 1;
		break; }
	case FOLDER_TARGET_WIDTH: {
		BinFolderTargetWidth *tgt = (BinFolderTargetWidth *)target;
		int w = idxbl->get_w();
		v = around(w - tgt->width, tgt->around);
		break; }
	case FOLDER_TARGET_HEIGHT: {
		BinFolderTargetHeight *tgt = (BinFolderTargetHeight *)target;
		int h = idxbl->get_h();
		v = around(h - tgt->height, tgt->around);
		break; }
	case FOLDER_TARGET_FRAMERATE: {
		BinFolderTargetFramerate *tgt = (BinFolderTargetFramerate *)target;
		double rate = idxbl->get_frame_rate();
		v = around(rate - tgt->framerate, tgt->around);
		break; }
	case FOLDER_TARGET_SAMPLERATE: {
		BinFolderTargetSamplerate *tgt = (BinFolderTargetSamplerate *)target;
		double rate = idxbl->get_sample_rate();
		v = around(rate - tgt->samplerate, tgt->around);
		break; }
	case FOLDER_TARGET_CHANNELS: {
		BinFolderTargetChannels *tgt = (BinFolderTargetChannels *)target;
		double chs = idxbl->get_audio_channels();
		v = around(chs - tgt->channels, tgt->around);
		break; }
	case FOLDER_TARGET_DURATION: {
		BinFolderTargetDuration *tgt = (BinFolderTargetDuration *)target;
		double len = 0;
		double video_rate = !idxbl->have_video() ? 0 : idxbl->get_frame_rate();
		if( video_rate > 0 ) {
			double video_length = idxbl->get_video_frames() / video_rate;
			if( video_length > len ) len = video_length;
		}
		double audio_rate = !idxbl->have_audio() ? 0 : idxbl->get_sample_rate();
		if( audio_rate > 0 ) {
			double audio_length = idxbl->get_audio_samples() / audio_rate;
			if( audio_length > len ) len = audio_length;
		}
		v = around(len - tgt->duration, tgt->around);
		break; }
	}

	return v;
}

BinFolderEnabled::BinFolderEnabled(BinFolderFilter *filter, int type)
 : BC_ListBoxItem(_(types[type]))
{
	this->filter = filter;
	this->type = type;
}

BinFolderEnabled::~BinFolderEnabled()
{
}

void BinFolderEnabled::update(int type)
{
	this->type = type;
	set_text(_(types[type]));
}

BinFolderEnabledType::BinFolderEnabledType(int no)
 : BC_MenuItem(_(BinFolderEnabled::types[no]))
{
	this->no = no;
}
BinFolderEnabledType::~BinFolderEnabledType()
{
}

int BinFolderEnabledType::handle_event()
{
	BinFolderEnabledPopup *enabled_popup = (BinFolderEnabledPopup *)get_popup_menu();
	BinFolderList *folder_list = enabled_popup->folder_list;
	int i = folder_list->get_selection_number(FOLDER_COLUMN_ENABLE, 0);
	if( i >= 0 ) {
		BinFolder *folder = folder_list->folder;
		BinFolderFilter *filter = folder->filters[i];
		filter->update_enabled(no);
		folder_list->create_list();
	}
	return 1;
}

BinFolderEnabledPopup::BinFolderEnabledPopup(BinFolderList *folder_list)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->folder_list = folder_list;
	enabled = 0;
}

void BinFolderEnabledPopup::create_objects()
{
	add_item(new BinFolderEnabledType(FOLDER_ENABLED_OFF));
	add_item(new BinFolderEnabledType(FOLDER_ENABLED_AND));
	add_item(new BinFolderEnabledType(FOLDER_ENABLED_OR));
	add_item(new BinFolderEnabledType(FOLDER_ENABLED_AND_NOT));
	add_item(new BinFolderEnabledType(FOLDER_ENABLED_OR_NOT));
}

void BinFolderEnabledPopup::activate_menu(BC_ListBoxItem *item)
{
	this->enabled = (BinFolderEnabled *)item;
	BC_PopupMenu::activate_menu();
}

BinFolderTarget::BinFolderTarget(BinFolderFilter *filter, int type)
 : BC_ListBoxItem(_(types[type]))
{
	this->filter = filter;
	this->type = type;
	around = -1;
}

BinFolderTarget::~BinFolderTarget()
{
}

BC_Window *BinFolderTarget::new_gui(ModifyTargetThread *thread)
{
	ModifyTargetGUI *window = new ModifyTargetGUI(thread);
	window->create_objects();
	return window;
}

BinFolderTargetType::BinFolderTargetType(int no)
 : BC_MenuItem(_(BinFolderTarget::types[no]))
{
	this->no = no;
}
BinFolderTargetType::~BinFolderTargetType()
{
}

int BinFolderTargetType::handle_event()
{
	BinFolderTargetPopup *target_popup = (BinFolderTargetPopup *)get_popup_menu();
	BinFolderList *folder_list = target_popup->folder_list;
	int i = folder_list->get_selection_number(FOLDER_COLUMN_TARGET, 0);
	if( i >= 0 ) {
		BinFolder *folder = folder_list->folder;
		BinFolderFilter *filter = folder->filters[i];
		filter->update_target(no);
		folder_list->create_list();
	}
	return 1;
}

BinFolderTargetPopup::BinFolderTargetPopup(BinFolderList *folder_list)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->folder_list = folder_list;
	target = 0;
}

void BinFolderTargetPopup::create_objects()
{
	add_item(new BinFolderTargetType(FOLDER_TARGET_PATTERNS));
	add_item(new BinFolderTargetType(FOLDER_TARGET_FILE_SIZE));
	add_item(new BinFolderTargetType(FOLDER_TARGET_MOD_TIME));
	add_item(new BinFolderTargetType(FOLDER_TARGET_TRACK_TYPE));
	add_item(new BinFolderTargetType(FOLDER_TARGET_WIDTH));
	add_item(new BinFolderTargetType(FOLDER_TARGET_HEIGHT));
	add_item(new BinFolderTargetType(FOLDER_TARGET_FRAMERATE));
	add_item(new BinFolderTargetType(FOLDER_TARGET_SAMPLERATE));
	add_item(new BinFolderTargetType(FOLDER_TARGET_CHANNELS));
	add_item(new BinFolderTargetType(FOLDER_TARGET_DURATION));
}

void BinFolderTargetPopup::activate_menu(BC_ListBoxItem *item)
{
	this->target = (BinFolderTarget *)item;
	BC_PopupMenu::activate_menu();
}


BinFolderTargetPatterns::BinFolderTargetPatterns(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_PATTERNS)
{
	text = 0;
	update("*");
}
BinFolderTargetPatterns::~BinFolderTargetPatterns()
{
	delete [] text;
}
 
void BinFolderTargetPatterns::save_xml(FileXML *file) {}
void BinFolderTargetPatterns::load_xml(FileXML *file) {}

void BinFolderTargetPatterns::copy_from(BinFolderTarget *that)
{
	BinFolderTargetPatterns *tp = (BinFolderTargetPatterns*)that;
	update(tp->text);
}

void BinFolderTargetPatterns::update(const char *text)
{
	delete [] this->text;
	this->text = cstrdup(text);
	filter->update_value(text);
}

BC_Window *BinFolderTargetPatterns::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetPatternsGUI(thread);
}


BinFolderTargetFileSize::BinFolderTargetFileSize(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_FILE_SIZE)
{
	file_size = 0;
	update(file_size, -1);
}
BinFolderTargetFileSize::~BinFolderTargetFileSize()
{
}

void BinFolderTargetFileSize::save_xml(FileXML *file)
{
	file->tag.set_property("FILE_SIZE", file_size);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetFileSize::load_xml(FileXML *file)
{
	int64_t file_size = file->tag.get_property("FILE_SIZE", this->file_size);
	double around = file->tag.get_property("AROUND", this->around);
	update(file_size, around);
}

void BinFolderTargetFileSize::copy_from(BinFolderTarget *that)
{
	BinFolderTargetFileSize *tp = (BinFolderTargetFileSize *)that;
	update(tp->file_size, tp->around);
}

void BinFolderTargetFileSize::update(int64_t file_size, double around)
{
	this->file_size = file_size;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(file_size, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetFileSize::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetFileSizeGUI(thread);
}


BinFolderTargetTime::BinFolderTargetTime(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_MOD_TIME)
{
	time_t t;  time(&t);
	mtime = (int64_t)t;
	update(mtime, -1);
}
BinFolderTargetTime::~BinFolderTargetTime()
{
}

void BinFolderTargetTime::save_xml(FileXML *file)
{
	file->tag.set_property("MTIME", mtime);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetTime::load_xml(FileXML *file)
{
	int64_t mtime = file->tag.get_property("MTIME", this->mtime);
	double around = file->tag.get_property("AROUND", this->around);
	update(mtime, around);
}

void BinFolderTargetTime::copy_from(BinFolderTarget *that)
{
	BinFolderTargetTime *tp = (BinFolderTargetTime *)that;
	update(tp->mtime, tp->around);
}

void BinFolderTargetTime::update(int64_t mtime, double around)
{
	this->mtime = mtime;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_date(mtime, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_duration(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetTime::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetTimeGUI(thread);
}


BinFolderTargetTrackType::BinFolderTargetTrackType(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_TRACK_TYPE)
{
	data_types = (1<<TRACK_AUDIO);
	update(data_types);
}
BinFolderTargetTrackType::~BinFolderTargetTrackType()
{
}

void BinFolderTargetTrackType::save_xml(FileXML *file)
{
	file->tag.set_property("DATA_TYPES", data_types);
}

void BinFolderTargetTrackType::load_xml(FileXML *file)
{
	int data_types = file->tag.get_property("DATA_TYPES", this->data_types);
	update(data_types);
}

void BinFolderTargetTrackType::copy_from(BinFolderTarget *that)
{
	BinFolderTargetTrackType *tp = (BinFolderTargetTrackType *)that;
	update(tp->data_types);
}

void BinFolderTargetTrackType::update(int data_types)
{
	this->data_types = data_types;
	this->around = -1;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	if( data_types & (1<<TRACK_AUDIO) ) {
		if( cp > txt && cp < ep ) *cp++ = ' ';
		cp += snprintf(cp, ep-cp, "%s",_("audio"));
	}
	if( data_types & (1<<TRACK_VIDEO) ) {
		if( cp > txt && cp < ep ) *cp++ = ' ';
		cp += snprintf(cp, ep-cp, "%s",_("video"));
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetTrackType::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetTrackTypeGUI(thread);
}


BinFolderTargetWidth::BinFolderTargetWidth(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_WIDTH)
{
	width = 0;
	update(width, -1);
}
BinFolderTargetWidth::~BinFolderTargetWidth()
{
}

void BinFolderTargetWidth::save_xml(FileXML *file)
{
	file->tag.set_property("WIDTH", width);
	file->tag.set_property("AROUND", around);
}
void BinFolderTargetWidth::load_xml(FileXML *file)
{
	int width = file->tag.get_property("WIDTH", this->width);
	double around = file->tag.get_property("AROUND", this->around);
	update(width, around);
}

void BinFolderTargetWidth::copy_from(BinFolderTarget *that)
{
	BinFolderTargetWidth *tp = (BinFolderTargetWidth *)that;
	update(tp->width, tp->around);
}

void BinFolderTargetWidth::update(int width, double around)
{
	this->width = width;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(width, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetWidth::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetWidthGUI(thread);
}


BinFolderTargetHeight::BinFolderTargetHeight(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_HEIGHT)
{
	height = 0;
	update(height, -1);
}
BinFolderTargetHeight::~BinFolderTargetHeight()
{
}

void BinFolderTargetHeight::save_xml(FileXML *file)
{
	file->tag.set_property("HEIGHT", height);
	file->tag.set_property("AROUND", around);
}
void BinFolderTargetHeight::load_xml(FileXML *file)
{
	int height = file->tag.get_property("HEIGHT", this->height);
	double around = file->tag.get_property("AROUND", this->around);
	update(height, around);
}

void BinFolderTargetHeight::copy_from(BinFolderTarget *that)
{
	BinFolderTargetHeight *tp = (BinFolderTargetHeight *)that;
	update(tp->height, tp->around);
}

void BinFolderTargetHeight::update(int height, double around)
{
	this->height = height;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(height, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetHeight::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetHeightGUI(thread);
}


BinFolderTargetFramerate::BinFolderTargetFramerate(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_FRAMERATE)
{
	framerate = 0;
	update(framerate, -1);
}
BinFolderTargetFramerate::~BinFolderTargetFramerate()
{
}

void BinFolderTargetFramerate::save_xml(FileXML *file)
{
	file->tag.set_property("FRAMERATE", framerate);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetFramerate::load_xml(FileXML *file)
{
	double framerate = file->tag.get_property("FRAMERATE", this->framerate);
	double around = file->tag.get_property("AROUND", this->around);
	update(framerate, around);
}

void BinFolderTargetFramerate::copy_from(BinFolderTarget *that)
{
	BinFolderTargetFramerate *tp = (BinFolderTargetFramerate *)that;
	update(tp->framerate, tp->around);
}

void BinFolderTargetFramerate::update(double framerate, double around)
{
	this->framerate = framerate;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(framerate, cp, ep, "%0.3f");
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep, "%0.3f");
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetFramerate::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetFramerateGUI(thread);
}


BinFolderTargetSamplerate::BinFolderTargetSamplerate(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_SAMPLERATE)
{
	samplerate = 0;
	update(samplerate, -1);
}

BinFolderTargetSamplerate::~BinFolderTargetSamplerate()
{
}

void BinFolderTargetSamplerate::save_xml(FileXML *file)
{
	file->tag.set_property("SAMPLERATE", samplerate);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetSamplerate::load_xml(FileXML *file)
{
	double samplerate = file->tag.get_property("SAMPLERATE", this->samplerate);
	double around = file->tag.get_property("AROUND", this->around);
	update(samplerate, around);
}

void BinFolderTargetSamplerate::copy_from(BinFolderTarget *that)
{
	BinFolderTargetSamplerate *tp = (BinFolderTargetSamplerate *)that;
	update(tp->samplerate, tp->around);
}

void BinFolderTargetSamplerate::update(int samplerate, double around)
{
	this->samplerate = samplerate;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(samplerate, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetSamplerate::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetSamplerateGUI(thread);
}


BinFolderTargetChannels::BinFolderTargetChannels(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_CHANNELS)
{
	channels = 0;
	update(channels, -1);
}
BinFolderTargetChannels::~BinFolderTargetChannels()
{
}

void BinFolderTargetChannels::save_xml(FileXML *file)
{
	file->tag.set_property("CHANNELS", channels);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetChannels::load_xml(FileXML *file)
{
	int channels = file->tag.get_property("CHANNELS", this->channels);
	double around = file->tag.get_property("AROUND", this->around);
	update(channels, around);
}

void BinFolderTargetChannels::copy_from(BinFolderTarget *that)
{
	BinFolderTargetChannels *tp = (BinFolderTargetChannels *)that;
	update(tp->channels, tp->around);
}

void BinFolderTargetChannels::update(int channels, double around)
{
	this->channels = channels;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_no(channels, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_no(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetChannels::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetChannelsGUI(thread);
}


BinFolderTargetDuration::BinFolderTargetDuration(BinFolderFilter *filter)
 : BinFolderTarget(filter, FOLDER_TARGET_DURATION)
{
	duration = 0;
	update(duration, -1);
}
BinFolderTargetDuration::~BinFolderTargetDuration()
{
}

void BinFolderTargetDuration::save_xml(FileXML *file)
{
	file->tag.set_property("DURATION", duration);
	file->tag.set_property("AROUND", around);
}

void BinFolderTargetDuration::load_xml(FileXML *file)
{
	int64_t duration = file->tag.get_property("DURATION", this->duration);
	double around = file->tag.get_property("AROUND", this->around);
	update(duration, around);
}

void BinFolderTargetDuration::copy_from(BinFolderTarget *that)
{
	BinFolderTargetDuration *tp = (BinFolderTargetDuration *)that;
	update(tp->duration, tp->around);
}

void BinFolderTargetDuration::update(int64_t duration, double around)
{
	this->duration = duration;
	this->around = around;
	char txt[BCSTRLEN], *cp = txt, *ep = cp + sizeof(txt)-1;
	show_duration(duration, cp, ep);
	if( around >= 0 && filter->op->type == FOLDER_OP_AROUND ) {
		if( cp < ep ) *cp++ = '+';
		show_duration(around, cp, ep);
	}
	*cp = 0;
	filter->update_value(txt);
}

BC_Window *BinFolderTargetDuration::new_gui(ModifyTargetThread *thread)
{
	return new ModifyTargetDurationGUI(thread);
}


BinFolderOp::BinFolderOp(BinFolderFilter *filter, int type)
 : BC_ListBoxItem(_(types[type]))
{
	this->filter = filter;
	this->type = type;
}

BinFolderOp::~BinFolderOp()
{
}

void BinFolderOp::copy_from(BinFolderOp *that)
{
	type = that->type;
}

BinFolderOpType::BinFolderOpType(int no)
 : BC_MenuItem(_(BinFolderOp::types[no]))
{
	this->no = no;
}
BinFolderOpType::~BinFolderOpType()
{
}

int BinFolderOpType::handle_event()
{
	BinFolderOpPopup *op_popup = (BinFolderOpPopup *)get_popup_menu();
	BinFolderList *folder_list = op_popup->folder_list;
	int i = folder_list->get_selection_number(FOLDER_COLUMN_OP, 0);
	if( i >= 0 ) {
		BinFolder *folder = folder_list->folder;
		BinFolderFilter *filter = folder->filters[i];
		filter->update_op(no);
		folder_list->create_list();
	}
	return 1;
}

BinFolderOpPopup::BinFolderOpPopup(BinFolderList *folder_list)
 : BC_PopupMenu(0, 0, 0, "", 0)
{
	this->folder_list = folder_list;
	op = 0;
}

void BinFolderOpPopup::create_objects()
{
	add_item(new BinFolderOpType(FOLDER_OP_AROUND));
	add_item(new BinFolderOpType(FOLDER_OP_EQ));
	add_item(new BinFolderOpType(FOLDER_OP_GE));
	add_item(new BinFolderOpType(FOLDER_OP_GT));
	add_item(new BinFolderOpType(FOLDER_OP_NE));
	add_item(new BinFolderOpType(FOLDER_OP_LE));
	add_item(new BinFolderOpType(FOLDER_OP_LT));
	add_item(new BinFolderOpType(FOLDER_OP_MATCHES));
}

void BinFolderOpPopup::activate_menu(BC_ListBoxItem *item)
{
	this->op = (BinFolderOp *)item;
	BC_PopupMenu::activate_menu();
}

double BinFolderOp::test(BinFolderTarget *target, Indexable *idxbl)
{
	return -1;
}

double BinFolderOpEQ::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v == 0 ? 1 : -1;
}

double BinFolderOpGT::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v > 0 ? 1 : -1;
}

double BinFolderOpGE::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v >= 0 ? 1 : -1;
}

double BinFolderOpNE::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v != 0 ? 1 : -1;
}

double BinFolderOpLT::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v < 0 ? 1 : -1;
}

double BinFolderOpLE::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v <= 0 ? 1 : -1;
}

double BinFolderOpMatches::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v;
}

double BinFolderOpAround::test(BinFolderTarget *target, Indexable *idxbl)
{
	double v = compare(target, idxbl);
	return v;
}

BinFolderValue::BinFolderValue(BinFolderFilter *filter, const char *text)
 : BC_ListBoxItem()
{
	this->filter = filter;
	update(text);
}

BinFolderValue::~BinFolderValue()
{
}


void BinFolderValue::update(const char *text)
{
	const char *cp = text;
	char txt[BCSTRLEN], *tp = txt;
	for( int i=sizeof(txt); --i>0 && *cp!=0 && *cp!='\n'; ++tp,++cp ) *tp = *cp;
	*tp = 0;
	set_text(txt);
}


BinFolderList::BinFolderList(BinFolder *folder, MWindow *mwindow,
		ModifyFolderGUI *window, int x, int y, int w, int h)
 : BC_ListBox(x, y, w, h, LISTBOX_TEXT, 0,
	0, 0, 1, 0, 0, LISTBOX_SINGLE, ICON_LEFT, 1)
{
	this->folder = folder;
	this->mwindow = mwindow;
	this->window = window;
	dragging_item = 0;
	set_process_drag(1);
	enabled_popup = 0;
	op_popup = 0;
	target_popup = 0;
	modify_target = 0;
}

BinFolderList::~BinFolderList()
{
	save_defaults(mwindow->defaults);
	delete modify_target;
}

void BinFolderList::create_objects()
{
	list_titles[FOLDER_COLUMN_ENABLE] = _("Enable");
	list_titles[FOLDER_COLUMN_TARGET] = _("Target");
	list_titles[FOLDER_COLUMN_OP]     = _("Op");
	list_titles[FOLDER_COLUMN_VALUE]  = _("Value");
	list_width[FOLDER_COLUMN_ENABLE]  = 80;
	list_width[FOLDER_COLUMN_TARGET]  = 80;
	list_width[FOLDER_COLUMN_OP]      = 50;
	list_width[FOLDER_COLUMN_VALUE]   = 180;
	load_defaults(mwindow->defaults);
	create_list();
	add_subwindow(enabled_popup = new BinFolderEnabledPopup(this));
	enabled_popup->create_objects();
	add_subwindow(op_popup = new BinFolderOpPopup(this));
	op_popup->create_objects();
	add_subwindow(target_popup = new BinFolderTargetPopup(this));
	target_popup->create_objects();

	modify_target = new ModifyTargetThread(this);
}

void BinFolderList::create_list()
{
	for( int i=0; i<FOLDER_COLUMNS; ++i )
		list_items[i].remove_all();
	for( int i=0; i<folder->filters.size(); ++i ) {
		BinFolderFilter *filter = folder->filters[i];
		list_items[FOLDER_COLUMN_ENABLE].append(filter->enabled);
		list_items[FOLDER_COLUMN_TARGET].append(filter->target);
		list_items[FOLDER_COLUMN_OP].append(filter->op);
		list_items[FOLDER_COLUMN_VALUE].append(filter->value);
	}
	update(list_items, list_titles, list_width, FOLDER_COLUMNS,
		get_xposition(), get_yposition(), get_highlighted_item(),
		1, 1);
}

int BinFolderList::handle_event()
{
	return 1;
}

int BinFolderList::selection_changed()
{
	if( !cursor_above() ) return 0;
	int no = get_selection_number(0, 0);
	if( no < 0 ) return 0;
	BinFolderFilter *filter = folder->filters[no];
	if( get_button_down() && get_buttonpress() == 3 ) {
		int cx = get_cursor_x(), col = -1;
		for( int i=0; col<0 && i<FOLDER_COLUMNS; ++i ) {
			int ofs = get_column_offset(i);
			if( cx >= ofs && cx < ofs+get_column_width(i) ) {
				col = i;  break;
			}
		}
		BC_ListBoxItem *item = col >= 0 ? get_selection(col, 0) : 0;
		if( item ) {
			deactivate_selection();
			switch( col ) {
			case FOLDER_COLUMN_ENABLE:
				enabled_popup->activate_menu(item);
				break;
			case FOLDER_COLUMN_TARGET:
				target_popup->activate_menu(item);
				break;
			case FOLDER_COLUMN_OP:
				op_popup->activate_menu(item);
				break;
			case FOLDER_COLUMN_VALUE: {
				modify_target->close_window();
				int cw = filter->target->type == FOLDER_TARGET_PATTERNS ? 400 : 320;
				int ch = filter->target->type == FOLDER_TARGET_PATTERNS ? 300 : 120;
				int cx, cy;  get_abs_cursor(cx, cy);
				if( (cx-=cw/2) < 50 ) cx = 50;
				if( (cy-=ch/2) < 50 ) cy = 50;
				modify_target->start(filter->target, cx, cy, cw, ch);
				break; }
			}
		}
	}
	return 1;
}

int BinFolderList::column_resize_event()
{
	for( int i = 0; i < FOLDER_COLUMNS; i++ ) {
		list_width[i] = get_column_width(i);
	}
	return 1;
}

int BinFolderList::drag_start_event()
{
	if( BC_ListBox::drag_start_event() ) {
		dragging_item = 1;
		return 1;
	}

	return 0;
}

int BinFolderList::drag_motion_event()
{
	if( BC_ListBox::drag_motion_event() ) {
		return 1;
	}
	return 0;
}

int BinFolderList::drag_stop_event()
{
	if( dragging_item ) {
		int src = get_selection_number(0, 0);
		int dst = get_highlighted_item();
		if( src != dst ) {
			move_filter(src, dst);
		}
		BC_ListBox::drag_stop_event();
		dragging_item = 0;
	}
	return 0;
}

void BinFolderList::move_filter(int src, int dst)
{
	BinFolderFilters &filters = folder->filters;
	BinFolderFilter *src_filter = filters[src];
	if( dst < 0 ) dst = filters.size()-1;

	if( dst != src ) {
		for( int i=src; i<filters.size()-1; ++i )
			filters[i] = filters[i+1];
		for( int i=filters.size(); --i>dst; )
			filters[i] = filters[i-1];
		filters[dst] = src_filter;
	}
}

void BinFolderList::save_defaults(BC_Hash *defaults)
{
	defaults->update("BIN_FOLDER_ENA", list_width[FOLDER_COLUMN_ENABLE]);
	defaults->update("BIN_FOLDER_TGT", list_width[FOLDER_COLUMN_TARGET]);
	defaults->update("BIN_FOLDER_OPR", list_width[FOLDER_COLUMN_OP]);
	defaults->update("BIN_FOLDER_VAL", list_width[FOLDER_COLUMN_VALUE]);
}
void BinFolderList::load_defaults(BC_Hash *defaults)
{
	list_width[FOLDER_COLUMN_ENABLE] = defaults->get("BIN_FOLDER_ENA", list_width[FOLDER_COLUMN_ENABLE]);
	list_width[FOLDER_COLUMN_TARGET] = defaults->get("BIN_FOLDER_TGT", list_width[FOLDER_COLUMN_TARGET]);
	list_width[FOLDER_COLUMN_OP]     = defaults->get("BIN_FOLDER_OPR", list_width[FOLDER_COLUMN_OP]);
	list_width[FOLDER_COLUMN_VALUE]  = defaults->get("BIN_FOLDER_VAL", list_width[FOLDER_COLUMN_VALUE]);
}

BinFolderAddFilter::BinFolderAddFilter(BinFolderList *folder_list, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->folder_list = folder_list;
}
BinFolderAddFilter::~BinFolderAddFilter()
{
}

int BinFolderAddFilter::handle_event()
{
	folder_list->modify_target->close_window();
// default new filter
	BinFolderFilter *filter = new BinFolderFilter();
	filter->update_enabled(FOLDER_ENABLED_OR);
	filter->update_target(FOLDER_TARGET_PATTERNS);
	filter->update_op(FOLDER_OP_MATCHES);
	BinFolderTargetPatterns *patterns = (BinFolderTargetPatterns *)(filter->target);
	filter->update_value(patterns->text);
	folder_list->folder->filters.append(filter);
	folder_list->create_list();
	return 1;
}

BinFolderDelFilter::BinFolderDelFilter(BinFolderList *folder_list, int x, int y)
 : BC_GenericButton(x, y, _("Del"))
{
	this->folder_list = folder_list;
}
BinFolderDelFilter::~BinFolderDelFilter()
{
}

int BinFolderDelFilter::handle_event()
{
	folder_list->modify_target->close_window();
	int no = folder_list->get_selection_number(0, 0);
	if( no >= 0 ) {
		folder_list->folder->filters.remove_object_number(no);
		folder_list->create_list();
	}
	return 1;
}

BinFolderApplyFilter::BinFolderApplyFilter(BinFolderList *folder_list, int x, int y)
 : BC_GenericButton(x, y, _("Apply"))
{
	this->folder_list = folder_list;
}
BinFolderApplyFilter::~BinFolderApplyFilter()
{
}

int BinFolderApplyFilter::handle_event()
{
	ModifyFolderThread *thread = folder_list->window->thread;
	thread->original->copy_from(thread->folder);
	thread->agui->async_update_assets();
	return 1;
}


NewFolderGUI::NewFolderGUI(NewFolderThread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": New folder"),
		x, y, w, h, -1, -1, 0, 0, 1)
{
	this->thread = thread;
}

NewFolderGUI::~NewFolderGUI()
{
}

void NewFolderGUI::create_objects()
{
	lock_window("NewFolderGUI::create_objects");
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Folder name:")));
	y += title->get_h() + 5;
	const char *text = !thread->is_clips ? _("media bin") : _("clip bin");
	add_subwindow(text_box = new BC_TextBox(x, y, 300, 1, text));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

const char* NewFolderGUI::get_text()
{
	return text_box->get_text();
}


NewFolderThread::NewFolderThread(AWindowGUI *agui)
 : BC_DialogThread()
{
	this->agui = agui;
	is_clips = -1;
}

NewFolderThread::~NewFolderThread()
{
	close_window();
}

void NewFolderThread::start(int x, int y, int w, int h, int is_clips)
{
	close_window();
	this->is_clips = is_clips;
	this->wx = x;  this->wy = y;
	this->ww = w;  this->wh = h;
	Thread::start();
}

BC_Window *NewFolderThread::new_gui()
{
	window = new NewFolderGUI(this, wx, wy, ww, wh);
	window->create_objects();
	return window;
}

void NewFolderThread::handle_done_event(int result)
{
	if( !result ) {
		const char *text = window->get_text();
		agui->mwindow->new_folder(text, is_clips);
	}
}

void NewFolderThread::handle_close_event(int result)
{
	is_clips = -1;
}

ModifyFolderGUI::ModifyFolderGUI(ModifyFolderThread *thread, int x, int y, int w, int h)
 : BC_Window(_(PROGRAM_NAME ": Modify folder"), x, y, w, h, 320, 200, 1, 0, 1)
{
	this->thread = thread;
}

ModifyFolderGUI::~ModifyFolderGUI()
{
}

int ModifyFolderGUI::receive_custom_xatoms(xatom_event *event)
{
	if( event->message_type == modify_folder_xatom ) {
		update_filters();
		return 1;
	}
	return 0;
}

void ModifyFolderGUI::async_update_filters()
{
        xatom_event event;
        event.message_type = modify_folder_xatom;
        send_custom_xatom(&event);
}


void ModifyFolderGUI::create_objects()
{
	lock_window("ModifyFolderGUI::create_objects");
	modify_folder_xatom = create_xatom("CWINDOWGUI_UPDATE_FILTERS");
	int x = 10, y = 10;
	BC_Title *title;
	add_subwindow(title = new BC_Title(x, y, _("Enter the name of the folder:")));
	const char *text = !thread->folder->is_clips ? _("Media") : _("Clips");
	int tw = BC_Title::calculate_w(this, text, LARGEFONT);
	int x0 = get_w() - 50 - tw;
	add_subwindow(text_title = new BC_Title(x0, y, text, LARGEFONT, YELLOW));
	y += title->get_h() + 10;
	add_subwindow(text_box = new BC_TextBox(x, y, 300, 1, thread->folder->title));
	y += text_box->get_h() + 10;
	int lh = get_h() - y - BC_OKButton::calculate_h() - 30;
	int lw = get_w() - x - 160;
	add_subwindow(folder_list =
		new BinFolderList(thread->folder, thread->agui->mwindow, this, x, y, lw, lh));
	folder_list->create_objects();
	int x1 = x + folder_list->get_w() + 15, y1 = y;
	add_subwindow(add_filter = new BinFolderAddFilter(folder_list, x1, y1));
	y1 += add_filter->get_h() + 10;
	add_subwindow(del_filter = new BinFolderDelFilter(folder_list, x1, y1));
	y1 += del_filter->get_h() + 10;
	add_subwindow(apply_filter = new BinFolderApplyFilter(folder_list, x1, y1));
	add_subwindow(ok_button = new BC_OKButton(this));
	add_subwindow(cancel_button = new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int ModifyFolderGUI::resize_event(int w, int h)
{
	MWindow *mwindow = thread->agui->mwindow;
	mwindow->session->bwindow_w = w;
	mwindow->session->bwindow_h = h;
	int tx = text_title->get_x() + w - get_w();
	int ty = text_title->get_y();
	text_title->reposition_window(tx, ty);
	int lx = folder_list->get_x();
	int ly = folder_list->get_y();
	int lh = h - ly - BC_OKButton::calculate_h() - 30;
	int lw = w - lx - 160;
	folder_list->reposition_window(lx, ly, lw, lh);
	int x1 = lx + lw + 15;
	add_filter->reposition_window(x1, add_filter->get_y());
	del_filter->reposition_window(x1, del_filter->get_y());
	apply_filter->reposition_window(x1,apply_filter->get_y());
	ok_button->resize_event(w, h);
	cancel_button->resize_event(w, h);
	return 1;
}

const char* ModifyFolderGUI::get_text()
{
	return text_box->get_text();
}

void ModifyFolderGUI::update_filters()
{
	folder_list->create_list();
}


ModifyFolderThread::ModifyFolderThread(AWindowGUI *agui)
 : BC_DialogThread()
{
	this->agui = agui;
	original = 0;
	modify_edl = 0;
	folder = 0;
}

ModifyFolderThread::~ModifyFolderThread()
{
	close_window();
	delete folder;
}

void ModifyFolderThread::start(BinFolder *folder, int x, int y, int w, int h)
{
	close_window();
	this->original = folder;
	this->modify_edl = agui->mwindow->edl;
	this->modify_edl->add_user();
	this->folder = new BinFolder(*folder);
	this->wx = x;  this->wy = y;
	this->ww = w;  this->wh = h;
	Thread::start();
}

BC_Window *ModifyFolderThread::new_gui()
{
	window = new ModifyFolderGUI(this, wx, wy, ww, wh);
	window->create_objects();
	return window;
}

void ModifyFolderThread::handle_done_event(int result)
{
	if( !result ) {
		const char *title = window->get_text();
		if( strcmp(folder->title, title) ) {
			if( agui->mwindow->edl->get_folder_number(title) >= 0 ) {
				eprintf("folder already exists: %s", title);
				result = 1;
			}
			else
				strncpy(folder->title, title,sizeof(folder->title));
		}
	}
	if( !result ) {
		original->copy_from(folder);
		agui->async_update_assets();
	}
	delete folder;  folder = 0;
	original = 0;
	modify_edl->remove_user();
}

void ModifyFolderThread::handle_close_event(int result)
{
}


ModifyTargetThread::ModifyTargetThread(BinFolderList *folder_list)
 : BC_DialogThread()
{
	this->folder_list = folder_list;
	target = 0;
}

ModifyTargetThread::~ModifyTargetThread()
{
	close_window();
}

void ModifyTargetThread::start(BinFolderTarget *target, int x, int y, int w, int h)
{
	this->target = target;
	wx = x;  wy = y;
	ww = w;  wh = h;
	Thread::start();
}

BC_Window *ModifyTargetThread::new_gui()
{
	window = (ModifyTargetGUI *)target->new_gui(this);
	window->create_objects();
	return window;
}

void ModifyTargetThread::handle_done_event(int result)
{
	if( !result ) {
		window->update();
	        folder_list->window->async_update_filters();
	}
}

void ModifyTargetThread::handle_close_event(int result)
{
	target = 0;
}

ModifyTargetGUI::ModifyTargetGUI(ModifyTargetThread *thread, int allow_resize)
 : BC_Window(_(PROGRAM_NAME ": Modify target"),
		thread->wx, thread->wy, thread->ww, thread->wh,
		-1, -1, allow_resize, 0, 1)
{
	this->thread = thread;
}

ModifyTargetGUI::~ModifyTargetGUI()
{
}

void ModifyTargetGUI::create_objects(BC_TextBox *&text_box)
{
	lock_window("ModifyTargetGUI::create_objects");
	int x = 10, y = 10;
	const char *text = thread->target->filter->value->get_text();
	add_subwindow(text_box = new BC_TextBox(x, y, get_w()-20, 1, text));
	add_subwindow(new BC_OKButton(this));
	add_subwindow(new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int ModifyTargetGUI::resize_event(int w, int h)
{
	return BC_WindowBase::resize_event(w, h);
}

ModifyTargetPatternsGUI::ModifyTargetPatternsGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread, 1)
{
	this->thread = thread;
	scroll_text_box = 0;
	text_rowsz = 0;
}

ModifyTargetPatternsGUI::~ModifyTargetPatternsGUI()
{
	delete scroll_text_box;
}

void ModifyTargetPatternsGUI::create_objects()
{
	lock_window("ModifyTargetPatternsGUI::create_objects");
	BinFolderTargetPatterns *target = (BinFolderTargetPatterns *)thread->target;
	int x = 10, y = 10;
	int text_font = MEDIUMFONT;
	text_rowsz = get_text_ascent(text_font)+1 + get_text_descent(text_font)+1;
	int th = get_h() - y - BC_OKButton::calculate_h() - 20;
	int rows = th / text_rowsz;
	int text_len = strlen(target->text);
	if( text_len < BCTEXTLEN ) text_len = BCTEXTLEN;
	scroll_text_box = new BC_ScrollTextBox(this, x, y, get_w()-20, rows,
		target->text, 2*text_len);
	scroll_text_box->create_objects();
	add_subwindow(ok_button = new BC_OKButton(this));
	add_subwindow(cancel_button = new BC_CancelButton(this));
	show_window();
	unlock_window();
}

int ModifyTargetPatternsGUI::resize_event(int w, int h)
{
	int tx = scroll_text_box->get_x();
	int ty = scroll_text_box->get_y();
	int th = h - ty - BC_OKButton::calculate_h() - 20;
	int tw = w - 20;
	int rows = th / text_rowsz;
	scroll_text_box->reposition_window(tx, ty, tw, rows);
	ok_button->resize_event(w, h);
	cancel_button->resize_event(w, h);
	return 1;
}

void ModifyTargetPatternsGUI::update()
{
	BinFolderTargetPatterns *target = (BinFolderTargetPatterns *)thread->target;
	const char *cp = scroll_text_box->get_text();
	target->update(cp);
}


ModifyTargetFileSizeGUI::ModifyTargetFileSizeGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetFileSizeGUI::~ModifyTargetFileSizeGUI()
{
}

void ModifyTargetFileSizeGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetFileSizeGUI::update()
{
	BinFolderTargetFileSize *target = (BinFolderTargetFileSize *)thread->target;
	double file_size = target->file_size, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, file_size, around);
	target->update(file_size, around);
}


ModifyTargetTimeGUI::ModifyTargetTimeGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetTimeGUI::~ModifyTargetTimeGUI()
{
}

void ModifyTargetTimeGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetTimeGUI::update()
{
	BinFolderTargetTime *target = (BinFolderTargetTime *)thread->target;
	int64_t mtime = target->mtime;  double around = target->around;
	const char *cp = text_box->get_text(); char *bp = 0;
	int64_t v = scan_date(cp, bp);
	if( bp > cp ) {
		mtime = v;
		if( *bp == '+' ) {
			v = scan_duration(cp=bp+1, bp);
			if( bp > cp ) around = v;
		}
	}
	target->update(mtime, around);
}


ModifyTargetTrackTypeGUI::ModifyTargetTrackTypeGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetTrackTypeGUI::~ModifyTargetTrackTypeGUI()
{
}

void ModifyTargetTrackTypeGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetTrackTypeGUI::update()
{
	BinFolderTargetTrackType *target = (BinFolderTargetTrackType *)thread->target;
	const char *cp = text_box->get_text();
	int data_types = 0;
	if( bstrcasestr(cp, _("audio")) ) data_types |= (1<<TRACK_AUDIO);
	if( bstrcasestr(cp, _("video")) ) data_types |= (1<<TRACK_VIDEO);
	target->update(data_types);
}


ModifyTargetWidthGUI::ModifyTargetWidthGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetWidthGUI::~ModifyTargetWidthGUI()
{
}

void ModifyTargetWidthGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetWidthGUI::update()
{
	BinFolderTargetWidth *target = (BinFolderTargetWidth *)thread->target;
	double width = target->width, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, width, around);
	target->update(width, around);
}


ModifyTargetHeightGUI::ModifyTargetHeightGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetHeightGUI::~ModifyTargetHeightGUI()
{
}

void ModifyTargetHeightGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetHeightGUI::update()
{
	BinFolderTargetHeight *target = (BinFolderTargetHeight *)thread->target;
	double height = target->height, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, height, around);
	target->update(height, around);
}


ModifyTargetFramerateGUI::ModifyTargetFramerateGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetFramerateGUI::~ModifyTargetFramerateGUI()
{
}

void ModifyTargetFramerateGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetFramerateGUI::update()
{
	BinFolderTargetFramerate *target = (BinFolderTargetFramerate *)thread->target;
	double framerate = target->framerate, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, framerate, around);
	target->update(framerate, around);
}


ModifyTargetSamplerateGUI::ModifyTargetSamplerateGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetSamplerateGUI::~ModifyTargetSamplerateGUI()
{
}

void ModifyTargetSamplerateGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetSamplerateGUI::update()
{
	BinFolderTargetSamplerate *target = (BinFolderTargetSamplerate *)thread->target;
	double samplerate = target->samplerate, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, samplerate, around);
	target->update(samplerate, around);
}


ModifyTargetChannelsGUI::ModifyTargetChannelsGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetChannelsGUI::~ModifyTargetChannelsGUI()
{
}

void ModifyTargetChannelsGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetChannelsGUI::update()
{
	BinFolderTargetChannels *target = (BinFolderTargetChannels *)thread->target;
	double channels = target->channels, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	scan_around(cp, bp, channels, around);
	target->update(channels, around);
}


ModifyTargetDurationGUI::ModifyTargetDurationGUI(ModifyTargetThread *thread)
 : ModifyTargetGUI(thread)
{
}

ModifyTargetDurationGUI::~ModifyTargetDurationGUI()
{
}

void ModifyTargetDurationGUI::create_objects()
{
	ModifyTargetGUI::create_objects(text_box);
}

void ModifyTargetDurationGUI::update()
{
	BinFolderTargetDuration *target = (BinFolderTargetDuration *)thread->target;
	int64_t duration = target->duration, around = target->around;
	const char *cp = text_box->get_text();  char *bp = 0;
	int64_t v = scan_duration(cp, bp);
	if( bp > cp ) {
		duration = v;
		if( *bp == '+' ) {
			v = scan_duration(cp=bp+1, bp);
			if( bp > cp ) around = v;
		}
	}
	target->update(duration, around);
}

