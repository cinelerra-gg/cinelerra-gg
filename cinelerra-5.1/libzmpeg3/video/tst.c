#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
//#define MMX_TRACE
#include "mmx.h"

static inline int clip2(int v, int mn, int mx) {
  if( v > mx ) return mx;
  if( v < mn ) return mn;
  return v;
}

static inline uint8_t clip(int32_t v) {
  return ((uint8_t)v) == v ? v : clip2(v,0,255);
}

#define m_(v) (*(mmx_t*)(((long long *)(v))))

int main(int ac, char **av)
{
   int i, j, k, n;
   unsigned char dat0[8] = { 0x01, 0xf2, 0x03, 0x04, 0x05, 0x06, 0xf7, 0x08 };
   long long *datp = (long long *)&dat0;
   int16_t dat1[8] = { 0x10, 0x20, -0x130, -0x140, 0x50, -0x160, -0x170, 0x80 };
   volatile uint8_t *rfp = dat0;
   volatile int16_t *bp  = dat1;
   unsigned char ans1[8], ans2[8];

   n = 0;
   for( i=-32768; i<32768; ++i ) {
     j = 0;
     while( j < 256 ) {
        for( k=0; k<8; ++k ) {
          dat0[k] = i;
          dat1[k] = j++;
        }
       movq_m2r(m_(&rfp[0]),mm1);  /* rfp[0..7] */
       pxor_r2r(mm3,mm3);
       pxor_r2r(mm4,mm4);
       movq_m2r(m_(&bp[0]),mm5);   /* bp[0..3] */
       movq_r2r(mm1,mm2);
       movq_m2r(m_(&bp[4]),mm6);   /* bp[4..7] */
       punpcklbw_r2r(mm3,mm1);     /* rfp[0,2,4,6] */
       punpckhbw_r2r(mm3,mm2);     /* rfp[1,3,5,7] */
       paddsw_r2r(mm5,mm1);        /* bp[0..3] */
       paddsw_r2r(mm6,mm2);        /* bp[4..7] */
       pcmpgtw_r2r(mm1,mm3);
       pcmpgtw_r2r(mm2,mm4);
       pandn_r2r(mm1,mm3);
       pandn_r2r(mm2,mm4);
       packuswb_r2r(mm4,mm3);
       movq_r2m(mm3,m_(&ans1[0]));
       emms();

       ans2[0] = clip(bp[0] + rfp[0]);
       ans2[1] = clip(bp[1] + rfp[1]);
       ans2[2] = clip(bp[2] + rfp[2]);
       ans2[3] = clip(bp[3] + rfp[3]);
       ans2[4] = clip(bp[4] + rfp[4]);
       ans2[5] = clip(bp[5] + rfp[5]);
       ans2[6] = clip(bp[6] + rfp[6]);
       ans2[7] = clip(bp[7] + rfp[7]);

       if( *(uint64_t *)&ans1[0] != *(uint64_t *)&ans2[0] )
       {
         printf(" i=%5d %02x %02x %02x %02x  %02x %02x %02x %02x\n", i,
           ans1[0], ans1[1], ans1[2], ans1[3], ans1[4], ans1[5], ans1[6], ans1[7]);
         printf(" j=%5d %02x %02x %02x %02x  %02x %02x %02x %02x\n", j,
           ans2[0], ans2[1], ans2[2], ans2[3], ans2[4], ans2[5], ans2[6], ans2[7]);
       //  exit(0);
       }
       n += 8;
     }
   }

   printf("n=%d\n",n);
   return 0;
}
