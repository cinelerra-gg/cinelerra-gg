#include "libzmpeg3.h"

#define ABS(x) ((x) >= 0 ? (x) : -(x))

#if 0
#define rowsz 16

static void
dmp(unsigned char *bp,int l)
{
   int i, c, n, fad;
   unsigned char ch[rowsz];
   fad = 0;
   c = *bp++;  --l;
   while( l >= 0 ) {
      printf(" %04x -",fad);
      for( i=0; i<rowsz && l>=0; ++i ) {
         printf(" %02x",c);
         c &= 0x7f;
         ch[i] = c>=' ' && c<='~' ? c : ' ';
         c = *bp++;  --l;
      }
      for( n=i; i<rowsz; ++i ) printf("   ");
      printf("   ");
      for( i=0; i<n; ++i ) printf("%c", ch[i]);
      printf("\n");  fad += n;
   }
}
#endif

int zdemuxer_t::
get_adaptation_field()
{
//zmsgs("get_adaptation_field %d\n", adaptation_field_control);
  ++adaptation_fields;
  /* get adaptation field length */
  int length = packet_read_char();
  int avail = raw_size - raw_offset;
  if( length > avail ) length = avail;
  if( length > 0 ) {
    int flags = packet_read_char();
    if( ((flags >> 7) & 1) != 0 )
      discontinuity = 1;
    int pcr_flag = (flags >> 4) & 1;           
    if( pcr_flag ) {
      uint32_t clk_ref_base = packet_read_int32();
      uint32_t clk_ref_ext = packet_read_int16();
      if (clk_ref_base > 0x7fffffff ) { /* correct for invalid numbers */
        clk_ref_base = 0;               /* ie. longer than 32 bits when multiplied by 2 */
        clk_ref_ext = 0;                /* multiplied by 2 corresponds to shift left 1 (<<=1) */
      }
      else {                            /* Create space for bit */
        clk_ref_base <<= 1; 
        clk_ref_base |= (clk_ref_ext >> 15); /* Take bit */
        clk_ref_ext &= 0x01ff;             /* Only lower 9 bits */
      }
      time = ((double)clk_ref_base + clk_ref_ext / 300) / 90000;
      if( length )
        packet_skip(length-7);
      if( dump ) zmsgs(" pcr_flag=%x time=%f\n", pcr_flag, time);
    }
    else
      packet_skip(length-1);
  }
  return 0;
}

int zdemuxer_t::
get_program_association_table()
{
  ++program_association_tables;
  table_id = packet_read_char();
  section_length = packet_read_int16() & 0xfff;
  transport_stream_id = packet_read_int16();
  packet_skip(raw_size - raw_offset);
  if( dump ) {
    zmsgs("table_id=0x%x section_length=%d transport_stream_id=0x%x\n",
      table_id, section_length, transport_stream_id);
  }
  return 0;
}

int zdemuxer_t::
get_transport_payload(int is_audio, int is_video)
{
  int bytes = raw_size - raw_offset;
  if( bytes < 0 ) {
//    zerr("got negative payload size!\n");
    return 1;
  }
/*  if( zdata.size + bytes > RAW_SIZE )
      bytes = RAW_SIZE - zdata.size; */
//zmsgs("2 %d %d %d\n", bytes, read_all, is_audio);

  if( read_all && is_audio ) {
// if( pid == 0x1100 )
//   zmsgs("1 0x%x %d\n", audio_pid, bytes);
    memcpy(zaudio.buffer+zaudio.size, raw_data+raw_offset, bytes);
    zaudio.size += bytes;
  }
  else if( read_all && is_video ) {
//zmsgs("2 0x%x %d\n", video_pid, bytes);
    memcpy(zvideo.buffer+zvideo.size, raw_data+raw_offset, bytes);
    zvideo.size += bytes;
  }
  else {
    memcpy(zdata.buffer+zdata.size, raw_data+raw_offset, bytes);
    zdata.size += bytes;
//zmsgs("10 pid=0x%x bytes=0x%x zdata.size=0x%x\n", pid, bytes, zdata.size);
  }
  raw_offset += bytes;
  return 0;
}

int zdemuxer_t::
get_pes_packet_header(uint64_t *pts, uint64_t *dts)
{
  uint32_t pes_header_bytes = 0;

  /* drop first 8 bits */
  packet_read_char();  
  uint32_t pts_dts_flags = (packet_read_char() >> 6) & 0x3;
  int pes_header_data_length = packet_read_char();

  /* Get Presentation Time stamps and Decoding Time Stamps */
  if( pts_dts_flags & 2 )  {
    uint64_t tpts = (packet_read_char() >> 1) & 7;  /* Only low 4 bits (7==1111) */
    tpts <<= 15;
    tpts |= (packet_read_int16() >> 1);
    tpts <<= 15;
    tpts |= (packet_read_int16() >> 1);
    *pts = tpts;
    if( pts_dts_flags & 1 ) {
      uint64_t tdts = (packet_read_char() >> 1) & 7;  /* Only low 4 bits (7==1111) */
      tdts <<= 15;
      tdts |= (packet_read_int16() >> 1);
      tdts <<= 15;
      tdts |= (packet_read_int16() >> 1);
      *dts = tdts;
      pes_header_bytes = 10;
    }
    else
      pes_header_bytes = 5;
  }
  /* extract other stuff here! */
  packet_skip(pes_header_data_length-pes_header_bytes);
  time = (double)*pts / 90000;
  if( dump ) {
    zmsgs("pos=0x%012jx pid=%04x  pts=%f dts=%f pts_dts_flags=0x%02x\n",
      absolute_position(), pid, (double)*pts / 90000, (double)*dts / 90000, pts_dts_flags);
  }
  return 0;
}

int zdemuxer_t::
get_unknown_data()
{
  int bytes = raw_size - raw_offset;
  memcpy(zdata.buffer+zdata.size, raw_data+raw_offset, bytes);
  zdata.size += bytes;
  raw_offset += bytes;
  return 0;
}

int zdemuxer_t::
get_transport_pes_packet()
{
  uint64_t pts = 0, dts = 0;
  get_pes_packet_header(&pts, &dts);
//zmsgs("get_transport_pes_packet: stream_id=%x\n", stream_id);
  /* check ac3 audio (0xbd) or Blu-Ray audio (0xfd) */
  if( stream_id == 0xbd || stream_id == 0xfd ) {
//zmsgs("get_transport_pes_packet %x\n", pid);
    /* AC3 audio */
    stream_id = 0x0;
    custom_id = got_audio = pid;

    if( read_all ) astream_table[custom_id] = afmt_AC3;
    if( astream == -1 ) astream = custom_id;
    if( dump ) {
      zmsgs("offset=0x%jx 0x%x bytes AC3 custom_id=0x%x astream=0x%x do_audio=%p\n", 
        absolute_position(), raw_size - raw_offset,
        custom_id, astream, do_audio);
    }
    if( (custom_id == astream && do_audio) || read_all ) {
      audio_pid = pid;
      set_audio_pts(pts, 90000.);
      return get_transport_payload(1, 0);
    }
  }
  else if( (stream_id >> 4) == 0x0c || (stream_id >> 4) == 0x0d ) {
    /* MPEG audio */
    custom_id = got_audio = pid;
    /* Just pick the first available stream if no ID is set */
    if( read_all ) astream_table[custom_id] = afmt_MPEG;
    if( astream == -1 ) astream = custom_id;
    if( dump ) zmsgs("   0x%x bytes MP2 audio\n", raw_size-raw_offset);
    if( (custom_id == astream && do_audio) || read_all ) {
      audio_pid = pid;
      set_audio_pts(pts, 90000.);
      return get_transport_payload(1, 0);
    }
  }
  else if( (stream_id >> 4) == 0x0e ) {
    /* Video */
    custom_id = got_video = pid;

    /* Just pick the first available stream if no ID is set */
    if( read_all )
      vstream_table[custom_id] = 1;
    else if( vstream == -1 )
      vstream = custom_id;
    if( dump ) zmsgs("   0x%x bytes video data\n", raw_size - raw_offset);
    if( (custom_id == vstream && do_video) || read_all ) {
      video_pid = pid;
      set_video_pts(pts, 90000.);
      return get_transport_payload(0, 1);
    }
  }
  packet_skip(raw_size - raw_offset);
  return 0;
}

