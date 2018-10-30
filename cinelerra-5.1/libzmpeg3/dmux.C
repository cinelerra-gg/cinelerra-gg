#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <alsa/asoundlib.h>
#include <linux/dvb/dmx.h>
#include <linux/dvb/frontend.h>

/* c++ `pkg-config --cflags --libs gtk+-2.0` dmux.C thread.C ./x86_64/libzmpeg3.a -lpthread -lasound -lm */

#include "libzmpeg3.h"
#include "thread.h"

double the_time();
void reset();
int open_tuner(int dev_no,int chan,int vid_pid,int aud_pid);
void close_tuner();
double audio_time(int stream);
void alsa_close();
void alsa_open(int chs,int rate);
short *alsa_bfr(int ch);
int alsa_bfrsz();
int alsa_recovery(int ret);
int alsa_write(int len);
void dst_exit(GtkWidget *widget, gpointer data);
gint key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data);
double video_time(int stream);
void stats();
void sig_int(int n);
int main(int ac,char **av);

/* alpha currently is broken in gdk */
/*   would render faster if alpha worked correctly */
#define RGBA FALSE

#if RGBA
#define BPP 4
#define COLOR_MODEL zmpeg3_t::cmdl_RGBA8888
#else
#define BPP 3
#define COLOR_MODEL zmpeg3_t::cmdl_RGB888
#endif

int verbose = 0;
double nudge = .5; // 3.5;
const double ahead = .5;

//#define DEBUG
#ifndef DEBUG
#define dprintf(s...) do{}while(0)
#else
#define dprintf(s...) printf(s)
#endif

#define NETTUNE_AIR 1
#define NETTUNE_CABLE 2

static unsigned long ntsc_dvb[ ] = {
  0,  0,  57,  63,  69,  79,  85, 177, 183, 189 ,
  195, 201, 207, 213, 473, 479, 485, 491, 497, 503 ,
  509, 515, 521, 527, 533, 539, 545, 551, 557, 563 ,
  569, 575, 581, 587, 593, 599, 605, 611, 617, 623 ,
  629, 635, 641, 647, 653, 659, 665, 671, 677, 683 ,
  689, 695, 701, 707, 713, 719, 725, 731, 737, 743 ,
  749, 755, 761, 767, 773, 779, 785, 791, 797, 803 ,
  809, 815, 821, 827, 833, 839, 845, 851, 857, 863 ,
  869, 875, 881, 887, 893, 899, 905, 911, 917, 923 ,
  929, 935, 941, 947, 953, 959, 965, 971, 977, 983 ,
  989, 995, 1001, 1007, 1013, 1019, 1025, 1031, 1037, 1043
};

static unsigned long catv_dvb[] = {
  0, 0, 57, 63, 69, 79, 85, 177, 183, 189,
  195, 201, 207, 213, 123, 129, 135, 141, 147, 153,
  159, 165, 171, 219, 225, 231, 237, 243, 249, 255,
  261, 267, 273, 279, 285, 291, 297, 303, 309, 315,
  321, 327, 333, 339, 345, 351, 357, 363, 369, 375,
  381, 387, 393, 399, 405, 411, 417, 423, 429, 435,
  441, 447, 453, 459, 465, 471, 477, 483, 489, 495,
  501, 507, 513, 519, 525, 531, 537, 543, 549, 555,
  561, 567, 573, 579, 585, 591, 597, 603, 609, 615,
  621, 627, 633, 639, 645,  93,  99, 105, 111, 117,
  651, 657, 663, 669, 675, 681, 687, 693, 699, 705,
  711, 717, 723, 729, 735, 741, 747, 753, 759, 765,
  771, 777, 783, 789, 795, 781, 807, 813, 819, 825,
  831, 837, 843, 849, 855, 861, 867, 873, 879, 885,
  891, 897, 903, 909, 915, 921, 927, 933, 939, 945,
  951, 957, 963, 969, 975, 981, 987, 993, 999
};

