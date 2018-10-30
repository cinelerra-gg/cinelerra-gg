#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

const double nudge = 3.5;
const double ahead = .5;

#ifdef __cplusplus
#include "libzmpeg3.h"
#else
#include "../libmpeg3/libmpeg3.h"
#define zmpeg3_t mpeg3_t
#endif

zmpeg3_t *ysrc;

/* alpha currently is broken in gdk */
/*   would render faster if alpha worked correctly */
#define RGBA FALSE

#if RGBA
#define BPP 4
#ifdef __cplusplus
#define COLOR_MODEL zmpeg3_t::cmdl_RGBA8888
#else
#define COLOR_MODEL MPEG3_RGBA8888
#endif

#else
#define BPP 3
#ifdef __cplusplus
#define COLOR_MODEL zmpeg3_t::cmdl_RGB888
#else
#define COLOR_MODEL MPEG3_RGB888
#endif
#endif


#define AUDIO
//#define VIDEO
static int verbose = 0;

/* c++ `pkg-config --cflags --libs gtk+-2.0` y.C ./x86_64/libzmpeg3.a -lpthread -lasound -lm */

#ifdef AUDIO
snd_pcm_t *zpcm = 0;
snd_pcm_uframes_t zpcm_ufrm_size = 0;
snd_pcm_sframes_t zpcm_sbfr_size = 0;
snd_pcm_sframes_t zpcm_sper_size = 0;
snd_pcm_sframes_t zpcm_total_samples = 0;
double zpcm_play_time = 0.;
pthread_mutex_t zpcm_lock;
unsigned int zpcm_rate = 0;
static const char *zpcm_device = "plughw:0,0";
static unsigned int zpcm_bfr_time_us = 500000; 
static unsigned int zpcm_per_time_us = 200000;
static snd_pcm_channel_area_t *zpcm_areas = 0;
static int zpcm_channels = 0;
static short **zpcm_buffers = 0;
static short *zpcm_samples = 0;

void alsa_close()
{
#ifdef __cplusplus
  if( zpcm_buffers != 0 ) { delete [] zpcm_buffers; zpcm_buffers = 0; }
  if( zpcm_areas != 0 ) { delete [] zpcm_areas; zpcm_areas = 0; }
  if( zpcm_samples != 0 ) { delete [] zpcm_samples; zpcm_samples = 0; }
#else
  if( zpcm_buffers != 0 ) { free(zpcm_buffers); zpcm_buffers = 0; }
  if( zpcm_areas != 0 ) { free(zpcm_areas); zpcm_areas = 0; }
  if( zpcm_samples != 0 ) { free(zpcm_samples); zpcm_samples = 0; }
#endif
  if( zpcm != 0 ) { snd_pcm_close(zpcm);  zpcm = 0; }
  pthread_mutex_destroy(&zpcm_lock);
}

void alsa_open(int chs,int rate)
{
  int ich, bits, byts, dir, ret;
  pthread_mutex_init(&zpcm_lock,0);
  snd_pcm_format_t fmt = SND_PCM_FORMAT_S16;
  snd_pcm_hw_params_t *phw;
  snd_pcm_sw_params_t *psw;
  snd_pcm_hw_params_alloca(&phw);
  snd_pcm_sw_params_alloca(&psw);

  zpcm = 0;
  zpcm_total_samples = 0;
  zpcm_play_time = 0.;
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
#ifdef __cplusplus
     zpcm_areas = new snd_pcm_channel_area_t[chs];
     byts = snd_pcm_format_physical_width(fmt) / 8;
     zpcm_samples = new short[zpcm_sper_size * chs * byts/2];
     zpcm_buffers = new short *[chs];
#else
     zpcm_areas = calloc(chs, sizeof(snd_pcm_channel_area_t));
     bits = snd_pcm_format_physical_width(fmt);
     byts = bits / 8;
     zpcm_samples = malloc(zpcm_sper_size * chs * byts);
     zpcm_buffers = malloc(chs * sizeof(short*));
#endif
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

int alsa_write(int length)
{
  struct timeval tv;
  int i, ret, count, retry;
  snd_pcm_sframes_t sample_delay;
  double play_time, time;
#ifdef __cplusplus
  short *bfrs[zpcm_channels];
#else
  short **bfrs = alloca(zpcm_channels*sizeof(short *));
#endif
  retry = 3;
  ret = count = 0;
  for( i=0; i<zpcm_channels; ++i ) bfrs[i] = zpcm_buffers[i];

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
  pthread_mutex_lock(&zpcm_lock);
  zpcm_total_samples += count;
  snd_pcm_delay(zpcm,&sample_delay);
  if( sample_delay > zpcm_total_samples )
    sample_delay = zpcm_total_samples;
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  play_time = (zpcm_total_samples - sample_delay) / (double)zpcm_rate;
  zpcm_play_time = time - play_time;
  pthread_mutex_unlock(&zpcm_lock);
  return ret < 0 ? ret : 0;
}

double alsa_time()
{
  double time, play_time;
  struct timeval tv;
  pthread_mutex_lock(&zpcm_lock);
  gettimeofday(&tv,NULL);
  time = tv.tv_sec + tv.tv_usec/1000000.0;
  play_time = time - zpcm_play_time;
  pthread_mutex_unlock(&zpcm_lock);
  return play_time;
}

#endif

int done = 0;

void sigint(int n)
{
  done = 1;
}


void dst_exit(GtkWidget *widget, gpointer data)
{
   exit(0);
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
    audio_channels = mpeg3_audio_channels(zsrc, astream);
    printf("   audio_channels = %d\n", audio_channels);
    sample_rate = mpeg3_sample_rate(zsrc, astream);
    printf("   sample_rate = %d\n", sample_rate);
    audio_samples = mpeg3_audio_samples(zsrc, astream);
    printf("   audio_samples = %ld\n", audio_samples);
  }
  printf("\n");
  has_video = mpeg3_has_video(zsrc);
  printf(" has_video = %d\n", has_video);
  total_vstreams = mpeg3_total_vstreams(zsrc);
  printf(" total_vstreams = %d\n", total_vstreams);
  for( vstream=0; vstream<total_vstreams; ++vstream ) {
    width = mpeg3_video_width(zsrc, vstream);
    printf("   video_width = %d\n", width);
    height = mpeg3_video_height(zsrc, vstream);
    printf("   video_height = %d\n", height);
    frame_rate = mpeg3_frame_rate(zsrc, vstream);
    printf("   frame_rate = %f\n", frame_rate);
    video_frames = mpeg3_video_frames(zsrc, vstream);
    printf("   video_frames = %ld\n", video_frames);
    colormodel = mpeg3_colormodel(zsrc, vstream);
    printf("   colormodel = %d\n", colormodel);
  }
}

