#include "../libzmpeg3/libzmpeg3.h"
#include "arraylist.h"
#include "mediadb.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include <sys/time.h>
#include <sys/resource.h>

// c++ `cat x86_64/c_flags` -c -o x86_64/cutads.o cutads.C
// c++ -pthread -o x86_64/cutads x86_64/cutads.o x86_64/mediadb.o x86_64/filexml.o
//    ../libzmpeg3/x86_64/libzmpeg3.a ../db/x86_64/db.a -lX11

using namespace std;
#define fail(s) do { printf("fail %s%s:%d\n",__func__,#s,__LINE__); return 1; } while(0)

/* missing from system headers, no /usr/include <linux/ioprio.h>
 *   IOPRIO_WHO_PROCESS, IOPRIO_CLASS_SHIFT, IOPRIO_CLASS_IDLE */
enum { IOPRIO_CLASS_NONE, IOPRIO_CLASS_RT, IOPRIO_CLASS_BE, IOPRIO_CLASS_IDLE, };
#define IO_CLASS(n) (((int)(n)) << 13)
#define IO_WHO_PROCESS  1
#include <sys/syscall.h>
#include <asm/unistd.h>

// commercial edge detection:
// must have audio < min_audio
//   and within +- check_margin seconds of low audio
//   must have video < min_video or delta video > min_dvideo

static double min_audio = 0.5e-3 * 32767;   // (-63db quite quiet)
static double min_video = 0.1 * 255;        // (pretty dark)
static double min_dvideo = 0.1 * 255;       // (gittery)
static double lo_video = 0.075 * 255;       // (dark)
static double hi_video = 0.925 * 255;       // (light)
static double check_margin = 1.0;           // t-1.0..t+1.0 secs
static double min_clip_time = 3;            // ignore clips shorter than this
static int    video_cutoff = 15;            // video outside color space

MediaDb *db = 0;

class Src;
class Weights;
class MarginFrame;
class Margin;
class Video;
class Audio;
class Scan;

static int ioprio_set(int which, int who, int ioprio)
{
	return syscall(SYS_ioprio_set, which, who, ioprio);
}

static inline int clip(int v, int mn, int mx)
{
  return v<mn ? mn : v>mx ? mx : v;
}



class Src
{
	char path[1024];
	zmpeg3_t* zsrc;
public:
	bool open() { return zsrc != 0; }
	operator zmpeg3_t *() { return zsrc; }
	zmpeg3_t *operator ->() { return zsrc; }
	const char *asset_path() { return path; }

	Src(const char *fn) {
		strcpy(path, fn);
		int ret;  zsrc = new zmpeg3_t(path, ret,
			ZIO_UNBUFFERED+ZIO_SINGLE_ACCESS);
		if( ret ) { delete zsrc; zsrc = 0; }
	}
	~Src() { delete zsrc; }
};



class Weights
{
	int cur;
	ArrayList<double> positions;
	ArrayList<double> values;
public:
	int prev() { return cur > 0 ? --cur : -1; }
	int next() { return cur < positions.size()-1 ? ++cur : -1; }
	double position() { return positions.get(cur); }
	double value() { return values.get(cur); }
	double next_value() { return values.get(cur++); }
	int length() { return values.size() - cur; }
	int find(double pos);
	int locate(double pos);
	double err(double start, double len, double irate,
		double *iweights, int iframes);
	void add(double p, double v) { positions.append(p); values.append(v); }
	int minimum_forward(double start, double end, double &time);
	int minimum_backward(double start, double end, double &time);
	int video_forward(double start, double end, double &time);
	int audio_forward(double start, double end, double &time);
	int video_backward(double start, double end, double &time);
	int audio_backward(double start, double end, double &time);
	double get_weight(double pos);
	double average(double pos, int len);
	int save(double pos, double *wp, int len);
	void write(const char *fn);
	Weights() : cur(0) {}
	~Weights() {}
};

int Weights::find(double pos)
{
	int l = -1;
	int r = positions.size();
  	while( (r - l) > 1 ) {
		int i = (l+r) >> 1;
		double v = positions.get(i);
		if( pos == v ) return i;
		if( pos > v ) l = i; else r = i;
	}
	return l;
}

int Weights::locate(double pos)
{
	int ret = find(pos);
	if( ret < 0 ) ret = 0;
	return cur = ret;
}

