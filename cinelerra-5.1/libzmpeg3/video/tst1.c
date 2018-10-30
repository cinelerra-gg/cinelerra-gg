#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define USE_MMX
#define MMX_ACCURATE

#ifdef USE_MMX
#include "mmx.h"
#define m_(v) (*(mmx_t*)(((long long *)(v))))
#ifdef MMX_ACCURATE
static uint32_t sadd1[2] = { 0x00010001, 0x00010001 };
static uint32_t sadd2[2] = { 0x00020002, 0x00020002 };
#else
static uint32_t bmask[2] = { 0x7f7f7f7f, 0x7f7f7f7f };
static uint32_t badd1[2] = { 0x01010101, 0x01010101 };
#endif
#endif

static inline void reca(uint8_t *s, uint8_t  *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (uint32_t)(dp[0] + sp[0] + 1) >> 1;
    dp[1] = (uint32_t)(dp[1] + sp[1] + 1) >> 1;
    dp[2] = (uint32_t)(dp[2] + sp[2] + 1) >> 1;
    dp[3] = (uint32_t)(dp[3] + sp[3] + 1) >> 1;
    dp[4] = (uint32_t)(dp[4] + sp[4] + 1) >> 1;
    dp[5] = (uint32_t)(dp[5] + sp[5] + 1) >> 1;
    dp[6] = (uint32_t)(dp[6] + sp[6] + 1) >> 1;
    dp[7] = (uint32_t)(dp[7] + sp[7] + 1) >> 1;
    dp[8] = (uint32_t)(dp[8] + sp[8] + 1) >> 1;
    dp[9] = (uint32_t)(dp[9] + sp[9] + 1) >> 1;
    dp[10] = (uint32_t)(dp[10] + sp[10] + 1) >> 1;
    dp[11] = (uint32_t)(dp[11] + sp[11] + 1) >> 1;
    dp[12] = (uint32_t)(dp[12] + sp[12] + 1) >> 1;
    dp[13] = (uint32_t)(dp[13] + sp[13] + 1) >> 1;
    dp[14] = (uint32_t)(dp[14] + sp[14] + 1) >> 1;
    dp[15] = (uint32_t)(dp[15] + sp[15] + 1) >> 1;
    sp += lx2;  dp += lx2;
  }
}

static inline void mreca(uint8_t *s, uint8_t  *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(dp+0),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp+8),mm1);
    movq_m2r(m_(dp+8),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  dp += lx2;
  }
  emms();
}

static inline void recac(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (uint32_t)(dp[0] + sp[0] + 1)>>1;
    dp[1] = (uint32_t)(dp[1] + sp[1] + 1)>>1;
    dp[2] = (uint32_t)(dp[2] + sp[2] + 1)>>1;
    dp[3] = (uint32_t)(dp[3] + sp[3] + 1)>>1;
    dp[4] = (uint32_t)(dp[4] + sp[4] + 1)>>1;
    dp[5] = (uint32_t)(dp[5] + sp[5] + 1)>>1;
    dp[6] = (uint32_t)(dp[6] + sp[6] + 1)>>1;
    dp[7] = (uint32_t)(dp[7] + sp[7] + 1)>>1;
    sp += lx2;  dp += lx2;
  }
}

static inline void mrecac(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(dp+0),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  dp += lx2;
  }
  emms();
}

static inline void recv(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (uint32_t)(sp[0] + sp2[0] + 1) >> 1;
    dp[1] = (uint32_t)(sp[1] + sp2[1] + 1) >> 1;
    dp[2] = (uint32_t)(sp[2] + sp2[2] + 1) >> 1;
    dp[3] = (uint32_t)(sp[3] + sp2[3] + 1) >> 1;
    dp[4] = (uint32_t)(sp[4] + sp2[4] + 1) >> 1;
    dp[5] = (uint32_t)(sp[5] + sp2[5] + 1) >> 1;
    dp[6] = (uint32_t)(sp[6] + sp2[6] + 1) >> 1;
    dp[7] = (uint32_t)(sp[7] + sp2[7] + 1) >> 1;
    dp[8] = (uint32_t)(sp[8] + sp2[8] + 1) >> 1;
    dp[9] = (uint32_t)(sp[9] + sp2[9] + 1) >> 1;
    dp[10] = (uint32_t)(sp[10] + sp2[10] + 1) >> 1;
    dp[11] = (uint32_t)(sp[11] + sp2[11] + 1) >> 1;
    dp[12] = (uint32_t)(sp[12] + sp2[12] + 1) >> 1;
    dp[13] = (uint32_t)(sp[13] + sp2[13] + 1) >> 1;
    dp[14] = (uint32_t)(sp[14] + sp2[14] + 1) >> 1;
    dp[15] = (uint32_t)(sp[15] + sp2[15] + 1) >> 1;
    sp += lx2;  sp2 += lx2 ; dp += lx2;
  }
}

