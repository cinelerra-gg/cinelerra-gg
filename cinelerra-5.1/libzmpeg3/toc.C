#include "libzmpeg3.h"

#define PUT_INT32(x) do { \
 uint32_t be = htobe32(x); \
 fwrite((uint8_t*)&be,1,sizeof(be),toc_fp); \
} while(0)

#define PUT_INT64(x) do { \
 uint64_t be = htobe64(x); \
 fwrite((uint8_t*)&be,1,sizeof(be),toc_fp); \
} while(0)


//#define DATA_DUMP
#ifdef DATA_DUMP
#include <stdio.h>
static FILE **video_fp = 0;
static FILE **audio_fp = 0;
#endif

static inline void read_data(uint8_t *bfr, int &pos, uint8_t *out, int len)
{
  memcpy(out, bfr + pos, len);
  pos += len;
}

/* Concatenate title and toc directory if title is not absolute and */
/*  toc path has a directory section. */
static inline void concat_path(char *full_path, char *toc_path, char *path)
{
  if( path[0] != '/' ) {
    char *ptr = strrchr(toc_path, '/');
    if( ptr ) {
      strcpy(full_path, toc_path);
      strcpy(&full_path[ptr - toc_path + 1], path);
    }
  }
  else
    strcpy(full_path, path);
}

int zmpeg3_t::
read_toc(int *atracks_return, int *vtracks_return, const char *title_path)
{
  int i, j;
  int64_t current_byte = 0;
  int debug = 0;
  /* Fix title paths for Cinelerra VFS */
  int vfs_len = strlen(ZRENDERFARM_FS_PREFIX);
  int is_vfs = !strncmp(fs->path,ZRENDERFARM_FS_PREFIX, vfs_len) ? 1 : 0;
  fs->seek(0);
  /* Test version */
  fs->read_uint32();
  uint32_t toc_version = fs->read_uint32();
  if( toc_version != TOC_VERSION ) {
    zerrs("invalid TOC version %08x (should be %08x)\n", 
      toc_version, TOC_VERSION);
    return ERR_INVALID_TOC_VERSION;
  }
  /* File type */
  while( !fs->eof() ) {
  /* Get section type */
    int section_type = fs->read_uint32();
//zmsgs("section_type=%d position=%jx\n", section_type, fs->tell());
    switch( section_type ) {
      case toc_FILE_TYPE_PROGRAM:
        file_type = FT_PROGRAM_STREAM;
        break;
      case toc_FILE_TYPE_TRANSPORT:
        file_type = FT_TRANSPORT_STREAM;
        break;
      case toc_FILE_TYPE_AUDIO:
        file_type = FT_AUDIO_STREAM;
        break;
      case toc_FILE_TYPE_VIDEO:
        file_type = FT_VIDEO_STREAM;
        break;

      case toc_FILE_INFO: {
        char string[STRLEN];
        char string2[STRLEN];
        fs->read_data((uint8_t*)&string[0],STRLEN);
        if( title_path )
          strcpy(string2, title_path);
        else
          concat_path(string2, fs->path, string);
        source_date = fs->read_uint64();
        int64_t current_date = calculate_source_date(string2);
//zmsgs("zsrc=%s source_date=%jd current_date=%jd\n", 
// string2, source_date, current_date);
        if( current_date != source_date ) {
          zerrs("date mismatch %jx (should be %jx)\n",
             current_date, source_date);
          return ERR_TOC_DATE_MISMATCH;
        }
        break; }

      case toc_STREAM_AUDIO: {
        int n = fs->read_uint32();
        demuxer->astream_table[n] = fs->read_uint32();
        break; }

      case toc_STREAM_VIDEO: {
        int n = fs->read_uint32();
        demuxer->vstream_table[n] = fs->read_uint32();
        break; }


      case toc_ATRACK_COUNT: {
        int atracks = fs->read_uint32();
        *atracks_return = atracks;
        channel_counts = znew(int,atracks);
        nudging = znew(int,atracks);
        sample_offsets = znew(int64_t*,atracks);
        total_sample_offsets = znew(int,atracks);
        audio_eof = znew(int64_t,atracks);
        total_samples = znew(int64_t,atracks);
        indexes = znew(index_t*,atracks);
        total_indexes = atracks;
        for( i=0; i < atracks; ++i ) {
          audio_eof[i] = fs->read_uint64();
          channel_counts[i] = fs->read_uint32();
          nudging[i] = fs->read_uint32();
          total_sample_offsets[i] = fs->read_uint32();
          total_samples[i] = fs->read_uint64();

          if(total_samples[i] < 1) total_samples[i] = 1;
          sample_offsets[i] = znew(int64_t,total_sample_offsets[i]);
          for( j=0; j < total_sample_offsets[i]; ++j ) {
            sample_offsets[i][j] = fs->read_uint64();
          }

          index_t *index = indexes[i] = new index_t();
          index->index_size = fs->read_uint32();
          index->index_zoom = fs->read_uint32();
//zmsgs("%d %d %d\n", i, index->index_size, index->index_zoom);
          int channels = index->index_channels = channel_counts[i];
          if( channels ) {
            index->index_data = znew(float*,channels);
            for( j=0; j < channels; ++j ) {
              index->index_data[j] = znew(float,index->index_size*2);
              fs->read_data((uint8_t*)index->index_data[j], 
                sizeof(float) * index->index_size * 2);
            }
          }
        }
        break; }

      case toc_VTRACK_COUNT: {
        int vtracks = fs->read_uint32();
        *vtracks_return = vtracks;
        frame_offsets = znew(int64_t*,vtracks);
        total_frame_offsets = znew(int,vtracks);
        keyframe_numbers = znew(int*,vtracks);
        total_keyframe_numbers = znew(int,vtracks);
        video_eof = znew(int64_t,vtracks);
        for( i=0; i < vtracks; ++i ) {
          video_eof[i] = fs->read_uint64();
          total_frame_offsets[i] = fs->read_uint32();
          frame_offsets[i] = new int64_t[total_frame_offsets[i]];
          if( debug ) zmsgs("62 %d %jx %jx\n",
            total_frame_offsets[i], fs->tell(), fs->ztotal_bytes());
          for( j=0; j < total_frame_offsets[i]; ++j ) {
            frame_offsets[i][j] = fs->read_uint64();
//zmsgs("frame %llx\n", frame_offsets[i][j]);
          }
          if(debug) zmsg("64\n");
          total_keyframe_numbers[i] = fs->read_uint32();
          keyframe_numbers[i] = new int[total_keyframe_numbers[i]];
          for( j=0; j < total_keyframe_numbers[i]; ++j ) {
            keyframe_numbers[i][j] = fs->read_uint32();
          }
        }
        break; }

      case toc_STRACK_COUNT: {
        total_stracks = fs->read_uint32();
        for( i=0; i < total_stracks; ++i ) {
          int id = fs->read_uint32();
          strack_t *strk = new strack_t(id);
          strack[i] = strk;
          strk->total_offsets = fs->read_uint32();
          strk->offsets = znew(int64_t,strk->total_offsets);
          strk->allocated_offsets = strk->total_offsets;
          for( j=0; j < strk->total_offsets; ++j ) {
            strk->offsets[j] = fs->read_uint64();
          }
        }
        break; }

      case toc_TITLE_PATH: {
        if(debug) zmsg("11\n");
        char string[STRLEN];
        fs->read_data((uint8_t*)string, STRLEN);
        /* Detect Blu-Ray */
        if( debug ) printf("11 position=%jx\n", fs->tell());
        char *ext = strrchr(string, '.');
        if( ext ) {
          if( !strncasecmp(ext, ".m2ts", 5) ||
              !strncasecmp(ext, ".mts", 4) ) file_type |= FT_BD_FILE;
        }
        if(debug) printf("12\n");
        // if title_path concatenate dirname title_path and basename path
        if( title_path ) {
          char string2[STRLEN], *bp = string2, *cp = string2;
          for( const char *tp=title_path; (*cp++=*tp) != 0; ++tp )
            if( *tp == '/' ) bp = cp;
          for( char *sp=cp=string; *sp != 0; ++sp )
            if( *sp == '/' ) cp = sp+1;
          while( (*bp++=*cp++)!=0 );
          strcpy(string, string2);
        }
        // Test title availability
        if( access(string,R_OK) ) {
          /* Concatenate title and toc directory if title is not absolute and */
          /* toc path has a directory section. */
          if( (!is_vfs && string[0] != '/') ||
               (is_vfs && string[vfs_len] != '/') ) {
            /* Get toc filename without path */
            char *ptr = strrchr(fs->path, '/');
            if( ptr ) {
              char string2[STRLEN];
              /* Stack filename on toc path */
              strcpy(string2, fs->path);
              if( !is_vfs )
                strcpy(&string2[ptr - fs->path + 1], string);
              else
                strcpy(&string2[ptr - fs->path + 1], string + vfs_len);
              if( access(string2,R_OK) ) {
                zerrs("failed to open %s or %s\n", string, string2);
                return 1;
              }
              strcpy(string, string2);
            }
            else {
              zerrs("failed to open %s\n", string);
              return 1;
            }
          }
          else {
            zerrs("failed to open %s\n", string);
            return 1;
          }
        }

        if(debug) zmsg("30\n");
        demuxer_t::title_t *title = new demuxer_t::title_t(this, string);
        demuxer->titles[demuxer->total_titles++] = title;
        title->total_bytes = fs->read_uint64();
        title->start_byte = current_byte;
        title->end_byte = title->start_byte + title->total_bytes;
        current_byte = title->end_byte;
        /* Cells */
        title->cell_table_allocation = fs->read_uint32();
        title->cell_table_size = title->cell_table_allocation;
//zmsgs("40 %llx %d\n", title->total_bytes, title->cell_table_size);
        title->cell_table =
          new demuxer_t::title_t::cell_t[title->cell_table_size];
        for( i=0; i < title->cell_table_size; ++i ) {
          demuxer_t::title_t::cell_t *cell = &title->cell_table[i];
          cell->title_start = fs->read_uint64();
          cell->title_end = fs->read_uint64();
          cell->program_start = fs->read_uint64();
          cell->program_end = fs->read_uint64();
          union { double d; uint64_t u; } cell_time;
          cell_time.u = fs->read_uint64();
          cell->cell_time = cell_time.d;
          cell->cell_no = fs->read_uint32();
          cell->discontinuity = fs->read_uint32();
//zmsgs("50 %jx-%jx %jx-%jx %d %d\n", 
// cell->title_start, cell->title_end, cell->program_start,
// cell->program_end, cell->cell_no, cell->discontinuity);
        }
        break; }

      case toc_IFO_PALETTE:
        for(i = 0; i < 16; i++) {
          int k = i*4;
          palette[k + 0] = fs->read_char();
          palette[k + 1] = fs->read_char();
          palette[k + 2] = fs->read_char();
          palette[k + 3] = fs->read_char();
//zmsgs("color %02d: 0x%02x 0x%02x 0x%02x 0x%02x\n", 
// i, palette[k+0], palette[k+1], palette[k+2], palette[k+3]);
        }
        have_palette = 1;
        break;
      case toc_IFO_PLAYINFO: {
        int ncells = fs->read_uint32();
        icell_table_t *cells = new icell_table_t();
        if( playinfo ) delete playinfo;
        playinfo = cells;
        while( --ncells >= 0 ) {
          icell_t *cell = cells->append_cell();
          cell->start_byte = fs->read_uint64();
          cell->end_byte = fs->read_uint64();
          cell->vob_id = fs->read_uint32();
          cell->cell_id = fs->read_uint32();
          cell->inlv = fs->read_uint32();
          cell->discon = fs->read_uint32();
        }
        break;
      }
      case toc_SRC_PROGRAM: {
        vts_title = fs->read_uint32();
        total_vts_titles = fs->read_uint32();
        int inlv = fs->read_uint32();
        angle = inlv / 10;  interleave = inlv % 10;
        total_interleaves = fs->read_uint32();
        break;
      }
    }
  }
  if(debug) zmsg("90\n");
  demuxer->open_title(0);
  if(debug) zmsg("100\n");

  return 0;
}


