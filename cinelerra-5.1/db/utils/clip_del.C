#include<stdio.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
#include "s.C"

int del_clip_set(theDb *db, int clip_id)
{
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
  setbuf(stdout,0);
  theDb db;
  db.open(av[1]);
  //db.access(av[1], 34543, 0);
  if( !db.opened() || db.error() ) exit(1);

  del_clip_set(&db,atoi(av[2]));

  db.commit();
  db.close();
  return 0;
}

