#include "../libzmpeg3.h"

int zslice_decoder_t::
get_macroblock_address()
{
  int zcode, val = 0;
  while( (zcode = slice_buffer->show_bits(11)) < 24 ) {
    if( zcode != 15 ) {     /* Is not macroblock_stuffing */
      if( zcode != 8 ) {    /* Is not macroblock_escape */
//zerrs("invalid macroblock_address_increment code %d\n",zcode);
        fault = 1;
        return 1;
      }
      val += 33;
    }
    slice_buffer->flush_bits(11);
  }

  if( zcode >= 1024 ) {
    slice_buffer->flush_bit();
    return val + 1;
  }

  if( zcode >= 128 ) {
    zcode >>= 6;
    slice_buffer->flush_bits(MBAtab1[zcode].len);
    return val + MBAtab1[zcode].val;
  }

  zcode -= 24;
  slice_buffer->flush_bits(MBAtab2[zcode].len);

  return val + MBAtab2[zcode].val;
}

/* macroblock_type for pictures with spatial scalability */
inline int zslice_decoder_t::
getsp_imb_type()
{
  uint32_t zcode = slice_buffer->show_bits(4);
  if( !zcode ) {
//zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }

  slice_buffer->flush_bits(spIMBtab[zcode].len);
  return spIMBtab[zcode].val;
}

inline int zslice_decoder_t::
getsp_pmb_type()
{
  int zcode = slice_buffer->show_bits(7);
  if( zcode < 2 ) {
//  zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }

  if( zcode >= 16 ) {
    zcode >>= 3;
    slice_buffer->flush_bits(spPMBtab0[zcode].len);

    return spPMBtab0[zcode].val;
  }

  slice_buffer->flush_bits(spPMBtab1[zcode].len);
  return spPMBtab1[zcode].val;
}

inline int zslice_decoder_t::
getsp_bmb_type()
{
  VLCtab_t *p;
  int zcode = slice_buffer->show_bits(9);

  if( zcode >= 64 )
    p = &spBMBtab0[(zcode >> 5) - 2];
  else if( zcode >= 16 )
    p = &spBMBtab1[(zcode >> 2) - 4];
  else if( zcode >= 8 )  
    p = &spBMBtab2[zcode - 8];
  else {
//zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }

 slice_buffer->flush_bits(p->len);
 return p->val;
}

inline int zslice_decoder_t::
get_imb_type()
{
  if( slice_buffer->get_bit() ) return 1;
  if( !slice_buffer->get_bit() ) {
//zerr("invalid macroblock_type code\n");
    fault = 1;
    return 0;
  }

  return 17;
}

inline int zslice_decoder_t::
get_pmb_type()
{
  int zcode;
  if( (zcode = slice_buffer->show_bits(6)) >= 8 ) {
    zcode >>= 3;
    slice_buffer->flush_bits(PMBtab0[zcode].len);
    return PMBtab0[zcode].val;
  }

  if( zcode == 0 ) {
//zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }

  slice_buffer->flush_bits(PMBtab1[zcode].len);
  return PMBtab1[zcode].val;
}

inline int zslice_decoder_t::
get_bmb_type()
{
  int zcode;
  if( (zcode = slice_buffer->show_bits(6)) >= 8 ) {
    zcode >>= 2;
    slice_buffer->flush_bits(BMBtab0[zcode].len);
    return BMBtab0[zcode].val;
  }

  if( zcode == 0 ) {
//zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }

  slice_buffer->flush_bits(BMBtab1[zcode].len);
  return BMBtab1[zcode].val;
}

inline int zslice_decoder_t::
get_dmb_type()
{
  if( !slice_buffer->get_bit() ) {
//zerr("invalid macroblock_type code\n");
    fault=1;
    return 0;
  }

  return 1;
}


inline int zslice_decoder_t::
get_snrmb_type()
{
  int zcode = slice_buffer->show_bits(3);
  if( zcode == 0 ) {
//zerrs("invalid macroblock_type code %d\n",zcode);
    fault = 1;
    return 0;
  }
  slice_buffer->flush_bits(SNRMBtab[zcode].len);
  return SNRMBtab[zcode].val;
}

int zslice_decoder_t::
get_mb_type()
{
  if( video->scalable_mode == sc_SNR )
    return get_snrmb_type();
  switch(video->pict_type) {
  case video_t::pic_type_I:
    return video->pict_scal ? getsp_imb_type() : get_imb_type();
  case video_t::pic_type_P:
    return video->pict_scal ? getsp_pmb_type() : get_pmb_type();
  case video_t::pic_type_B:
    return video->pict_scal ? getsp_bmb_type() : get_bmb_type();
  case video_t::pic_type_D:
    return get_dmb_type();
  default: /* MPEG-1 only, not implemented */
//zerrs("unknown coding type %d\n",video->pict_type);
    break;
  }

  return 0;
}

int zslice_decoder_t::
macroblock_modes(int *pmb_type, int *pstwtype, int *pstwclass, int *pmotion_type,
    int *pmv_count, int *pmv_format, int *pdmv, int *pmvscale, int *pdct_type)
{
  int mb_type;
  int stwtype, stwcode, stwclass;
  int motion_type, mv_count, mv_format, dmv, mvscale;
  int dct_type;
  static uint8_t stwc_table[3][4] = { {6,3,7,4}, {2,1,5,4}, {2,5,7,4} };
  static uint8_t stwclass_table[9] = {0, 1, 2, 1, 1, 2, 3, 3, 4};
  motion_type = 0;

  /* get macroblock_type */
  mb_type = get_mb_type();
  if( fault ) return 1;

  /* get spatial_temporal_weight_code */
  if( mb_type & mb_WEIGHT ) {
    if( video->stwc_table_index == 0 )
      stwtype = 4;
    else {
      stwcode = slice_buffer->get_bits(2);
      stwtype = stwc_table[video->stwc_table_index - 1][stwcode];
    }
  }
  else
    stwtype = (mb_type & mb_CLASS4) ? 8 : 0;

  /* derive spatial_temporal_weight_class (Table 7-18) */
  stwclass = stwclass_table[stwtype];

  /* get frame/field motion type */
  if( mb_type & (mb_FORWARD | mb_BACKWARD) ) {
    if( video->pict_struct == video_t::pics_FRAME_PICTURE ) { 
      /* frame_motion_type */
      motion_type = video->frame_pred_dct ?
        mc_FRAME : slice_buffer->get_bits(2);
    }
    else { 
     /* field_motion_type */
     motion_type = slice_buffer->get_bits(2);
    }
  }
  else if( (mb_type & mb_INTRA) && video->conceal_mv ) {
    /* concealment motion vectors */
    motion_type = video->pict_struct == video_t::pics_FRAME_PICTURE ?
      mc_FRAME : mc_FIELD;
  }

  /* derive mv_count, mv_format and dmv, (table 6-17, 6-18) */
  if( video->pict_struct == video_t::pics_FRAME_PICTURE ) {
    mv_count = (motion_type == mc_FIELD && stwclass < 2) ? 2 : 1;
    mv_format = (motion_type == mc_FRAME) ?  mv_FRAME : mv_FIELD;
  }
  else {
    mv_count = (motion_type == mc_16X8) ? 2 : 1;
    mv_format = mv_FIELD;
  }

  dmv = motion_type == mc_DMV ? 1 : 0; /* dual prime */

  /* field mv predictions in frame pictures have to be scaled */
  mvscale = mv_format == mv_FIELD &&
    video->pict_struct == video_t::pics_FRAME_PICTURE ? 1 : 0;

  /* get dct_type (frame DCT / field DCT) */
  dct_type = video->pict_struct == video_t::pics_FRAME_PICTURE && 
    !video->frame_pred_dct && (mb_type & (mb_PATTERN | mb_INTRA)) ? 
       slice_buffer->get_bit() : 0;

  /* return values */
  *pmb_type = mb_type;
  *pstwtype = stwtype;
  *pstwclass = stwclass;
  *pmotion_type = motion_type;
  *pmv_count = mv_count;
  *pmv_format = mv_format;
  *pdmv = dmv;
  *pmvscale = mvscale;
  *pdct_type = dct_type;
  return 0;
}