zmpeg3_t* zmpeg3_t::
start_toc(const char *path, const char *toc_path,
    int program, int64_t *total_bytes)
{
  if( total_bytes ) *total_bytes = 0;
  zmpeg3_t *zsrc = new zmpeg3_t(path);
  zsrc->toc_fp = fopen(toc_path, "w");
  if(!zsrc->toc_fp) {
    perrs("%s",toc_path);
    delete zsrc;
    return 0;
  }
  
  zsrc->set_vts_title(program / 100);
  zsrc->set_angle((program / 10) % 10);
  zsrc->set_interleave(program % 10);
  zsrc->last_cell_no = 0;

  if( !total_bytes ) {
    zsrc->fs->sequential();
    zsrc->file_type = FT_TRANSPORT_STREAM;
    zsrc->packet_size = zsrc->calculate_packet_size();
  }

  /* Authenticate encryption before reading a single byte */
  /*  and Determine file type */
  int toc_atracks = 0, toc_vtracks = 0;
  if( zsrc->fs->open_file() ||
      (!zsrc->file_type &&
        zsrc->get_file_type(&toc_atracks, &toc_vtracks, 0) ) ) {
    fclose(zsrc->toc_fp);  zsrc->toc_fp = 0;
    delete zsrc;
    return 0;
  }

  demuxer_t *demux = zsrc->demuxer;
  /* Create title without scanning for tracks */
  if( !demux->total_titles ) {
    demuxer_t::title_t *title = new demuxer_t::title_t(zsrc);
    demux->titles[0] = title;
    demux->total_titles = 1;
    demux->open_title(0);
    title->total_bytes = title->fs->ztotal_bytes();
    title->start_byte = 0;
    title->end_byte = title->total_bytes;
    title->new_cell(title->end_byte);
  }

  /*  mpeg3demux_seek_byte(zsrc->demuxer, 0x1734e4800LL); */
  demux->seek_byte(0);
  demux->read_all = -1;
  int64_t bytes = demux->movie_size();
  if( total_bytes ) *total_bytes = bytes;

//*total_bytes = 500000000;
  zsrc->allocate_slice_decoders();
  return zsrc;
}

void zmpeg3_t::
divide_index(int track_number)
{
  if( total_indexes <= track_number ) return;
  index_t *idx = indexes[track_number];
  idx->index_size /= 2;
  idx->index_zoom *= 2;
  for( int i=0; i < idx->index_channels; ++i ) {
    float *current_channel = idx->index_data[i];
    float *out = current_channel;
    float *in = current_channel;
    for( int j=0; j < idx->index_size; ++j ) {
      float max = ZMAX(in[0], in[2]);
      float min = ZMIN(in[1], in[3]);
      *out++ = max;
      *out++ = min;
      in += 4;
    }
  }
}

