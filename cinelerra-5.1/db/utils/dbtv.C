#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <sys/time.h>

#include "s.C"

#define WIDTH 640
#define HEIGHT 480

#define SWIDTH 80
#define SHEIGHT 45

double the_time()
{
  struct timeval tv;
  gettimeofday(&tv,NULL);
  return tv.tv_sec + tv.tv_usec/1000000.0;
}

void loadImage(int x, int y, XImage *image, unsigned char *cp, int width, int height)
{
  int i, j, k, b0, bi, npixel, nline;
  unsigned long rmask, gmask, bmask;
  double rshft, gshft, bshft;
  unsigned char r, g, b, *bp, *dp, *data;
  unsigned long pix, pixel;

  npixel = image->bits_per_pixel / 8;
  /*  A, R, G, B  or A, B, G, R */
  if( image->byte_order != MSBFirst ) {
    bi = 1;  b0 = 0;
  }
  else {
    bi = -1; b0 = npixel-1;
  }

  rmask = image->red_mask;    rshft = rmask/255.0;
  gmask = image->green_mask;  gshft = gmask/255.0;
  bmask = image->blue_mask;   bshft = bmask/255.0;
  nline = image->bytes_per_line;
  data = (unsigned char *)image->data + y*nline + x*npixel;

  for( k=0; k<height; k++ ) {
    dp = data;
    for( j=0; j<width; ++j ) {
      r = g = b = *cp;
      pixel = ((int)(r*rshft + 0.5) & rmask)
            + ((int)(g*gshft + 0.5) & gmask)
            + ((int)(b*bshft + 0.5) & bmask);
      bp = dp + b0;  pix = pixel;
      for( i=npixel; --i>0; pix>>=8 ) {
        *bp = pix;  bp += bi;
      }
      dp += npixel;  ++cp;
    }
    data += nline;
  }

}

int done = -1;
theDb db;

static void
clientMsg(Display *dpy,Window w,Atom amsg,int fmt)
{
  XClientMessageEvent ev;
  memset(&ev,0,sizeof(ev));
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = amsg;
  ev.format = fmt;
   XSendEvent(dpy,w,False,0,(XEvent *)&ev);
}

static void
scale0(uint8_t *idata,int iw,int ih,uint8_t *odata,int ow,int oh)
{
   int i, dx, dy;
   double iw1 = iw-1;
   double ow1 = ow-1;

   int idx[ow];
   for( dx=0; dx<ow; ++dx )
      idx[dx] = ((int)((dx*iw1)/ow1));

   int rsz = iw;
   double ih1 = ih-1;
   double oh1 = oh-1;
   for( dy=0;  dy<oh; ++dy ) {
      uint8_t *row = &idata[rsz * ((int)((dy*ih1)/oh1))];
      for( dx=0; dx<ow; ++dx ) {
         uint8_t *bp = &row[idx[dx]];
         for( i=1; --i>=0; *odata++ = bp[i]);
      }
   }
}

#if 0
static void
scale1(uint8_t *idata,int iw,int ih,int rx,int ry,int rw,int rh,
       uint8_t *odata,int ow,int oh,int sx,int sy,int sw,int sh)
{
   //if( rw < 3 || rh < 3 || sw < 3 || sh < 3 ) return;
   int i, dx, dy;  int ibpp = 1, obpp = 1;
   int rsz = ibpp * iw, ssz = obpp * ow;
   double iw2 = rw-2, ih2 = rh-2;
   double ow1 = iw2/(sw-1), oh1 = ih2/(sh-1);
   odata += sy*ssz + 4*sx;

   for( dy=0; dy<sh; ++dy,odata+=ssz ) {
      double fy = dy*oh1;
      int iy = (int)fy;
      int yf1 = fy - iy;
      int yf0 = 1.0 - yf1;
      int idy = rsz * (iy + ry);
      uint8_t *row0 = &idata[idy];
      uint8_t *row1 = &idata[idy+rsz];
      uint8_t *bp = odata;

      for( dx=0; dx<sw; ++dx ) {
         double fx = dx*ow1;
         int ix = (int)fx;
         double xf1 = fx - ix;
         double xf0 = 1.0 - xf1;
         int idx = ibpp * (ix + rx);
         uint8_t *cp00 = &row0[idx];
         uint8_t *cp01 = &row0[idx+ibpp];
         uint8_t *cp10 = &row1[idx];
         uint8_t *cp11 = &row1[idx+ibpp];
         double a00 = yf0 * xf0;
         double a01 = yf0 * xf1;
         double a10 = yf1 * xf0;
         double a11 = yf1 * xf1;
         for( bp[i=ibpp]=0; --i>=0; ++cp00, ++cp01, ++cp10, ++cp11 )
            bp[i] = *cp00*a00 + *cp01*a01 + *cp10*a10 + *cp11*a11 + 0.5;
         bp += obpp;
      }
   }
}


