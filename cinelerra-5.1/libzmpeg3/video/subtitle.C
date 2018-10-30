#include "../libzmpeg3.h"

zsubtitle_t::
subtitle_t()
{
//zmsgs("subtitle=%p\n",this);
}

// closed caption or display
zsubtitle_t::
subtitle_t(int nid, int ww, int hh)
{
//zmsgs("subtitle=%p %dx%d\n", this, ww, hh);
  id = nid;
  done = -1;
  active = -1;
  draw = -1;
  set_image_size(ww, hh);
}

zsubtitle_t::
~subtitle_t()
{
//zmsgs("subtitle=%p/%d subtitle->data=%p %d %d\n", 
//  this, id, data, data_allocated, data_used);
  delete [] data;
  delete [] image_y;
  delete [] image_u;
  delete [] image_v;
  delete [] image_a;
}

void zsubtitle_t::
set_image_size(int isz)
{
//zmsgs("subtitle=%p/%d\n", this, isz);
  sz = isz;
  /* Allocate image buffers */
  delete [] image_y;
  delete [] image_u;
  delete [] image_v;
  delete [] image_a;
  image_y = znew(uint8_t,isz);
  image_u = znew(uint8_t,isz);
  image_v = znew(uint8_t,isz);
  image_a = znew(uint8_t,isz);
}

void zsubtitle_t::
set_image_size(int ww, int hh)
{
  w = ww;  h = hh;
  int isz = ww*hh + ww;
  if( isz > sz ) set_image_size(isz);
}