int zmpeg3_t::
update_index(int track_number, int flush)
{
  int i, j, k, n;
  int result = 0;
  atrack_t *atrk = atrack[track_number];
  if( track_number >= total_indexes ) {
    /* Create index table */
    int new_allocation = track_number + 1;
    index_t **new_indexes = new index_t*[new_allocation];
    for( i=0; i<total_indexes; ++i )
      new_indexes[i] = indexes[i];
    while( i < new_allocation )
      new_indexes[i++] = new index_t();
    delete [] indexes;
    indexes = new_indexes;
    total_indexes = new_allocation;
  }
  index_t *idx = indexes[track_number];
  audio_t *aud = atrk->audio;
//zmsgs("%d atrk->audio->output_size=%d\n", __LINE__, aud->output_size);
  while( ( flush && aud->output_size) ||
         (!flush && aud->output_size > AUDIO_CHUNKSIZE) ) {
    int fragment = AUDIO_CHUNKSIZE;
    if( aud->output_size < fragment ) fragment = aud->output_size;
    int index_fragments = fragment / idx->index_zoom;
    if( flush ) ++index_fragments;
    int new_index_samples = index_fragments + idx->index_size;
    /* Update number of channels */
    if( idx->index_allocated && idx->index_channels < atrk->channels ) {
      float **new_index_data = znew(float*,atrk->channels);
      for( i=0; i < idx->index_channels; ++i )
        new_index_data[i] = idx->index_data[i];
      for( i=idx->index_channels; i < atrk->channels; ++i )
        new_index_data[i] = znew(float,idx->index_allocated*2);
      idx->index_channels = atrk->channels;
      delete [] idx->index_data;
      idx->index_data = new_index_data;
    }

    /* Allocate index buffer */
    if( new_index_samples > idx->index_allocated ) {
      /* Double current number of samples */
      idx->index_allocated = new_index_samples * 2;
      if( !idx->index_data ) {
        idx->index_data = znew(float*,atrk->channels);
      }
      /* Allocate new size in high and low pairs */
      k = idx->index_allocated * sizeof(float*) * 2;
      for( i=0; i < atrk->channels; ++i ) {
        float *old_data = idx->index_data[i];
        float *new_data = new float[k];
        n = idx->index_size*2;
        for( j=0; j<n; ++j ) new_data[j] = old_data[j];
        while( j < k ) new_data[j++] = 0.;
        delete [] idx->index_data[i];
        idx->index_data[i] = new_data;
      }
      idx->index_channels = atrk->channels;
    }

    /* Calculate new index chunk */
    for( i=0; i<atrk->channels; ++i ) {
      float *in_channel = i < aud->output_channels ? aud->output[i] : 0;
      float *out_channel = idx->index_data[i] + idx->index_size*2;
      /* Calculate index frames */
      for( j=0; j < index_fragments; ++j ) {
        float min = 0;
        float max = 0;
        int remaining_fragment = fragment - j*idx->index_zoom;
        int len = remaining_fragment < idx->index_zoom ?
          remaining_fragment : idx->index_zoom;
        if( in_channel && len > 0 ) {
          min = max = *in_channel++;
          for( k=1; k < len; ++k ) {
            if( *in_channel > max ) max = *in_channel;
            else if( *in_channel < min ) min = *in_channel;
            ++in_channel;
          }
        }
        *out_channel++ = max;
        *out_channel++ = min;
      }
    }

    idx->index_size = new_index_samples;
    /* Shift audio buffer */
    aud->shift_audio(fragment);
    /* Create new toc entry */
    atrk->append_samples(atrk->prev_offset);
    atrk->current_position += fragment;
    result = 1;
  }
  /* Divide index by 2 and increase zoom */
  k = idx->index_size * atrk->channels * sizeof(float) * 2;
  if( k > index_bytes && !(idx->index_size % 2) )
    divide_index(track_number);
  return result;
}


int zatrack_t::
handle_audio_data(int track_number)
{
  zmpeg3_t *zsrc = audio->src;
  /* Decode samples */
  while( !audio->decode_audio(0, atyp_NONE, 0, AUDIO_HISTORY) ) {
    /* add downsampled samples to the index buffer and create toc entry. */
    if( !zsrc->update_index(track_number, 0) ) break;
    prev_offset = curr_offset;
  }
  return 0;
}

int zatrack_t::
handle_audio(int track_number)
{
#ifdef DATA_DUMP
if( !audio_fp ) {
  audio_fp = new FILE*[65536];
  memset(audio_fp,0,65536*sizeof(FILE*));
}
if( !audio_fp[pid] ) {
  char fn[512];
  sprintf(fn,"/tmp/dat/a%04x.mpg",pid);
  audio_fp[pid] = fopen(fn,"w");
}
if( audio_fp[pid] ) {
  zmpeg3_t *zsrc = audio->src;
  demuxer_t *demux = zsrc->demuxer;
  if( demux->zaudio.size )
    fwrite(demux->zaudio.buffer, demux->zaudio.size, 1, audio_fp[pid]);
  else if( demux->zdata.size )
    fwrite(demux->zdata.buffer, demux->zdata.size, 1, audio_fp[pid]);
}
#endif

  int result = 0;
  if( format != afmt_IGNORE ) {
    zmpeg3_t *zsrc = audio->src;
    demuxer_t *demux = zsrc->demuxer;
    /* Assume last packet of stream */
    audio_eof = demux->tell_byte();
    /* Append demuxed data to track buffer */
    if( demux->zaudio.size )
      demuxer->append_data(demux->zaudio.buffer,demux->zaudio.size);
    else if( demux->zdata.size )
      demuxer->append_data(demux->zdata.buffer,demux->zdata.size);
    if( demux->pes_audio_pid == pid && demux->pes_audio_time >= 0 ) {
      demuxer->pes_audio_time = demux->pes_audio_time;
      demux->pes_audio_time = -1.;
    }
    if( !sample_rate ) {
      /* check for audio starting past first cell */
      if( zsrc->pts_padding > 0 && zsrc->cell_time >= 0. ) {
        audio_time = zsrc->cell_time;
        pts_position = 0;
        if( pts_starttime < 0. ) {
          pts_offset = zsrc->cell_time;
          pts_starttime = 0.;
        }
      }
    }
//zmsgs("%d pid=%p zaudio.size=%d zdata.size=%d\n", __LINE__,
//  demux->pid, demux->zaudio.size, demux->zdata.size);
#if 0
  if( pid == 0x1100 ) {
    static FILE *test = 0;
    if( !test ) test = fopen("/hmov/test.ac3", "w");
    fwrite(demux->zaudio.buffer,
           demux->zaudio.size, 1, test);
  }
#endif
    result = handle_audio_data(track_number);
    demuxer->shift_data();
  }
  return result;
}

