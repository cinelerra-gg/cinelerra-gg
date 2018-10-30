#include<stdio.h>
#include<stdint.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include "../guicast/linklist.h"



class HistEq {
        void fit(int *dat, int n, double &a, double &b);
public:
        void eq(unsigned char *dp, int len);
        HistEq() {}
        ~HistEq() {}
};

double mean(uint8_t *dat, int n)
{
  int s = 0;
  for( int i=0; i<n; ++i ) s += *dat++;
  return (double)s / n;
}

double variance(uint8_t *dat, int n, double m=-1)
{
  if( m < 0 ) m = mean(dat, n);
  double ss = 0, s = 0;
  for( int i=0; i<n; ++i ) {
    double dx = dat[i]-m;
    s += dx;  ss += dx*dx;
  }
  return (ss - s*s / n) / (n-1);
}

double std_dev(uint8_t *dat, int n, double m=-1)
{
  return sqrt(variance(dat, n, m));
}

static inline int clip(int v, int mn, int mx)
{
  return v<mn ? mn : v>mx ? mx : v;
}

void HistEq::
fit(int *dat, int n, double &a, double &b)
{
  double st2 = 0, sb = 0;
  int64_t sy = 0;
  for( int i=0; i<n; ++i ) sy += dat[i];
  double mx = (n-1)/2., my = (double)sy / n;
  for( int i=0; i<n; ++i ) {
    double t = i - mx;
    st2 += t * t;
    sb += t * dat[i];
  }
  b = sb / st2;
  a = my - mx*b;
}

void HistEq::
eq(uint8_t *dp, int len)
{
  int hist[256], wts[256], map[256];
  for( int i=0; i<256; ++i ) hist[i] = 0;
  uint8_t *bp = dp;
  for( int i=len; --i>=0; ++bp ) ++hist[*bp];
  int t = 0;
  for( int i=0; i<256; ++i ) { t += hist[i];  wts[i] = t; }
  int lmn = len/20, lmx = len-lmn;
  int mn =   0;  while( mn < 256 && wts[mn] < lmn ) ++mn;
  int mx = 255;  while( mx > mn && wts[mx] > lmx ) --mx;
  double a, b;
  fit(&wts[mn], mx-mn, a, b);
  double r = 256./len;
  double a1 = (a - b*mn) * r, b1 = b * r;
  for( int i=0 ; i<256;  ++i ) map[i] = clip(a1 + i*b1, 0, 255);
  bp = dp;
  for( int i=len; --i>=0; ++bp ) *bp = map[*bp];
}


int main(int ac, char **av)
{
  FILE *ifp = !strcmp(av[1],"-") ? stdin : fopen(av[1],"r");
  FILE *ofp = !strcmp(av[2],"-") ? stdout : fopen(av[2],"w");
  char line[120];
  fgets(line,sizeof(line),ifp);  fputs(line,ofp);
  fgets(line,sizeof(line),ifp);  fputs(line,ofp);
  int w, h;  if( sscanf(line,"%d %d\n",&w,&h) != 2 ) exit(1);
  fgets(line,sizeof(line),ifp);  fputs(line,ofp);
  int len = w*h;
  unsigned char data[len], *bp = data;
  for( int ch, i=len; --i>=0 && (ch=getc(ifp)) >= 0; ++bp ) *bp = ch;
  HistEq().eq(data,len);
  bp = data;
  for( int i=len; --i>=0; ++bp ) putc(*bp,ofp);

  if( ifp != stdin ) fclose(ifp);
  if( ofp != stdout ) fclose(ofp);
  return 0;
}

