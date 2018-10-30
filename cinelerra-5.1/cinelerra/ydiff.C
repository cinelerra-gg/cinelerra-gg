#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

extern "C" {
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfilter.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}

int done = 0;
int64_t tm = 0, tn = 0;

void sigint(int n)
{
  done = 1;
}

class gg_window
{
public:
  gg_window(Display *display, int x, int y, int w, int h);
  ~gg_window();
  Display *display;
  Window win;
  GC gc;
  int x, y, w, h;
  void show();
  void hide();
};

gg_window::gg_window(Display *display, int x, int y, int w, int h)
{
  this->display = display;
  this->x = x;  this->y = y;
  this->w = w;  this->h = h;

  Window root = DefaultRootWindow(display);
  Screen *screen = DefaultScreenOfDisplay(display);
  Visual *visual = DefaultVisualOfScreen(screen);
  int depth = DefaultDepthOfScreen(screen);
  int border = 0;
  unsigned long gcmask = GCGraphicsExposures;
  XGCValues gcvalues;
  gcvalues.graphics_exposures = 0;
  gc = XCreateGC(display, root, gcmask, &gcvalues);

  XSetWindowAttributes attributes;
  attributes.background_pixel = BlackPixel(display, DefaultScreen(display));
  attributes.border_pixel = WhitePixel(display, DefaultScreen(display));
  attributes.event_mask =
    EnterWindowMask | LeaveWindowMask |
    ButtonPressMask | ButtonReleaseMask |
    PointerMotionMask | FocusChangeMask;
  int valueMask = CWBackPixel | CWBorderPixel | CWEventMask;
  this->win = XCreateWindow(display, root, x, y, w, h, border, depth,
      InputOutput, visual, valueMask, &attributes);
}

gg_window::~gg_window()
{
  XFreeGC(display, gc);
  XDestroyWindow(display, win);
}

void gg_window::show()
{
  XMapWindow(display,win);
  XFlush(display);
}
void gg_window::hide()
{
  XUnmapWindow(display,win);
  XFlush(display);
}

class gg_ximage
{
public:
  Display *display;
  XShmSegmentInfo shm_info;
  XImage *ximage;
  int w, h;
  unsigned char *data;
  int shm, sz;
  uint32_t lsb[3];
  gg_ximage(Display *display, int w, int h, int shm);
  ~gg_ximage();

  void put_image(gg_window &gw);
};

gg_ximage::gg_ximage(Display *display, int w, int h, int shm)
{
  this->display = display;
  this->w = w;  this->h = h;
  this->shm = shm;

  ximage = 0;  sz = 0;  data = 0;
  Screen *screen = DefaultScreenOfDisplay(display);
  Visual *visual = DefaultVisualOfScreen(screen);
  int depth = DefaultDepthOfScreen(screen);

  if( shm ) {
    ximage = XShmCreateImage(display, visual, depth, ZPixmap, (char*)NULL, &shm_info, w, h);
// Create shared memory
    sz = h * ximage->bytes_per_line;
    shm_info.shmid = shmget(IPC_PRIVATE, sz + 8, IPC_CREAT | 0777);
    if(shm_info.shmid < 0) perror("shmget");
    data = (unsigned char *)shmat(shm_info.shmid, NULL, 0);
// This causes it to automatically delete when the program exits.
    shmctl(shm_info.shmid, IPC_RMID, 0);
    ximage->data = shm_info.shmaddr = (char*)data;
    shm_info.readOnly = 0;
// Get the real parameters
    if(!XShmAttach(display, &shm_info)) perror("XShmAttach");
  }
  else {
      ximage = XCreateImage(display, visual, depth, ZPixmap, 0, (char*)data, w, h, 8, 0);
      sz = h * ximage->bytes_per_line;
      data = new unsigned char[sz+8];
  }
  memset(data, 0, sz);
  ximage->data = (char*) data;
  lsb[0] = ximage->red_mask & ~(ximage->red_mask<<1);
  lsb[1] = ximage->green_mask & ~(ximage->green_mask<<1);
  lsb[2] = ximage->blue_mask & ~(ximage->blue_mask<<1);
}

gg_ximage::~gg_ximage()
{
  if( shm ) {
    data = 0;
    ximage->data = 0;
    XDestroyImage(ximage);
    XShmDetach(display, &shm_info);
    XFlush(display);
    shmdt(shm_info.shmaddr);
  }
  else {
    delete [] data;
    data = 0;
    ximage->data = 0;
    XDestroyImage(ximage);
  }
}