int zvtrack_t::
handle_video_data(int track_number, int prev_data_size)
{
  zmpeg3_t *zsrc = video->src;
  /* Scan for a start code a certain number of bytes from the end of the  */
  /* buffer.  Then scan the header using the video decoder to get the  */
  /* repeat count. */
  while( demuxer->zdata.length() >= VIDEO_STREAM_SIZE ) {
    if( !demuxer->last_code ) {
      if( video->thumb && video->pict_type == video_t::pic_type_I ) {
        if( tcode >= SLICE_MIN_START && tcode <= SLICE_MAX_START ) {
          // start a new slice
          slice = video->get_slice_buffer();
//int vtrk, sbfr = slice - &video->slice_buffers[0];
//for( vtrk=0; vtrk<zsrc->total_vtracks && video!=zsrc->vtrack[vtrk]->video; ++vtrk );
//printf("get_slice_buffer %p vtrk=%d, sbfr=%d\n",slice,vtrk,sbfr);
          sbp = slice->data;
          for( int i=32; i>0; ) *sbp++ = (tcode>>(i-=8));
          sb_size = slice->buffer_allocation-4-4;
          tcode = ~0;
        }
      }

      if( !slice ) {
        for(;;) {
          if( (uint8_t)tcode != 0 ) {
            tcode = (tcode<<8) | demuxer->read_char();
            tcode = (tcode<<8) | demuxer->read_char();
          }
          tcode = (tcode<<8) | demuxer->read_char();
          if( (tcode&0xffffff) == PACKET_START_CODE_PREFIX ) break;
          if( demuxer->zdata.length() < VIDEO_STREAM_SIZE ) return 0;
        }
        tcode = (tcode<<8) | demuxer->read_char();
      }
      else {
        for(;;) {
          if( sb_size <= 0 ) {
            sbp = slice->expand_buffer(sbp - slice->data);
            sb_size = slice->buffer_size;
          }
          if( (uint8_t)tcode != 0 ) {
            sb_size -= 3;
            tcode = (tcode<<8) | (*sbp++ = demuxer->read_char());
            tcode = (tcode<<8) | (*sbp++ = demuxer->read_char());
          }
          else
            --sb_size;
          tcode = (tcode<<8) | (*sbp++ = demuxer->read_char());
          if( (tcode&0xffffff) == PACKET_START_CODE_PREFIX ) break;
          if( demuxer->zdata.length() < VIDEO_STREAM_SIZE ) return 0;
        }
        tcode = (tcode<<8) | (*sbp++ = demuxer->read_char());
        // finish slice
        slice->buffer_size = sbp - slice->data;
        *--sbp = 0;  sbp = 0;
//#define MULTI_THREADED
#ifdef MULTI_THREADED
// this is actually slower than single threaded
        zsrc->decode_slice(slice);
#else
        slice_decoder_t *decoder = &zsrc->slice_decoders[0];
        decoder->slice_buffer = slice;
        decoder->video = video;
        decoder->decode_slice();
        video->put_slice_buffer(slice);
#endif
        slice = 0;
      }
//zmsgs("trk=%d, pos=0x%04x,size=0x%04x,length=0x%04x, cur_pos 0x%012lx/0x%012lx, tcode=0x%08x\n",
// track_number, demuxer->zdata.position, demuxer->zdata.size, demuxer->zdata.length(),
// prev_offset, curr_offset, tcode);
      switch( tcode ) {
      case GOP_START_CODE:
      case PICTURE_START_CODE:
        if( !video->found_seqhdr ) continue;
        /* else fall through */
      case SEQUENCE_START_CODE:
        /* save start of packet, data before prev_data_size from prev_offset */
        if( prev_frame_offset == -1 )
          prev_frame_offset = demuxer->zdata.position < prev_data_size ?
            prev_offset : curr_offset;
        video->vstream->reset(tcode);
        break;
      default:
        continue;
      }

      /* Wait for decoders to finish */
      if( video->slice_active_locked ) {
        video->slice_active.lock();
        video->slice_active.unlock();
      }
 
      if( total_frame_offsets-1 > video->framenum ) { 
        video->framenum = total_frame_offsets-1;
        update_video_time();
        if( video->framenum >= 0 ) {
          int64_t last_frame_offset = frame_offsets[video->framenum];
          while( video->video_pts_padding() ) {
            append_frame(last_frame_offset, 0);
            video->framenum = total_frame_offsets-1;
          }
        }
        if( video->video_pts_skipping() )
          continue;
        update_frame_pts();
      }
      /* if last frame was P-Frame after I-Frame show thumbnail */
      if( video->thumb && video->decoder_initted ) {
        switch( video->pict_type ) {
        case video_t::pic_type_P:
          if( !video->ithumb ) break;
          video->ithumb = 0;
          zsrc->thumbnail_fn(zsrc->thumbnail_priv, track_number);
          break;
        case video_t::pic_type_I:
          video->ithumb = 1;
          break;
        }
      }
    }

    /* Use video decoder to get repeat count and field type. */
    /* Should never hit EOF in here.  This rereads up to the current ptr */
    /* since zdata.position isn't updated by handle_video. */
    /* if get_header fails, try this offset again with more data */
    int data_position = demuxer->zdata.position;
    int have_seqhdr = video->found_seqhdr;
    int32_t repeat_data = video->repeat_data;
    if( video->get_header() ) {
      if( video->found_seqhdr ) {
        /* try again with more data */
        demuxer->last_code = tcode;
        demuxer->zdata.position = data_position;
      }
      video->found_seqhdr = have_seqhdr;
      video->repeat_data = repeat_data;
      break;
    }

    if( !video->decoder_initted ) {
      video->init_decoder();
      width = video->horizontal_size;
      height = video->vertical_size;
      frame_rate = video->frame_rate;
    }

    /* check for video starting past first cell */
    if( !have_seqhdr && zsrc->pts_padding > 0 &&
        video_time < 0 && zsrc->cell_time >= 0. ) {
      video_time = zsrc->cell_time;
      pts_position = 0;
      if( pts_starttime < 0. ) {
        pts_offset = zsrc->cell_time;
        pts_starttime = 0.;
      }
    }

    video->secondfield = 0;
    if( video->pict_struct != video_t::pics_FRAME_PICTURE ) {
      if( video->got_top || video->got_bottom )
        video->secondfield = 1;
    }
    
    int field = video->secondfield ? 2 : 1;
    if( video->pict_struct == video_t::pics_TOP_FIELD )
      video->got_top = field;
    else if( video->pict_struct == video_t::pics_BOTTOM_FIELD )
      video->got_bottom = field;

    if( video->pict_struct == video_t::pics_FRAME_PICTURE ||
        video->secondfield || !video->pict_struct ) {
//zmsgs("video pid %02x framenum %d video_time %12.5f %c 0x%012lx rep %d\n",
//  pid, video->framenum, get_video_time(), "XIPBD"[video->pict_type],
//  prev_frame_offset, video->repeat_fields-2);
      if( video->pict_type == video_t::pic_type_I )
        got_keyframe = 1;
      append_frame(prev_frame_offset, got_keyframe);
      /* Add entry for every repeat count. */
      video->current_field += 2;
      while( video->repeat_fields-video->current_field >= 2 ) {
        append_frame(prev_frame_offset, 0);
        video->current_field += 2;
      }
      if( (video->repeat_fields-=video->current_field) < 0 )
        video->repeat_fields = 0;
      /* Shift out data from before frame */
      prev_frame_offset = -1;
      got_keyframe = 0;
    }
    else {
      // This was a TOP/BOTTOM FIELD and was not secondfield
      // Shift out data from this field
      if( video->pict_type == video_t::pic_type_I )
        got_keyframe = 1;
      video->current_field += 2;
    }
    tcode = video->vstream->show_bits(32);
    if( !video->repeat_fields ) video->current_field = 0;
    demuxer->last_code = 0;
  }
  return 0;
}

int zvtrack_t::
handle_video(int track_number)
{
  zmpeg3_t *zsrc = video->src;
  demuxer_t *demux = zsrc->demuxer;
  /* Assume last packet of stream */
  video_eof = demux->tell_byte();
  int prev_data_size = demuxer->zdata.size;
//zmsgs("%d %d %02x %02x %02x %02x %02x %02x %02x %02x\n", 
// demux->zvideo.size, demux->zdata.size,
// demux->zvideo.buffer[0], demux->zvideo.buffer[1],
// demux->zvideo.buffer[2], demux->zvideo.buffer[3],
// demux->zvideo.buffer[4], demux->zvideo.buffer[5],
// demux->zvideo.buffer[6], demux->zvideo.buffer[7]);

#ifdef DATA_DUMP
if( !video_fp ) {
  video_fp = new FILE*[65536];
  memset(video_fp,0,65536*sizeof(FILE*));
}
if( !video_fp[pid] ) {
  char fn[512];
  sprintf(fn,"/tmp/dat/v%04x.mpg",pid);
  video_fp[pid] = fopen(fn,"w");
}
if( video_fp[pid] ) {
  if( demux->zvideo.size )
    fwrite(demux->zvideo.buffer, demux->zvideo.size, 1, video_fp[pid]);
  else if( demux->zdata.size )
    fwrite(demux->zdata.buffer, demux->zdata.size, 1, video_fp[pid]);
}
#endif

  /* Append demuxed data to track buffer */
  if( demux->zvideo.size )
    demuxer->append_data(demux->zvideo.buffer, demux->zvideo.size);
  else if( demux->zdata.size )
    demuxer->append_data(demux->zdata.buffer, demux->zdata.size);
  if( demux->pes_video_pid == pid && demux->pes_video_time >= 0 ) {
    demuxer->pes_video_time = demux->pes_video_time;
    demux->pes_video_time = -1.;
  }
#if 0
  if( !test_file ) test_file = fopen("/tmp/test.m2v", "w");
  if( demuxer->zdata.position > 0 ) fwrite(demuxer->zdata.buffer,
           demuxer->zdata.position, 1, test_file);
#endif

  handle_video_data(track_number, prev_data_size);
  demuxer->shift_data();
  return 0;
}