double Weights::
err(double pos, double len, double irate,
	double *iweights, int iframes)
{
// trim leading / trailing dark video
	locate(pos);
	if( position() > pos ) return 256.;
	double end_pos = pos + len;
	while( position() < end_pos ) {
		if( value() >= min_video ) break;
		if( next() < 0 ) return 256.;
	}
	int l = 0, n = 0;
	double vv = 0, lvv = 0, dd = 0, ldd = 0, ldv = 0;
	while( position() < end_pos ) {
		int k = (position()-pos) * irate + 1./1001+0.5;
		if( k >= 0 ) {
			if( k >= iframes ) break;
			double v = value(), dv = v - iweights[k];
			vv += fabs(dv);
			dd += fabs(dv - ldv);  ldv = dv;
			if( v > min_video ) { lvv = vv; ldd = dd; l = n; }
			++n;
		}
		if( next() < 0 ) break;
	}

	if( ldd < lvv && lvv < MEDIA_MEAN_ERRLMT*l ) lvv = ldd;
	return !l ? MEDIA_WEIGHT_ERRLMT : lvv / l;
}

double Weights::
get_weight(double pos)
{
	locate(pos);
	return position() > pos ? -1 : value();
}

double Weights::
average(double pos, int len)
{
	locate(pos);
	int n = length();
	if( len < n ) n = len;
	double v = 0;
	for( int i=len; --i>=0; ) v += next_value();
	return n > 0 ? v / n : 256;
}

int Weights::
save(double pos, double *wp, int len)
{
	locate(pos);
	if( position() > pos ) fail();
	if( length() < len ) fail();
	for( int i=len; --i>=0; ++wp ) *wp = next_value();
	return 0;
}

void Weights::
write(const char *fn)
{
	FILE *fp = fopen(fn,"w");
	if( fp ) {
		locate(0);
		if( length() > 0 ) do {
			fprintf(fp,"%f %f\n",position(),value());
		} while( next() >= 0 );
		fclose(fp);
	}
}



class Video
{
	zmpeg3_t *src;
	char *yp, *up, *vp;
	int trk, trks;
	int zpid, w, h, ww, hh, is_scaled;
	Weights *weights;
	double rate, vorigin;
	int64_t len, pos;
	uint8_t sbfr[SFRM_SZ];
	char name[64];
	Margin *margin;

	void init();
public:
	uint8_t *get_y() { return (uint8_t*)yp; }
	uint8_t *get_u() { return (uint8_t*)up; }
	uint8_t *get_v() { return (uint8_t*)vp; }
	int tracks() { return trks; }
	int track() { return trk; }
	int width() { return w; }
	int height() { return h; }
	int coded_width() { return ww; }
	int coded_height() { return hh; }
	int pid() { return zpid; }
	bool eof() { return src->end_of_video(trk); }
	double time() { return src->get_video_time(trk); }
	void set_title(char *cp) { strcpy(name,cp); }
	char *title() { return name; }
	double framerate() { return rate; }
	int64_t frame_count(double position) { return position*rate + 1./1001; }
	double frame_position(int64_t count) { return count / rate; }
	void set_origin(double t) { vorigin = t; }
	double origin() { return vorigin; }
	int64_t length() { return len; }
	int64_t frame_no() { return pos; }
	void disable_weights() { delete weights;  weights = 0; }
	void enable_weights() { weights = new Weights(); }
	void write_weights(const char *fn) { weights->write(fn); }
	void margin_frames(Margin *mp=0) { margin = mp; }
	int forward(double start, double end, double &time) {
		return weights->video_forward(start, end, time);
	}
	int backward(double start, double end, double &time) {
		return weights->video_backward(start, end, time);
	}
	double err(double pos, double len, double irate,
		double *iweights, int iframes) {
		return weights->err(pos, len, irate, iweights, iframes);
	}
	double frame_weight(double pos) {
		return weights->get_weight(pos);
	}
	double average_weight(double pos, double len) {
		return weights->average(pos, len*rate);
	}
	int save_weights(double pos, double *wp, int len) {
		return weights->save(pos, wp, len);
	}
	void init(zmpeg3_t *zsrc, int ztrk);
	int load_frame();
	int read_frame();
	uint8_t *scaled();
	double raw_weight();
	double weight(uint8_t *bp);

	Video() { weights = 0; }
	~Video() { delete weights; }
} video;

