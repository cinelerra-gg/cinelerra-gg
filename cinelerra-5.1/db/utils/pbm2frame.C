#include<stdio.h>
#include<stdarg.h>
#include<time.h>
#include<math.h>

#include "tdb.h"
#include "s.C"

#include "../../cinelerra/mediadb.inc"

double errlmt = 8;
int dist = 1024;
#define SWIDTH 80
#define SHEIGHT 45
#define SFRM_SZ (SWIDTH*SHEIGHT)

double
mean(uint8_t *dat, int n)
{
  int s = 0;
  for( int i=0; i<n; ++i, ++dat ) s += *dat;
  return (double)s / n;
}

double
variance(uint8_t *dat, double m, int n)
{
  double ss = 0, s = 0;
  for( int i=0; i<n; ++i, ++dat ) {
    double dx = *dat-m;
    s += dx;  ss += dx*dx;
  }
  return (ss - s*s / n) / (n-1);
}

double
std_dev(uint8_t *dat, double m, int n)
{
  return sqrt(variance(dat,m,n));
}

double 
center(uint8_t *dat, int n, int stride)
{
  int s = 0, ss = 0;
  for( int i=0; i<n; dat+=stride ) { s += *dat;  ss += ++i * *dat; }
  return s > 0 ? (double)ss / s : n / 2.;
}

void
centroid(uint8_t *dat, int w, int h, double *xx, double *yy)
{
        double x = 0, y = 0;
        uint8_t *dp = dat;
        for( int i=h; --i>=0; dp+=w ) x += center(dp, w, 1);
        for( int i=w; --i>=0; ) y += center(dat+i, h, w);
        *xx = x / h;  *yy = y / w;
}


static inline int clip(int v, int mn, int mx)
{
  return v<mn ? mn : v>mx ? mx : v;
}

void show_diff(uint8_t *kp0, uint8_t *kp1)
{
  int w=SWIDTH, h=SHEIGHT;
  uint8_t tgt[3*SFRM_SZ];
  uint8_t *ap = tgt, *bp = tgt+1*SFRM_SZ, *dp = tgt+2*SFRM_SZ;
  memcpy(ap, kp0, SFRM_SZ);  memcpy(bp, kp1, SFRM_SZ);
  for( int i=SFRM_SZ; --i>=0; ++dp,++ap,++bp ) *dp = clip(*ap-*bp+128, 0 ,255);
  FILE *fp = popen("xv -geom 600x1125 -","w");
  fprintf(fp,"P5\n%d %d\n255\n",w,3*h);
  fwrite(tgt,w,3*h,fp);
  fclose(fp);
}

int64_t compare(uint8_t *ap, uint8_t *bp, int sz)
{
  int64_t v = 0;
  for( int i=sz; --i>=0; ++ap,++bp ) v += fabs(*ap-*bp);
  return v;
}

double fmean, fsd, fcx, fcy;
uint8_t frm[SFRM_SZ];
int video_compares = 0, frame_compares = 0;

int64_t diff(uint8_t *a, uint8_t *b, int w, int h, int dx, int dy, int bias)
{
  int axofs = dx < 0 ? 0 : dx;
  int ayofs = w * (dy < 0 ? 0 : dy);
  int aofs = ayofs + axofs;
  int bxofs = dx < 0 ? -dx : 0;
  int byofs = w * (dy < 0 ? -dy : 0);
  int bofs = byofs + bxofs;
  uint8_t *ap = a + aofs, *bp = b + bofs;
  int ierr = 0, ww = w-abs(dx), hh = h-abs(dy);
  for( int y=hh; --y>=0; ap+=w, bp+=w ) {
    a = ap;  b = bp;
    for( int x=ww; --x>=0; ++a, ++b ) { ierr += abs(*a-bias - *b); }
  }
  return ierr;
}

int64_t 
compare(Db::ObjectLoc &loc)
{
++video_compares;
        Video_frameLoc &frame = *(Video_frameLoc*)&loc;
        double dm = frame.Frame_mean()-fmean;
        if( fabs(dm) > MEDIA_MEAN_ERRLMT ) return LONG_MAX;
        double ds = frame.Frame_std_dev()-fsd;
        if( fabs(ds) > MEDIA_STDDEV_ERRLMT ) return LONG_MAX;
        double dx = frame.Frame_cx()-fcx;
        if( fabs(dx) > MEDIA_XCENTER_ERRLMT ) return LONG_MAX;
        double dy = frame.Frame_cy()-fcy;
        if( fabs(dy) > MEDIA_YCENTER_ERRLMT ) return LONG_MAX;
        uint8_t *dat = frame._Frame_data();
        //int64_t err = cmpr(frm, dat, SFRM_SZ);
        int64_t err = diff(frm, dat, SWIDTH, SHEIGHT, lround(dx), lround(dy), lround(dm));
//show_diff(frm, frame.Frame_data());
++frame_compares;
	return err;
}


int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  FILE *fp = fopen(av[2],"r");
  for( int i=3; --i>=0; ) {
    for( int ch=fgetc(fp); ch>=0 && ch!='\n'; ch=fgetc(fp) );
  }
  
  int w = SWIDTH, h = SHEIGHT;
  fread(frm,1,SFRM_SZ,fp);

  fmean = mean(frm, SFRM_SZ);
  fsd = std_dev(frm, fmean, SFRM_SZ);
  centroid(frm, w, h, &fcx, &fcy);
  double fkey = fmean;
  if( (ret = Video_frameLoc::ikey_Frame_weight(db.video_frame, fkey).Locate()) != 0 ) {
    printf(" not found, ret = %d\n",ret);
    return 1;
  }
  uint32_t lid = db.video_frame.id(), lfid = lid;
  printf("frm %d mean %f, std_dev %f, cxy %f+%f = %f\n", lid, fmean, fsd, fcx, fcy, fkey);
  int64_t lerr = compare(db.video_frame);
  int64_t pixlmt = SFRM_SZ*errlmt;
  if( lerr < pixlmt )
    printf("+  0frame %d / %f\n", db.video_frame.id(),(double)lerr/SFRM_SZ);
//show_diff(frm, db.video_frame._Frame_data());

  Video_frameLoc prev(db.video_frame), next(db.video_frame);
  Video_frameLoc::rkey_Frame_weight prev_key(prev), next_key(next);
  Db::pgRef next_loc;  next_key.NextLoc(next_loc);

  for( int i=1; i<=dist; ++i ) {
    if( !prev_key.Locate(Db::keyLT) ) {
      int64_t err = compare(prev);
      uint32_t prev_id = prev.id();
      if( err < pixlmt && lfid != prev_id )
        printf("-%3dframe %d / %f\n",i, prev_id,(double)err/SFRM_SZ);
      if( lerr > err ) { lerr = err;  lid = prev_id; }
    }
    if( !next_key.Next(next_loc) ) {
      int64_t err = compare(next);
      uint32_t next_id = next.id();
      if( err < pixlmt && lfid != next_id )
        printf("+%3dframe %d / %f\n",i, next_id,(double)err/SFRM_SZ);
      if( lerr > err ) { lerr = err;  lid = next_id; }
    }
  }
  printf("%c frame %d / %f, compares %d/%d = %f\n",
    lerr<pixlmt ? '*' : '#', lid, (double)lerr/SFRM_SZ,
    frame_compares, video_compares, (double)frame_compares/video_compares);

  db.close();
  return 0;
}