void gg_ximage::put_image(gg_window &gw)
{
  Display *display = gw.display;
  Window win = gw.win;
  GC gc = gw.gc;
  if( shm )
    XShmPutImage(display, win, gc, ximage, 0,0, 0,0,w,h, 0);
  else
    XPutImage(display, win, gc, ximage, 0,0, 0,0,w,h); 
  XFlush(display);
}

class gg_thread
{
public:
  pthread_t tid;
  gg_window &gw;
  gg_ximage *imgs[2], *img;
  int active, done;
  gg_thread(gg_window &gw, int shm) ;
  ~gg_thread();

  static void *entry(void *t);
  void start();
  void *run();
  void join();
  void post(gg_ximage *ip);
  gg_ximage *next_img();

  pthread_mutex_t draw;
  void draw_lock() { pthread_mutex_lock(&draw); }
  void draw_unlock() { pthread_mutex_unlock(&draw); }
};

gg_thread::gg_thread(gg_window &gw, int shm)
 : gw(gw)
{
  imgs[0] = new gg_ximage(gw.display, gw.w, gw.h, shm);
  imgs[1] = new gg_ximage(gw.display, gw.w, gw.h, shm);
  done = -1;
  img = 0;  active = 0;
  pthread_mutex_init(&draw, 0);
}

gg_thread::~gg_thread()
{
  delete imgs[0];
  delete imgs[1];
  pthread_mutex_destroy(&draw);
}

void *gg_thread::entry(void *t)
{
  return ((gg_thread*)t)->run();
}

void gg_thread::start()
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setinheritsched(&attr, PTHREAD_INHERIT_SCHED);
  done = 0;
  pthread_create(&tid, &attr, &entry, this);
  pthread_attr_destroy(&attr);
}
void gg_thread::join()
{
  done = 1;
  pthread_join(tid, 0);
}

void *gg_thread::run()
{
  while( !done ) {
    if( XPending(gw.display) ) {
      XEvent xev;
      XNextEvent(gw.display, &xev);
      switch( xev.type ) {
      case KeyPress:
      case KeyRelease:
      case ButtonPress:
      case Expose:
        break;
      }
      continue;
    }

    if( !img ) { usleep(10000);  continue; }
    img->put_image(gw);
    img = 0;
    draw_unlock();
  }
  return (void*)0;
}

gg_ximage *gg_thread::next_img()
{
  gg_ximage *ip = imgs[active];
  active ^= 1;
  return ip;
}

void gg_thread::post(gg_ximage *ip)
{
  this->img = ip;
}


class ffcmpr {
public:
  ffcmpr();
  ~ffcmpr();
  AVPacket ipkt;
  AVFormatContext *fmt_ctx;
  AVFrame *ipic;
  AVStream *st;
  AVCodecContext *ctx;
  AVPixelFormat pix_fmt;
  double frame_rate;
  int width, height;
  int need_packet, eof;
  int open_decoder(const char *filename, int vid_no);
  void close_decoder();
  AVFrame *read_frame();
};

ffcmpr::ffcmpr()
{
  av_init_packet(&this->ipkt);
  this->fmt_ctx = 0;
  this->ipic = 0;
  this->st = 0;
  this->ctx = 0 ;
  this->frame_rate = 0;
  this->need_packet = 0;
  this->eof = 0;
  this->pix_fmt = AV_PIX_FMT_NONE;
  width = height = 0;
}

void ffcmpr::close_decoder()
{
  av_packet_unref(&ipkt);
  if( !fmt_ctx ) return;
  if( ctx ) avcodec_free_context(&ctx);
  avformat_close_input(&fmt_ctx);
  av_frame_free(&ipic);
}

ffcmpr::~ffcmpr()
{
  close_decoder();
}