void Video::
init(zmpeg3_t *zsrc, int ztrk)
{
	src = zsrc;  trk = ztrk;
	trks = src->total_vstreams();
	zpid = src->video_pid(trk);
	rate = src->frame_rate(trk);
	w = src->video_width(trk);
	h = src->video_height(trk);
	ww = src->coded_width(trk);
	hh = src->coded_height(trk);
	name[0] = 0;
	margin = 0;
	is_scaled = 0;
	vorigin = -1;
	len = src->video_frames(trk);
	pos = 0;
}

int Video::load_frame()
{
	++pos;  is_scaled = 0;
	return src->read_yuvframe_ptr(&yp, &up, &vp,trk);
}



class MarginFrame
{
	uint8_t sbfr[SFRM_SZ];
public:
	uint8_t *frame() { return sbfr; }
	MarginFrame(uint8_t *bp) { memcpy(sbfr,bp,sizeof(sbfr)); }
};

class Margin : public ArrayList<MarginFrame*>
{
	int cur;
	int64_t start_frame;
	double start_time;
public:
	int locate(double t);
	void copy(Margin *that);
	double position() { return start_time + video.frame_position(cur); }
	int check(Clips *clips, double start, double end, int group_mask);
	int length() { return size() - cur; }
	uint8_t *get_frame() { return get(cur++)->frame(); }
	void add(uint8_t *sbfr) { append(new MarginFrame(sbfr)); }

	Margin(int64_t frame, double time) : cur(0) {
		start_frame = frame;  start_time = time;
	}
	~Margin() { remove_all_objects(); }
} *prefix = 0, *suffix = 0;


int Margin::
locate(double t)
{
	int pos = video.frame_count(t-start_time);
	if( pos < 0 || pos >= size() ) return -1;
	return cur = pos;
}

void Margin::
copy(Margin *that)
{
	this->start_frame = that->start_frame + cur;
	this->start_time = that->position();
	while( that->length() > 0 ) add(that->get_frame());
}

int Margin::
check(Clips *clips, double start, double end, int group_mask)
{
	if( locate(start) < 0 ) fail();
	double pos;
	while( length() > 0 && (pos=position()) < end ) {
		uint8_t *sbfr = get_frame();  int fid;
// write_pbm(sbfr,SWIDTH,SHEIGHT,"/tmp/dat/f%07.3f.pbm",pos-video.origin());
		db->get_frame_key(sbfr, fid, clips, pos, group_mask);
	}
	return 0;
}


int Video::read_frame()
{
	if( load_frame() ) fail();
// write_pbm(get_y(),width(),height(),"/tmp/dat/f%05ld.pbm",pos);
	if( margin ) margin->add(scaled());
	if( weights ) weights->add(time(), weight(scaled()));
	return 0;
}

uint8_t *Video::scaled()
{
	if( !is_scaled ) {
		is_scaled = 1;
		uint8_t *yp = get_y();  int sh = hh;
//		while( sh>h && *yp<video_cutoff ) { yp += ww;  --sh; }
//static int fno = 0;
//write_pbm(yp,ww,hh,"/tmp/data/f%04d.pbm",fno);
		Scale(yp,0,ww,sh, 0,0,w,h).
			scale(sbfr,SWIDTH,SHEIGHT, 0,0,SWIDTH,SHEIGHT);
//write_pbm(sbfr,SWIDTH,SHEIGHT,"/tmp/data/s%04d.pbm",fno);
//++fno;
	}
	return sbfr;
}

double Video::raw_weight()
{
	uint8_t *yp = get_y();
	int64_t wt = 0;
	for( int y=h; --y>=0; yp+=ww ) {
		uint8_t *bp = yp;  int n = 0;
		for( int x=w; --x>=0; ++bp ) n += *bp;
		wt += n;
	}
	return (double)wt / (w*h);
}

double Video::weight(uint8_t *bp)
{
	int64_t wt = 0;
	for( int i=SFRM_SZ; --i>=0; ++bp ) wt += *bp;
	return (double)wt / SFRM_SZ;
}

int Weights::
minimum_forward(double start, double end, double &time)
{
	locate(start);
	double t = position();
	double min = value();
	while( next() >= 0 && position() < end ) {
		if( value() > min ) continue;
		min = value();  t = position();
	}
	time = t;
	return 0;
}