static void
scale2(uint8_t *idata,int iw,int ih,int rx,int ry,int rw,int rh,
      uint8_t *odata,int ow,int oh,int sx,int sy,int sw,int sh)
{
	if( rw < 3 || rh < 3 || sw < 3 || sh < 3 ) return;
	const int ibpp=1, obpp=1;
	int x, y, dx, dy;
	int rsz = ibpp*iw, ssz = obpp*ow;
	double rw0=rw, rh0=rh;
	double ow0=sw, oh0=sh;
	double px = rw0/ow0, py = rh0/oh0;
	double pz = (ow0*oh0)/(rw0*rh0);
	double ly = 0.0;
	odata += sy*ssz + obpp*sx;

	for( dy=1; dy<=sh; ++dy,odata+=ssz ) {
		int iy0 = (int)ly;
		double yf0 = ly - iy0;
		double ny = dy*py + 1e-99;
		int iy1 = (int)ny;
		double yf1 = ny - iy1;
		uint8_t *bp = odata;
		double lx = 0.0;

		for( dx=1; dx<=sw; ++dx ) {
			int ix0 = (int)lx;
			double xf0 = lx - ix0;
			double nx = dx*px + 1e-99;
			int ix1 = (int)nx;
			double xf1 = nx - ix1;
			double r=0.0;
			for( y=iy0; y<=iy1; ++y ) {
				int idy = rsz * (y + ry);
				uint8_t *row = &idata[idy];
				double yw = pz;
				if( y == iy0 ) yw *= (y==iy1 ? yf1-yf0 : 1.0-yf0);
				else if( y == iy1 ) yw *= yf1;
				if( yw <= 0.0 ) continue;
				for( x=ix0; x<=ix1; ++x ) {
					double xyw = yw;
					if( x == ix0 ) xyw *= (x==ix1 ? xf1-xf0 : 1.0-xf0);
					else if( x == ix1 ) xyw *= xf1;
					if( xyw <= 0.0 ) continue;
					int idx = ibpp*(x + rx);
					uint8_t *cp = &row[idx];
					r += cp[0]*xyw;
				}
			}
			*bp++ = r + 0.5;
			xf0 = xf1;
			lx = nx;
		}
		yf0 = yf1;
		ly = ny;
	}
}

void write_pbm(uint8_t *tp, int w, int h, const char *fmt, ...)
{
  va_list ap;    va_start(ap, fmt);
  char fn[256];  vsnprintf(fn, sizeof(fn), fmt, ap);
  va_end(ap);
  FILE *fp = !strcmp(fn,"-") ? stdout : fopen(fn,"w");
  if( fp ) {
    fprintf(fp,"P5\n%d %d\n255\n",w,h);
    fwrite(tp,w,h,fp);
    fclose(fp);
  }
}

#endif

