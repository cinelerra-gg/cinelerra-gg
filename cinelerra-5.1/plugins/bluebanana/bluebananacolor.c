/*
 * Cinelerra :: Blue Banana - color modification plugin for Cinelerra-CV
 * Copyright (C) 2012-2013 Monty <monty@xiph.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

static inline void rgb8_to_RGB(unsigned char *row, float *R, float *G, float *B, int w){
  while(w--){
    *R++ = *row++*.0039215686f;
    *G++ = *row++*.0039215686f;
    *B++ = *row++*.0039215686f;
  }
}

static inline void rgba8_to_RGBA(unsigned char *row, float *R, float *G, float *B, float *A, int w){
  while(w--){
    *R++ = *row++*.0039215686f;
    *G++ = *row++*.0039215686f;
    *B++ = *row++*.0039215686f;
    *A++ = *row++*.0039215686f;
  }
}

static inline void rgbF_to_RGB(float *row, float *R, float *G, float *B, int w){
  while(w--){
    *R++ = *row++;
    *G++ = *row++;
    *B++ = *row++;
  }
}

static inline void rgbaF_to_RGBA(float *row, float *R, float *G, float *B, float *A, int w){
  while(w--){
    *R++ = *row++;
    *G++ = *row++;
    *B++ = *row++;
    *A++ = *row++;
  }
}


// Full swing and bt601 are chosen below to conform to existing Cinelerra convention.
// It's not correct, but it's consistent.

#if 1
// Full swing
#define Y_SWING  255.f
#define Y_SHIFT  0
#define C_SWING  255.f
#define C_SHIFT  128
#define CLAMP_LO 0
#define CLAMP_HI 255
#else
// Studio swing
#define Y_SWING  219.f
#define Y_SHIFT  16
#define C_SWING  224.f
#define C_SHIFT  128
#define CLAMP_LO 1
#define CLAMP_HI 254
#endif

#if 1
// bt601
#define Kr .299f
#define Kb .114f
#else
// bt709
#define Kr .2126f
#define Kb .0722f
#endif
#define Kg (1.f-Kr-Kb)

#define Br (.5f*-Kr/(1.f-Kb))
#define Bg (.5f*-Kg/(1.f-Kb))
#define Bb (.5f)

#define Rr (.5f)
#define Rg (.5f*-Kg/(1.f-Kr))
#define Rb (.5f*-Kb/(1.f-Kr))

#define Rv (2.f*(1.f-Kr))
#define Gu (2.f*(1.f-Kb)*Kb/Kg)
#define Gv (2.f*(1.f-Kr)*Kr/Kg)
#define Bu (2.f*(1.f-Kb))

static inline void yuv8_to_RGB(unsigned char *row, float *R, float *G, float *B, int w){
  while(w--){
    float y = (*row++-Y_SHIFT) * (1.f/Y_SWING);
    float u = (*row++-C_SHIFT);
    float v = (*row++-C_SHIFT);
    *R++ = y                  + (Rv/C_SWING)*v;
    *G++ = y - (Gu/C_SWING)*u - (Gv/C_SWING)*v;
    *B++ = y + (Bu/C_SWING)*u;
  }
}

static inline void yuva8_to_RGBA(unsigned char *row, float *R, float *G, float *B, float *A, int w){
  while(w--){
    float y = (*row++-Y_SHIFT) * (1.f/Y_SWING);
    float u = (*row++-C_SHIFT);
    float v = (*row++-C_SHIFT);
    *R++ = y                  + (Rv/C_SWING)*v;
    *G++ = y - (Gu/C_SWING)*u - (Gv/C_SWING)*v;
    *B++ = y + (Bu/C_SWING)*u;
    *A++ = *row++*(1.f/255.f);
  }
}

static inline void RGB_to_yuv8(float *R, float *G, float *B, float *S, float F, unsigned char *row, int w, int bpp){
  if(F>SELECT_THRESH){
    if(S){
      if(F<1.-SELECT_THRESH){
        while(w--){
          float s = *S++*F;
          float y = (Y_SHIFT + (Kr*Y_SWING)**R   + (Kg*Y_SWING)**G   + (Kb*Y_SWING)**B   - row[0])*s    + row[0] +.5f;
          float u = (C_SHIFT + (Br*C_SWING)**R   + (Bg*C_SWING)**G   + (Bb*C_SWING)**B   - row[1])*s    + row[1] +.5f;
          float v = (C_SHIFT + (Rr*C_SWING)**R++ + (Rg*C_SWING)**G++ + (Rb*C_SWING)**B++ - row[2])*s    + row[2] +.5f;
          row[0] = CLAMP(y,CLAMP_LO,CLAMP_HI);
          row[1] = CLAMP(u,CLAMP_LO,CLAMP_HI);
          row[2] = CLAMP(v,CLAMP_LO,CLAMP_HI);
          row+=bpp;
        }
      }else{
        while(w--){
          float y = (Y_SHIFT + (Kr*Y_SWING)**R   + (Kg*Y_SWING)**G   + (Kb*Y_SWING)**B   - row[0])**S   + row[0] +.5f;
          float u = (C_SHIFT + (Br*C_SWING)**R   + (Bg*C_SWING)**G   + (Bb*C_SWING)**B   - row[1])**S   + row[1] +.5f;
          float v = (C_SHIFT + (Rr*C_SWING)**R++ + (Rg*C_SWING)**G++ + (Rb*C_SWING)**B++ - row[2])**S++ + row[2] +.5f;
          row[0] = CLAMP(y,CLAMP_LO,CLAMP_HI);
          row[1] = CLAMP(u,CLAMP_LO,CLAMP_HI);
          row[2] = CLAMP(v,CLAMP_LO,CLAMP_HI);
          row+=bpp;
        }
      }
    }else{
      if(F<1.-SELECT_THRESH){
        while(w--){
          float y = (Y_SHIFT + (Kr*Y_SWING)**R   + (Kg*Y_SWING)**G   + (Kb*Y_SWING)**B   - row[0])*F    + row[0] +.5f;
          float u = (C_SHIFT + (Br*C_SWING)**R   + (Bg*C_SWING)**G   + (Bb*C_SWING)**B   - row[1])*F    + row[1] +.5f;
          float v = (C_SHIFT + (Rr*C_SWING)**R++ + (Rg*C_SWING)**G++ + (Rb*C_SWING)**B++ - row[2])*F    + row[2] +.5f;
          row[0] = CLAMP(y,CLAMP_LO,CLAMP_HI);
          row[1] = CLAMP(u,CLAMP_LO,CLAMP_HI);
          row[2] = CLAMP(v,CLAMP_LO,CLAMP_HI);
          row+=bpp;
        }
      }else{
        while(w--){
          float y = (Y_SHIFT+.5f) + (Kr*Y_SWING)**R   + (Kg*Y_SWING)**G   + (Kb*Y_SWING)**B  ;
          float u = (C_SHIFT+.5f) + (Br*C_SWING)**R   + (Bg*C_SWING)**G   + (Bb*C_SWING)**B  ;
          float v = (C_SHIFT+.5f) + (Rr*C_SWING)**R++ + (Rg*C_SWING)**G++ + (Rb*C_SWING)**B++;
          row[0] = CLAMP(y,CLAMP_LO,CLAMP_HI);
          row[1] = CLAMP(u,CLAMP_LO,CLAMP_HI);
          row[2] = CLAMP(v,CLAMP_LO,CLAMP_HI);
          row+=bpp;
        }
      }
    }
  }
}

static inline void RGB_to_rgb8(float *R, float *G, float *B, float *S, float F, unsigned char *row, int w, int bpp){
  if(F>SELECT_THRESH){
    if(S){
      if(F<1.-SELECT_THRESH){
        while(w--){
          float s = *S++*F;
          float r = (*R++*255.f-row[0])* s   +row[0] +.5f;
          float g = (*G++*255.f-row[1])* s   +row[1] +.5f;
          float b = (*B++*255.f-row[2])* s   +row[2] +.5f;
          row[0] = CLAMP(r,0,255);
          row[1] = CLAMP(g,0,255);
          row[2] = CLAMP(b,0,255);
          row+=bpp;
        }


      }else{
        while(w--){
          float r = (*R++*255.f-row[0])* *S   +row[0] +.5f;
          float g = (*G++*255.f-row[1])* *S   +row[1] +.5f;
          float b = (*B++*255.f-row[2])* *S++ +row[2] +.5f;
          row[0] = CLAMP(r,0,255);
          row[1] = CLAMP(g,0,255);
          row[2] = CLAMP(b,0,255);
          row+=bpp;
        }
      }
    }else{
      if(F<1.-SELECT_THRESH){
        while(w--){
          float r = (*R++*255.f-row[0])* F   +row[0] +.5f;
          float g = (*G++*255.f-row[1])* F   +row[1] +.5f;
          float b = (*B++*255.f-row[2])* F   +row[2] +.5f;
          row[0] = CLAMP(r,0,255);
          row[1] = CLAMP(g,0,255);
          row[2] = CLAMP(b,0,255);
          row+=bpp;
        }
      }else{
        while(w--){
          float r = *R++*255.f +.5f;
          float g = *G++*255.f +.5f;
          float b = *B++*255.f +.5f;
          row[0] = CLAMP(r,0,255);
          row[1] = CLAMP(g,0,255);
          row[2] = CLAMP(b,0,255);
          row+=bpp;
        }
      }
    }
  }
}

static inline void Aal_to_alp8(float *S, float F, unsigned char *row, int w, int bpp){
  if(S){
    while(w--){
      float a = *S*F*255.f +.5f;
      row[3] = CLAMP(a,0,255);
      row+=bpp;  ++S;
    }
  }else{
    float a = F*255.f +.5f;
    unsigned char s = CLAMP(a,0,255);
    while(w--){ row[3] = s; row+=bpp; }
  }
}


static inline void RGB_to_rgbF(float *R, float *G, float *B, float *S, float F, float *row, int w, int bpp){
  if(F>SELECT_THRESH){
    if(S){
      if(F<1.-SELECT_THRESH){
        while(w--){
          float s = *S++*F;
          row[0] = (*R++-row[0])* s   +row[0];
          row[1] = (*G++-row[1])* s   +row[1];
          row[2] = (*B++-row[2])* s   +row[2];
          row+=bpp;
        }
      }else{
        while(w--){
          row[0] = (*R++-row[0])* *S   +row[0];
          row[1] = (*G++-row[1])* *S   +row[1];
          row[2] = (*B++-row[2])* *S++ +row[2];
          row+=bpp;
        }
      }
    }else{
      if(F<1.-SELECT_THRESH){
        while(w--){
          row[0] = (*R++-row[0])* F   +row[0];
          row[1] = (*G++-row[1])* F   +row[1];
          row[2] = (*B++-row[2])* F   +row[2];
          row+=bpp;
        }
      }else{
        while(w--){
          row[0] = *R++;
          row[1] = *G++;
          row[2] = *B++;
          row+=bpp;
        }
      }
    }
  }
}

static inline void Aal_to_alpF(float *S, float F, float *row, int w, int bpp){
  if(S){
    while(w--){ row[3] = *S++ * F; row+=bpp; }
  }else{
    float a = F;
    while(w--){ row[3] = a; row+=bpp; }
  }
}


static inline void unmask_rgba8(unsigned char *row,int w){
  while(w--){
    row[3] = 255;
    row+=4;
  }
}

static inline void unmask_rgbaF(float *row, int w){
  while(w--){
    row[3] = 1.f;
    row+=4;
  }
}

static inline void unmask_yuva8(unsigned char *row, int w){
  while(w--){
    row[3] = 255;
    row+=4;
  }
}

// A modified HSV colorspace: saturation is calculated using |V| + a
// fixed bias to shift the absolute blackpoint well below the in-use
// color range.  This eliminates the singularity and desaturates
// values near the functional blackpoint (zero).  The modifications
// also allows us to use HSV with footroom without clipping chroma
// when V<=0.

// H is constrained to the range 0 to 1.
// S is strictly >=0 but the upper range is technically unbounded.
//   It is reasonable to clip to the reference range [0:1]
// V is unbounded and may extend considerably below 0 and above 1 depending on foot and head room

/* Simple mods of the original Cinelerra conversion routines */
static inline void RGB_to_HSpV(float R, float G, float B, float &H, float &Sp, float &V){
  if(R<G){
    if(B<R){
      V = G;
      H = 2 + (B-R) / (G-B);
      Sp = (G-B)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
    }else{
      if(B<G){
        V = G;
        H = 2 + (B-R) / (G-R);
        Sp = (G-R)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
      }else{
        V = B;
        H = 4 + (R-G) / (B-R);
        Sp = (B-R)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
      }
    }
  }else{
    if(B<G){
      V = R;
      H = (G-B) / (R-B);
      Sp = (R-B)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
    }else{
      if(B<R){
        V = R;
        H = 6 + (G-B) / (R-G);
        Sp = (R-G)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
      }else{
        V = B;
        H = 4 + (R-G) / (B-G+.0001);
        Sp = (B-G)/(fabs(V)+HSpV_SATURATION_BIAS)*HSpV_SATURATION_SCALE;
      }
    }
  }
}