/* Returns 1 if failure */
/*   is_8bit mode has not been tested (no test media) */
int zsubtitle_t::
decompress_subtitle(zmpeg3_t *zsrc)
{
  //int got_alpha = 0;
  int is_8bit = 0;
  uint8_t *yuv_palette = 0;
  uint8_t *yuv_alpha = 0;
  int ofsz = 16;
  bits_t st(zsrc,0);
  st.use_ptr(data);
  uint8_t *end = data + data_used;
  if( st.eos(end,16) ) return 1;
  int pkt_sz = st.get_bits(16);
  if( pkt_sz == 0 ) ofsz = 32;
  int field_offset[3];
  field_offset[0] = 0; // begining of even field
  field_offset[1] = 0; // end of even field, begining odd field
  if( st.eos(end,ofsz) ) return 1;
  /* end of odd field, beginning of ctl seq */
  field_offset[2] = st.get_bits(ofsz);
//zmsgs("%d 0x%02x%02x data_used=%d data_size=%d\n", 
//  __LINE__, data[0],data[1],data_used,field_offset[2]);
  if( data + field_offset[2] > end ) return 1;
  palette[0] = 0x00;  palette[1] = 0x01;
  palette[2] = 0x02;  palette[3] = 0x03;
  alpha[0]   = 0xff;  alpha[1]   = 0xff;
  alpha[2]   = 0xff;  alpha[3]   = 0xff;
  int ww = 0, hh = 0;
  x1 = 0;  x2 = 0;
  y1 = 0;  y2 = 0;
  force = 0;
  start_time = stop_time = 0;

  /* Advance to control sequences */
  uint8_t *ptr = data + field_offset[2];
  uint8_t *last_control_start = 0;
  uint8_t *next_control_start = ptr;

  while( last_control_start != next_control_start ) {
    st.use_ptr(next_control_start);
    if( st.eos(end,16) ) break;  /* date */
    int date = st.get_bits(16) * 10;
    /* Offset of next control sequence */
    if( st.eos(end,ofsz) ) break;
    int next = st.get_bits(ofsz);
    last_control_start = next_control_start;
    next_control_start = data + next;
    if( next_control_start > end ) return 1;

    int done = 0;
    while( !done && !st.eos(end,8) ) {
      int type = st.get_bits(8);
      switch( type ) {
        case 0x00:
          force = 1;
          break;
        case 0x01:
          start_time = date;
//zmsgs("start_time %d\n", start_time);
          break;
        case 0x02:
          stop_time = date;
//zmsgs("stop_time %d\n", stop_time);
          break;
        case 0x83:
          if( st.eos(end,768*8) ) return 1;
          yuv_palette = st.input_ptr;
          st.input_ptr += 768;
          break;
        case 0x03:
          /* Entry in palette of each color */
          if( st.eos(end,16) ) return 1;
          for( int i=4; --i>=0; ) palette[i] = st.get_bits(4);
//zmsgs("palette %d %d %d %d\n", palette[0], palette[1], palette[2], palette[3]);
          break;
        case 0x84:
          if( st.eos(end,256*8) ) return 1;
          yuv_alpha = st.input_ptr;
          st.input_ptr += 256;
          break;
        case 0x04:
          /* Alpha corresponding to each color */
          if( st.eos(end,16) ) return 1;
          for( int i=4; --i>=0; ) alpha[i] = st.get_bits(4)*255 / 15;
          //got_alpha = 1;
//zmsgs("alphas %d %d %d %d\n", alpha[0], alpha[1], alpha[2], alpha[3]);
#if 0
          alpha[3] = 0xff; alpha[2] = 0x80;
          alpha[1] = 0x40; alpha[0] = 0x00;
#endif
          break;
        case 0x85:
          is_8bit = 1; // fall thru
        case 0x05:
          /* Extent of image on screen */
          if( st.eos(end,48) ) return 1;
          x1 = st.get_bits(12) & 0x7ff;
          x2 = st.get_bits(12) + 1;
          y1 = st.get_bits(12) & 0x7ff;
          y2 = st.get_bits(12) + 1;
          ww = x2 - x1;
          hh = y2 - y1;
//zmsgs("20 x1=%d x2=%d y1=%d y2=%d, %dx%d\n", 
//  x1, x2, y1, y2, ww, hh);
          ww = clip(ww, 1, 2048);  hh = clip(hh, 1, 2048);
          x1 = clip(x1, 0, 2048);  x2 = clip(x2, 0, 2048);
          y1 = clip(y1, 0, 2048);  y2 = clip(y2, 0, 2048);
          break;
        case 0x86:
          if( st.eos(end,64) ) return 1;
          field_offset[0] = st.get_bits(32);
          field_offset[1] = st.get_bits(32);
//zmsgs("offsets even=0x%x odd=0x%x\n", field_offset[0], field_offset[1]);
          break;
        case 0x06:
/* offsets of even and odd field in compressed data */
          if( st.eos(end,32) ) return 1;
          field_offset[0] = st.get_bits(16);
          field_offset[1] = st.get_bits(16);
//zmsgs("offsets even=0x%x odd=0x%x\n", field_offset[0], field_offset[1]);
          break;
        case 0xff:
          done = 1;
          break;
        default:
//zmsgs("unknown type %02x\n", type);
          return 1;
      }
    }
  }

  if( ww <= 0 || hh <= 0 || !field_offset[0] ) return 1;
  if( is_8bit && !yuv_palette ) return 1;
  set_image_size(ww, hh);

  /* Decode image */
  int x = 0, y = 0;
  int field = 0;

  while( field < 2 ) {
    y = field;
    ptr = data + field_offset[field++];
    end = data + field_offset[field];
    st.use_ptr(ptr);
    x = 0;
    while( !st.eos(end) ) {
      int len, y_color, u_color, v_color, a_color;
      if( !is_8bit ) {
        if( st.eos(end,4) ) break;
        uint32_t zcode = st.get_bits(4);
        if( zcode < 0x4 ) {
          if( st.eos(end,4) ) break;
          zcode = (zcode << 4) | st.get_bits(4);
          if( zcode < 0x10 ) {
            if( st.eos(end,4) ) break;
            zcode = (zcode << 4) | st.get_bits(4);
            if( zcode < 0x40 ) {
              if( st.eos(end,4) ) break;
              zcode = (zcode << 4) | st.get_bits(4);
              if( zcode < 0x4 ) /* carriage return */
                zcode |= (w - x) << 2;
            }
          }
        }
        int color = (zcode & 0x3);
        len = zcode >> 2;
        int n = w - x;
        if( n < len ) len = n;
        int k = 4*palette[color];
        y_color = zsrc->palette[k+0];
        u_color = zsrc->palette[k+1];
        v_color = zsrc->palette[k+2];
        a_color = alpha[color];
      }
      else {
        if( st.eos(end,1) ) break;
        int has_run = st.get_bits(1);
        if( st.eos(end,1) ) break;
        int sz = st.get_bits(1) ? 8 : 2;
        if( st.eos(end,sz) ) break;
        int color = st.get_bits(sz);
        if( has_run ) {
          if( st.eos(end,1) ) break;
          if( st.get_bits(1) ) {
            if( st.eos(end,7) ) break;
            int n = st.get_bits(7);
            len = n == 0 ? w - x : n + 9;
          }
          else {
            if( st.eos(end,3) ) break;
            len = st.get_bits(3) + 2;
          }
        }
        else
          len = 1;
        int k = 3*color;
        y_color = yuv_palette[k+0];
        u_color = yuv_palette[k+1];
        v_color = yuv_palette[k+2];
        a_color = yuv_alpha ? yuv_alpha[color] : 0xff;
      }
//zmsgs("%d, 0x%02x 0x%02x 0x%02x %02x\n",len,y_color,u_color,v_color,a_color);
      int ofs = y * w + x;
      memset(image_y+ofs,y_color,len);
      memset(image_u+ofs,u_color,len);
      memset(image_v+ofs,v_color,len);
      memset(image_a+ofs,a_color,len);
      if( (x+=len) >= w ) {
        st.next_byte_align();
        if( (y+=2) >= h ) break;
        x = 0;
      }
    }
  }

#if 0
/* this code computes a threshold (pass 0),
 *  and forces pixels below the threshold (pass 1)
 *  to black and the pixels above to white, as 2x2 yuv macropixels */
// Normalize image colors
  float min_h = 360;
  float max_h = 0;
  float threshold = 0;
#define HISTOGRAM_SIZE 1000
// Decompression coefficients straight out of jpeglib
#define V_TO_R    1.40200
#define V_TO_G    -0.71414

#define U_TO_G    -0.34414
#define U_TO_B    1.77200
  uint8_t histogram[HISTOGRAM_SIZE];
  bzero(histogram, HISTOGRAM_SIZE);
  for( int pass=0; pass<2; ++pass ) {
    for( y=0; y<h; ++y ) {
      for( x=0; x<w; ++x ) {
        int ofs = y*w + x;
        if( image_a[ofs]) {
          uint8_t *y_color = image_y + ofs;
          uint8_t *a_color = image_a + ofs;
          uint8_t *u_color = image_u + ofs;
          uint8_t *v_color = image_v + ofs;

          /* Convert to RGB */
          float r = (*y_color + *v_color * V_TO_R);
          float g = (*y_color + *u_color * U_TO_G + *v_color * V_TO_G);
          float b = (*y_color + *u_color * U_TO_B);

          /* Multiply alpha */
          /* r = r * *a_color / 0xff; */
          /* g = g * *a_color / 0xff; */
          /* b = b * *a_color / 0xff; */
          /* Convert to HSV */
          float h, s, v;
          float min, max, delta;
          min = ((r < g) ? r : g) < b ? ((r < g) ? r : g) : b;
          max = ((r > g) ? r : g) > b ? ((r > g) ? r : g) : b;
          v = max; 
          delta = max - min;
          if( max != 0 && delta != 0 ) {
            s = delta / max;              /* s */
            if( r == max )
              h = (g - b) / delta;        /* between yellow & magenta */
            else if( g == max )
              h = 2 + (b - r) / delta;    /* between cyan & yellow */
            else
              h = 4 + (r - g) / delta;    /* between magenta & cyan */

            h *= 60;                      /* degrees */
            if( h < 0 ) h += 360;
          }
          else {
            /* r = g = b = 0; */          /* s = 0, v is undefined */
            s = 0;
            h = -1;
          }
          /* int magnitude = (int)(*y_color * *y_color +  */
          /*   *u_color * *u_color +  */
          /*   *v_color * *v_color); */

          /* Multiply alpha */
          h = (h * *a_color) / 0xff;
          if( pass == 0 ) {
            ++histogram[(int)h];
            if( h < min_h ) min_h = h;
            if( h > max_h ) max_h = h;
          }
          else {
// Set new color in a 2x2 pixel block
            if( h > threshold ) {
              *y_color = 0xff;
            }
            else {
              *y_color = 0;
            }

            *u_color = 0x80;
            *v_color = 0x80;
            *a_color = 0xff;
          }
        }
      }
    }

    if( pass == 0 ) {
      /* int hist_total = 0;                  */
      /* for( i=0; i<HISTOGRAM_SIZE; ++i )    */
      /*   hist_total += histogram[i];        */
      /* int hist_count = 0;                  */
      /* for( i=0; i<HISTOGRAM_SIZE; ++i ) {  */
      /*   hist_count += histogram[i];        */
      /*   if( hist_count > hist_total/3 ) {  */
      /*     threshold = i;                   */
      /*     break;                           */
      /*   }                                  */
      /* }                                    */
      threshold = (min_h + max_h) / 2;
//    threshold = 324;
//zmsgs("min_h=%f max_h=%f threshold=%f\n", min_h, max_h, threshold);
    }
  }
/* end b/w threshold code */
#endif

//zmsgs("coords: %d,%d - %d,%d size: %d,%d start_time=%d end_time=%d\n", 
//  x1, y1, x2, y2, w, h, start_time, stop_time);
  return 0;
}