int Weights::
minimum_backward(double start, double end, double &time)
{
	locate(end);
	double t = position();
	double min = value();
	while( prev() >= 0 && position() >= start ) {
		if( value() > min ) continue;
		min = value();  t = position();
	}
	time = t;
	return 0;
}

// find trailing edge scanning backward
//   must have video < min_video or delta video > min_dvideo
int Weights::
video_backward(double start, double end, double &time)
{
	locate(end);
	double t = position(), v = value();
	for(;;) {
		if( t < start ) return 1;
		double vv = v;  v = value();
		if( v <= min_video ) break;
		if( fabs(vv-v) >= min_dvideo ) break;
		if( prev() < 0 ) fail();
		t = position();
	}
	while( prev() >= 0 ) {
		if( position() < start ) break;
		t = position();
		if( value() > min_video ) break;
	}
	time = t;
	return 0;
}

// find leading edge scanning backward
//   must have audio < min_audio
int Weights::
audio_backward(double start, double end, double &time)
{
	locate(end);
	double t = position();
	for(;;) {
		if( t < start ) fail();
		if( value() <= min_audio ) break;
		if( prev() < 0 ) fail();
		t = position();
	}
	time = t;
	return 0;
}

// find trailing edge scanning forward
//   must have video < min_video or delta video > min_dvideo
int Weights::
video_forward(double start, double end, double &time)
{
	locate(start);
	double t = position(), v = value();
	for(;;) {
		if( t >= end ) return 1;
		double vv = v;  v = value();
		if( v <= min_video ) break;
		if( fabs(vv-v) >= min_dvideo ) break;
		if( next() < 0 ) fail();
		t = position();
	}
	while( next() >= 0 ) {
		if( position() >= end ) break;
		t = position();
		if( value() > min_video ) break;
	}
	time = t;
	return 0;
}

// find leading edge scanning forward
//   must have audio < min_audio
int Weights::
audio_forward(double start, double end, double &time)
{
	locate(start);
	double t = position();
	for(;;) {
		if( t >= end ) fail();
		if( value() <= min_audio ) break;
		if( next() < 0 ) fail();
		t = position();
	}
	time = t;
	return 0;
}



class Audio {
	zmpeg3_t *src;
	int trk, trks;
	int chans, rate;
	int64_t len, pos;
	Weights *weights;
	int bfr_sz;
	short *bfr;

public:
	int tracks() { return trks; }
	int track() { return trk; }
	int channels() { return chans; }
	int samplerate() { return rate; }
	int64_t length() { return len; }
	int64_t sample_no() { return pos; }
	bool eof() { return src->end_of_audio(trk); }
	double time() { return src->get_audio_time(trk); }
	void set_buffer(int n) {
		delete [] bfr;
		bfr = new short[bfr_sz = clip(n,256,32767)];
	}
	void disable_weights() { delete weights;  weights = 0; }
	void enable_weights() { weights = new Weights(); }
	void write_weights(const char *fn) { weights->write(fn); }
	int forward(double start, double end, double &time) {
		//if( !weights->audio_forward(start, end, time) ) return 0;
		return weights->minimum_forward(start, end, time);
	}
	int backward(double start, double end, double &time) {
		//if( !weights->audio_backward(start, end, time) ) return 0;
		return weights->minimum_backward(start, end, time);
	}

	void init(zmpeg3_t *zsrc, int ztrk);
	int load_samples(int ch);
	int read_samples();
	double weight();

	Audio() { weights = 0; bfr = 0; }
	~Audio() { delete weights;  delete [] bfr; }
} audio;

void Audio::init(zmpeg3_t *zsrc, int ztrk)
{
	src = zsrc;  trk = ztrk;
	trks = src->total_astreams();
	chans = src->audio_channels(trk);
	rate = src->sample_rate(trk);
	len = src->audio_samples(trk);
	bfr = 0; bfr_sz = 0;
	pos = 0;
}

int Audio::load_samples(int ch)
{
	if( ch ) return src->reread_audio(bfr, ch, bfr_sz, trk);
	if( src->read_audio(bfr, ch, bfr_sz, trk) ) fail();
	pos += bfr_sz;
	return 0;
}

int Audio::read_samples()
{
	if( !weights ) return load_samples(0);
	double wt = 0;
	for( int ch=0; ch<chans; wt+=weight(), ++ch )
		if( load_samples(ch) ) fail();
	weights->add(time(), wt/chans);
	return 0;
}

