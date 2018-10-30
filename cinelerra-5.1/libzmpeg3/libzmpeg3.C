#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "libzmpeg3.h"

static uint8_t def_palette[] = { /* better than nothing */
  0x00, 0x80, 0x80, 0x00, /* black  #000000 */
  0xff, 0x80, 0x80, 0x00, /* white  #ffffff */
  0x4c, 0x54, 0xff, 0x00, /* red    #ff0000 */
  0x96, 0x2b, 0x15, 0x00, /* green  #00ff00 */
  0x1d, 0xff, 0x6b, 0x00, /* blue   #0000ff */
  0xe2, 0x00, 0x94, 0x00, /* yellow #ffff00 */
  0x69, 0xd4, 0xea, 0x00, /* purple #ff00ff */
  0xb3, 0xab, 0x00, 0x00, /* cyan   #00ffff */
  0x80, 0x80, 0x80, 0x00, /* grey   #808080 */
  0xa6, 0x6a, 0xbf, 0x00, /* pink   #ff8080 */
  0xcb, 0x55, 0x4a, 0x00, /* lime   #80ff80 */
  0x8e, 0xbf, 0x75, 0x00, /* lavndr #8080ff */
  0xd9, 0x95, 0x40, 0x00, /* brt cyan   #80ffff */
  0xb4, 0xaa, 0xb5, 0x00, /* brt purple #ff80ff */
  0xf1, 0x40, 0x8a, 0x00, /* brt yellow #ffff80 */
  0xff, 0x80, 0x80, 0x00, /* white  #ffffff */
};

int zmpeg3_t::
init(const char *title_path)
{
  cpus = 1;
  vts_title = 0;
  total_vts_titles = 1;
  interleave = angle = 0;
  total_interleaves = 1;
  bitfont_t::init_fonts();
  demuxer = new demuxer_t(this, 0, 0, -1);
  seekable = 1;  log_errs = getenv("LOG_ERRS") ? 1 : 0;
  memcpy(&palette[0], &def_palette[0], sizeof(def_palette));
  char *cp = getenv("PTS_PADDING");
  pts_padding = cp ? atoi(cp) : 1;
  index_bytes = 0x300000;
  cell_time = -1.;
  recd_fd = -1;
  iopened = fs->is_open;
  int ret = !iopened ? fs->open_file() : 0;
  if( ret == 0 ) {
    int toc_atracks = INT_MAX;
    int toc_vtracks = INT_MAX;
    ret = get_file_type(&toc_atracks, &toc_vtracks, title_path);
    if( ret == 0 ) {
      /* Start from scratch */
      if( !demuxer->total_titles )
        demuxer->create_title();
      /* Generate tracks */
      if( is_transport_stream() || is_program_stream() ) {
        int i, n;
        /* Create video tracks */
        for( i=n=0; i<MAX_STREAMZ && total_vtracks<toc_vtracks; ++i ) {
          if( demuxer->vstream_table[i] ) {
            vtrack[total_vtracks] = new_vtrack_t(i, demuxer, n++);
            if( vtrack[total_vtracks] ) ++total_vtracks;
          }
        }
        /* Create audio tracks */
        for( i=n=0; i<MAX_STREAMZ && total_atracks<toc_atracks; ++i ) {
          int format = demuxer->astream_table[i];
          if( format > 0 ) {
            atrack[total_atracks] = new_atrack_t(i, format, demuxer, n++);
            if( atrack[total_atracks] ) ++total_atracks;
          }
        }
        /* Create subtile tracks */
        for( i=0; i<MAX_STREAMZ; ++i ) {
          if( demuxer->sstream_table[i] ) {
            strack[total_stracks] = new strack_t(i);
            if( strack[total_stracks] ) ++total_stracks;
          }
        }
      }
      else if( is_video_stream() ) {
        /* Create video tracks */
        vtrack[0] = new_vtrack_t(-1, demuxer, 0);
        if( vtrack[0] ) ++total_vtracks;
      }
      else if( is_audio_stream() ) {
        /* Create audio tracks */
        atrack[0] = new_atrack_t(-1, afmt_UNKNOWN, demuxer, 0);
        if( atrack[0] ) ++total_atracks;
      }
    }
    if( (fs->access() & ZIO_SEQUENTIAL) != 0 )
      set_pts_padding(-1);
    demuxer->seek_byte(0);
    restart();
    if( !iopened )
      fs->close_file();
  }
  allocate_slice_decoders();
  return ret;
}

