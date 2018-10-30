#include "../libzmpeg3.h"

/* Algorithm */
/*   r = (int)(*y + 1.371 * (*cr - 128)); */
/*   g = (int)(*y - 0.698 * (*cr - 128) - 0.336 * (*cb - 128)); */
/*   b = (int)(*y + 1.732 * (*cb - 128)); */


#define INPUT_420 \
    int yi = y_table[h] + in_y; int uvi = (yi >> 1) * chrom_width; \
    uint8_t *y_in = &yy[yi * coded_picture_width] + in_x; \
    uint8_t *cb_in = &uu[uvi] + in_x1; \
    uint8_t *cr_in = &vv[uvi] + in_x1

#define INPUT_422 \
    int yi = y_table[h] + in_y; int uvi = yi * chrom_width; \
    uint8_t *y_in = &yy[yi * coded_picture_width] + in_x; \
    uint8_t *cb_in = &uu[uvi] + in_x1; \
    uint8_t *cr_in = &vv[uvi] + in_x1

#define OUTPUT_420 \
    uint8_t *y_out = output_rows[h]; int h1 = h >> 1; \
    uint8_t *u_out = output_rows[h1+offset0]; \
    uint8_t *v_out = output_rows[h1+offset2]

#define OUTPUT_422 \
    uint8_t *y_out = output_rows[h]; \
    uint8_t *u_out = output_rows[h+offset0]; \
    uint8_t *v_out = output_rows[h+offset1]

static int z601[256];
#define Y_YUV(v) (v)
#define Y_601(v) z601[v]

/* RGB output */

#define RGB_HEAD(fmt) \
  for( int h=0; h<ht; ++h ) { INPUT_##fmt; \
    uint8_t *rgb = output_rows[h];

#define RGB_TAIL \
  }