void zmpeg3_t::
handle_subtitle()
{
  for( int i=0; i<total_stracks; ++i ) {
    strack_t *strk = get_strack(i);
    while( strk->total_subtitles ) {
      subtitle_t *subtitle = strk->subtitles[0];
      strk->append_subtitle_offset(subtitle->offset);
      strk->del_subtitle(subtitle);
    }
  }
}

void zmpeg3_t::
handle_cell(int cell_no)
{
  int idx;
  cell_time = -1.;

//zmsgs("cell %d\n", cell_no);
  for( idx=0; idx<total_vtracks; ++idx ) {
    zvtrack_t *vtrk = vtrack[idx];
    zvideo_t *vid = vtrk->video;
    if( !vid ) continue;
    double pts = vtrk->get_video_time();
//zmsgs("  vtrack %d = %f\n", idx, pts);
    if( cell_time < pts ) cell_time = pts;
  }
  for( idx=0; idx<total_atracks; ++idx ) {
    zatrack_t *atrk = atrack[idx];
    zaudio_t *aud = atrk->audio;
    if( !aud ) continue;
    double pts = atrk->get_audio_time();
//zmsgs("  atrack %d = %f\n", idx, pts);
    if( cell_time < pts ) cell_time = pts;
  }

  zcell_t *cell = 0;
  if( !demuxer->get_cell(cell_no, cell) )
    cell->cell_time = cell_time;
  if( pts_padding <= 0 ) return;

  int discon = cell ? cell->discontinuity : 0;
//zmsgs("cell %d cell_time=%f pos=%jx discon %d\n",
//  cell_no, cell_time, !cell? -1 : cell->program_start, discon);
  if( !discon ) return;

  for( idx=0; idx<total_vtracks; ++idx ) {
    zvtrack_t *vtrk = vtrack[idx];
    zvideo_t *vid = vtrk->video;
    if( !vid ) continue;
    if( !vtrk->total_frame_offsets ) continue;
    vtrk->video_time = cell_time;
    vid->framenum = vtrk->pts_position = vtrk->total_frame_offsets-1;
    if( vid->framenum >= 0 ) {
      int64_t prev_frame_offset = vtrk->frame_offsets[vid->framenum];
      while( vid->video_pts_padding() ) {
        vtrk->append_frame(prev_frame_offset, 0);
        vid->framenum = vtrk->total_frame_offsets-1;
      }
    }
    if( discon ) {
      vtrk->reset_pts();
      vtrk->pts_offset = cell_time;
      if( discon < 0 ) vtrk->pts_starttime = 0.;
    }
  }

  for( idx=0; idx<total_atracks; ++idx ) {
    zatrack_t *atrk = atrack[idx];
    zaudio_t *aud = atrk->audio;
    if( !aud ) continue;
    atrk->audio_time = cell_time;
    atrk->pts_position = aud->audio_position();
    while( aud->audio_pts_padding() ) {
      if( update_index(idx, 0) )
        atrk->prev_offset = atrk->curr_offset;
    }
    if( discon ) {
      atrk->reset_pts();
      atrk->pts_offset = cell_time;
      if( discon < 0 ) atrk->pts_starttime = 0.;
    }
  }
}

int zmpeg3_t::
handle_nudging()
{
  int i, n, atrk, vtrk;
  int ret = 0;
  for( i=0; i<total_atracks; ++i )
    atrack[i]->nudge = 0;

  int total_channels = dvb.channel_count();
#if 1
// lines up first apts/vpts
  for( n=0; n<total_channels; ++n ) {
    int astreams, vstreams;
    dvb.total_vstreams(n, vstreams);
    double apts = -1., vpts = -1.;
    for( i=0; i<vstreams; ++i ) {
      dvb.vstream_number(n, i, vtrk);
      if( vtrack[vtrk]->pts_origin > vpts )
        vpts = vtrack[vtrk]->pts_origin;
    }
    dvb.total_astreams(n, astreams);
    for( i=0; i<astreams; ++i ) {
      dvb.astream_number(n, i, atrk);
      double samplerate = sample_rate(atrk);
      if( samplerate < 0. ) samplerate = 0.;
      apts = atrack[atrk]->pts_origin;
      if( apts >= 0. && vpts >= 0. ) {
        atrack[atrk]->nudge = (long)((vpts - apts) * samplerate);
        ++ret;
      }
    }
  }
#else
// lines up nearest apts/vpts
  class elem_t {
  public:
    elem_t *next;  int vtrk, atrk;
    double apts, vpts, pts_nudge;
    uint64_t apos, vpos, pts_dist;
    elem_t(elem_t *p, int vtrk, int atrk)
    : next(p), vtrk(vtrk), atrk(atrk) {
      apos = vpos = 0; apts = vpts = 0.;
      pts_dist = ~0;   pts_nudge = 0;
    }
  } *ep, *elems = 0;
  // collect all vtrk[0]-atrk[i] associations
  for( n=0; n<total_channels; ++n ) {
    int astreams, vstreams;
    dvb.total_vstreams(n, vstreams);
    if( vstreams <= 0 ) continue;
    dvb.vstream_number(n, 0, vtrk);
    if( vtrk < 0 || vtrk >= total_vtracks ) continue;
    dvb.total_astreams(n, astreams);
    for( i=0; i<astreams; ++i ) {
      dvb.astream_number(n, i, atrk);
      if( atrk < 0 || atrk >= total_atracks ) continue;
      elems = new elem_t(elems, vtrk, atrk);
    }
  }

  demuxer->pes_audio_time = -1.;
  demuxer->pes_video_time = -1.;
  demuxer->seek_byte(START_BYTE);

  // probe for atrk/vtrk pts nearest to each other in file
  while( !demuxer->titles[0]->fs->eof() ) {
    int64_t pos = demuxer->absolute_position();
    if( pos > START_BYTE + MAX_TS_PROBE ) break;
    if( demuxer->read_next_packet() ) break;
    ep = 0;
    if( demuxer->pes_audio_time >= 0 ) {
      for( ep=elems; ep && atrack[ep->atrk]->pid!=demuxer->pid; ep=ep->next );
      if( ep ) { ep->apos = pos; ep->apts = demuxer->pes_audio_time; }
      demuxer->pes_audio_time = -1.;
    }
    if( demuxer->pes_video_time >= 0 ) {
      for( ep=elems; ep && vtrack[ep->vtrk]->pid!=demuxer->pid; ep=ep->next );
      if( ep ) { ep->vpos = pos; ep->vpts = demuxer->pes_video_time; }
      demuxer->pes_video_time = -1.;
    }
    if( !ep || !ep->apos || !ep->vpos ) continue;
    int64_t dist = ep->apos - ep->vpos;
    if( dist < 0 ) dist = -dist;
    if( (uint64_t)dist < ep->pts_dist ) {
      ep->pts_dist = dist;
      ep->pts_nudge = ep->vpts - ep->apts;
    }
  }

  // nudge audio to align timestamps
  while( elems ) {
    ep = elems;  elems = elems->next;
//zmsgs("v/atrk %d/%d, dist %jd\n", ep->vtrk, ep->atrk, ep->pts_dist);
//zmsgs("   v/apts %f/%f, nudge %f\n", ep->vpts, ep->apts, ep->pts_nudge);
//zmsgs("   v/aorg %f/%f, nudge %f\n",
// vtrack[ep->vtrk]->pts_origin, atrack[ep->atrk]->pts_origin,
// vtrack[ep->vtrk]->pts_origin-atrack[ep->atrk]->pts_origin);
    double samplerate = sample_rate(ep->atrk);
    if( samplerate < 0. ) samplerate = 0.;
    if( ep->apts >= 0. && ep->vpts >= 0. ) {
      atrack[ep->atrk]->nudge = (long)(ep->pts_nudge * samplerate);
      ++ret;
    }
    delete ep;
  }
#endif
  return ret;
}