void zmpeg3_t::
restart()
{
  int i;

  for( i=0; i<total_vtracks; ++i ) {
    vtrack_t *vtrk = vtrack[i];
    if( vtrk && vtrk->video ) {
      vtrk->current_position = 0;
      vtrk->reset_pts();
      vtrk->frame_cache->clear();
      vtrk->video->rewind_video(0);
    }
  }

  for( i=0; i<total_atracks; ++i ) {
    atrack_t *atrk = atrack[i];
    if( atrk && atrk->audio ) {
      atrk->current_position = 0;
      atrk->reset_pts();
      atrk->audio->output_size = 0;
      atrk->audio->output_position = 0;
      atrk->audio->rewind_audio();
    }
  }

  fs->restart();
}

zmpeg3_t::
zmpeg3_t(const char *path)
#ifdef ZDVB
 : dvb(this)
#endif
{
  fs = new fs_t(this, path);
  cpus = 1;  log_errs = getenv("LOG_ERRS") ? 1 : 0;
  demuxer = new demuxer_t(this, 0, 0, -1);
// for toc generation
  seekable = 0;
  memcpy(&palette[0], &def_palette[0], sizeof(def_palette));
  char *cp = getenv("PTS_PADDING");
  pts_padding = cp ? atoi(cp) : 1;
  index_bytes = 0x300000;
  cell_time = -1.;
  recd_fd = -1;
}

zmpeg3_t::
zmpeg3_t(const char *path, int &ret, int atype, const char *title_path)
#ifdef ZDVB
 : dvb(this)
#endif
{
  fs = new fs_t(this, path, atype);
  ret = init(title_path);
}

zmpeg3_t::
zmpeg3_t(int fd, int &ret, int atype)
#ifdef ZDVB
 : dvb(this)
#endif
{
  fs = new fs_t(this, fd, atype);
  ret = init();
}

zmpeg3_t::
zmpeg3_t(FILE *fp, int &ret, int atype)
#ifdef ZDVB
 : dvb(this)
#endif
{
  fs = new fs_t(this, fp, atype);
  ret = init();
}

zmpeg3_t::
~zmpeg3_t()
{
  int i;
  int debug = 0;

  if( debug ) zmsg("0\n");
  stop_record();
  delete_slice_decoders();

  if( debug ) zmsg("1\n");
  for( i=0; i<total_stracks; ++i )
    delete strack[i];
  
  if( debug ) zmsg("2\n");
  for( i=0; i<total_atracks; ++i )
    delete atrack[i];

  if( debug ) zmsg("3\n");
  for( i=0; i<total_vtracks; ++i )
    delete vtrack[i];

  if( debug ) zmsg("4\n");
  delete demuxer;
  delete fs;
  if( playinfo ) delete playinfo;

  if( debug ) zmsg("5\n");
  if( frame_offsets ) {
    for( i=0; i<total_vtracks; ++i ) {
      delete [] frame_offsets[i];
      delete [] keyframe_numbers[i];
    }

    if( debug ) zmsg("6\n");
    delete [] frame_offsets;
    delete [] keyframe_numbers;
    delete [] total_frame_offsets;
    delete [] total_keyframe_numbers;
    delete [] video_eof;
  }

  if( debug ) zmsg("7\n");
  if( sample_offsets ) {
    for( i=0; i<total_atracks; ++i )
      delete [] sample_offsets[i];

    delete [] sample_offsets;
    delete [] total_sample_offsets;
  }

  if( debug ) zmsg("8\n");
  delete [] channel_counts;
  delete [] nudging;
  delete [] total_samples;
  delete [] audio_eof;

  if( debug ) zmsg("9\n");
  if( indexes ) {
    for( i=0; i<total_indexes; ++i )
      delete indexes[i];

    if( debug ) zmsg("10\n");
    delete [] indexes;
  }
  bitfont_t::destroy_fonts();
}


zmpeg3_t:: index_t::
index_t()
{
  index_zoom = 1;
}

zmpeg3_t:: index_t::
~index_t()
{
  for( int i=0;i<index_channels; ++i )
    delete [] index_data[i];
  delete [] index_data;
}