static inline void mrecv(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp2+0),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp +8),mm1);
    movq_m2r(m_(sp2+8),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  sp2 += lx2 ; dp += lx2;
  }
  emms();
}

static inline void recvc(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (uint32_t)(sp[0]+sp2[0]+1)>>1;
    dp[1] = (uint32_t)(sp[1]+sp2[1]+1)>>1;
    dp[2] = (uint32_t)(sp[2]+sp2[2]+1)>>1;
    dp[3] = (uint32_t)(sp[3]+sp2[3]+1)>>1;
    dp[4] = (uint32_t)(sp[4]+sp2[4]+1)>>1;
    dp[5] = (uint32_t)(sp[5]+sp2[5]+1)>>1;
    dp[6] = (uint32_t)(sp[6]+sp2[6]+1)>>1;
    dp[7] = (uint32_t)(sp[7]+sp2[7]+1)>>1;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrecvc(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp2+0),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void recva(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (dp[0] + ((uint32_t)(sp[0]+sp2[0]+1)>>1) + 1)>>1;
    dp[1] = (dp[1] + ((uint32_t)(sp[1]+sp2[1]+1)>>1) + 1)>>1;
    dp[2] = (dp[2] + ((uint32_t)(sp[2]+sp2[2]+1)>>1) + 1)>>1;
    dp[3] = (dp[3] + ((uint32_t)(sp[3]+sp2[3]+1)>>1) + 1)>>1;
    dp[4] = (dp[4] + ((uint32_t)(sp[4]+sp2[4]+1)>>1) + 1)>>1;
    dp[5] = (dp[5] + ((uint32_t)(sp[5]+sp2[5]+1)>>1) + 1)>>1;
    dp[6] = (dp[6] + ((uint32_t)(sp[6]+sp2[6]+1)>>1) + 1)>>1;
    dp[7] = (dp[7] + ((uint32_t)(sp[7]+sp2[7]+1)>>1) + 1)>>1;
    dp[8] = (dp[8] + ((uint32_t)(sp[8]+sp2[8]+1)>>1) + 1)>>1;
    dp[9] = (dp[9] + ((uint32_t)(sp[9]+sp2[9]+1)>>1) + 1)>>1;
    dp[10] = (dp[10] + ((uint32_t)(sp[10]+sp2[10]+1)>>1) + 1)>>1;
    dp[11] = (dp[11] + ((uint32_t)(sp[11]+sp2[11]+1)>>1) + 1)>>1;
    dp[12] = (dp[12] + ((uint32_t)(sp[12]+sp2[12]+1)>>1) + 1)>>1;
    dp[13] = (dp[13] + ((uint32_t)(sp[13]+sp2[13]+1)>>1) + 1)>>1;
    dp[14] = (dp[14] + ((uint32_t)(sp[14]+sp2[14]+1)>>1) + 1)>>1;
    dp[15] = (dp[15] + ((uint32_t)(sp[15]+sp2[15]+1)>>1) + 1)>>1;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrecva(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp2+0),mm3);
    movq_m2r(m_(dp +0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp +8),mm1);
    movq_m2r(m_(sp2+8),mm3);
    movq_m2r(m_(dp +8),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void recvac(uint8_t *s, uint8_t *d, int lx,int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  int j; for( j=0; j<h; ++j ) {
    dp[0] = (dp[0] + ((uint32_t)(sp[0]+sp2[0]+1)>>1) + 1)>>1;
    dp[1] = (dp[1] + ((uint32_t)(sp[1]+sp2[1]+1)>>1) + 1)>>1;
    dp[2] = (dp[2] + ((uint32_t)(sp[2]+sp2[2]+1)>>1) + 1)>>1;
    dp[3] = (dp[3] + ((uint32_t)(sp[3]+sp2[3]+1)>>1) + 1)>>1;
    dp[4] = (dp[4] + ((uint32_t)(sp[4]+sp2[4]+1)>>1) + 1)>>1;
    dp[5] = (dp[5] + ((uint32_t)(sp[5]+sp2[5]+1)>>1) + 1)>>1;
    dp[6] = (dp[6] + ((uint32_t)(sp[6]+sp2[6]+1)>>1) + 1)>>1;
    dp[7] = (dp[7] + ((uint32_t)(sp[7]+sp2[7]+1)>>1) + 1)>>1;
    sp += lx2;  sp2 += lx2;  dp+= lx2;
  }
}

static inline void mrecvac(uint8_t *s, uint8_t *d, int lx,int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp2+0),mm3);
    movq_m2r(m_(dp +0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void rech(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  int j; for( j=0; j<h; ++j ) {
    s1=sp[0];
    dp[0] = (uint32_t)(s1+(s2=sp[1])+1)>>1;
    dp[1] = (uint32_t)(s2+(s1=sp[2])+1)>>1;
    dp[2] = (uint32_t)(s1+(s2=sp[3])+1)>>1;
    dp[3] = (uint32_t)(s2+(s1=sp[4])+1)>>1;
    dp[4] = (uint32_t)(s1+(s2=sp[5])+1)>>1;
    dp[5] = (uint32_t)(s2+(s1=sp[6])+1)>>1;
    dp[6] = (uint32_t)(s1+(s2=sp[7])+1)>>1;
    dp[7] = (uint32_t)(s2+(s1=sp[8])+1)>>1;
    dp[8] = (uint32_t)(s1+(s2=sp[9])+1)>>1;
    dp[9] = (uint32_t)(s2+(s1=sp[10])+1)>>1;
    dp[10] = (uint32_t)(s1+(s2=sp[11])+1)>>1;
    dp[11] = (uint32_t)(s2+(s1=sp[12])+1)>>1;
    dp[12] = (uint32_t)(s1+(s2=sp[13])+1)>>1;
    dp[13] = (uint32_t)(s2+(s1=sp[14])+1)>>1;
    dp[14] = (uint32_t)(s1+(s2=sp[15])+1)>>1;
    dp[15] = (uint32_t)(s2+sp[16]+1)>>1;
    sp += lx2; dp += lx2;
  }
}

static inline void mrech(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(sp+1),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp+8),mm1);
    movq_m2r(m_(sp+9),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  dp += lx2;
  }
  emms();
}


static inline void rechc(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  int j; for( j=0; j<h; ++j ) {
    s1=sp[0];
    dp[0] = (uint32_t)(s1+(s2=sp[1])+1)>>1;
    dp[1] = (uint32_t)(s2+(s1=sp[2])+1)>>1;
    dp[2] = (uint32_t)(s1+(s2=sp[3])+1)>>1;
    dp[3] = (uint32_t)(s2+(s1=sp[4])+1)>>1;
    dp[4] = (uint32_t)(s1+(s2=sp[5])+1)>>1;
    dp[5] = (uint32_t)(s2+(s1=sp[6])+1)>>1;
    dp[6] = (uint32_t)(s1+(s2=sp[7])+1)>>1;
    dp[7] = (uint32_t)(s2+sp[8]+1)>>1;
    sp += lx2;  dp += lx2;
  }
}

static inline void mrechc(uint8_t *s, uint8_t *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(sp+1),mm3);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  dp += lx2;
  }
  emms();
}

