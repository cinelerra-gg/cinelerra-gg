#include "../libzmpeg3.h"

/* zig-zag scan */
uint8_t zvideo_t::
zig_zag_scan_nommx[64] = {
   0 , 1,  8, 16,  9,  2,  3, 10, 17, 24, 32, 25, 18, 11,  4,  5, 
  12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13,  6,  7, 14, 21, 28, 
  35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51, 
  58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

/* alternate scan */
uint8_t zvideo_t::
alternate_scan_nommx[64] = {
   0,  8, 16, 24, 1,  9,  2, 10, 17, 25, 32, 40, 48, 56, 57, 49, 
  41, 33, 26, 18, 3, 11,  4, 12, 19, 27, 34, 42, 50, 58, 35, 43, 
  51, 59, 20, 28, 5, 13,  6, 14, 21, 29, 36, 44, 52, 60, 37, 45, 
  53, 61, 22, 30, 7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63,
};

/* default intra quantization matrix */
uint8_t zvideo_t::
default_intra_quantizer_matrix[64] = {
   8, 16, 19, 22, 26, 27, 29, 34,
  16, 16, 22, 24, 27, 29, 34, 37,
  19, 22, 26, 27, 29, 34, 34, 38,
  22, 22, 26, 27, 29, 34, 37, 40,
  22, 26, 27, 29, 32, 35, 40, 48,
  26, 27, 29, 32, 35, 40, 48, 58,
  26, 27, 29, 34, 38, 46, 56, 69,
  27, 29, 35, 38, 46, 56, 69, 83,
};

double zvideo_t::
frame_rate_table[16] = {
  0.0,   /* Pad */
  (double)24000.0/1001.0,       /* Official frame rates */
  (double)24.0,
  (double)25.0,
  (double)30000.0/1001.0,
  (double)30.0,
  (double)50.0,
  (double)60000.0/1001.0,
  (double)60.0,

  1.0, 5.0, 10.0, 12.0, 15.0,   /* Unofficial economy rates */
  0.0, 0.0,
};

static int blk_cnt_tab[3] = { 6, 8, 12 };

int zvideo_t::
init_decoder()
{
  int i, j, sz, cc;
  long size[4], padding[2];         /* Size of Y, U, and V buffers */

  if( !mpeg2 ) { /* force MPEG-1 parameters */
    prog_seq = 1;
    prog_frame = 1;
    pict_struct = pics_FRAME_PICTURE;
    frame_pred_dct = 1;
    chroma_format = cfmt_420;
    matrix_coefficients = 5;
  }

/* Get dimensions rounded to nearest multiple of coded macroblocks */
  mb_width = (horizontal_size + 15) / 16;
  mb_height = (mpeg2 && !prog_seq) ? 
    (2 * ((vertical_size + 31) / 32)) : ((vertical_size + 15) / 16);
  coded_picture_width = 16 * mb_width;
  coded_picture_height = 16 * mb_height;
  chrom_width = (chroma_format == cfmt_444) ? 
    coded_picture_width : (coded_picture_width >> 1);
  chrom_height = (chroma_format != cfmt_420) ? 
    coded_picture_height : (coded_picture_height >> 1);
  blk_cnt = blk_cnt_tab[chroma_format - 1];

/* Get sizes of YUV buffers */
  padding[0] = 16*coded_picture_width + 16;
  size[0] = coded_picture_width*coded_picture_height + padding[0];

  padding[1] = 16*chrom_width + 16;
  size[1] = chrom_width*chrom_height + padding[1];
  size[3] = (size[2] = (llw * llh)) / 4;

/* Allocate contiguous fragments for YUV buffers for hardware YUV decoding */
  sz = size[0] + 2*size[1];
  yuv_buffer[0] = new uint8_t[sz]; memset(yuv_buffer[0],0,sz);
  yuv_buffer[1] = new uint8_t[sz]; memset(yuv_buffer[1],0,sz);
  yuv_buffer[2] = new uint8_t[sz]; memset(yuv_buffer[2],0,sz);

  if( scalable_mode == slice_decoder_t::sc_SPAT ) {
    sz = size[2] + 2 * size[3];
    yuv_buffer[3] = new uint8_t[sz]; memset(yuv_buffer[3],0,sz);
    yuv_buffer[4] = new uint8_t[sz]; memset(yuv_buffer[4],0,sz);
  }

  tdat = new uint8_t[4 * mb_width * mb_height];
  memset(tdat, 0, 4 * mb_width * mb_height);

  slice_decoder_t::init_tables();

/* Direct pointers to areas of contiguous fragments in YVU order per Microsoft */  
  for( cc=0; cc<3; ++cc ) {
    llframe0[cc] = 0;
    llframe1[cc] = 0;
    newframe[cc] = 0;
  }

  refframe[0]    = yuv_buffer[0];
  oldrefframe[0] = yuv_buffer[1];
  auxframe[0]    = yuv_buffer[2];
  sz             = size[0];
  refframe[2]    = yuv_buffer[0] + sz;
  oldrefframe[2] = yuv_buffer[1] + sz;
  auxframe[2]    = yuv_buffer[2] + sz;
  sz            += size[1];
  refframe[1]    = yuv_buffer[0] + sz;
  oldrefframe[1] = yuv_buffer[1] + sz;
  auxframe[1]    = yuv_buffer[2] + sz;

  if( scalable_mode == slice_decoder_t::sc_SPAT ) {
/* this assumes lower layer is 4:2:0 */
    sz          = padding[0];
    llframe0[0] = yuv_buffer[3] + sz;
    llframe1[0] = yuv_buffer[4] + sz;
    sz          = padding[1] + size[2];
    llframe0[2] = yuv_buffer[3] + sz;
    llframe1[2] = yuv_buffer[4] + sz;
    sz         += size[3];
    llframe0[1] = yuv_buffer[3] + sz;
    llframe1[1] = yuv_buffer[4] + sz;
  }

/* Initialize the YUV tables for software YUV decoding */
  cr_to_r = new int[256];
  crb_to_g = new int[256*256];
  cb_to_b = new int[256];
  int *cr_to_r_ptr = cr_to_r + 128;
  int *crb_to_g_ptr = crb_to_g + 128*256 + 128;
  int *cb_to_b_ptr = cb_to_b + 128;

  for( i=-128; i<128; ++i ) {
    cr_to_r_ptr[i] = (int)(1.371*i * 65536.);
    for( j=-128; j<128; ++j )
      crb_to_g_ptr[i*256 + j] = (int)((-0.698*i + -0.336*j) * 65536.);
    cb_to_b_ptr[i] = (int)((1.732*i) * 65536.);
  }

  decoder_initted = 1;
  return 0;
}

int zvideo_t::
delete_decoder()
{
  if( yuv_buffer[0] ) delete [] yuv_buffer[0];
  if( yuv_buffer[1] ) delete [] yuv_buffer[1];
  if( yuv_buffer[2] ) delete [] yuv_buffer[2];

  if( subtitle_frame[0] ) delete [] subtitle_frame[0];
  if( subtitle_frame[1] ) delete [] subtitle_frame[1];
  if( subtitle_frame[2] ) delete [] subtitle_frame[2];

  if( llframe0[0] ) {
    delete [] yuv_buffer[3];
    delete [] yuv_buffer[4];
  }
  delete [] tdat;

  delete [] cr_to_r;
  delete [] crb_to_g;
  delete [] cb_to_b;
  return 0;
}

void zvideo_t::
init_scantables()
{
  zigzag_scan_table = zig_zag_scan_nommx;
  alternate_scan_table = alternate_scan_nommx;
}

void zvideo_t::
init_video()
{
  int result;
  init_decoder();
  track->width = horizontal_size;
  track->height = vertical_size;
  track->frame_rate = frame_rate;
  demuxer_t *demux = vstream->demuxer;

  /* Try to get the length of the file from GOP's */
  if( !track->frame_offsets ) {
    if( src->is_video_stream() ) {
      /* Load the first GOP */
      rewind_video(0);
      result = vstream->next_code(GOP_START_CODE);
      if( !result ) vstream->get_bits(32);
      if( !result ) result = get_gop_header();
      first_frame = gop_to_frame(&gop_timecode);
      /* GOPs are supposed to have 16 frames */
      frames_per_gop = 16;
      /* Read the last GOP in the file by seeking backward. */
      demux->seek_byte(demux->movie_size());
      demux->start_reverse();
      result = demux->prev_code(GOP_START_CODE);
      demux->start_forward();
      vstream->reset();
      vstream->get_bits(8);
      if(!result) result = get_gop_header();
      last_frame = gop_to_frame(&gop_timecode);
//zmsgs("3 %p\n", this);
      /* Count number of frames to end */
      while( !result ) {
        result = vstream->next_code(PICTURE_START_CODE);
        if( !result ) {
          vstream->get_byte_noptr();
          ++last_frame;
        }
      }

      track->total_frames = last_frame-first_frame+1;
//zmsgs("mpeg3video_new 3 %ld\n", track->total_frames);
    }
    else {
      /* Try to get the length of the file from the multiplexing. */
      /* Need a table of contents */
/*       first_frame = 0;
 *       track->total_frames = last_frame =
 *         (long)(demux->length() * frame_rate);
 *       first_frame = 0;
 */
    }
  }
  else {
    /* Get length from table of contents */
    track->total_frames = track->total_frame_offsets-1;
  }

  maxframe = track->total_frames;
  rewind_video(0);
}

int zvideo_t::
video_pts_padding()
{
  int result = 0;
  if( track->video_time >= 0 && src->pts_padding > 0 ) {
    int pts_framenum = track->video_time*track->frame_rate + 0.5;
    int padding = pts_framenum - track->pts_position;
    if( padding > 0 ) {
      if( track->vskip <= 0 ) {
        int limit = 3;
        if( padding > limit ) {
          zmsgs("video start padding  pid %02x @ %12d (%12.6f) %d\n",
                track->pid, framenum, track->get_video_time(), padding);
          track->vskip = 1;
          result = 1;
        }
      }
      else if( !(++track->vskip & 1) || padding > track->frame_rate )
        result = 1;
      if( result ) ++track->pts_position;
    }
    else if( track->vskip > 0 ) {
      zmsgs("video end   padding  pid %02x @ %12d (%12.6f) %d\n",
            track->pid, framenum, track->get_video_time(), (1+track->vskip)/2);
      track->vskip = 0;
    }
  }
  return result;
}

int zvideo_t::
video_pts_skipping()
{
  int result = 0;
  if( track->video_time >= 0 && src->pts_padding > 0 ) {
    int pts_framenum = track->video_time*track->frame_rate + 0.5;
    int skipping = track->pts_position - pts_framenum;
    if( skipping > 0 ) {
      if( track->vskip >= 0 ) {
        int limit = 3;
        if( skipping > limit ) {
          zmsgs("video start skipping pid %02x @ %12d (%12.6f) %d frames\n",
                track->pid, framenum, track->get_video_time(), skipping);
          track->vskip = -1;
          result = 1;
        }
      }
      else if( !(--track->vskip & 1) || skipping > track->frame_rate )
        result = 1;
      if( result ) --track->pts_position;
    }
    else if( track->vskip < 0 ) {
        zmsgs("video end   skipping pid %02x @ %12d (%12.6f) %d frames\n",
                track->pid, framenum, track->get_video_time(), (1-track->vskip)/2);
      track->vskip = 0;
    }
  }
  return result;
}

int zvideo_t::
eof()
{
  while( vstream->eof() ) {
    demuxer_t *demux = vstream->demuxer;
    if( demux->seek_phys() ) return 1;
  }
  return 0;
}


/* The way they do field based encoding,  */
/* the I frames have the top field but both the I frame and */
/* subsequent P frame are interlaced to make the keyframe. */

int zvideo_t::
read_picture()
{
  int result = 0;
  int field = 0;
  got_top = got_bottom = 0;
  secondfield = 0;

  do {
    if( (result=eof()) != 0 ) break;
    if( (result=get_header()) != 0 ) break;
    if( pict_struct != pics_FRAME_PICTURE ) secondfield = field;
    /* if dropping frames then skip B frames, */
    /*   Don't skip the B frames which generate original */
    /*   repeated frames at the end of the skip sequence */
    if( !skip_bframes || pict_type != pic_type_B ||
        repeat_fields > 2*skip_bframes ) {
      if( (result=get_picture()) != 0 ) break;
    }
    ++field;
    if( pict_struct == pics_FRAME_PICTURE ) { got_top = got_bottom = field; break; }
    if( pict_struct == pics_TOP_FIELD ) got_top = field;
    else if( pict_struct == pics_BOTTOM_FIELD ) got_bottom = field;
  } while( !secondfield );

  if( pict_type != pic_type_B )
    ++ref_frames;

  return result;
}

int zvideo_t::
read_frame_backend(int zskip_bframes)
{
  int result = 0;
//if( src->seekable ) {
//  int app_pos = track->apparent_position();
//  double vtime = track->get_video_time();
//  int pts_frm = vtime * track->frame_rate + 0.5;
//  zmsgs(" apr_pos %f: %d/%d + %d: pts_frame %d + %d\n", vtime,
//    app_pos, framenum, app_pos - framenum, pts_frm, pts_frm-framenum);
//}
  if( !mpeg2 ) current_field = repeat_fields = 0;

  /* Repeat if current_field is more than 1 field from repeat_fields */
  if( !repeat_fields || current_field+2 >= repeat_fields ) {
    if( (repeat_fields -= current_field) < 0 ) repeat_fields = 0;
    track->update_video_time();

    // if pts lags framenum, skip to next picture
    //  only skip once (double speed) to catch up
    while( !(result=find_header()) ) {
      if( !video_pts_skipping() ) break;
      vstream->refill();
    }

    // if framenum lags pts, repeat picture
    if( !result && !video_pts_padding() ) {
      track->update_frame_pts();
      skip_bframes = zskip_bframes;
//static const char *pstruct[] = { "nil", "top", "bot", "fld" };
//zmsgs("video %d PID %02x frame %d vtime %12.5f %c/%s %d 0x%010lx/0x%010lx %d/%d\n",
//  result, track->pid, framenum, track->get_video_time(), "XIPBD"[pict_type],
//  pstruct[pict_struct], skip_bframes, vstream->demuxer->last_packet_start,
//  vstream->demuxer->absolute_position()-vstream->demuxer->zdata.length(),
//  repeat_fields, current_field);
      result = read_picture();
#if 0
{ char fn[512];
  snprintf(&fn[0],sizeof(fn),"/tmp/dat/f%05d.pnm",framenum);
  int fd = open(&fn[0],O_CREAT+O_TRUNC+O_WRONLY,0666);
  write(fd,&fn, snprintf(&fn[0],sizeof(fn),
    "P5\n%d %d\n255\n", coded_picture_width, coded_picture_height));
  write(fd,output_src[0],coded_picture_width*coded_picture_height);
  close(fd);
}
#endif
    }
  }

  if( !result ) {
    if( mpeg2 ) current_field  = !repeat_fields ? 0 : current_field+2;
    decode_subtitle();
    last_number = framenum++;
  }

  return result;
}

int* zvideo_t::
get_scaletable(int in_l, int out_l)
{
  int *result = new int[out_l];
  double scale = (double)in_l / out_l;
  for( int i=0; i<out_l; ++i ) {
    result[i] = (int)(scale * i);
  }
  return result;
}

long zvideo_t::
gop_to_frame(timecode_t *gop_timecode)
{
/* Mirror of what mpeg2enc does */
  int fps = (int)(frame_rate + 0.5);
  int hour = gop_timecode->hour;
  int minute = gop_timecode->minute;
  int second = gop_timecode->second;
  int frame = gop_timecode->frame;
  long result = ((long)hour*3600 + minute*60 + second)*fps + frame;
  return result;
}


/* ======================================================================= */
/*                                    ENTRY POINTS */
/* ======================================================================= */



zvideo_t::
video_t(zmpeg3_t *zsrc, zvtrack_t *ztrack)
{
  src = zsrc;
  track = ztrack;
  vstream = new bits_t(zsrc, track->demuxer);
//zmsgs("%d\n", vstream->eof());
  last_number = -1;
/* First frame is all green */
  framenum = -1;
  byte_seek = -1;
  frame_seek = -1;
  subtitle_track = -1;

  init_scantables();
  init_output();
  allocate_slice_buffers();
  slice_wait.lock();
}

zvideo_t *zmpeg3_t::
new_video_t(zvtrack_t *ztrack)
{
  int result = 0;
  video_t *new_video = new video_t(this,ztrack);

/* Get encoding parameters from stream */
  if( seekable ) {
    result = new_video->get_header();
    if( !result )
      new_video->init_video();
    else {
/* No header found */
#ifdef TODO
      zerr("no header found.\n");
      delete new_video;
      new_video = 0;
#endif
    }
  }

  return new_video;
}

zvideo_t::
~video_t()
{
  delete_slice_buffers();
  if( decoder_initted )
    delete_decoder();
  delete vstream;
  delete cc;
  if( x_table ) {
    delete [] x_table;
    delete [] y_table;
  }
}

int zvideo_t::
set_cpus(int cpus)
{
  return 0;
}

int zvideo_t::
set_mmx(int use_mmx)
{
  init_scantables();
  return 0;
}

/* Read all the way up to and including the next picture start code */
int zvideo_t::
read_raw(uint8_t *output, long *size, long max_size)
{
  uint32_t zcode = 0;
  long sz = 0;
  while( zcode!=PICTURE_START_CODE && zcode!=SEQUENCE_END_CODE &&
         sz < max_size && !eof() ) {
    uint8_t byte = vstream->get_byte_noptr();
    *output++ = byte;
    zcode = (zcode << 8) | byte;
    ++sz;
  }
  *size = sz;
  return eof();
}

int zvideo_t::read_frame( uint8_t **output_rows,
    int in_x, int in_y, int in_w, int in_h, 
    int out_w, int out_h, int color_model)
{
  uint8_t *y, *u, *v;
  int frame_number, result;
  result = 0;
  want_yvu = 0;
  this->output_rows = output_rows;
  this->color_model = color_model;

/* Get scaling tables */
  if( this->out_w != out_w || this->out_h != out_h ||
      this->in_w != in_w   || this->in_h != in_h ||
      this->in_x != in_x   || this->in_y != in_y) {
    if(x_table) {
      delete [] x_table;  x_table = 0;
      delete [] y_table;  y_table = 0;
    }
  }

  this->out_w = out_w; this->out_h = out_h;
  this->in_w = in_w;   this->in_h = in_h;
  this->in_x = in_x;   this->in_y = in_y;

  if( !x_table ) {
    x_table = get_scaletable(in_w, out_w);
    y_table = get_scaletable(in_h, out_h);
  }
//zmsgs("mpeg3video_read_frame 1 %d\n", framenum);

/* Recover from cache */
  frame_number = frame_seek >= 0 ? frame_seek : framenum;
  cache_t *cache = track->frame_cache;
  if( frame_seek != last_number &&
      cache->get_frame(frame_number, &y, &u, &v) ) {
//zmsgs("1 %d\n", frame_number);
    /* Transfer with cropping */
    if( y ) present_frame(y, u, v);
    /* Advance either framenum or frame_seek */
    if( frame_number == framenum )
      framenum = ++frame_number;
    else if( frame_number == frame_seek )
      frame_seek = ++frame_number;
  }
  else {
    /* Only decode if it's a different frame */
    if( frame_seek < 0 || last_number < 0 ||
        frame_seek != last_number) {
      if( !result ) result = seek();
      if( !result ) result = read_frame_backend(0);
    }
    else {
      framenum = frame_seek + 1;
      last_number = frame_seek;
      frame_seek = -1;
    }
    if( output_src[0] )
      present_frame(output_src[0], output_src[1], output_src[2]);
  }
  return result;
}

int zvideo_t::
read_yuvframe(char *y_output, char *u_output, char *v_output,
              int in_x, int in_y, int in_w, int in_h)
{
  uint8_t *y = 0, *u = 0, *v = 0;
  int result = 0;
//zmsgs("1 %d\n", framenum);
  want_yvu = 1;
  this->y_output = y_output;
  this->u_output = u_output;
  this->v_output = v_output;
  this->in_x = in_x;
  this->in_y = in_y;
  this->in_w = in_w;
  this->in_h = in_h;

  /* Recover from cache if framenum exists */
  int frame_number = frame_seek >= 0 ? frame_seek : framenum;
  cache_t *cache = track->frame_cache;
  if( cache->get_frame(frame_number, &y, &u, &v) ) {
//zmsgs("1 %d\n", frame_number);
    /* Transfer with cropping */
    if( y ) present_frame(y, u, v);
    /* Advance either framenum or frame_seek */
    if( frame_number == framenum )
      framenum = ++frame_number;
    else if( frame_number == frame_seek )
      frame_seek = ++frame_number;
  }
  else {
    if( !result ) result = seek();
    if( !result ) result = read_frame_backend(0);
    if( !result && output_src[0] )
      present_frame(output_src[0], output_src[1], output_src[2]);
  }

  want_yvu = 0;
  byte_seek = -1;
  return result;
}

int zvideo_t::
read_yuvframe_ptr(char **y_output, char **u_output, char **v_output)
{
  uint8_t *y, *u, *v;
  int frame_number, result;
  int debug = 0;
  result = 0;
  want_yvu = 1;
  *y_output = *u_output = *v_output = 0;

  frame_number = frame_seek >= 0 ? frame_seek : framenum;
  cache_t *cache = track->frame_cache;
  if( debug ) zmsgs("%d\n", __LINE__);
  if( cache->get_frame(frame_number, &y, &u, &v) ) {
    if( debug ) zmsgs("%d\n", __LINE__);
    *y_output = (char*)y;
    *u_output = (char*)u;
    *v_output = (char*)v;
    /* Advance either framenum or frame_seek */
    if( frame_number == framenum )
      framenum = ++frame_number;
    else if( frame_number == frame_seek )
      frame_seek = ++frame_number;
    if( debug ) zmsgs("%d\n", __LINE__);
  }
  /* Only decode if it's a different frame */
  else if( frame_seek < 0 || last_number < 0 ||
           frame_seek != last_number) {
    if( debug ) zmsgs("%d\n", __LINE__);
    if( !result ) result = seek();
    if( debug ) zmsgs("%d\n", __LINE__);
    if( !result ) result = read_frame_backend(0);
    if( debug ) zmsgs("%d\n", __LINE__);

    if( output_src[0] ) {
      *y_output = (char*)output_src[0];
      *u_output = (char*)output_src[1];
      *v_output = (char*)output_src[2];
    }
    if( debug ) zmsgs("%d\n", __LINE__);
  }
  else {
    if( debug ) zmsgs("%d\n", __LINE__);
    framenum = frame_seek + 1;
    last_number = frame_seek;
    frame_seek = -1;

    if( output_src[0 ]) {
      *y_output = (char*)output_src[0];
      *u_output = (char*)output_src[1];
      *v_output = (char*)output_src[2];
    }
    if( debug ) zmsgs("%d\n", __LINE__);
  }

  if( debug ) zmsgs("%d\n", __LINE__);
  want_yvu = 0;
/* Caching not used if byte seek */
  byte_seek = -1;
  return result;
}

int zvideo_t::
colormodel()
{
  switch( chroma_format ) {
    case cfmt_422: return cmdl_YUV422P; break;
    case cfmt_420: return cmdl_YUV420P; break;
  }
  return cmdl_YUV420P;
}

zcc_t *zvideo_t::
get_cc()
{
  if( !cc )
    cc = new cc_t(this);
  return cc;
}

void zvideo_t::
reset_subtitles()
{
  for( int i=0; i<src->total_stracks; ++i ) {
    strack_t *strack = src->strack[i];
    if( strack->video != this ) continue;
    strack->del_all_subtitles();
  }
}

int zvideo_t::
show_subtitle(int strk)
{
  int ret = subtitle_track;
  if( subtitle_track != strk ) {
    reset_subtitles();
    subtitle_track = strk;
    if( cc ) {
      if( strk >= 0 ) cc->reset();
      else { delete cc;  cc = 0; }
    }
  }
  return ret;
}

void zvideo_t::
dump()
{
  zmsg("\n");
  zmsg(" *** sequence extension 1\n");
  zmsgs("prog_seq=%d\n", prog_seq);
  zmsg(" *** picture header 1\n");
  zmsgs("pict_type=%d field_sequence=%d\n", pict_type, field_sequence);
  zmsg(" *** picture coding extension 1\n");
  zmsgs("field_sequence=%d repeatfirst=%d prog_frame=%d pict_struct=%d\n", 
    field_sequence, repeatfirst, prog_frame, pict_struct);
}