int frontend_fd;
int audio_fd;
int video_fd;
int dvr_fd;
int prev_lock;
int has_lock;
int done;
zmpeg3_t *zsrc;
int aud, vid;
int subchan;
int audio_done;
int video_done;

class vid_thread : public Thread
{
  void Proc();
} *the_vid;

class aud_thread : public Thread
{
  void Proc();
} *the_aud;

class lck_thread : public Thread
{
  void Proc();
} *the_lck;

int do_status()
{
  fe_status_t status;
  uint16_t snr, signal;
  uint32_t ber, uncorrected_blocks;

  bzero(&status, sizeof(status));
  ioctl(frontend_fd, FE_READ_STATUS, &status);
  if( verbose ) {
    ioctl(frontend_fd, FE_READ_SIGNAL_STRENGTH, &signal);
    ioctl(frontend_fd, FE_READ_SNR, &snr);
    ioctl(frontend_fd, FE_READ_BER, &ber);
    ioctl(frontend_fd, FE_READ_UNCORRECTED_BLOCKS, &uncorrected_blocks);
    printf("lck:: %02x | signal %04x | snr %04x | ber %08x | unc %08x | ",
           status, signal, snr, ber, uncorrected_blocks);
  }
  has_lock =  status & FE_HAS_LOCK ? 1 : 0;
  if( verbose )
    printf( has_lock ? "lock\n" : "lost\n" );
  if( prev_lock != has_lock ) {
    printf(" %s\n",has_lock ? "signal locked" : "signal lost");
    prev_lock = has_lock;
  }
  return has_lock;
}

void lck_thread::
Proc()
{
  dprintf("lck_thread::Proc()\n");
  while( Running() ) {
    do_status();
    sleep(1);
  }
}

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

void reset()
{
  frontend_fd = -1;
  audio_fd = -1;
  video_fd = -1;
  dvr_fd = -1;
  the_vid = 0;
  the_lck = 0;
  prev_lock = 0;
  has_lock = 0;
  tstart = -1.;
  aud = 0;
  vid = 0;
  subchan = -1;
}

