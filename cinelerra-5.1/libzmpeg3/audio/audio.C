#include "../libzmpeg3.h"

static class audio_decode_t {
  pthread_mutex_t zlock;
public:
  audio_decode_t() {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutex_init(&zlock, &attr);
  }
  ~audio_decode_t() {
    pthread_mutex_destroy(&zlock);
  }
  void lock() { pthread_mutex_lock(&zlock); }
  void unlock() { pthread_mutex_unlock(&zlock); }
} audio_decode;

static void toc_error()
{
  zerr( "mpeg3audio: sample accurate seeking without a table of contents \n"
    "is no longer supported.  Use mpeg3toc <mpeg file> <table of contents>\n"
    "to generate a table of contents and load the table of contents instead.\n");
}


int zaudio_t::
rewind_audio()
{
  if( track->sample_offsets )
    track->demuxer->seek_byte(track->sample_offsets[0]);
  else
    track->demuxer->seek_byte(0);
  track->reset_pts();
  packet_position = 0;
  return 0;
}

/* Advance to next header and read it. */
int zaudio_t::
read_header()
{
  int count = 0x10000;
  int got_it = 0;
  int result = 1;
  int pos = 0;

  switch(track->format) {
  case afmt_AC3:
    while( count > 0 && !track->demuxer->eof() ) {
      if( pos >= (int)sizeof(packet_buffer)-8 ) {
        memmove(packet_buffer, &packet_buffer[pos], packet_position-=pos);
        pos = 0;
      }
      if( packet_position-pos < 8 ) {
        packet_buffer[packet_position++] = track->demuxer->read_char();
        continue;
      }
      got_it = ac3_decoder->ac3_header(&packet_buffer[pos]);
      if( got_it ) break;
      ++pos;  --count;
    }

    if( got_it ) {
      channels = ac3_decoder->channels;
      samplerate = ac3_decoder->samplerate;
      framesize = ac3_decoder->framesize;
      result = 0;
    }
    break;

  case afmt_MPEG:
    /* Layer 1 not supported */
    if( layer_decoder->layer == 1 ) break;

    /* Load starting bytes and check format when not seekable */
    if( !src->seekable && !packet_position && !track->total_sample_offsets ) {
      if( src->demuxer->payload_unit_start_indicator ) {
        if( track->demuxer->read_data(packet_buffer,packet_position=4) ) break;
        /* reject mp4 data */
        if( !zaudio_decoder_layer_t::id3_check(packet_buffer) &&
              zaudio_decoder_layer_t::layer_check(packet_buffer) ) {
          zerrs("bad mp3 header, pid=%04x, track ignored\n", track->pid);
          track->format = afmt_IGNORE;
          result = -1;
        }
      }
    }

    while( count > 0 && !track->demuxer->eof() ) {
      if( pos >= (int)sizeof(packet_buffer)-4 ) {
        memmove(packet_buffer, &packet_buffer[pos], packet_position-=pos);
        pos = 0;
      }
      if( packet_position-pos < 4 ) {
        packet_buffer[packet_position++] = track->demuxer->read_char();
        continue;
      }
      got_it = layer_decoder->layer3_header(&packet_buffer[pos]);
      if( got_it ) break;
//zmsgs("%d got_it=%d packet=%02x%02x%02x%02x\n", __LINE__,
//  got_it, (uint8_t)packet_buffer[pos+0], (uint8_t)packet_buffer[pos+1],
//  (uint8_t)packet_buffer[pos+2], (uint8_t)packet_buffer[pos+3]);
      ++pos;  --count;
      /* ID3 tags need to reset the count to skip the tags */
      if( layer_decoder->id3_state != id3_IDLE ) count = 0x10000;
    }

    if( got_it ) {
      channels = layer_decoder->channels;
      samplerate = layer_decoder->samplerate;
      framesize = layer_decoder->framesize;
      result = 0;
    }
    break;

  case afmt_PCM:
    while( count > 0 && !track->demuxer->eof() ) {
      if( pos >= (int)sizeof(packet_buffer)-PCM_HEADERSIZE ) {
        memmove(packet_buffer, &packet_buffer[pos], packet_position-=pos);
        pos = 0;
      }
      if( packet_position-pos < PCM_HEADERSIZE ) {
        packet_buffer[packet_position++] = track->demuxer->read_char();
        continue;
      }
      got_it = pcm_decoder->pcm_header(&packet_buffer[pos]);
      if( got_it ) break;
      ++pos;  --count;
    }

    if( got_it ) {
      channels = pcm_decoder->channels;
      samplerate = pcm_decoder->samplerate;
      framesize = pcm_decoder->framesize;
      result = 0;
    }
    break;

  default:
    result = -1;
    break;
  }

  if( pos > 0 )
    memmove(packet_buffer, &packet_buffer[pos], packet_position-=pos);

  if( track->demuxer->error() )
    result = 1;
  if( !result ) {
    if( channels > track->channels )
      track->channels = channels;
    track->sample_rate = samplerate;
  }
//zmsgs("%d %d %d\n", track->channels, track->sample_rate, framesize);

  return result;
}

