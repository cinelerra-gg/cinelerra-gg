#include "libzmpeg3.h"

zvtrack_t::
vtrack_t(zmpeg3_t *zsrc, int custom_id, demuxer_t *demux, int no)
{
  demuxer = new demuxer_t(zsrc, 0, this, custom_id);
  frame_cache = new cache_t();
  if( zsrc->seekable ) {
    demux->copy_titles(demuxer);
  }

  current_position = 0;
  number = no;
  pid = custom_id;
  pts_origin = -1.;
  reset_pts();

// Copy pointers
  if( zsrc->frame_offsets ) {
    frame_offsets = zsrc->frame_offsets[number];
    total_frame_offsets = zsrc->total_frame_offsets[number];
    keyframe_numbers = zsrc->keyframe_numbers[number];
    total_keyframe_numbers = zsrc->total_keyframe_numbers[number];
    demuxer->stream_end = zsrc->video_eof[number];
  }
}

zvtrack_t *zmpeg3_t::
new_vtrack_t(int custom_id, demuxer_t *demux, int number)
{
  vtrack_t *new_vtrack = new vtrack_t(this,custom_id,demux,number);

/* Get information about the track here. */
  new_vtrack->video = new_video_t(new_vtrack);

  if( !new_vtrack->video ) { /* Failed */
    delete new_vtrack;
    new_vtrack = 0;
  }

  return new_vtrack;
}

zvtrack_t::
~vtrack_t()
{
  if( video ) delete video;
  if( demuxer ) delete demuxer;
  if( private_offsets ) {
    if( frame_offsets_allocated ) delete [] frame_offsets;
    if( keyframe_numbers_allocated ) delete [] keyframe_numbers;
  }
  delete frame_cache;
}

void zvtrack_t::
extend_frame_offsets()
{
  if( frame_offsets_allocated <= total_frame_offsets ) {
    long new_allocation = ZMAX(2*total_frame_offsets, 1024);
    int64_t *new_offsets = new int64_t[new_allocation];
    for( int i=0; i<total_frame_offsets; ++i )
      new_offsets[i] = frame_offsets[i];
    delete [] frame_offsets;
    frame_offsets = new_offsets;
    frame_offsets_allocated = new_allocation;
  }
}

void zvtrack_t::
extend_keyframe_numbers()
{
  if( keyframe_numbers_allocated <= total_keyframe_numbers ) {
    long new_allocation = ZMAX(2*total_keyframe_numbers, 1024);
    int *new_numbers = new int[new_allocation];
    for( int i=0; i<total_keyframe_numbers; ++i )
      new_numbers[i] = keyframe_numbers[i];
    delete [] keyframe_numbers;
    keyframe_numbers = new_numbers;
    keyframe_numbers_allocated = new_allocation;
  }
}

void zvtrack_t::
append_frame(int64_t offset, int is_keyframe)
{
  extend_frame_offsets();
  frame_offsets[total_frame_offsets] = offset;

  if( is_keyframe || total_frame_offsets == 0 ) {
    extend_keyframe_numbers();
    keyframe_numbers[total_keyframe_numbers++] = total_frame_offsets;
  }

  ++total_frame_offsets;
  private_offsets = 1;
}

int zvtrack_t::
find_keyframe_index(int64_t frame)
{
  /* l,r boundarys are exclusive (not included) */
  int l = -1;
  int r = total_keyframe_numbers;
  while( (r-l) > 1 ) {
    int m = (r+l) >> 1;
    int mframe = keyframe_numbers[m];
    if( frame == mframe ) return m;
    if( frame > mframe ) l = m;
    else r = m;
  }
  return l;
}

void zvtrack_t::update_video_time()
{
  double pts = frame_pts;
  if( video->is_refframe() ) {
    pts = refframe_pts;
    refframe_pts = frame_pts;
  }
  frame_pts = -1.;
  if( pts < 0 ) return;
  if( pts_starttime < 0 && frame_rate > 0 ) {
    pts_starttime = pts;
    pts_offset = (double)video->framenum / frame_rate;
    if( pts_origin < 0. ) pts_origin = pts_starttime - pts_offset;
  }
  else if( pts < pts_starttime ) { // check for pts rollover
    if( pts_starttime-pts > 0x100000000ll / 90000 )
      pts += 0x200000000ll / 90000;
  }
  /* update track time */
  double vtime = pts_video_time(pts);
  if( vtime > video_time ) {
    video_time = vtime;
    pts_position = video->framenum;
//zmsgs("track %02x video_time=%f\n",track->pid, vtime);
  }
}

double zvtrack_t::get_video_time()
{
  double vtime = video_time; 
  if( vtime >= 0 && frame_rate > 0 ) {
    vtime += (video->framenum - pts_position) / frame_rate;
  }
  return vtime;
}

void zvtrack_t::
reset_pts()
{
  vskip = 0;
  demuxer->pes_video_time = -1.;
  pts_starttime = demuxer->src->pts_padding >= 0 ? -1. : 0.;
  video_time = -1.;
  refframe_pts = -1.;
  frame_pts = -1.;
  pts_position = 0;
  pts_offset = 0;
}

int zvtrack_t::apparent_position()
{
  if( !frame_offsets ) return 0;
  int64_t pos = demuxer->absolute_position();
  int l = -1;
  int r = total_frame_offsets;
  while( (r-l) > 1 ) {
    int m = (r+l) >> 1;
    int64_t mpos = frame_offsets[m];
    if( pos == mpos ) return m;
    if( pos > mpos ) l = m;
    else r = m;
  }   
  return l;
}

