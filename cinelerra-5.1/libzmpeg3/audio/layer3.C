#include "../libzmpeg3.h"

int zaudio_decoder_layer_t::
get_scale_factors_1(int *scf, l3_info_t *l3_info, int ch, int gr)
{
  static uint8_t slen[2][16] = 
    {{0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4},
     {0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3}};
  int i, numbits;
  int num0 = slen[0][l3_info->scalefac_compress];
  int num1 = slen[1][l3_info->scalefac_compress];
  if( l3_info->block_type == 2 ) {
    i = 18;
    numbits = (num0 + num1) * 18;

    if( l3_info->mixed_block_flag ) {
      for( ; --i>9; ) *scf++ = stream->get_bits(num0);
      /* num0*17 + num1*18 */
      numbits -= num0; 
    }
    while( --i >= 0 ) *scf++ = stream->get_bits(num0);
    for( i=18; --i>=0; ) *scf++ = stream->get_bits(num1);
    /* short[13][0..2] = 0 */
    *scf++ = 0; *scf++ = 0; *scf++ = 0; 
  }
  else {
    int scfsi = l3_info->scfsi;
    if( scfsi < 0 ) { 
      /* scfsi < 0 => granule == 0 */
      for( i=11; --i>=0; ) *scf++ = stream->get_bits(num0);
      for( i=10; --i>=0; ) *scf++ = stream->get_bits(num1);
      numbits = (num0 + num1) * 10 + num0;
      *scf++ = 0;
    }
    else {
      numbits = 0;
      if( !(scfsi & 0x8) ) {
        for( i=6; --i>=0; ) *scf++ = stream->get_bits(num0);
        numbits += num0 * 6;
      }
      else
        scf += 6; 
      if( !(scfsi & 0x4) ) {
        for( i=5; --i>=0; ) *scf++ = stream->get_bits(num0);
        numbits += num0 * 5;
      }
      else 
        scf += 5;
      if(!(scfsi & 0x2)) {
        for( i=5; --i>=0; ) *scf++ = stream->get_bits(num1);
        numbits += num1 * 5;
      }
      else
        scf += 5; 
      if( !(scfsi & 0x1) ) {
        for( i=5; --i>=0; ) *scf++ = stream->get_bits(num1);
        numbits += num1 * 5;
      }
      else
        scf += 5;
      *scf++ = 0;  /* no l[21] in original sources */
    }
  }
  return numbits;
}

int zaudio_decoder_layer_t::
get_scale_factors_2(int *scf, l3_info_t *l3_info, int i_stereo)
{
  int i, j;
  static uint8_t stab[3][6][4] = 
    {{{ 6, 5, 5,5 }, { 6, 5, 7,3 }, { 11,10,0,0},
      { 7, 7, 7,0 }, { 6, 6, 6,3 }, {  8, 8,5,0}},
     {{ 9, 9, 9,9 }, { 9, 9,12,6 }, { 18,18,0,0},
      {12,12,12,0 }, {12, 9, 9,6 }, { 15,12,9,0}},
     {{ 6, 9, 9,9 }, { 6, 9,12,6 }, { 15,18,0,0},
      { 6,15,12,0 }, { 6,12, 9,6 }, {  6,18,9,0}}}; 

  /* i_stereo AND second channel -> do_layer3() checks this */
  unsigned int slen = i_stereo ?
    i_slen2[l3_info->scalefac_compress >> 1] :
    n_slen2[l3_info->scalefac_compress];

  l3_info->preflag = (slen >> 15) & 0x1;
  int n = 0;
  if( l3_info->block_type == 2 ) {
    ++n;
    if( l3_info->mixed_block_flag ) ++n ;
  }
  uint8_t *pnt = stab[n][(slen >> 12) & 0x7];
  int numbits = 0;
  for( i=0; i<4; ++i ) {
    int num = slen & 0x7;
    slen >>= 3;
    if( num ) {
      for( j=(int)pnt[i]; --j>=0; ) *scf++ = stream->get_bits(num);
      numbits += pnt[i] * num;
    }
    else
      for( j=(int)pnt[i]; --j>=0; ) *scf++ = 0;
  }
  
  n = (n << 1) + 1;
  for( i=0; i<n; ++i )
    *scf++ = 0;

  return numbits;
}

static int pretab1[22] = {0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,2,2,3,3,3,2,0};
static int pretab2[22] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/*
 * Dequantize samples (includes huffman decoding)
 *
 * 24 is enough because tab13 has max. a 19 bit huffvector
 */

#define BITSHIFT ((int)(sizeof(int32_t)-1) * 8)
#define REFRESH_MASK if( num < BITSHIFT ) { \
  if( -part2remain >= num ) break; \
  do { \
    mask |= stream->get_bits(8) << (BITSHIFT-num); \
    num += 8; part2remain -= 8; \
} while( num < BITSHIFT ); }