int ffcmpr::open_decoder(const char *filename, int vid_no)
{
  struct stat fst;
  if( stat(filename, &fst) ) return 1;

  av_log_set_level(AV_LOG_VERBOSE);
  fmt_ctx = 0;
  AVDictionary *fopts = 0;
  av_register_all();
  av_dict_set(&fopts, "formatprobesize", "5000000", 0);
  av_dict_set(&fopts, "scan_all_pmts", "1", 0);
  av_dict_set(&fopts, "threads", "auto", 0);
  int ret = avformat_open_input(&fmt_ctx, filename, NULL, &fopts);
  av_dict_free(&fopts);
  if( ret < 0 ) {
    fprintf(stderr,"file open failed: %s\n", filename);
    return ret;
  }
  ret = avformat_find_stream_info(fmt_ctx, NULL);
  if( ret < 0 ) {
    fprintf(stderr,"file probe failed: %s\n", filename);
    return ret;
  }

  this->st = 0;
  for( int i=0; !this->st && ret>=0 && i<(int)fmt_ctx->nb_streams; ++i ) {
    AVStream *fst = fmt_ctx->streams[i];
    AVMediaType type = fst->codecpar->codec_type;
    if( type != AVMEDIA_TYPE_VIDEO ) continue;
    if( --vid_no < 0 ) this->st = fst;
  }

  AVCodecID codec_id = st->codecpar->codec_id;
  AVDictionary *copts = 0;
  //av_dict_copy(&copts, opts, 0);
  AVCodec *decoder = avcodec_find_decoder(codec_id);
  ctx = avcodec_alloc_context3(decoder);
  avcodec_parameters_to_context(ctx, st->codecpar);
  if( avcodec_open2(ctx, decoder, &copts) < 0 ) {
    fprintf(stderr,"codec open failed: %s\n", filename);
    return -1;
  }
  av_dict_free(&copts);
  ipic = av_frame_alloc();
  eof = 0;
  need_packet = 1;

  AVRational framerate = av_guess_frame_rate(fmt_ctx, st, 0);
  this->frame_rate = !framerate.den ? 0 : (double)framerate.num / framerate.den;
  this->pix_fmt = (AVPixelFormat)st->codecpar->format;
  this->width  = st->codecpar->width;
  this->height = st->codecpar->height;
  return 0;
}

AVFrame *ffcmpr::read_frame()
{
  av_frame_unref(ipic);

  for( int retrys=1000; --retrys>=0; ) {
    if( need_packet ) {
      if( eof ) return 0;
      AVPacket *pkt = &ipkt;
      av_packet_unref(pkt);
      int ret = av_read_frame(fmt_ctx, pkt);
      if( ret < 0 ) {
        if( ret != AVERROR_EOF ) return 0;
        ret = 0;  eof = 1;  pkt = 0;
      }
      if( pkt ) {
        if( pkt->stream_index != st->index ) continue;
        if( !pkt->data || !pkt->size ) continue;
      }
      avcodec_send_packet(ctx, pkt);
      need_packet = 0;
    }
    int ret = avcodec_receive_frame(ctx, ipic);
    if( ret >= 0 ) return ipic;
    if( ret != AVERROR(EAGAIN) ) {
      eof = 1; need_packet = 0;
      break;
    }
    need_packet = 1;
  }
  return 0;
}

static int diff_frame(AVFrame *afrm, AVFrame *bfrm, gg_ximage *ximg, int w, int h)
{
  int n = 0, m = 0;
  uint8_t *arow = afrm->data[0];
  uint8_t *brow = bfrm->data[0];
  uint8_t *frow = ximg->data;
  int asz = afrm->linesize[0], bsz = afrm->linesize[0];
  XImage *ximage = ximg->ximage;
  int fsz = ximage->bytes_per_line;
  int rsz = w, bpp = (ximage->bits_per_pixel+7)/8;
  uint32_t *lsb = ximg->lsb;

  for( int y=h; --y>=0; arow+=asz, brow+=bsz, frow+=fsz ) {
    uint8_t *ap = arow, *bp = brow, *fp = frow;
    for( int x=rsz; --x>=0; ) {
      uint32_t rgb = 0;  uint8_t *rp = fp;
      for( int i=0; i<3; ++i ) {
        int d = *ap++ - *bp++;
        int v = d + 128;
        if( v > 255 ) v = 255;
        else if( v < 0 ) v = 0;
        rgb |= v * lsb[i];
        m += d;
        if( d < 0 ) d = -d;
        n += d;
      }
      if( ximage->byte_order == MSBFirst )
        for( int i=3; --i>=0; ) *rp++ = rgb>>(8*i);
      else
        for( int i=0; i<3; ++i ) *rp++ = rgb>>(8*i);
      fp += bpp;
    }
  }
  int sz = h*rsz;
  printf("%d %d %d %f", sz, m, n, (double)n/sz);
  tm += m;  tn += n;
  return n;
}