// Return number of tracks in the table of contents
int zmpeg3_t::
index_tracks()
{
  return total_indexes;
}

// Return number of channels in track
int zmpeg3_t::
index_channels(int track)
{
  if( track<0 || track>=total_indexes ) return 0;
  return indexes[track]->index_channels;
}

// Return zoom factor of index
int zmpeg3_t::
index_zoom()
{
  return !total_indexes ? 0 : indexes[0]->index_zoom;
}

// Number of high/low pairs in a channel of the track
int zmpeg3_t::
index_size(int track)
{
  if( track<0 || track>=total_indexes ) return 0;
  int size = indexes[track]->index_size;
  if( track < total_atracks && atrack[track]->nudge > 0 )
    size -= atrack[track]->nudge / indexes[0]->index_zoom;
  return size;
}

// Get data for one index channel
float* zmpeg3_t::
index_data(int track, int channel)
{
  if( track<0 || track>=total_indexes ) return 0;
  float *data = indexes[track]->index_data[channel];
  if( track < total_atracks && atrack[track]->nudge > 0 )
    data += (atrack[track]->nudge / indexes[0]->index_zoom) * 2;
  return data;
}

zstrack_t* zmpeg3_t::
get_strack_id(int id, video_t *vid) 
{
  for( int i=0; i<total_stracks; ++i ) {
    strack_t *strk = strack[i];
    if( strk->video && strk->video != vid ) continue;
    if( strk->id == id ) return strack[i];
  }
  return 0;
}

zstrack_t* zmpeg3_t::
get_strack(int number) 
{
  if( number >= total_stracks || number < 0 ) return 0;
  return strack[number];
}

zstrack_t* zmpeg3_t::
create_strack(int id, zvideo_t *vid)
{
  int i, j;
  strack_t *strk = 0;

  if( !(strk=get_strack_id(id, vid)) ) {
    strk = new strack_t(id, vid);
    for( i=0; i<total_stracks; ++i ) { 
      /* Search for first ID > current id */
      if( strack[i]->id > id ) {
        /* Shift back 1 */
        for( j=total_stracks; --j>=i; )
          strack[j+1] = strack[j];
        break;
      }
    }

/* Store in table */
    strack[i] = strk;
    ++total_stracks;
  }

  return strk;
}

int zmpeg3_t::
show_subtitle(int stream, int strk)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video )
      result = vtrk->video->show_subtitle(strk);
  }
  return result;
}

static inline int is_toc_(uint32_t bits)
{
  return (bits == zmpeg3_t::TOC_PREFIX);
}


static inline int is_ifo_(uint32_t bits)
{
  return (bits == zmpeg3_t::IFO_PREFIX);
}

static inline int is_transport_(uint32_t bits)
{
  return (((bits >> 24) & 0xff) == zmpeg3_t::SYNC_BYTE);
}

static inline int is_bd_(uint32_t bits1, uint32_t bits2, char *ext)
{
  return (((bits2 >> 24) & 0xff) == zmpeg3_t::SYNC_BYTE) && 
    ( !ext || (ext && 
      (!strncasecmp(ext, ".m2ts", 5) || 
       !strncasecmp(ext, ".mts", 4))) );
}

static inline int is_program_(uint32_t bits)
{
  return (bits == zmpeg3_t::PACK_START_CODE);
}

static inline int is_mpeg_audio_(uint32_t bits)
{
  return (bits & 0xfff00000) == 0xfff00000 ||
         (bits & 0xffff0000) == 0xffe30000 ||
         ((bits >> 8) == zmpeg3_t::ID3_PREFIX) ||
         (bits == zmpeg3_t::RIFF_CODE);
}

static inline int is_mpeg_video_(uint32_t bits)
{
  return (bits == zmpeg3_t::SEQUENCE_START_CODE ||
          bits == zmpeg3_t::PICTURE_START_CODE);
}

static inline int is_ac3_(uint32_t bits)
{
  return (((bits & 0xffff0000) >> 16) == zmpeg3_t::AC3_START_CODE);
}