void zvideo_t::
overlay_subtitle(zsubtitle_t *subtitle)
{
  uint8_t *img_y = subtitle->image_y;
  uint8_t *img_u = subtitle->image_u;
  uint8_t *img_v = subtitle->image_v;
  uint8_t *img_a = subtitle->image_a;

  if( !img_y || !img_u || !img_v || !img_a ) return;
  int iofs = 0;
  int x1 = subtitle->x1;
  if( x1 < 0 ) { iofs += -x1;  x1 = 0; }
  int x2 = subtitle->x2;
  if( x2 > coded_picture_width ) x2 = coded_picture_width;
  int y1 = subtitle->y1;
  if( y1 < 0 ) { iofs += -y1*subtitle->w;  y1 = 0; }
  int y2 = subtitle->y2;
  if( y2 > coded_picture_height ) y2 = coded_picture_height;

  for( int y=y1; y<y2; ++y ) {
    int oofs = y * coded_picture_width + x1;
    uint8_t *output_y = subtitle_frame[0] + oofs;
    oofs = y/2 * chrom_width + x1/2;
    uint8_t *output_u = subtitle_frame[1] + oofs;
    uint8_t *output_v = subtitle_frame[2] + oofs;
    int ofs = iofs + (y-y1) * subtitle->w;
    uint8_t *input_y = img_y + ofs;
    uint8_t *input_u = img_u + ofs;
    uint8_t *input_v = img_v + ofs;
    uint8_t *input_a = img_a + ofs;

    for( int x=x1; x<x2; ++x ) {
      int opacity = *input_a;
      int transparency = 0xff - opacity;
      *output_y = (*input_y*opacity + *output_y*transparency) / 0xff;
      ++output_y;
      if( !(y & 1) && !(x & 1) ) {
        *output_u = (*input_u*opacity + *output_u*transparency) / 0xff;
        *output_v = (*input_v*opacity + *output_v*transparency) / 0xff;
        ++output_u;  ++output_v;
      }
      ++input_y;  ++input_a;
      ++input_u;  ++input_v;
    }
  }
}

