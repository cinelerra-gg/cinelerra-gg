#include "libzmpeg3.h"

zstrack_t::
strack_t(int zid, video_t *vid)
{
  id = zid;
  video = vid;
}

zstrack_t::
~strack_t()
{
  if( offsets ) delete [] offsets;
}

zstrack_t::
strack_t(zstrack_t &strack)
{
  id = strack.id;
  video = strack.video;
  allocated_offsets = strack.allocated_offsets;
  total_offsets = strack.total_offsets;
  offsets = new int64_t[allocated_offsets];
  for( int i=0; i<total_offsets; ++i )
    offsets[i] = strack.offsets[i];
}

void zstrack_t::
extend_offsets()
{
  if( allocated_offsets <= total_offsets ) {
    long new_allocation = ZMAX(2*total_offsets, 16);
    int64_t *new_offsets = new int64_t[new_allocation];
    for( int i=0; i<total_offsets; ++i )
      new_offsets[i] = offsets[i];
    delete [] offsets;
    offsets = new_offsets;
    allocated_offsets = new_allocation;
  }
}

void zstrack_t::
append_subtitle_offset(int64_t program_offset)
{
  extend_offsets();
  offsets[total_offsets++] = program_offset;
}

int zstrack_t::
append_subtitle(zsubtitle_t *subtitle, int lock)
{
  int ret = 1;
  rwlock.write_enter(lock);
  if( total_subtitles < MAX_SUBTITLES ) {
    subtitles[total_subtitles++] = subtitle;
    subtitle->done = 1;
    ret = 0;
  }
  rwlock.write_leave(lock);
  return ret;
}

void zstrack_t::
del_subtitle(subtitle_t *subtitle, int lock)
{
  rwlock.write_enter(lock);
  int i = 0;
  while( i<total_subtitles && subtitles[i]!=subtitle ) ++i;
  if( i < total_subtitles ) {
    while( ++i<total_subtitles ) subtitles[i-1] = subtitles[i];
    --total_subtitles;
  }
  subtitle->draw = subtitle->done = -1;
  rwlock.write_leave(lock);
}

void zstrack_t::
del_all_subtitles()
{
  rwlock.write_enter();
  for( int i=0; i<total_subtitles; ++i ) {
    subtitle_t *sp = subtitles[i];
    sp->draw = sp->done = -1;
    if( sp->force < 0 ) delete sp;
  }
  total_subtitles = 0;
  rwlock.write_leave();
}

void zstrack_t::
del_subtitle(int idx, int lock)
{
  del_subtitle(subtitles[idx], lock);
}