int zmpeg3_t::
check_sig(char *path)
{
  int result = 0;
  fs_t *fs = new fs_t(0, path);

  if( !fs->open_file() ) { /* File found */
    char *ext = strrchr(path, '.');
    uint32_t bits = fs->read_uint32();
//  uint32_t bits2 = fs->read_uint32();
 
    /* pre-approved suffixes */
    if( ext && !strncasecmp(ext, ".mp3", 4) ) {
      result = 1;
    }
    /* Test header */
    else if( bits == TOC_PREFIX ) {
      result = 1;
    }
/* don't use, does not work well at all */
//    /* Blu-Ray or AVC-HD*/
//    else if( is_bd_(bits, bits2, ext) ) {
//      result = 1;
//    }
    else if( (((bits >> 24) & 0xff) == zmpeg3_t::SYNC_BYTE ) ||
             (bits == zmpeg3_t::PACK_START_CODE) ||
             ((bits & 0xfff00000) == 0xfff00000) ||
             ((bits & 0xffff0000) == 0xffe30000) ||
             (bits == zmpeg3_t::SEQUENCE_START_CODE) ||
             (bits == zmpeg3_t::PICTURE_START_CODE) ||
             (((bits & 0xffff0000) >> 16) == zmpeg3_t::AC3_START_CODE) ||
             ((bits >> 8) == zmpeg3_t::ID3_PREFIX) || 
             (bits == zmpeg3_t::RIFF_CODE) ||
             (bits == zmpeg3_t::IFO_PREFIX)) {
      result = 1;
  
      /* Test file extension. */
      if( ext ) {  ++ext;
        if( strcasecmp(ext, "ifo") && 
            strcasecmp(ext, "mp2") && 
            strcasecmp(ext, "mp3") &&
            strcasecmp(ext, "m1v") &&
            strcasecmp(ext, "m2v") &&
            strcasecmp(ext, "m2s") &&
            strcasecmp(ext, "mpg") &&
            strcasecmp(ext, "vob") &&
            strcasecmp(ext, "ts") &&
            strcasecmp(ext, "vts") &&
            strcasecmp(ext, "mpeg") &&
            strcasecmp(ext, "m2t") &&
            strcasecmp(ext, "ac3") )
          result = 0;
      }
    }
    fs->close_file();
  }

  delete fs;
  return result;
}

int zmpeg3_t::
calculate_packet_size()
{
  if( is_transport_stream() )
    return is_bd() ?  BD_PACKET_SIZE : TS_PACKET_SIZE;
  if( is_program_stream() )
    return 0;
  if( is_audio_stream() || is_video_stream() )
    return DVD_PACKET_SIZE;
  zerr("undefined stream type.\n");
  return -1;
}

int zmpeg3_t::
get_file_type(int *toc_atracks, int *toc_vtracks, const char *title_path)
{
  int result = 0;
  file_type = 0;
  uint32_t bits = fs->read_uint32();
  uint32_t bits2 = fs->read_uint32();

  /* TOC  */
  if( is_toc_(bits) ) {
    /* Table of contents for another title set */
    result = toc_atracks && toc_vtracks ?
      read_toc(toc_atracks, toc_vtracks, title_path) : 1;
    if( result ) fs->close_file();
  }
  else if( is_ifo_(bits) ) {
  /* IFO file */
    if( !read_ifo() )
      file_type = FT_PROGRAM_STREAM | FT_IFO_FILE;
    fs->close_file();
  }
  else if( is_bd_(bits,bits2,0) ) {
    file_type = FT_TRANSPORT_STREAM | FT_BD_FILE;
  }
  /* Transport stream */
  else if( is_transport_(bits) ) {
    file_type = FT_TRANSPORT_STREAM;
  }
  /* Program stream */
  /* Determine packet size empirically */
  else if( is_program_(bits) ) {
    file_type = FT_PROGRAM_STREAM;
  }
  /* MPEG Audio only */
  else if( is_mpeg_audio_(bits) ) {
    file_type = FT_AUDIO_STREAM;
  }
  /* Video only */
  else if( is_mpeg_video_(bits) ) {
    file_type = FT_VIDEO_STREAM;
  }
  /* AC3 Audio only */
  else if( is_ac3_(bits) ) {
    file_type = FT_AUDIO_STREAM;
  }
  else {
    const char *ext = strrchr(fs->path, '.');
    if( ext ) {
      if( !strncasecmp(ext, ".mp3", 4) ) {
        file_type = FT_AUDIO_STREAM;
      }
    }
  }
//zmsgs("2 %x\n", file_type);
  if( !file_type ) {
    zerr("not a readable stream.\n");
    if( !result ) result = ERR_UNDEFINED_ERROR;
  }
  else
    packet_size = calculate_packet_size();
  return result;
}

