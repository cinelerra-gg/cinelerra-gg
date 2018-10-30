#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>

#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "libzmpeg3/libzmpeg3.h"
#include "thr.h"
#include "tdb.h"

/*
c++ -g -pthread -I../.. -I ../guicast xtv.C \
  ../../cinelerra/x86_64/mediadb.o ../../cinelerra/x86_64/filexml.o \
  ../../libzmpeg/x86_64/libzmpeg3.a ../../db/x86_64/db.a \
  -lX11 -lXext -lasound -lm
*/

using namespace std;
int building = getenv("BUILDING") ? 1 : 0;

zmpeg3_t *ysrc;

const double nudge = 0.0;
const double ahead = 0.0;
static int verbose = 0;

typedef int (*skim_fn)(void *vp, int track);


#define AUDIO
#define VIDEO
#define DB
//#define LAST8M


#ifdef AUDIO
snd_pcm_t *zpcm = 0;
snd_pcm_uframes_t zpcm_ufrm_size = 0;
snd_pcm_sframes_t zpcm_sbfr_size = 0;
snd_pcm_sframes_t zpcm_sper_size = 0;
snd_pcm_sframes_t zpcm_total_samples = 0;
double zpcm_play_time = 0.;
Mutex zpcm_lock;
unsigned int zpcm_rate = 0;
static const char *zpcm_device = "plughw:0,0";
static unsigned int zpcm_bfr_time_us = 500000; 
static unsigned int zpcm_per_time_us = 200000;
static snd_pcm_channel_area_t *zpcm_areas = 0;
static int zpcm_channels = 0;
static short **zpcm_buffers = 0;
static short *zpcm_samples = 0;
static short *zpcm_silence = 0;
static int alsa_mute = 0;

void alsa_close()
{
  if( zpcm_buffers != 0 ) { delete [] zpcm_buffers; zpcm_buffers = 0; }
  if( zpcm_areas != 0 ) { delete [] zpcm_areas; zpcm_areas = 0; }
  if( zpcm_silence != 0 ) { delete [] zpcm_silence; zpcm_silence = 0; }
  if( zpcm_samples != 0 ) { delete [] zpcm_samples; zpcm_samples = 0; }
  if( zpcm != 0 ) { snd_pcm_close(zpcm);  zpcm = 0; }
}