static inline void recha(uint8_t *s, uint8_t *d,int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  int j; for( j=0; j<h; ++j ) {
    s1 = sp[0];
    dp[0] = (dp[0] + ((uint32_t)(s1 + (s2 = sp[1]) + 1) >> 1) + 1) >> 1;
    dp[1] = (dp[1] + ((uint32_t)(s2 + (s1 = sp[2]) + 1) >> 1) + 1) >> 1;
    dp[2] = (dp[2] + ((uint32_t)(s1 + (s2 = sp[3]) + 1) >> 1) + 1) >> 1;
    dp[3] = (dp[3] + ((uint32_t)(s2 + (s1 = sp[4]) + 1) >> 1) + 1) >> 1;
    dp[4] = (dp[4] + ((uint32_t)(s1 + (s2 = sp[5]) + 1) >> 1) + 1) >> 1;
    dp[5] = (dp[5] + ((uint32_t)(s2 + (s1 = sp[6]) + 1) >> 1) + 1) >> 1;
    dp[6] = (dp[6] + ((uint32_t)(s1 + (s2 = sp[7]) + 1) >> 1) + 1) >> 1;
    dp[7] = (dp[7] + ((uint32_t)(s2 + (s1 = sp[8]) + 1) >> 1) + 1) >> 1;
    dp[8] = (dp[8] + ((uint32_t)(s1 + (s2 = sp[9]) + 1) >> 1) + 1) >> 1;
    dp[9] = (dp[9] + ((uint32_t)(s2 + (s1 = sp[10]) + 1) >> 1) + 1) >> 1;
    dp[10] = (dp[10] + ((uint32_t)(s1 + (s2 = sp[11]) + 1) >> 1) + 1) >> 1;
    dp[11] = (dp[11] + ((uint32_t)(s2 + (s1 = sp[12]) + 1) >> 1) + 1) >> 1;
    dp[12] = (dp[12] + ((uint32_t)(s1 + (s2 = sp[13]) + 1) >> 1) + 1) >> 1;
    dp[13] = (dp[13] + ((uint32_t)(s2 + (s1 = sp[14]) + 1) >> 1) + 1) >> 1;
    dp[14] = (dp[14] + ((uint32_t)(s1 + (s2 = sp[15]) + 1) >> 1) + 1) >> 1;
    dp[15] = (dp[15] + ((uint32_t)(s2 + sp[16] + 1) >> 1) + 1) >> 1;
    sp += lx2;  dp += lx2;
  }
}