zaudio_t::
~audio_t()
{
  if( output ) {
    for( int i=0; i<output_channels; ++i )
      delete [] output[i];
    delete [] output;
  }
  if( ac3_decoder ) delete ac3_decoder;
  if( layer_decoder ) delete layer_decoder;
  if( pcm_decoder ) delete pcm_decoder;
}

void zaudio_t::
update_channels()
{
  int i;
  float **new_output = new float *[channels];
  if( output_channels < channels ) {
    for( i=0; i<output_channels; ++i )
      new_output[i] = output[i];
    while( i < channels ) // more channels
      new_output[i++] = new float[output_allocated];
  }
  else {
    for( i=0; i<channels; ++i )
      new_output[i] = output[i];
    while( i < output_channels ) // fewer channels
      delete [] output[i++];
  }
  delete [] output;
  output = new_output;
  output_channels = channels;
}

int zaudio_t::
read_frame(int render)
{
  zdemuxer_t *demux = track->demuxer;
  int samples = -1;

  /* Find and read next header */
  int result = read_header();
//if( src->seekable ) {
//  int64_t app_pos = track->apparent_position();
//  int64_t aud_pos = audio_position();
//  double atime = track->get_audio_time();
//  int64_t pts_pos = atime * track->sample_rate + 0.5;
//  zmsgs(" apr_pos %jd/%jd + %jd  %jd + %jd\n",
//    app_pos, aud_pos, app_pos-aud_pos, pts_pos, pts_pos-aud_pos);
//}
  if( !result ) {
    /* Handle changes in channel count, for ATSC */
    if( output_channels != channels )
      update_channels();
    /* try to read rest of frame */
    int len = framesize - packet_position;
    uint8_t *bfr = packet_buffer + packet_position;
    if( !src->seekable ) {
      int data_length = demux->zdata.length();
      if( data_length < len ) len = data_length;
    }
    if( !(result = demux->read_data(bfr, len)) )
      packet_position += len;
  }

  if( !result && packet_position >= framesize ) {
    float *out[output_channels];
    for( int i=0; i<output_channels; ++i )
      out[i] = output[i] + output_size;

    switch(track->format) {
    case afmt_AC3:
      audio_decode.lock(); /* Liba52 is not reentrant */
      samples = ac3_decoder->do_ac3(packet_buffer,framesize,out,render);
      audio_decode.unlock();
//zmsgs("ac3 %d samples\n", samples);
      break;
    case afmt_MPEG:
      switch( layer_decoder->layer ) {
      case 2:
        samples = layer_decoder->do_layer2(packet_buffer,framesize,out,render);
        break;
      case 3:
        samples = layer_decoder->do_layer3(packet_buffer,framesize,out,render);
        break;
      }
      break;
    case afmt_PCM:
      samples = pcm_decoder->do_pcm(packet_buffer,framesize,out,render);
      break;
    }
    if( samples > 0 )
      output_size += samples;
    packet_position = 0;
  }

  return samples;
}