void alsa_open(int chs,int rate)
{
  int ich, bits, dir, ret;
  zpcm_lock.reset();
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16;
  snd_pcm_hw_params_t *phw;
  snd_pcm_sw_params_t *psw;
  snd_pcm_hw_params_alloca(&phw);
  snd_pcm_sw_params_alloca(&psw);

  zpcm = 0;
  zpcm_total_samples = 0;
  zpcm_play_time = 0.;
  ret = snd_pcm_open(&zpcm, zpcm_device,
     SND_PCM_STREAM_PLAYBACK, 0 /* + SND_PCM_NONBLOCK */);
  if( ret >= 0 )
    ret = snd_pcm_hw_params_any(zpcm, phw);
  if( ret >= 0 )
    ret = snd_pcm_hw_params_set_rate_resample(zpcm, phw, 1);
  if( ret >= 0 )
    ret = snd_pcm_hw_params_set_access(zpcm, phw,
      SND_PCM_ACCESS_RW_NONINTERLEAVED);
  if( ret >= 0 )
    ret = snd_pcm_hw_params_set_format(zpcm, phw, fmt);
  if( ret >= 0 ) {
    zpcm_channels = chs;
    ret = snd_pcm_hw_params_set_channels(zpcm, phw, chs);
  }
  if( ret >= 0 ) {
    zpcm_rate = rate;
    ret = snd_pcm_hw_params_set_rate_near(zpcm, phw, &zpcm_rate, 0);
    if( (int)zpcm_rate != rate )
      printf("nearest audio_rate for %d is %u\n",rate,zpcm_rate);
  }
  if( ret >= 0 )
    ret = snd_pcm_hw_params_set_buffer_time_near(zpcm, phw,
      &zpcm_bfr_time_us, &dir);
  if( ret >= 0 )
    ret = snd_pcm_hw_params_get_buffer_size(phw, &zpcm_ufrm_size);
  if( ret >= 0 ) {
    zpcm_sbfr_size = zpcm_ufrm_size;
    ret = snd_pcm_hw_params_set_period_time_near(zpcm, phw,
      &zpcm_per_time_us, &dir);
  }
  if( ret >= 0 )
    ret = snd_pcm_hw_params_get_period_size(phw, &zpcm_ufrm_size, &dir);
  if( ret >= 0 ) {
    zpcm_sper_size = zpcm_ufrm_size;
    ret = snd_pcm_hw_params(zpcm, phw);
  }
  if( ret >= 0 )
    ret = snd_pcm_sw_params_current(zpcm, psw);
  if( ret >= 0 )
    ret = snd_pcm_sw_params_set_start_threshold(zpcm, psw,
      (zpcm_sbfr_size / zpcm_sper_size) * zpcm_sper_size);
  if( ret >= 0 )
    ret = snd_pcm_sw_params_set_avail_min(zpcm, psw, zpcm_sper_size);
  if( ret >= 0 )
    ret = snd_pcm_sw_params(zpcm, psw);
  /* snd_pcm_dump(zpcm, stdout); */

  if( ret >= 0 ) {
     zpcm_areas = new snd_pcm_channel_area_t[chs];
     bits = snd_pcm_format_physical_width(fmt);
     zpcm_silence = new short[zpcm_sper_size];
     memset(zpcm_silence,0,zpcm_sper_size*sizeof(zpcm_silence[0]));
     zpcm_samples = new short[zpcm_sper_size * chs];
     zpcm_buffers = new short *[chs];
     if( zpcm_samples ) {
       for( ich = 0; ich < chs; ++ich ) {
         zpcm_areas[ich].addr = zpcm_samples;
         zpcm_areas[ich].first = ich * zpcm_sper_size * bits;
         zpcm_areas[ich].step = bits;
         zpcm_buffers[ich] = zpcm_samples + ich*zpcm_sper_size;
       }
     }
     else {
       fprintf(stderr,"alsa sample buffer allocation failure.\n");
       ret = -999;
     }
  }
  if( ret < 0 ) {
    if( ret > -999 )
      printf("audio error: %s\n", snd_strerror(ret));
    alsa_close();
  }
}

short *alsa_bfr(int ch) { return zpcm_buffers[ch]; }
int alsa_bfrsz() { return zpcm_sper_size; }

int alsa_recovery(int ret)
{
  printf("alsa recovery\n");
  switch( ret ) {
  case -ESTRPIPE:
    /* wait until the suspend flag is released, then fall through */
    while( (ret=snd_pcm_resume(zpcm)) == -EAGAIN ) usleep(100000);
  case -EPIPE:
    ret = snd_pcm_prepare(zpcm);
    if( ret < 0 )
      printf("underrun, prepare failed: %s\n", snd_strerror(ret));
    break;
  default:
    printf("unhandled error: %s\n",snd_strerror(ret));
    break;
  }
  return ret;
}

int alsa_write(int length)
{
  struct timeval tv;
  int i, ret, count, retry;
  snd_pcm_sframes_t sample_delay;
  double play_time, time;
  short *bfrs[zpcm_channels];
  retry = 3;
  ret = count = 0;
  for( i=0; i<zpcm_channels; ++i ) bfrs[i] = alsa_mute ? zpcm_silence : zpcm_buffers[i];

  while( count < length ) {
    ret = snd_pcm_writen(zpcm,(void **)bfrs, length-count);
    if( ret == -EAGAIN ) continue;
    if ( ret < 0 ) {
      if( --retry < 0 ) return ret;
      alsa_recovery(ret);
      ret = 0;
      continue;
    }
    for( i=0; i<zpcm_channels; ++i ) bfrs[i] += ret;
    count += ret;
  }
  zpcm_lock.lock();
  zpcm_total_samples += count;
  snd_pcm_delay(zpcm,&sample_delay);
  if( sample_delay > zpcm_total_samples )
    sample_delay = zpcm_total_samples;
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  play_time = (zpcm_total_samples - sample_delay) / (double)zpcm_rate;
  zpcm_play_time = time - play_time;
  zpcm_lock.unlock();
  return ret < 0 ? ret : 0;
}