int zmpeg3_t::
do_toc(int64_t *bytes_processed)
{
  /* Starting byte before our packet read */
  int64_t start_byte = demuxer->tell_byte();
//zmsgs("%d offset=%llx file_type=%x\n", __LINE__, 
//  start_byte, file_type);
  int result = demuxer->seek_phys();
  if( !result ) {
    int cell_no = demuxer->current_cell_no();
    if( last_cell_no != cell_no )
      handle_cell(last_cell_no=cell_no);
    result = demuxer->read_next_packet();
  }
//zmsgs("%d %d %llx\n", __LINE__, result, demuxer->tell_byte());
  /* if(start_byte > 0x1b0000 && start_byte < 0x1c0000) */
//zmsgs("1 start_byte=%llx custum_id=%x got_audio=%d got_video=%d"
//      " zaudio.size=%d zvideo.size=%d zdata.size=%d\n", start_byte, 
// demuxer->custom_id, demuxer->got_audio, demuxer->got_video,
// demuxer->zaudio.size, demuxer->zvideo.size, demuxer->zdata.size);
  if( !result ) {
    int idx = 0;
    /* Find current PID in tracks. */
    /* Got subtitle */
    if( demuxer->got_subtitle )
      handle_subtitle();
    if( is_transport_stream() )
      dvb.atsc_tables(demuxer, demuxer->custom_id);
     /* In a transport stream the audio or video is determined by the PID. */
     /* In other streams the data type is determined by stream ID. */
    if( demuxer->got_audio >= 0 || is_transport_stream() || is_audio_stream() ) {
      int audio_pid = is_transport_stream() ? demuxer->custom_id : demuxer->got_audio;
      atrack_t *atrk = 0;
      for( idx=0; idx < total_atracks; ++idx ) {
        if( atrack[idx]->pid == audio_pid ) { /* Update an audio track */
          atrk = atrack[idx];
          break;
        }
      }
      if( !atrk ) {
        if( is_audio_stream() ||
            (audio_pid >= 0 && demuxer->astream_table[audio_pid]) ) {
          int format = demuxer->astream_table[audio_pid];
          atrk = new_atrack_t(audio_pid, format, demuxer, total_atracks);
          if( atrk ) {
            atrack[idx=total_atracks++] = atrk;
            atrk->prev_offset = start_byte;
          }
        }
      }
      if( atrk ) {
        atrk->curr_offset = start_byte;
        atrk->handle_audio(idx);
      }
    }
    if( demuxer->got_video >= 0 || is_transport_stream() || is_video_stream() ) {
      int video_pid = is_transport_stream() ? demuxer->custom_id : demuxer->got_video;
      vtrack_t *vtrk = 0;
      for( idx=0; idx < total_vtracks; ++idx ) {
        if( vtrack[idx]->pid == video_pid ) { /* Update a video track */
          vtrk = vtrack[idx];
          break;
        }
      }
      if( !vtrk ) {
        if( is_video_stream() ||
            (video_pid >= 0 && demuxer->vstream_table[video_pid]) ) {
          vtrk = new_vtrack_t(video_pid, demuxer, total_vtracks);
          /* Make the first offset correspond to the */
          /*  start of the first packet. */
          if( vtrk ) {
            vtrack[total_vtracks++] = vtrk;
            vtrk->tcode = ~0;
            vtrk->prev_frame_offset = -1;
            vtrk->curr_offset = start_byte;
            if( thumbnail_fn )
              vtrk->video->thumb = vtrk->video->skim = 1;
          }
        }
      }
      if( vtrk ) {
        vtrk->prev_offset = vtrk->curr_offset;
        vtrk->curr_offset = start_byte;
        vtrk->handle_video(idx);
      }
    }
  }
  /* Make user value independant of data type in packet */
  *bytes_processed = demuxer->tell_byte();
//zmsgs("1000 %jx\n", *bytes_processed);
  return 0;
}