static inline void mrecha(uint8_t *s, uint8_t *d,int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(sp+1),mm3);
    movq_m2r(m_(dp+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp+8),mm1);
    movq_m2r(m_(sp+9),mm3);
    movq_m2r(m_(dp+8),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  dp += lx2;
  }
  emms();
}


static inline void rechac(uint8_t *s, uint8_t  *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  int j; for( j=0; j<h; ++j ) {
    s1 = sp[0];
    dp[0] = (dp[0] + ((uint32_t)(s1 + (s2 = sp[1]) + 1) >> 1) + 1) >> 1;
    dp[1] = (dp[1] + ((uint32_t)(s2 + (s1 = sp[2]) + 1) >> 1) + 1) >> 1;
    dp[2] = (dp[2] + ((uint32_t)(s1 + (s2 = sp[3]) + 1) >> 1) + 1) >> 1;
    dp[3] = (dp[3] + ((uint32_t)(s2 + (s1 = sp[4]) + 1) >> 1) + 1) >> 1;
    dp[4] = (dp[4] + ((uint32_t)(s1 + (s2 = sp[5]) + 1) >> 1) + 1) >> 1;
    dp[5] = (dp[5] + ((uint32_t)(s2 + (s1 = sp[6]) + 1) >> 1) + 1) >> 1;
    dp[6] = (dp[6] + ((uint32_t)(s1 + (s2 = sp[7]) + 1) >> 1) + 1) >> 1;
    dp[7] = (dp[7] + ((uint32_t)(s2 + sp[8] + 1) >> 1) + 1) >> 1;
    sp += lx2;  dp += lx2;
  }
}

static inline void mrechac(uint8_t *s, uint8_t  *d, int lx2, int h)
{
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm1);
    movq_m2r(m_(sp+1),mm3);
    movq_m2r(m_(dp+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm6,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  dp += lx2;
  }
  emms();
}


