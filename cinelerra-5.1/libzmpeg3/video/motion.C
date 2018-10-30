#include "../libzmpeg3.h"

/* calculate motion vector component */

static void
calc_mv(int *pred, int r_size, int motion_code, int motion_r, int full_pel_vector)
{
  int lim = 16 << r_size;
  int vec = full_pel_vector ? (*pred >> 1) : (*pred);

  if( motion_code > 0 ) {
    vec += ((motion_code - 1) << r_size) + motion_r + 1;
    if( vec >= lim ) vec -= lim + lim;
  }
  else if( motion_code < 0 ) {
    vec -= ((-motion_code - 1) << r_size) + motion_r + 1;
    if( vec < -lim ) vec += lim + lim;
  }
  *pred = full_pel_vector ? (vec << 1) : vec;
}


/*
int *dmvector, * differential motion vector *
int mvx, int mvy  * decoded mv components (always in field format) *
*/
void zvideo_t::
calc_dmv(int DMV[][2], int *dmvector, int mvx, int mvy)
{
  if( pict_struct == pics_FRAME_PICTURE ) {
    if( topfirst ) {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((mvx  + (mvx>0)) >> 1) + dmvector[0];
      DMV[0][1] = ((mvy  + (mvy>0)) >> 1) + dmvector[1] - 1;

      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((3 * mvx + (mvx > 0)) >> 1) + dmvector[0];
      DMV[1][1] = ((3 * mvy + (mvy > 0)) >> 1) + dmvector[1] + 1;
    }
    else {
      /* vector for prediction of top field from bottom field */
      DMV[0][0] = ((3 * mvx + (mvx>0)) >> 1) + dmvector[0];
      DMV[0][1] = ((3 * mvy + (mvy>0)) >> 1) + dmvector[1] - 1;
      /* vector for prediction of bottom field from top field */
      DMV[1][0] = ((mvx + (mvx>0)) >> 1) + dmvector[0];
      DMV[1][1] = ((mvy + (mvy>0)) >> 1) + dmvector[1] + 1;
    }
  }
  else {
    /* vector for prediction from field of opposite 'parity' */
    DMV[0][0] = ((mvx + (mvx > 0)) >> 1) + dmvector[0];
    DMV[0][1] = ((mvy + (mvy > 0)) >> 1) + dmvector[1];
    /* correct for vertical field shift */
    if(  pict_struct == pics_TOP_FIELD )
      --DMV[0][1];
    else 
      ++DMV[0][1];
  }
}

int zslice_decoder_t::
get_mv()
{
  int zcode;
  if( slice_buffer->get_bit() ) return 0;

  if( (zcode=slice_buffer->show_bits(9)) >= 64 ) {
    zcode >>= 6;
    slice_buffer->flush_bits(MVtab0[zcode].len);
    return slice_buffer->get_bit() ?
      -MVtab0[zcode].val : MVtab0[zcode].val;
  }

  if( zcode >= 24 ) {
    zcode >>= 3;
    slice_buffer->flush_bits(MVtab1[zcode].len);
    return slice_buffer->get_bit() ?
      -MVtab1[zcode].val : MVtab1[zcode].val;
  }

  if( (zcode-=12) < 0 ) {
//zerrs("invalid motion_vector code %d\n",zcode+12);
    fault = 1;
    return 1;
  }
  slice_buffer->flush_bits(MVtab2[zcode].len);
  return slice_buffer->get_bit() ?
    -MVtab2[zcode].val : MVtab2[zcode].val;
}

/* get differential motion vector (for dual prime prediction) */
int zslice_decoder_t::
get_dmv()
{
  if( slice_buffer->get_bit() )
    return slice_buffer->get_bit() ? -1 : 1;
  return 0;
}


/* get and decode motion vector and differential motion vector */
void zslice_decoder_t::
motion_vector(int *PMV, int *dmvector, int h_r_size, int v_r_size,
    int dmv, int mvscale, int full_pel_vector)
{
  int motion_r;
  int motion_code = get_mv();
  if( fault ) return;
  motion_r = (h_r_size != 0 && motion_code != 0) ?
    slice_buffer->get_bits(h_r_size) : 0;
  calc_mv(&PMV[0], h_r_size, motion_code, motion_r, full_pel_vector);
  if( dmv ) dmvector[0] = get_dmv();
  motion_code = get_mv();
  if( fault )  return;
  motion_r = (v_r_size != 0 && motion_code != 0) ?
    slice_buffer->get_bits(v_r_size) : 0;

  /* DIV 2 */
  if( mvscale ) PMV[1] >>= 1; 
  calc_mv(&PMV[1], v_r_size, motion_code, motion_r, full_pel_vector);
  if( mvscale ) PMV[1] <<= 1;
  if( dmv ) dmvector[1] = get_dmv();
}

int zslice_decoder_t::
motion_vectors( int PMV[2][2][2], int dmvector[2], int mv_field_sel[2][2],
    int s, int mv_count, int mv_format, int h_r_size, int v_r_size, 
    int dmv, int mvscale)
{
  if( mv_count == 1 ) {
    if( mv_format == mv_FIELD && !dmv )
      mv_field_sel[1][s] = mv_field_sel[0][s] = slice_buffer->get_bit();
    motion_vector(PMV[0][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
    if( fault ) return 1;

    /* update other motion vector predictors */
    PMV[1][s][0] = PMV[0][s][0];
    PMV[1][s][1] = PMV[0][s][1];
  }
  else {
    mv_field_sel[0][s] = slice_buffer->get_bit();
    motion_vector(PMV[0][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
    if( fault ) return 1;
    mv_field_sel[1][s] = slice_buffer->get_bit();
    motion_vector(PMV[1][s], dmvector, h_r_size, v_r_size, dmv, mvscale, 0);
    if( fault ) return 1;
  }
  return 0;
}

