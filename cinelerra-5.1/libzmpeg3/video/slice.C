#include "../libzmpeg3.h"

#ifdef __x86_64__
#define USE_MMX
#endif
#ifdef USE_MMX
#include "mmx.h"
#if defined(__x86_64__)
#define m_(v) (*(mmx_t*)(v))
#else
#define m_(v) (*(char*)(v))
#endif
#endif



zslice_buffer_t::
slice_buffer_t()
{
  data = new uint8_t[buffer_allocation=4096];
  buffer_size = 0;
  buffer_position = 0;
  bits_size = 0;
  bits = 0;
}

zslice_buffer_t::
~slice_buffer_t()
{
  delete [] data;
}

uint8_t *zslice_buffer_t::
expand_buffer(int bfrsz)
{
  long new_allocation = 2*buffer_allocation;
  uint8_t *new_buffer = new uint8_t[new_allocation];
  if( bfrsz > 0 ) memcpy(new_buffer,data,bfrsz);
  delete [] data;
  data = new_buffer;
  buffer_allocation = new_allocation;
  buffer_size = buffer_allocation-bfrsz-4;
  return data + bfrsz;
}

    
void zslice_buffer_t::
fill_buffer(zbits_t *vstream)
{
  uint8_t *sbp = data;
  vstream->next_byte_align();
  demuxer_t *demux = vstream->demuxer;
  /* sync stream to zcode and access demuxer */
  uint32_t zcode = vstream->show_bits32_noptr();
  for( int i=32; i>0; ) *sbp++ = (zcode>>(i-=8));
  vstream->reset();
  buffer_size = buffer_allocation-(4+4); /* sizeof(zcode)+padding */

  /* Read the slice into the buffer including the slice start code */
  while( !vstream->eof() ) {
    if( buffer_size <= 0 ) sbp = expand_buffer(sbp - data);
    if( (zcode&0xff) != 0 ) {
      buffer_size -= 3;
      zcode = (zcode<<8) | (*sbp++ = demux->read_char());
      zcode = (zcode<<8) | (*sbp++ = demux->read_char());
    }
    else
      --buffer_size;
    zcode = (zcode<<8) | (*sbp++ = demux->read_char());
    if( (zcode&0xffffff) == PACKET_START_CODE_PREFIX ) break;
  }

  /* finish trailing code as picture start code (0x100) */
  *sbp++ = 0;
  buffer_size = sbp - data;
  /* reload stream bfr, position to start code */
  vstream->reset(zcode);
  vstream->get_byte_noptr();
}

/* limit coefficients to -2048..2047 */

/* move/add 8x8-Block from block[comp] to refframe */