#define RGB_UNSCALED(out,csp) \
  for( int w=0, iuv=0; w<wd; w+=2, ++iuv ) { \
    int cr = cr_in[iuv], cb = cb_in[iuv]; \
    int y_l = Y_##csp(y_in[w+0]) << 16; { \
    STORE_##out##_R(y_l + cr2r[cr]); \
    STORE_##out##_G(y_l + crb2g[(cr<<8)|cb]); \
    STORE_##out##_B(y_l + cb2b[cb]); \
    STORE_##out##_A(0); } \
    if( w+1 >= wd ) break; \
    y_l = y_in[w+1] << 16; { \
    STORE_##out##_R(y_l + cr2r[cr]); \
    STORE_##out##_G(y_l + crb2g[(cr<<8)|cb]); \
    STORE_##out##_B(y_l + cb2b[cb]); \
    STORE_##out##_A(0); } \
  }

#define RGB_SCALED(out,csp) \
  for( int w=0; w<wd; ++w ) { \
    int ix = x_table[w]; int iuv = ix >> 1; \
    int cr = cr_in[iuv], cb = cb_in[iuv]; \
    int y_l = Y_##csp(y_in[ix]) << 16; \
    STORE_##out##_R(y_l + cr2r[cr]); \
    STORE_##out##_G(y_l + crb2g[(cr<<8)|cb]); \
    STORE_##out##_B(y_l + cb2b[cb]); \
    STORE_##out##_A(0); \
  }

#define STORE_BGR888_R(v) rgb[2] = clip((v) >> 16)
#define STORE_BGR888_G(v) rgb[1] = clip((v) >> 16)
#define STORE_BGR888_B(v) rgb[0] = clip((v) >> 16)
#define STORE_BGR888_A(v) rgb += 3

#define STORE_BGRA8888_R(v) rgb[2] = clip((v) >> 16)
#define STORE_BGRA8888_G(v) rgb[1] = clip((v) >> 16)
#define STORE_BGRA8888_B(v) rgb[0] = clip((v) >> 16)
#define STORE_BGRA8888_A(v) rgb[3] = 255;  rgb += 4

#define STORE_RGB565_R(v) int r = clip((v) >> 16)
#define STORE_RGB565_G(v) int g = clip((v) >> 16)
#define STORE_RGB565_B(v) int b = clip((v) >> 16)
#define STORE_RGB565_A(v) *((uint16_t*)rgb) = \
  ((r&0xf8)<<8) | ((g&0xfc)<<3) | ((b&0xf8)>>3); \
  rgb += 2

#define STORE_RGB888_R(v) rgb[0] = clip((v) >> 16)
#define STORE_RGB888_G(v) rgb[1] = clip((v) >> 16)
#define STORE_RGB888_B(v) rgb[2] = clip((v) >> 16)
#define STORE_RGB888_A(v) rgb += 3

#define STORE_RGBA8888_R(v) rgb[0] = clip((v) >> 16)
#define STORE_RGBA8888_G(v) rgb[1] = clip((v) >> 16)
#define STORE_RGBA8888_B(v) rgb[2] = clip((v) >> 16)
#define STORE_RGBA8888_A(v) rgb[3] = 255;  rgb += 4

#define STORE_RGBA16161616_R(v) rgb_s[0] = clip((v) >> 8, -32768, 32767)
#define STORE_RGBA16161616_G(v) rgb_s[1] = clip((v) >> 8, -32768, 32767)
#define STORE_RGBA16161616_B(v) rgb_s[2] = clip((v) >> 8, -32768, 32767)
#define STORE_RGBA16161616_A(v) rgb_s[3] = 32767;  rgb_s += 4


/* YUV output */

#define YUV_HEAD_420 \
  for( int h=0; h<ht; ++h ) { INPUT_420; \
    uint8_t *yuv_out = output_rows[h];

#define YUV_HEAD_422 \
  for( int h=0; h<ht; ++h ) { INPUT_422; \
    uint8_t *yuv_out = output_rows[h];

#define YUV_TAIL \
  }

#define YUV_SCALED_YUV888(csp) \
  for( int w=0; w<wd; ) { \
    int ix = x_table[w++]; int iuv = ix >> 1; \
    STORE_Y(Y_##csp(y_in[ix])); STORE_U(cb_in[iuv]); \
    STORE_V(cr_in[iuv]); \
  }

#define YUV_SCALED_YUVA8888(csp) \
  for( int w=0; w<wd; ) { \
    int ix = x_table[w++]; int iuv = ix >> 1; \
    STORE_Y(Y_##csp(y_in[ix])); STORE_U(cb_in[iuv]); \
    STORE_V(cr_in[iuv]); STORE_A(255); \
  }

#define YUV_SCALED_YUYV(csp) \
  for( int w=0; w<wd; ) { \
    int ix = x_table[w++]; int iuv = ix >> 1; \
    STORE_Y(Y_##csp(y_in[ix])); STORE_U(cb_in[iuv]); \
    ix = x_table[w++]; iuv = ix >> 1; \
    STORE_Y(Y_##csp(y_in[ix])); STORE_V(cr_in[iuv]); \
  }

#define YUV_SCALED_UYVY(csp) \
  for( int w=0; w<wd; ) { \
    int ix = x_table[w++]; int iuv = ix >> 1; \
    STORE_U(cb_in[iuv]); STORE_Y(Y_##csp(y_in[ix])); \
    ix = x_table[w++]; iuv = ix >> 1; \
    STORE_V(cr_in[iuv]); STORE_Y(Y_##csp(y_in[ix])); \
  }

#define YUV_UNSCALED_YUV888(csp) \
  for( int w=0, iuv=0; w<wd; w+=2, ++iuv ) { \
    STORE_Y(Y_##csp(y_in[w+0])); STORE_U(cb_in[iuv]); STORE_V(cr_in[iuv]); \
    if( w+1 >= wd ) break; \
    STORE_Y(Y_##csp(y_in[w+1])); STORE_U(cb_in[iuv]); STORE_V(cr_in[iuv]); \
  }

#define YUV_UNSCALED_YUVA8888(csp) \
  for( int w=0, iuv=0; w<wd; w+=2, ++iuv ) { \
    STORE_Y(Y_##csp(y_in[w+0])); STORE_U(cb_in[iuv]); \
    STORE_V(cr_in[iuv]); STORE_A(255); \
    if( w+1 >= wd ) break; \
    STORE_Y(Y_##csp(y_in[w+1])); STORE_U(cb_in[iuv]); \
    STORE_V(cr_in[iuv]); STORE_A(255); \
  }

#define YUV_UNSCALED_YUYV(csp) \
  for( int w=0, iuv=0; w<wd; ++iuv ) { \
    STORE_Y(Y_##csp(y_in[w++])); STORE_U(cb_in[iuv]); \
    STORE_Y(Y_##csp(y_in[w++])); STORE_V(cr_in[iuv]); \
  }

#define YUV_UNSCALED_UYVY(csp) \
  for( int w=0, iuv=0; w<wd; ++iuv ) { \
    STORE_U(cb_in[iuv]); STORE_Y(Y_##csp(y_in[w++])); \
    STORE_V(cr_in[iuv]); STORE_Y(Y_##csp(y_in[w++])); \
  }

#define STORE_Y(v) *yuv_out++ = (v)
#define STORE_U(v) *yuv_out++ = (v)
#define STORE_V(v) *yuv_out++ = (v)
#define STORE_A(v) *yuv_out++ = (v)

/* YUVP output */


#define YUV420P_HEAD_420 \
  for( int h=0; h<ht; ++h ) { INPUT_420; OUTPUT_420;

#define YUV422P_HEAD_420 \
  for( int h=0; h<ht; ++h ) { INPUT_420; OUTPUT_422;

#define YUV420P_HEAD_422 \
  for( int h=0; h<ht; ++h ) { INPUT_422; OUTPUT_420;

#define YUV422P_HEAD_422 \
  for( int h=0; h<ht; ++h ) { INPUT_422; OUTPUT_422;

#define YUVP_TAIL \
  }

#define YUV_SCALED_YUVP(CSP) \
  for( int w=0; w<wd; ) { \
    int ix = x_table[w++]; int iuv = ix >> 1; \
    STORE_UP(cb_in[iuv]); STORE_YP(Y_##CSP(y_in[ix])); \
    ix = x_table[w++]; iuv = ix >> 1; \
    STORE_VP(cr_in[iuv]); STORE_YP(Y_##CSP(y_in[ix])); \
  }

#define YUV_UNSCALED_YUVP(CSP) \
  for( int w=0, iuv=0; w<wd; ++iuv ) { \
    STORE_UP(cb_in[iuv]); STORE_YP(Y_##CSP(y_in[w++])); \
    STORE_VP(cr_in[iuv]); STORE_YP(Y_##CSP(y_in[w++])); \
  }

#define STORE_YP(v) *y_out++ = (v)
#define STORE_UP(v) *u_out++ = (v)
#define STORE_VP(v) *v_out++ = (v)

/* transfer macros */
/*  CSP = input colorspace */
/*  FMT = input format */
/*  OUT = output colorspace/format */

#define SCALED_RGB(CSP,FMT,OUT) case cmdl_##CSP##_##OUT: { \
  RGB_HEAD(FMT) \
  RGB_SCALED(OUT,CSP) \
  RGB_TAIL \
  break; }

#define UNSCALED_RGB(CSP,FMT,OUT) case cmdl_##CSP##_##OUT: { \
  RGB_HEAD(FMT) \
  RGB_UNSCALED(OUT,CSP) \
  RGB_TAIL \
  break; }

#define SCALED_YUV(CSP,FMT,OUT) case cmdl_##CSP##_##OUT: { \
  OUT##_HEAD(FMT) \
  YUV_SCALED_##OUT(CSP) \
  YUV_TAIL \
  break; }

#define UNSCALED_YUV(CSP,FMT,OUT) case cmdl_##CSP##_##OUT: { \
  OUT##_HEAD(FMT) \
  YUV_UNSCALED_##OUT(CSP) \
  YUV_TAIL \
  break; }


/* naming equivalances */

#define YUV_UNSCALED_YUV420P YUV_UNSCALED_YUVP
#define YUV_UNSCALED_YUV422P YUV_UNSCALED_YUVP
#define YUV_SCALED_YUV420P YUV_SCALED_YUVP
#define YUV_SCALED_YUV422P YUV_SCALED_YUVP

#define YUYV_HEAD(fmt) YUV_HEAD_##fmt
#define UYVY_HEAD(fmt) YUV_HEAD_##fmt
#define YUV888_HEAD(fmt) YUV_HEAD_##fmt
#define YUVA8888_HEAD(fmt) YUV_HEAD_##fmt
#define YUV420P_HEAD(fmt) YUV420P_HEAD_##fmt
#define YUV422P_HEAD(fmt) YUV422P_HEAD_##fmt

#define cmdl_YUV_BGR888   cmdl_BGR888
#define cmdl_YUV_BGRA8888 cmdl_BGRA8888
#define cmdl_YUV_RGB565   cmdl_RGB565
#define cmdl_YUV_RGB888   cmdl_RGB888
#define cmdl_YUV_RGBA8888 cmdl_RGBA8888

#define cmdl_YUV_YUV888   cmdl_YUV888
#define cmdl_YUV_YUVA8888 cmdl_YUVA8888
#define cmdl_YUV_YUV420P  cmdl_YUV420P
#define cmdl_YUV_YUV422P  cmdl_YUV422P
#define cmdl_YUV_YUYV     cmdl_YUYV
#define cmdl_YUV_UYVY     cmdl_UYVY

int zvideo_t::
dither_frame(uint8_t *yy, uint8_t *uu, uint8_t *vv, uint8_t **output_rows)
{
  int *cr2r = cr_to_r;
  int *crb2g = crb_to_g;
  int *cb2b = cb_to_b;
  int wd = out_w, ht = out_h;
  int offset0 = ht, in_x1 = in_x >> 1;
  int offset1 = offset0 + ht;
  int offset2 = offset0 + ht/2;
  if( chroma_format == cfmt_420 ) {
    if( out_w != horizontal_size ) {
      switch( color_model ) {
      SCALED_RGB(YUV,420,BGR888);
      SCALED_RGB(YUV,420,BGRA8888);
      SCALED_RGB(YUV,420,RGB565);
      SCALED_RGB(YUV,420,RGB888);
      SCALED_RGB(YUV,420,RGBA8888);
      SCALED_RGB(601,420,BGR888);
      SCALED_RGB(601,420,BGRA8888);
      SCALED_RGB(601,420,RGB565);
      SCALED_RGB(601,420,RGB888);
      SCALED_RGB(601,420,RGBA8888);

      SCALED_YUV(YUV,420,YUV420P);
      SCALED_YUV(YUV,420,YUV422P);
      SCALED_YUV(YUV,420,YUYV);
      SCALED_YUV(YUV,420,UYVY);
      SCALED_YUV(YUV,420,YUV888);
      SCALED_YUV(YUV,420,YUVA8888);
      SCALED_YUV(601,420,YUV420P);
      SCALED_YUV(601,420,YUV422P);
      SCALED_YUV(601,420,YUYV);
      SCALED_YUV(601,420,UYVY);
      SCALED_YUV(601,420,YUV888);
      SCALED_YUV(601,420,YUVA8888);

      case cmdl_RGBA16161616: {
        RGB_HEAD(420) \
        uint16_t *rgb_s = (uint16_t*)rgb;
        RGB_SCALED(RGBA16161616,YUV)
        RGB_TAIL
        break;
        }
      }
    }
    else {
      switch( color_model ) {
      UNSCALED_RGB(YUV,420,BGR888);
      UNSCALED_RGB(YUV,420,BGRA8888);
      UNSCALED_RGB(YUV,420,RGB565);
      UNSCALED_RGB(YUV,420,RGB888);
      UNSCALED_RGB(YUV,420,RGBA8888);
      UNSCALED_RGB(601,420,BGR888);
      UNSCALED_RGB(601,420,BGRA8888);
      UNSCALED_RGB(601,420,RGB565);
      UNSCALED_RGB(601,420,RGB888);
      UNSCALED_RGB(601,420,RGBA8888);

      UNSCALED_YUV(YUV,420,YUV420P);
      UNSCALED_YUV(YUV,420,YUV422P);
      UNSCALED_YUV(YUV,420,YUYV);
      UNSCALED_YUV(YUV,420,UYVY);
      UNSCALED_YUV(YUV,420,YUV888);
      UNSCALED_YUV(YUV,420,YUVA8888);
      UNSCALED_YUV(601,420,YUV420P);
      UNSCALED_YUV(601,420,YUV422P);
      UNSCALED_YUV(601,420,YUYV);
      UNSCALED_YUV(601,420,UYVY);
      UNSCALED_YUV(601,420,YUV888);
      UNSCALED_YUV(601,420,YUVA8888);

      case cmdl_RGBA16161616: {
        RGB_HEAD(420) \
        uint16_t *rgb_s = (uint16_t*)rgb;
        RGB_UNSCALED(RGBA16161616,YUV)
        RGB_TAIL
        break;
        }
      }
    }
  }
  else {
    if( out_w != horizontal_size ) {
      switch( color_model ) {
      SCALED_RGB(YUV,422,BGR888);
      SCALED_RGB(YUV,422,BGRA8888);
      SCALED_RGB(YUV,422,RGB565);
      SCALED_RGB(YUV,422,RGB888);
      SCALED_RGB(YUV,422,RGBA8888);
      SCALED_RGB(601,422,BGR888);
      SCALED_RGB(601,422,BGRA8888);
      SCALED_RGB(601,422,RGB565);
      SCALED_RGB(601,422,RGB888);
      SCALED_RGB(601,422,RGBA8888);

      SCALED_YUV(YUV,422,YUV420P);
      SCALED_YUV(YUV,422,YUV422P);
      SCALED_YUV(YUV,422,YUYV);
      SCALED_YUV(YUV,422,UYVY);
      SCALED_YUV(YUV,422,YUV888);
      SCALED_YUV(YUV,422,YUVA8888);
      SCALED_YUV(601,422,YUV420P);
      SCALED_YUV(601,422,YUV422P);
      SCALED_YUV(601,422,YUYV);
      SCALED_YUV(601,422,UYVY);
      SCALED_YUV(601,422,YUV888);
      SCALED_YUV(601,422,YUVA8888);

      case cmdl_RGBA16161616: {
        RGB_HEAD(422) \
        uint16_t *rgb_s = (uint16_t*)rgb;
        RGB_SCALED(RGBA16161616,YUV)
        RGB_TAIL
        break;
        }
      }
    }
    else {
      switch( color_model ) {
      UNSCALED_RGB(YUV,422,BGR888);
      UNSCALED_RGB(YUV,422,BGRA8888);
      UNSCALED_RGB(YUV,422,RGB565);
      UNSCALED_RGB(YUV,422,RGB888);
      UNSCALED_RGB(YUV,422,RGBA8888);
      UNSCALED_RGB(601,422,BGR888);
      UNSCALED_RGB(601,422,BGRA8888);
      UNSCALED_RGB(601,422,RGB565);
      UNSCALED_RGB(601,422,RGB888);
      UNSCALED_RGB(601,422,RGBA8888);

      UNSCALED_YUV(YUV,422,YUV420P);
      UNSCALED_YUV(YUV,422,YUV422P);
      UNSCALED_YUV(YUV,422,YUYV);
      UNSCALED_YUV(YUV,422,UYVY);
      UNSCALED_YUV(YUV,422,YUV888);
      UNSCALED_YUV(YUV,422,YUVA8888);
      UNSCALED_YUV(601,422,YUV420P);
      UNSCALED_YUV(601,422,YUV422P);
      UNSCALED_YUV(601,422,YUYV);
      UNSCALED_YUV(601,422,UYVY);
      UNSCALED_YUV(601,422,YUV888);
      UNSCALED_YUV(601,422,YUVA8888);

      case cmdl_RGBA16161616: {
        RGB_HEAD(422) \
        uint16_t *rgb_s = (uint16_t*)rgb;
        RGB_UNSCALED(RGBA16161616,YUV)
        RGB_TAIL
        break;
        }
      }
    }
  }
  return 0;
}

int zvideo_t::
dither_frame444(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  return 0;
}

int zvideo_t::
dithertop(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  return dither_frame(yy, uu, vv, output_rows);
}

int zvideo_t::
dithertop444(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  return 0;
}

int zvideo_t::
ditherbot(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  return 0;
}

int zvideo_t::
ditherbot444(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  return 0;
}

int zvideo_t::
init_output()
{
  int i, value;
  for( i=0; i<256; ++i ) {
    value = (int)(1.1644 * i - 255 * 0.0627 + 0.5);
    z601[i] = clip(value) << 16;
  }
  return 0;
}

int zvideo_t::
present_frame(uint8_t *yy, uint8_t *uu, uint8_t *vv)
{
  int i;
  ++frame_time;
  ++seek_time;

  /* Copy YUV buffers */
  if( want_yvu ) {
    long size0, size1;
    long offset0, offset1;
    int chroma_denominator;
    /* Drop a frame */
    if( !y_output ) return 0;

    chroma_denominator = chroma_format == cfmt_420 ? 2 : 1;

    /* Copy a frame */
    /* Three blocks */
    if( in_x == 0 && in_w >= coded_picture_width &&
        row_span == coded_picture_width ) {
      size0 = coded_picture_width * in_h;
      size1 = chrom_width * (int)((float)in_h / chroma_denominator + 0.5);
      offset0 = coded_picture_width * in_y;
      offset1 = chrom_width * (int)((float)in_y / chroma_denominator + 0.5);

//zmsg("1\n");
/*
 *       if(in_y > 0)
 *       {
 *         offset[1] += chrom_width / 2;
 *         size[1] += chrom_width / 2;
 *       }
 */

      memcpy(y_output, yy + offset0, size0);
      memcpy(u_output, uu + offset1, size1);
      memcpy(v_output, vv + offset1, size1);
    }
    else {
      /* One block per row */
//zmsgs("2 %d %d %d\n", in_w, coded_picture_width, chrom_width);
      int row_span = in_w;
      int row_span0, sofs;
      int row_span1, dofs;

      if( row_span )
        row_span = row_span;

      row_span0 = row_span;
      row_span1 = (row_span >> 1);
      size0 = in_w;
      size1 = (in_w >> 1);
      offset0 = coded_picture_width * in_y;
      offset1 = chrom_width * in_y / chroma_denominator;
  
      for( i=0; i<in_h; ++i ) {
        memcpy(y_output + i*row_span0, yy + offset0 + in_x, size0);
        offset0 += coded_picture_width;
        if( chroma_denominator == 1 || !(i % 2) ) {
          sofs = offset1 + (in_x>>1);
          dofs = i/chroma_denominator * row_span1; 
          memcpy(u_output + dofs, uu + sofs, size1);
          memcpy(v_output + dofs, vv + sofs, size1);
          if( horizontal_size < in_w ) {
            dofs = i/chroma_denominator * row_span1 + (horizontal_size>>1);
            sofs = (in_w>>1) - (horizontal_size>>1);
            memset(u_output + dofs, 0x80, sofs);
            memset(v_output + dofs, 0x80, sofs);
          }
        }
        

        if( chroma_denominator == 1 || (i%2) )
          offset1 += chrom_width;
      }
    }

    return 0;
  }

/* Want RGB buffer */
/* Copy the frame to the output with YUV to RGB conversion */
  if( prog_seq ) {
    if( chroma_format != cfmt_444 )
      dither_frame(yy, uu, vv, output_rows);
    else
      dither_frame444(yy, uu, vv);
  }
  else {
    if( (pict_struct == pics_FRAME_PICTURE && topfirst) || 
         pict_struct == pics_BOTTOM_FIELD) {
/* top field first */
      if( chroma_format != cfmt_444 ) {
        dithertop(yy, uu, vv);
        ditherbot(yy, uu, vv);
      }
      else {
        dithertop444(yy, uu, vv);
        ditherbot444(yy, uu, vv);
      }
    }
    else {
/* bottom field first */
      if( chroma_format != cfmt_444 ) {
        ditherbot(yy, uu, vv);
        dithertop(yy, uu, vv);
      }
      else {
        ditherbot444(yy, uu, vv);
        dithertop444(yy, uu, vv);
      }
    }
  }
  return 0;
}