int zdemuxer_t::
get_pes_packet()
{
  ++pes_packets;
  /* Skip startcode */
  packet_read_int24();
  stream_id = packet_read_char();
  if( dump ) zmsgs("  stream_id=0x%02x\n", stream_id);

  /* Skip pes packet length */
  packet_read_int16();

  if( (stream_id >= 0xc0 && stream_id < 0xf0) ||
      stream_id == 0x0bd || stream_id == 0x0fd )
    return get_transport_pes_packet();

  switch( stream_id ) {
  case PRIVATE_STREAM_2:
    zmsg("stream_id == PRIVATE_STREAM_2\n");
  case PADDING_STREAM:
    packet_skip(raw_size-raw_offset);
    return 0;
  }

  if( src->log_errs )
    zmsgs("unknown stream_id (0x%02x) in pes packet\n", stream_id);
  packet_skip(raw_size-raw_offset);
  return 1;
}

int zdemuxer_t::
get_payload()
{
//zmsgs("get_payload 1 pid=0x%x unit_start=%d\n", pid, payload_unit_start_indicator);
  if( payload_unit_start_indicator ) {
    if( pid == 0 )
      get_program_association_table();
#ifdef ZDVB
    else if( read_all && src->dvb.atsc_pid(pid) )
      get_transport_payload(0, 0);
#endif
    else if( packet_next_int24() == PACKET_START_CODE_PREFIX )
      get_pes_packet();
    else
      packet_skip(raw_size-raw_offset);
  }
  else {
    if( dump ) zmsgs(" 0x%x bytes elementary data\n", raw_size-raw_offset);
// if( pid == 0x1100 ) zmsgs("get_payload 1 0x%x\n", audio_pid);
    if( pid == audio_pid && (do_audio || read_all) ) {
      if( do_audio ) got_audio = audio_pid;
      if( dump ) {
        zmsgs(" offset=0x%jx 0x%x bytes AC3 pid=0x%x\n", 
          absolute_position(), raw_size-raw_offset, pid);
      }
      get_transport_payload(1, 0);
    }
    else if( pid == video_pid && (do_video || read_all) ) {
      if( do_video ) got_video = video_pid;
      get_transport_payload(0, 1);
    }
    else {
      if( read_all ) {
        get_transport_payload(0, 0);
      }
/*    packet_skip(raw_size-raw_offset); */
    }
  }
  return 0;
}

/* Read a transport packet */
int zdemuxer_t::
read_transport()
{
  uint32_t bits = 0;
  int table_entry;
  title_t *title = titles[current_title];
  //dump = 1;

  /* Packet size is known for transport streams */
  raw_size = src->packet_size;
  raw_offset = 0;
  stream_id = 0;
  got_audio = -1;
  got_video = -1;
  custom_id = -1;

//zerrs("read transport 1 %jx %jx\n", title->fs->current_byte, title->fs->total_bytes);

  /* Skip BD or AVC-HD header */
  if( src->is_bd() )
    title->fs->read_uint32();

  /* Search for Sync byte */
  for( int i=0x10000; --i>=0 && !title->fs->eof(); ) {
    if( (bits=title->fs->read_char()) == SYNC_BYTE ) break;
  }

  program_byte = absolute_position();
  /* Store byte just read as first byte */
  if( !title->fs->eof() && bits == SYNC_BYTE ) {
    last_packet_start = program_byte-1;
    raw_data[0] = SYNC_BYTE;
    /* Read packet */
    int fragment_size = src->packet_size - 1;
    /* Skip BD header */
    if( src->is_bd() ) {
      fragment_size -= 4;
      raw_size -= 4;
    }
    title->fs->read_data(raw_data+1, fragment_size);
  }
  else {
    /* Failed */
    return 1;
  }

  /* Sync byte */
  packet_read_char();
  bits =  packet_read_int24() & 0x00ffffff;
  transport_error_indicator = (bits >> 23) & 0x1;
  payload_unit_start_indicator = (bits >> 22) & 0x1;
  pid = custom_id = (bits >> 8) & 0x00001fff;

  transport_scrambling_control = (bits >> 6) & 0x3;
  adaptation_field_control = (bits >> 4) & 0x3;
  continuity_counter = bits & 0xf;

  is_padding = pid == 0x1fff ? 1 : 0;
  if( dump ) {
    zmsgs("offset=0x%jx pid=0x%02x continuity=0x%02x padding=%d adaptation=%d unit_start=%d\n", 
      last_packet_start, pid, continuity_counter, is_padding,
      adaptation_field_control, payload_unit_start_indicator);
  }

  /* Abort if padding.  Should abort after pid == 0x1fff for speed. */
  if( is_padding || transport_error_indicator || (!read_all && (
       (do_video && do_video->pid != pid) ||
       (do_audio && do_audio->pid != pid))) ) {
    program_byte = absolute_position();
    return 0;
  }

  /* Get pid from table */
  int result = 0;
  for( table_entry=0; table_entry<total_pids; ++table_entry ) {
    if( pid == pid_table[table_entry] ) {
      result = 1;
      break;
    }
  }

  /* Not in pid table */
  if( !result && total_pids<PIDMAX ) {
    pid_table[table_entry] = pid;
    continuity_counters[table_entry] = continuity_counter;  /* init */
    total_pids++;
  }

  result = 0;
  if( adaptation_field_control & 0x2 )
    result = get_adaptation_field();

  /* Need to enter in astream and vstream table: */
  /* PID ored with stream_id */
  if( adaptation_field_control & 0x1 )
    result = get_payload();

  program_byte = absolute_position();
  return result;
}

int zdemuxer_t::
get_system_header()
{
  title_t *title = titles[current_title];
  int length = title->fs->read_uint16();
  title->fs->seek_relative(length);
  return 0;
}

uint64_t zdemuxer_t::
get_timestamp()
{
  uint64_t timestamp;
  title_t *title = titles[current_title];
  /* Only low 4 bits (7==1111) */
  timestamp = (title->fs->read_char() >> 1) & 7;  
  timestamp <<= 15;
  timestamp |= (title->fs->read_uint16() >> 1);
  timestamp <<= 15;
  timestamp |= (title->fs->read_uint16() >> 1);
  return timestamp;
}

int zdemuxer_t::
get_pack_header()
{
  uint32_t i, j;
  uint32_t clock_ref, clock_ref_ext;
  title_t *title = titles[current_title];

  /* Get the time code */
  if( (title->fs->next_char() >> 4) == 2 ) {
    time = (double)get_timestamp() / 90000; /* MPEG-1 */
    title->fs->read_uint24();               /* Skip 3 bytes */
  }
  else if( (title->fs->next_char() & 0x40) ) {
    i = title->fs->read_uint32();
    j = title->fs->read_uint16();
    if( (i & 0x40000000) || (i >> 28) == 2 ) {
      clock_ref = ((i & 0x38000000) << 3);
      clock_ref |= ((i & 0x03fff800) << 4);
      clock_ref |= ((i & 0x000003ff) << 5);
      clock_ref |= ((j & 0xf800) >> 11);
      clock_ref_ext = (j >> 1) & 0x1ff;
      time = (double)(clock_ref + clock_ref_ext / 300) / 90000;
      /* Skip 3 bytes */
      title->fs->read_uint24();
      i = title->fs->read_char() & 0x7;
      /* stuffing */
      title->fs->seek_relative(i);  
    }
  }
  else {
    title->fs->seek_relative(2);
  }
  return 0;
}

int zdemuxer_t::
get_program_payload(int bytes, int is_audio, int is_video)
{
  title_t *title = titles[current_title];
  int n = 0;
  if( read_all && is_audio ) {
    if( (n=zaudio.allocated-zaudio.size) > bytes ) n = bytes;
    title->fs->read_data(zaudio.buffer+zaudio.size, n);
    zaudio.size += n;
  }
  else if( read_all && is_video ) {
    if( (n=zvideo.allocated-zvideo.size) > bytes ) n = bytes;
    title->fs->read_data(zvideo.buffer+zvideo.size, n);
    zvideo.size += n;
  }
  else {
    if( (n=zdata.allocated-zdata.size) > bytes ) n = bytes;
    title->fs->read_data(zdata.buffer+zdata.size, n);
    zdata.size += n;
  }
  if( bytes > n )
    title->fs->seek_relative(bytes - n);
  return 0;
}