double alsa_time()
{
  double time, play_time;
  struct timeval tv;
  zpcm_lock.lock();
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  play_time = time - zpcm_play_time;
  zpcm_lock.unlock();
  return play_time;
}

#else

double alsa_time()
{
  double time;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  return time;
}

#endif


double tstart;

double the_time()
{
  double time;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  if( tstart < 0. ) tstart = time;
  return time - tstart;
}


void mpeg3_stats(zmpeg3_t *zsrc)
{
  int astream, vstream;
  int has_audio, total_astreams, audio_channels, sample_rate;
  int has_video, total_vstreams, width, height, colormodel;
  long audio_samples, video_frames;
  float frame_rate;

  has_audio = mpeg3_has_audio(zsrc);
  printf(" has_audio = %d\n", has_audio);
  total_astreams = mpeg3_total_astreams(zsrc);
  printf(" total_astreams = %d\n", total_astreams);
  for( astream=0; astream<total_astreams; ++astream ) {
    audio_channels = zsrc->audio_channels(astream);
    printf("   audio_channels = %d\n", audio_channels);
    sample_rate = zsrc->sample_rate(astream);
    printf("   sample_rate = %d\n", sample_rate);
    audio_samples = zsrc->audio_samples(astream);
    printf("   audio_samples = %ld\n", audio_samples);
  }
  printf("\n");
  has_video = mpeg3_has_video(zsrc);
  printf(" has_video = %d\n", has_video);
  total_vstreams = mpeg3_total_vstreams(zsrc);
  printf(" total_vstreams = %d\n", total_vstreams);
  for( vstream=0; vstream<total_vstreams; ++vstream ) {
    width = zsrc->video_width(vstream);
    printf("   video_width = %d\n", width);
    height = zsrc->video_height(vstream);
    printf("   video_height = %d\n", height);
    frame_rate = zsrc->frame_rate(vstream);
    printf("   frame_rate = %f\n", frame_rate);
    video_frames = zsrc->video_frames(vstream);
    printf("   video_frames = %ld\n", video_frames);
    colormodel = zsrc->colormodel(vstream);
    printf("   colormodel = %d\n", colormodel);
  }
}

#ifdef DB

#include "guicast/linklist.h"
#include "guicast/arraylist.h"
#include "cinelerra/mediadb.h"

class SdbPacket;
class SdbPacketQueue;
class SdbSkimFrame;
class SkimDbThread;
class Media;


class SdbPacket : public ListItem<SdbPacket>
{
public:
	enum sdb_packet_type { sdb_none, sdb_skim_frame, } type;
	SkimDbThread *thread;
	void start();
	virtual void run() = 0;

	SdbPacket(sdb_packet_type ty, SkimDbThread *tp) : type(ty), thread(tp) {}
	~SdbPacket() {}
};

class SdbPacketQueue : public List<SdbPacket>, public Mutex
{
public:
	SdbPacket *get_packet();
	void put_packet(SdbPacket *p);
};

void SdbPacketQueue::
put_packet(SdbPacket *p)
{
        lock();
        append(p);
        unlock();
}

SdbPacket *SdbPacketQueue::
get_packet()
{
        lock();
        SdbPacket *p = first;
        remove_pointer(p);
        unlock();
        return p;
}


class SdbSkimFrame : public SdbPacket
{
public:
	int pid;
	int64_t framenum;
	double framerate;
	uint8_t dat[SWIDTH*SHEIGHT];

	void load(int pid, int64_t framenum, double position,
		uint8_t *idata,int mw,int mh,int iw,int ih);
	void run();

	SdbSkimFrame(SkimDbThread *t) : SdbPacket(sdb_skim_frame, t) {}
	~SdbSkimFrame() {}
};


class Media {
public:
  int skimming_active;

  int set_skimming(int track, int skim, skim_fn fn, void *vp);
  int get_video_info(int track, int &pid,
    double &framerate, int &width, int &height, char *title);
  static int skimming(void *vp, int track);
  int skimming(int track);
  void start_skimming();
  void stop_skimming();

  Media() { skimming_active = 0; }
  ~Media() {}
};

int Media::set_skimming(int track, int skim, skim_fn fn, void *vp)
{
	return !fn ? ysrc->set_thumbnail_callback(track, 0, 0, 0, 0) :
		ysrc->set_thumbnail_callback(track, skim, 1, fn, vp);
}