int open_tuner(int dev_no,int chan,int vid_pid,int aud_pid)
{
  char frontend_path[512];
  char demux_path[512];
  char dvr_path[512];
  //dvr_fd = open("/tmp/dat7",O_RDONLY);
  //return dvr_fd >= 0 ? 0 : 1;

  sprintf(frontend_path, "/dev/dvb/adapter%d/frontend%d", dev_no, 0);
  sprintf(demux_path, "/dev/dvb/adapter%d/demux%d", dev_no, 0);
  sprintf(dvr_path, "/dev/dvb/adapter%d/dvr%d", dev_no, 0);
  if( (frontend_fd=::open(frontend_path, O_RDWR)) < 0 ) {
    fprintf(stderr, "open_tuner %s: %s\n", frontend_path, strerror(errno));
    return 1;
  }

   struct dvb_frontend_parameters frontend_param;
   bzero(&frontend_param, sizeof(frontend_param));

// Set frequency
   int index = chan;
   int table = NETTUNE_AIR;

   switch(table) {
   case NETTUNE_AIR:
     frontend_param.frequency = ntsc_dvb[index] * 1000000;
     frontend_param.u.vsb.modulation = VSB_8;
     break;
   case NETTUNE_CABLE:
     frontend_param.frequency = catv_dvb[index] * 1000000;
     frontend_param.u.vsb.modulation = QAM_AUTO;
     break;
   }

   if( ioctl(frontend_fd, FE_SET_FRONTEND, &frontend_param) < 0 ) {
     fprintf(stderr, "open_tuner FE_SET_FRONTEND frequency=%d: %s",
       frontend_param.frequency, strerror(errno));
     return 1;
   }

   int retry = 5;
   while( !do_status() && --retry>=0 ) sleep(1);

   if( retry < 0 ) {
     fprintf(stderr, "open_tuner: no signal on channel %d\n",chan);
     return 1;
   }

   if( (video_fd=::open(demux_path, O_RDWR)) < 0 ) {
     fprintf(stderr, "open_tuner %s for video: %s\n",
       demux_path, strerror(errno));
     return 1;
   }

// Setting exactly one PES filter to 0x2000 dumps the entire
// transport stream.
   struct dmx_pes_filter_params pesfilter;
   if( !vid_pid && !aud_pid ) {
     pesfilter.pid = 0x2000;
     pesfilter.input = DMX_IN_FRONTEND;
     pesfilter.output = DMX_OUT_TS_TAP;
     pesfilter.pes_type = DMX_PES_OTHER;
     pesfilter.flags = DMX_IMMEDIATE_START;
     if( ioctl(video_fd, DMX_SET_PES_FILTER, &pesfilter) < 0 ) {
       fprintf(stderr, "open_tuner DMX_SET_PES_FILTER for raw: %s\n",
         strerror(errno));
       return 1;
     }
   }


   if( vid_pid ) {
     pesfilter.pid = vid_pid;
     pesfilter.input = DMX_IN_FRONTEND;
     pesfilter.output = DMX_OUT_TS_TAP;
     pesfilter.pes_type = DMX_PES_VIDEO;
     pesfilter.flags = DMX_IMMEDIATE_START;
     if( ioctl(video_fd, DMX_SET_PES_FILTER, &pesfilter) < 0 ) {
       fprintf(stderr, "open_tuner DMX_SET_PES_FILTER for video: %s\n",
         strerror(errno));
       return 1;
     }
   }

   if( aud_pid ) {
     if( (audio_fd=::open(demux_path,O_RDWR)) < 0 ) {
       fprintf(stderr, "open_tuner %s for audio: %s\n",
         demux_path, strerror(errno));
       return 1;
     }
     pesfilter.pid = aud_pid;
     pesfilter.input = DMX_IN_FRONTEND;
     pesfilter.output = DMX_OUT_TS_TAP;
     pesfilter.pes_type = DMX_PES_AUDIO;
     pesfilter.flags = DMX_IMMEDIATE_START;
     if( ioctl(audio_fd, DMX_SET_PES_FILTER, &pesfilter) < 0 ) {
       fprintf(stderr, "open_tuner DMX_SET_PES_FILTER for audio: %s\n",
         strerror(errno));
       return 1;
     }
   }

// Open transport stream for reading
   if( (dvr_fd=::open(dvr_path, O_RDONLY)) < 0 ) {
     fprintf(stderr, "open_tuner %s: %s\n", dvr_path, strerror(errno));
     return 1;
   }

   return 0;
}

void close_tuner()
{
  if( frontend_fd >= 0 ) close(frontend_fd);
  if( audio_fd >= 0 )    close(audio_fd);
  if( video_fd >= 0 )    close(video_fd);
  if( dvr_fd >= 0 )      close(dvr_fd);
}

int open_stream()
{
  if( zsrc == 0 && dvr_fd >= 0 ) {
    int ret;
    zsrc = new zmpeg3_t(dvr_fd, ret, ZIO_THREADED);
    //zsrc = new zmpeg3_t(dvr_fd, ret);
    zsrc->set_cpus(2);
    if( ret ) {
      fprintf(stderr,"mpeg3 open failed (%d)\n", ret);
      return 1;
    }
    if( subchan >= 0 ) {
      int nch = zsrc->dvb.channel_count();
      while( --nch >= 0 ) {
        int mjr, mnr;
        if( zsrc->dvb.get_channel(nch, mjr, mnr) ) continue;
        if( mnr == subchan ) break;
      }
      if( nch >= 0 ) {
        zsrc->dvb.vstream_number(nch, 0, vid);
        zsrc->dvb.astream_number(nch, 0, aud, 0);
      }
    }
  }
  return 0;
}

void close_stream()
{
  if( zsrc ) { delete zsrc;  zsrc = 0; }
}

