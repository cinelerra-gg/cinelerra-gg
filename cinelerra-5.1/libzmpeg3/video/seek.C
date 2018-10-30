#include "../libzmpeg3.h"

static void
toc_error()
{
  zerr( "frame accurate seeking without a table of contents \n"
    "is no longer supported.  Use mpeg3toc <mpeg file> <table of contents>\n"
    "to generate a table of contents and load the table of contents instead.\n");
}

void zvideo_t::
cache_frame()
{
  if( output_src[0] ) {
    int coded_size = coded_picture_width * coded_picture_height;
    int chrom_size = chrom_width * chrom_height;
    track->frame_cache->put_frame(framenum-1,
      output_src[0], output_src[1], output_src[2],
      coded_size, chrom_size, chrom_size);
  }
}

int zvideo_t::
drop_frames(long frames, int cache_it)
{
  int result = 0;
  long frame_number = framenum + frames;
//zmsgs("drop frames framenum %d frames %ld\n", framenum, frames);

/* Read the selected number of frames and skip b-frames */
  while(!result && frame_number > framenum ) {
    if( cache_it ) {
      result = read_frame_backend(0);
      if( !result && ref_frames > 1 )
        cache_frame();
    }
    else {
//zmsgs("framenum %d frame_offset %jx last_packet_start %jx repeat %d/%d",
//  framenum,track->frame_offsets[framenum+1],vstream->demuxer->last_packet_start,
//  repeat_fields, current_field);
      result = read_frame_backend(frame_number - framenum);
//zmsgs(" type %c\n","XIPBD"[pict_type]);
    }
  }

  return result;
}

int zbits_t::
next_start_code()
{
//zmsg("1\n");
  /* search forwards */
  prev_byte_align();
  int64_t stream_end = demuxer->stream_end;
  if( demuxer->src->is_transport_stream() )
    demuxer->stream_end = demuxer->program_byte + MAX_TS_PROBE;
  else if( demuxer->src->is_program_stream() )
    demuxer->stream_end = demuxer->program_byte + MAX_PGM_PROBE;
  int zcode = -1, result = -1;

  while( !eof() ) {
    int bits = (uint8_t)zcode!=0 ? 24 : 8;
    zcode = (zcode<<bits) | get_bits_noptr(bits);
    if( (zcode&0xffffff) == PACKET_START_CODE_PREFIX ) {
      zcode = (zcode<<8) + get_byte_noptr();
      reset(result = zcode);
//zmsgs("5 %04x\n",zcode);
      break;
    }
  }

  demuxer->stream_end = stream_end;
  return result;
}

/* Line up on the beginning of the next code. */
int zbits_t::
next_code(uint32_t zcode)
{
  while(!eof() && show_bits_noptr(32) != zcode ) {
    get_byte_noptr();
  }
  return eof();
}

/* Line up on the beginning of the previous code. */
int zdemuxer_t::
prev_code(uint32_t zcode)
{
  uint32_t current_code = 0;
#define PREV_CODE_MACRO { current_code >>= 8; \
  current_code |= ((uint32_t)read_prev_char()) << 24; }

  PREV_CODE_MACRO
  PREV_CODE_MACRO
  PREV_CODE_MACRO
  PREV_CODE_MACRO

  while( !bof() && current_code != zcode ) {
    PREV_CODE_MACRO
  }
  return bof();
}

int zvideo_t::
seek_byte(int64_t byte)
{
  byte_seek = byte;
  return 0;
}

int zvideo_t::
seek_frame(long frame)
{
  frame_seek = frame;
  return 0;
}

int zvideo_t::
rewind_video(int preload)
{
  vstream->reset();
  framenum = 0;
  if( track->frame_offsets )
    vstream->seek_byte(track->frame_offsets[0]);
  else
    vstream->seek_byte(0);
  track->reset_pts();
  repeat_data = ref_frames = 0;
  last_number = framenum = -1;
  return preload ? read_frame_backend(0) : 0;
}