static inline void HSpV_to_RGB(float H, float Sp, float V, float &R, float &G, float &B){
  float vp = (fabs(V)+HSpV_SATURATION_BIAS)*Sp*HSpV_SATURATION_ISCALE;
  int i = (int)H;
  switch(i){
  default:
    R = V;
    G = V + (H-i-1)*vp;
    B = V - vp;
    return;
  case 1:
    R = V - (H-i)*vp;
    G = V;
    B = V - vp;
    return;
  case 2:
    R = V - vp;
    G = V;
    B = V + (H-i-1)*vp;
    return;
  case 3:
    R = V - vp;
    G = V - (H-i)*vp;
    B = V;
    return;
  case 4:
    R = V + (H-i-1)*vp;
    G = V - vp;
    B = V;
    return;
  case 5:
    R = V;
    G = V - vp;
    B = V - (H-i)*vp;
    return;
  }
}

static inline void HSpV_correct_RGB(float H, float Sp, float &V, float &R, float &G, float &B){
  float vp = Sp<0.f?0.f:(fabs(V)+HSpV_SATURATION_BIAS)*Sp*HSpV_SATURATION_ISCALE;
#define R_TO_Y (Kr)
#define G_TO_Y (Kg)
#define B_TO_Y (Kb)

  int i = (int)H;
  switch(i){
  default:
    R = V = R*R_TO_Y + (G-(H-i-1)*vp)*G_TO_Y + (B+vp)*B_TO_Y;
    G = V + (H-i-1)*vp;
    B = V - vp;
    return;
  case 1:
    G = V = (R+(H-i)*vp)*R_TO_Y + G*G_TO_Y + (B+vp)*B_TO_Y;
    R = V - (H-i)*vp;
    B = V - vp;
    return;
  case 2:
    G = V = (R+vp)*R_TO_Y + G*G_TO_Y + (B-(H-i-1)*vp)*B_TO_Y;
    R = V - vp;
    B = V + (H-i-1)*vp;
    return;
  case 3:
    B = V = (R+vp)*R_TO_Y + (G+(H-i)*vp)*G_TO_Y + B*B_TO_Y;
    R = V - vp;
    G = V - (H-i)*vp;
    return;
  case 4:
    B = V = (R-(H-i-1)*vp)*R_TO_Y + (G+vp)*G_TO_Y + B*B_TO_Y;
    R = V + (H-i-1)*vp;
    G = V - vp;
    return;
  case 5:
    R = V = R*R_TO_Y + (G+vp)*G_TO_Y + (B+(H-i)*vp)*B_TO_Y;
    G = V - vp;
    B = V - (H-i)*vp;
    return;
  }
}