/* Get the length. */
/* Use chunksize if demuxer has a table of contents */
/* For elementary streams use sample count over a certain number of */
/*  bytes to guess total samples */
/* For program streams use timecode */
int zaudio_t::
get_length()
{
  int result = 0;
  int samples = 0;
  int64_t stream_end = track->demuxer->stream_end;
  int64_t total_bytes = track->demuxer->movie_size();
  int64_t stream_max = total_bytes;
  if( stream_max > 0x1000000 ) stream_max = 0x1000000;
  track->demuxer->stream_end = stream_max;
  rewind_audio();
  /* Table of contents */
  if( track->sample_offsets || !src->is_audio_stream() ) {
    /* Estimate using multiplexed stream size in seconds */
    /* Get stream parameters for header validation */
    /* Need a table of contents */
    for( int retry=0; retry<100 && samples<=0; ++retry ) {
      samples = read_frame(0);
    }
    result = track->sample_offsets ? track->total_samples : 0;
    /* : (long)(track->demuxer->length() * track->sample_rate); */
  }
  else { /* Estimate using average bitrate */
    long test_bytes = 0;
    long max_bytes = 0x40000;
    long test_samples = 0;
    while( test_bytes < max_bytes ) {
      int samples = read_frame(0);
      if( samples <= 0 ) break;
      test_samples += samples;
      test_bytes += framesize;
    }
    result = (long)(((double)total_bytes / test_bytes) * test_samples + 0.5);
  }

  track->demuxer->stream_end = stream_end;
  output_size = 0;
  rewind_audio();
  return result;
}

int zatrack_t::
calculate_format(zmpeg3_t *src)
{
  uint8_t header[8];
  int result = 0;
  /* Determine the format of the stream.  */
  /* If it isn't in the first 8 bytes give up and go to a movie. */
  switch( format ) {
  case afmt_UNKNOWN:
    /* Need these 8 bytes later on for header parsing */
    result = demuxer->read_data(header, sizeof(header));
//zmsgs("%jd\n", demuxer->tell_byte());
    if( !result )
      format = !zaudio_decoder_ac3_t::ac3_check(header) ? afmt_AC3 : afmt_MPEG;
    if( audio ) {
      memcpy(audio->packet_buffer+1, header, sizeof(header));
      audio->packet_position = 9;
    }
    break;
  case afmt_IGNORE:
    result = 1;
    break;
  default:
    break;
  }

  return result;
}

int zaudio_t::
init_audio(zmpeg3_t *zsrc, zatrack_t *ztrack, int zformat)
{
  int result = 0;
  src = zsrc;
  track = ztrack;
  demuxer_t *demux = track->demuxer;
  byte_seek = -1;
  sample_seek = -1;
  track->format = zformat;

  if( zsrc->seekable )
    result = track->calculate_format(src);
//zmsgs("%jd\n", demux->tell_byte());
  /* get stream parameters */
  if( !result && zsrc->seekable ) {
    switch( track->format ) {
    case afmt_AC3:
      ac3_decoder = new audio_decoder_ac3_t();
      break;
    case afmt_MPEG:
      layer_decoder = new audio_decoder_layer_t();
      break;
    case afmt_PCM:
      pcm_decoder = new audio_decoder_pcm_t();
      break;
    default:
      result = 1;
    }
    if( !result ) {
      int64_t stream_end = demux->stream_end;
      //if( !track->sample_offsets )
      {
        if( zsrc->is_transport_stream() )
          demux->stream_end = START_BYTE + MAX_TS_PROBE;
        else if( src->is_program_stream() )
          demux->stream_end = START_BYTE + MAX_PGM_PROBE;
      }
      rewind_audio();
      result = read_header();
      if( !result && src->is_audio_stream() )
        start_byte = demux->tell_byte() - zsrc->packet_size;
      demux->stream_end = stream_end;
    }
//zmsgs("1 %d %d %d start_byte=0x%jx\n", track->format, 
// layer_decoder->layer, result, start_byte);
  }
  return result;
}

zaudio_t *zmpeg3_t::
new_audio_t(zatrack_t *ztrack, int zformat)
{
  audio_t *new_audio = new audio_t();
  int result = new_audio->init_audio(this,ztrack,zformat);
  /* Calculate Length */
  if( !result && seekable ) {
    new_audio->rewind_audio();
    ztrack->total_samples = new_audio->get_length();
  }
  else if( seekable ) {
    delete new_audio;
    new_audio = 0;
  }
  return new_audio;
}