double Audio::weight()
{
	int64_t wt = 0;  short *bp = bfr;
	int v = *bp++;
	for( int i=bfr_sz; --i>0; v=*bp++ ) wt += abs(v-*bp);
	return (double)wt / (bfr_sz-1);
}

class Scan
{
	int clip_id;
	int64_t creation_time, system_time;
	int prefix_frames, suffix_frames, frames;
	double prefix_length, suffix_length;
	double reaction_time, margin;
	double clip_start, clip_end, *clip_weights;
	double start_time, end_time;
	const char *asset_path;
	static void set_priority(int pr, int io) {
		setpriority(PRIO_PROCESS, 0, pr);  // lowest cpu priority
		ioprio_set(IO_WHO_PROCESS, 0, IO_CLASS(io));
	}
	class low_priority { public:
		low_priority() { set_priority(19, IOPRIO_CLASS_IDLE); }
		~low_priority() { set_priority(0, IOPRIO_CLASS_BE); }
	};
	class high_priority { public:
		high_priority() { set_priority(-4, IOPRIO_CLASS_BE); }
		~high_priority() { set_priority(0, IOPRIO_CLASS_BE); }
	};
	int write_clip();
public:
	int scan(Src &src, Deletions *dels);
	int cut_clip(Src &src, Dele *del, Dele *next);
	int add_clip();
	int verify_clip(double position, double length);
	int save_group(Margin *margin, double pos, int frame_no, int len, int group);

	Scan();
	~Scan();
};

Scan::
Scan()
{
	prefix_length = suffix_length = 2; // length of prefix/suffix in seconds
	prefix_frames = video.frame_count(prefix_length);
	suffix_frames = video.frame_count(suffix_length);
	reaction_time = 1.0; // 1 sec reaction time for default start/end time
	margin = 1.0; // 1 sec search window -1.0..+1.0 search margin
	creation_time = system_time = 0;
	clip_start = clip_end = 0;
	clip_id = 0; clip_weights = 0;
	db = new MediaDb();
	db->openDb();
	db->detachDb();
}

Scan::
~Scan()
{
	db->closeDb();
	db = 0;
}

int Scan::
verify_clip(double position, double length)
{
	Clips clips;
	double start = position, end = position + length;
	if( prefix->check(&clips, start, start+prefix_length, 1) ) fail();
	if( !clips.count ) return 0;
	if( suffix->check(&clips, end-suffix_length, end, 2) ) fail();
	double avg_wt = video.average_weight(position, length);
	for( Clip *clip=clips.first; clip; clip=clip->next ) {
		int clip_id = clip->clip_id;
		if( db->clip_id(clip_id) ) continue;
		double bias = avg_wt - db->clip_average_weight();
		if( fabs(bias) > 2*MEDIA_MEAN_ERRLMT ) continue;
		double iframerate = db->clip_framerate();
		if( iframerate <= 0. ) continue;
		int mframes = 2*check_margin * iframerate;
		int iframes = db->clip_frames();
		if( iframes < mframes ) continue;
		double ilength = iframes / iframerate;
		if( fabs(ilength-length) > 2*check_margin ) continue;
		double *iweights = db->clip_weights();
		double pos = position-check_margin;
		double kerr = MEDIA_WEIGHT_ERRLMT, kpos = -1;
		for( double dt=1/iframerate; --mframes>=0; pos+=dt ) {
			double err = video.err(pos, length, iframerate,
						iweights, iframes);
//printf(" clip %d, pos %f, err %f\n",clip_id,pos,err);
			if( err < kerr ) {
				if( kpos >= 0 && err > kerr ) break;
				kerr = err; kpos = pos;
			}
		}
//printf("vs clip %d, err %f, pos %f\n",clip_id,kerr,kpos);
		if( kpos >= 0 ) {
			printf(" dupl=%d, err=%f", clip_id, kerr);
			return -1;
		}
	}
	return 0;
}

int Scan::
save_group(Margin *margin, double pos, int frame_no, int len, int group)
{
	if( margin->locate(pos) < 0 ) fail();
	if( margin->length() < len ) fail();
	while( --len >= 0 ) {
		uint8_t *sbfr = margin->get_frame();
		double offset = video.frame_position(frame_no++);
		double wt = video.frame_weight(margin->position());
		if( wt < lo_video || wt > hi_video ) continue;
		db->new_frame(clip_id, sbfr, frame_no, group, offset);
//write_pbm(sbfr,SWIDTH,SHEIGHT,"/tmp/dat/c%03df%05d.pbm",clip_id,frame_no);
	}
	return 0;
}