int zdemuxer_t::
handle_scrambling(int decryption_offset)
{
  title_t *title = titles[current_title];
  /* Advance 2048 bytes if scrambled.  We might pick up a spurrius */
  /* packet start code in the scrambled data otherwise. */
  int64_t next_packet_position = last_packet_start + DVD_PACKET_SIZE;
  if( next_packet_position > absolute_position() ) {
    title->fs->seek_relative(next_packet_position-absolute_position());
  }
  /* Descramble if desired. */
  if( zdata.size || zaudio.size || zvideo.size ) {
    uint8_t *buffer_ptr = 0;
    if( zdata.size ) buffer_ptr = zdata.buffer;
    else if( zaudio.size ) buffer_ptr = zaudio.buffer;
    else if( zvideo.size ) buffer_ptr = zvideo.buffer;
//zmsgs(" zdata.size=%x decryption_offset=%x\n",
//  zdata.size, decryption_offset);
    if( title->fs->css.decrypt_packet(buffer_ptr, decryption_offset)) {
      zerr("handle_scrambling: Decryption not available\n");
      return 1;
    }
  }
  return 0;
}

void zdemuxer_t::
del_subtitle(int idx)
{
  delete subtitles[idx];
  for( int i=idx; ++i<total_subtitles; )
    subtitles[i-1] = subtitles[i];
  --total_subtitles;
}

/* Create new subtitle object if none with the same id is in the table */
zsubtitle_t* zdemuxer_t::
get_subtitle(int id, int64_t offset)
{
//zmsgs(" id=0x%04x ofs=%012lx\n", id, offset);
  subtitle_t *subtitle = 0, *reuse = 0;
  /* Get current/reuse subtitle object for the stream */
  /* delete extra expired subtitles */
  int i = 0;
  while( i < total_subtitles ) {
    subtitle_t *sp = subtitles[i];
    if( sp->done < 0 ) {
      if( reuse ) { del_subtitle(i);  continue; }
      reuse = sp;
    }
    else if( !subtitle && !sp->done && sp->id == id )
      subtitle = sp;
    ++i;
  }
  /* found current unfinished subtitle */
  if( subtitle ) return subtitle;
  /* Make new/reused subtitle object */
  if( reuse ) {
    subtitle = reuse;
    subtitle->data_used = 0;
    subtitle->done = 0;
    subtitle->draw = 0;
    subtitle->active = 0;
    subtitle->force = 0;
    subtitle->frame_time = 0;
  }
  else if( total_subtitles < MAX_SUBTITLES ) {
    subtitle = new subtitle_t();
    subtitles[total_subtitles++] = subtitle;
  }
  else
    return 0;
  subtitle->id = id;
  subtitle->offset = offset;
  return subtitle;
}

void zsubtitle_t::
realloc_data(int n)
{
  uint8_t *new_data = new uint8_t[n];
  memcpy(new_data,data,data_used);
  delete [] data;  data = new_data;
  data_allocated = n;
}

void zdemuxer_t::
handle_subtitle(zmpeg3_t *src, int stream_id, int bytes)
{
  int pos = zdata.position;
  int size = zdata.size - zdata.position;
  zdata.size = zdata.position = 0;
  if( size < bytes ) bytes = size;
  subtitle_t *subtitle = get_subtitle(stream_id, program_byte);
  if( !subtitle ) return;
  size = subtitle->data_used + bytes;
  if( subtitle->data_allocated < size ) subtitle->realloc_data(size);
  memcpy(subtitle->data+subtitle->data_used,zdata.buffer+pos,bytes);
  subtitle->data_used += bytes;
  if( subtitle->data_used >= 2 ) {
    uint8_t *bfr = subtitle->data;
    size = (bfr[0]<<8) + bfr[1];
    if( subtitle->data_used >= size ) {
      zvideo_t *vid = do_video ? do_video->video : 0;
      strack_t *strack = src->create_strack(subtitle->id, vid);
      if( strack->append_subtitle(subtitle) )
        subtitle->done = -1;
      else
        got_subtitle = 1;
    }
  }
}

int zdemuxer_t::
handle_pcm(int bytes)
{
  /* Synthesize PCM header and delete MPEG header for PCM data */
  /* Must be done after decryption */
  uint8_t zcode;
  int bits_code;
  int bits;
  int samplerate_code;
  int samplerate;
  uint8_t *output = 0;
  uint8_t *data_buffer = 0;
  int data_start = 0;
  int *data_size = 0;
  int i, j;

  if( read_all && zaudio.size ) {
    output = zaudio.buffer + zaudio.start;
    data_buffer = zaudio.buffer;
    data_start = zaudio.start;
    data_size = &zaudio.size;
  }
  else {
    output = zdata.buffer + zdata.start;
    data_buffer = zdata.buffer;
    data_start = zdata.start;
    data_size = &zdata.size;
  }

  /* Shift audio back */
  zcode = output[1];
  j = *data_size+zaudio_t::PCM_HEADERSIZE-3-1; 
  for( i=*data_size-1; i>=data_start; --i,--j )
    *(data_buffer+j) = *(data_buffer+i);
  *data_size += zaudio_t::PCM_HEADERSIZE - 3;

  bits_code = (zcode >> 6) & 3;
  samplerate_code = (zcode >>4) & 1;

  output[0] = 0x7f;
  output[1] = 0x7f;
  output[2] = 0x80;
  output[3] = 0x7f;
  /* Samplerate */
  switch( samplerate_code ) {
  case 1:  samplerate = 96000; break;
  default: samplerate = 48000; break;
  }
  *(int32_t*)(output + 4) = samplerate;
  /* Bits */
  switch( bits_code ) {
    case 0:   bits = 16;  break;
    case 1:   bits = 20;  break;
    case 2:   bits = 24;  break;
    default:  bits = 16;  break;            
  }
  *(int32_t*)(output+ 8) = bits;
  *(int32_t*)(output+12) = (zcode & 0x7) + 1; /* Channels */
  *(int32_t*)(output+16) = bytes - 3 +
     zaudio_t::PCM_HEADERSIZE;               /* Framesize */
//zmsgs(" %d %d %d\n", *(int32_t*)(output+ 8),
//  *(int32_t*)(output+12), *(int32_t*)(output+16));
  return 0;
}

