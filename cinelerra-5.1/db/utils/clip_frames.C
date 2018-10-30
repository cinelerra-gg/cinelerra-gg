#include<stdio.h>
#include<stdarg.h>
#include<time.h>

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

  if( !(ret=db.clip_set.FindId(atoi(av[2]))) ) {
    int cid = db.clip_set.id();
//    printf("clip_set %d, frames %d", cid, db.clip_set.Frames());
//    printf(" prefix %d/suffix %d\n", db.clip_set.Prefix_size(),db.clip_set.Suffix_size());
    TimelineLoc::ikey_Sequences ikey(db.timeline,cid,0);
    if( (ret=ikey.Locate()) || (int)db.timeline.Clip_id() != cid ) {
      printf("missed seq for clip_set %d\n",cid);
      exit(1);
    }
    const char *apath =  db.clip_set._Asset_path();
    const char *cp = strrchr(apath,'/');
    if( cp ) apath = cp+1;
    int fid = -1, n = 0;
    TimelineLoc::rkey_Sequences rkey(db.timeline);
    do {
      if( (int)db.timeline.Clip_id() != cid ) break;
      //if( fid == (int)db.timeline.Frame_id() ) continue;
      int seq = db.timeline.Sequence_no();
      fid = db.timeline.Frame_id();
      if( (ret=db.video_frame.FindId(fid)) ) {
        printf("missed frame %d(%d) in clip_set %d\n", seq,fid,cid);
      }
      printf("%4d %4d %6d %f %f %f %f %f\n", n, seq, db.video_frame.id(),
        db.video_frame.Frame_mean(), db.video_frame.Frame_std_dev(),
        db.video_frame.Frame_cx(), db.video_frame.Frame_cy(),
        db.video_frame.Frame_moment());
      if( ac > 3 ) {
        int w = 80, h = 45;
	uint8_t *dat = db.video_frame._Frame_data();
        write_pbm(dat,w,h,"%s/c%04df%03d.pbm",av[3],cid,n);
      }
      ++n;
    } while( !(ret=rkey.Next()) );
  }

  db.close();
  return 0;
}