void zmpeg3_t::
stop_toc()
{
  delete_slice_decoders();
  if( !is_ifo_file() ) {
    int64_t total_bytes = demuxer->tell_byte();
    demuxer->end_title(total_bytes);
  }
  /* Create final chunk for audio tracks to count the last samples. */
  int i, j, k;
  atrack_t *atrk;
  index_t *idx;
  /* Flush audio indexes */
  for( i=0; i < total_atracks; ++i )
    update_index(i, 1);
  for( i=0; i < total_atracks; ++i ) {
    atrk = atrack[i];
    atrk->append_samples(atrk->curr_offset);
  }

  /* Make all indexes the same scale */
  int max_scale = 1;
  for( i=0; i < total_atracks; ++i ) {
    atrk = atrack[i];
    idx = indexes[i];
    if( idx->index_data && idx->index_zoom > max_scale )
      max_scale = idx->index_zoom;
  }
  for( i=0; i < total_atracks; ++i ) {
    atrk = atrack[i];
    idx = indexes[i];
    if( idx->index_data && idx->index_zoom < max_scale ) {
      while( idx->index_zoom < max_scale )
        divide_index(i);
    }
  }
  /* delete vtracks with no frames */
  for( i=0; i < total_vtracks; ) {
    if( !vtrack[i]->total_frame_offsets ) {
      demuxer->vstream_table[vtrack[i]->pid] = 0;
      delete vtrack[i];
      --total_vtracks;
      for( j=i; j < total_vtracks; ++j )
        vtrack[j] = vtrack[j+1];
      continue;
    }
    ++i;
  }
  for( i=0; i < MAX_STREAMZ; ++i ) {
    if( demuxer->vstream_table[i] ) {
      for( j=total_vtracks; --j>=0; )
        if( vtrack[j]->pid == i ) break;
      if( j < 0 )
        demuxer->vstream_table[i] = 0;
    }
  }
  /* delete atracks with no samples */
  for( i=0; i < total_atracks; ) {
    if( !atrack[i]->total_sample_offsets || atrack[i]->format == afmt_IGNORE ) {
      demuxer->astream_table[atrack[i]->pid] = 0;
      delete atrack[i];  delete indexes[i];
      --total_atracks; --total_indexes;
      for( j=i; j < total_atracks; ++j ) {
        indexes[j] = indexes[j+1];
        atrack[j] = atrack[j+1];
      }
      continue;
    }
    ++i;
  }
  for( i=0; i < MAX_STREAMZ; ++i ) {
    if( demuxer->astream_table[i] ) {
      for( j=total_atracks; --j>=0; )
        if( atrack[j]->pid == i ) break;
      if( j < 0 )
        demuxer->astream_table[i] = 0;
    }
  }
  /* Sort tracks by PID */
  int done = 0;
  while( !done ) {
    done = 1;
    for( i=1; i < total_atracks; ++i ) {
      atrack_t *atrk0 = atrack[i - 0];
      atrack_t *atrk1 = atrack[i - 1];
      if( atrk1->pid > atrk0->pid ) {
        atrack[i - 1] = atrk0;
        atrack[i - 0] = atrk1;
        index_t *idx0 = indexes[i - 0];
        index_t *idx1 = indexes[i - 1];
        indexes[i - 1] = idx0;
        indexes[i - 0] = idx1;
        done = 0;
      }
    }
  }
  for( i=0; i < total_atracks; ++i )
    atrack[i]->number = i;
  done = 0;
  while( !done ) {
    done = 1;
    for( i=1; i < total_vtracks; ++i ) {
      vtrack_t *vtrk0 = vtrack[i - 0];
      vtrack_t *vtrk1 = vtrack[i - 1];
      if( vtrk1->pid > vtrk0->pid ) {
        vtrack[i - 1] = vtrk0;
        vtrack[i - 0] = vtrk1;
        done = 0;
      }
    }
  }
  for( i=0; i < total_vtracks; ++i )
    vtrack[i]->number = i;

  if( is_transport_stream() )
    handle_nudging();

  demuxer->read_all = 0;

  /* Output toc to file */
  /* Write file type */
  fputc('T', toc_fp);
  fputc('O', toc_fp);
  fputc('C', toc_fp);
  fputc(' ', toc_fp);
  /* Write version */
  PUT_INT32(TOC_VERSION);
  /* Write program */
  PUT_INT32(toc_SRC_PROGRAM);
  PUT_INT32(vts_title);
  PUT_INT32(total_vts_titles);
  PUT_INT32(angle*10 + interleave);
  PUT_INT32(total_interleaves);
  /* Write stream type */
  if( is_program_stream() )
    PUT_INT32(toc_FILE_TYPE_PROGRAM);
  else if( is_transport_stream() )
    PUT_INT32(toc_FILE_TYPE_TRANSPORT);
  else if( is_audio_stream() )
    PUT_INT32(toc_FILE_TYPE_AUDIO);
  else if( is_video_stream() )
    PUT_INT32(toc_FILE_TYPE_VIDEO);
  /* Store file information */
  PUT_INT32(toc_FILE_INFO);
  char *path = fs->path;
  fprintf(toc_fp, "%s", path);
  for( j=strlen(path); j < STRLEN; ++j ) fputc(0, toc_fp);
  source_date = calculate_source_date(path);
  PUT_INT64(source_date);
  /* Write stream ID's */
  /* Only program and transport streams have these */
  for( i=0; i < MAX_STREAMZ; ++i ) {
    if( demuxer->astream_table[i] ) {
      PUT_INT32(toc_STREAM_AUDIO);
      PUT_INT32(i);
      PUT_INT32(demuxer->astream_table[i]);
    }
    if( demuxer->vstream_table[i] ) {
      PUT_INT32(toc_STREAM_VIDEO);
      PUT_INT32(i);
      PUT_INT32(demuxer->vstream_table[i]);
    }
  }
  /* Write titles */
  for( i=0; i < demuxer->total_titles; ++i ) {
    demuxer_t::title_t *title = demuxer->titles[i];
    /* Path */
    PUT_INT32(toc_TITLE_PATH);
    path = !is_program_stream() ? path : title->fs->path;
    fprintf(toc_fp, "%s", path);
    for( j=strlen(path); j < STRLEN; ++j ) fputc(0, toc_fp);
    /* Total bytes */
    PUT_INT64(title->total_bytes);
    /* Total cells in title */
    PUT_INT32(demuxer->titles[i]->cell_table_size);
    for( j=0; j < title->cell_table_size; ++j ) {
      demuxer_t::title_t::cell_t *cell = &title->cell_table[j];
//zmsgs("%x: %llx-%llx %llx-%llx %d %d %d\n",
//  ftell(toc_fp), cell->title_start, cell->title_end,
//  cell->program_start, cell->program_end, cell->cell_no,
//  cell->program, cell->discontinuity);
      PUT_INT64(cell->title_start);
      PUT_INT64(cell->title_end);
      PUT_INT64(cell->program_start);
      PUT_INT64(cell->program_end);
      union { double d; uint64_t u; } cell_time;
      cell_time.d = cell->cell_time;
      PUT_INT64(cell_time.u);
      PUT_INT32(cell->cell_no);
      PUT_INT32(cell->discontinuity);
    }
  }
  PUT_INT32(toc_ATRACK_COUNT);
  PUT_INT32(total_atracks);
  /* Audio streams */
  for( j=0; j < total_atracks; ++j ) {
    atrack_t *atrk = atrack[j];
    PUT_INT64(atrk->audio_eof);
    PUT_INT32(atrk->channels);
    PUT_INT32(atrk->nudge);
    PUT_INT32(atrk->total_sample_offsets);
    /* Total samples */
    PUT_INT64(atrk->current_position);
    /* Sample offsets */
    for( i=0; i < atrk->total_sample_offsets; ++i )
      PUT_INT64(atrk->sample_offsets[i]);
    /* Index */
    index_t *idx = indexes[j];
    if( idx->index_data ) {
      PUT_INT32(idx->index_size);
      PUT_INT32(idx->index_zoom);
      for( k=0; k < atrk->channels; ++k ) {
        fwrite(idx->index_data[k], 2*sizeof(float), 
            idx->index_size, toc_fp);
      }
    }
    else {
      PUT_INT32(0);
      PUT_INT32(1);
    }
  }
  PUT_INT32(toc_VTRACK_COUNT);
  PUT_INT32(total_vtracks);
  /* Video streams */
  for( j=0; j < total_vtracks; ++j ) {
    vtrack_t *vtrk = vtrack[j];
    PUT_INT64(vtrk->video_eof);
    PUT_INT32(vtrk->total_frame_offsets);
    for( i=0; i < vtrk->total_frame_offsets; ++i )
      PUT_INT64(vtrk->frame_offsets[i]);
    PUT_INT32(vtrk->total_keyframe_numbers);
    for( i=0; i < vtrk->total_keyframe_numbers; ++i )
      PUT_INT32(vtrk->keyframe_numbers[i]);
  }
  PUT_INT32(toc_STRACK_COUNT);
  PUT_INT32(total_stracks);
  /* Subtitle tracks */
  for( i=0; i < total_stracks; ++i ) {
    strack_t *strk = strack[i];
    PUT_INT32(strk->id);
    PUT_INT32(strk->total_offsets);
    for(j = 0; j < strk->total_offsets; j++)
      PUT_INT64(strk->offsets[j]);
  }
  PUT_INT32(toc_IFO_PALETTE);
  for( i=0; i < 16*4; ++i )
    fputc(palette[i], toc_fp);
  if( playinfo ) {
    PUT_INT32(toc_IFO_PLAYINFO);
    PUT_INT32(playinfo->total_cells);
    icell_t *cell = &playinfo->cells[0];
    for( i=playinfo->total_cells; --i>=0; ++cell ) {
      PUT_INT64(cell->start_byte);
      PUT_INT64(cell->end_byte);
      PUT_INT32(cell->vob_id);
      PUT_INT32(cell->cell_id);
      PUT_INT32(cell->inlv);
      PUT_INT32(cell->discon);
    }
  }

  fs->close_file();
  fclose(toc_fp);
  delete this;
}