int zmpeg3_t::
set_pts_padding(int v)
{
  int ret = pts_padding;
  pts_padding = v;
  if( pts_padding >= 0 ) {
    for( int vtk=0; vtk<total_vtracks; ++vtk )
      vtrack[vtk]->pts_starttime = -1.;
    for( int atk=0; atk<total_atracks; ++atk )
      atrack[atk]->pts_starttime = -1.;
  }
  else {
    for( int vtk=0; vtk<total_vtracks; ++vtk )
      vtrack[vtk]->pts_starttime = vtrack[vtk]->pts_offset = 0.;
    for( int atk=0; atk<total_atracks; ++atk )
      atrack[atk]->pts_starttime = atrack[atk]->pts_offset = 0.;
  }
  return ret;
}

int zmpeg3_t::
set_cpus(int cpus)
{
  this->cpus = cpus;
  int i;
  for( i=0; i<total_vtracks; ++i )
    vtrack[i]->video->set_cpus(cpus);
  return 0;
}

int zmpeg3_t::
has_audio()
{
  return total_atracks > 0;
}

int zmpeg3_t::
total_astreams()
{
  return total_atracks;
}

int zmpeg3_t::
audio_channels(int stream)
{
  return stream>=0 && stream<total_atracks ?
    atrack[stream]->channels : -1;
}

double zmpeg3_t::
sample_rate(int stream)
{
  return stream>=0 && stream<total_atracks ?
    (double)atrack[stream]->sample_rate : -1.;
}

long zmpeg3_t::
get_sample(int stream)
{
  return stream>=0 && stream<total_atracks ?
    atrack[stream]->current_position : -1;
}

int zmpeg3_t::
set_sample(long sample, int stream)
{
  int result = 1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk && atrk->audio ) {
//zmsgs("1 %d %ld\n", atrk->current_position, sample);
      if( atrk->current_position != sample ) {
        atrk->current_position = sample;
        atrk->audio->seek_sample(sample);
      }
      result = 0;
    }
  }
  return result;
}

long zmpeg3_t::
audio_nudge(int stream)
{
  long result = 0;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk ) result = atrk->nudge;
  }
  return result;
}

long zmpeg3_t::
audio_samples(int stream)
{
  long result = -1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk ) result = atrk->total_samples;
  }
  return result;
}

const char* zmpeg3_t::
audio_format(int stream)
{
  const char *result = 0;
  if( stream>=0 && stream<total_atracks ) {
    switch( atrack[stream]->format ) {
    case afmt_IGNORE:  result = "Ignore";  break;
    case afmt_UNKNOWN: result = "Unknown"; break;
    case afmt_MPEG:    result = "MPEG";    break;
    case afmt_AC3:     result = "AC3";     break;
    case afmt_PCM:     result = "PCM";     break;
    case afmt_AAC:     result = "AAC";     break;
    case afmt_JESUS:   result = "Vorbis";  break;
    default:           result = "Undef";   break;
    }
  }
  return result;
}

int zmpeg3_t::
has_video()
{
  return total_vtracks > 0;
}

int zmpeg3_t::
total_vstreams()
{
  return total_vtracks;
}

int zmpeg3_t::
video_width(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->width;
  }
  return result;
}

int zmpeg3_t::
video_height(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->height;
  }
  return result;
}

int zmpeg3_t::
coded_width(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk )
      result = vtrk->video->coded_picture_width;
  }
  return result;
}

int zmpeg3_t::
coded_height(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk )
      result = vtrk->video->coded_picture_height;
  }
  return result;
}

int zmpeg3_t::
video_pid(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->pid;
  }
  return result;
}

float zmpeg3_t::
aspect_ratio(int stream)
{
  float result = 0.0;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->aspect_ratio;
  }
  return result;
}

double zmpeg3_t::
frame_rate(int stream)
{
  double result = -1.0;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->frame_rate;
  }
  return result;
}

long zmpeg3_t::
video_frames(int stream)
{
  long result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->total_frames;
  }
  return result;
}

long zmpeg3_t::
get_frame(int stream)
{
  long result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk ) result = vtrk->current_position;
  }
  return result;
}