int zvideo_t::
seek()
{
  int result = 0;
  int64_t byte;
  long frame_number;
  int debug = 0;
  demuxer_t *demux = vstream->demuxer;

/* Must do seeking here so files which don't use video don't seek. */
/* Seeking is done in the demuxer */
/* Seek to absolute byte */
  if( byte_seek >= 0 ) {
    byte = byte_seek;
    byte_seek = -1;
    if( byte != demux->tell_byte() ) {
      demux->seek_byte(byte);
      track->reset_pts();
      if( byte > 0 ) {
  //zmsg("1\n");
        demux->start_reverse();
  //zmsgs("1 %jd\n", demux->tell_byte());
        for( int i=0; i<2; ++i ) {     /* Rewind 2 I-frames */
          if( has_gops ?
            demux->prev_code(GOP_START_CODE) :
            demux->prev_code(SEQUENCE_START_CODE) ) break;
        }
  //zmsgs("2 %jd\n", demux->tell_byte());
        demux->start_forward();
        if( track->frame_offsets ) {
          int64_t offset = demux->tell_byte();
          for( int k=0; k<track->total_keyframe_numbers; ++k ) {
            frame_number = track->keyframe_numbers[k];
            if( track->frame_offsets[frame_number] >= offset ) {
              /* first entry is special */
              framenum = frame_number>0 ? frame_number-1 : 0;
              break;
            }
          }
        }
        else {
          /* framenum no longer accurate, clear subtitles */
          reset_subtitles();
        }
      }
      else { /* Read first frame */
        rewind_video();
      }
      vstream->reset();
      repeat_data = ref_frames = 0;
  //zmsgs("4 %jd\n", demux->tell_byte());
      while( !result && !demux->eof() && demux->tell_byte() < byte ) {
        result = read_frame_backend(0);
      }
    }
  }
  else if( frame_seek >= 0 ) {
    /* Seek to a frame number */
    frame_number = frame_seek;
    frame_seek = -1;
    if( frame_number != framenum ) {
      if( debug ) zmsgs("%d\n", __LINE__);
      if( frame_number < 0 ) frame_number = 0;
      if( frame_number > maxframe ) frame_number = maxframe;
      if(debug) zmsgs("%d %ld %d\n", __LINE__, frame_number, framenum);
      /* Seek to I frame in table of contents. */
      /* Determine time between seek position and previous subtitle. */
      /* Subtract time difference from subtitle display time. */
      if( frame_number > 0 && track->frame_offsets ) {
        if( debug ) zmsgs("%d\n", __LINE__);
        track->frame_cache->reset();
        if( debug ) zmsgs("%d\n", __LINE__);
        if( frame_number < framenum || 
            frame_number-framenum > SEEK_THRESHOLD ) {
          int idx = track->find_keyframe_index(frame_number);
          /* back up 2 I frames, data may reference old refframes */
          if( --idx < 0 ) idx = 0;
          idx = track->keyframe_numbers[idx];
          byte = track->frame_offsets[idx];
          /* maybe lots of frames in one block, move is to earliest frame */
          while( idx > 0 && track->frame_offsets[idx-1] == byte ) --idx;
          if( debug ) zmsgs("%ld idx=%d, byte=%jd\n", frame_number,idx,byte);
          framenum = idx-1;
          vstream->seek_byte(byte);
          track->reset_pts();
          repeat_data = ref_frames = 0;
          /* get the first I-frame.  It does not count in the frame sequence */
          /*   phys order is IBBPBBPBB... presentation order is BBIBBPBB... */
          result = read_frame_backend(0);
        }
        if( debug ) zmsgs("%d %ld %d\n", __LINE__,frame_number,framenum);
        if( !result && frame_number > framenum ) {
          /* Read up to current frame */
          int n = frame_number - framenum;
          result = drop_frames(n, n < MAX_CACHE_FRAMES ? 1 : 0);
          if( debug ) zmsgs("%d\n", __LINE__);
        }
      }
      else if( frame_number == 0 ) {
        result = rewind_video();
      }
      else {
        /* No support for seeking without table of contents */
        toc_error();
        result = 1;
      }
    }
  }
  else if( framenum < 0 ) {   /* preload */
    result = rewind_video(1);
  }
  else
    return 0;

  seek_time = 0;
  return result;
}

int zvideo_t::
previous_frame()
{
  int result = 0;
  int64_t target_byte = 0;
  demuxer_t *demux = vstream->demuxer;
  if( demux->tell_byte() <= 0 ) return 1;
  /* Get location of end of previous picture */
  demux->start_reverse();
  result = demux->prev_code(PICTURE_START_CODE);
  if( !result ) result = demux->prev_code(PICTURE_START_CODE);
  if( !result ) result = demux->prev_code(PICTURE_START_CODE);
  if( !result ) target_byte = demux->tell_byte();
  if( !result ) { /* Rewind 2 I-frames */
    if( has_gops )
      result = demux->prev_code(GOP_START_CODE);
    else
      result = demux->prev_code(SEQUENCE_START_CODE);
  }

  if( !result )
  {
    if( has_gops )
      result = demux->prev_code(GOP_START_CODE);
    else
      result = demux->prev_code(SEQUENCE_START_CODE);
  }

  demux->start_forward();
  vstream->reset();

  result = 0;    /* Read up to correct byte */
  repeat_data = ref_frames = 0;
  while( !result && !demux->eof() && demux->tell_byte() < target_byte ) {
    result = read_frame_backend(0);
  }
  repeat_data = 0;

  return 0;
}

