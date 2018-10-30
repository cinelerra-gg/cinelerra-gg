#include "../libzmpeg3.h"

#ifdef __x86_64__
#define USE_MMX
#endif
#define MMX_ACCURATE

#ifdef USE_MMX
#include "mmx.h"
#if defined(__x86_64__)
#define m_(v) (*(mmx_t*)(v))
#else
#define m_(v) (*(char*)(v))
#endif
static uint32_t sadd1[2] = { 0x00010001, 0x00010001 };
static uint32_t sadd2[2] = { 0x00020002, 0x00020002 };
#ifndef MMX_ACCURATE
static uint32_t bmask[2] = { 0x7f7f7f7f, 0x7f7f7f7f };
static uint32_t badd1[2] = { 0x01010101, 0x01010101 };
#endif
#endif


static inline void rec(uint8_t *s, uint8_t *d, int lx2, int h)
{
  for( int j=0; j<h; ++j, s+=lx2, d+=lx2 ) {
#if 0
    d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
    d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
    d[8] = s[8]; d[9] = s[9]; d[10] = s[10]; d[11] = s[11];
    d[12] = s[12]; d[13] = s[13]; d[14] = s[14]; d[15] = s[15];
#else
    *(uint64_t*)(d+0) = *(uint64_t*)(s+0);
    *(uint64_t*)(d+8) = *(uint64_t*)(s+8);
#endif
  }
}


static inline void recc(uint8_t *s, uint8_t *d, int lx2, int h)
{
  for( int j=0; j<h; ++j, s+=lx2, d+=lx2) {
#if 0
    d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
    d[4] = s[4]; d[5] = s[5]; d[6] = s[6]; d[7] = s[7];
#else
    *(uint64_t*)d = *(uint64_t*)s;
#endif
  }
}

static inline void reca(uint8_t *s, uint8_t  *d, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm0);
    movq_m2r(m_(dp+0),mm1);
    movq_m2r(m_(sp+8),mm2);
    movq_m2r(m_(dp+8),mm3);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    psrlw_i2r(1,mm3);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    pand_r2r(mm6,mm2);
    pand_r2r(mm6,mm3);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm3,mm2);
    paddusb_r2r(mm7,mm0);
    paddusb_r2r(mm7,mm2);
    movq_r2m(mm0,m_(dp+0));
    movq_r2m(mm2,m_(dp+8));
    sp += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}

static inline void recac(uint8_t *s, uint8_t *d, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp),mm0);
    movq_m2r(m_(dp),mm1);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm7,mm0);
    movq_r2m(mm0,m_(dp));
    sp += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}

static inline void recv(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm0);
    movq_m2r(m_(sp2+0),mm1);
    movq_m2r(m_(sp +8),mm2);
    movq_m2r(m_(sp2+8),mm3);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    psrlw_i2r(1,mm3);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    pand_r2r(mm6,mm2);
    pand_r2r(mm6,mm3);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm3,mm2);
    paddusb_r2r(mm7,mm0);
    paddusb_r2r(mm7,mm2);
    movq_r2m(mm0,m_(dp+0));
    movq_r2m(mm2,m_(dp+8));
    sp += lx2;  sp2 += lx2 ; dp += lx2;
  }
#endif
  emms();
#endif
}

static inline void recvc(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp),mm0);
    movq_m2r(m_(sp2),mm1);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm7,mm0);
    movq_r2m(mm0,m_(dp));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}


static inline void recva(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp +0),mm0);
    movq_m2r(m_(sp2+0),mm1);
    movq_m2r(m_(sp +8),mm2);
    movq_m2r(m_(sp2+8),mm3);
    movq_m2r(m_(dp +0),mm4);
    movq_m2r(m_(dp +8),mm5);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    psrlw_i2r(1,mm3);
    psrlw_i2r(1,mm4);
    psrlw_i2r(1,mm5);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    pand_r2r(mm6,mm2);
    pand_r2r(mm6,mm3);
    pand_r2r(mm6,mm4);
    pand_r2r(mm6,mm5);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm3,mm2);
    paddusb_r2r(mm7,mm0);
    paddusb_r2r(mm7,mm2);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm2);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm2);
    paddusb_r2r(mm0,mm4);
    paddusb_r2r(mm2,mm5);
    paddusb_r2r(mm7,mm4);
    paddusb_r2r(mm7,mm5);
    movq_r2m(mm4,m_(dp+0));
    movq_r2m(mm5,m_(dp+8));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}