int zmpeg3_t::
show_toc(int flags)
{
  int i, j, k;
  int64_t current_byte = 0;

  fs->seek(0);
  fs->read_uint32();
  uint32_t toc_version = fs->read_uint32();
  if( toc_version != TOC_VERSION ) {
    zmsgs("invalid TOC version %08x (should be %08x)\n", 
      toc_version, TOC_VERSION);
    return ERR_INVALID_TOC_VERSION;
  }

  while( !fs->eof() ) {
  /* Get section type */
    int section_type = fs->read_uint32();
    switch( section_type ) {
      case toc_FILE_TYPE_PROGRAM:
        zmsg("is_program_stream\n");
        break;

      case toc_FILE_TYPE_TRANSPORT:
        zmsg("is_transport_stream\n");
        break;

      case toc_FILE_TYPE_AUDIO:
        zmsg("is_audio_stream\n");
        break;

      case toc_FILE_TYPE_VIDEO:
        zmsg("is_video_stream\n");
        break;

      case toc_FILE_INFO: {
        char string[STRLEN];
        char string2[STRLEN];
        fs->read_data((uint8_t*)&string[0],STRLEN);
        concat_path(string2, fs->path, string);
        int64_t sdate = fs->read_uint64();
        zmsgs("zsrc=%s source_date=%jd\n", string2, sdate);
        break; }

      case toc_STREAM_AUDIO: {
        static const char *afmts[] = {
          "unknown", "mpeg", "ac3", "pcm", "aac", "jesus",
        };
        int n = fs->read_uint32();
        int fmt = fs->read_uint32();
        if( fmt < 0 || fmt >= lengthof(afmts) ) fmt = 0;
        zmsgs("  audio stream 0x%04x is %s\n", n, afmts[fmt]);
        break; }

      case toc_STREAM_VIDEO: {
        int n = fs->read_uint32();
        fs->read_uint32();
        zmsgs("  video stream 0x%04x\n", n);
        break; }


      case toc_ATRACK_COUNT: {
        zmsg("\n");
        int atracks = fs->read_uint32();
        zmsgs("audio tracks = %d\n", atracks);
        for( i=0; i < atracks; ++i ) {
          int64_t eof = fs->read_uint64();
          int channels = fs->read_uint32();
          int nudge = fs->read_uint32();
          int total_sample_offsets = fs->read_uint32();
          int64_t total_samples = fs->read_uint64();
          zmsgs(" audio %d, eof=0x%012jx ch=%d, offsets=%d samples=%jd nudge=%d\n",
            i, eof, channels, total_sample_offsets, total_samples, nudge);
          if( flags & show_toc_SAMPLE_OFFSETS ) {
            for( j=0; j < total_sample_offsets; ) {
              if( !(j&7) ) zmsgs(" %8d: ",j*AUDIO_CHUNKSIZE);
              printf(" %08jx", fs->read_uint64());
              if( !(++j&7) ) printf("\n");
            }
            if( (j&7) ) printf("\n");
          }
          else
            fs->seek_relative(sizeof(int64_t) * total_sample_offsets);
          int index_size = fs->read_uint32();
          int index_zoom = fs->read_uint32();
          zmsgs(" index_size=%d index_zoom=%d\n", index_size, index_zoom);
          for( j=0; j < channels; ++j ) {
            if( flags & show_toc_AUDIO_INDEX ) {
              zmsgs("audio track %d, channel %d\n",i,j);
              for( k=0; k<index_size; ) {
                union { int i; float f; } min, max;
                min.i = bswap_32(fs->read_uint32());
                max.i = bswap_32(fs->read_uint32());
                if( !(k&3) ) zmsgs(" %08x: ",k*index_zoom);
                printf(" %5.3f/%-5.3f",min.f,max.f);
                if( !(++k&3) ) printf("\n");
              }
              if( (k&3) ) printf("\n");
            }
            else
              fs->seek_relative(sizeof(float) * index_size * 2);
          }
        }
        break; }

      case toc_VTRACK_COUNT: {
        zmsg("\n");
        int vtracks = fs->read_uint32();
        zmsgs("video tracks = %d\n", vtracks);
        for( i=0; i < vtracks; ++i ) {
          int64_t eof = fs->read_uint64();
          int total_frame_offsets = fs->read_uint32();
          if( flags & show_toc_FRAME_OFFSETS ) {
            int64_t *frame_offsets = new int64_t[total_frame_offsets];
            for( k=0; k<total_frame_offsets; ++k ) {
              frame_offsets[k] = fs->read_uint64();
            }
            int total_keyframes = fs->read_uint32();
            zmsgs(" video %d, eof=0x%012jx offsets=%d keyframes=%d\n",
              i, eof, total_frame_offsets, total_keyframes);
            int *keyframe_numbers = new int[total_keyframes];
            for( j=0; j<total_keyframes; ++j ) {
              keyframe_numbers[j] = fs->read_uint32();
            }
            for( j=k=0; k<total_frame_offsets; ) {
              if( !(k&7) ) zmsgs(" %8d: ",k);
              if( k >= keyframe_numbers[j] ) {
                printf(" *");  ++j;
              }
              else
                printf("  ");
              printf("%08jx",frame_offsets[k]);
              if( !(++k&7) ) printf("\n");
            }
            if( (k&7) ) printf("\n");
            delete [] keyframe_numbers;
            delete [] frame_offsets;
          }
          else {
            fs->seek_relative(sizeof(int64_t) * total_frame_offsets);
            int total_keyframes = fs->read_uint32();
            zmsgs(" video %d, eof=0x%012jx offsets=%d keyframes=%d\n",
              i, eof, total_frame_offsets, total_keyframes);
            fs->seek_relative(sizeof(int) * total_keyframes);
          }
        }
        break; }

      case toc_STRACK_COUNT: {
        total_stracks = fs->read_uint32();
        for( i=0; i < total_stracks; ++i ) {
          int id = fs->read_uint32();
          int total_offsets = fs->read_uint32();
          for( j=0; j < total_offsets; ++j ) {
            fs->read_uint64();
          }
          zmsgs(" subtitle %d, id=0x%04x offsets=%d\n",
            i, id, total_offsets);
        }
        break; }

      case toc_TITLE_PATH: {
        char string[STRLEN];
        fs->read_data((uint8_t*)string, STRLEN);
        int64_t total_bytes = fs->read_uint64();
        int64_t start_byte = current_byte;
        int64_t end_byte = start_byte + total_bytes;
        current_byte = end_byte;
        zmsgs("title %s,\n", &string[0]);
        zmsgs(" bytes=0x%012jx, start=0x%012jx, end=0x%012jx\n",
          total_bytes, start_byte, end_byte);
        int cell_table_size = fs->read_uint32();
        for( i=0; i < cell_table_size; ++i ) {
          int64_t title_start = fs->read_uint64();
          int64_t title_end = fs->read_uint64();
          int64_t program_start = fs->read_uint64();
          int64_t program_end = fs->read_uint64();
          union { double d; uint64_t u; } cell_time;
          cell_time.u = fs->read_uint64();
          int cell_no = fs->read_uint32();
          int discontinuity = fs->read_uint32();
          zmsgs("  cell[%2d],  title 0x%012jx-0x%012jx, cell_no %2d, cell_time %5.3f\n",
            i, title_start, title_end, cell_no, cell_time.d);
          zmsgs("     discon %d program 0x%012jx-0x%012jx\n",
            discontinuity, program_start, program_end);
        }
        break; }

      case toc_IFO_PALETTE:
        zmsg("\n");
        zmsg("palette\n");
        for( i=0; i<16/4; ++i ) {
          uint32_t rgb0 = fs->read_uint32();
          uint32_t rgb1 = fs->read_uint32();
          uint32_t rgb2 = fs->read_uint32();
          uint32_t rgb3 = fs->read_uint32();
          zmsgs(" 0x%08x 0x%08x 0x%08x 0x%08x\n", rgb0, rgb1, rgb2, rgb3);
        }
        break;

      case toc_IFO_PLAYINFO: {
        zmsg("\n");
        int ncells = fs->read_uint32();
        zmsgs("playcells %d\n", ncells);
        for( i=0; i<ncells; ++i) {
          int64_t start_byte = fs->read_uint64();
          int64_t end_byte = fs->read_uint64();
          int vob_id = fs->read_uint32();
          int cell_id = fs->read_uint32();
          int inlv = fs->read_uint32();
          int discon = fs->read_uint32();
          zmsgs("  cell[%2d], start=0x%012jx, end=0x%012jx discon %d\n",
            i, start_byte, end_byte, discon);
          zmsgs("             vob_id %d, cell_id %d, inlv %d\n",
            vob_id, cell_id, inlv);
        }
        break;
      }

      case toc_SRC_PROGRAM: {
        int vtitl = fs->read_uint32();
        int vtotl = fs->read_uint32();
        int inlv = fs->read_uint32();
        int tinlv = fs->read_uint32();
        zmsgs("vts_title %d\n",vtitl);
        zmsgs("total_vts_title %d\n",vtotl);
        zmsgs("interleave/angle %d/%d\n",inlv%10,inlv/10);
        zmsgs("total_interleaves %d\n",tinlv);
        break;
      }
    }
  }

  return 0;
}