int Media::get_video_info(int track, int &pid,
    double &framerate, int &width, int &height, char *title)
{       
  pid = ysrc->video_pid(track);
  framerate = ysrc->frame_rate(track);
  width = ysrc->video_width(track);
  height = ysrc->video_height(track);
  if( !title ) return 0;
  *title = 0;
  
  int elements = ysrc->dvb.channel_count();
  for( int n=0; n<elements; ++n ) {
    int major, minor, total_vstreams, vstream, vidx;
    if( ysrc->dvb.get_channel(n, major, minor) ||
        ysrc->dvb.total_vstreams(n, total_vstreams) ) continue;
    for( vidx=0; vidx<total_vstreams; ++vidx ) {
      if( ysrc->dvb.vstream_number(n, vidx, vstream) ) continue;
      if( vstream < 0 ) continue;
      if( vstream == track ) {
        sprintf(title, "%3d.%-3d", major, minor);
        return 0;
      }
    }
  }
  return 0;
}

int Media::skimming(void *vp, int track)
{
  return ((Media*)vp)->skimming(track);
}


class SkimDbThread : public Thread, public Media, public MediaDb
{
	SdbPacketQueue active_packets;
	Condition *input_lock;
	friend class SdbSkimFrame;
public: 
	int done;
	int sfrm_sz;
	double framerate_ratio;

	SdbPacketQueue skim_frames;
	Snips *snips;

	void start();
	void stop();
	void run();
	void put_packet(SdbPacket *p);
	int skim(int pid,int64_t framenum,double framerate,
		uint8_t *idata,int mw,int mh,int iw,int ih);
	int skim_frame(Snips *snips, uint8_t *tdat, double position);
	double abs_err(Snip *snip, double *ap, int j, int len, double iframerate);
	double frame_weight(uint8_t *tdat, int rowsz, int width, int height);
	int verify_snip(Snip *snip, double weight, double position);

	SkimDbThread();
	~SkimDbThread();
} *the_db;

int Media::skimming(int track)
{
  int64_t framenum; uint8_t *tdat; int mw, mh;
  if( ysrc->get_thumbnail(track, framenum, tdat, mw, mh) ) return 1;
  int pid, width, height;  double framerate;
  if( get_video_info(track, pid, framerate, width, height, 0) ) return 1;
  if( !framerate ) return 1;
//printf("Media::skimming framenum %ld, framerate %f\n",framenum,framerate);
  return the_db->skim(pid,framenum,framerate, tdat,mw,mh,width,height);
}

void Media::start_skimming()
{
  if( !skimming_active ) {
    skimming_active = 1;
    the_db->start();
    set_skimming(0, 0, skimming, this);
  }
}

void Media::stop_skimming()
{
  if( skimming_active ) {
    skimming_active = 0;
    set_skimming(0, 0, 0, 0);
    the_db->stop();
  }
}


SkimDbThread::
SkimDbThread()
 : Thread()
{
	input_lock = new Condition(0); // "SkimDbThread::input_lock");
	sfrm_sz = SWIDTH*SHEIGHT;
	for( int i=32; --i>=0; ) skim_frames.append(new SdbSkimFrame(this));
	snips = new Snips();
	done = 1;
}

SkimDbThread::
~SkimDbThread()
{
	stop();
	delete snips;
	delete input_lock;
}



void SkimDbThread::
start()
{
	if( openDb() ) return;
	if( detachDb() ) return;
	done = 0;
	Thread::start();
}

void SkimDbThread::
stop()
{
	if( running() ) {
		done = 1;
		input_lock->unlock();
		cancel();
		join();
		closeDb();
	}
}

int SkimDbThread::
skim(int pid,int64_t framenum,double framerate,
	uint8_t *idata,int mw,int mh,int iw,int ih)
{
	SdbSkimFrame *sf = (SdbSkimFrame *)skim_frames.get_packet();
if( !sf ) printf("SkimDbThread::skim no packet\n");
	if( !sf ) return 1;
	sf->load(pid,framenum,framerate, idata,mw,mh,iw,ih);
	sf->start();
	return 0;
}


void SdbPacket::start()
{
	thread->put_packet(this);
}