/* Program packet reading core */
int zdemuxer_t::
get_program_pes_packet( uint32_t header)
{
  uint64_t pts = 0, dts = 0;
  int pes_packet_length;
  int64_t pes_packet_start, pes_packet_end;
  int decryption_offset = 0;
  title_t *title = titles[current_title];
  int scrambling = 0;
  int do_pcm = 0;
  int do_subtitle = 0;

  zdata.start = zdata.size;
  zaudio.start = zaudio.size;
  zvideo.start = zvideo.size;

  stream_id = header & 0xff;
  pes_packet_length = title->fs->read_uint16();
  pes_packet_start = absolute_position();
  pes_packet_end = pes_packet_start + pes_packet_length;
//zmsgs(" pes_packet_start=0x%jx pes_packet_length=%x zdata.size=%x\n", 
//      pes_packet_start, pes_packet_length, zdata.size);

  if( stream_id != PRIVATE_STREAM_2 && stream_id != PADDING_STREAM ) {
    if( (title->fs->next_char() >> 6) == 0x02 ) {
      /* Get MPEG-2 packet */
      int pes_header_bytes = 0;
      int pts_dts_flags;
      int pes_header_data_length;
      last_packet_decryption = absolute_position();
      scrambling = title->fs->read_char() & 0x30;
   /* scrambling = 1; */
   /* Reset scrambling bit for the mpeg3cat utility */
   /*      if( scrambling ) raw_data[raw_offset - 1] &= 0xcf; */
   /* Force packet length if scrambling */
      if( scrambling )
        pes_packet_length = DVD_PACKET_SIZE - pes_packet_start + last_packet_start;
      pts_dts_flags = (title->fs->read_char() >> 6) & 0x3;
      pes_header_data_length = title->fs->read_char();

      /* Get Presentation and Decoding Time Stamps */
      if( pts_dts_flags == 2 ) {
        pts = get_timestamp();
        if( dump ) zmsgs("pts=0x%jx\n", pts);
        pes_header_bytes += 5;
      }
      else if( pts_dts_flags == 3 ) {
        pts = get_timestamp();
        dts = get_timestamp();
        if( dump ) zmsgs("pts=%jd dts=%jd\n", pts, dts);
/*      pts = (title->fs->read_char() >> 1) & 7;
 *      pts <<= 15;
 *      pts |= (title->fs->read_uint16() >> 1);
 *      pts <<= 15;
 *      pts |= (title->fs->read_uint16() >> 1);
 *      dts = (title->fs->read_char() >> 1) & 7;
 *      dts <<= 15;
 *      dts |= (title->fs->read_uint16() >> 1);
 *      dts <<= 15;
 *      dts |= (title->fs->read_uint16() >> 1);
 */
        pes_header_bytes += 10;
      }
//zmsgs("get_program_pes_packet do_audio=%p do_video=%p pts=%x dts=%x\n", 
//  do_audio, do_video, pts, dts);
      /* Skip unknown */
      title->fs->seek_relative(pes_header_data_length-pes_header_bytes);
    }
    else {
      int pts_dts_flags;
      /* Get MPEG-1 packet */
      while( !title->fs->eof() && title->fs->next_char() == 0xff ) {
        title->fs->read_char();
      }
      /* Skip STD buffer scale */
      if( (title->fs->next_char() & 0x40) ) {
        title->fs->seek_relative(2);
      }
      /* Decide which timestamps are available */
      pts_dts_flags = title->fs->next_char();
      if( pts_dts_flags >= 0x30 ) {
        /* Get the presentation and decoding time stamp */
        pts = get_timestamp();
        dts = get_timestamp();
      }
      else if( pts_dts_flags >= 0x20 ) {
        /* Get just the presentation time stamp */
        pts = get_timestamp();
      }
      else if( pts_dts_flags == 0x0f ) {
        /* End of timestamps */
        title->fs->read_char();
      }
      else {
        return 1;     /* Error */
      }
    }
    /* Now extract the payload. */
    if( (stream_id >> 4) == 0xc || (stream_id >> 4) == 0xd ) {
      /* Audio data */
      /* Take first stream ID if -1 */
      pes_packet_length -= absolute_position() - pes_packet_start;
      custom_id = got_audio = stream_id & 0x0f;
      if( read_all )
        astream_table[custom_id] = afmt_MPEG;
      else if( astream == -1 )
        astream = custom_id;
      if( (custom_id == astream && do_audio) || read_all ) {
        set_audio_pts(pts, 90000.); //60000
        decryption_offset = absolute_position() - last_packet_start;
        if( dump ) {
          zmsgs(" MP2 audio data offset=0x%jx custom_id=%x size=%x\n", 
            program_byte, custom_id, pes_packet_length);
        }
        get_program_payload(pes_packet_length, 1, 0);
      }
      else {
        if( dump ) zmsgs(" skipping audio size=%x\n", pes_packet_length);
      }
    }
    else if( (stream_id >> 4) == 0xe ) {
      /* Video data */
      /* Take first stream ID if -1 */
      pes_packet_length -= absolute_position() - pes_packet_start;
      custom_id = got_video = stream_id & 0x0f;
      if( read_all ) {
        vstream_table[custom_id] = 1;
      } else if( vstream == -1 ) 
        vstream = custom_id;
      if( (custom_id == vstream && do_video) || read_all ) {
        set_video_pts(pts, 90000.); //60000
        decryption_offset = absolute_position() - last_packet_start;
        if( dump ) {
          zmsgs(" video offset=0x%jx custom_id=%x size=%x\n", 
            program_byte, custom_id, pes_packet_length);
        }
        get_program_payload(pes_packet_length, 0, 1);
      }
      else {
        if( dump ) zmsgs(" skipping video size=%x\n", pes_packet_length);
      }
    }
    else if( (stream_id == 0xbd || stream_id == 0xbf) && 
             ((title->fs->next_char() & 0xf0) == 0x20) ) {
      /* DVD subtitle data */
      stream_id = title->fs->read_char();
      custom_id = stream_id & 0x0f;
      if( read_all ) {
        if( !sstream_table[custom_id] ) {
          sstream_table[custom_id] = 1;
          src->strack[src->total_stracks] = new strack_t(custom_id);
          if( src->strack[src->total_stracks] ) ++src->total_stracks;
	}
      }
      /* Get data length */
      pes_packet_length -= absolute_position() - pes_packet_start;
      if( do_video ) {
        decryption_offset = absolute_position() - last_packet_start;
        get_program_payload(pes_packet_length, 0, 0);
        do_subtitle = 1;
      }
      else {
        if( dump ) zmsgs(" skipping subtitle size=%x\n", pes_packet_length);
      }
//zmsgs("id=0x%02x size=%d\n", stream_id, pes_packet_length);
    }
    else if( (stream_id == 0xbd || stream_id == 0xbf) && 
             title->fs->next_char() != 0xff &&
             ((title->fs->next_char() & 0xf0) == 0x80 ||
             (title->fs->next_char() & 0xf0) == 0xa0) ) {
      /* DVD audio data */
      /* Get the audio format */
      int format = (title->fs->next_char() & 0xf0) == 0xa0 ?
        afmt_PCM : afmt_AC3;
      /* Picks up bogus data if (& 0xf) or (& 0x7f) */
      stream_id = title->fs->next_char();
      /* only 8 streams, counting from 0x80 */
      custom_id = stream_id & 0x87;
      if( astream_table[custom_id] >= 0 ) {
        got_audio = custom_id;
        /* Take first stream ID if not building TOC. */
        if( read_all )
          astream_table[custom_id] = format;
        else if( astream == -1 )
          astream = custom_id;
        if( (custom_id == astream && do_audio) || read_all ) {
          set_audio_pts(pts, 90000.); //60000
          aformat = format;
          title->fs->read_uint32();
          pes_packet_length -= absolute_position() - pes_packet_start;
          decryption_offset = absolute_position() - last_packet_start;
          if( format == afmt_PCM )
            do_pcm = 1;
//zmsgs("get_program_pes_packet 5 %x\n", decryption_offset);
          if( dump ) zmsgs(" AC3 audio offset=0x%jx, custom_id=%03x, size=%x\n",
               program_byte, custom_id, pes_packet_length);
          get_program_payload(pes_packet_length, 1, 0);
        }
      }
    }
  }
  else if( stream_id == PRIVATE_STREAM_2 || stream_id == PADDING_STREAM ) {
    pes_packet_length -= absolute_position() - pes_packet_start;
    if( stream_id == NAV_PACKET_STREAM /* == PRIVATE_STREAM_2 */ ) {
      if( !nav ) nav = new nav_t();
      int sub_stream = title->fs->read_char();
      --pes_packet_length;
      switch( sub_stream ) {
      case NAV_PCI_SSID:
        if( pes_packet_length >= NAV_PCI_BYTES ) {
          title->fs->read_data(&nav->pci[0],NAV_PCI_BYTES);
//zmsgs("nav_pci: %lu + %lu\n",absolute_position()/2048,absolute_position()%2048);
//dmp(&nav->pci[0],NAV_PCI_BYTES);
          pes_packet_length -= NAV_PCI_BYTES;
        }
        break;
      case NAV_DSI_SSID:
        if( pes_packet_length >= NAV_DSI_BYTES ) {
          title->fs->read_data(&nav->dsi[0],NAV_DSI_BYTES);
//zmsgs("nav_dsi: %lu + %lu\n",absolute_position()/2048,absolute_position()%2048);
//dmp(&nav->dsi[0],NAV_DSI_BYTES);
          pes_packet_length -= NAV_DSI_BYTES;
          int64_t blk_pos = ((int64_t)nav->dsi_gi_pck_lbn() & 0x7fffffffU) * DVD_PACKET_SIZE;
          if( blk_pos != 0 && last_packet_start != blk_pos )
            zmsgs("blk_pos 0x%jx != 0x%jx last_packet_start\n", blk_pos, last_packet_start);
          int64_t next_pos, next_vobu, end_byte, end_pos;
          int64_t blk_size = (int64_t)nav->dsi_gi_vobu_ea() * DVD_PACKET_SIZE;
          nav_cell_end_byte = program_byte + blk_size + DVD_PACKET_SIZE;
          end_pos = program_to_absolute(nav_cell_end_byte, &end_byte);
          if( end_byte != nav_cell_end_byte ) 
            zmsgs("end_byte 0x%jx != 0x%jx nav_cell_end_byte\n", end_byte, nav_cell_end_byte);
          uint32_t next_vobu_offset = nav->dsi_si_next_vobu();
          if( next_vobu_offset == NAV_SRI_END_OF_CELL ) {
            next_vobu = playinfo_next_cell();
            next_pos = next_vobu >= 0 ? program_to_absolute(next_vobu) : -1;
          }
          else {
            next_pos = blk_pos + ((int64_t)next_vobu_offset & 0x7fffffffU) * DVD_PACKET_SIZE;
            next_vobu = absolute_to_program(next_pos);
          }
          if( next_vobu > 0 && end_pos != next_pos ) {
            nav_cell_next_vobu = next_vobu;
            zmsgs("blk end_pos 0x%jx != 0x%jx next_pos, jump to 0x%jx\n",
              end_pos, next_pos, nav_cell_next_vobu);
          }
          else
            nav_cell_end_byte = nav_cell_next_vobu = -1;
if( nav_cell_end_byte >= 0 )
  zmsgs("nav pkt at 0x%jx ends 0x%jx next_vobu 0x%jx/0x%jx\n",
    program_byte, nav_cell_end_byte, nav_cell_next_vobu, next_pos);
        }
        break;
      }
    }
  }

  int64_t len = pes_packet_end - absolute_position();
  if( len > 0 )
    title->fs->seek_relative(len);

  if( scrambling )
    handle_scrambling(decryption_offset);
  if( do_pcm )
    handle_pcm(pes_packet_length);
  else if( do_subtitle )
    handle_subtitle(src, custom_id, pes_packet_length);
  return 0;
}