void zvideo_t::
new_output()
{
  int sz = coded_picture_width * coded_picture_height;
  int isz = chrom_width * chrom_height;
  if( !subtitle_frame[0] ) {
    subtitle_frame[0] = new uint8_t[sz+8];
    subtitle_frame[1] = new uint8_t[isz+8];
    subtitle_frame[2] = new uint8_t[isz+8];
  }

  memcpy(subtitle_frame[0], output_src[0], sz);
  memcpy(subtitle_frame[1], output_src[1], isz);
  memcpy(subtitle_frame[2], output_src[2], isz);

  output_src[0] = subtitle_frame[0];
  output_src[1] = subtitle_frame[1];
  output_src[2] = subtitle_frame[2];
}

int zsubtitle_t::
decode(video_t *video)
{
//zmsgs("id=%d strk=%d i=%d start=%f stop=%f active=%d draw=%d framenow=%f\n",
// subtitle->id,strk,i,subtitle->start_frametime,subtitle->stop_frametime,
// subtitle->active,subtitle->draw,frame_now);
  double framerate = video->frame_rate;
  if( !framerate ) framerate = 30000./1001;
  if( !active ) {
    active = 1;
    if( decompress_subtitle(video->src) ) return -1;
    int start_frms = framerate * start_time/1000.;
    int stop_frms = framerate * stop_time/1000.;
    start_frame = video->framenum + start_frms;
    stop_frame = video->framenum + stop_frms;
    frame_time = video->frame_time;
  }
  int life_time = video->frame_time - frame_time;
  if( life_time > 20*framerate ) return -1; // doesn't live forever
  if( draw < 0 ) return 0;  // never draw
  if( draw || force > 0 ) return 1;  // always draw
  if( video->seek_time > 2 ) {
    /* at least 2 consecutive frames shown since last seek */
    int hold_frames = framerate * SUBTITLE_HOLD_TIME;
    int early_frame = start_frame-hold_frames;
    if( video->framenum < early_frame ) return -1; // way too early
    int late_frame = stop_frame+hold_frames;
    if( video->framenum > late_frame ) return -1; // way too late
  }
  if( video->framenum < start_frame ) return 0; // too early
  if( video->framenum >= stop_frame ) return -1;  // too late
  return 1;
}

