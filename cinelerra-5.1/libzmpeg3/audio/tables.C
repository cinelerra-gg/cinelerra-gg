#include "../libzmpeg3.h"
#include <math.h>

/* Bitrate indexes */
int zaudio_decoder_layer_t::
tabsel_123[2][3][16] = {
  { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
    {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
    {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

  { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
    {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long zaudio_decoder_layer_t::
freqs[9] = {
  44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000
};

#ifdef USE_3DNOW
float zaudio_decoder_layer_t:: decwin[2 * (512 + 32)];
float zaudio_decoder_layer_t:: cos64[32];
float zaudio_decoder_layer_t:: cos32[16];
float zaudio_decoder_layer_t:: cos16[8];
float zaudio_decoder_layer_t:: cos8[4];
float zaudio_decoder_layer_t:: cos4[2];
#else
float zaudio_decoder_layer_t:: decwin[512 + 32];
float zaudio_decoder_layer_t:: cos64[16];
float zaudio_decoder_layer_t:: cos32[8];
float zaudio_decoder_layer_t:: cos16[4];
float zaudio_decoder_layer_t:: cos8[2];
float zaudio_decoder_layer_t:: cos4[1];
#endif

float *zaudio_decoder_layer_t::
pnts[] = {
  zaudio_decoder_layer_t:: cos64,
  zaudio_decoder_layer_t:: cos32,
  zaudio_decoder_layer_t:: cos16,
  zaudio_decoder_layer_t:: cos8,
  zaudio_decoder_layer_t:: cos4
};

int zaudio_decoder_layer_t:: grp_3tab[32 * 3] = { 0, };   /* used: 27 */
int zaudio_decoder_layer_t:: grp_5tab[128 * 3] = { 0, };  /* used: 125 */
int zaudio_decoder_layer_t:: grp_9tab[1024 * 3] = { 0, }; /* used: 729 */
float zaudio_decoder_layer_t:: muls[27][64];  /* also used by layer 1 */
float zaudio_decoder_layer_t:: gainpow2[256 + 118 + 4];
float zaudio_decoder_layer_t:: ispow[8207];
float zaudio_decoder_layer_t:: aa_ca[8];
float zaudio_decoder_layer_t:: aa_cs[8];
float zaudio_decoder_layer_t:: win[4][36];
float zaudio_decoder_layer_t:: win1[4][36];
float zaudio_decoder_layer_t:: COS1[12][6];
float zaudio_decoder_layer_t:: COS9[9];
float zaudio_decoder_layer_t:: COS6_1;
float zaudio_decoder_layer_t:: COS6_2;
float zaudio_decoder_layer_t:: tfcos36[9];
float zaudio_decoder_layer_t:: tfcos12[3];
float zaudio_decoder_layer_t:: cos9[3];
float zaudio_decoder_layer_t:: cos18[3];
float zaudio_decoder_layer_t:: tan1_1[16];
float zaudio_decoder_layer_t:: tan2_1[16];
float zaudio_decoder_layer_t:: tan1_2[16];
float zaudio_decoder_layer_t:: tan2_2[16];
float zaudio_decoder_layer_t:: pow1_1[2][16];
float zaudio_decoder_layer_t:: pow2_1[2][16];
float zaudio_decoder_layer_t:: pow1_2[2][16];
float zaudio_decoder_layer_t:: pow2_2[2][16];

long zaudio_decoder_layer_t::
intwinbase[] = {
     0,    -1,    -1,    -1,    -1,    -1,    -1,    -2,    -2,    -2,
    -2,    -3,    -3,    -4,    -4,    -5,    -5,    -6,    -7,    -7,
    -8,    -9,   -10,   -11,   -13,   -14,   -16,   -17,   -19,   -21,
   -24,   -26,   -29,   -31,   -35,   -38,   -41,   -45,   -49,   -53,
   -58,   -63,   -68,   -73,   -79,   -85,   -91,   -97,  -104,  -111,
  -117,  -125,  -132,  -139,  -147,  -154,  -161,  -169,  -176,  -183,
  -190,  -196,  -202,  -208,  -213,  -218,  -222,  -225,  -227,  -228,
  -228,  -227,  -224,  -221,  -215,  -208,  -200,  -189,  -177,  -163,
  -146,  -127,  -106,   -83,   -57,   -29,     2,    36,    72,   111,
   153,   197,   244,   294,   347,   401,   459,   519,   581,   645,
   711,   779,   848,   919,   991,  1064,  1137,  1210,  1283,  1356,
  1428,  1498,  1567,  1634,  1698,  1759,  1817,  1870,  1919,  1962,
  2001,  2032,  2057,  2075,  2085,  2087,  2080,  2063,  2037,  2000,
  1952,  1893,  1822,  1739,  1644,  1535,  1414,  1280,  1131,   970,
   794,   605,   402,   185,   -45,  -288,  -545,  -814, -1095, -1388,
 -1692, -2006, -2330, -2663, -3004, -3351, -3705, -4063, -4425, -4788,
 -5153, -5517, -5879, -6237, -6589, -6935, -7271, -7597, -7910, -8209,
 -8491, -8755, -8998, -9219, -9416, -9585, -9727, -9838, -9916, -9959,
 -9966, -9935, -9863, -9750, -9592, -9389, -9139, -8840, -8492, -8092,
 -7640, -7134, -6574, -5959, -5288, -4561, -3776, -2935, -2037, -1082,
   -70,   998,  2122,  3300,  4533,  5818,  7154,  8540,  9975, 11455,
 12980, 14548, 16155, 17799, 19478, 21189, 22929, 24694, 26482, 28289,
 30112, 31947, 33791, 35640, 37489, 39336, 41176, 43006, 44821, 46617,
 48390, 50137, 51853, 53534, 55178, 56778, 58333, 59838, 61289, 62684,
 64019, 65290, 66494, 67629, 68692, 69679, 70590, 71420, 72169, 72835,
 73415, 73908, 74313, 74630, 74856, 74992, 75038 };

int zaudio_decoder_layer_t::
longLimit[9][23];
int zaudio_decoder_layer_t::
shortLimit[9][14];

struct zaudio_decoder_layer_t::
bandInfoStruct zaudio_decoder_layer_t::
bandInfo[9] = 
{ 

/* MPEG 1.0 */
 { {0,4,8,12,16,20,24,30,36,44,52,62,74, 90,110,134,162,196,238,288,342,418,576},
   {4,4,4,4,4,4,6,6,8, 8,10,12,16,20,24,28,34,42,50,54, 76,158},
   {0,4*3,8*3,12*3,16*3,22*3,30*3,40*3,52*3,66*3, 84*3,106*3,136*3,192*3},
   {4,4,4,4,6,8,10,12,14,18,22,30,56} } ,

 { {0,4,8,12,16,20,24,30,36,42,50,60,72, 88,106,128,156,190,230,276,330,384,576},
   {4,4,4,4,4,4,6,6,6, 8,10,12,16,18,22,28,34,40,46,54, 54,192},
   {0,4*3,8*3,12*3,16*3,22*3,28*3,38*3,50*3,64*3, 80*3,100*3,126*3,192*3},
   {4,4,4,4,6,6,10,12,14,16,20,26,66} } ,

 { {0,4,8,12,16,20,24,30,36,44,54,66,82,102,126,156,194,240,296,364,448,550,576} ,
   {4,4,4,4,4,4,6,6,8,10,12,16,20,24,30,38,46,56,68,84,102, 26} ,
   {0,4*3,8*3,12*3,16*3,22*3,30*3,42*3,58*3,78*3,104*3,138*3,180*3,192*3} ,
   {4,4,4,4,6,8,12,16,20,26,34,42,12} }  ,

/* MPEG 2.0 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 } ,
   {0,4*3,8*3,12*3,18*3,24*3,32*3,42*3,56*3,74*3,100*3,132*3,174*3,192*3} ,
   {4,4,4,6,6,8,10,14,18,26,32,42,18 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,114,136,162,194,232,278,330,394,464,540,576},
   {6,6,6,6,6,6,8,10,12,14,16,18,22,26,32,38,46,52,64,70,76,36 } ,
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,136*3,180*3,192*3} ,
   {4,4,4,6,8,10,12,14,18,24,32,44,12 } } ,

 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576},
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54 },
   {0,4*3,8*3,12*3,18*3,26*3,36*3,48*3,62*3,80*3,104*3,134*3,174*3,192*3},
   {4,4,4,6,8,10,12,14,18,24,30,40,18 } } ,
/* MPEG 2.5 */
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,6,12,18,24,30,36,44,54,66,80,96,116,140,168,200,238,284,336,396,464,522,576} ,
   {6,6,6,6,6,6,8,10,12,14,16,20,24,28,32,38,46,52,60,68,58,54},
   {0,12,24,36,54,78,108,144,186,240,312,402,522,576},
   {4,4,4,6,8,10,12,14,18,24,30,40,18} },
 { {0,12,24,36,48,60,72,88,108,132,160,192,232,280,336,400,476,566,568,570,572,574,576},
   {12,12,12,12,12,12,16,20,24,28,32,40,48,56,64,76,90,2,2,2,2,2},
   {0, 24, 48, 72,108,156,216,288,372,480,486,492,498,576},
   {8,8,8,12,16,20,24,28,36,2,2,2,26} } ,
};

int zaudio_decoder_layer_t:: mapbuf0[9][152];
int zaudio_decoder_layer_t:: mapbuf1[9][156];
int zaudio_decoder_layer_t:: mapbuf2[9][44];
int *zaudio_decoder_layer_t:: map[9][3];
int *zaudio_decoder_layer_t:: mapend[9][3];

unsigned int zaudio_decoder_layer_t:: n_slen2[512]; /* MPEG 2.0 slen for 'normal' mode */
unsigned int zaudio_decoder_layer_t:: i_slen2[256]; /* MPEG 2.0 slen for intensity stereo */

int zaudio_decoder_layer_t::
init_layer2()
{
  static double mulmul[27] = {
    0.0 , -2.0/3.0 , 2.0/3.0 ,
    2.0/7.0 , 2.0/15.0 , 2.0/31.0, 2.0/63.0 , 2.0/127.0 , 2.0/255.0 ,
    2.0/511.0 , 2.0/1023.0 , 2.0/2047.0 , 2.0/4095.0 , 2.0/8191.0 ,
    2.0/16383.0 , 2.0/32767.0 , 2.0/65535.0 ,
    -4.0/5.0 , -2.0/5.0 , 2.0/5.0, 4.0/5.0 ,
    -8.0/9.0 , -4.0/9.0 , -2.0/9.0 , 2.0/9.0 , 4.0/9.0 , 8.0/9.0 
  };
  static int base[3][9] = {
    { 1 , 0, 2 , } ,
    { 17, 18, 0 , 19, 20 , } ,
    { 21, 1, 22, 23, 0, 24, 25, 2, 26 } 
  };
  static int tablen[3] = { 3, 5, 9 };
  static int *itable, *tables[3] = { grp_3tab, grp_5tab, grp_9tab };
  int i, j, k, l, len;
  float *table;

  for(i = 0; i < 3; i++)
  {
      itable = tables[i];
      len = tablen[i];
      for(j = 0; j < len; j++)
        for(k = 0; k < len; k++)
            for(l = 0; l < len; l++)
            {
              *itable++ = base[i][l];
              *itable++ = base[i][k];
              *itable++ = base[i][j];
            }
  }

  for(k = 0; k < 27; k++)
  {
      double m = mulmul[k];
      table = muls[k];
      for(j = 3, i = 0; i < 63; i++, j--)
          *table++ = m * pow(2.0, (double)j / 3.0);
      *table++ = 0.0;
  }
  return 0;
}

int zaudio_decoder_layer_t::
init_layer3()
{
  int i, j, k, l;
  int down_sample_sblimit = 32;

  mp3_block[0][0][0] = 0;
  mp3_blc[0] = 0;
  mp3_blc[1] = 0;

  for(i = -256; i < 118 + 4; i++)
      gainpow2[i + 256] = pow((double)2.0, -0.25 * (double)(i + 210));

  for(i = 0; i < 8207; i++)
      ispow[i] = pow((double)i, (double)4.0 / 3.0);

  for(i = 0; i < 8; i++) 
  {
    static double Ci[8] = {-0.6,-0.535,-0.33,-0.185,-0.095,-0.041,-0.0142,-0.0037};
    double sq = sqrt(1.0+Ci[i]*Ci[i]);
    aa_cs[i] = 1.0/sq;
    aa_ca[i] = Ci[i]/sq;
  }

  for( i=0; i<18; ++i ) {
    win[0][i]      =
    win[1][i]      = 0.5 * sin( M_PI/72.0 * (double)(2*(i+ 0)+1)) /
                           cos(      M_PI * (double)(2*(i+ 0)+19) / 72.0);
    win[0][i + 18] =
    win[3][i + 18] = 0.5 * sin( M_PI/72.0 * (double)(2*(i+18)+1)) /
                           cos(      M_PI * (double)(2*(i+18)+19) / 72.0);
  }
  for( i=0; i<6; ++i ) {
    win[1][i + 18] = 0.5 / cos(      M_PI * (double)(2*(i+18)+19) / 72.0);
    win[3][i + 12] = 0.5 / cos(      M_PI * (double)(2*(i+12)+19) / 72.0);
    win[1][i + 24] = 0.5 * sin( M_PI/24.0 * (double)(2*(i+ 0)+13)) /
                           cos(      M_PI * (double)(2*(i+24)+19) / 72.0);
    win[1][i + 30] = win[3][i] = 0.0;
    win[3][i + 6 ] = 0.5 * sin( M_PI/24.0 * (double)(2*(i+ 0)+1)) /
                           cos(      M_PI * (double)(2*(i+ 6)+19) / 72.0);
  }

  for(i = 0; i < 9; i++)
    COS9[i] = cos(M_PI / 18.0 * (double)i);

  for(i = 0; i < 9; i++)
    tfcos36[i] = 0.5 / cos (M_PI * (double) (i*2+1) / 36.0);
  for(i = 0; i < 3; i++)
    tfcos12[i] = 0.5 / cos (M_PI * (double) (i*2+1) / 12.0);

  COS6_1 = cos( M_PI / 6.0 * (double) 1);
  COS6_2 = cos( M_PI / 6.0 * (double) 2);

  cos9[0]  = cos(1.0 * M_PI / 9.0);
  cos9[1]  = cos(5.0 * M_PI / 9.0);
  cos9[2]  = cos(7.0 * M_PI / 9.0);
  cos18[0] = cos(1.0 * M_PI / 18.0);
  cos18[1] = cos(11.0 * M_PI / 18.0);
  cos18[2] = cos(13.0 * M_PI / 18.0);

  for( i=0; i < 12; ++i ) {
    win[2][i]  = 0.5 * sin(M_PI / 24.0 * (double)(2*i + 1)) /
                       cos(M_PI * (double)(2*i + 7) / 24.0);
    for( j=0; j < 6; ++j )
      COS1[i][j] = cos(M_PI / 24.0 * (double)((2*i + 7) * (2*j + 1)));
  }

  for( j=0; j < 4; j++) {
    static int len[4] = {36, 36, 12, 36};
    for(i = 0; i < len[j]; i+=2 ) win1[j][i] =  win[j][i];
    for(i = 1; i < len[j]; i+=2 ) win1[j][i] = -win[j][i];
  }

  for( i=0; i < 16; ++i ) {
    double t = tan( (double) i * M_PI / 12.0 );
    tan1_1[i] = t / (1.0 + t);
    tan2_1[i] = 1.0 / (1.0 + t);
    tan1_2[i] = M_SQRT2 * t / (1.0 + t);
    tan2_2[i] = M_SQRT2 / (1.0 + t);

    for( j=0; j < 2; ++j ) {
      double base = pow(2.0, -0.25 * (j + 1.0));
      double p1 = 1.0,p2 = 1.0;
      if( i > 0 ) {
        if( i & 1 )
          p1 = pow(base, (i + 1.0) * 0.5);
        else
          p2 = pow(base, i * 0.5);
      }
      pow1_1[j][i] = p1;
      pow2_1[j][i] = p2;
      pow1_2[j][i] = M_SQRT2 * p1;
      pow2_2[j][i] = M_SQRT2 * p2;
    }
  }

  for( j=0; j < 9; ++j ) {
    bandInfoStruct *bi = &bandInfo[j];
    int cb, lwin;
    int *mp = map[j][0] = mapbuf0[j];
    int *bdf = bi->longDiff;
    for( i=0, cb=0; cb < 8; ++cb, i+=*bdf++ ) {
      *mp++ = *bdf >> 1;
      *mp++ = i;
      *mp++ = 3;
      *mp++ = cb;
    }
    bdf = bi->shortDiff + 3;
    for( cb=3; cb < 13; ++cb ) {
      int l = (*bdf++) >> 1;
      for( lwin=0; lwin < 3; ++lwin ) {
        *mp++ = l;
        *mp++ = i + lwin;
        *mp++ = lwin;
        *mp++ = cb;
      }
      i += 6 * l;
    }
    mapend[j][0] = mp;

    mp = map[j][1] = mapbuf1[j];
    bdf = bi->shortDiff+0;
    for( i=0,cb=0; cb < 13; ++cb ) {
      int l = (*bdf++) >> 1;
      for( lwin=0; lwin < 3; ++lwin ) {
        *mp++ = l;
        *mp++ = i + lwin;
        *mp++ = lwin;
        *mp++ = cb;
      }
      i += 6 * l;
    }
    mapend[j][1] = mp;

    mp = map[j][2] = mapbuf2[j];
    bdf = bi->longDiff;
    for( cb=0; cb < 22 ; ++cb ) {
      *mp++ = (*bdf++) >> 1;
      *mp++ = cb;
    }
    mapend[j][2] = mp;
  }

  for( j=0; j < 9; ++j ) {
    for( i=0; i < 23; ++i ) {
      longLimit[j][i] = (bandInfo[j].longIdx[i] - 1 + 8) / 18 + 1;
      if( longLimit[j][i] > down_sample_sblimit )
        longLimit[j][i] = down_sample_sblimit;
    }
    for( i=0; i < 14; ++i ) {
      shortLimit[j][i] = (bandInfo[j].shortIdx[i] - 1) / 18 + 1;
      if( shortLimit[j][i] > down_sample_sblimit )
        shortLimit[j][i] = down_sample_sblimit;
    }
  }

  for( i=0; i < 5; ++i ) {
    for( j=0; j < 6; ++j ) {
      for( k=0; k < 6; ++k ) {
        int n = k + j * 6 + i * 36;
        i_slen2[n] = i | (j << 3) | (k << 6) | (3 << 12);
      }
    }
  }
  for( i=0; i < 4; ++i ) {
    for( j=0; j < 4; ++j ) {
        for( k=0; k < 4; ++k ) {
          int n = k + j * 4 + i * 16;
          i_slen2[n+180] = i | (j << 3) | (k << 6) | (4 << 12);
        }
    }
  }
  for( i=0; i < 4; ++i ) {
    for( j=0; j < 3; ++j ) {
      int n = j + i * 3;
      i_slen2[n + 244] = i | (j << 3) | (5 << 12);
      n_slen2[n + 500] = i | (j << 3) | (2 << 12) | (1 << 15);
    }
  }
  for( i=0; i < 5; ++i ) {
    for( j=0; j < 5; ++j ) {
      for( k=0; k < 4; ++k ) {
        for( l=0; l < 4; ++l ) {
          int n = l + k * 4 + j * 16 + i * 80;
          n_slen2[n] = i | (j << 3) | ( k << 6) | (l << 9) | (0 << 12);
        }
      }
    }
  }
  for( i=0; i < 5; ++i ) {
    for( j=0; j < 5; ++j ) {
      for( k=0; k < 4; ++k ) {
        int n = k + j * 4 + i * 20;
        n_slen2[n + 400] = i | (j << 3) | (k << 6) | (1 << 12);
      }
    }
  }
  return 0;
}

int zaudio_decoder_layer_t::
init_decode_tables()
{
  static int inited = 0;
  if( inited ) return 0;
  inited = 1;
  int i, j, k, kr, divv;
  float *costab;
  int idx;
  long scaleval = 1;
  
  for( i=0; i < 5; ++i ) {
    kr = 0x10 >> i; 
    divv = 0x40 >> i;
    costab = pnts[i];
    for( k=0; k < kr; ++k )
      costab[k] = 1.0 / (2.0 * cos(M_PI * ((double)k * 2.0 + 1.0) / (double)divv));

#ifdef USE_3DNOW
      for( k=0; k < kr; ++k )
        costab[k + kr] = -costab[k];
#endif
  }

  idx = 0;
  scaleval = -scaleval;
  for( i=0, j=0; i < 256; ++i, ++j, idx+=32 ) {
    if( idx < 512 + 16 ) decwin[idx+16] = decwin[idx] =
          (double)intwinbase[j] / 65536.0 * (double)scaleval;
    if( i % 32 == 31 ) idx -= 1023;
    if( i % 64 == 63 ) scaleval = -scaleval;
  }

  for( ; i < 512; ++i, --j, idx+=32 ) {
    if( idx < 512 + 16 ) decwin[idx + 16] = decwin[idx] =
          (double)intwinbase[j] / 65536.0 * (double)scaleval;
    if( i % 32 == 31 ) idx -= 1023;
    if( i % 64 == 63 ) scaleval = -scaleval;
  }

#ifdef USE_3DNOW
  if( !param.down_sample ) {
    for( i=0; i < 512 + 32; ++i ) {
      decwin[512 + 31 - i] *= 65536.0; /* allows faster clipping in 3dnow code */
      decwin[512 + 32 + i] = decwin[512 + 31 - i];
    }
  }
#endif

/* Initialize AC3 */
//  imdct_init();
/* Initialize MPEG */
  /* inits also shared tables with layer1 */
  init_layer2();
  init_layer3();
  return 0;
}