int zaudio_t::
seek()
{
  demuxer_t *demux = track->demuxer;
  int seeked = 0;
/* Stream is unseekable */
  if( !src->seekable ) return 0;
/* Sample seek was requested */
  if( sample_seek >= 0 ) {
    sample_seek += track->nudge;
    if( sample_seek < 0 ) sample_seek = 0;
/* Doesn't work with VBR streams + ID3 tags */
//zmsgs("%d %jd %jd %jd %jd\n", __LINE__, sample_seek,
//  track->current_position, output_position, audio_position());
/* Don't do anything if the destination is inside the sample buffer */
    if( sample_seek < output_position ||
        sample_seek >= audio_position() ) {
      if( sample_seek != output_position ) {
        /* Use table of contents */
        if( track->sample_offsets ) {
          int index = sample_seek / AUDIO_CHUNKSIZE;
          if( index >= track->total_sample_offsets )
            index = track->total_sample_offsets - 1;
          /* incase of padding */
          int64_t byte = track->sample_offsets[index];
          while( index > 0 && byte == track->sample_offsets[index-1] ) --index;
          output_position = (int64_t)index * AUDIO_CHUNKSIZE;
          output_size = 0;
          demux->seek_byte(byte);
        }
        else if( sample_seek > 0 && !src->is_audio_stream() ) { /* Use demuxer */
          toc_error();
/*        double time_position = (double)sample_seek / track->sample_rate; */
/*        result |= mpeg3demux_seek_time(demuxer, time_position); */
          output_position = sample_seek;
        }
        else { /* Use bitrate for elementary stream */
          int64_t total_bytes = demux->movie_size() - start_byte;
          int64_t byte = (int64_t)(total_bytes * 
            ((double)sample_seek)/track->total_samples) + start_byte;
//zmsgs("%d byte=%jd\n", __LINE__, byte);
          output_position = sample_seek;
          output_size = 0;
          demux->seek_byte(byte);
        }
        seeked = 1;
      }
    }
    sample_seek = -1;
  }
  else if( byte_seek >= 0 ) {
    output_position = 0;
    output_size = 0;
    /* Percentage seek was requested */
    demux->seek_byte(byte_seek);
    /* Scan for pts if we're the first to seek. */
    /* if( src->percentage_pts < 0 ) */
    /*   file->percentage_pts = demux->scan_pts(); */
    /* else */
    /*   demux->goto_pts(src->percentage_pts); */
    seeked = 1;
    byte_seek = -1;
  }

  if( seeked ) {
    packet_position = 0;
    track->reset_pts();
    switch( track->format ) {
    case afmt_MPEG:
      layer_decoder->layer_reset();
      break;
    }
  }

  return 0;
}

/* ================================*/
/*          ENTRY POINTS           */
/* ================================*/

int zaudio_t::
seek_byte(int64_t byte)
{
  byte_seek = byte;
  return 0;
}

int zaudio_t::
seek_sample(long sample)
{
/* Doesn't work for rereading audio during percentage seeking */
/*  if(sample > track->total_samples) sample = track->total_samples; */
  sample_seek = (int64_t)sample;
  return 0;
}

/* Read raw frames for the mpeg3cat utility */
int zaudio_t::
read_raw(uint8_t *output, long *size, long max_size)
{
  *size = 0;
  switch( track->format ) {
  case afmt_AC3: /* Just write the AC3 stream */
    if( track->demuxer->read_data(output, 0x800) ) return 1;
    *size = 0x800;
    break;

  case afmt_MPEG: /* Fix the mpeg stream */
    if( track->demuxer->read_data(output, 0x800) ) return 1;
    *size = 0x800;
    break;
    
  case afmt_PCM: /* This is required to strip the headers */
    if( track->demuxer->read_data(output, framesize) ) return 1;
    *size = framesize;
    break;
  }

  return 0;
#if 0
/* This probably doesn't work. */
  result = read_header(audio);
  switch( track->format ) {
  case afmt_AC3: /* Just write the AC3 stream */
    result = track->demuxer->read_data(output, framesize);
    *size = framesize;
//zmsgs("1 %d\n", framesize);
    break;
  case afmt_MPEG: /* Fix the mpeg stream */
    if( !result ) {
      if(track->demuxer->read_data(output, framesize)) return 1;
      *size += framesize;
    }
    break;
  case afmt_PCM:
    if(track->demuxer->read_data(output, framesize)) return 1;
    *size = framesize;
    break;
  }
  return result;
#endif
}

