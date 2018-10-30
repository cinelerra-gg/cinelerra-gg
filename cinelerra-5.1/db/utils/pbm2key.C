#include<stdio.h>
#include<stdarg.h>
#include<time.h>
#include<math.h>

#include "tdb.h"
#include "s.C"

#define SWIDTH 80
#define SHEIGHT 45

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
  
  int w = SWIDTH, h = SHEIGHT, sfrm_sz = w * h;
  uint8_t dat[sfrm_sz];
  fread(dat,1,sfrm_sz,fp);

  double mn = mean(dat,sfrm_sz);
  double sd = std_dev(dat,mn,sfrm_sz);
  double cx, cy;  centroid(dat, w, h, &cx, &cy);
  double moment = cx + cy;
  if( (ret = Video_frameLoc::ikey_Frame_weight(db.video_frame, mn).Locate()) != 0 ) {
    printf(" not found, ret = %d\n",ret);
    return 1;
  }
  printf(" id %d, mean %f-%f=%f, std_dev %f-%f=%f, "
      " cx %f-%f=%f, cy %f-%f=%f, moment %f-%f=%f\n", db.video_frame.id(),
    mn, db.video_frame.Frame_mean(), mn-db.video_frame.Frame_mean(),
    sd, db.video_frame.Frame_std_dev(), sd-db.video_frame.Frame_std_dev(), 
    cx, db.video_frame.Frame_cx(), cx-db.video_frame.Frame_cx(), 
    cy, db.video_frame.Frame_cy(), cy-db.video_frame.Frame_cy(), 
    moment, db.video_frame.Frame_moment(), moment-db.video_frame.Frame_moment());
  if( (ret = Video_frameLoc::ikey_Frame_center(db.video_frame, moment).Locate()) != 0 ) {
    printf(" not found, ret = %d\n",ret);
    return 1;
  }
  printf(" id %d, mean %f-%f=%f, std_dev %f-%f=%f, "
      " cx %f-%f=%f, cy %f-%f=%f, moment %f-%f=%f\n", db.video_frame.id(),
    mn, db.video_frame.Frame_mean(), mn-db.video_frame.Frame_mean(),
    sd, db.video_frame.Frame_std_dev(), sd-db.video_frame.Frame_std_dev(), 
    cx, db.video_frame.Frame_cx(), cx-db.video_frame.Frame_cx(), 
    cy, db.video_frame.Frame_cy(), cy-db.video_frame.Frame_cy(), 
    moment, db.video_frame.Frame_moment(), moment-db.video_frame.Frame_moment());
  db.close();
  return 0;
}

