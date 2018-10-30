#include <stdio.h>
#include "libzmpeg3.h"

/* reads a toc file which must refer to a transport stream
 *  decodes the dvb data and attempts to identify the stream
 *  nudge values to resync the related audio/video data
 */

int main(int ac, char **av)
{
  int ret, i, n, atrk, vtrk, w, h, channels, apid, vpid;
  char name[256], enc[32];
  const char *fmt;
  double pts, apts, vpts, nudge;
  zmpeg3_t *src = new zmpeg3_t(av[1],ret,ZIO_UNBUFFERED+ZIO_SINGLE_ACCESS);
  if( !src || ret != 0 ) {
    fprintf(stderr,"unable to open %s\n",av[1]);
    exit(1);
  }
  if( !src->has_toc() ) {
    fprintf(stderr,"not a toc file %s\n",av[1]);
    exit(1);
  }
  char *path = src->title_path(0);
  int total_atracks = src->total_astreams();
  int total_vtracks = src->total_vstreams();
  printf("toc file for %s, %d atracks, %d vtracks\n", path, total_atracks, total_vtracks);
  if( !src->is_transport_stream() ) {
    fprintf(stderr,"not a transport stream %s\n",path);
    exit(1);
  }
  src->demuxer->create_title();

  int total_channels = src->dvb.channel_count();
  for( n=0; n<total_channels; ++n ) {
    int major, minor, astreams, vstreams;
    double frame_rate = 0.;
    int sample_rate = 0;
    src->dvb.get_channel(n, major, minor);
    src->dvb.get_station_id(n, name);
    src->dvb.total_astreams(n, astreams);
    src->dvb.total_vstreams(n, vstreams);
    printf("channel %d.%d,  %d video streams, %d audio streams\n",
      major, minor, vstreams, astreams);
    apts = vpts = -1.;
    for( i=0; i<vstreams; ++i ) {
      src->dvb.vstream_number(n, i, vtrk);
      w = src->video_width(vtrk);
      h = src->video_height(vtrk);
      frame_rate = src->frame_rate(vtrk);
      src->vtrack[vtrk]->video->rewind_video();
      vpid = src->vtrack[vtrk]->pid;
      pts = src->vtrack[vtrk]->demuxer->scan_pts();
      printf(" vtrk %-2d/%03x stream %2d  %5dx%-5d %f   pts %f\n", 
        i, vpid, vtrk, w, h, frame_rate, pts);
      if( pts > vpts ) vpts = pts;
    }
    for( i=0; i<astreams; ++i ) {
      src->dvb.astream_number(n, i, atrk, &enc[0]);
      channels = src->audio_channels(atrk);
      fmt = src->audio_format(atrk);
      sample_rate = src->sample_rate(atrk);
      src->atrack[atrk]->audio->rewind_audio();
      apid = src->atrack[atrk]->pid;
      pts = src->atrack[atrk]->demuxer->scan_pts();
      printf(" atrk %-2d/%03x stream %2d %6s %2dch %-5d (%s)  pts %f\n", 
        i, apid, atrk, fmt, channels, sample_rate, &enc[0], pts);
      if( pts > apts ) apts = pts;
    }
    if( astreams > 0 ) {
      if( apts >= 0. && vpts >= 0. ) {
        nudge = vpts - apts;
        printf("   nudge %f (%f frames, %d samples)",
          -nudge, nudge*frame_rate, (int)(-nudge*sample_rate));
      }
      printf("\n");
    }
  }
  delete src;
  return 0;
}