void zaudio_t::
shift_audio(int64_t diff)
{
  int len = diff > 0 ? output_size - diff : 0;
  if( len > 0 ) {
    for( int i=0; i<output_channels; ++i )
      memmove(output[i], output[i]+diff, len*sizeof(*output[i]));
    output_position += diff;
    output_size -= diff;
  }
  else {
    output_position += output_size;
    output_size = 0;
  }
}

/* zero pad data to match pts */
int zaudio_t::
audio_pts_padding()
{
  int result = 0, silence = 0;
  if( track->audio_time >= 0 && src->pts_padding > 0 ) {
    int64_t pts_pos = track->audio_time * track->sample_rate + 0.5;
    int padding = pts_pos - track->pts_position;
    if( padding > 0 ) {
      if( track->askip <= 0 ) {
        int limit = track->sample_rate/8;
        if( padding > limit ) {
          int64_t aud_pos = audio_position();
          zmsgs("audio start padding  pid %02x @ %12jd (%12.6f) %d samples\n",
                track->pid, aud_pos, track->get_audio_time(), padding);
          track->askip = 1;
          result = 1;
        }
      }
      else if( padding > track->sample_rate )
        result = silence = 1;
      else if( !(++track->askip & 1) )
        result = 1;
      if( result ) {
        if( padding > AUDIO_MAX_DECODE ) padding = AUDIO_MAX_DECODE;
        int avail = output_allocated - output_size;
        if( padding > avail ) padding = avail;
        if( padding > 0 ) {
          int n = track->sample_rate/20;
          if( output_size < n ) silence = 1;
          if( silence ) {
            for( int chan=0; chan<output_channels; ++chan ) {
              float *out = output[chan] + output_size;
              for( int i=0; i<padding; ++i ) *out++ = 0;
            }
          }
          else {
            int k = output_size - n;
            for( int chan=0; chan<output_channels; ++chan ) {
              float *out = output[chan] + output_size;
              float *pad = output[chan] + k;
              for( int i=0; i<padding; ++i ) *out++ = *pad++;
            }
          }
          output_size += padding;
          track->pts_position += padding;
        }
      }
    }
    else if( track->askip > 0 ) {
      int64_t aud_pos = audio_position();
      zmsgs("audio end   padding  pid %02x @ %12jd (%12.6f) %d\n",
            track->pid, aud_pos, track->get_audio_time(), (1+track->askip)/2);
      track->askip = 0;
    }
  }
  return result;
}

int zaudio_t::
audio_pts_skipping(int samples)
{
  int result = 0;
  if( track->audio_time >= 0 && src->pts_padding > 0 ) {
    int64_t pts_pos = track->audio_time * track->sample_rate + 0.5;
    int skipping = track->pts_position - pts_pos;
    if( skipping > 0 ) {
      if( track->askip >= 0 ) {
        int limit = track->sample_rate/8;
        if( skipping > 3*limit ) {
          int64_t aud_pos = audio_position();
          zmsgs("audio start skipping pid %02x @ %12jd (%12.6f) %d samples\n",
                track->pid, aud_pos, track->get_audio_time(), skipping);
          track->askip = -1;
          result = 1;
        }
      }
      else if( !(--track->askip & 1) || skipping > track->sample_rate )
        result = 1;
      if( result ) {
        if( samples < skipping ) skipping = samples;
        if( output_size < skipping ) skipping = output_size;
        output_size -= skipping;
        track->pts_position -= skipping;
      }
    }
    else if( track->askip < 0 ) {
      int64_t aud_pos = audio_position();
      zmsgs("audio end   skipping pid %02x @ %12jd (%12.6f) %d\n",
            track->pid, aud_pos, track->get_audio_time(), (1-track->askip)/2);
      track->askip = 0;
    }
  }
  return result;
}

void zaudio_t::
update_audio_history()
{
  int64_t diff = track->track_position() - output_position;
  if( diff ) shift_audio(diff);
}