void SkimDbThread::
put_packet(SdbPacket *p)
{
	active_packets.put_packet(p);
	input_lock->unlock();
}

void SkimDbThread::
run()
{
	while( !done ) {
		input_lock->lock(); //"SkimDbThread::run");
		if( done ) break;
		SdbPacket *p = active_packets.get_packet();
		if( !p ) continue;
		attachDb();
		p->run();
		detachDb();
	}
}


void SdbSkimFrame::
load(int pid,int64_t framenum, double framerate, uint8_t *idata,int mw,int mh,int iw,int ih)
{
	int sw=SWIDTH, sh=SHEIGHT;
	this->pid = pid;
	this->framenum = framenum;
	this->framerate = framerate;
//write_pbm(idata, mw, mh, "/tmp/data/f%05d.pbm",framenum);
	Scale(idata,0,mw,mh,0,0,iw/8,ih/8).scale(dat,sw,sh,0,0,sw,sh);
//write_pbm(dat, SWIDTH, SHEIGHT, "/tmp/data/s%05d.pbm",framenum);
}

void SdbSkimFrame::
run()
{
//write_pbm(dat, SWIDTH, SHEIGHT, "/tmp/data/h%05d.pbm",framenum);
	double position = framenum / framerate;
	thread->skim_frame(thread->snips, dat, position);
	thread->skim_frames.put_packet(this);
}


double SkimDbThread::
frame_weight(uint8_t *tdat, int rowsz, int width, int height)
{
        int64_t weight = 0;
        for( int y=height; --y>=0; tdat+=rowsz ) {
                uint8_t *bp = tdat;
                for( int x=width; --x>=0; ++bp ) weight += *bp;
        }
        return (double)weight / (width*height);
}

int SkimDbThread::
skim_frame(Snips *snips, uint8_t *tdat, double position)
{
	int fid, result = get_frame_key(tdat, fid, snips, position, 1);
	double weight = frame_weight(tdat, SWIDTH, SWIDTH, SHEIGHT);
	for( Clip *next=0,*clip=snips->first; clip; clip=next ) {
		next = clip->next;
		result = verify_snip((Snip*)clip,weight,position);
		if( !result ) {
			if( clip->votes >= 2 || position-clip->start > 2 ) {
				if( !(clip->groups & 4) ) {
					clip->groups |= 4;
					if( !alsa_mute ) {
						alsa_mute = 1;
						printf("*** MUTE AUDIO clip %d\n", clip->clip_id);
					}
				}
			}
		}
		else if( result < 0 || --clip->votes < 0 ) {
printf("delete clip %d, snips %d\n", clip->clip_id, snips->count-1);
			delete clip;  --snips->count;
			if( !snips->first && alsa_mute ) {
				alsa_mute = 0;
				printf("*** UNMUTE AUDIO clip %d\n", clip->clip_id);
			}
		}
	}
if( snips->count > 0 ) { printf("snips %d = ", snips->count);
  for( Clip *cp=snips->first; cp; cp=cp->next ) printf(" %d/%d",cp->clip_id,cp->votes);
printf("\n"); }
	return 0;
}

double SkimDbThread::
abs_err(Snip *snip, double *ap, int j, int len, double iframerate)
{
	double vv = 0;
	int i, k, sz = snip->weights.size(), n = 0;
	for( i=0; i<sz; ++i ) {
		k = j + (snip->positions[i] - snip->start) * iframerate + 0.5;
		if( k < 0 ) continue;
		if( k >= len ) break;
		double a = ap[k], b =  snip->weights[i];
		double dv = fabs(a - b);
		vv += dv;
		++n;
	}
	return !n ? MEDIA_WEIGHT_ERRLMT : vv / n;
}

