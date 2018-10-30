
#include<stdio.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
#include "s.C"

int clip(int v) { return v > 255 ? 255 : v < 0 ? 0 : v; }

int main(int ac, char **av)
{
  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);
  int aid = atoi(av[2]);
  int bid = atoi(av[3]);

  Video_frameLoc aframe(db.video_frame);
  Video_frameLoc bframe(db.video_frame);
  if( aframe.FindId(aid) ) {
    printf("cant access frame %d\n", aid);
    exit(1);
  }
  if( bframe.FindId(bid) ) {
    printf("cant access frame %d\n", aid);
    exit(1);
  }

  uint8_t *adat = aframe._Frame_data(), *bdat = bframe._Frame_data();
  FILE *sfp = stdout;
  int w = 80, h = 45;
  int n = 0, m = 0;
  if( ac > 4 ) {
    FILE *fp;
    if( !strcmp(av[4],"-") ) {
      fp = stdout;
      sfp = stderr;
    }
    else
      fp = fopen(av[4],"w");
    if( fp ) {
      fprintf(fp,"P5\n%d %d\n255\n",w,h);
      uint8_t *ap = adat, *bp = bdat;
      for( int i=w*h; --i>=0; ++ap, ++bp ) {
        int d = *ap-*bp;
        m += d;
        if( d < 0 ) d = -d;
        n += d;
        putc(clip(*ap-*bp+128), fp);
      }
      if( fp != stdout ) fclose(fp);
    }
    else {
      perror(av[4]);
      exit(1);
    }
  }
  else {
    uint8_t *ap = adat, *bp = bdat;
    int n = 0, m = 0;
    for( int i=w*h; --i>=0; ++ap, ++bp ) {
      int d = *ap-*bp;
      m += d;
      if( d < 0 ) d = -d;
      n += d;
    }
  }
  fprintf(sfp, "%d %d\n",n,m);
  db.close();
  return 0;
}