int start_stream()
{
  if( !the_lck && frontend_fd >= 0 ) {
    the_lck = new lck_thread();
    the_lck->Run();
  }
  if( !the_vid && dvr_fd >= 0 ) {
    the_vid = new vid_thread();
    the_vid->Run();
  }
  if( !the_aud && dvr_fd >= 0 ) {
    the_aud = new aud_thread();
    the_aud->Run();
  }
  return 0;
}

void stop_stream()
{
  if( the_lck ) { the_lck->Kill();  the_lck = 0; }
  if( the_vid ) { the_vid->Kill();  the_vid = 0; }
  if( the_aud ) { the_aud->Kill();  the_aud = 0; }
}

double astart;

double audio_time(int stream)
{
  double anow = zsrc->get_audio_time(stream);
  if( astart < 0. ) astart = anow;
  return anow - astart;
}

void aud_thread::
Proc()
{
  int audio_channels, sample_rate, more_data;

  audio_channels = zsrc->audio_channels(aud);
  sample_rate = zsrc->sample_rate(aud);
  alsa_open(audio_channels,sample_rate);
  astart = -1.;
  more_data = 1;

  while( done == 0 && more_data ) {
    more_data = 0;
    int ich;
    int n = alsa_bfrsz();
    for( ich=0; ich<audio_channels; ++ich ) {
      short *bfr = alsa_bfr(ich);
      int ret = ich == 0 ?
        zsrc->read_audio(bfr, ich, n, aud) :
        zsrc->reread_audio(bfr, ich, n, aud);
      if( verbose )
        printf("read_audio(stream=%d,channel=%d) = %d\n", aud, ich, ret);
    }
    alsa_write(n);
    //double atime = audio_time(aud);
    //double ttime = the_time();
    //doule delay = atime - ttime;
    //printf("delay = %f  (%f-%f)\n",delay,atime,ttime);
    //if( (delay-ahead) > 0. )
    //  usleep((int)(delay*1000000.0/2.));
    more_data |= !zsrc->end_of_audio(aud);
  }

  alsa_close();
  audio_done = 1;
}

snd_pcm_t *zpcm;
snd_pcm_uframes_t zpcm_ufrm_size;
snd_pcm_sframes_t zpcm_sbfr_size;
snd_pcm_sframes_t zpcm_sper_size;
unsigned int zpcm_rate;
static const char *zpcm_device = "plughw:0,0";
static unsigned int zpcm_bfr_time_us = 500000; 
static unsigned int zpcm_per_time_us = 200000;
static snd_pcm_channel_area_t *zpcm_areas = 0;
static int zpcm_channels = 0;
static short **zpcm_buffers = 0;
static short *zpcm_samples = 0;

void alsa_close()
{
  if( zpcm_buffers != 0 ) { delete [] zpcm_buffers; zpcm_buffers = 0; }
  if( zpcm_areas != 0 ) { delete [] zpcm_areas; zpcm_areas = 0; }
  if( zpcm_samples != 0 ) { delete [] zpcm_samples; zpcm_samples = 0; }
  if( zpcm != 0 ) { snd_pcm_close(zpcm);  zpcm = 0; }
}