int SkimDbThread::
verify_snip(Snip *snip, double weight, double position)
{
	int cid = snip->clip_id;
	if( clip_id(cid) ) return -1;
	double iframerate = clip_framerate();
	int iframes = clip_frames();
	double pos = position - snip->start;
        int iframe_no = pos * iframerate + 0.5;
        if( iframe_no >= iframes ) return -1;
        snip->weights.append(weight);
        snip->positions.append(position);
printf("%f %d ",weight,iframe_no);
        double *iweights = clip_weights();
	double errlmt = MEDIA_WEIGHT_ERRLMT;
        double err = errlmt, kerr = err;
        int k = 0;
        int tmargin = TRANSITION_MARGIN * iframerate + 0.5;
        for( int j=-tmargin; j<=tmargin; ++j ) {
                err = abs_err(snip, iweights, j, iframes, iframerate);
//printf("   err %f, j=%d\n", err, j);
                if( err < kerr ) { kerr = err;  k = j; }
        }
printf("kerr %f, k=%d ", kerr, k);
        if( kerr >= errlmt ) return 1;
        if( iframe_no + k >= iframes ) return -1;
        return 0;
}


#endif

#ifdef VIDEO

class Video : public Thread {
  zmpeg3_t *zsrc;
  double vstart;
  double last_vtime;
  double video_time(int stream);

public:
  void run();

  Video(zmpeg3_t *z) : zsrc(z) {}
  ~Video() {}
} *the_video;


double Video::video_time(int stream)
{
  double vtime = zsrc->get_video_time(stream);
  double vnow = vtime;
  if( vstart == 0. ) vstart = vnow;
  if( vtime < last_vtime )
    vstart -= last_vtime;
  last_vtime = vtime;
  return vnow - vstart + nudge;
}