int read_to(double time)
{
	int ret = 0;
	while( !(ret=video.eof()) && video.time() < time ) video.read_frame();
	while( !(ret=audio.eof()) && audio.time() < time ) audio.read_samples();
	return ret;
}


int Scan::
scan(Src &src, Deletions *dels)
{
	Dele *next = dels->first;
	if( !next || !next->next ) return 0;
	if( next->action != DEL_START ) fail();
	next->action = DEL_MARK;
	if( next->next->action == DEL_OOPS )
		next->next->action = DEL_SKIP;

	time_t ct;  time(&ct);
	creation_time = (int64_t)ct;
	prefix = suffix = 0;
	Dele *del = 0;

	while( next ) {
		switch( next->action ) {
		case DEL_MARK:
			if( !cut_clip(src, del, next) ) add_clip();
			del = next;
			break;
		case DEL_SKIP:
			del = 0;
			break;
		default:
			fail();
		}
		do {
			Dele *nxt = next->next;
			while( nxt && nxt->next && nxt->next->action == DEL_OOPS ) {
				while( (nxt=nxt->next) && nxt->action == DEL_OOPS );
			}
			next = nxt;
		} while( next && del && (next->time-del->time) < 3 );
		delete prefix;  prefix = suffix;  suffix = 0;
	}

	delete prefix;  prefix = 0;
	return 0;
}


int Scan::
cut_clip(Src &src, Dele *del, Dele *next)
{
	low_priority set_lo;
	double next_suffix_length = !del ? 0 : suffix_length;
	double next_prefix_length = !next->next ? 0 : prefix_length;
	double suffix_start_time =
		next->time - prefix_length - next_suffix_length - margin;
	if( read_to(suffix_start_time) ) fail();
	// create new suffix, copy prefix if overlap
	suffix = new Margin(video.frame_no(), video.time());
	if( prefix && prefix->locate(suffix_start_time) >= 0 )
		suffix->copy(prefix);
	// read_to next mark, get system time
	video.margin_frames(suffix);
	if( read_to(next->time) ) fail();
	system_time = src->dvb.get_system_time();
	// read_to end of next prefix
	double prefix_end_time = next->time + next_prefix_length + margin;
	if( read_to(prefix_end_time) ) fail();
	video.margin_frames();
	// if no previous mark, return
	if( !del ) return -1;
	asset_path = src.asset_path();
	double cut_start = del->time, cut_end = next->time;
	if( cut_end-cut_start < min_clip_time ) return 1;
	// default start/end clip endpoints
	start_time = cut_start - reaction_time;
	end_time = cut_end - reaction_time;
	// find edges, if possible.  default to reaction time
	audio.backward(cut_start-prefix_length, cut_start, start_time);
	video.forward(start_time-margin,start_time+margin, start_time);
	audio.forward(cut_end-suffix_length, cut_end, end_time);
	video.backward(end_time-margin,end_time+margin, end_time);
	// validate length
	frames = video.frame_count(end_time - start_time);
	if( frames < prefix_frames + suffix_frames ) fail();
	// report ranges
	clip_start = start_time - video.origin();
	clip_end = end_time - video.origin();
	printf(" %f-%f (%f), %f-%f %jd-%jd",
		start_time, end_time, end_time-start_time, clip_start, clip_end,
		video.frame_count(clip_start), video.frame_count(clip_end));
	return 0;
}

int Scan::
add_clip()
{
	high_priority set_hi;
	if( !db || !db->is_open() ) {
		fprintf(stderr,"unable to open db: " MEDIA_DB "\n");
		fail();
	}
	db->attachDb(1);
	// check for duplicate
	if( verify_clip(start_time, end_time-start_time) ) {
	}
	else if( !write_clip() ) {
		printf(" new %d",clip_id);
		db->commitDb();
	}
	else {
		printf(" failed");
		db->undoDb();
	}
	db->detachDb();
	printf("\n"); fflush(stdout);
	return 0;
}

