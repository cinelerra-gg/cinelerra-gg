#include<stdio.h>
#include<string.h>
#include<stdarg.h>

#include "tdb.h"
#include "s.C"

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

int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  unsigned int id = atoi(av[2]);
  if( (ret=db.video_frame.FindId(id)) != 0 ) {
    printf(" not found, ret = %d\n",ret);
    return 1;
  }
  uint8_t *dat = db.video_frame._Frame_data();
  write_pbm(dat,80,45,"%s",av[3]);
  db.close();
  return 0;
}