int zaudio_decoder_layer_t::
dequantize_sample(float xr[SBLIMIT][SSLIMIT], int *scf,
       l3_info_t *l3_info, int sfreq, int part2bits)
{
  short a, *val;
  float v, vv;
  int i, l[3], cb, mc, *m, *me;
  int shift = 1 + l3_info->scalefac_scale;
  float *xrpnt = (float*)xr;
  int part2remain = l3_info->part2_3_length - part2bits;
  int num = stream->get_bit_offset();
  int32_t mask = stream->get_bits(num);

//zmsgs("1 %08x %d\n", mask, num);
  mask = mask << (BITSHIFT + 8 - num);
  part2remain -= num;

  int bv       = l3_info->big_values;
  int region1  = l3_info->region1start;
  int region2  = l3_info->region2start;
  int l3 = ((576 >> 1) - bv) >> 1;   
  /* we may lose the 'odd' bit here !!, check this later again */
  if( bv <= region1 ) {
    l[0] = bv; 
    l[1] = l[2] = 0;
  }
  else {
    l[0] = region1;
    if( bv <= region2 ) {
      l[1] = bv - l[0];  l[2] = 0;
    }
    else {
      l[1] = region2 - l[0]; 
      l[2] = bv - region2;
    }
  }
 
  if( l3_info->block_type == 2 ) {
    /* decoding with short or mixed mode BandIndex table */
    int max[4];
    int step = 0;
    int lwin = 3;
    cb = 0;
    v = 0.0;
    if( l3_info->mixed_block_flag ) {
      max[0] = max[1] = max[2] = 2;
      max[3] = -1;
      m = map[sfreq][0];
      me = mapend[sfreq][0];
    }
    else {
      /* max[3] not floatly needed in this case */
      max[0] = max[1] = max[2] = max[3] = -1;
      m = map[sfreq][1];
      me = mapend[sfreq][1];
    }

    for( mc=i=0; i<2; ++i ) {
      int lp = l[i];
      huffman_t *h = ht + l3_info->table_select[i];
      for( ; lp>0; --lp, --mc ) {
        int x, y;
        if( !mc ) {
          mc    = *m++;
          xrpnt = ((float*)xr) + (*m++);
          lwin  = *m++;
          cb    = *m++;
          if( lwin == 3 ) {
            v = l3_info->pow2gain[(*scf++) << shift];
            step = 1;
          }
          else {
            v = l3_info->full_gain[lwin][(*scf++) << shift];
            step = 3;
          }
        }

        val = h->table;
        REFRESH_MASK;
        while( (y=*val++) < 0) {
          if( mask < 0 ) val -= y;
          --num;
          mask <<= 1;
        }
        x = y >> 4;
        y &= 0xf;

        if( x == 15 && h->linbits ) {
          max[lwin] = cb;
          REFRESH_MASK;
          x += ((uint32_t)mask) >> (BITSHIFT + 8 - h->linbits);
          num -= h->linbits + 1;
          mask <<= h->linbits;
          vv = ispow[x]*v;
          *xrpnt = mask < 0 ? -vv : vv;
          mask <<= 1;
        }
        else if( x != 0 ) {
          max[lwin] = cb;
          if( -part2remain >= num ) break;
          vv = ispow[x]*v;
          *xrpnt = mask < 0 ? -vv : vv;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt = 0.0;

        xrpnt += step;
        if( y == 15 && h->linbits ) {
          max[lwin] = cb;
          REFRESH_MASK;
          y += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
          num -= h->linbits + 1;
          mask <<= h->linbits;
          vv = ispow[y]*v;
          *xrpnt = mask < 0 ? -vv : vv;
          mask <<= 1;
        }
        else if( y != 0 ) {
          max[lwin] = cb;
          if( -part2remain >= num ) break;
          vv = ispow[y]*v;
          *xrpnt = mask < 0 ? -vv : vv;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt = 0.0;
        xrpnt += step;
      }
    }

    for( ;l3 && -part2remain < num; l3--) {
      huffman_t *h = htc + l3_info->count1table_select;
      val = h->table;

      REFRESH_MASK;
      while( (a=*val++) < 0) {
        if( mask < 0 ) val -= a;
        --num;
        mask <<= 1;
      }

      for( i=0; i<4; ++i ) {
        if( !(i & 1) ) {
          if( !mc ) {
            mc = *m++;
            xrpnt = ((float*)xr) + (*m++);
            lwin = *m++;
            cb = *m++;
            if( lwin == 3 ) {
              v = l3_info->pow2gain[(*scf++) << shift];
              step = 1;
            }
            else {
              v = l3_info->full_gain[lwin][(*scf++) << shift];
              step = 3;
            }
          }
          --mc;
        }
        if( (a & (0x8 >> i)) ) {
          max[lwin] = cb;
          if( -part2remain >= num ) break;
          *xrpnt = mask < 0 ?  -v : v;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt = 0.0;
        xrpnt += step;
      }
    }

    if( lwin < 3 ) { /* short band? */
      for(;;) {
        while( --mc >= 0 ) {
          /* short band -> step=3 */
          *xrpnt = 0.0; xrpnt += 3; 
          *xrpnt = 0.0; xrpnt += 3;
        }
        if( m >= me ) break;
        mc = *m++;
        xrpnt = ((float*)xr) + *m++;
        /* optimize: field will be set to zero at the end of the function */
        if( *m++ == 0 ) break; 
        /* cb */
        ++m; 
      }
    }

    l3_info->maxband[0] = max[0] + 1;
    l3_info->maxband[1] = max[1] + 1;
    l3_info->maxband[2] = max[2] + 1;
    l3_info->maxbandl = max[3] + 1;
    int rmax = max[0] > max[1] ? max[0] : max[1];
    rmax = (rmax > max[2] ? rmax : max[2]) + 1;
    l3_info->maxb = rmax ?
      shortLimit[sfreq][rmax] :
      longLimit[sfreq][max[3] + 1];
  }
  else {
    /* decoding with 'long' BandIndex table (block_type != 2) */
    int *pretab = l3_info->preflag ? pretab1 : pretab2;
    int max = -1;
    cb = 0;
    m = map[sfreq][2];
    v = 0.0;
    mc = 0;
    /* long hash table values */
    for( i=0; i<3; ++i ) {
      int lp = l[i];
      huffman_t *h = ht + l3_info->table_select[i];
      for( ; lp>0; --lp, --mc ) {
        int x, y;
        if( !mc ) {
          mc = *m++;
          cb = *m++;
          v = cb != 21 ?
            l3_info->pow2gain[(*scf++ + *pretab++) << shift] : 0.0;
        }
        val = h->table;
        REFRESH_MASK;
        while( (y=*val++) < 0 ) {
          if( mask < 0 ) val -= y;
          --num;
          mask <<= 1;
        }
        x = y >> 4;
        y &= 0x0f;

        if( x == 15 && h->linbits ) {
          max = cb;
          REFRESH_MASK;
          x += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
          num -= h->linbits + 1;
          mask <<= h->linbits;
          vv = ispow[x]*v;
          *xrpnt++ = mask < 0 ? -vv : vv;
          mask <<= 1;
        }
        else if( x ) {
          max = cb;
          if( -part2remain >= num ) break;
          vv = ispow[x]*v;
          *xrpnt++ = mask < 0 ? -vv : vv;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt++ = 0.0;

        if( y == 15 && h->linbits ) {
          max = cb;
          REFRESH_MASK;
          y += ((uint32_t) mask) >> (BITSHIFT + 8 - h->linbits);
          num -= h->linbits + 1;
          mask <<= h->linbits;
          vv = ispow[y]*v;
          *xrpnt++ = mask < 0 ? -vv : vv;
          mask <<= 1;
        }
        else if( y != 0 ) {
          max = cb;
          if( -part2remain >= num ) break;
          vv = ispow[y]*v;
          *xrpnt++ = mask < 0 ? -vv : vv;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt++ = 0.0;
      }
    }

    /* short (count1table) values */
    for( ; l3 && -part2remain < num; l3-- ) {
      huffman_t *h = htc + l3_info->count1table_select;
      val = h->table;
      REFRESH_MASK;
      while( (a=*val++) < 0) {
        if( mask < 0 ) val -= a;
        --num;
        mask <<= 1;
      }

      for( i=0; i<4; ++i ) {
        if( !(i & 1) ) {
          if( !mc ) {
            mc = *m++;
            cb = *m++;
            v =  cb != 21 ?
              l3_info->pow2gain[((*scf++) + (*pretab++)) << shift] : 0.0;
          }
          --mc;
        }
        if( (a & (0x8 >> i)) ) {
          max = cb;
          if( -part2remain >= num ) break;
          *xrpnt++ = mask < 0 ? -v : v;
          --num;
          mask <<= 1;
        }
        else
          *xrpnt++ = 0.0;
      }
    }

    l3_info->maxbandl = max + 1;
    l3_info->maxb = longLimit[sfreq][l3_info->maxbandl];
  }

  while( xrpnt < &xr[SBLIMIT][0] ) *xrpnt++ = 0.0;

  if( -part2remain > num )
    num = -part2remain;
  part2remain += num;

  if( num > 0 ) {
    stream->start_reverse();
    stream->get_bits_reverse(num);
    stream->start_forward();
  }

//zmsgs("3 %d %04x\n", stream->bit_number, stream->show_bits(16));
  while( part2remain > 16 ) {
    stream->get_bits(16); /* Dismiss stuffing Bits */
    part2remain -= 16;
  }

  if( part2remain > 0 )
    stream->get_bits(part2remain);
  else if( part2remain < 0 ) {
    zmsgs("can't rewind stream %d bits! data=%02x%02x%02x%02x\n", -part2remain,
      (uint8_t)stream->input_ptr[-3], (uint8_t)stream->input_ptr[-2], 
      (uint8_t)stream->input_ptr[-1], (uint8_t)stream->input_ptr[0]);
    return 1; /* -> error */
  }
  return 0;
}

int zaudio_decoder_layer_t::
get_side_info(l3_sideinfo_t *si, int channels, int ms_stereo,
    long sfreq, int single, int lsf)
{
  int i, ch, gr;
  int powdiff = (single == 3) ? 4 : 0;
  static const int tabs[2][5] = { { 2,9,5,3,4 } , { 1,8,1,2,9 } };
  const int *tab = tabs[lsf];

  si->main_data_begin = stream->get_bits(tab[1]);
  si->private_bits = stream->get_bits(channels == 1 ? tab[2] : tab[3]);
  if( !lsf ) {
    for( ch=0; ch<channels; ++ch ) {
      si->ch[ch].gr[0].scfsi = -1;
      si->ch[ch].gr[1].scfsi = stream->get_bits(4);
    }
  }

  for( gr=0; gr<tab[0]; ++gr ) {
    for( ch=0; ch<channels; ++ch ) {
      l3_info_t *l3_info = &(si->ch[ch].gr[gr]);
      l3_info->part2_3_length = stream->get_bits(12);
      l3_info->big_values = stream->get_bits(9);
      if( l3_info->big_values > 288 ) {
        zerrs(" big_values too large! %d\n",l3_info->big_values);
        l3_info->big_values = 288;
      }
      l3_info->pow2gain = gainpow2 + 256 - stream->get_bits(8) + powdiff;
      if( ms_stereo ) l3_info->pow2gain += 2;
      l3_info->scalefac_compress = stream->get_bits(tab[4]);

      if(stream->get_bits(1)) {
        /* window switch flag  */
        l3_info->block_type       = stream->get_bits(2);
        l3_info->mixed_block_flag = stream->get_bits(1);
        l3_info->table_select[0]  = stream->get_bits(5);
        l3_info->table_select[1]  = stream->get_bits(5);
        /* table_select[2] not needed, because there is no region2, */
        /* but to satisfy some verifications tools we set it either. */
        l3_info->table_select[2] = 0;
        for( i=0; i<3; ++i )
          l3_info->full_gain[i] = l3_info->pow2gain + (stream->get_bits(3) << 3);

        if( l3_info->block_type == 0 ) {
          zerr("Blocktype == 0 and window-switching == 1 not allowed.\n");
          return 1;
        }
        /* region_count/start parameters are implicit in this case. */       
        if( !lsf || l3_info->block_type == 2 )
          l3_info->region1start = 36 >> 1;
        else {
          /* check this again for 2.5 and sfreq=8 */
          l3_info->region1start =  sfreq == 8 ? 108 >> 1 : 54 >> 1;
        }
        l3_info->region2start = 576 >> 1;
      }
      else {
        int r0c, r1c;
        for( i=0; i<3; ++i )
          l3_info->table_select[i] = stream->get_bits(5);
        r0c = stream->get_bits(4);
        r1c = stream->get_bits(3);
        l3_info->region1start = bandInfo[sfreq].longIdx[r0c + 1] >> 1 ;
        l3_info->region2start = bandInfo[sfreq].longIdx[r0c + 1 + r1c + 1] >> 1;
        l3_info->block_type = 0;
        l3_info->mixed_block_flag = 0;
      }
      if( !lsf ) l3_info->preflag = stream->get_bits(1);
      l3_info->scalefac_scale = stream->get_bits(1);
      l3_info->count1table_select = stream->get_bits(1);
    }
  }
  return 0;
}

int zaudio_decoder_layer_t::
hybrid( float fsIn[SBLIMIT][SSLIMIT], float tsOut[SSLIMIT][SBLIMIT],
     int ch, l3_info_t *l3_info)
{
  float *tspnt = (float *) tsOut;
  float *rawout1,*rawout2;
  int i, b, bt, sb;
  int (*zdct)(float *inbuf, float *o1, float *o2, float *wintab, float *tsbuf);
  
  b = mp3_blc[ch];
  rawout1 = mp3_block[b][ch];
  b = -b + 1;
  rawout2 = mp3_block[b][ch];
  mp3_blc[ch] = b;
  
  sb = 0;
  if( l3_info->mixed_block_flag ) {
    sb = 2;
    dct36(fsIn[0], rawout1, rawout2, win[0], tspnt);
    dct36(fsIn[1], rawout1 + 18, rawout2 + 18, win1[0], tspnt + 1);
    rawout1 += 36; rawout2 += 36; 
    tspnt += 2;
  }

  bt = l3_info->block_type;
  zdct = bt == 2 ? dct12 : dct36;
  for( ; sb <(int)l3_info->maxb; sb+=2, tspnt+=2, rawout1+=36, rawout2+=36 ) {
    zdct(fsIn[sb], rawout1, rawout2, win[bt], tspnt);
    zdct(fsIn[sb + 1], rawout1 + 18, rawout2 + 18, win1[bt], tspnt + 1);
  }

  for( ; sb < (int)SBLIMIT; ++sb, ++tspnt ) {
    for( i=0; i<(int)SSLIMIT; ++i ) {
      tspnt[i * SBLIMIT] = *rawout1++;
      *rawout2++ = 0.0;
    }
  }
  return 0;
}

int zaudio_decoder_layer_t::
antialias(float xr[SBLIMIT][SSLIMIT], l3_info_t *l3_info)
{
  int sblim;
  if( l3_info->block_type == 2 ) {
    if( !l3_info->mixed_block_flag ) return 0;
    sblim = 1; 
  }
  else
    sblim = l3_info->maxb-1;

  /* 31 alias-reduction operations between each pair of sub-bands */
  /* with 8 butterflies between each pair                         */

  float *xr1 = (float*)xr[1];
  for( int sb=sblim; sb > 0; --sb, xr1+=10 ) {
    float *cs = aa_cs;
    float *ca = aa_ca;
    float *xr2 = xr1;
    for( int ss=8; --ss>=0;) {
      /* upper and lower butterfly inputs */
      float bu = *--xr2;
      float bd = *xr1;
      *xr2   = (bu * (*cs)   ) - (bd * (*ca)   );
      *xr1++ = (bd * (*cs++) ) + (bu * (*ca++) );
    }
  }
  return 0;
}

/* 
 * calculate float channel values for Joint-I-Stereo-mode
 */
int zaudio_decoder_layer_t::
calc_i_stereo(float xr_buf[2][SBLIMIT][SSLIMIT], int *scalefac,
       l3_info_t *l3_info, int sfreq, int ms_stereo, int lsf)
{
  float (*xr)[SBLIMIT*SSLIMIT] = (float (*)[SBLIMIT*SSLIMIT])xr_buf;
  struct bandInfoStruct *bi = &bandInfo[sfreq];
  static const float *tabs[3][2][2] = { /* TODO: optimize as static */
    { { tan1_1, tan2_1 }       , { tan1_2, tan2_2 } },
    { { pow1_1[0], pow2_1[0] } , { pow1_2[0], pow2_2[0] } } ,
    { { pow1_1[1], pow2_1[1] } , { pow1_2[1], pow2_2[1] } } 
  };

  int tab = lsf + (l3_info->scalefac_compress & lsf);
  const float *tab1 = tabs[tab][ms_stereo][0];
  const float *tab2 = tabs[tab][ms_stereo][1];
  if( l3_info->block_type == 2 ) {
    int lwin,do_l = 0;
    if( l3_info->mixed_block_flag ) do_l = 1;
    for(lwin = 0; lwin < 3; lwin++) { 
      /* process each window */
      /* get first band with zero values */
      /* sfb is minimal 3 for mixed mode */
      int is_p, sb, idx, sfb = l3_info->maxband[lwin];  
      if(sfb > 3) do_l = 0;

      for( ; sfb < 12 ; ++sfb ) {
        /* scale: 0-15 */ 
        is_p = scalefac[sfb * 3 + lwin - l3_info->mixed_block_flag]; 
        if( is_p != 7 ) {
          sb  = bi->shortDiff[sfb];
          idx = bi->shortIdx[sfb] + lwin;
          float t1  = tab1[is_p]; 
          float t2 = tab2[is_p];
          for( ; sb > 0; --sb, idx+=3 ) {
            float v = xr[0][idx];
            xr[0][idx] = v * t1;
            xr[1][idx] = v * t2;
          }
        }
      }

      /* in the original: copy 10 to 11, */
      /*  here: copy 11 to 12 maybe still wrong??? (copy 12 to 13?) */
      /* scale: 0-15 */
      is_p = scalefac[11 * 3 + lwin - l3_info->mixed_block_flag]; 
      sb   = bi->shortDiff[12];
      idx  = bi->shortIdx[12] + lwin;
      if( is_p != 7 ) {
        float t1, t2;
        t1 = tab1[is_p]; 
        t2 = tab2[is_p];
        for( ; sb > 0; --sb, idx+=3 ) {  
          float v = xr[0][idx];
          xr[0][idx] = v * t1;
          xr[1][idx] = v * t2;
        }
      }
    } /* end for(lwin; .. ; . ) */

    /* also check l-part, if ALL bands in the three windows are 'empty' */
    /*  and mode = mixed_mode */
    if( do_l ) {
      int sfb = l3_info->maxbandl;
      int idx = bi->longIdx[sfb];

      for ( ; sfb < 8; ++sfb ) {
        int sb = bi->longDiff[sfb];
        /* scale: 0-15 */
        int is_p = scalefac[sfb]; 
        if( is_p != 7 ) {
          float t1, t2;
          t1 = tab1[is_p]; 
          t2 = tab2[is_p];
          for( ; sb > 0; sb--, idx++) {
            float v = xr[0][idx];
            xr[0][idx] = v * t1;
            xr[1][idx] = v * t2;
          }
        }
        else 
          idx += sb;
      }
    }     
  } 
  else { /* ((l3_info->block_type != 2)) */
    int sfb = l3_info->maxbandl;
    int is_p, idx = bi->longIdx[sfb];
    for( ; sfb < 21; sfb++) {
      int sb = bi->longDiff[sfb];
      /* scale: 0-15 */
      is_p = scalefac[sfb]; 
      if( is_p != 7 ) {
        float t1 = tab1[is_p]; 
        float t2 = tab2[is_p];
        for( ; sb > 0; --sb, ++idx ) {
          float v = xr[0][idx];
          xr[0][idx] = v * t1;
          xr[1][idx] = v * t2;
        }
      }
      else
        idx += sb;
    }

    is_p = scalefac[20];
    if(is_p != 7) {  
      /* copy l-band 20 to l-band 21 */
      float t1 = tab1[is_p];
      float t2 = tab2[is_p]; 
      for( int sb=bi->longDiff[21]; sb > 0; --sb, ++idx ) {
        float v = xr[0][idx];
        xr[0][idx] = v * t1;
        xr[1][idx] = v * t2;
      }
    }
  } /* ... */

  return 0;
}

int zaudio_decoder_layer_t::
do_layer3(uint8_t *zframe, int zframe_size, float **zoutput, int render)
{
  float *in0, *in1;
  int i, n, gr, ch, ss;
  /* max 39 for short[13][3] mode, mixed: 38, long: 22 */
  int scalefacs[2][39]; 
  l3_sideinfo_t sideinfo;
  int ms_stereo=0, i_stereo=0;
  int sfreq = sampling_frequency_code;
  int stereo1, granules;
  int output_offset = 0;

//zmsg("1\n");
  zframe += 4; /* Skip header */
  zframe_size -= 4;

  /* flip/init buffer */
  bsbuf = &bsspace[bsnum][512];
  bsnum ^= 1;
  /* Copy frame into history buffer */
  memcpy(bsbuf, zframe, zframe_size);
//zmsgs(" %d %02x%02x%02x%02x\n", first_frame, 
// (uint8_t)bsbuf[0], (uint8_t)bsbuf[1], (uint8_t)bsbuf[2], (uint8_t)bsbuf[3]);

  int prev_len = -1;
  uint8_t *ptr = 0;
  if( !first_frame ) {
    /* Set up bitstream to use buffer */
    stream->use_ptr(bsbuf);
//zmsgs(" 7 %x\n", stream->show_bits(16));
    /* CRC must be skipped here for proper alignment with the backstep */
    if( error_protection ) stream->get_bits(16);
//zmsgs(" 8 %x\n", stream->show_bits(16));
    if( channels == 1 ) { /* stream is mono */
      stereo1 = 1;
      single = 0;
    }
    else { /* Stereo */
      stereo1 = 2;
    }

    if( mode == md_JOINT_STEREO ) {
      ms_stereo = (mode_ext & 0x2) >> 1;
      i_stereo  = mode_ext & 0x1;
    }
    else
      ms_stereo = i_stereo = 0;
    granules = lsf ? 1 : 2;
//zmsg(" 6\n");
    if( get_side_info(&sideinfo, channels, ms_stereo, sfreq, single, lsf) ) {
      layer_reset();
      return output_offset;
    }
//zmsg(" 7\n");
    /* Step back */
    if( sideinfo.main_data_begin >= 512 ) return output_offset;
//  if( sideinfo.main_data_begin ) {  /* apparently, zero is legal */
      prev_len = sideinfo.main_data_begin;
      uint8_t *prev = prev_bsbuf + prev_framesize - prev_len;
//zmsgs(" 7 %ld %d %ld\n", ssize, sideinfo.main_data_begin, prev_framesize);
      ptr = bsbuf + ssize - prev_len;
      memcpy(ptr, prev, prev_len);
      past_framesize += prev_framesize;
//  }
  }
  if( ptr && past_framesize >= prev_len ) {
    stream->use_ptr(ptr);
    for( gr=0; gr<granules; ++gr ) {
      float hybridIn [2][SBLIMIT][SSLIMIT];
      float hybridOut[2][SSLIMIT][SBLIMIT];

      l3_info_t *l3_info = &(sideinfo.ch[0].gr[gr]);
      int32_t part2bits;
      part2bits = lsf ?
        get_scale_factors_2(scalefacs[0], l3_info, 0) :
        get_scale_factors_1(scalefacs[0], l3_info, 0, gr);
//zmsgs("4 %04x\n", stream->show_bits(16));
      if( dequantize_sample(hybridIn[0], scalefacs[0],
               l3_info, sfreq, part2bits) ) {
          layer_reset();
          return output_offset;
      }
//zmsgs("5 %04x\n", stream->show_bits(16));
      if( channels == 2 ) {
        l3_info_t *l3_info = &(sideinfo.ch[1].gr[gr]);
        int32_t part2bits = lsf ? 
          get_scale_factors_2(scalefacs[1], l3_info, i_stereo) :
          get_scale_factors_1(scalefacs[1], l3_info, 1, gr);

       if( dequantize_sample(hybridIn[1], scalefacs[1], 
               l3_info, sfreq, part2bits) ) {
          layer_reset();
          return output_offset;
        }

        if( ms_stereo ) {
          int maxb = sideinfo.ch[0].gr[gr].maxb;
          if( (int)sideinfo.ch[1].gr[gr].maxb > maxb )
            maxb = sideinfo.ch[1].gr[gr].maxb;
          for( i=0; i<(int)SSLIMIT * maxb; ++i ) {
            float tmp0 = ((float*)hybridIn[0])[i];
            float tmp1 = ((float*)hybridIn[1])[i];
            ((float*)hybridIn[0])[i] = tmp0 + tmp1;
            ((float*)hybridIn[1])[i] = tmp0 - tmp1;
          }
        }

          if( i_stereo )
            calc_i_stereo(hybridIn, scalefacs[1], l3_info, sfreq, ms_stereo, lsf);
        if( ms_stereo || i_stereo || (single == 3)) {
          if( l3_info->maxb > sideinfo.ch[0].gr[gr].maxb )
            sideinfo.ch[0].gr[gr].maxb = l3_info->maxb;
          else
            l3_info->maxb = sideinfo.ch[0].gr[gr].maxb;
        }
        in0 = (float*)hybridIn[0];
        in1 = (float*)hybridIn[1];
        n = SSLIMIT*l3_info->maxb; 
        switch( single ) {
        case 3:
          /* *0.5 done by pow-scale */
          for( i=0; i<n; ++i, ++in0 ) *in0 += *in1++; 
          break;
        case 1:
          for( i=0; i<n; ++i ) *in0++ = *in1++;
          break;
        }
      }

//zmsg(" 9\n");
      for(ch = 0; ch < stereo1; ch++) {
        l3_info_t *l3_info = &sideinfo.ch[ch].gr[gr];
//zmsg(" 9.1\n");
        antialias(hybridIn[ch], l3_info);
//zmsg(" 9.2\n");
        hybrid(hybridIn[ch], hybridOut[ch], ch, l3_info);
//zmsg(" 9.3\n");
      }
//zmsg(" 10\n");
      if( render && (zoutput[0] || (single < 0 && zoutput[1])) ) {
        int offset0 = output_offset;
        if( single >= 0 ) {
          for( ss=0; ss<(int)SSLIMIT; ++ss )
            synth_stereo(hybridOut[0][ss], 0, zoutput[0], &output_offset);
        }
        else {
          for( ss=0; ss<(int)SSLIMIT; ++ss ) {
            int offset1 = offset0;
            if( zoutput[0] )
              synth_stereo(hybridOut[0][ss], 0, zoutput[0], &offset0);
            if( zoutput[1] ) {
              synth_stereo(hybridOut[1][ss], 1, zoutput[1], &offset1);
              offset0 = offset1;
            }
          }
        }
      }
      output_offset += 32 * SSLIMIT;
    }
  }

//zmsg(" 12\n");
  first_frame = 0;
  prev_bsbuf = bsbuf;
  prev_framesize = zframe_size;
  return output_offset;
}

void zaudio_decoder_layer_t::
layer_reset()
{
//zmsg("1\n");
  first_frame = 1;
  past_framesize = 0;
  bsnum = 0;
  bsbuf = &bsspace[1][512];
  prev_bsbuf = 0;
/*  prev_framesize = 0; */
/*  memset(bsspace, 0, sizeof(bsspace)); */
  memset(mp3_block, 0, sizeof(mp3_block));
  memset(mp3_blc, 0, sizeof(mp3_blc));
  synths_reset();
}

/* Return 1 if the head check doesn't find a header. */
int zaudio_decoder_layer_t::
layer_check(uint8_t *data)
{
  uint32_t head =
    ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
    ((uint32_t)data[2] <<  8) | ((uint32_t)data[3]);
  if( (head & 0xffe00000) != 0xffe00000 ) return 1;
  if( !((head >> 17) & 3) ) return 1;
  if( ((head >> 12) & 0xf) == 0xf ) return 1;
  if( !((head >> 12) & 0xf) ) return 1;
  if( ((head >> 10) & 0x3) == 0x3 ) return 1;
  if( ((head >> 19) & 1) == 1 &&
      ((head >> 17) & 3) == 3 &&
      ((head >> 16) & 1) == 1 ) return 1;
  if( (head & 0xffff0000) == 0xfffe0000 ) return 1;
  if( (head & 0xffff0000) == 0xffed0000 ) return 1; /* JPEG header */
  return 0;
}

int zaudio_decoder_layer_t::
id3_check(uint8_t *data)
{
  return data[0]=='I' && data[1]=='D' && data[2]=='3' ? 1 : 0;
}

/* Decode layer header */
int zaudio_decoder_layer_t::
layer3_header(uint8_t *data)
{
  int zlsf, zmpeg35;
  int zlayer, zchannels, zmode;
  int zsampling_frequency_code;
  uint32_t zheader;
  switch( id3_state ) { /* ID3 tag */
  case id3_IDLE:
    /* Read header */
    if( id3_check(data) ) {
      id3_state = id3_HEADER;
      id3_current_byte = 0;
      return 0;
    }
    break;

    case id3_HEADER:
      if( ++id3_current_byte >= 6 ) {
        id3_size =
          ((uint32_t)data[0] << 21) | ((uint32_t)data[1] << 14) |
          ((uint32_t)data[2] << 7)  | ((uint32_t)data[3]);
        id3_current_byte = 0;
        id3_state = id3_SKIP;
//zmsgs("%d %02x%02x%02x%02x size=0x%x layer=%d\n", __LINE__,
//  data[0], data[1], data[2], data[3], id3_size, layer);
      }
      return 0;

    case id3_SKIP:
//zmsgs("%d id3_current_byte=0x%x %02x%02x%02x%02x\n", __LINE__,
// id3_current_byte, data[0], data[1], data[2], data[3]);
      if( ++id3_current_byte >= id3_size )
        id3_state = id3_IDLE;
      return 0;
  }

  if( layer_check(data) ) return 0;
//zmsgs("%d id3_state=%d %02x%02x%02x%02x\n", __LINE__,
// id3_state, data[0], data[1], data[2], data[3]);
  zheader =
    ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
    ((uint32_t)data[2] <<  8) | ((uint32_t)data[3]);
  if( zheader & (1 << 20) ) {
    zlsf = (zheader & (1 << 19)) ? 0x0 : 0x1;
    zmpeg35 = 0;
  }
  else {
    zlsf = 1;
    zmpeg35 = 1;
  }

  zlayer = 4 - ((zheader >> 17) & 3);
//zmsgs("1 %d zheader=%08x zlayer=%d layer=%d\n", __LINE__,
//   zheader, zlayer, layer);
  if( layer != 0 && zlayer != layer ) return 0;
  zsampling_frequency_code =  zmpeg35 ?
    6 + ((zheader >> 10) & 0x3) : ((zheader >> 10) & 0x3) + (zlsf * 3);

  if( samplerate != 0 &&
      zsampling_frequency_code != sampling_frequency_code ) return 0;
  zmode = ((zheader >> 6) & 0x3);
  zchannels = (zmode == md_MONO) ? 1 : 2;
/*  if( channels >= 0 && zchannels != channels ) return 0; */
/*  if( zchannels > channels ) channels = zchannels; */
  channels = zchannels;
  layer = zlayer;
  lsf = zlsf;
  mpeg35 = zmpeg35;
  mode = zmode;
  sampling_frequency_code = zsampling_frequency_code;
  samplerate = freqs[sampling_frequency_code];
  error_protection = ((zheader >> 16) & 0x1) ^ 0x1;
  bitrate_index = ((zheader >> 12) & 0xf);
  padding   = ((zheader >> 9) & 0x1);
  extension = ((zheader >> 8) & 0x1);
  mode_ext  = ((zheader >> 4) & 0x3);
  copyright = ((zheader >> 3) & 0x1);
  original  = ((zheader >> 2) & 0x1);
  emphasis  = zheader & 0x3;
  single = channels > 1 ? -1 : 3;
  if( !bitrate_index ) return 0;
  bitrate = 1000 * tabsel_123[lsf][layer-1][bitrate_index];
  switch( layer ) {
  case 1:
    framesize  = (long)tabsel_123[lsf][0][bitrate_index] * 12000;
    framesize /= freqs[sampling_frequency_code];
    framesize  = ((framesize + padding) << 2);
    break;
  case 2:
    framesize = (long)tabsel_123[lsf][1][bitrate_index] * 144000;
    framesize /= freqs[sampling_frequency_code];
    framesize += padding;
    break;
  case 3:
    ssize =  lsf ?
      ((channels == 1) ? 9 : 17) :
      ((channels == 1) ? 17 : 32);
    if( error_protection ) ssize += 2;
    framesize  = (long)tabsel_123[lsf][2][bitrate_index] * 144000;
    framesize /= freqs[sampling_frequency_code] << lsf;
    framesize += padding;
    break; 
  default:
    return 0;
  }
//zmsgs("%d bitrate=%d framesize=%ld samplerate=%d channels=%d layer=%d\n", 
// __LINE__, bitrate, framesize, samplerate, channels, layer);
  if( bitrate < 64000 && layer != 3 ) return 0;
  if( framesize > (int)MAXFRAMESIZE ) return 0;
//zmsgs("10 %d\n", layer);
  return framesize;
}

zaudio_decoder_layer_t::
audio_decoder_layer_t()
{
  bo = 1;
  channels = -1;
  stream = new bits_t(0, 0);
  init_decode_tables();
  layer_reset();
}

zaudio_decoder_layer_t::
~audio_decoder_layer_t()
{
  delete stream;
}

