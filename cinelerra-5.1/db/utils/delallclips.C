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

int del_clip_set(theDb *db, int clip_id)
{
printf("del clip %d\n",clip_id);
	if( db->clip_set.FindId(clip_id) ) return 1;
	db->clip_set.Destruct();
	db->clip_set.Deallocate();

	if( Clip_viewsLoc::ikey_Clip_access(db->clip_views,clip_id).Find() ) return 1;
	db->clip_views.Destruct();
	db->clip_views.Deallocate();

	while( !TimelineLoc::ikey_Sequences(db->timeline,clip_id,0).Locate() ) {
		if( clip_id != (int)db->timeline.Clip_id() ) break;
		int frame_id = db->timeline.Frame_id();
		db->timeline.Destruct();
		db->timeline.Deallocate();
		if( !TimelineLoc::ikey_Timelines(db->timeline, frame_id).Locate() &&
			frame_id == (int)db->timeline.Frame_id() ) continue;
		if( db->video_frame.FindId(frame_id) ) continue;
		db->video_frame.Destruct();
		db->video_frame.Deallocate();
	}
	return 0;
}

int main(int ac, char **av)
{
  int ret;  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  while( !(ret=db.clip_set.FirstId()) ) {
    printf("clip %d %f %d\n", db.clip_set.id(), db.clip_set.Position(), db.clip_set.Frames());
    del_clip_set(&db, db.clip_set.id());
  }
  if( !(ret=db.video_frame.FirstId()) ) do {
    printf("frame %d\n", db.video_frame.id());
    if( ac > 2 ) {
      uint8_t *dat = db.video_frame._Frame_data();
      write_pbm(dat,80,45,"%s/f%05d.pbm",av[2],db.video_frame.id());
    }
  } while( !(ret=db.video_frame.NextId()) );

  db.close();
  return 0;
}