void alsa_open(int chs,int rate)
{
  int ich, bits, byts, dir, ret;
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16;
  snd_pcm_hw_params_t *phw;
  snd_pcm_sw_params_t *psw;
  snd_pcm_hw_params_alloca(&phw);
  snd_pcm_sw_params_alloca(&psw);

  zpcm = 0;
  ret = snd_pcm_open(&zpcm, zpcm_device,
     SND_PCM_STREAM_PLAYBACK, 0 /* SND_PCM_NONBLOCK */);
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
     byts = snd_pcm_format_physical_width(fmt) / 8;
     zpcm_samples = new short[zpcm_sper_size * chs * byts/2];
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

short *alsa_bfr(int ch)
{
  return zpcm_buffers[ch];
}

int alsa_bfrsz()
{
  return zpcm_sper_size;
}

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

int alsa_write(int len)
{
  int i, ret;
  short *bfrs[zpcm_channels];
  ret = 0;
  for( i=0; i<zpcm_channels; ++i ) bfrs[i] = zpcm_buffers[i];

  while( len > 0 ) {
    ret = snd_pcm_writen(zpcm,(void **)bfrs, len);
    if( ret == -EAGAIN ) continue;
    if ( ret < 0 ) {
      alsa_recovery(ret);
      break;
    }
    for( i=0; i<zpcm_channels; ++i ) bfrs[i] += ret;
    len -= ret;
  }
  return ret < 0 ? ret : 0;
}

void dst_exit(GtkWidget *widget, gpointer data)
{
   exit(0);
}

gint 
key_press_cb(GtkWidget* widget, GdkEventKey* event, gpointer data)
{
#if 0
   if (event->length > 0)
      printf("The key event's string is `%s'\n", event->string);

   printf("The name of this keysym is `%s'\n", 
           gdk_keyval_name(event->keyval));

   switch (event->keyval) {
   case GDK_Home:
      printf("The Home key was pressed.\n");
      break;
   case GDK_Up:
      printf("The Up arrow key was pressed.\n");
      break;
   default:
      break;
   }

   if( gdk_keyval_is_lower(event->keyval) ) {
      printf("A non-uppercase key was pressed.\n");
   }
   else if( gdk_keyval_is_upper(event->keyval) ) {
      printf("An uppercase letter was pressed.\n");
   }
#endif
   switch (event->keyval) {
   case GDK_plus:
   case GDK_equal:
      nudge += 0.1;
      printf("+nudge = %f\n",nudge);
      break;
   case GDK_minus:
   case GDK_underscore:
      nudge -= 0.1;
      printf("-nudge = %f\n",nudge);
      break;
   case GDK_q:
      printf("exit\n");
      dst_exit(widget,data);
      break;
   }
   return TRUE;
}


double vstart;

double video_time(int stream)
{
  double vnow = zsrc->get_video_time(stream);
  if( vstart < 0. && vnow > 0. ) vstart = vnow;
  vnow += nudge;
  return vnow - vstart;
}

void vid_thread::
Proc()
{
  GtkWidget *window;
  GtkWidget *panel_hbox;
  GtkWidget *image;
  GdkPixbuf *pbuf0, *pbuf1;
  GdkImage  *img0, *img1;
  GdkVisual *visual;
  float frame_rate, frame_delay;
  unsigned char **rows, **row0, **row1, *cap, *cap0, *cap1;
  int ret, row, frame, more_data;
  int width, height, owidth, oheight;
  const int frame_drop = 1;
  int rheight = 1050;
  int rwidth = 1680;

  frame_rate = zsrc->frame_rate(vid);
  frame_delay = 2.0 / frame_rate;
  height = zsrc->video_height(vid);
  width = zsrc->video_width(vid);
  //oheight = rheight-96;
  //owidth = rwidth-64;
  oheight = height;
  owidth = width;
  int fheight = rheight - 96;
  int fwidth = rwidth - 64;
  if( oheight > fheight || owidth > fwidth ) {
    int mheight = (owidth * fheight) / fwidth;
    int mwidth = (oheight * fwidth) / fheight;
    if( mheight > fheight ) mheight = fheight;
    if( mwidth > fwidth ) mwidth = fwidth;
    oheight = mheight;
    owidth = mwidth;
  }
  else if( oheight < 1050/2 && owidth < 1650/2 ) {
    oheight = oheight * 2;
    owidth  = owidth * 2;
  }

  visual = gdk_visual_get_system();
  /* toplevel window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window),"destroy",
     GTK_SIGNAL_FUNC(dst_exit),NULL);
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
     GTK_SIGNAL_FUNC(key_press_cb),NULL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
  /* try for shared image bfr, only seems to work with gtk_rgb */
  img0 = gdk_image_new(GDK_IMAGE_SHARED, visual, owidth, oheight);
  pbuf0 = gdk_pixbuf_new_from_data((const guchar *)img0->mem,
             GDK_COLORSPACE_RGB,RGBA,8,owidth,oheight,owidth*BPP,NULL,NULL);
  cap0 = gdk_pixbuf_get_pixels(pbuf0);
  image = gtk_image_new_from_pixbuf(pbuf0);
  /* double buffered */
  img1 = gdk_image_new(GDK_IMAGE_SHARED, visual, owidth, oheight);
  pbuf1 = gdk_pixbuf_new_from_data((const guchar *)img1->mem,
             GDK_COLORSPACE_RGB,RGBA,8,owidth,oheight,owidth*BPP,NULL,NULL);
  cap1 = gdk_pixbuf_get_pixels(pbuf1);

  panel_hbox = gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(window), panel_hbox);
  /* pack image into panel */
  gtk_box_pack_start(GTK_BOX(panel_hbox), image, TRUE, TRUE, 0);


  row0 = new unsigned char *[oheight];
  row1 = new unsigned char *[oheight];
  int cmdl = COLOR_MODEL;
  for( row=0; row<oheight; ++row ) {
    row0[row] = cap0 + row*owidth*BPP;
    row1[row] = cap1 + row*owidth*BPP;
  }

  gtk_widget_show_all(window);
  cap = cap0;
  rows = row0;
  frame = 0;
  vstart = -1.;
  more_data = 1;

  while( done == 0 && more_data ) {
    if( !gtk_events_pending() ) {
      more_data = 0;
      double delay =  the_time() - video_time(vid);
      if( frame_drop ) {
        if( delay >= frame_delay ) {
          int nframes = (long)ceil(delay/frame_rate);
          if( nframes > 2 ) nframes = 2;
          dprintf(" d%d",nframes);
          zsrc->drop_frames(nframes,vid);
        }
      }
      delay = -delay;
      if( delay > 0 ) {
        dprintf(" D%5.3f",delay*1000.0);
        int us = (int)(delay*1000000.0);
        usleep(us);
      }
      ret = zsrc->read_frame(rows, 0, 0, width, height,
               owidth, oheight, cmdl, vid);
      //printf("%d %ld\n",frame,zsrc->vtrack[0]->demuxer->titles[0]->fs->current_byte);
      //printf("%d %ld %ld %ld\n",frame,zsrc->vtrack[0]->demuxer->titles[0]->fs->current_byte,
      //    zsrc->vtrack[0]->demuxer->titles[0]->fs->buffer->file_pos,
      //    zsrc->vtrack[0]->demuxer->titles[0]->fs->buffer->file_pos -
      //    zsrc->vtrack[0]->demuxer->titles[0]->fs->current_byte);
      if( verbose )
        printf("read_video(stream=%d, frame=%d) = %d\n", vid, frame, ret);
      GdkGC *blk = image->style->black_gc;
      gdk_draw_rgb_image(image->window,blk, 0,0,owidth,oheight,
         GDK_RGB_DITHER_NONE,cap,owidth*BPP);
#if 0
#if 0
      { static FILE *fp = 0;
        int sz = owidth*oheight*BPP;
        if( fp == 0 ) fp = fopen("/tmp/dat.raw","w");
        fwrite(cap,1,sz,fp);
      }
#else
      if( frame < 150 ) { static FILE *fp = 0;
        int z, sz = owidth*oheight*BPP;
        uint8_t zbfr[sz];
        if( fp == 0 ) fp = fopen("/tmp/dat.raw","r");
        fread(zbfr,1,sz,fp);
        for( z=0; z<sz && abs(zbfr[z]-cap[z])<=1; ++z );
        //for( z=0; z<sz && zbfr[z]==cap[z]; ++z );
        if( z < sz )
          printf("bug %d\n",z);
      }
#endif
#endif
      *(unsigned long *)&cap ^= ((unsigned long)cap0 ^ (unsigned long)cap1);
      *(unsigned long *)&rows ^= ((unsigned long)row0 ^ (unsigned long)row1);
      more_data |= !zsrc->end_of_video(vid);
      ++frame;
    }
    else
      gtk_main_iteration();
  }
  video_done = 1;
}