static inline void rec4(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  int j; for( j=0; j<h; ++j ) {
    s1=sp[0]; s3=sp2[0];
    dp[0] = (uint32_t)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2;
    dp[1] = (uint32_t)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2;
    dp[2] = (uint32_t)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2;
    dp[3] = (uint32_t)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2;
    dp[4] = (uint32_t)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2;
    dp[5] = (uint32_t)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2;
    dp[6] = (uint32_t)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2;
    dp[7] = (uint32_t)(s2+(s1=sp[8])+s4+(s3=sp2[8])+2)>>2;
    dp[8] = (uint32_t)(s1+(s2=sp[9])+s3+(s4=sp2[9])+2)>>2;
    dp[9] = (uint32_t)(s2+(s1=sp[10])+s4+(s3=sp2[10])+2)>>2;
    dp[10] = (uint32_t)(s1+(s2=sp[11])+s3+(s4=sp2[11])+2)>>2;
    dp[11] = (uint32_t)(s2+(s1=sp[12])+s4+(s3=sp2[12])+2)>>2;
    dp[12] = (uint32_t)(s1+(s2=sp[13])+s3+(s4=sp2[13])+2)>>2;
    dp[13] = (uint32_t)(s2+(s1=sp[14])+s4+(s3=sp2[14])+2)>>2;
    dp[14] = (uint32_t)(s1+(s2=sp[15])+s3+(s4=sp2[15])+2)>>2;
    dp[15] = (uint32_t)(s2+sp[16]+s4+sp2[16]+2)>>2;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrec4(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp +1),mm3);
    movq_m2r(m_(sp2+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+1),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp +8),mm1);
    movq_m2r(m_(sp +9),mm3);
    movq_m2r(m_(sp2+8),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+9),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void rec4c(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  int j; for( j=0; j<h; ++j ) {
    s1=sp[0]; s3=sp2[0];
    dp[0] = (uint32_t)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2;
    dp[1] = (uint32_t)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2;
    dp[2] = (uint32_t)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2;
    dp[3] = (uint32_t)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2;
    dp[4] = (uint32_t)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2;
    dp[5] = (uint32_t)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2;
    dp[6] = (uint32_t)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2;
    dp[7] = (uint32_t)(s2+sp[8]+s4+sp2[8]+2)>>2;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrec4c(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp +1),mm3);
    movq_m2r(m_(sp2+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+1),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm7,mm1);
    paddusw_r2r(mm7,mm2);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void rec4a(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  int j; for( j=0; j<h; ++j ) {
    s1 = sp[0];  s3 = sp2[0];
    dp[0] = (dp[0] + ((uint32_t)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2) + 1)>>1;
    dp[1] = (dp[1] + ((uint32_t)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2) + 1)>>1;
    dp[2] = (dp[2] + ((uint32_t)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2) + 1)>>1;
    dp[3] = (dp[3] + ((uint32_t)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2) + 1)>>1;
    dp[4] = (dp[4] + ((uint32_t)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2) + 1)>>1;
    dp[5] = (dp[5] + ((uint32_t)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2) + 1)>>1;
    dp[6] = (dp[6] + ((uint32_t)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2) + 1)>>1;
    dp[7] = (dp[7] + ((uint32_t)(s2+(s1=sp[8])+s4+(s3=sp2[8])+2)>>2) + 1)>>1;
    dp[8] = (dp[8] + ((uint32_t)(s1+(s2=sp[9])+s3+(s4=sp2[9])+2)>>2) + 1)>>1;
    dp[9] = (dp[9] + ((uint32_t)(s2+(s1=sp[10])+s4+(s3=sp2[10])+2)>>2) + 1)>>1;
    dp[10] = (dp[10] + ((uint32_t)(s1+(s2=sp[11])+s3+(s4=sp2[11])+2)>>2) + 1)>>1;
    dp[11] = (dp[11] + ((uint32_t)(s2+(s1=sp[12])+s4+(s3=sp2[12])+2)>>2) + 1)>>1;
    dp[12] = (dp[12] + ((uint32_t)(s1+(s2=sp[13])+s3+(s4=sp2[13])+2)>>2) + 1)>>1;
    dp[13] = (dp[13] + ((uint32_t)(s2+(s1=sp[14])+s4+(s3=sp2[14])+2)>>2) + 1)>>1;
    dp[14] = (dp[14] + ((uint32_t)(s1+(s2=sp[15])+s3+(s4=sp2[15])+2)>>2) + 1)>>1;
    dp[15] = (dp[15] + ((uint32_t)(s2+sp[16]+s4+sp2[16]+2)>>2) + 1)>>1;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrec4a(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp +1),mm3);
    movq_m2r(m_(sp2+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+1),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    movq_m2r(m_(dp +0),mm3);
    paddusw_r2r(mm7,mm1);
    movq_r2r(mm3,mm4);
    paddusw_r2r(mm7,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    movq_m2r(m_(sadd1),mm5);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm5,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    movq_m2r(m_(sp +8),mm1);
    movq_m2r(m_(sp +9),mm3);
    movq_m2r(m_(sp2+8),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+9),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    movq_m2r(m_(dp +8),mm3);
    paddusw_r2r(mm7,mm1);
    movq_r2r(mm3,mm4);
    paddusw_r2r(mm7,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    movq_m2r(m_(sadd1),mm5);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm5,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+8));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}


static inline void rec4ac(uint8_t *s, uint8_t  *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1,s2,s3,s4;
  int j; for( j=0; j<h; ++j ) {
    s1=sp[0]; s3=sp2[0];
    dp[0] = (dp[0] + ((uint32_t)(s1+(s2=sp[1])+s3+(s4=sp2[1])+2)>>2) + 1)>>1;
    dp[1] = (dp[1] + ((uint32_t)(s2+(s1=sp[2])+s4+(s3=sp2[2])+2)>>2) + 1)>>1;
    dp[2] = (dp[2] + ((uint32_t)(s1+(s2=sp[3])+s3+(s4=sp2[3])+2)>>2) + 1)>>1;
    dp[3] = (dp[3] + ((uint32_t)(s2+(s1=sp[4])+s4+(s3=sp2[4])+2)>>2) + 1)>>1;
    dp[4] = (dp[4] + ((uint32_t)(s1+(s2=sp[5])+s3+(s4=sp2[5])+2)>>2) + 1)>>1;
    dp[5] = (dp[5] + ((uint32_t)(s2+(s1=sp[6])+s4+(s3=sp2[6])+2)>>2) + 1)>>1;
    dp[6] = (dp[6] + ((uint32_t)(s1+(s2=sp[7])+s3+(s4=sp2[7])+2)>>2) + 1)>>1;
    dp[7] = (dp[7] + ((uint32_t)(s2+sp[8]+s4+sp2[8]+2)>>2) + 1)>>1;
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
}

static inline void mrec4ac(uint8_t *s, uint8_t  *d, int lx, int lx2, int h)
{
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  int j; for( j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm1);
    movq_m2r(m_(sp +1),mm3);
    movq_m2r(m_(sp2+0),mm5);
    movq_r2r(mm1,mm2);
    movq_r2r(mm3,mm4);
    movq_r2r(mm5,mm6);
    punpcklbw_r2r(mm0,mm1);
    punpckhbw_r2r(mm0,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    punpcklbw_r2r(mm0,mm5);
    punpckhbw_r2r(mm0,mm6);
    paddusw_r2r(mm3,mm1);
    movq_m2r(m_(sp2+1),mm3);
    paddusw_r2r(mm4,mm2);
    movq_r2r(mm3,mm4);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    paddusw_r2r(mm5,mm3);
    paddusw_r2r(mm6,mm4);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    movq_m2r(m_(dp +0),mm3);
    paddusw_r2r(mm7,mm1);
    movq_r2r(mm3,mm4);
    paddusw_r2r(mm7,mm2);
    punpcklbw_r2r(mm0,mm3);
    punpckhbw_r2r(mm0,mm4);
    psrlw_i2r(2,mm1);
    psrlw_i2r(2,mm2);
    movq_m2r(m_(sadd1),mm5);
    paddusw_r2r(mm3,mm1);
    paddusw_r2r(mm4,mm2);
    paddusw_r2r(mm5,mm1);
    paddusw_r2r(mm5,mm2);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    packuswb_r2r(mm2,mm1);
    movq_r2m(mm1,m_(dp+0));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
  emms();
}



int main(int ac, char **av)
{
  int i, j, k, l, m, n, done;
  uint8_t dat0[32], dat1[32], dat2[32];
  for( m=0; m<14; ++m ) {
    done = 0;
    printf("m=%d\n",m);
    n = 16 - 8*(m&1);
    for( i=0; i<256 && !done ; ++i ) {
      for( j=0; j<256 && !done; j+=n ) {
        k = 0;
        for( k=0; k<256; k+=n ) {
          for( l=0; l<n; ++l ) {
            dat0[l] = i;
            dat1[l] = dat2[l] = j+l;
            dat0[l+16] = dat1[l+16] = dat2[l+16] = k;
          }
          switch( m ) {
          case 0:
            reca   (&dat0[0], &dat1[0], 16, 1);
            mreca  (&dat0[0], &dat2[0], 16, 1);
            break;
          case 1:
            recac  (&dat0[0], &dat1[0], 16, 1);
            mrecac (&dat0[0], &dat2[0], 16, 1);
            break;
          case 2:
            recv   (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrecv  (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 3:
            recvc  (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrecvc (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 4:
            recva  (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrecva (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 5:
            recvac (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrecvac(&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 6:
            rech   (&dat0[0], &dat1[0], 16, 1);
            mrech  (&dat0[0], &dat2[0], 16, 1);
            break;
          case 7:
            rechc  (&dat0[0], &dat1[0], 16, 1);
            mrechc (&dat0[0], &dat2[0], 16, 1);
            break;
          case 8:
            recha  (&dat0[0], &dat1[0], 16, 1);
            mrecha (&dat0[0], &dat2[0], 16, 1);
            break;
          case 9:
            rechac (&dat0[0], &dat1[0], 16, 1);
            mrechac(&dat0[0], &dat2[0], 16, 1);
            break;
          case 10:
            rec4   (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrec4  (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 11:
            rec4c  (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrec4c (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 12:
            rec4a  (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrec4a (&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          case 13:
            rec4ac (&dat0[0], &dat1[0], 0x10, 16, 1);
            mrec4ac(&dat0[0], &dat2[0], 0x10, 16, 1);
            break;
          }
        }
        if( (m&1) ) {
          if( *(uint64_t *)&dat1[0] != *(uint64_t *)&dat2[0] )
          {
            printf("k=%d\n",k);
            printf(" i=%5d %02x %02x %02x %02x  %02x %02x %02x %02x\n", i,
              dat1[0], dat1[1], dat1[2], dat1[3], dat1[4], dat1[5], dat1[6], dat1[7]);
            printf(" j=%5d %02x %02x %02x %02x  %02x %02x %02x %02x\n", j,
              dat2[0], dat2[1], dat2[2], dat2[3], dat2[4], dat2[5], dat2[6], dat2[7]);
            done = 1;
            break;
          }
        }
        else {
          if( *(uint64_t *)&dat1[0] != *(uint64_t *)&dat2[0] ||
              *(uint64_t *)&dat1[8] != *(uint64_t *)&dat2[8] )
          {
            printf("k=%d\n",k);
            printf(" i=%5d %02x %02x %02x %02x  %02x %02x %02x %02x", i,
              dat1[0], dat1[1], dat1[2], dat1[3], dat1[4], dat1[5], dat1[6], dat1[7]);
            printf(" %02x %02x %02x %02x  %02x %02x %02x %02x\n", dat1[8],
              dat1[9], dat1[10], dat1[11], dat1[12], dat1[13], dat1[14], dat1[15]);
            printf(" j=%5d %02x %02x %02x %02x  %02x %02x %02x %02x", j,
              dat2[0], dat2[1], dat2[2], dat2[3], dat2[4], dat2[5], dat2[6], dat2[7]);
            printf(" %02x %02x %02x %02x  %02x %02x %02x %02x\n", dat2[8],
              dat2[9], dat2[10], dat2[11], dat2[12], dat2[13], dat2[14], dat2[15]);
            done = 1;
            break;
          }
        }
      }
    }
  }
  return 0;
}