int main(int ac, char **av)
{
  int ret;
  setbuf(stdout,NULL);
  XInitThreads();
  Display *display = XOpenDisplay(getenv("DISPLAY"));
  if( !display ) {
    fprintf(stderr,"Unable to open display\n");
    exit(1);
  }

  ffcmpr a, b;
  if( a.open_decoder(av[1],0) ) return 1;
  if( b.open_decoder(av[2],0) ) return 1;

  printf("file a:%s\n", av[1]);
  printf("  id 0x%06x:", a.ctx->codec_id);
  const AVCodecDescriptor *adesc = avcodec_descriptor_get(a.ctx->codec_id);
  printf("  video %s\n", adesc ? adesc->name : " (unkn)");
  printf(" %dx%d %5.2f", a.width, a.height, a.frame_rate);
  const char *apix = av_get_pix_fmt_name(a.pix_fmt);
  printf(" pix %s\n", apix ? apix : "(unkn)");

  printf("file b:%s\n", av[2]);
  printf("  id 0x%06x:", b.ctx->codec_id);
  const AVCodecDescriptor *bdesc = avcodec_descriptor_get(b.ctx->codec_id);
  printf("  video %s\n", bdesc ? bdesc->name : " (unkn)");
  printf(" %dx%d %5.2f", b.width, b.height, b.frame_rate);
  const char *bpix = av_get_pix_fmt_name(b.pix_fmt);
  printf(" pix %s\n", bpix ? bpix : "(unkn)");

//  if( a.ctx->codec_id != b.ctx->codec_id ) { printf("codec mismatch\n"); return 1;}
  if( a.width != b.width ) { printf("width mismatch\n"); return 1;}
  if( a.height != b.height ) { printf("height mismatch\n"); return 1;}
//  if( a.frame_rate != b.frame_rate ) { printf("framerate mismatch\n"); return 1;}
//  if( a.pix_fmt != b.pix_fmt ) { printf("format mismatch\n"); return 1;}

  signal(SIGINT,sigint);

  struct SwsContext *a_cvt = sws_getCachedContext(0, a.width, a.height, a.pix_fmt,
                a.width, a.height, AV_PIX_FMT_RGB24, SWS_POINT, 0, 0, 0);
  struct SwsContext *b_cvt = sws_getCachedContext(0, b.width, b.height, b.pix_fmt,
                b.width, b.height, AV_PIX_FMT_RGB24, SWS_POINT, 0, 0, 0);
  if( !a_cvt || !b_cvt ) {
    printf("sws_getCachedContext() failed\n");
    return 1;
  }

  AVFrame *afrm = av_frame_alloc();
  av_image_alloc(afrm->data, afrm->linesize,
     a.width, a.height, AV_PIX_FMT_RGB24, 1);

  AVFrame *bfrm = av_frame_alloc();
  av_image_alloc(bfrm->data, bfrm->linesize,
     b.width, b.height, AV_PIX_FMT_RGB24, 1);
{ gg_window gw(display, 10,10, a.width,a.height);
  gw.show();
  gg_thread thr(gw, 1);
  thr.start();

  int64_t err = 0;
  int frm_no = 0;

  if( ac>3 && (ret=atoi(av[3])) ) {
    while( ret > 0 ) { a.read_frame(); --ret; }
    while( ret < 0 ) { b.read_frame(); ++ret; }
  }

  while( !done ) {
    AVFrame *ap = a.read_frame();
    if( !ap ) break;
    AVFrame *bp = b.read_frame();
    if( !bp ) break;
    sws_scale(a_cvt, ap->data, ap->linesize, 0, ap->height,
       afrm->data, afrm->linesize);
    sws_scale(b_cvt, bp->data, bp->linesize, 0, bp->height,
       bfrm->data, bfrm->linesize);
    thr.draw_lock();
    gg_ximage *fimg = thr.next_img();
    ret = diff_frame(afrm, bfrm, fimg, ap->width, ap->height);
    thr.post(fimg);
    err += ret;  ++frm_no;
    printf("  %d\n",frm_no);
  }

  av_freep(&afrm->data);
  av_frame_free(&afrm);
  av_freep(&bfrm->data);
  av_frame_free(&bfrm);
  
  b.close_decoder();
  a.close_decoder();

  thr.join();
  gw.hide(); }
  XCloseDisplay(display);
  printf("\n%jd %jd\n", tm, tn);
  return 0;
}