void stats()
{
  int has_audio = zsrc->has_audio();
  printf(" has_audio = %d\n", has_audio);
  int total_astreams = zsrc->total_astreams();
  printf(" total_astreams = %d\n", total_astreams);
  for( int astream=0; astream<total_astreams; ++astream ) {
    int audio_channels = zsrc->audio_channels(astream);
    printf("   audio_channels = %d\n", audio_channels);
    int sample_rate = zsrc->sample_rate(astream);
    printf("   sample_rate = %d\n", sample_rate);
    long audio_samples = zsrc->audio_samples(astream);
    printf("   audio_samples = %ld\n", audio_samples);
  }
  printf("\n");
  int has_video = zsrc->has_video();
  printf(" has_video = %d\n", has_video);
  int total_vstreams = zsrc->total_vstreams();
  printf(" total_vstreams = %d\n", total_vstreams);
  for( int vstream=0; vstream<total_vstreams; ++vstream ) {
    int width = zsrc->video_width(vstream);
    printf("   video_width = %d\n", width);
    int height = zsrc->video_height(vstream);
    printf("   video_height = %d\n", height);
    float frame_rate = zsrc->frame_rate(vstream);
    printf("   frame_rate = %f\n", frame_rate);
    long video_frames = zsrc->video_frames(vstream);
    printf("   video_frames = %ld\n", video_frames);
    int colormodel = zsrc->colormodel(vstream);
    printf("   colormodel = %d\n", colormodel);
  }
  int channel_count = zsrc->dvb.channel_count();
  printf("\ndvb data: %d channels\n",channel_count);
  for( int channel=0; channel<channel_count; ++channel ) {
    int major, minor, stream;  char name[8], enc[4];
    zsrc->dvb.get_channel(channel, major, minor);
    zsrc->dvb.get_station_id(channel, &name[0]);
    zsrc->dvb.total_astreams(channel, total_astreams);
    zsrc->dvb.total_vstreams(channel, total_vstreams);
    printf("  dvb channel %d.%d (%s) %d vstreams, %d astreams\n",
      major, minor, name, total_vstreams, total_astreams);
    for( int vstream=0; vstream<total_vstreams; ++vstream ) {
      zsrc->dvb.vstream_number(channel, vstream, stream);
      printf("    video%-2d = %d\n",vstream,stream);
    }
    for(int astream=0; astream<total_astreams; ++astream ) {
      zsrc->dvb.astream_number(channel, astream, stream, &enc[0]);
      printf("    audio%-2d = %d (%s)\n",astream,stream,&enc[0]);
    }
  }
  printf("\n");
}