int Scan::
write_clip()
{
	if( db->new_clip_set(video.title(), asset_path, clip_start,
		video.framerate(), frames, prefix_frames, suffix_frames,
		creation_time, system_time) ) fail();
	// save prefix video group
	int frame_no = 0;
	clip_id = db->clip_id();
	if( save_group(prefix, start_time, frame_no, prefix_frames, 1) ) fail();
	// save suffix video group
	double start_suffix = end_time - suffix_length;
	frame_no = frames - suffix_frames;
	if( save_group(suffix, start_suffix, frame_no, suffix_frames, 2) ) fail();
	// save video weights
	if( video.save_weights(start_time, db->clip_weights(), frames) ) fail();
	double avg_wt = video.average_weight(start_time, end_time-start_time);
	db->clip_average_weight(avg_wt);
	return 0;
}

void sig_segv(int n)
{
	printf("\nsegv, pid=%d\n",getpid()); fflush(stdout);
	sleep(1000000);
}

void sig_hup(int n)
{
}

int main(int ac, char **av)
{
printf("started %d\n",getpid());
	signal(SIGSEGV,sig_segv);
	signal(SIGHUP,sig_hup);

	if( ac < 2 ) {
		fprintf(stderr, "usage: %s <xml.del>\n", av[1]);
		fail();
	}
	// read in deletions
	Deletions *deletions = Deletions::read_dels(av[1]);
	if( !deletions ) {
		fprintf(stderr, "read_dels: %s failed\n", av[1]);
		fail();
	}

	// open mpeg3 dvb media
	Src src(deletions->file_path());
	if( !src.open() ) {
		fprintf(stderr, "open: %s failed\n", deletions->file_path());
		fail();
	}
	src->set_cpus(4);
	src->set_pts_padding(-1);

	// find video stream
	int vtrk = src->total_vstreams();
	while( --vtrk >= 0 && src->video_pid(vtrk) != deletions->pid );
	if( vtrk < 0 ) {
		fprintf(stderr, "pid %d failed\n", deletions->pid);
		fail();
	}
	video.init(src, vtrk);
	if( video.framerate() <= 0 ) {
		fprintf(stderr, "framerate %d failed\n", vtrk);
		fail();
	}

	// find first audio stream associate to video stream
	int n, atrk = -1;
	int elements = src->dvb.channel_count();
	for( n=0; atrk<0 && n<elements; ++n ) {
		int atracks, vtracks;
		if( src->dvb.total_vstreams(n,vtracks) ) continue;
		for( int i=0; atrk<0 && i<vtracks; ++i ) {
			if( src->dvb.vstream_number(n,i,vtrk) ) continue;
			if( vtrk != video.track() ) continue;
			if( src->dvb.total_astreams(n,atracks) ) continue;
			if( !atracks ) continue;
			if( src->dvb.astream_number(n,0,atrk,0) ) continue;
		}
	}
	int major, minor;  char title[64];
	if( !src->dvb.get_channel(n, major, minor) ) {
		sprintf(title, "%3d.%-3d", major, minor);
		video.set_title(title);
	}

	if( atrk < 0 ) {
		fprintf(stderr, "audio %d failed\n", video.track());
		fail();
	}
	audio.init(src, atrk);
	audio.set_buffer(audio.samplerate()/video.framerate()+64);

	// read to the first valid frame
	if( read_to(0) ) fail();
	double last_time = video.time(), origin = -1;
	while( !video.eof() ) {
		if( video.frame_position(video.frame_no()) >= 5 ) break;
		if( video.raw_weight() >= video_cutoff ) { origin = last_time; break; }
		last_time = video.time();
		if( video.read_frame() ) break;
	}
	if( origin < 0 ) {
		fprintf(stderr, "origin %d failed\n", video.track());
		fail();
	}
	video.set_origin(origin);

	double audio_time = audio.time(), video_time = video.time();
	double start_time = audio_time > video_time ? audio_time : video_time;
	if( read_to(start_time) ) fail();

	audio.enable_weights();
	video.enable_weights();

	Scan s;
	s.scan(src, deletions);

//audio.write_weights("/tmp/audio.wts");
//video.write_weights("/tmp/video.wts");
	audio.disable_weights();
	video.disable_weights();

//	::remove(deletions->path());
//	::remove(av[1]);

	delete deletions;
printf("completed %d\n",getpid());
//sleep(1000000);
	return 0;
}