/* Channel is 0 to channels - 1 */
/* Always render since now the TOC contains index files. */
int zaudio_t::
decode_audio(void *output_v, int atyp, int channel, int len)
{
  int i, j, k;
  zdemuxer_t *demux = track->demuxer;
//zmsgs("decode_audio(out_pos=%jd-%jd, aud_pos %jd, seek %jd  ch=%d, len=%d)\n",
//  output_position, audio_position(), track->track_position(),
//  (sample_seek<0 ? sample_seek : sample_seek+track->nudge), channel, len);

  /* Get header for streaming mode */
  if( track->format == afmt_IGNORE ) return 1;
  if( track->format == afmt_UNKNOWN ) {
    if( track->calculate_format(src) ) return 1;
  }
  if( track->format == afmt_AC3 && !ac3_decoder )
    ac3_decoder = new audio_decoder_ac3_t();
  else if( track->format == afmt_MPEG && !layer_decoder )
    layer_decoder = new audio_decoder_layer_t();
  else if( track->format == afmt_PCM && !pcm_decoder )
    pcm_decoder = new audio_decoder_pcm_t();
  /* Handle seeking requests */
  seek();
  int64_t end_pos = track->track_position() + len;
  long new_size = end_pos - output_position + MAXFRAMESAMPLEZ;
  /* Expand output until enough room exists for new data */
  if( new_size > output_allocated ) {
    for( i=0; i<output_channels; ++i ) {
      float *old_output = output[i];
      float *new_output = new float[new_size];
      if( output_size ) {
        int len = output_size * sizeof(new_output[0]);
        memmove(new_output, old_output, len);
        len = (new_size-output_size) * sizeof(new_output[0]);
        memset(new_output+output_size, 0, len);
      }
      delete [] output[i];
      output[i] = new_output;
    }
    output_allocated = new_size;
  }


  /* Decode frames until the output is ready */
  int retry = 0;

  while( retry < 256 ) {
    int64_t aud_pos = audio_position();
    int64_t demand = end_pos - aud_pos;
    if( demand <= 0 ) break;
    if( audio_pts_padding() ) continue;
    if( output_allocated-output_size < AUDIO_MAX_DECODE ) break;
//zmsgs(" out_pos/sz %jd/%d=aud_pos %jd, end_pos %jd, demand %jd,
//  tell/eof %jd/%d\n", output_position, output_size, aud_pos, end_pos,
//  demand, demux->tell_byte(), demux->eof());
    if( demux->eof() ) break;
    /* if overflowing, shift audio back */
    if( src->seekable && output_size > AUDIO_HISTORY ) {
      int diff = demand + output_size - AUDIO_HISTORY;
      shift_audio(diff);
    }
    int samples = read_frame(1);
    if( samples < 0 ) break;
    if( !samples ) { ++retry; continue; }
    retry = 0;
    if( audio_pts_skipping(samples) ) continue;
    track->update_frame_pts();
    track->update_audio_time();
  }

  /* Copy the buffer to the output */
  if( !track->channels ) return 1;
  if( channel >= track->channels )
    channel = track->channels - 1;
  i = 0;
  j = track->track_position() - output_position;
  k = output_size - j;
  if( k > len ) k = len;
  float *out = channel < output_channels ?
    output[channel] + j : 0;

  /* transmit data in specified format */
  switch( atyp ) {
  case atyp_NONE:
    break;
  case atyp_DOUBLE: {
    double *output_d = (double *)output_v;
    if( out ) while( i < k )
      output_d[i++] = *out++;
    while( i < len ) output_d[i++] = 0.;
    break; }
  case atyp_FLOAT: {
    float *output_f = (float *)output_v;
    if( out )
      memmove(output_f,out,(i=k)*sizeof(output_f[0]));
    while( i < len ) output_f[i++] = 0.;
    break; }
  case atyp_INT: {
    int *output_i = (int *)output_v;
    if( out ) while( i < k )
      output_i[i++] = clip((int)(*out++ * 0x7fffff),-0x800000,0x7fffff);
    while( i < len ) output_i[i++] = 0;
    break; }
  case atyp_SHORT: {
    short *output_s = (short *)output_v;
    if( out ) while( i < k )
      output_s[i++] = clip((int)(*out++ * 0x7fff),-0x8000,0x7fff);
    while( i < len ) output_s[i++] = 0;
    break; }
  default:
    return 1;
  }

//zmsgs("output_size=%d chan=%d len=%d\n",output_size,channel,len);
  return output_size > 0 ? 0 : 1;
}