int zmpeg3_t::
set_frame(long frame, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      if( vtrk->current_position != frame ) {
        vtrk->current_position = frame;
        vtrk->video->seek_frame(frame);
      }
      result = 0;
    }
  }
  return result;
}

int zmpeg3_t::
seek_byte(int64_t byte)
{
  int i;

  for( i=0; i<total_vtracks; ++i ) {
    vtrack_t *vtrk = vtrack[i];
    if( vtrk && vtrk->video ) {
      vtrk->current_position = 0;
      if( byte )
        vtrk->video->seek_byte(byte);
      else
        vtrk->video->rewind_video();
    }
  }

  for( i=0; i<total_atracks; ++i ) {
    atrack_t *atrk = atrack[i];
    if( atrk && atrk->audio ) {
      atrk->current_position = 0;
      atrk->audio->seek_byte(byte);
    }
  }

  return 0;
}

int zmpeg3_t::
previous_frame(int stream)
{
  int result = 1;

  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      last_type_read = 2;
      last_stream_read = stream;
      result = vtrk->video->previous_frame();
    }
  }
  return result;
}

int64_t zmpeg3_t::
tell_byte()
{
  int64_t result = -1;
  if( last_type_read == 1 )
    result = atrack[last_stream_read]->demuxer->tell_byte();
  else if( last_type_read == 2 )
    result =vtrack[last_stream_read]->demuxer->tell_byte();
  return result;
}

int64_t zmpeg3_t::
get_bytes()
{
  return demuxer->movie_size();
}

double zmpeg3_t::
get_audio_time(int stream)
{
  double atime = -1.0;
  if( stream >= 0 && stream < total_atracks ) {
    double samplerate = sample_rate(stream);
    if( samplerate > 0. ) {
      int64_t sample = get_sample(stream);
      if( !pts_padding || is_audio_stream() || is_video_stream() ) {
        atime = sample / samplerate;
      }
      else {
        atime = atrack[stream]->audio_time +
          (sample - atrack[stream]->pts_position) / samplerate;
      }
    }
  }
  return atime;
}

double zmpeg3_t::
get_video_time(int stream)
{
  double vtime = -1.0;
  if( stream >= 0 && stream < total_vtracks ) {
    double framerate = frame_rate(stream);
    if( !pts_padding || is_audio_stream() || is_video_stream() ) {
      if( framerate > 0. )
        vtime = (double)get_frame(stream) / framerate;
    }
    else {
      vtime = vtrack[stream]->video_time;
      if( vtime >= 0. && framerate > 0. )
        vtime += (get_frame(stream) - vtrack[stream]->pts_position) / framerate;
    }
  }
  return vtime;
}

/* pts timecode available only in transport stream */
double zmpeg3_t::
get_time()
{
  double time = 0;
  if( last_type_read == 1 )
    time = get_audio_time(last_stream_read);
  else if( last_type_read == 2 )
    time = get_video_time(last_stream_read);
  return time;
}

int zmpeg3_t::
get_total_vts_titles()
{
  return total_vts_titles;
}

int zmpeg3_t::
set_vts_title(int title)
{
  int result = vts_title;
  if( title >= 0 )
    vts_title = title;
  return result;
}

int zmpeg3_t::
get_total_interleaves()
{
  return total_interleaves;
}

int zmpeg3_t::
set_interleave(int inlv)
{
  int result = interleave;
  if( inlv >= 0 )
    interleave = inlv;
  return result;
}

int zmpeg3_t::
set_angle(int a)
{
  int result = angle;
  if( a >= 0 )
    angle = a;
  return result;
}

int zmpeg3_t::
set_program(int no)
{
  int result = set_vts_title()*100 + set_angle()*10 + set_interleave();
  if( no >= 0 ) {
    set_vts_title(no / 100);
    set_angle((no/10) % 10);
    set_interleave(no % 10);
  }
  return result;
}

int zmpeg3_t::
get_cell_time(int no, double &time)
{
  int result = 1, i = 0;
  zcell_t *cell = 0;
  while( no >= 0 ) {
    result = demuxer->get_cell(i, cell);
    if( result < 0 ) return result;
    if( result > 0 ) i = cell->cell_no;
    ++i;  --no;
  }
  time = cell->cell_time;
  return 0;
}

