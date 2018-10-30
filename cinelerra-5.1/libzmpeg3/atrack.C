#include "libzmpeg3.h"

zatrack_t::
atrack_t(zmpeg3_t *zsrc, int custom_id, int format,
         demuxer_t *demux, int no)
{
  channels = zsrc->channel_counts ? zsrc->channel_counts[no] : 0;
  sample_rate = 0;
  total_samples = 0;
  demuxer = new demuxer_t(zsrc, this, 0, custom_id);
  if( zsrc->seekable ) {
    demux->copy_titles(demuxer);
  }

  current_position = 0;
  nudge = 0;
  number = no;
  pid = custom_id;
  pts_origin = -1.;
  reset_pts();

  if( zsrc->sample_offsets ) { /* Copy pointers */
    sample_offsets = zsrc->sample_offsets[number];
    total_sample_offsets = zsrc->total_sample_offsets[number];
    total_samples = zsrc->total_samples[number];
    demuxer->stream_end = zsrc->audio_eof[number];
    nudge = zsrc->nudging[number];
  }
}

zatrack_t *zmpeg3_t::
new_atrack_t(int custom_id, int format, demuxer_t *demux, int number)
{
  atrack_t *new_atrack = new atrack_t(this,custom_id,format,demux,number);
  new_atrack->audio = new_audio_t(new_atrack, format);
  if( !new_atrack->audio ) { /* Failed */
    delete new_atrack;
    new_atrack = 0;
  }

  return new_atrack;
}

zatrack_t::
~atrack_t()
{
  if( audio ) delete audio;
  if( demuxer ) delete demuxer;
  if( sample_offsets && private_offsets ) {
    delete [] sample_offsets;
  }
}

void zatrack_t::
extend_sample_offsets()
{
  if( sample_offsets_allocated <= total_sample_offsets ) {
    long new_allocation = ZMAX(2*total_sample_offsets, 1024);
    int64_t *new_offsets = new int64_t[new_allocation];
    for( int i=0; i<total_sample_offsets; ++i )
      new_offsets[i] = sample_offsets[i];
    delete [] sample_offsets;
    sample_offsets = new_offsets;
    sample_offsets_allocated = new_allocation;
  }
}

void zatrack_t::
append_samples(int64_t offset)
{
  extend_sample_offsets();
  sample_offsets[total_sample_offsets++] = offset;
  private_offsets = 1;
}

void zatrack_t::update_audio_time()
{
  double pts = frame_pts;
  frame_pts = -1;
  if( pts < 0 ) return;
  int64_t audio_pos = audio->audio_position();
  if( pts_starttime < 0. && sample_rate > 0 ) {
    pts_starttime = pts;
    pts_offset = audio_pos / (double)sample_rate;
    if( pts_origin < 0. )
      pts_origin = pts - pts_offset;
  }
  else if( pts < pts_starttime ) { // check for pts rollover
    if( pts_starttime-pts > 0x100000000ll / 90000 )
      pts += 0x200000000ll / 90000;
  }
  double atime = pts_audio_time(pts);
  if( atime > audio_time ) {
    pts_position = audio_pos;
    audio_time = atime;
//zmsgs("track %02x audio_time=%f\n", pid, audio_time);
  }
}

double zatrack_t::get_audio_time()
{
  double atime = audio_time;
  if( atime >= 0 && sample_rate > 0 ) {
    int64_t audio_pos = audio->audio_position();
    atime += (audio_pos-pts_position) / (double)sample_rate;
  }
  return atime;
}

void zatrack_t::
reset_pts()
{
  askip = 0;
  demuxer->pes_audio_time = -1.;
  pts_starttime = demuxer->src->pts_padding >= 0 ? -1. : 0.;
  audio_time = -1.;
  frame_pts = -1.;
  pts_position = 0;
  pts_offset = 0;
}

int64_t zatrack_t::apparent_position()
{
  int64_t pos = demuxer->absolute_position();
  int l = -1;
  int r = total_sample_offsets;
  while( (r-l) > 1 ) {
    int m = (r+l) >> 1;
    int64_t mpos = sample_offsets[m];
    if( pos == mpos ) return m;
    if( pos > mpos ) l = m;
    else r = m;
  }
  int64_t result = (int64_t)l * AUDIO_CHUNKSIZE;
  if( r < total_sample_offsets )
    result += (AUDIO_CHUNKSIZE * (pos - sample_offsets[l])) /
		 (sample_offsets[r] - sample_offsets[l]);
  return result;
}