int main(int ac,char **av)
{
  Window w, root;
  Display *display;
  GC gc;
  XGCValues gcv;
  XEvent xev;
  XImage *image;
  Visual *visual;
  Screen *screen;
  Atom aMsg;
  char *pixels;
  int n, depth, depthfactor;
  int client_msg;

  setbuf(stdout,NULL);
  if( db.open(av[1]) ) {
    fprintf(stderr,"cant open db: %s\n",av[1]);
    exit(1);
  }
  int clip_id = atoi(av[2]);
  if( db.clip_set.FindId(clip_id) ) {
    printf("cant open clip %d\n",clip_id);
    exit(1);
  }
  double framerate = db.clip_set.Framerate();
  int ret = TimelineLoc::ikey_Sequences(db.timeline,clip_id,0).Locate();
  if( ret || clip_id != (int)db.timeline.Clip_id() ) {
    printf("cant access timeline of clip %d\n",clip_id);
    exit(1);
  }
  int frame_no = 0, frame_id = -1;
  uint8_t *dat = 0;

  display = XOpenDisplay(NULL);
  if( display == NULL ) {
    fprintf(stderr,"cant open display\n");
    exit(1);
  }

  root = RootWindow(display,0);
  screen = DefaultScreenOfDisplay(display);
  depth = DefaultDepthOfScreen(screen);
  visual = DefaultVisualOfScreen(screen);
  client_msg = 0;
 
  switch( visual->c_class ) {
  case TrueColor:
    depthfactor = 4;
    break;  
  default:
    fprintf(stderr,"bad visual class %d\n",visual->c_class);
    exit(1);
  }

  w = XCreateSimpleWindow(display, root, 0, 0, WIDTH, HEIGHT,
          0, 0, WhitePixelOfScreen(screen));

  pixels = (char *) malloc(HEIGHT*WIDTH*depthfactor);
  image = XCreateImage( display,  visual, depth, ZPixmap, 0,
                        pixels, WIDTH, HEIGHT, 8, WIDTH*depthfactor);
 
  XSelectInput(display, w, ExposureMask|StructureNotifyMask|ButtonPressMask);
  XMapWindow(display, w);
  XFlush(display);
  aMsg = XInternAtom(display,"dbtv",False);
  gcv.function = GXcopy;
  gcv.foreground = BlackPixelOfScreen(DefaultScreenOfDisplay(display));
  gcv.line_width = 1;
  gcv.line_style = LineSolid;
  n = GCFunction|GCForeground|GCLineWidth|GCLineStyle;
  gc = XCreateGC(display,w,n,&gcv);
  double start_time = the_time();

  while( done < 0 ) {
    XNextEvent(display,&xev);
    /* printf("xev.type = %d/%08x\n",xev.type,xev.xany.window); */
    switch( xev.type ) {
    case ButtonPress:
      if( xev.xbutton.window != w ) continue;
      if( xev.xbutton.button != Button1 ) {
        done = 1;
        break;
      }
      done = 0;
      break;

    case Expose:
      if( client_msg != 0 ) break;
      client_msg = 1; /* fall through */
      start_time = the_time();

    case ClientMessage:
      frame_id = db.timeline.Frame_id();
      if( db.video_frame.FindId(frame_id) ) {
        printf("cant access frame %d (id %d) in clip %d\n",
          frame_no, frame_id, clip_id);  done = 1;  break;
      }
      dat = db.video_frame._Frame_data();
      //write_pbm(dat, SWIDTH,SHEIGHT, "%s/f%05d.pbm",av[2], fid);
      uint8_t img[WIDTH*HEIGHT];
      scale0(dat,SWIDTH,SHEIGHT, img,WIDTH,HEIGHT);
//      scale1(dat, SWIDTH,SHEIGHT, 0,0,SWIDTH,SHEIGHT,
//             img, WIDTH, HEIGHT,  0,0,WIDTH, HEIGHT);
      loadImage(0,0, image, img, WIDTH, HEIGHT);
      while( (int)(1000 * (frame_no/framerate - (the_time()-start_time))) > 0 ) {
        usleep(1000);
      }
      ++frame_no;
      XPutImage( display, w, gc, image, 0, 0, 0, 0, WIDTH, HEIGHT);
      XFlush(display);
      ret = TimelineLoc::rkey_Sequences(db.timeline).Next();
      if( ret || clip_id != (int)db.timeline.Clip_id() ) { done = 1;  break; }
      clientMsg(display,w,aMsg,32);
      break;

    case DestroyNotify:
      done = 0;
      break;
    }
  }

  XCloseDisplay(display);
  return done;
}