void Video::run()
{
  Window w, root;
  Display *display;
  GC gc;
  XGCValues gcv;
  XEvent xev;
  XImage *image0, *image1, *image;
  Visual *visual;
  Screen *screen;
  XShmSegmentInfo info0, info1;
  int done;
  double delay;
  float frame_rate, frame_delay;
  uint8_t **rows, **row0, **row1, *cap, *cap0, *cap1;
  int ret, row, frame, dropped, more_data;
  int width, height, depth, owidth, oheight;
  const int frame_drop = 0;
  const int shared_memory = 1;

  display = XOpenDisplay(NULL);
  if( display == NULL ) {
    fprintf(stderr,"cant open display\n");
    exit(1);
  }

  root = RootWindow(display,0);
  screen = DefaultScreenOfDisplay(display);
  depth = DefaultDepthOfScreen(screen);
  visual = DefaultVisualOfScreen(screen);
  if( visual->c_class != TrueColor ) {
    printf("visual class not truecolor\n");
    exit(1);
  }
  int image_bpp = 4;
 
  frame_rate = zsrc->frame_rate(0);
  frame_delay = 2.0 / frame_rate;
  height = zsrc->video_height(0);
  width = zsrc->video_width(0);

  //oheight = 2*height / 3;
  //owidth = 2*width / 3;
  //oheight = 1050-96;
  //owidth = 1680-64;
  //oheight = height;
  //owidth = width;
  oheight = 720;
  owidth = 1280;

  w = XCreateSimpleWindow(display, root, 0, 0, owidth, oheight,
    0, 0, WhitePixelOfScreen(screen));
  XSelectInput(display, w, ExposureMask|StructureNotifyMask|ButtonPressMask);

  if( !shared_memory ) {
    int sz0 = oheight*owidth*image_bpp + 4;
    cap0 = new uint8_t[sz0];
    image0 = XCreateImage(display, visual, depth, ZPixmap, 0,
         (char*)cap0, owidth, oheight, 8, owidth*4);
    int sz1 = oheight*owidth*image_bpp + 4;
    cap1 = new uint8_t[sz1];
    image1 = XCreateImage(display, visual, depth, ZPixmap, 0,
         (char*)cap1, owidth, oheight, 8, owidth*4);
  }
  else {
    image0 = XShmCreateImage(display, visual, depth, ZPixmap, 0,
         &info0, owidth, oheight);
    int sz0 = oheight*image0->bytes_per_line+image_bpp;
    info0.shmid = shmget(IPC_PRIVATE, sz0, IPC_CREAT | 0777);
    if( info0.shmid < 0) { perror("shmget"); exit(1); }
    cap0 = (uint8_t *)shmat(info0.shmid, NULL, 0);
    shmctl(info0.shmid, IPC_RMID, 0);
    image0->data = info0.shmaddr = (char *)cap0;
    info0.readOnly = 0;
    XShmAttach(display,&info0);
    image1 = XShmCreateImage(display, visual, depth, ZPixmap, 0,
         &info1, owidth, oheight);
    int sz1 = oheight*image1->bytes_per_line+image_bpp;
    info1.shmid = shmget(IPC_PRIVATE, sz1, IPC_CREAT | 0777);
    if( info1.shmid < 0) { perror("shmget"); exit(1); }
    cap1 = (uint8_t *)shmat(info1.shmid, NULL, 0);
    shmctl(info1.shmid, IPC_RMID, 0);
    image1->data = info1.shmaddr = (char *)cap1;
    info1.readOnly = 0;
    XShmAttach(display,&info1);
  }

  row0 = new uint8_t *[oheight];
  row1 = new uint8_t *[oheight];
  for( row=0; row<oheight; ++row ) {
    row0[row] = cap0 + row*owidth*image_bpp;
    row1[row] = cap1 + row*owidth*image_bpp;
  }

  cap = cap0;
  rows = row0;
  image = image0;
  if( image->bits_per_pixel != 32 ) {
    printf("image bits_per_pixel=%d\n",image->bits_per_pixel);
    exit(1);
  }
  int cmdl = -1;
  unsigned int rmsk = image->red_mask;
  unsigned int gmsk = image->green_mask;
  unsigned int bmsk = image->blue_mask;
  switch( image_bpp ) {
  case 4:
    if( bmsk==0xff0000 && gmsk==0x00ff00 && rmsk==0x0000ff )
      cmdl = zmpeg3_t::cmdl_RGBA8888;
    else if( bmsk==0x0000ff && gmsk==0x00ff00 && rmsk==0xff0000 )
      cmdl = zmpeg3_t::cmdl_BGRA8888;
    break;
  case 3:
    if( bmsk==0xff0000 && gmsk==0x00ff00 && rmsk==0x0000ff )
      cmdl = zmpeg3_t::cmdl_RGB888;
    else if( bmsk==0x0000ff && gmsk==0x00ff00 && rmsk==0xff0000 )
      cmdl = zmpeg3_t::cmdl_BGR888;
    break;
  case 2:
    if( bmsk==0x00f800 && gmsk==0x0007e0 && rmsk==0x00001f )
      cmdl = zmpeg3_t::cmdl_RGB565;
    break;
  }
  if( cmdl < 0 ) {
    printf("unknown color model, bpp=%d, ",image_bpp);
    printf(" rmsk=%08x,gmsk=%08x,bmsk=%08x\n", rmsk, gmsk, bmsk);
    exit(1);
  }

  XMapWindow(display, w);
  XFlush(display);
  gcv.function = GXcopy;
  gcv.foreground = BlackPixelOfScreen(DefaultScreenOfDisplay(display));
  gcv.line_width = 1;
  gcv.line_style = LineSolid;
  int m = GCFunction|GCForeground|GCLineWidth|GCLineStyle;
  gc = XCreateGC(display,w,m,&gcv);

  frame = dropped = 0;
  vstart = last_vtime = 0.;
  more_data = 1;
  done = 0;

  while( !done && running() && more_data ) {
    if( !XPending(display) ) {
      more_data = 0;
      delay = (frame+dropped)/frame_rate - alsa_time();
//      delay = the_time() - video_time(zsrc,0);
      if( frame_drop ) {
        if( -delay >= frame_delay ) {
          int nframes = (long)ceil(-delay/frame_rate);
          if( nframes > 2 ) nframes = 2;
          zsrc->drop_frames(nframes,0);
          dropped += nframes;
        }
      }
      //printf("delay %f\n",delay*1000.);
      if( delay > 0 )
        usleep((int)(delay*1000000.0));
      ret = zsrc->read_frame(rows, 0, 0, width, height,
         owidth, oheight, cmdl, 0);
      //printf("%d %ld\n",frame,zsrc->vtrack[0]->demuxer->titles[0]->fs->current_byte);
      if( verbose )
        printf("read_video(stream=%d, frame=%d) = %d, %f sec\n", 0, frame, ret, the_time());
      ++frame;

      if( !shared_memory )
        XPutImage(display, w, gc, image, 0, 0, 0, 0, owidth, oheight);
      else
        XShmPutImage(display, w, gc, image, 0, 0, 0, 0, owidth, oheight, False);

      cap = cap==cap0 ?
  (rows=row1, image=image1, cap1) :
  (rows=row0, image=image0, cap0) ;
      //printf(" %d/%d  %f  %f  %f\n",frame,dropped,(frame+dropped)/frame_rate,
      //  video_time(zsrc,0),zsrc->get_video_time(0));
      more_data |= !zsrc->end_of_video(0);
    }
    else {
      XNextEvent(display,&xev);
      /* printf("xev.type = %d/%08x\n",xev.type,xev.xany.window); */
      switch( xev.type ) {
      case ButtonPress:
        if( xev.xbutton.window != w ) continue;
        if( xev.xbutton.button != Button1 )
          done = 1;
        break;
      case Expose:
        break;
      case DestroyNotify:
        done = 1;
        break;
      }
    }
  }

  XCloseDisplay(display);
  delete [] row0; row0 = 0;
  delete [] row1; row1 = 0;
}

