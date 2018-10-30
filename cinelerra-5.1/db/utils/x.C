#include<stdio.h>
#include<stdarg.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<math.h>

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

class Scale {
  int iw, ih;
  double rx, ry, rw, rh;
  int ix0, ix1, iy0, iy1, rsh;
  double xf0, xf1, yf0, yf1, pw;
  uint8_t *idat, *odat;
  uint8_t *row(int y) { return idat + (iw<<rsh) * (y>>rsh); }

  inline double scalex(uint8_t *rp, double v, double yw) {
    int x = ix0;  rp += x;
    v += yw*xf0 * *rp++;
    while( ++x < ix1 ) v += yw * *rp++;
    v += yw*xf1 * *rp++;
    return v;
  }

  inline double scalexy() {
    double yw, v = 0;
    int y = iy0;
    if( (yw = pw * yf0) > 0 ) v = scalex(row(y), v, yw);
    while( ++y < iy1 ) v = scalex(row(y), v, pw);
    if( (yw = pw * yf1) > 0 ) v = scalex(row(y), v, yw);
    return v;
  }
public:
  Scale(uint8_t *idata, int type, int iw, int ih,
                double rx, double ry, double rw, double rh) {
    rsh = type ? 1 : 0;
    this->idat = idata;
    this->iw = iw;  this->ih = ih;
    this->rx = rx;  this->ry = ry;
    this->rw = rw;  this->rh = rh;
  }
  void scale(uint8_t *odata, int ow, int oh, int sx, int sy, int sw, int sh);
};

void Scale::
scale(uint8_t *odata, int ow, int oh, int sx, int sy, int sw, int sh)
{
  pw = (double)(ow * oh) / (rw * rh);
  odat = odata + sy*ow + sx;
  double ly = 0.0;

  for( int dy=1; dy<=sh; ++dy, odat+=ow ) {
    iy0 = (int) ly;  yf0 = 1.0 - (ly-iy0);
    double ny = (dy * rh) / oh + ry;
    iy1 = (int) ny;  yf1 = ny-iy1;
    uint8_t *bp = odat;
    double lx = rx;

    for( int dx=1; dx<=sw; ++dx ) {
      ix0 = (int)lx;  xf0 = 1.0 - (lx-ix0);
      double nx = (dx*rw) / ow + rx;
      ix1 = (int)nx;  xf1 = nx-ix1;
      *bp++ = scalexy();
      xf0 = 1.0-xf1;  lx = nx;
    }
    yf0 = 1.0 - yf1;  ly = ny;
  }
}


double mean(uint8_t *dat, int n, int stride)
{
  int s = 0;
  for( int i=0; i<n; ++i, dat+=stride ) s += *dat;
  return (double)s / n;
}

double variance(uint8_t *dat, double m, int n, int stride)
{
  double ss = 0, s = 0;
  for( int i=0; i<n; ++i, dat+=stride ) {
    double dx = *dat-m;
    s += dx;  ss += dx*dx;
  }
  return (ss - s*s / n) / (n-1);
}

double std_dev(uint8_t *dat, double m, int n, int stride)
{
	return sqrt(variance(dat, m, n, stride));
}

double centroid(uint8_t *dat, int n, int stride)
{
  int s = 0, ss = 0;
  for( int i=0; i<n; dat+=stride ) { s += *dat;  ss += ++i * *dat; }
  return s > 0 ? (double)ss / s : n / 2.;
}

void deviation(uint8_t *a, uint8_t *b, int sz, int margin, int stride, double &dev, int &ofs)
{
  double best = 1e100;  int ii = -1;
  for( int i=-margin; i<=margin; ++i ) {
    int aofs = i < 0 ? 0 : i*stride;
    int bofs = i < 0 ? -i*stride : 0;
    uint8_t *ap = a + aofs, *bp = b + bofs;
    int ierr = 0; int n = sz - abs(i);
    for( int j=n; --j>=0; ap+=stride,bp+=stride ) ierr += abs(*ap - *bp);
    double err = (double)ierr / n;
    if( err < best ) { best = err;  ii = i; }
  }
  dev = best;
  ofs = ii;
}