int zmpeg3_t::
end_of_audio(int stream)
{
  int result = 1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk && atrk->demuxer )
      result = atrk->demuxer->eof();
  }
  return result;
}

int zmpeg3_t::
end_of_video(int stream)
{
  int result = 1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->demuxer )
      result = vtrk->demuxer->eof();
  }
  return result;
}

int zmpeg3_t::
drop_frames(long frames, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      result = vtrk->video->drop_frames(frames, 0);
      if( frames > 0 )
        vtrk->current_position += frames;
      last_type_read = 2;
      last_stream_read = stream;
    }
  }
  return result;
}

int zmpeg3_t::
colormodel(int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video )
      result = vtrk->video->colormodel();
  }
  return result;
}

int zmpeg3_t::
set_rowspan(int bytes, int stream)
{
  int result = 1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      vtrk->video->row_span = bytes;
      result = 0;
    }
  }
  return result;
}


int zmpeg3_t::
read_frame(uint8_t **output_rows, 
    int in_x, int in_y, int in_w, int in_h, 
    int out_w, int out_h, int color_model, int stream)
{
  int result = -1;
//zmsgs("1 %d\n", vtrack[stream]->current_position);
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      result = vtrk->video->read_frame(output_rows,
          in_x, in_y, in_w, in_h, out_w, out_h, color_model);
//zmsg(" 2\n");
      last_type_read = 2;
      last_stream_read = stream;
      vtrk->current_position++;
    }
  }
//zmsgs("2 %d\n", vtrack[stream]->current_position);
  return result;
}

int zmpeg3_t::
read_yuvframe(char *y_output, char *u_output, char *v_output,
    int in_x, int in_y, int in_w, int in_h, int stream)
{
  int result = -1;
//zmsg("1\n");
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      result = vtrk->video->read_yuvframe(y_output, u_output, v_output,
          in_x, in_y, in_w, in_h);
      last_type_read = 2;
      last_stream_read = stream;
      vtrk->current_position++;
    }
  }
//zmsg("100\n");
  return result;
}

int zmpeg3_t::
read_yuvframe_ptr(char **y_output, char **u_output, char **v_output,
    int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      result = vtrk->video->read_yuvframe_ptr(y_output, u_output, v_output);
      last_type_read = 2;
      last_stream_read = stream;
      vtrk->current_position++;
    }
  }
  return result;
}

int zmpeg3_t::
read_audio(void *output, int type, int channel, long samples, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk && atrk->audio ) {
      atrk->audio->update_audio_history();
      result = atrk->audio->decode_audio(output, type, channel, samples);
      last_type_read = 1;
      last_stream_read = stream;
      atrk->current_position += samples;
    }
  }
  return result;
}

int zmpeg3_t::
read_audio(double *output_d, int channel, long samples, int stream)
{
  return read_audio((void *)output_d, atyp_DOUBLE, channel, samples, stream);
}

int zmpeg3_t::
read_audio(float *output_f, int channel, long samples, int stream)
{
  return read_audio((void *)output_f, atyp_FLOAT, channel, samples, stream);
}

int zmpeg3_t::
read_audio(int *output_i, int channel, long samples, int stream)
{
  return read_audio((void *)output_i, atyp_INT, channel, samples, stream);
}

int zmpeg3_t::
read_audio(short *output_s, int channel, long samples, int stream)
{
  return read_audio((void *)output_s, atyp_SHORT, channel, samples, stream);
}

int zmpeg3_t::
reread_audio(void *output, int atyp, int channel, long samples, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk && atrk->audio ) {
      set_sample( atrk->current_position-samples, stream);
      result = atrk->audio->decode_audio(output, atyp, channel, samples);
      last_type_read = 1;
      last_stream_read = stream;
      atrk->current_position += samples;
    }
  }
  return result;
}

int zmpeg3_t::
reread_audio(double *output_d, int channel, long samples, int stream)
{
  return reread_audio((void *)output_d, atyp_DOUBLE, channel, samples, stream);
}

int zmpeg3_t::
reread_audio(float *output_f, int channel, long samples, int stream)
{
  return reread_audio((void *)output_f, atyp_FLOAT, channel, samples, stream);
}

int zmpeg3_t::
reread_audio(int *output_i, int channel, long samples, int stream)
{
  return reread_audio((void *)output_i, atyp_INT, channel, samples, stream);
}