static inline void recvac(uint8_t *s, uint8_t *d, int lx,int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp),mm0);
    movq_m2r(m_(sp2),mm1);
    movq_m2r(m_(dp),mm4);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm4);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    pand_r2r(mm6,mm4);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm7,mm0);
    psrlw_i2r(1,mm0);
    pand_r2r(mm6,mm0);
    paddusb_r2r(mm0,mm4);
    paddusb_r2r(mm7,mm4);
    movq_r2m(mm4,m_(dp));
    sp += lx2;  sp2 += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}


static inline void rech(uint8_t *s, uint8_t *d, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm0);
    movq_m2r(m_(sp+1),mm1);
    movq_m2r(m_(sp+8),mm2);
    movq_m2r(m_(sp+9),mm3);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    psrlw_i2r(1,mm2);
    psrlw_i2r(1,mm3);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    pand_r2r(mm6,mm2);
    pand_r2r(mm6,mm3);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm3,mm2);
    paddusb_r2r(mm7,mm0);
    paddusb_r2r(mm7,mm2);
    movq_r2m(mm0,m_(dp+0));
    movq_r2m(mm2,m_(dp+8));
    sp += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}


static inline void rechc(uint8_t *s,uint8_t *d, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  for( int j=0; j<h; ++j ) {
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
#else
#ifdef MMX_ACCURATE
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  movq_m2r(m_(bmask),mm6);
  movq_m2r(m_(badd1),mm7);
  for( int j=0; j<h; ++j ) {
    movq_m2r(m_(sp+0),mm0);
    movq_m2r(m_(sp+1),mm1);
    psrlw_i2r(1,mm0);
    psrlw_i2r(1,mm1);
    pand_r2r(mm6,mm0);
    pand_r2r(mm6,mm1);
    paddusb_r2r(mm1,mm0);
    paddusb_r2r(mm7,mm0);
    movq_r2m(mm0,m_(dp+0));
    sp += lx2;  dp += lx2;
  }
#endif
  emms();
#endif
}

static inline void recha(uint8_t *s, uint8_t *d,int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}


static inline void rechac(uint8_t *s,uint8_t  *d, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s;
  uint32_t s1, s2;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd1),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}


static inline void rec4(uint8_t *s, uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}


static inline void rec4c(uint8_t *s,uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}


static inline void rec4a(uint8_t *s,uint8_t *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1, s2, s3, s4;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}