int zdemuxer_t::
read_program()
{
  int64_t pos;
  int result = 0;
  title_t *title = titles[current_title];
  uint32_t header = 0;
  int pack_count = 0;
  const int debug = 0;

  got_audio = -1;
  got_video = -1;
  stream_id = -1;
  custom_id = -1;
  got_subtitle = 0;

  if( title->fs->eof() ) return 1;
  last_packet_start = absolute_position();
  program_byte = absolute_to_program(last_packet_start);

/* Search for next header */
/* Parse packet until the next packet start code */
  while( !result ) {
    title = titles[current_title];
    if( debug )
      zmsgs("%d %d 0x%jx 0x%jx\n", 
        __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
    if( title->fs->eof() ) break;
    pos = absolute_position();
    header = title->fs->read_uint32();
    if( header == PACK_START_CODE ) {
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n", 
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
/* Second start code in this call.  Don't read it. */
      if( pack_count ) {
        title->fs->seek_relative(-4);
        break;
      }
      last_packet_start = pos;
      result = get_pack_header();
      pack_count++;
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n",
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
    }
    else if( header == SYSTEM_START_CODE && pack_count ) {
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n",
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
      result = get_system_header();
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n",
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
    }
    else if( header == PROGRAM_END_CODE ) {
      title->fs->total_bytes = title->fs->current_byte;
      stream_end = program_byte;
    }
    else if( (header >> 8) == PACKET_START_CODE_PREFIX && pack_count ) {
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n",
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
      result = get_program_pes_packet(header);
      if( debug )
        zmsgs("%d %d 0x%jx 0x%jx\n",
          __LINE__, result, title->fs->current_byte, title->fs->total_bytes);
    }
    else {
/* if whamming on bad blocks, if possible skip to the next cell and continue */
      int errs = title->fs->errors();
      if( errs ) {
        if( total_titles > 1 || title->cell_table_size > 1 ) {
          zmsgs("%d device errs, truncating cell %d\n", errs, current_cell_no());
          program_byte = absolute_to_program(last_packet_start);
          src->demuxer->titles[current_title]->cell_table[title_cell].program_end =
            title->cell_table[title_cell].program_end = program_byte;
          nav_cell_end_byte = 0;
        }
      }
      else
/* Try again starting with next byte */
        program_byte = absolute_to_program(pos) + 1;
      result = seek_phys();
      last_packet_start = absolute_position();
    }
  }
/* Ignore errors in the parsing.  Just quit if eof. */
  result = 0;
  if( debug )
    zmsgs("%d\n", __LINE__);
  last_packet_end = absolute_position();
  pos = absolute_to_program(last_packet_start);
  program_byte = pos + (last_packet_end - last_packet_start);
  if( debug )
    zmsgs("%d\n", __LINE__);
  return result;
}

int zdemuxer_t::
get_cell(int no, zcell_t *&v)
{
  for( int i=0; i<total_titles; ++i ) {
    title_t *title = titles[i];
    int n = title->cell_table_size;
    for( int k=0; k<n; ++k ) {
      zcell_t *cell = &title->cell_table[k];
      if( cell->cell_no >= no ) {
        v = cell;
        return cell->cell_no == no ? 0 : 1;
      }
    }
  }
  return -1;
}

int zdemuxer_t::
seek_phys()
{
  int last_cell_no = current_cell_no();

  int64_t next_byte = program_byte;
  if( !reverse && nav_cell_end_byte >= 0 && next_byte >= nav_cell_end_byte ) {
    if( nav_cell_next_vobu >= 0 ) {
      if( next_byte != nav_cell_next_vobu ) {
        zmsgs("next_vobu 0x%jx!=0x%jx next_byte\n", nav_cell_next_vobu, next_byte);
        next_byte = nav_cell_next_vobu;
      }
      nav_cell_next_vobu = -1;
    }
    nav_cell_end_byte = -1;
  }

  int next_title, next_cell;
  int64_t pos = program_to_absolute(next_byte, &next_byte, &next_title, &next_cell);
  if( next_byte < 0 ) return 1;


  int do_cell_change = 0;
  if( program_byte != next_byte ) {
    zmsgs("program_byte hopped from 0x%jx to 0x%jx\n", program_byte, next_byte);
    program_byte = next_byte;
    do_cell_change = 1;
  }
  if( next_title != current_title ) {
    open_title(next_title);
    do_cell_change = 1;
  }
  if( title_cell != next_cell ) {
    title_cell = next_cell;
    do_cell_change = 1;
  }

  if( do_cell_change ) {
/* Need to change cells if we get here. */
    int cell_no = current_cell_no();
    if( last_cell_no != cell_no ) {
      nav_cell_end_byte = -1;
      if( !read_all ) {
        zcell_t *cell = 0;
        if( !get_cell(cell_no, cell) && cell->discontinuity ) {
          if( do_audio ) {
            atrack_t *atrk = do_audio;
            atrk->reset_pts();
            atrk->pts_offset = atrk->audio_time = cell->cell_time;
            atrk->pts_position = atrk->audio_time * atrk->sample_rate;
          }
          if( do_video ) {
            vtrack_t *vtrk = do_video;
            vtrk->reset_pts();
            vtrk->pts_offset = vtrk->video_time = cell->cell_time;
            vtrk->pts_position = vtrk->video_time * vtrk->frame_rate;
          }
        }
      }
    }
  }

  if( !zdata.length() ) {
    if( program_byte >= movie_size() ) return 1;
    if( stream_end >= 0 && program_byte >= stream_end ) return 1;
  }
  error_flag = 0;
  title_t *title = titles[current_title];
  title->fs->seek(pos - title->start_byte);
  return 0;
}

int zdemuxer_t::
next_code(uint32_t zcode)
{
  uint32_t result = 0;
  int error = 0;

  while( result!=zcode && !error ) {
    result <<= 8;
    title_t *title = titles[current_title];
    result |= title->fs->read_char() & 0xff;
    ++program_byte;
    error = seek_phys();
  }

  return error;
}

/* Read packet in the forward direction */
int zdemuxer_t::
read_next_packet()
{
  if( current_title < 0 ) return 1;
  int result = 0;
  title_t *title = titles[current_title];
/* Reset output buffer */
  zdata.size = 0;
  zdata.position = 0;
  zaudio.size = 0;
  zvideo.size = 0;
//zmsgs("%d program_byte=0x%jx reverse=%d\n", 
//   __LINE__, program_byte, reverse);

/* Switch to forward direction. */
  if( reverse ) {
/* Don't reread anything if we're at the beginning of the file. */
    if( program_byte < 0 ) {
      program_byte = 0;
      result = seek_phys();
/* First character was the -1 byte which brings us to 0 after this function. */
      result = 1;
    }
    else if( src->packet_size > 0 ) { /* Transport or elementary stream */
      program_byte += src->packet_size;
      result = seek_phys();
    }
    else {
/* Packet just read */
      if( !result )
        result = next_code(PACK_START_CODE);
/* Next packet */
      if( !result )
        result = next_code(PACK_START_CODE);
    }
    reverse = 0;
  }

/* Read packets until the output buffer is full. */
/* Read a single packet if not fetching audio or video. */
  if( !result ) do {
    title = titles[current_title];
//zmsgs("10 0x%jx\n", absolute_position());
    if( src->is_transport_stream() ) {
      result = seek_phys();
      if( !result )
        result = read_transport();
    }
    else if( src->is_program_stream() ) {
      result = seek_phys();
//zmsgs("%d 0x%jx\n", __LINE__, tell_byte());
      if( !result )
        result = read_program();
//zmsgs("%d 0x%jx\n", __LINE__, tell_byte());
    }
    else {
      if( read_all && src->is_audio_stream() ) {
/* Read elementary stream. */
        result = title->fs->read_data(zaudio.buffer, src->packet_size);
        zaudio.size = src->packet_size;
      }
      else if( read_all && src->is_video_stream() ) {
/* Read elementary stream. */
        result = title->fs->read_data(zvideo.buffer, src->packet_size);
        zvideo.size = src->packet_size;
      }
      else {
        result = title->fs->read_data(zdata.buffer, src->packet_size);
        zdata.size = src->packet_size;
      }
      program_byte += src->packet_size;
      result |= seek_phys();
    }
//zmsgs("100 result=%d zdata.size=0x%x\n", result, zdata.size);
  } while( !result && zdata.size == 0 && (do_audio || do_video) );

//zmsgs("%d result=%d zdata.size=0x%x\n", __LINE__, result, zdata.size);
  return result;
}

int zdemuxer_t::
previous_code(uint32_t zcode)
{
  uint32_t result = 0;
  int error = 0;

  while( result!=zcode && program_byte>0 && !error ) {
    result >>= 8;
    title_t *title = titles[current_title];
    title->fs->seek(program_byte-title->start_byte-1);
    result |= (title->fs->read_char() & 0xff) << 24;
    --program_byte;
    error = seek_phys();
  }
  return error;
}

/* Read the packet right before the packet we're currently on. */
int zdemuxer_t::
read_prev_packet()
{
  int result = 0;
  title_t *title = titles[current_title];
  nav_cell_end_byte = -1;
  zdata.size = 0;
  zdata.position = 0;
/* Switch to reverse direction */
  if( !reverse ) {
    reverse = 1;
/* Transport stream or elementary stream case */
    if( src->packet_size > 0 ) {
      program_byte -= src->packet_size;
      result = seek_phys();
    }
    else { /* Program stream */
      result = previous_code(PACK_START_CODE);
    }
  }

/* Go to beginning of previous packet */
  do {
    title = titles[current_title];

/* Transport stream or elementary stream case */
    if( src->packet_size > 0 ) {
//zmsgs("1 result=%d title=%d tell=0x%jx program_byte=0x%jx\n",
//        result, current_title, absolute_position(), program_byte);
      program_byte -= src->packet_size;
      result = seek_phys();
//zmsgs("100 result=%d title=%d tell=0x%jx program_byte=0x%jx\n",
//        result, current_title, absolute_position(), program_byte);
    }
    else {
      if( !result )
        result = previous_code(PACK_START_CODE);
    }

/* Read packet and then rewind it */
    title = titles[current_title];
    if( src->is_transport_stream() && !result ) {
      result = read_transport();

      if( program_byte > 0 ) {
        program_byte -= src->packet_size;
        result = seek_phys();
      }
    }
    else if( src->is_program_stream() && !result ) {
      int64_t cur_position = program_byte;
/* Read packet */
      result = read_program();
/* Rewind packet */
      while( program_byte > cur_position && !result ) {
        result = previous_code(PACK_START_CODE);
      }
    }
    else if( !result ) {
/* Elementary stream */
/* Read the packet forwards and seek back to the start */
      result = title->fs->read_data(zdata.buffer, src->packet_size);
      if( !result ) {
        zdata.size = src->packet_size;
        result = title->fs->seek(program_byte);
      }
    }
  } while (!result && zdata.size == 0 && (do_audio || do_video) );

  return result;
}

/* For audio */
int zdemuxer_t::
read_data(uint8_t *output, int size)
{
  int result = 0;
  error_flag = 0;
//zmsg("1\n");

  if( zdata.position >= 0 ) {
    int count = 0;
/* Read forwards */
    while( count<size && !result ) {
      int fragment_size = size - count;
      int len = zdata.length();
      if( fragment_size > len ) fragment_size = len;
      memcpy(output+count, zdata.buffer+zdata.position, fragment_size);
      zdata.position += fragment_size;
      count += fragment_size;
      if( count >= size ) break;
      result = read_next_packet();
//zmsgs("10 offset=0x%jx pid=0x%x bytes=0x%x i=0x%x\n", 
//   tell_byte(), pid, zdata.size, count);
    }
//zmsg("10\n");
  }
  else {
    int cur_position = zdata.position;
/* Read backwards a full packet. */
/* Only good for reading less than the size of a full packet, but */
/* this routine should only be used for searching for previous markers. */
    result = read_prev_packet();
    if( !result ) zdata.position = zdata.size + cur_position;
    memcpy(output, zdata.buffer+zdata.position, size);
    zdata.position += size;
  }

//zmsg("2\n");
  error_flag = result;
  return result;
}

uint8_t zdemuxer_t::
read_char_packet()
{
  error_flag = 0;
  next_char = -1;
  if( zdata.position >= zdata.size )
    error_flag = read_next_packet();
  if( !error_flag ) 
    next_char = zdata.buffer[zdata.position++];
  return next_char;
}

uint8_t zdemuxer_t::
read_prev_char_packet()
{
  error_flag = 0;
  if( --zdata.position < 0 ) {
    error_flag = read_prev_packet();
    if( !error_flag ) zdata.position = zdata.size - 1;
  }

  if( zdata.position >= 0 )
    next_char = zdata.buffer[zdata.position];
  return next_char;
}

int zdemuxer_t::
open_title(int title_number)
{
  title_t *title;

  if( title_number<total_titles && title_number >= 0 ) {
    if( current_title >= 0 ) {
      titles[current_title]->fs->close_file();
      current_title = -1;
    }
    title = titles[title_number];
    if( title->fs->open_file() ) {
      error_flag = 1;
      perrs("%s",title->fs->path);
    }
    else
      current_title = title_number;
  }
  else
    zerrs("title_number = %d\n", title_number);
  return error_flag;
}

int zdemuxer_t::
copy_titles(zdemuxer_t *dst)
{
  int i;
  memcpy(&dst->vstream_table[0],&vstream_table[0],sizeof(dst->vstream_table));
  memcpy(&dst->astream_table[0],&astream_table[0],sizeof(dst->astream_table));
  memcpy(&dst->sstream_table[0],&sstream_table[0],sizeof(dst->sstream_table));

  for( i=0; i<dst->total_titles; ++i )
    delete dst->titles[i];

  dst->total_titles = total_titles;
  for( i=0; i<total_titles; ++i ) {
    dst->titles[i] = new title_t(*titles[i]);
  }

  if( current_title >= 0 )
    dst->open_title(current_title);

  dst->title_cell = -1;
  return 0;
}

void zdemuxer_t::
end_title(int64_t end_byte)
{
  if( !total_titles ) return;
  title_t *title = titles[total_titles-1];
  title->total_bytes = end_byte - title->start_byte;
  title->end_byte = end_byte;
  if( !title->cell_table_size ) return;
  title_t::cell_t *cell = &title->cell_table[title->cell_table_size-1];
  cell->title_end = title->total_bytes;
  cell->program_end = cell->title_end - cell->title_start + cell->program_start;
}

/* ==================================================================== */
/*                            Entry points */
/* ==================================================================== */

zdemuxer_t::
demuxer_t(zmpeg3_t *zsrc, zatrack_t *do_aud, zvtrack_t *do_vid, int cust_id)
{
/* The demuxer will change the default packet size for its own use. */
  src = zsrc;
  do_audio = do_aud;
  do_video = do_vid;

/* Allocate buffer + padding */
  raw_data = new uint8_t[RAW_SIZE];
  zdata.buffer = new uint8_t[zdata.allocated=RAW_SIZE];
  zaudio.buffer = new uint8_t[zaudio.allocated=RAW_SIZE];
  zvideo.buffer = new uint8_t[zvideo.allocated=RAW_SIZE];

/* System specific variables */
  audio_pid = cust_id;
  video_pid = cust_id;
  subtitle_pid = cust_id;
  astream = cust_id;
  vstream = cust_id;
  current_title = -1;
  title_cell = -1;
  pes_audio_time = -1.;
  pes_video_time = -1.;
//zmsgs("%f\n", time);
  stream_end = -1;
  nav_cell_end_byte = -1;
}

zdemuxer_t::
~demuxer_t()
{
  int i;

  if( current_title >= 0 )
    titles[current_title]->fs->close_file();

  for( i=0; i<total_titles; ++i )
    delete titles[i];

  if( zdata.buffer ) delete [] zdata.buffer;
  if( raw_data ) delete [] raw_data;
  if( zaudio.buffer ) delete [] zaudio.buffer;
  if( zvideo.buffer ) delete [] zvideo.buffer;

  for( i=0; i<total_subtitles; ++i )
    delete subtitles[i];

  if( nav ) delete nav;
}

uint8_t zdemuxer_t::
read_prev_char()
{
  if( zdata.position )
    return zdata.buffer[--zdata.position];
  return read_prev_char_packet();
}

bool zdemuxer_t::
bof()
{
  return current_title == 0 && titles[0]->fs->bof();
}

bool zdemuxer_t::
eof()
{
  if( !src->seekable ) return zdata.eof();
  if( error() ) return 1;
  if( current_title >= total_titles ) return 1;
  if( current_title >= 0 && titles[current_title]->fs->eof() ) return 1;
  /* Same operation performed in seek_phys */
  return stream_end >= 0 && program_byte >= stream_end && !zdata.length();
}

void zdemuxer_t::
start_reverse()
{
  reverse = 1;
}

void zdemuxer_t::
start_forward()
{
  reverse = 0;
}

/* Seek to absolute byte */
int zdemuxer_t::
seek_byte(int64_t byte)
{
/* for( int i=0; i<total_titles; ++i ) titles[i]->dump_title(); */
  error_flag = 0;
  title_cell = -1;
  nav_cell_end_byte = -1;
  program_byte = byte;
  zdata.position = 0;
  zdata.size = 0;

  int result = seek_phys();
//zmsgs("1 %p %d 0x%jx 0x%jx\n", do_video, result, byte, program_byte);
  return result;
}

void zdemuxer_t::
set_audio_pts(uint64_t pts, const double denom)
{
  if( pts && pes_audio_time < 0 ) {
    pes_audio_pid = custom_id;
    pes_audio_time = pts / denom;
//zmsgs("pid 0x%03x, pts %f @0x%jx\n",pes_audio_pid, pes_audio_time,
//  absolute_position());
  }
}

void zdemuxer_t::
set_video_pts(uint64_t pts, const double denom)
{
  if( pts && pes_video_time < 0 ) {
    pes_video_pid = custom_id;
    pes_video_time = pts / denom;
//zmsgs("pid 0x%03x, pts %f @0x%jx\n",pes_video_pid, pes_video_time,
//  absolute_position());
  }
}

void zdemuxer_t::
reset_pts()
{
  atrack_t *atrk = do_audio;
  vtrack_t *vtrk = do_video;
  if( !atrk && !vtrk ) {
    for( int i=0; !atrk && i<src->total_atracks; ++i )
      if( src->atrack[i]->pid == pid ) atrk = src->atrack[i];
    for( int i=0; !vtrk && i<src->total_vtracks; ++i )
      if( src->vtrack[i]->pid == pid ) vtrk = src->vtrack[i];
  }
  if( atrk ) atrk->reset_pts();
  if( vtrk ) vtrk->reset_pts();
}

double zdemuxer_t::
scan_pts()
{
  int64_t start_position = tell_byte();
  int64_t end_position = start_position + PTS_RANGE;
  int64_t current_position = start_position;
  int result = 0;

  reset_pts();
  while( !result && current_position < end_position &&
         ( (do_audio && pes_audio_time < 0) ||
           (do_video && pes_video_time < 0) ) ) {
    result = read_next_packet();
    current_position = tell_byte();
  }

/* Seek back to starting point */
  seek_byte(start_position);

  if( do_audio ) return pes_audio_time;
  if( do_video ) return pes_video_time;
  zerr("no active data to scan\n");
  return 0;
}

int zdemuxer_t::
goto_pts(double pts)
{
  int64_t start_position = tell_byte();
  int64_t end_position = start_position + PTS_RANGE;
  int64_t current_position = start_position;
  int result = 0;

/* Search forward for nearest pts */
  reset_pts();
  while( !result && current_position < end_position ) {
    result = read_next_packet();
    if( pes_audio_time > pts ) break;
    current_position = tell_byte();
  }

/* Search backward for nearest pts */
  end_position = current_position - PTS_RANGE;
  read_prev_packet();
  while( !result && current_position > end_position ) {
    result = read_prev_packet();
    if( pes_audio_time < pts ) break;
    current_position = tell_byte();
  }
  return 0;
}

int zdemuxer_t::
current_cell_no()
{
  if( current_title < 0 || current_title >= total_titles ) return -1;
  title_t *title = titles[current_title];
  if( !title->cell_table || title_cell < 0 ) return -1;
  return title->cell_table[title_cell].cell_no;
}

int64_t zdemuxer_t::
absolute_to_program(int64_t byte)
{
//zmsgs("%d\n", zdata.size);
/* Can only offset to current cell since we can't know what cell the */
/* byte corresponds to. */
  title_t *title = titles[current_title];
  title_t::cell_t *cell = &title->cell_table[title_cell];
  return byte - cell->title_start - title->start_byte + cell->program_start;
}

int64_t zdemuxer_t::
prog2abs_fwd(int64_t byte, int64_t *nbyte, int *ntitle, int *ncell)
{
  int ltitle = -1, lcell = -1;
  int64_t lbyte = -1;
  int ititle = current_title;
  int icell = title_cell;
  title_t *title;
  title_t::cell_t *cell;
// check current cell first
  if( ititle >= 0 && ititle < total_titles ) {
    title = titles[ititle];
    if( icell >= 0 && icell < title->cell_table_size ) {
      cell = &title->cell_table[icell];
      if( byte >= cell->program_start && byte < cell->program_end ) goto xit;
    }
  }

  for( ititle=0; ititle<total_titles; ++ititle ) {
// check all title cells
    title = titles[ititle];
    int ncells = title->cell_table_size;
    if( ncells <= 0 ) continue;
    if( byte >= title->cell_table[ncells-1].program_end ) continue;
    for( icell=0; icell<ncells; ++icell ) {
      cell = &title->cell_table[icell];
      if( byte < cell->program_end ) goto xit;
      if( lbyte > cell->program_end ) continue;
      lbyte = cell->program_end;
      ltitle = ititle;  lcell = icell;
    }
  }
// not found, return search region boundry
  if( nbyte ) *nbyte = lbyte;
  if( ntitle ) *ntitle = ltitle;
  if( ncell ) *ncell = lcell;
  return total_titles>0 ? titles[total_titles-1]->end_byte : -1;

xit:
  if( byte < cell->program_start ) byte = cell->program_start;
  if( nbyte ) *nbyte = byte;
  if( ntitle ) *ntitle = ititle;
  if( ncell ) *ncell = icell;
  return title->start_byte + cell->title_start + byte - cell->program_start;
}

int64_t zdemuxer_t::
prog2abs_rev(int64_t byte, int64_t *nbyte, int *ntitle, int *ncell)
{
  int ititle = current_title;
  int icell = title_cell;
  title_t *title;
  title_t::cell_t *cell;
// check current cell first
  if( ititle >= 0 && ititle < total_titles ) {
    title = titles[ititle];
    if( icell >= 0 && icell < title->cell_table_size ) {
      cell = &title->cell_table[icell];
      if( byte > cell->program_start && byte <= cell->program_end ) goto xit;
    }
  }

  for( ititle=total_titles; --ititle>=0; ) {
// check all title cells
    title = titles[ititle];
    int ncells = title->cell_table_size;
    if( ncells <= 0 ) continue;
    if( byte < title->cell_table[0].program_start ) continue;
    for( icell=ncells; --icell>=0; ) {
      cell = &title->cell_table[icell];
      if( byte > cell->program_start ) goto xit;
    }
  }
// not found, return search region boundry
  if( nbyte ) *nbyte = 0;
  if( ntitle ) *ntitle = 0;
  if( ncell ) *ncell = 0;
  return 0;

xit:
  if( byte > cell->program_end ) byte = cell->program_end;
  if( nbyte ) *nbyte = byte;
  if( ntitle ) *ntitle = ititle;
  if( ncell ) *ncell = icell;
  return title->start_byte + cell->title_start + byte - cell->program_start;
}

int64_t zdemuxer_t::
program_to_absolute(int64_t byte, int64_t *nbyte, int *ntitle, int *ncell)
{
  return !reverse ?
    prog2abs_fwd(byte, nbyte, ntitle, ncell) :
    prog2abs_rev(byte, nbyte, ntitle, ncell) ;
}

int64_t zdemuxer_t::
movie_size()
{
  if( !total_bytes ) {
    int64_t result = 0;
    int i, j;
    for( i=0; i<total_titles; ++i ) {
      title_t *title = titles[i];
      if( title->cell_table ) {
        for( j=0; j<title->cell_table_size; ++j ) {
          title_t::cell_t *cell = &title->cell_table[j];
/*        result = cell->program_end - cell->program_start; */
          if( result < cell->program_end ) result = cell->program_end;
        }
      }
/*    result += titles[i]->total_bytes; */
    }
    total_bytes = result;
  }
  return total_bytes;
}

int64_t zdemuxer_t::
next_cell()
{
  if( current_title < 0 || current_title >= total_titles ) return -1;
  int i = current_title;
  if( !titles[i]->cell_table || title_cell < 0 ) return -1;
  int k = title_cell;
  int n = titles[i]->cell_table[k].cell_no;

  /* always forward search, stop when cell_no changes */
  while ( i < total_titles && titles[i]->cell_table[k].cell_no == n ) {
    ++k;
    /* some titles have no cells (may be another program) */
    while( !titles[i]->cell_table || k >= titles[i]->cell_table_size ) {
      if( ++i >= total_titles ) break;
      k = 0;
    }
  }

  return  i < total_titles ? titles[i]->cell_table[k].program_start : movie_size();
}

int64_t zdemuxer_t::
playinfo_next_cell()
{
  int64_t result = -1;
  int i = title_cell;
  int pcell_no = titles[current_title]->cell_table[i++].cell_no;

  for( int n=current_title ; result<0 && n<total_titles; ++n ) {
    title_t *title = titles[n];
    for( ; result<0 && i<title->cell_table_size; ++i ) {
      if( title->cell_table[i].cell_no != pcell_no )
         result = title->cell_table[i].program_start;
    }
    i = 0;
  }

  return result;
}

int64_t zdemuxer_t::
title_bytes()
{
  title_t *title = titles[current_title];
  return title->total_bytes;
}

void zdemuxer_t::
append_data(uint8_t *data, int bytes)
{
  if( bytes <= 0 ) return;
  int new_data_size = zdata.size + bytes;
  if( zdata.size + new_data_size >= zdata.allocated ) {
    zdata.allocated = (zdata.size + new_data_size) * 2;
    uint8_t *new_data_buffer = new uint8_t[zdata.allocated];
    memcpy(new_data_buffer,zdata.buffer,zdata.size);
    delete [] zdata.buffer;
    zdata.buffer = new_data_buffer;
  }

  memcpy(zdata.buffer+zdata.size, data, bytes);
  zdata.size += bytes;
}

void zdemuxer_t::
shift_data()
{
  if( zdata.position <= 0 ) return;
  memmove(zdata.buffer,
          zdata.buffer+zdata.position,
          zdata.size-zdata.position);
  zdata.size -= zdata.position;
  zdata.position = 0;
}

/* Create a title and get PID's by scanning first few bytes. */
int zdemuxer_t::
create_title(int full_scan)
{
  int debug = 0;

  error_flag = 0;
  read_all = 1;

/* Create a single title */
  if( !total_titles ) {
    titles[0] = new title_t(src);
    total_titles = 1;
    open_title(0);
  }

  title_t *title = titles[0];
  title->total_bytes = title->fs->ztotal_bytes();
  title->start_byte = 0;
  title->end_byte = title->total_bytes;
  if( debug )
    zmsgs("%d path=%s total_bytes=%jd\n",
      __LINE__, src->fs->path, title->total_bytes);

/* Create default cell */
  if( !title->cell_table_size )
	  title->new_cell(title->end_byte);

/* Get PID's and tracks */
  if( src->is_transport_stream() || src->is_program_stream() ) {
    title->fs->seek(START_BYTE);
    int64_t next_byte = START_BYTE;
    int64_t last_time_pos = next_byte;
    // only spend a few seconds on this
    struct timeval tv;
    gettimeofday(&tv,NULL);
    double start_time = tv.tv_sec + tv.tv_usec/1000000.0;
    while( !title->fs->eof() ) {
      if( read_next_packet() ) break;
      next_byte = title->fs->tell();
#ifdef ZDVB
      if( src->is_transport_stream() ) {
        src->dvb.atsc_tables(this, custom_id);
      }
#endif
      if( full_scan ) continue;
      if( src->is_transport_stream() ) {
        if( next_byte > START_BYTE + MAX_TS_PROBE ) break;
#ifdef ZDVB
        if( src->dvb.signal_time() > 3 ) break;
#endif
      }
      else if( src->is_program_stream() )
        if( next_byte > START_BYTE + MAX_PGM_PROBE ) break;
      if( next_byte - last_time_pos > 0x40000 ) {
        gettimeofday(&tv,NULL);
        double next_time = tv.tv_sec + tv.tv_usec/1000000.0;
        if( next_time - start_time > 3 ) break;
        last_time_pos = next_byte;
      }
    }
  }
#ifdef ZDVB
  if( src->is_transport_stream() )
    src->dvb.read_dvb(this);
#endif
  title->fs->seek(START_BYTE);
  read_all = 0;
  return 0;
}

void zdemuxer_t::
skip_video_frame()
{
  uint32_t header = 0;
  title_t *title = titles[current_title];

  do {
    header <<= 8;
    header &= 0xffffffff;
    header |= title->fs->read_char();
  } while( header != PICTURE_START_CODE && !title->fs->eof() );
/*
 *  if(!mpeg3io_eof(title->fs))
 *    mpeg3io_seek_relative(title->fs, -4);
 */
  src->last_type_read = 2;
}