void sig_int(int n)
{
  done = 1;
}

int ich[] = { /* June 14, 2009 */
   7, //  7.1  (KMGH-DT) 1 vstreams, 1 astreams (eng)
      //  7.27 (KZCO-SD) 1 vstreams, 1 astreams (spa)
   9, //  9.1  (KUSA-DT) 1 vstreams, 1 astreams (eng)
      //  9.2  (WX-Plus) 1 vstreams, 1 astreams (eng)
      //  9.3  (NBC Uni) 1 vstreams, 1 astreams (eng)
  13, // 12.1  (KBDI-DT) 1 vstreams, 1 astreams (eng)
      // 12.2  (KBDI-DC) 1 vstreams, 1 astreams (eng)
      // 12.3  (KBDI-WV) 1 vstreams, 1 astreams (eng)
  15, // 14.1  (KTFD-DT) 1 vstreams, 1 astreams (spa)
  19, // 20.1  (KTVD-DT) 1 vstreams, 1 astreams (eng)
  21, // 22.1  (KDVR DT) 1 vstreams, 2 astreams (eng) (spa)
  29, // 25.1  (KDEN-DT) 1 vstreams, 1 astreams (eng)
  32, // 31.1  (KDVR DT) 1 vstreams, 2 astreams (eng) (spa)
  34, //  2.1  (KWGN-DT) 1 vstreams, 2 astreams (eng) (eng)
  35, //  4.1  (KCNC-DT) 1 vstreams, 2 astreams (eng) (spa)
  38, // 38.1  (KPJR-1_) 1 vstreams, 1 astreams (eng)
      // 38.2  (KPJR-2_) 1 vstreams, 1 astreams (eng)
      // 38.3  (KPJR-3_) 1 vstreams, 1 astreams (eng)
      // 38.4  (KPJR-4_) 1 vstreams, 1 astreams (eng)
      // 38.5  (KPJR-5_) 1 vstreams, 1 astreams (eng)
  40, // 41.1  (KRMT 41) 1 vstreams, 1 astreams (eng)
  43, // 59.1  (ION____) 1 vstreams, 1 astreams (eng)
      // 59.2  (qubo___) 1 vstreams, 2 astreams (eng) (eng)
      // 59.3  (IONLife) 1 vstreams, 1 astreams (eng)
      // 59.4  (Worship) 1 vstreams, 1 astreams (eng)
  46, // 53.1  (KWHD-DT) 1 vstreams, 1 astreams (eng)
  51, // 50.1  (Univisi) 1 vstreams, 1 astreams (spa)
};