#ifdef VIDEO
double vstart;
double last_vtime;

double video_time(zmpeg3_t *zsrc,int stream)
{
#ifdef __cplusplus
  double vtime = mpeg3_get_video_time(zsrc,stream);
#else
  double vtime = mpeg3_get_time(zsrc);
#endif
  double vnow = vtime;
  if( vstart == 0. ) vstart = vnow;
  if( vtime < last_vtime )
    vstart -= last_vtime;
  last_vtime = vtime;
  return vnow - vstart + nudge;
}

pthread_t the_video;

void *video_thread(void *the_zsrc)
{
  GtkWidget *window;
  GtkWidget *panel_hbox;
  GtkWidget *image;
  GdkPixbuf *pbuf0, *pbuf1;
  GdkImage  *img0, *img1;
  GdkVisual *visual;
  double delay;
  float frame_rate, frame_delay;
  unsigned char **rows, **row0, **row1, *cap, *cap0, *cap1;
  int ret, row, frame, dropped, more_data;
  int width, height, owidth, oheight;
  const int frame_drop = 1;
  zmpeg3_t *zsrc = (zmpeg3_t *)the_zsrc;

  frame_rate = mpeg3_frame_rate(zsrc, 0);
  frame_delay = 2.0 / frame_rate;
  height = mpeg3_video_height(zsrc, 0);
  width = mpeg3_video_width(zsrc, 0);
  //oheight = 2*height / 3;
  //owidth = 2*width / 3;
  //oheight = 1050-96;
  //owidth = 1680-64;
  //oheight = height;
  //owidth = width;
  oheight = 720;
  owidth = 1280;

  visual = gdk_visual_get_system();
  /* toplevel window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window),"destroy",
     GTK_SIGNAL_FUNC(dst_exit),NULL);
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


#ifdef __cplusplus
  row0 = new unsigned char *[oheight];
  row1 = new unsigned char *[oheight];
  int cmdl = COLOR_MODEL;
#else
  row0 = malloc(oheight*sizeof(unsigned char *));
  row1 = malloc(oheight*sizeof(unsigned char *));
  int cmdl = COLOR_MODEL;
#endif
  for( row=0; row<oheight; ++row ) {
    row0[row] = cap0 + row*owidth*BPP;
    row1[row] = cap1 + row*owidth*BPP;
  }

  gtk_widget_show_all(window);
  cap = cap0;
  rows = row0;
  frame = dropped = 0;
  vstart = last_vtime = 0.;
  more_data = 1;

  while( done == 0 && more_data ) {
    if( !gtk_events_pending() ) {
      more_data = 0;
//      delay = (frame+dropped)/frame_rate - alsa_time();
//      delay = the_time() - video_time(zsrc,0);
      if( frame_drop ) {
        if( -delay >= frame_delay ) {
          int nframes = (long)ceil(-delay/frame_rate);
          if( nframes > 2 ) nframes = 2;
          mpeg3_drop_frames(zsrc,nframes,0);
          dropped += nframes;
        }
      }
      //if( delay > 0 )
      //  usleep((int)(delay*1000000.0));
      ret = mpeg3_read_frame(zsrc, rows, 0, 0, width, height,
               owidth, oheight, cmdl, 0);
      //printf("%d %ld\n",frame,zsrc->vtrack[0]->demuxer->titles[0]->fs->current_byte);
      if( verbose )
        printf("read_video(stream=%d, frame=%d) = %d\n", 0, frame, ret);
      //if( frame > 500 && frame < 800 ) {
      //  mpeg3_previous_frame(zsrc,0);
      //  mpeg3_previous_frame(zsrc,0);
      //}
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
      { static FILE *fp = 0;
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
      if( frame > 250 ) exit(0);
#endif
      *(unsigned long *)&cap ^= ((unsigned long)cap0 ^ (unsigned long)cap1);
      *(unsigned long *)&rows ^= ((unsigned long)row0 ^ (unsigned long)row1);
      //printf(" %d/%d  %f  %f  %f\n",frame,dropped,(frame+dropped)/frame_rate,
      //  video_time(zsrc,0),mpeg3_get_video_time(zsrc,0));
      ++frame;
      more_data |= !mpeg3_end_of_video(zsrc,0);
    }
    else
      gtk_main_iteration();
  }

  return (void*)0;
}

#endif

#ifdef AUDIO
double astart;

double audio_time(zmpeg3_t *zsrc, int stream)
{
#ifdef __cplusplus
  double anow = mpeg3_get_audio_time(zsrc,stream);
#else
  double anow = mpeg3_get_time(zsrc);
#endif
  if( astart < 0. ) astart = anow;
  return anow - astart;
}

pthread_t the_audio;

void *audio_thread(void *the_zsrc)
{
  double delay;
  int audio_channels, sample_rate, more_data;
  zmpeg3_t *zsrc = (zmpeg3_t *)the_zsrc;
  int stream = 0;

  audio_channels = mpeg3_audio_channels(zsrc, stream);
  sample_rate = mpeg3_sample_rate(zsrc, stream);
  alsa_open(audio_channels,sample_rate);
  astart = -1.;
  more_data = 1;

  while( done == 0 && more_data ) {
    more_data = 0;
    int ich;
    int n = alsa_bfrsz();
    for( ich=0; ich<audio_channels; ++ich) {
      short *bfr = alsa_bfr(ich);
      int ret = ich == 0 ?
        mpeg3_read_audio(zsrc, 0, bfr, ich, n, stream) :
        mpeg3_reread_audio(zsrc, 0, bfr, ich, n, stream);
      if( verbose )
        printf("read_audio(stream=%d,channel=%d) = %d\n", stream, ich, ret);
    }

    alsa_write(n);
    delay = audio_time(zsrc,stream) - the_time();
    if( (delay-ahead) > 0. )
      usleep((int)(delay*1000000.0/2.));
    more_data |= !mpeg3_end_of_audio(zsrc,0);
  }

  alsa_close();
  return (void*)0;
}

#endif


int main(int ac, char **av)
{
  int ret;
  setbuf(stdout,NULL);
  tstart = -1.;

#ifdef VIDEO
  gtk_set_locale();
  gtk_init(&ac, &av);
#endif

  //zmpeg3_t* zsrc = mpeg3_open("/tmp/dat",&ret);
  //zmpeg3_t* zsrc = mpeg3_zopen(0,"/tmp/dat",&ret,ZIO_UNBUFFERED+ZIO_SINGLE_ACCESS+ZIO_SEQUENTIAL);
  //zmpeg3_t* zsrc = mpeg3_open("/tmp/dat.toc",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/root/LimeWire/Shared/Britney Spears - Pepsi Commercial with Bob Dole.mpeg",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/tmp/dat.mp3",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/dvd/VIDEO_TS/VTS_01_0.IFO",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/home/dat2.vts",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/dvd/BDMV/STREAM/00000.m2ts",&ret);
  zmpeg3_t* zsrc = mpeg3_open(av[1],&ret);
  ysrc = zsrc;
  printf(" ret = %d\n",ret);
  if( ret != 0 ) exit(1);
  mpeg3_stats(zsrc);

#ifdef AUDIO
#ifdef __cplusplus
  zmpeg3_t* zsrc1 = zsrc;
#else
  zmpeg3_t* zsrc1 = mpeg3_open_copy(zsrc->fs->path, zsrc, &ret);
#endif
#endif

  mpeg3_set_cpus(zsrc,3);
  mpeg3_seek_byte(zsrc,0);
  mpeg3_show_subtitle(zsrc, 0);
  signal(SIGINT,sigint);

#ifdef AUDIO
  pthread_create(&the_audio, NULL, audio_thread, (void*)zsrc1);
#endif
#ifdef VIDEO
  pthread_create(&the_video, NULL, video_thread, (void*)zsrc);
#endif
#ifdef VIDEO
  pthread_join(the_video,NULL);
#endif
#ifdef AUDIO
  pthread_join(the_audio,NULL);
#endif

  mpeg3_close(zsrc);
  return 0;
}

