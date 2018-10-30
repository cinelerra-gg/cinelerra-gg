#include <stdio.h>
#include <stdlib.h>
#include <alsa/asoundlib.h>
#include <sys/time.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#ifdef __cplusplus
#include "libzmpeg3.h"
#else
#include "../libmpeg3/libmpeg3.h"
#define zmpeg3_t mpeg3_t
#endif

#define AUDIO
#define VIDEO

/* c++ `pkg-config --cflags --libs gtk+-2.0` y.C ./x86_64/libzmpeg3.a -lpthread -lasound -lm */

#ifdef AUDIO
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
     SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
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

int alsa_write(int len)
{
  int i, ret;
#ifdef __cplusplus
  short *bfrs[zpcm_channels];
#else
  short **bfrs = alloca(zpcm_channels*sizeof(short *));
#endif
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

double the_time()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + tv.tv_usec/1000000.0;
}

int main(int ac, char **av)
{
  int ret, more_data, frame, astream, vstream;
  int has_audio, total_astreams, audio_channels, sample_rate;
  int has_video, total_vstreams, width, height, colormodel;
  int owidth, oheight;
  long audio_samples, video_frames;
  float frame_rate;

#ifdef VIDEO
  GtkWidget *window;
  GtkWidget *panel_hbox;
  GtkWidget *image;
  GdkPixbuf *pbuf0, *pbuf1;
  GdkImage  *img0, *img1;
  GdkVisual *visual;
  double start, zstart, now, znow, delay;
  unsigned char **rows, **row0, **row1, *cap, *cap0, *cap1;
  int row;

  start = the_time();
  zstart = -1;
#endif

  setbuf(stdout,NULL);

  zmpeg3_t* zsrc = mpeg3_open("/tmp/dat",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/root/LimeWire/Shared/Britney Spears - Pepsi Commercial with Bob Dole.mpeg",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/tmp/dat.mp3",&ret);
  //zmpeg3_t* zsrc = mpeg3_open("/dvd/VIDEO_TS/VIDEO_TS.IFO",&ret);
  printf(" ret = %d\n",ret);
  if( ret != 0 ) exit(1);
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

  //mpeg3_set_cpus(zsrc,2);
  mpeg3_seek_byte(zsrc,0);
  //oheight = 1050-96;
  //owidth = 1680-64;
  oheight = height;
  owidth = width;

#ifdef AUDIO
  alsa_open(audio_channels,sample_rate);
#endif
#ifdef VIDEO
  gtk_set_locale();
  gtk_init(&ac, &av);
  visual = gdk_visual_get_system();
  /* toplevel window */
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window),"destroy",
     GTK_SIGNAL_FUNC(dst_exit),NULL);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_NONE);
  /* try for shared image bfr, only seems to work with gtk_rgb */
  img0 = gdk_image_new(GDK_IMAGE_SHARED, visual, owidth, oheight);
  pbuf0 = gdk_pixbuf_new_from_data((const guchar *)img0->mem,
             GDK_COLORSPACE_RGB,FALSE,8,owidth,oheight,owidth*3,NULL,NULL);
  cap0 = gdk_pixbuf_get_pixels(pbuf0);
  image = gtk_image_new_from_pixbuf(pbuf0);
  /* double buffered */
  img1 = gdk_image_new(GDK_IMAGE_SHARED, visual, owidth, oheight);
  pbuf1 = gdk_pixbuf_new_from_data((const guchar *)img1->mem,
             GDK_COLORSPACE_RGB,FALSE,8,owidth,oheight,owidth*3,NULL,NULL);
  cap1 = gdk_pixbuf_get_pixels(pbuf1);

  panel_hbox = gtk_hbox_new(FALSE,0);
  gtk_container_add(GTK_CONTAINER(window), panel_hbox);
  /* pack image into panel */
  gtk_box_pack_start(GTK_BOX(panel_hbox), image, TRUE, TRUE, 0);


#ifdef __cplusplus
  row0 = new unsigned char *[oheight];
  row1 = new unsigned char *[oheight];
  int cmdl = zmpeg3_t::cmdl_RGB888;
#else
  row0 = malloc(oheight*sizeof(unsigned char *));
  row1 = malloc(oheight*sizeof(unsigned char *));
  int cmdl = MPEG3_RGB888;
#endif
  for( row=0; row<oheight; ++row ) {
    row0[row] = cap0 + row*owidth*3;
    row1[row] = cap1 + row*owidth*3;
  }

  gtk_widget_show_all(window);
  cap = cap0;
  rows = row0;
#endif

  signal(SIGINT,sigint);
  frame = 0;
  more_data = 1;

  while( done == 0 && more_data ) {
#ifdef VIDEO
    if( !gtk_events_pending() ) {
#endif
      more_data = 0;
#ifdef AUDIO
      int ich;
      int n = alsa_bfrsz();
      for( ich=0; ich<audio_channels; ++ich) {
        short *bfr = alsa_bfr(ich);
        ret = ich == 0 ?
          mpeg3_read_audio(zsrc, 0, bfr, ich, n, 0) :
          mpeg3_reread_audio(zsrc, 0, bfr, ich, n, 0);
        printf("read_audio(stream=%d,channel=%d) = %d\n", 0, ich, ret);
      }
      alsa_write(n);
      more_data |= !mpeg3_end_of_audio(zsrc,0);
#endif
#ifdef VIDEO
      ret = mpeg3_read_frame(zsrc, rows, 0, 0, width, height,
               owidth, oheight, cmdl, 0);
      printf("read_video(stream=%d, frame=%d) = %d\n", 0, frame, ret);
      GdkGC *blk = image->style->black_gc;
      gdk_draw_rgb_image(image->window,blk, 0,0,owidth,oheight,
         GDK_RGB_DITHER_NONE,cap,owidth*3);
      *(unsigned long *)&cap ^= ((unsigned long)cap0 ^ (unsigned long)cap1);
      *(unsigned long *)&rows ^= ((unsigned long)row0 ^ (unsigned long)row1);
      now = the_time();
      znow = mpeg3_get_time(zsrc);
      if( zstart < 0. ) zstart = znow;
      delay = (znow-zstart) - (now-start);
      if( delay > 0 ) usleep((int)(delay*100000.0));
      more_data |= !mpeg3_end_of_video(zsrc,0);
#endif
      ++frame;
#ifdef VIDEO
    }
    else
      gtk_main_iteration();
#endif
  }

#ifdef AUDIO
  alsa_close();
#endif
  mpeg3_close(zsrc);
  return 0;
}