int main(int ac, char **av)
{
  setbuf(stdout,NULL);
  setbuf(stderr,NULL);
  if( ac < 2 ) {
    printf("channel ordinal required\n");
    return 1;
  }
  int ch_ord = atoi(av[1]);
  if( ch_ord >= 0 ) {
    gtk_set_locale();
    gtk_init(&ac, &av);
    audio_done = 0;
    video_done = 0;
    reset();
    if( ac == 3 ) {
      subchan = atoi(av[2]);
    }
    else if( ac == 4 ) {
      vid = atoi(av[2]);
      aud = atoi(av[3]);
    }
    if( !open_tuner(0,ch_ord,0,0) ) {
      if( !open_stream() ) {
        stats();
        zsrc->seek_byte(0);
        // zsrc->show_subtitle(0);
        start_stream();
        while( !done ) {
          usleep(100000);
          if( audio_done && video_done) break;
        }
        stop_stream();
        close_stream();
      }
    }
    close_tuner();
  }
  else {
    for( int ch=-ch_ord>2? -ch_ord : 2; ch<78; ++ch ) {
      printf("\r  %2d?\r",ch);
      reset();
      /* extra open/close clears previous bfr data */
      if( !open_tuner(0,ch,0,0) ) close_tuner();
      usleep(100000);
      if( !open_tuner(0,ch,0,0) && !open_stream() ) {
        int nch = zsrc->dvb.channel_count();
        if( nch > 0 ) {
          for( int ich=0, n=0; ich<nch; ++ich ) {
            int mjr, mnr;
            zsrc->dvb.get_channel(ich, mjr, mnr);
            int astrs, vstrs, stream;  char name[8], enc[4];
            zsrc->dvb.get_station_id(ich, &name[0]);
            zsrc->dvb.total_astreams(ich, astrs);
            zsrc->dvb.total_vstreams(ich, vstrs);
            if( n++ ) printf("      "); else printf("  %2d: ", ch);
            printf("   %2d.%-2d (%s) %d vstreams, %d astreams",
              mjr, mnr, name, vstrs, astrs);
            for( int j=0; j<astrs; ++j ) {
              zsrc->dvb.astream_number(ich, j, stream, &enc[0]);
              printf(" (%s)",&enc[0]);
            }
            printf("\n");
          }
        }
        close_stream();
      }
      close_tuner();
    }
  }
  return 0;
}