void diff(uint8_t *a, uint8_t *b, int w, int h, int dx, int dy, int bias, double &dev)
{
  int axofs = dx < 0 ? 0 : dx;
  int ayofs = w * (dy < 0 ? 0 : dy);
  int aofs = ayofs + axofs;
  int bxofs = dx < 0 ? -dx : 0;
  int byofs = w * (dy < 0 ? -dy : 0);
  int bofs = byofs + bxofs;
  int sz = w * h;
  uint8_t *ap = a + aofs, *bp = b + bofs;
  int ierr = 0, ww = w-abs(dx), hh = h-abs(dy), ssz = ww * hh;
uint8_t dif[ssz], *dp = dif;
  for( int y=hh; --y>=0; ap+=w, bp+=w ) {
    a = ap;  b = bp;
    for( int x=ww; --x>=0; ++a, ++b ) { ierr += abs(*a-bias - *b); *dp++ = *a-bias-*b+128; }
  }
write_pbm(dif, ww, hh, "/tmp/x");
  dev = (double)ierr / ssz;
}


int main(int ac, char **av)
{
  char line[120];
  FILE *afp = fopen(av[1],"r");
  FILE *bfp = fopen(av[2],"r");
  fgets(line,sizeof(line),afp);
  fgets(line,sizeof(line),afp);
  int aw, ah;  sscanf(line,"%d %d",&aw, &ah);
  fgets(line,sizeof(line),afp);
  fgets(line,sizeof(line),bfp);
  fgets(line,sizeof(line),bfp);
  int bw, bh;  sscanf(line,"%d %d",&bw, &bh);
  if( aw != bw || ah != bh ) exit(1);
  int w = aw, h = ah, sz = w * h;
  fgets(line,sizeof(line),bfp);
  uint8_t a[sz], b[sz];
  uint8_t *ap = a, *bp = b;
  int ach, bch, n = 0;
  while( (ach=getc(afp))>=0 && (bch=getc(bfp))>=0 ) {
    *ap++ = ach;  *bp++ = bch;  if( ++n >= sz ) break;
  }
  double dev;
  for( int dy=-2; dy<=2; ++dy ) {
    printf(" dy=%d: ", dy);
    for( int dx=-2; dx<=2; ++dx ) {
      diff(a, b, w, h, dx, dy, 0, dev),
      printf(" dx=%d/dev %f ", dx, dev);
    }
    printf("\n");
  }
  uint8_t *ax = a + sz/2, *ay = a + w/2;
  uint8_t *bx = b + sz/2, *by = b + w/2;
  double xam = mean(ax, w, 1), xbm = mean(bx, w, 1);
  double yam = mean(ay, h, w), ybm = mean(by, h, w);
  double am = (xam + yam) / 2, bm = (xbm + ybm) / 2;
  int bias = am-bm;
  printf("xam/yam %f/%f, xbm/ybm %f/%f, am/bm %f/%f, bias %d\n",xam,yam,xbm,ybm,am,bm,bias);
  double xasd = std_dev(ax, xam, w, 1), xbsd = std_dev(bx, xbm, w, 1);
  double yasd = std_dev(ay, yam, h, w), ybsd = std_dev(by, ybm, h, w);
  double asd = (xasd + yasd) / 2, bsd = (xbsd + ybsd) / 2;
  printf("xasd/yasd %f/%f, xbsd/ybsd %f/%f, asd/bsd %f/%f\n",xasd,yasd,xbsd,ybsd,asd,bsd);
  double xdev, ydev;  int xofs, yofs;
  deviation(ax, bx, w, 5, 1, xdev, xofs);
  deviation(ay, by, h, 5, w, ydev, yofs);
  diff(a, b, w, h, xofs, yofs, bias, dev);
  printf("xdev/ofs %f/%d, ydev/ofs %f/%d, dev %f\n",xdev,xofs,ydev,yofs,dev);

  double cax = centroid(ax, w, 1),  cay = centroid(ay, h, w);
  double cbx = centroid(bx, w, 1),  cby = centroid(by, h, w);
  double dx = cax - cbx, dy = cay - cby;
  printf("cax/cay %f/%f,  cbx/cby %f/%f, dx/dy %f/%f", cax, cay, cbx, cby, dx, dy);
  int ww = w+7, hh = h+7;  uint8_t aa[ww*hh];  memset(aa,0,ww*hh);
  ap = a;  bp = aa + 3*ww + 3;  uint8_t c[sz];
  for( int y=0; y<h; ++y, bp+=7 ) for( int x=0; x<w; ++x ) *bp++ = *ap++;
  Scale(aa,0, ww,hh, 3+dx,3+dy,w,h).scale(a, w,h, 0,0,w,h);
write_pbm(a,w,h,"/tmp/a");
  diff(a, b, w, h, 0, 0, 0, dev);
  printf(" dev %f\n", dev);
  return 0;
}


