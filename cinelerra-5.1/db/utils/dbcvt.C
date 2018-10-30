#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdarg.h>
#include<time.h>

#include "tdb.h"
namespace old {
#include "../x.C"
};
#include "s.C"

class new_Video_frame : public Video_frameLoc {
public:
  int copy(Db::ObjectLoc &o);
  new_Video_frame(Db::Entity &e) : Video_frameLoc(e) {}
  ~new_Video_frame() {}
};

int new_Video_frame::
copy(Db::ObjectLoc &o)
{
  old::Video_frameLoc &t = *(old::Video_frameLoc *)&o;
  Allocate();
  Frame_mean( t.Frame_mean() );
  Frame_mean( t.Frame_mean() );
  Frame_std_dev( t.Frame_std_dev() );
  Frame_cx( t.Frame_cx() );
  Frame_cy( t.Frame_cy() );
  Frame_moment( t.Frame_moment() );
  Frame_data(t._Frame_data(), t.size_Frame_data());
  return 0;
}

class new_Timeline : public TimelineLoc {
public:
  int copy(Db::ObjectLoc &o);
  new_Timeline(Db::Entity &e) : TimelineLoc(e) {}
  ~new_Timeline() {}
};

int new_Timeline::
copy(Db::ObjectLoc &o)
{
  old::TimelineLoc &t = *(old::TimelineLoc *)&o;
  Allocate();
  Clip_id( t.Clip_id() );
  Sequence_no( t.Sequence_no() );
  Frame_id( t.Frame_id() );
  Group( t.Group() );
  Time_offset( t.Time_offset() );
  return 0;
}

class new_Clip_set : public Clip_setLoc {
public:
  int copy(Db::ObjectLoc &o);
  new_Clip_set(Db::Entity &e) : Clip_setLoc(e) {}
  ~new_Clip_set() {}
};

int new_Clip_set::
copy(Db::ObjectLoc &o)
{
  old::Clip_setLoc &t = *(old::Clip_setLoc *)&o;
  Allocate();
  int title_sz = t.size_Title();
  char title[1024];  memcpy(title, t._Title(), title_sz);
  if( !title_sz || title[title_sz-1] != 0 ) title[title_sz++] = 0;
  Title( title, title_sz );
  int apath_sz = t.size_Asset_path();
  char apath[1024];  memcpy(apath, t._Asset_path(), apath_sz);
  if( !apath_sz || apath[apath_sz-1] != 0 ) apath[apath_sz++] = 0;
  Asset_path( apath, apath_sz );
  Position( t.Position() );
  int tid = t.id();
  Clip_setLoc aloc(entity);
  if( Clip_setLoc::ikey_Clip_path_pos(aloc, apath, Position(), tid).Find() ) {
    Clip_setLoc::rkey_Clip_path_pos rkey(*this);
    if( entity->index("Clip_path_pos")->Insert(rkey,&tid) ) {
      printf("err inserting Clip_path_pos for id %d\n", tid);
    }
  }
  Framerate( t.Framerate() );
  Average_weight( t.Average_weight() );
  Frames( t.Frames() );
  Prefix_size( t.Prefix_size() );
  Suffix_size( t.Suffix_size() );
  Weights(t._Weights(), t.size_Weights());
  System_time( t.System_time() );
  Creation_time( t.Creation_time() );
  return 0;
}

class new_Clip_views : public Clip_viewsLoc {
public:
  int copy(Db::ObjectLoc &o);
  new_Clip_views(Db::Entity &e) : Clip_viewsLoc(e) {}
  ~new_Clip_views() {}
};

int new_Clip_views::
copy(Db::ObjectLoc &o)
{
  old::Clip_viewsLoc &t = *(old::Clip_viewsLoc *)&o;
  Allocate();
  Access_clip_id( t.Access_clip_id() );
  Access_time( t.Access_time() );
  Access_count( t.Access_count() );
  return 0;
}



int main(int ac, char **av)
{
  setbuf(stdout,0);
  if( ac < 3 ) { printf("usage: %s in.db out.db\n",av[0]); exit(1); }
  old::theDb idb;
  const char *ifn = av[1];
  if( idb.open(ifn) || !idb.opened() || idb.error() ) {
    fprintf(stderr,"unable to open idb \"%s\"\n",ifn);  exit(1);
  }
  theDb odb;
  const char *ofn = av[2];
  remove(ofn);
  if( odb.create(ofn) ) {
    fprintf(stderr,"unable to create odb \"%s\"\n",ofn);  exit(1);
  }
  if( odb.open(ofn) || !odb.opened() || odb.error() ) {
    fprintf(stderr,"unable to open odb \"%s\"\n",ofn);  exit(1);
  }

  new_Video_frame new_video_frame(odb.Video_frame);
  new_Timeline new_timeline(odb.Timeline);
  new_Clip_set new_clip_set(odb.Clip_set);
  new_Clip_views new_clip_views(odb.Clip_views);
 
  Db::Objects new_objects = 0;
  new_objects = new Db::ObjectList(new_objects, new_video_frame);
  new_objects = new Db::ObjectList(new_objects, new_timeline);
  new_objects = new Db::ObjectList(new_objects, new_clip_set);
  new_objects = new Db::ObjectList(new_objects, new_clip_views);

  odb.copy(&idb, new_objects);

  odb.commit();
  odb.close();
  idb.close();
  return 0;
}