static inline void rec4ac(uint8_t *s,uint8_t  *d, int lx, int lx2, int h)
{
#ifndef USE_MMX
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  uint32_t s1,s2,s3,s4;
  for( int j=0; j<h; ++j ) {
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
#else
  uint8_t *dp=d, *sp=s, *sp2=s+lx;
  pxor_r2r(mm0,mm0);
  movq_m2r(m_(sadd2),mm7);
  for( int j=0; j<h; ++j ) {
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
#endif
}

//#define DO_STATS
#ifdef DO_STATS
static class stats {
  int totals[16];
public:
  stats() { for( int i=0; i<16; ++i ) totals[i] = 0; }
  ~stats() {
    for( int i=0; i<16; ++i ) {
      static const char *fn[16] = {
        "recc",  "rec",  "recac",  "reca",
        "recvc", "recv", "recvac", "recva",
        "rechc", "rech", "rechac", "recha",
        "rec4c", "rec4", "rec4ac", "rec4a"
      };
      printf("%-8s %d\n",fn[i],totals[i]);
    }
  }
  void incr(int i) { if( i>0 && i<16 ) ++totals[i]; }
} mstats;
#endif


inline void zvideo_t::
recon_comp(uint8_t *s, uint8_t *d, int lx, int lx2, int h, int type)
{
/* probably Accelerated functions */
  switch( type ) {
  case 0x3:  reca(s, d, lx2, h);       break;
  case 0x2:  recac(s, d, lx2, h);      break;
  case 0x1:  rec(s, d, lx2, h);        break;
  case 0x0:  recc(s, d, lx2, h);       break;
  case 0x7:  recva(s, d, lx, lx2, h);  break;
  case 0x6:  recvac(s, d, lx, lx2, h); break;
  case 0x5:  recv(s, d, lx, lx2, h);   break;
  case 0x4:  recvc(s, d, lx, lx2, h);  break;
  case 0x9:  rech(s, d, lx2, h);       break;
  case 0x8:  rechc(s, d, lx2, h);      break;
/* maybe Unaccelerated functions */
  case 0xb:  recha(s, d, lx2, h);      break;
  case 0xa:  rechac(s, d, lx2, h);     break;
  case 0xf:  rec4a(s, d, lx, lx2, h);  break;
  case 0xe:  rec4ac(s, d, lx, lx2, h); break;
  case 0xd:  rec4(s, d, lx, lx2, h);   break;
  case 0xc:  rec4c(s, d, lx, lx2, h);  break;
  }
#ifdef DO_STATS
  mstats.incr(type);
#endif
}

/*
  uint8_t *src[]; * prediction source buffer *
  int sfield;           * prediction source field number (0 or 1) *
  uint8_t *dst[]; * prediction destination buffer *
  int dfield;           * prediction destination field number (0 or 1)*
  int lx,lx2;           * horizontal offsets *
  int w,h;              * prediction block/sub-block width, height *
  int x,y;              * pixel co-ordinates of top-left sample in current MB *
  int dx,dy;            * horizontal, vertical motion vector *
  int addflag;          * add prediction error to prediction ? *
*/
void zvideo_t::
recon( uint8_t *src[], int sfield,
   uint8_t *dst[], int dfield, int lx, int lx2,
   int w, int h, int x, int y, int dx, int dy, int addflag)
{
/* validate parameters */
  int err = 0;
  int sofs = (y+(dy>>1))*lx + x+(dx>>1);
  int dofs = y*lx + x;
  if( sfield ) sofs += lx2 >> 1;
  if( dfield ) dofs += lx2 >> 1;
  if( sofs >= 0 && dofs >= 0 ) {
    int dsz = coded_picture_width * coded_picture_height;
    int ssz = dsz + 16*coded_picture_width + 16;
    int dlen = (h-1)*lx2 + w*8+8-1;
    int slen = dlen + (dy&1)*lx + (dx&1);
    if( sofs+slen >= ssz || dofs+dlen >= dsz )
      err = '+';
  }
  else
    err = '-';
  if( err ) {
    if( this->src->log_errs ) {
      zmsgs("err%c frm %dx%d @ %d,%d %dx%d dx=%d, dy=%d, sofs=%d, dofs=%d\n",
        err, coded_picture_width, coded_picture_height,
        x, y, 8+w*8, h, dx, dy, sofs, dofs);
    }
    return;
  }

/* half pel scaling */
  int type = ((dx & 1) << 3) | ((dy & 1) << 2) | w;
  if( addflag ) type |= 2; 

  recon_comp(src[0]+sofs, dst[0]+dofs, lx, lx2, h, type);  /* Y */

  if( chroma_format != cfmt_444 ) {
    lx >>= 1;   dx /= 2; 
    lx2 >>= 1;  w = 0; 
    x >>= 1; 

    if( chroma_format == cfmt_420 ) {
      h >>= 1;   dy /= 2; 
      y >>= 1; 
    }

    sofs = (y+(dy>>1))*lx + x+(dx>>1);
    dofs = y*lx + x;
    if( sfield ) sofs += lx2 >> 1;
    if( dfield ) dofs += lx2 >> 1;
    type = ((dx & 1) << 3) | ((dy & 1) << 2) | w;
    if( addflag ) type |= 2; 
  }

  recon_comp(src[1]+sofs, dst[1]+dofs, lx, lx2, h, type);  /* Cb */
  recon_comp(src[2]+sofs, dst[2]+dofs, lx, lx2, h, type);  /* Cr */
}

#define WIDTH 1

int zvideo_t::
reconstruct( int bx, int by, int mb_type, int motion_type,
             int PMV[2][2][2], int mv_field_sel[2][2], 
             int dmvector[2], int stwtype)
{
  int currentfield;
  uint8_t **predframe;
  int DMV[2][2];
  int stwtop, stwbot;

  stwtop = stwtype % 3; /* 0:temporal, 1:(spat+temp), 2:spatial */
  stwbot = stwtype / 3;

  if( (mb_type & slice_decoder_t::mb_FORWARD) || (pict_type == pic_type_P) ) {
    if( pict_struct == pics_FRAME_PICTURE ) {
      if( (motion_type == slice_decoder_t::mc_FRAME) ||
          !(mb_type & slice_decoder_t::mb_FORWARD) ) {
/* frame-based prediction */
        if( stwtop < 2 )
          recon(oldrefframe, 0, newframe, 0,
                coded_picture_width, coded_picture_width<<1,
                WIDTH, 8, bx, by, PMV[0][0][0], PMV[0][0][1], stwtop);

        if( stwbot < 2 )
          recon(oldrefframe, 1, newframe, 1,
                coded_picture_width, coded_picture_width<<1,
                WIDTH, 8, bx, by, PMV[0][0][0], PMV[0][0][1], stwbot);
      }
      else if(motion_type == slice_decoder_t::mc_FIELD) { /* field-based prediction */
/* top field prediction */
        if( stwtop < 2 )
          recon(oldrefframe, mv_field_sel[0][0], newframe, 0,
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, PMV[0][0][0], PMV[0][0][1]>>1, stwtop);

/* bottom field prediction */
        if( stwbot < 2 )
          recon(oldrefframe, mv_field_sel[1][0], newframe, 1,
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, PMV[1][0][0], PMV[1][0][1]>>1, stwbot);
      }
      else if( motion_type == slice_decoder_t::mc_DMV ) { 
/* dual prime prediction */
/* calculate derived motion vectors */
        calc_dmv(DMV, dmvector, PMV[0][0][0], PMV[0][0][1]>>1);

        if( stwtop < 2 ) {
/* predict top field from top field */
          recon(oldrefframe, 0, newframe, 0, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, PMV[0][0][0], PMV[0][0][1]>>1, 0);

/* predict and add to top field from bottom field */
          recon(oldrefframe, 1, newframe, 0, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, DMV[0][0], DMV[0][1], 1);
        }

        if( stwbot < 2 ) {
/* predict bottom field from bottom field */
          recon(oldrefframe, 1, newframe, 1, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, PMV[0][0][0], PMV[0][0][1]>>1, 0);

/* predict and add to bottom field from top field */
          recon(oldrefframe, 0, newframe, 1, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by>>1, DMV[1][0], DMV[1][1], 1);
        }
      }
      else if( src->log_errs ) {
/* invalid motion_type */
        zerrs("invalid motion_type 1 (%d)\n",motion_type);
      }
    }
    else {
/* pics_TOP_FIELD or pics_BOTTOM_FIELD */
/* field picture */
      currentfield = (pict_struct == pics_BOTTOM_FIELD);

/* determine which frame to use for prediction */
      if( (pict_type == pic_type_P) && secondfield &&
          (currentfield != mv_field_sel[0][0]) )
        predframe = refframe; /* same frame */
      else
        predframe = oldrefframe; /* previous frame */

      if( (motion_type == slice_decoder_t::mc_FIELD) ||
          !(mb_type & slice_decoder_t::mb_FORWARD) ) {
/* field-based prediction */
        if( stwtop < 2 )
          recon(predframe,mv_field_sel[0][0],newframe,0,
                coded_picture_width<<1,coded_picture_width<<1,
                WIDTH, 16, bx, by, PMV[0][0][0], PMV[0][0][1], stwtop);
      }
      else if(motion_type == slice_decoder_t::mc_16X8) {
        if( stwtop < 2 ) {
          recon(predframe, mv_field_sel[0][0], newframe, 0, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by, PMV[0][0][0], PMV[0][0][1], stwtop);

          /* determine which frame to use for lower half prediction */
          if( (pict_type == pic_type_P) && secondfield &&
              (currentfield != mv_field_sel[1][0]) )
            predframe = refframe; /* same frame */
          else
            predframe = oldrefframe; /* previous frame */

          recon(predframe, mv_field_sel[1][0], newframe, 0, 
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, by+8, PMV[1][0][0], PMV[1][0][1], stwtop);
        }
      }
      else if(motion_type == slice_decoder_t::mc_DMV) { /* dual prime prediction */
        if( secondfield )
          predframe = refframe; /* same frame */
        else
          predframe = oldrefframe; /* previous frame */

/* calculate derived motion vectors */
        calc_dmv(DMV, dmvector, PMV[0][0][0], PMV[0][0][1]);

/* predict from field of same parity */
        recon(oldrefframe, currentfield, newframe, 0, 
              coded_picture_width<<1, coded_picture_width<<1,
              WIDTH, 16, bx, by, PMV[0][0][0], PMV[0][0][1], 0);

/* predict from field of opposite parity */
        recon(predframe, !currentfield, newframe, 0, 
              coded_picture_width<<1, coded_picture_width<<1,
              WIDTH, 16, bx, by, DMV[0][0], DMV[0][1], 1);
      }
      else if( src->log_errs ) {
/* invalid motion_type */
        zerrs("invalid motion_type 2 (%d)\n",motion_type);
      }
    }
    stwtop = stwbot = 1;
  }

  if( (mb_type & slice_decoder_t::mb_BACKWARD) ) {
    if( pict_struct == pics_FRAME_PICTURE ) {
      if( motion_type == slice_decoder_t::mc_FRAME ) {
/* frame-based prediction */
        if( stwtop < 2 )
          recon(refframe, 0, newframe, 0, 
                coded_picture_width, coded_picture_width<<1,
                WIDTH, 8, bx, by, PMV[0][1][0], PMV[0][1][1], stwtop);
        if( stwbot < 2 )
          recon(refframe, 1, newframe, 1, 
                coded_picture_width, coded_picture_width<<1,
                WIDTH, 8, bx, by, PMV[0][1][0], PMV[0][1][1], stwbot);
      }
      else {           
/* field-based prediction */
/* top field prediction */
        if( stwtop < 2 )
          recon(refframe, mv_field_sel[0][1], newframe, 0,
                coded_picture_width<<1,coded_picture_width<<1,
                WIDTH, 8, bx, (by>>1), PMV[0][1][0], PMV[0][1][1]>>1, stwtop);
/* bottom field prediction */
        if( stwbot < 2 )
          recon(refframe, mv_field_sel[1][1], newframe, 1,
                coded_picture_width<<1, coded_picture_width<<1,
                WIDTH, 8, bx, (by>>1), PMV[1][1][0], PMV[1][1][1]>>1, stwbot);
      }
    }
    else {
/* pics_TOP_FIELD or pics_BOTTOM_FIELD */
/* field picture */
      if( motion_type == slice_decoder_t::mc_FIELD ) {
/* field-based prediction */
        recon(refframe, mv_field_sel[0][1], newframe, 0, 
              coded_picture_width<<1, coded_picture_width<<1,
              WIDTH, 16, bx, by, PMV[0][1][0], PMV[0][1][1], stwtop);
      }
      else if( motion_type==slice_decoder_t::mc_16X8 ) {
        recon(refframe, mv_field_sel[0][1], newframe, 0, 
              coded_picture_width<<1, coded_picture_width<<1,
              WIDTH, 8, bx, by, PMV[0][1][0], PMV[0][1][1], stwtop);

        recon(refframe, mv_field_sel[1][1], newframe, 0, 
              coded_picture_width<<1, coded_picture_width<<1,
              WIDTH, 8, bx, by+8, PMV[1][1][0], PMV[1][1][1], stwtop);
      }
      else if( src->log_errs ) {
/* invalid motion_type */
        zerrs("invalid motion_type 3 (%d)\n",motion_type);
      }
    }
  } /* mb_type & slice_decoder_t::mb_BACKWARD */
  return 0;
}