void zvideo_t::
decode_subtitle()
{
  if( !output_src[0] ) return;
  int copied = 0;
  for( int strk=0; strk<src->subtitle_tracks(); ++strk ) {
    strack_t *strack = src->strack[strk];
    if( !strack ) continue;
    if( strack->video && strack->video != this ) continue;
    strack->rwlock.enter();
    int i = 0;
    while( i < strack->total_subtitles ) {
      subtitle_t *subtitle = strack->subtitles[i];
      int ret = -1;
      if( strack->id == subtitle_track || subtitle->force ) {
        if( (ret=subtitle->decode(this)) > 0 ) {
          if( !copied ) { copied = 1; new_output(); }
          overlay_subtitle(subtitle);
        }
      }
      if( ret < 0 ) {
        strack->del_subtitle(subtitle, -1);
        if( subtitle->force < 0 ) delete subtitle;
        continue;
      }
      ++i;
    }
    strack->rwlock.leave();
  }
}

int zmpeg3_t::
display_subtitle(int stream, int sid, int id,
   uint8_t *yp, uint8_t *up, uint8_t *vp, uint8_t *ap,
   int x, int y, int w, int h, double start_msecs, double stop_msecs)
{
  int ret = 1;
  if( stream >= 0 && stream < total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    video_t *video = vtrk->video;
    strack_t *strack = create_strack(sid, video);
    strack->rwlock.write_enter();
    subtitle_t *subtitle = 0;
    for( int i=strack->total_subtitles; --i>=0 && !subtitle; ) {
      subtitle_t *sp = strack->subtitles[i];
      if( sp->id == id ) subtitle = sp;
    }
    if( !subtitle ) {
      subtitle = new subtitle_t(id, w, h);
      if( strack->append_subtitle(subtitle,0) ) {
        delete subtitle; subtitle = 0;
      }
    }
    else
      subtitle->set_image_size(w, h);
    if( subtitle ) {
      subtitle->x1 = x;  subtitle->x2 = x + w;
      subtitle->y1 = y;  subtitle->y2 = y + h;
      int sz = w * h;
      if( yp ) memcpy(subtitle->image_y, yp, sz);
      else memset(subtitle->image_y, 0xff, sz);
      if( up ) memcpy(subtitle->image_u, up, sz);
      else memset(subtitle->image_u, 0x80, sz);
      if( vp ) memcpy(subtitle->image_v, vp, sz);
      else memset(subtitle->image_v, 0x80, sz);
      if( ap ) memcpy(subtitle->image_a, ap, sz);
      else memset(subtitle->image_a, 0xff, sz);
      subtitle->start_time = start_msecs;
      subtitle->stop_time = stop_msecs;
      double framerate = vtrk->frame_rate;
      if( !framerate ) framerate = 30000./1001;
      int start_frms = framerate * start_msecs/1000.;
      int stop_frms = framerate * stop_msecs/1000.;
      subtitle->start_frame = video->framenum + start_frms;
      if( stop_msecs ) {
        subtitle->stop_frame = video->framenum + stop_frms;
        subtitle->frame_time = video->frame_time;
      }
      else // lives forever
        subtitle->frame_time = subtitle->stop_frame = INT_MAX;
      subtitle->force = -1;
      subtitle->draw = 0;
      ret = 0;
    }
    strack->rwlock.write_leave();
  }
  return ret;
}

int zmpeg3_t::
delete_subtitle(int stream, int sid, int id)
{
  int result = 1;
  if( stream >= 0 && stream < total_vtracks ) {
    vtrack_t *vtrk = vtrack[stream];
    video_t *vid = vtrk->video;
    strack_t *strack = get_strack_id(sid, vid);
    if( strack != 0 ) {
      strack->rwlock.write_enter();
      for( int i=strack->total_subtitles; --i>=0; ) {
        subtitle_t *sp = strack->subtitles[i];
        if( sp->id == id ) {
          strack->del_subtitle(i);
          if( sp->force < 0 ) delete sp;
          result = 0;
          break;
        }
      }
      strack->rwlock.write_leave();
    }
  }
  return result;
}