int zslice_decoder_t::
add_block(int comp, int bx, int by, int dct_type, int addflag)
{
  int cc, i, iincr;
  uint8_t *rfp;
  short *bp;
  /* color component index */
  cc = (comp < 4) ? 0 : (comp & 1) + 1; 

  if( cc == 0 ) {   
    /* luminance */
    if( video->pict_struct == video_t::pics_FRAME_PICTURE ) {
      if( dct_type ) {
        /* field DCT coding */
        rfp = video->newframe[0] + video->coded_picture_width *
                (by + ((comp & 2) >> 1)) + bx + ((comp & 1) << 3);
        iincr = (video->coded_picture_width << 1);
      }
      else {
        /* frame DCT coding */
        rfp = video->newframe[0] + video->coded_picture_width *
                (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
        iincr = video->coded_picture_width;
      }
    }
    else {
      /* field picture */
      rfp = video->newframe[0] + (video->coded_picture_width << 1) *
              (by + ((comp & 2) << 2)) + bx + ((comp & 1) << 3);
      iincr = (video->coded_picture_width << 1);
    }
  }
  else {
    /* chrominance */

    /* scale coordinates */
    if( video->chroma_format != video_t::cfmt_444 ) bx >>= 1;
    if( video->chroma_format == video_t::cfmt_420 ) by >>= 1;
    if( video->pict_struct == video_t::pics_FRAME_PICTURE ) {
      if( dct_type && (video->chroma_format != video_t::cfmt_420) ) {
        /* field DCT coding */
        rfp = video->newframe[cc] + video->chrom_width *
                (by + ((comp & 2) >> 1)) + bx + (comp & 8);
        iincr = (video->chrom_width << 1);
      }
      else {
        /* frame DCT coding */
        rfp = video->newframe[cc] + video->chrom_width *
                (by + ((comp & 2) << 2)) + bx + (comp & 8);
        iincr = video->chrom_width;
      }
    }
    else {
      /* field picture */
      rfp = video->newframe[cc] + (video->chrom_width << 1) *
              (by + ((comp & 2) << 2)) + bx + (comp & 8);
      iincr = (video->chrom_width << 1);
    }
  }

  bp = block[comp];

  if( addflag ) {
    for( i=0; i<8; ++i ) {
#ifndef USE_MMX
      rfp[0] = clip(bp[0] + rfp[0]);
      rfp[1] = clip(bp[1] + rfp[1]);
      rfp[2] = clip(bp[2] + rfp[2]);
      rfp[3] = clip(bp[3] + rfp[3]);
      rfp[4] = clip(bp[4] + rfp[4]);
      rfp[5] = clip(bp[5] + rfp[5]);
      rfp[6] = clip(bp[6] + rfp[6]);
      rfp[7] = clip(bp[7] + rfp[7]);
#else
      movq_m2r(m_(rfp),mm1);      /* rfp[0..7] */
      pxor_r2r(mm3,mm3);          /* zero */
      pxor_r2r(mm4,mm4);          /* zero */
      movq_m2r(m_(bp+0),mm5);     /* bp[0..3] */
      movq_r2r(mm1,mm2);          /* copy rfp[0..7] */
      movq_m2r(m_(bp+4),mm6);     /* bp[4..7] */
      punpcklbw_r2r(mm3,mm1);     /* rfp[0..3] */
      punpckhbw_r2r(mm3,mm2);     /* rfp[4..7] */
      paddsw_r2r(mm5,mm1);        /* + bp[0..3] */
      paddsw_r2r(mm6,mm2);        /* + bp[4..7] */
      pcmpgtw_r2r(mm1,mm3);       /* 1s to fields < 0 */
      pcmpgtw_r2r(mm2,mm4);       /* 1s to fields < 0 */
      pandn_r2r(mm1,mm3);         /* clip <0 = zero */
      pandn_r2r(mm2,mm4);         /* clip <0 = zero */
      packuswb_r2r(mm4,mm3);      /* pack/clip >255 = 255 */
      movq_r2m(mm3,m_(rfp));      /* store rfp[0..7] */
#endif
      rfp += iincr;
      bp += 8;
    }
  }
  else {
    for( i=0; i<8; ++i ) {
#ifndef USE_MMX
      rfp[0] = clip(bp[0] + 128);
      rfp[1] = clip(bp[1] + 128);
      rfp[2] = clip(bp[2] + 128);
      rfp[3] = clip(bp[3] + 128);
      rfp[4] = clip(bp[4] + 128);
      rfp[5] = clip(bp[5] + 128);
      rfp[6] = clip(bp[6] + 128);
      rfp[7] = clip(bp[7] + 128);
#else
      static short i128[4] = { 128, 128, 128, 128 };
      movq_m2r(m_(i128),mm1);     /* 128,,128 */
      pxor_r2r(mm3,mm3);          /* zero */
      pxor_r2r(mm4,mm4);          /* zero */
      movq_m2r(m_(bp+0),mm5);     /* bp[0..3] */
      movq_r2r(mm1,mm2);          /* 128,,128 */
      movq_m2r(m_(bp+4),mm6);     /* bp[4..7] */
      paddsw_r2r(mm5,mm1);        /* + bp[0..3] */
      paddsw_r2r(mm6,mm2);        /* + bp[4..7] */
      pcmpgtw_r2r(mm1,mm3);       /* 1s to fields < 0 */
      pcmpgtw_r2r(mm2,mm4);       /* 1s to fields < 0 */
      pandn_r2r(mm1,mm3);         /* clip <0 = zero */
      pandn_r2r(mm2,mm4);         /* clip <0 = zero */
      packuswb_r2r(mm4,mm3);      /* pack/clip >255 = 255 */
      movq_r2m(mm3,m_(rfp));      /* store rfp[0..7] */
#endif
      rfp+= iincr;
      bp += 8;
    }
  }
#ifdef USE_MMX
  emms();
#endif
  return 0;
}


uint8_t zslice_decoder_t::
non_linear_mquant_table[32] = {
   0,  1,  2,  3,  4,  5,  6,  7,
   8, 10, 12, 14, 16, 18, 20, 22,
  24, 28, 32, 36, 40, 44, 48, 52,
  56, 64, 72, 80, 88, 96,104,112,
};

int zslice_decoder_t::
decode_slice()
{
  int comp;
  int mb_type, cbp, motion_type = 0, dct_type;
  int macroblock_address, mba_inc, mba_max;
  int slice_vert_pos_ext;
  uint32_t zcode;
  int dc_dct_pred[3];
  int mv_count, mv_format, mvscale;
  int pmv[2][2][2], mv_field_sel[2][2];
  int dmv, dmvector[2];
  int qs;
  int stwtype, stwclass; 
  int snr_cbp;
  int i;
  /* number of macroblocks per picture */
  mba_max = video->mb_width * video->mb_height;

  /* field picture has half as many macroblocks as frame */
  if( video->pict_struct != video_t::pics_FRAME_PICTURE )
    mba_max >>= 1; 
  macroblock_address = 0; 
  /* first macroblock in slice is not skipped */
  mba_inc = 0;
  fault = 0;
  zcode = slice_buffer->get_bits(32);
  /* decode slice header (may change quant_scale) */
  slice_vert_pos_ext = get_slice_hdr();
  /* reset all DC coefficient and motion vector predictors */
  dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
  pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
  pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;

  for( i=0; !slice_buffer->eob(); ++i ) {
    if( mba_inc == 0 ) {
      if( !slice_buffer->show_bits(23) ) break; /* Done */
      /* decode macroblock address increment */
      mba_inc = get_macroblock_address();
      if( fault ) return 1;
      if( i == 0 ) {
        /* Get the macroblock_address */
        int mline = (slice_vert_pos_ext << 7) + (zcode&0xff) - 1;
        macroblock_address = mline * video->mb_width + mba_inc - 1;
        /* first macroblock in slice: not skipped */
        mba_inc = 1;
      }
    }

    if( fault ) return 1;
    if( macroblock_address >= mba_max ) {
      /* mba_inc points beyond picture dimensions */
//zerr("too many macroblocks in picture\n"); */
        return 1;
    }

    /* not skipped */
    if( mba_inc == 1 ) {
      macroblock_modes(&mb_type, &stwtype, &stwclass, &motion_type,
        &mv_count, &mv_format, &dmv, &mvscale, &dct_type);
      if( fault ) return 1;

      if( mb_type & mb_QUANT ) {
        qs = slice_buffer->get_bits(5);
        if( video->mpeg2 )
          quant_scale = video->qscale_type ?
            non_linear_mquant_table[qs] : (qs << 1);
        else 
          quant_scale = qs;

        /* make sure quant_scale is valid */
        if( video->scalable_mode == sc_DP )
          quant_scale = quant_scale; /*???*/
      }

      /* motion vectors */

      /* decode forward motion vectors */
      if( (mb_type & mb_FORWARD) ||
          ((mb_type & mb_INTRA) && video->conceal_mv) ) {
        if( video->mpeg2 )
          motion_vectors(pmv, dmvector, mv_field_sel, 0, mv_count, mv_format,
            video->h_forw_r_size, video->v_forw_r_size, dmv, mvscale);
        else
          motion_vector(pmv[0][0], dmvector,
            video->forw_r_size, video->forw_r_size, 0, 0, video->full_forw);
      }
      if( fault ) return 1;

      /* decode backward motion vectors */
      if( mb_type & mb_BACKWARD ) {
        if( video->mpeg2 )
          motion_vectors(pmv, dmvector, mv_field_sel, 1, mv_count, mv_format, 
          video->h_back_r_size, video->v_back_r_size, 0, mvscale);
        else
          motion_vector(pmv[0][1], dmvector, 
            video->back_r_size, video->back_r_size, 0, 0, video->full_back);
      }

      if( fault ) return 1;

      /* remove marker_bit */
      if( (mb_type & mb_INTRA) && video->conceal_mv )
        slice_buffer->flush_bit();

      /* macroblock_pattern */
      if( mb_type & mb_PATTERN ) {
        cbp = get_cbp();
        if( video->chroma_format == video_t::cfmt_422 ) {
          /* coded_block_pattern_1 */
          cbp = (cbp << 2) | slice_buffer->get_bits(2); 
        }
        else if( video->chroma_format == video_t::cfmt_444 ) {
          /* coded_block_pattern_2 */
          cbp = (cbp << 6) | slice_buffer->get_bits(6); 
        }
      }
      else
        cbp = (mb_type & mb_INTRA) ? ((1 << video->blk_cnt) - 1) : 0;
      if( fault ) return 1;

      /* decode blocks */
      clear_block(0, video->blk_cnt);
      for( comp=0; comp<video->blk_cnt; ++comp ) {
        if( cbp & (1 << (video->blk_cnt-comp-1)) ) {
          if( mb_type & mb_INTRA ) {
            if( video->mpeg2 )
              get_mpg2_intra_block(comp, dc_dct_pred);
            else
              get_intra_block(comp, dc_dct_pred);
          }
          else {
            if( video->mpeg2 )
              get_mpg2_inter_block(comp);
            else           
              get_inter_block(comp);
          }
          if( fault ) return 1;
        }
      }

      /* reset intra_dc predictors */
      if( !(mb_type & mb_INTRA) )
        dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

      /* reset motion vector predictors */
      if( (mb_type & mb_INTRA) && !video->conceal_mv ) {
        /* intra mb without concealment motion vectors */
        pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
        pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;
      }

      if( (video->pict_type == video_t::pic_type_P) &&
          !(mb_type & (mb_FORWARD | mb_INTRA)) ) {
        /* non-intra mb without forward mv in a P picture */
        pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;

        /* derive motion_type */
        if( video->pict_struct == video_t::pics_FRAME_PICTURE )
          motion_type = mc_FRAME;
        else {
          motion_type = mc_FIELD;
          /* predict from field of same parity */
          mv_field_sel[0][0] = video->pict_struct == video_t::pics_BOTTOM_FIELD ? 1 : 0;
        }
      }

      if( stwclass == 4 ) {
        /* purely spatially predicted macroblock */
        pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;
        pmv[0][1][0] = pmv[0][1][1] = pmv[1][1][0] = pmv[1][1][1] = 0;
      }
    }
    else {
      /* mba_inc!=1: skipped macroblock */
      clear_block(0, video->blk_cnt);

      /* reset intra_dc predictors */
      dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

      /* reset motion vector predictors */
      if( video->pict_type == video_t::pic_type_P )
        pmv[0][0][0] = pmv[0][0][1] = pmv[1][0][0] = pmv[1][0][1] = 0;

      /* derive motion_type */
      if( video->pict_struct == video_t::pics_FRAME_PICTURE )
        motion_type = mc_FRAME;
      else {
        motion_type = mc_FIELD;
        /* predict from field of same parity */
        mv_field_sel[0][0] = mv_field_sel[0][1] =
          (video->pict_struct == video_t::pics_BOTTOM_FIELD) ? 1 : 0;
      }

      /* skipped I are spatial-only predicted, */
      /* skipped P and B are temporal-only predicted */
      stwtype = video->pict_type == video_t::pic_type_I ? 8 : 0;
      mb_type &= ~mb_INTRA; /* clear mb_INTRA */
      cbp = 0; /* no block data */
    }

    snr_cbp = 0;

    /* pixel coordinates of top left corner of current macroblock */
    int mx = macroblock_address % video->mb_width;
    int my = macroblock_address / video->mb_width;

    /* thumbnails */
    if( video->thumb && video->pict_type == video_t::pic_type_I ) {
      uint8_t *ap = video->tdat + 4*my*video->mb_width + 2*mx;
      uint8_t *bp = ap + 2*video->mb_width;
      if( (mb_type&mb_INTRA) != 0 ) {
        *ap = clip(128 + block[0][0]/8);  ++ap;
        *ap = clip(128 + block[1][0]/8);
        *bp = clip(128 + block[2][0]/8);  ++bp;
        *bp = clip(128 + block[3][0]/8);
      }
      else {
        *ap = clip(*ap + block[0][0]/8);  ++ap;
        *ap = clip(*ap + block[1][0]/8);
        *bp = clip(*bp + block[2][0]/8);  ++bp;
        *bp = clip(*bp + block[3][0]/8);
      }
    }
    /* skimming */
    if( !video->skim ) {
      int bx = 16*mx, by = 16*my;
      /* motion compensation */
      if( !(mb_type & mb_INTRA) )
        video->reconstruct( bx, by, mb_type, motion_type, pmv, 
          mv_field_sel, dmvector, stwtype);

      /* copy or add block data into picture */
      for( comp=0; comp<video->blk_cnt; ++comp ) {
        if( (cbp | snr_cbp) & (1 << (video->blk_cnt-1-comp)) ) {
          idct_conversion(block[comp]);
          add_block(comp, bx, by, dct_type, !(mb_type & mb_INTRA) ? 1 : 0);
        }
      }
    }

    /* advance to next macroblock */
    ++macroblock_address;
    --mba_inc;
  }

  return 0;
}

void zslice_decoder_t::
slice_loop()
{
  while( !done ) {
    input_lock.lock();
    while( !done ) {
      decode_slice();
      video->put_slice_buffer(slice_buffer);
      if( get_active_slice_buffer() ) break;
    }
  }
}

void *zslice_decoder_t::
the_slice_loop(void *the_slice_decoder)
{
  ((slice_decoder_t *)the_slice_decoder)->slice_loop();
  return 0;
}

zslice_decoder_t::
slice_decoder_t()
{
  owner = pthread_self();
  video = 0;
  slice_buffer = 0;
  done = 0;
  input_lock.lock();
  pthread_create(&tid,0,the_slice_loop, this);
}

zslice_decoder_t::
~slice_decoder_t()
{
  done = 1;
  input_lock.unlock();
  pthread_join(tid, 0);
}


int zslice_decoder_t::
get_cbp()
{
  int zcode;
  if( (zcode=slice_buffer->show_bits(9)) >= 128 ) {
    zcode >>= 4;
    slice_buffer->flush_bits(CBPtab0[zcode].len);
    return CBPtab0[zcode].val;
  }

  if( zcode >= 8 ) {
    zcode >>= 1;
    slice_buffer->flush_bits(CBPtab1[zcode].len);
    return CBPtab1[zcode].val;
  }

  if( zcode < 1 ) {
//zerr("invalid coded_block_pattern code\n");
    fault = 1;
    return 0;
  }

  slice_buffer->flush_bits(CBPtab2[zcode].len);
  return CBPtab2[zcode].val;
}


/* set block to zero */
int zslice_decoder_t::
clear_block(int comp, int size)
{
  sparse[comp] = 1;
  memset(block[comp], 0, size*sizeof(block[0]));
  return 0;
}

int zslice_buffer_t::
get_dc_lum()
{
  int zcode, size, val;
  zcode = show_bits(5); /* decode length */
  if( zcode < 31 ) {
    size = DClumtab0[zcode].val;
    flush_bits(DClumtab0[zcode].len);
  }
  else {
    zcode = show_bits(9) - 0x1f0;
    size = DClumtab1[zcode].val;
    flush_bits(DClumtab1[zcode].len);
  }

  if( size ) {
    val = get_bits(size);
    if( (val & (1 << (size-1))) == 0 )
      val -= (1 << size)-1;
  }
  else
    val = 0;

  return val;
}


int zslice_buffer_t::
get_dc_chrom()
{
  int zcode, size, val;
  zcode = show_bits(5); /* decode length */
  if( zcode < 31 ) {
    size = DCchromtab0[zcode].val;
    flush_bits(DCchromtab0[zcode].len);
  }
  else {
    zcode = show_bits(10) - 0x3e0;
    size = DCchromtab1[zcode].val;
    flush_bits(DCchromtab1[zcode].len);
  }

  if( size ) {
    val = get_bits(size);
    if( (val & (1 << (size-1))) == 0 )
      val -= (1 << size)-1;
  }
  else 
    val = 0;

  return val;
}

uint16_t *zslice_decoder_t::DCTlutab[3] = { 0, };

void zslice_decoder_t::
init_lut(uint16_t *&lutbl, DCTtab_t *tabn, DCTtab_t *tab0, DCTtab_t *tab1)
{
  int i = 0;
  uint16_t *lut = new uint16_t[0x10000];
  while( i < 0x0010 ) { lut[i] = 0;  ++i; }
  while( i < 0x0020 ) { lut[i] = lu_pack(&DCTtab6[i - 16]);         ++i; }
  while( i < 0x0040 ) { lut[i] = lu_pack(&DCTtab5[(i >> 1) - 16]);  ++i; }
  while( i < 0x0080 ) { lut[i] = lu_pack(&DCTtab4[(i >> 2) - 16]);  ++i; }
  while( i < 0x0100 ) { lut[i] = lu_pack(&DCTtab3[(i >> 3) - 16]);  ++i; }
  while( i < 0x0200 ) { lut[i] = lu_pack(&DCTtab2[(i >> 4) - 16]);  ++i; }
  while( i < 0x0400 ) { lut[i] = lu_pack(   &tab1[(i >> 6) - 8]);   ++i; }
  int tblsz = !tabn ? 0x10000 : 0x4000;
  while( i < tblsz  ) { lut[i] = lu_pack(   &tab0[(i >> 8) - 4]);   ++i; }
  while( i <0x10000 ) { lut[i] = lu_pack(   &tabn[(i >>12) - 4]);   ++i; }
  lutbl = lut;
}

void zslice_decoder_t::
init_tables()
{
  if( DCTlutab[0] ) return;
  init_lut(DCTlutab[0], DCTtabfirst, DCTtab0,  DCTtab1);
  init_lut(DCTlutab[1], DCTtabnext , DCTtab0,  DCTtab1);
  init_lut(DCTlutab[2],          0 , DCTtab0a, DCTtab1a);
}

/* decode one MPEG-1 coef */

inline int zslice_decoder_t::
get_coef(uint16_t *lut)
{
  int v, s, i = idx;
  uint16_t zcode = slice_buffer->show_bits(16);
  uint16_t lu = lut[zcode];
  if( !lu ) return fault = 1;
  int len = lu_len(lu);
  slice_buffer->flush_bits(len);
  int run = lu_run(lu);
  if( run >= 32 ) {       /* escape */
    if( run == 32 ) return -1;  /* end of block */
    i += slice_buffer->get_bits(6);
    if( (v = slice_buffer->get_bits(8)) < 128 ) {
      s = 0;
      if( !v ) v = slice_buffer->get_bits(8);
    }
    else {
      if( v == 128 ) v = slice_buffer->get_bits(8);
      s = 1;  v = 256 - v;
    }
  }
  else {
    i += run;
    v = lu_level(lu);
    s = slice_buffer->get_bit();
  }
  if( i >= 64 ) return fault = 1;
  val = v;  sign = s;  idx = i;
  return 0;
}

/* decode one MPEG-2 coef */

inline int zslice_decoder_t::
get_mpg2_coef(uint16_t *lut)
{
  int v, s, i = idx;
  uint16_t zcode = slice_buffer->show_bits(16);
  uint16_t lu = lut[zcode];
  if( !lu ) return fault = 1;
  int len = lu_len(lu);
  slice_buffer->flush_bits(len);
  int run = lu_run(lu);
  if( run >= 32 ) {       /* escape */
    if( run == 32 ) return -1;  /* end of block */
    i += slice_buffer->get_bits(6);
    v = slice_buffer->get_bits(12);
    if( (v & 2047) == 0 ) return fault = 1;
    s = (v >= 2048) ? 1 : 0;
    if( s != 0 ) v = 4096 - v;
  }
  else {
    i += run;
    v = lu_level(lu);
    s = slice_buffer->get_bit();
  }
  if( i >= 64 ) return fault = 1;
  val = v;  sign = s;  idx = i;
  return 0;
}


/* decode one intra coded MPEG-1 block */

void zslice_decoder_t::
get_intra_block(int comp, int dc_dct_pred[])
{
  int dc_coef = /* decode DC coefficients */
    comp <  4 ? (dc_dct_pred[0] += slice_buffer->get_dc_lum())   :
    comp == 4 ? (dc_dct_pred[1] += slice_buffer->get_dc_chrom()) :
                (dc_dct_pred[2] += slice_buffer->get_dc_chrom()) ;
  short *bp = block[comp];
  bp[0] = dc_coef << 3;

  /* decode AC coefficients */
  int *qmat = video->intra_quantizer_matrix;
  int j = 0;
  idx = 1;
  while( !get_coef(DCTlutab[1]) ) {
    j = video->zigzag_scan_table[idx++];
    int v = (((val * quant_scale*qmat[j])>>3)-1) | 1;
    bp[j] = sign ? -v : v;
  }

  if( j > 0 ) /* not a sparse matrix ! */
    sparse[comp] = 0;
}

/* decode one intra coded MPEG-2 block */
void zslice_decoder_t::
get_mpg2_intra_block(int comp, int dc_dct_pred[])
{
  int *qmat = (comp < 4 || video->chroma_format == video_t::cfmt_420) ?
    video->intra_quantizer_matrix : video->chroma_intra_quantizer_matrix;
  int dc_coef = /* decode DC coefficients */
    comp < 4  ? (dc_dct_pred[0] += slice_buffer->get_dc_lum())   :
    !(comp&1) ? (dc_dct_pred[1] += slice_buffer->get_dc_chrom()) :
                (dc_dct_pred[2] += slice_buffer->get_dc_chrom()) ;
/* with data partitioning, data always goes to base layer */
  short *bp = block[comp]; 
  bp[0] = dc_coef << (3 - video->dc_prec);

  uint8_t *scan_table = video->altscan ?
    video->alternate_scan_table : video->zigzag_scan_table;
  uint16_t *lutbl = !video->intravlc ? DCTlutab[1] : DCTlutab[2];

  int j = 0;
  idx = 1;
  while( !get_mpg2_coef(lutbl) ) { /* decode AC coefficients */
    j = scan_table[idx++];
    int v = (val * quant_scale*qmat[j]) >> 4;
    bp[j] = sign ? -v : v;
  }

  if( j > 0 ) /* not a sparse matrix ! */
    sparse[comp] = 0;
}


/* decode one non-intra coded MPEG-1 block */

void zslice_decoder_t::
get_inter_block(int comp)
{
  short *bp = block[comp];

  idx = 0;
  if( get_coef(DCTlutab[0])) return;
  int j = video->zigzag_scan_table[idx++];
  int *qmat = video->non_intra_quantizer_matrix;
  int v = (((((val<<1) + 1) * quant_scale*qmat[j])>>4)-1) | 1;
  bp[j] = sign ? -v : v;

  /* decode AC coefficients */
  while( !get_coef(DCTlutab[1]) ) {
    j = video->zigzag_scan_table[idx++];
    v = (((((val<<1) + 1) * quant_scale*qmat[j])>>4)-1) | 1;
    bp[j] = sign ? -v : v;
  }

  if( j > 0 ) /* not a sparse matrix ! */
    sparse[comp] = 0;
}


/* decode one non-intra coded MPEG-2 block */

void zslice_decoder_t::
get_mpg2_inter_block(int comp)
{
  /* with data partitioning, data always goes to base layer */
  short *bp = block[comp];
  int *qmat = (comp < 4 || video->chroma_format == video_t::cfmt_420) ?
    video->non_intra_quantizer_matrix : video->chroma_non_intra_quantizer_matrix;
  uint8_t *scan_table = video->altscan ?
    video->alternate_scan_table : video->zigzag_scan_table;

  idx = 0;
  if( get_mpg2_coef(DCTlutab[0]) ) return;
  int j = scan_table[idx++];
  int v = (((val << 1)+1) * quant_scale*qmat[j]) >> 5;
  bp[j] = sign ? -v : v ;

  /* decode AC coefficients */
  while( !get_mpg2_coef(DCTlutab[1]) ) {
    j = scan_table[idx++];
    int v = (((val << 1)+1) * quant_scale*qmat[j]) >> 5;
    bp[j] = sign ? -v : v ;
  }

  if( j > 0 ) /* not a sparse matrix ! */
    sparse[comp] = 0;
}


void zmpeg3_t::
decode_slice(zslice_buffer_t *buffer)
{
  decoder_lock.lock();
  if( avail_slice_decoders ) {
    zslice_decoder_t *decoder = avail_slice_decoders;
    avail_slice_decoders = decoder->next;
    decoder->slice_buffer = buffer;
    decoder->video = buffer->video;
    if( !decoder_active_locked++ )
      decoder_active.lock();
    decoder->input_lock.unlock();
  }
  else {
    buffer->next = active_slice_buffers;
    active_slice_buffers = buffer;
  }
  decoder_lock.unlock();
}

int zslice_decoder_t::
get_active_slice_buffer()
{
  int result;
  src->decoder_lock.lock();
  zslice_buffer_t *buffer = src->active_slice_buffers;
  if( !buffer ) {
    next = src->avail_slice_decoders;
    src->avail_slice_decoders = this;
    if( !--src->decoder_active_locked )
      src->decoder_active.unlock();
    slice_buffer = 0;
    video = 0;
    result = 1;
  }
  else {
    src->active_slice_buffers = buffer->next;
    slice_buffer = buffer;
    video = buffer->video;
    result = 0;
  }
  src->decoder_lock.unlock();
  return result;
}

void zmpeg3_t::
allocate_slice_decoders()
{
  if( slice_decoders ) return;
  int count = cpus;
  if( count > MAX_CPUS ) count = MAX_CPUS;
  slice_decoders = new slice_decoder_t[count];
  slice_decoder_t *decoder = &slice_decoders[0];
  avail_slice_decoders = 0;
  for( int i=count; --i>=0; ++decoder ) {
    decoder->src = this;
    decoder->next = avail_slice_decoders;
    avail_slice_decoders = decoder;
  }
  total_slice_decoders = count;
}

void zmpeg3_t::
delete_slice_decoders()
{
  if( !slice_decoders ) return;
  delete [] slice_decoders;
  slice_decoders = 0;
  total_slice_decoders = 0;
}

void zmpeg3_t::
reallocate_slice_decoders()
{
  decoder_active.lock();
  decoder_active.unlock();
  decoder_lock.lock();
  if( !decoder_active_locked ) {
    delete_slice_decoders();
    allocate_slice_decoders();
  }
  decoder_lock.unlock();
}

void zvideo_t::
allocate_slice_buffers()
{
  if( slice_buffers ) return;
  int count = 2*src->cpus;
  if( count > 2*MAX_CPUS ) count = 2*MAX_CPUS;
  slice_buffers = new slice_buffer_t[count];
  slice_buffer_t *buffer = &slice_buffers[0];
  avail_slice_buffers = 0;
  for( int i=count; --i>=0; ++buffer ) {
    buffer->next = avail_slice_buffers;
    avail_slice_buffers = buffer;
  }
  total_slice_buffers = count;
}

void zvideo_t::
delete_slice_buffers()
{
  if( !slice_buffers ) return;
  delete [] slice_buffers;
  slice_buffers = 0;
  total_slice_buffers = 0;
}

void zvideo_t::
reallocate_slice_buffers()
{
  slice_active.lock();
  slice_active.unlock();
  slice_lock.lock();
  if( !slice_active_locked ) {
    delete_slice_buffers();
    allocate_slice_buffers();
  }
  slice_lock.unlock();
} 