int zmpeg3_t::
reread_audio(short *output_s, int channel, long samples, int stream)
{
  return reread_audio((void *)output_s, atyp_SHORT, channel, samples, stream);
}


int zmpeg3_t::
read_audio_chunk(uint8_t *output, long *size, long max_size, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_atracks ) {
    atrack_t *atrk = atrack[stream];
    if( atrk && atrk->audio ) {
      result = atrk->audio->read_raw(output, size, max_size);
      last_type_read = 1;
      last_stream_read = stream;
    }
  }
  return result;
}

int zmpeg3_t::
read_video_chunk( uint8_t *output, long *size, long max_size, int stream)
{
  int result = -1;
  if( stream>=0 && stream<total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    if( vtrk && vtrk->video ) {
      result = vtrk->video->read_raw(output, size, max_size);
      last_type_read = 2;
      last_stream_read = stream;
    }
  }
  return result;
}

int64_t zmpeg3_t::
memory_usage()
{
  int64_t result = 0;
  for( int i=0; i<total_vtracks; ++i )
    if( vtrack[i]->frame_cache )
      result += vtrack[i]->frame_cache->memory_usage();
  return result;
}

int zmpeg3_t::
start_record(int fd, int bsz)
{
  if( recd_fd >= 0 ) return 1;
  struct stat64 st;
  memset(&st, 0, sizeof(st));
  if( fstat64(fd, &st) < 0 ) return 1;
  recd_fd = fd;
  recd_pos = st.st_size;
  if (fs->start_record(bsz)) return recd_fd = -1;
  return 0;
}
int zmpeg3_t::
stop_record()
{
  if( fs->stop_record() ) return 1;
  recd_fd = -1;
  return 0;
}

void zmpeg3_t::
write_record(uint8_t *data, int len)
{
  if( recd_fd < 0 ) return;
  if( write(recd_fd, data, len) < 0 ) {
    zmsgs("write err: %s\n",strerror(errno));
    return;
  }
  recd_pos += len;
}

int zmpeg3_t::
pause_reader(int v)
{
  return fs->pause_reader(v);
}

int zmpeg3_t::
get_thumbnail(int trk, int64_t &frn, uint8_t *&t, int &w, int &h)
{
  int result = -1;
  if( trk>=0 && trk<total_vtracks ) {
    vtrack_t *vtrk = vtrack[trk];
    if( vtrk ) {
      video_t *vid = vtrk->video;
      frn = vid->framenum;
      t = vid->tdat;
      w = 2*vid->mb_width;
      h = 2*vid->mb_height;
      result = 0;
    }
  }
  return result;
}

int zmpeg3_t::
set_thumbnail_callback(int trk, int skim, int thumb,
        zthumbnail_cb fn, void *p)
{
  int result = 1;
  // all tracks, toc build
  if( trk == -1 )
    result = 0;
  // one track, commercials verify
  else if( trk >= 0 && trk<total_vtracks ) {
    vtrack_t *vtrk = vtrack[trk];
    if( vtrk ) {
      video_t *vid = vtrk->video;
      if( !(result=vid->seek_video()) ) {
        vid->skim = skim;
        vid->thumb = thumb;
        vid->ithumb = 0;
        result = 0;
      }
    }
  }
  if( !result ) {
    thumbnail_priv = p;
    thumbnail_fn = fn;
  }
  return result;
}

int zmpeg3_t::
set_cc_text_callback(int trk, zcc_text_cb fn)
{
  int result = -1;
  if( trk>=0 && trk<total_vtracks ) {
    vtrack_t *vtrk = vtrack[trk];
    if( vtrk ) {
      video_t *vid = vtrk->video;
      vid->get_cc()->text_cb = fn;
      result = 0;
    }
  }
  return result;
}

int64_t zmpeg3_t::
calculate_source_date(char *path)
{
  struct stat64 ostat;
  memset(&ostat,0,sizeof(struct stat64));
  return stat64(path, &ostat) < 0 ? 0 : ostat.st_mtime;
}

int64_t zmpeg3_t::
calculate_source_date(int fd)
{
  struct stat64 ostat;
  memset(&ostat,0,sizeof(struct stat64));
  return fstat64(fd, &ostat) < 0 ? 0 : ostat.st_mtime;
}

void zdebug(int n)
{
  zmsgs("%d\n",n);
}