#endif

#ifdef AUDIO

class Audio : public Thread {
  zmpeg3_t *zsrc;
  double astart;
  double audio_time(int stream);

public:
  void run();

  Audio(zmpeg3_t *z) : zsrc(z) {}
  ~Audio() {}
} *the_audio;


double Audio::audio_time(int stream)
{
  double anow = zsrc->get_audio_time(stream);
  if( astart < 0. ) astart = anow;
  return anow - astart;
}

void Audio::run()
{
  double delay;
  int audio_channels, sample_rate, more_data;
  int stream = 0;

  audio_channels = zsrc->audio_channels(stream);
  sample_rate = zsrc->sample_rate(stream);
  alsa_open(audio_channels,sample_rate);
  astart = -1.;
  more_data = 1;

  while( running() && more_data ) {
    more_data = 0;
    int ich;
    int n = alsa_bfrsz();
    for( ich=0; ich<audio_channels; ++ich) {
      short *bfr = alsa_bfr(ich);
      int ret = ich == 0 ?
        zsrc->read_audio(bfr, ich, n, stream) :
        zsrc->reread_audio(bfr, ich, n, stream);
      if( verbose )
        printf("read_audio(stream=%d,channel=%d) = %d\n", stream, ich, ret);
    }

    alsa_write(n);
    delay = audio_time(stream) - the_time();
    if( (delay-=ahead) > 0. ) {
      usleep((int)(delay*1000000.0/2.));
    }
    more_data |= !zsrc->end_of_audio(0);
  }

  alsa_close();
}

#endif

void sigint(int n)
{
#ifdef VIDEO
  the_video->cancel();
#endif
#ifdef AUDIO
  the_audio->cancel();
#endif
#ifdef DB
  the_db->stop_skimming();
#endif
}


int main(int ac, char **av)
{
  int ret;
  long pos;
#ifdef LAST8M
  struct stat st_buf;
#endif
  setbuf(stdout,NULL);
  tstart = -1.;

#ifdef VIDEO
  XInitThreads();
#endif

  zmpeg3_t* zsrc = new zmpeg3_t(av[1],ret);
  ysrc = zsrc;
  printf(" ret = %d\n",ret);
  if( ret != 0 ) exit(1);
  mpeg3_stats(zsrc);

  zsrc->set_cpus(8);
  pos = 0;
#ifdef LAST8M
  if( stat(zsrc->fs->path, &st_buf) >= 0 ) {
    if( (pos = (st_buf.st_size & ~0x7ffl) - 0x800000l) < 0 )
      pos = 0;
  }
#endif
  zsrc->seek_byte(pos);
//  zsrc->show_subtitle(0);
  signal(SIGINT,sigint);

#ifdef DB
  the_db = new SkimDbThread();
#endif
#ifdef AUDIO
  the_audio = new Audio(zsrc);
#endif
#ifdef VIDEO
  the_video = new Video(zsrc);
#endif
#ifdef DB
  the_db->start_skimming();
#endif
#ifdef AUDIO
  the_audio->start();
#endif
#ifdef VIDEO
  the_video->start();
#endif
#ifdef VIDEO
  the_video->join();
#endif
#ifdef AUDIO
  the_audio->join();
#endif
#ifdef DB
  the_db->stop_skimming();
#endif

  delete zsrc;
  return 0;
}

