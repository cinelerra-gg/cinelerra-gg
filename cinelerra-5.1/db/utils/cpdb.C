#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#include "../s.C"

using namespace std;

// c++ -ggdb cpydb.C

double
runtime(struct timeval *st)
{
  struct timeval tv;
  gettimeofday(&tv,0);
  double dt = (tv.tv_sec - st->tv_sec) +
    (tv.tv_usec - st->tv_usec) / 1000000.;
  return dt;
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
  setbuf(stdout,0);
  setbuf(stderr,0);

  theDb idb;
  Db odb;
  if( ac < 3 ) { printf("usage: %s in.db out.db\n",av[0]); exit(1); }
  const char *ifn = av[1];
  if( idb.open(ifn) ) { perror(ifn); exit(1); }
  const char *ofn = av[2];
  remove(ofn);
  int ofd = open(ofn,O_RDWR+O_CREAT+O_TRUNC,0666);
  if( ofd < 0 ) { perror(ofn); return 1; }
  if( odb.make(ofd) ) { perror(ofn); exit(1); }
  struct timeval st;
  gettimeofday(&st,0);

#if 1
  int next_id = 0;
  while( !idb.clip_set.LocateId(Db::keyGE,next_id) ) {
    next_id = idb.clip_set.id() + 1;
    if( Clip_viewsLoc::ikey_Clip_access(idb.clip_views,idb.clip_set.id()).Find() ) {
      printf("clip %d, missed\n", idb.clip_set.id()); continue;
    }
    time_t t = (time_t) idb.clip_views.Access_time();
    long dt = st.tv_sec - t;
    // 3 weeks + access count days
    if( dt < 3*7*24*60*60 + (long)(idb.clip_views.Access_count()*24*60*60) )
      continue;
    del_clip_set(&idb, idb.clip_set.id());
  }
#endif

  odb.copy(&idb,idb.objects);
  odb.commit(1);
  odb.commit(1);
  odb.close();
  idb.close();

  double secs = runtime(&st);
  printf("%f secs\n",secs);
  return 0;
}

