#!/bin/python

# Retain python2 compatibility
from __future__ import print_function

base = {
  "rgb8": {
    "i8": {
      "r": " uint32_t in = *inp; int r = ((in>>6)&3)*0x55u, g = ((in>>3)&7)*0x24u, b = (in&7)*0x24u;",
      "w": " uint32_t ot = (r&0xc0u) | ((g>>2)&0x38u) | ((b>>5)&0x07u);\n" +
           " *out++ = ot;",
    },
    "i16": {
      "r": " uint32_t in = *inp; int r = ((in>>6)&3)*0x5555u, g = ((in>>3)&7)*0x2492u, b = (in&7)*0x2492u;",
      "w": " uint32_t ot = ((r>>8)&0xc0u) | ((g>>10) & 0x38u) >> 2) | ((b>>13)&0x07u);\n" +
           " *out++ = ot;",
    },
    "fp": {
      "r": " int in = *inp; float r = (in>>6)/3.f, g = ((in>>3)&0x07u)/7.f, b = (in&0x07u)/7.f;",
      "w": " int vr = clp(4,r), vg = clp(8,g), vb = clp(8,b);\n" +
           " *out++ = ((vr<<6)&0xc0u) | ((vg<<3)&0x38u) | (vb&0x7u);",
    },
  },
  "rgb565": {
    "i8": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " int r = (in>>8)&0xf8u, g = (in>>3)&0xfcu, b = (in&0x1fu)<<3;",
      "w": " uint32_t ot = ((r<<8) & 0xf800u) | ((g<<3) & 0x07e0u) | ((b>>3) & 0x001fu);\n" +
           " *(uint16_t*)out = ot; out += sizeof(uint16_t);",
    },
    "i16": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " int r = in&0xf800u, g = (in<<5)&0xfc00u, b = (in<<11)&0xf800u;",
      "w": " uint32_t ot = (r&0xf800u) | ((g>>5) & 0x07e0u) | ((b>>11) & 0x001fu);\n" +
           " *out++ = ot;",
    },
    "fp": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " float r = (in>>11)/31.f, g = ((in>>5)&0x3fu)/63.f, b = (in&0x1fu)/31.f;",
      "w": " uint32_t vr = clp(32,r), vg = clp(64,g), vb = clp(32,b);\n" +
           " *out++ = ((vr<<11)&0xf800u) | ((vg<<6)&0x07e0u) | (vb&0x001fu);",
    },
  },
  "rgb888": {
    "i8": {
      "r": " int r = *inp++, g = *inp++, b = *inp++;",
      "w": " *out++ = r; *out++ = g; *out++ = b;",
    },
    "i16": {
      "r": " int r = *inp++, g = *inp++, b = *inp++;\n" +
           " r = (r<<8) | r;  g = (g<<8) | g;  b = (b<<8) | b;",
      "w": " *out++ = r>>8; *out++ = g>>8; *out++ = b>>8;",
    },
    "fp": {
      "r": " float r = fclp(*inp++,256), g=fclp(*inp++,256), b = fclp(*inp++,256);",
      "w": " *out++ = clp(256,r); *out++ = clp(256,g); *out++ = clp(256,b);",
    },
  },
  "rgb161616": {
    "i8": {
      "r": " int r = *inp++>>8, g = *inp++>>8, b = *inp++>>8;",
      "w": " *out++ = (r<<8) | r;  *out++ = (g<<8) | g;  *out++ = (b<<8) | b;"
    },
    "i16": {
      "r": " int r = *inp++, g = *inp++, b = *inp++;",
      "w": " *out++ = r; *out++ = g; *out++ = b;",
    },
    "fp": {
      "r": " float r = fclp(*inp++,65536), g=fclp(*inp++,65536), b = fclp(*inp++,65536);",
      "w": " *out++ = clp(65536,r); *out++ = clp(65536,g); *out++ = clp(65536,b);",
    },
  },
  "rgbfloat": {
    "i8": {
      "r": " int r = clp(256,*inp++), g = clp(256,*inp++), b = clp(256,*inp++);",
      "w": " *out++ = fclp(r,256); *out++ = fclp(g,256); *out++ = fclp(b,256);",
    },
    "i16": {
      "r": " int r = clp(65536,*inp++), g = clp(65536,*inp++), b = clp(65536,*inp++);",
      "w": " *out++ = fclp(r,65536); *out++ = fclp(g,65536); *out++ = fclp(b,65536);",
    },
    "fp": {
      "r": " float r = *inp++, g=*inp++, b = *inp++;",
      "w": " *out++ = r; *out++ = g; *out++ = b;",
    },
  },

  "bgr565": {
    "i8": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " int b = (in>>8)&0xf8u, g = (in>>3)&0xfcu, r = (in&0x1fu)<<3;",
      "w": " uint32_t ot = ((b&0xf8u)<<8) | ((g&0xfcu)<<3) | ((r&0xf8u)>>3);\n" +
           " *(uint16_t*)out = ot; out += sizeof(uint16_t)/sizeof(*out);",
    },
    "i16": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " int b = in&0xf800u, g = (in<<5)&0xfc00u, r = (in<<11)&0xf800u;",
      "w": " uint32_t ot = (b&0xf800u) | ((g>>5) & 0x07e0u) << 3) | (r>>11);\n" +
           " *out++ = ot;",
    },
    "fp": {
      "r": " uint32_t in = *(uint16_t*)inp;\n" +
           " float b = (in>>11)/31.f, g = ((in>>5)&0x3fu)/63.f, r = (in&0x1fu)/31.f;",
      "w": " uint32_t vb = clp(32,b), vg = clp(64,g), vr = clp(32,r);\n" +
           " *out++ = ((vb<<11)&0xf800u) | ((vg<<6)&0x03e0u) | (vr&0x001fu);",
    },
  },
  "bgr888": {
    "i8": {
      "r": " int b = *inp++, g = *inp++, r = *inp++;",
      "w": " *out++ = b; *out++ = g; *out++ = r;",
    },
    "i16": {
      "r": " int b = *inp++, g = *inp++, r = *inp++;\n" +
           " b = (b<<8) | b;  g = (g<<8) | g;  r = (r<<8) | r;",
      "w": " *out++ = b>>8; *out++ = g>>8; *out++ = r>>8;",
    },
    "fp": {
      "r": " float b = fclp(*inp++,256), g=fclp(*inp++,256), r = fclp(*inp++,256);",
      "w": " *out++ = clp(256,b); *out++ = clp(256,g); *out++ = clp(256,r);",
    },
  },
  "bgr8888": {
    "i8": {
      "r": " int b = *inp++, g = *inp++, r = *inp++;",
      "w": " *out++ = b; *out++ = g; *out++ = r; ++out;",
    },
    "i16": {
      "r": " int b = *inp++, g = *inp++, r = *inp++;\n" +
           " b = (b<<8) | b;  g = (g<<8) | g;  r = (r<<8) | r;",
      "w": " *out++ = b>>8; *out++ = g>>8; *out++ = r>>8; ++out;",
    },
    "fp": {
      "r": " float b = fclp(*inp++,256), g=fclp(*inp++,256), r = fclp(*inp++,256);",
      "w": " *out++ = clp(256,b); *out++ = clp(256,g); *out++ = clp(256,r); ++out;",
    },
  },
  "bgr161616": {
    "i8": {
      "r": " int b = *inp++>>8, g = *inp++>>8, r = *inp++>>8;",
      "w": " *out++ = (r<<8) | r;  *out++ = (g<<8) | g;  *out++ = (b<<8) | b;"
    },
    "i16": {
      "r": " int b = *inp++, g = *inp++, r = *inp++;",
      "w": " *out++ = b; *out++ = g; *out++ = r;",
    },
    "fp": {
      "r": " float b = fclp(*inp++,65536), g=fclp(*inp++,65536), r = fclp(*inp++,65536);",
      "w": " *out++ = clp(65536,b); *out++ = clp(65536,g); *out++ = clp(65536,r);",
    },
  },
  "bgrfloat": {
    "i8": {
      "r": " int b = clp(256,*inp++), g = clp(256,*inp++), r = clp(256,*inp++);",
      "w": " *out++ = fclp(b,256); *out++ = fclp(g,256); *out++ = fclp(r,256);",
    },
    "i16": {
      "r": " int b = clp(65536,*inp++), g = clp(65536,*inp++), r = clp(65536,*inp++);",
      "w": " *out++ = fclp(b,65536); *out++ = fclp(g,65536); *out++ = fclp(r,65536);",
    },
    "fp": {
      "r": " float b = *inp++, g=*inp++, r = *inp++;",
      "w": " *out++ = b; *out++ = g; *out++ = r;",
    },
  },

  "yuv888": {
    "i8": {
      "r": " int32_t y = *inp++, u = *inp++, v = *inp++;",
      "w": " *out++ = y; *out++ = u; *out++ = v;",
    },
    "i16": {
      "r": " int32_t iy = *inp++, y = (iy<<8) | iy, u = *inp++<<8, v = *inp++<<8;",
      "w": " *out++ = y>>8; *out++ = u>>8; *out++ = v>>8;",
    },
  },
  "yuv161616": {
    "i8": {
      "r": " int32_t y = *inp++>>8, u = *inp++>>8, v = *inp++>>8;",
      "w": " *out++ = (y<<8) | y; *out++ = u<<8; *out++ = v<<8;",
    },
    "i16": {
      "r": " int32_t y = *inp++, u = *inp++, v = *inp++;",
      "w": " *out++ = y; *out++ = u; *out++ = v;",
    },
  },

  "yuyv8888": {
    "i8": {
      "r": " int32_t y = inp[(j&1)<<1], u = inp[1], v = inp[3];",
      "w": " if( !(j&1) ) { *out++ = y; *out = u; out[2] = v; }\n" +
           " else { *out++ = u; *out++= y; *out++ = v; }",
    },
    "i16": {
      "r": " int32_t iy = inp[(j&1)<<1], y = (iy<<8) | iy, u = inp[1]<<8, v = inp[3]<<8;",
      "w": " if( !(j&1) ) { *out++ = y>>8; *out = u>>8; out[2] = v>>8; }\n" +
           " else { *out++ = u>>8; *out++= y>>8; *out++ = v>>8; }",
    },
  },

  "uyvy8888": {
    "i8": {
      "r": " int32_t u = inp[0], y = inp[((j&1)<<1)+1], v = inp[2];",
      "w": " if( !(j&1) ) { *out++ = u; *out++ = y; *out++ = v; *out = y; }\n" +
           " else { *out++= y; }",
    },
    "i16": {
      "r": " int32_t u = inp[0]<<8, iy = inp[((j&1)<<1)+1], y = (iy<<8) | iy, v = inp[2]<<8;",
      "w": " if( !(j&1) ) { *out++ = u>>8; *out++ = y>>8; *out++ = v>>8; *out = y>>8; }\n" +
           " else { *out++= y>>8; }",
    },
  },

  "yuv10101010": {
    "i8": {
      "r": " uint32_t it = *(uint32_t*)inp;\n" +
           " int32_t y = (it>>24)&0xffu, u = (it>>14)&0xffu, v = (it>>4)&0xffu;",
      "w": " uint32_t ot = (y<<24) | (u<<14) | (v<<4);\n" +
           " *(uint32_t*)out = ot; out += sizeof(uint32_t)/sizeof(*out);",
    },
    "i16": {
      "r": " uint32_t it = *(uint32_t*)inp;\n" +
           " int32_t y = (it>>16)&0xffc0u, u = (it>>6)&0xffc0u, v = (it<<4)&0xffc0u;",
      "w": " uint32_t ot = ((y&0xffc0u)<<16) | ((u&0xffc0u)<<6) | ((v&0xffc0u)>>4);\n" +
           " *(uint32_t*)out = ot; out += sizeof(uint32_t)/sizeof(*out);",
    },
  },

  "vyu888": {
    "i8": {
      "r": " int32_t v = *inp++, y = *inp++, u = *inp++;",
      "w": " *out++ = v; *out++ = y; *out++ = u;",
    },
    "i16": {
      "r": " int32_t v = *inp++<<8, iy = *inp++, y = (iy<<8) | iy, u = *inp++<<8;",
      "w": " *out++ = v>>8; *out++ = y>>8; *out++ = u>>8;",
    },
  },

  "uyv888": {
    "i8": {
      "r": " int32_t u = *inp++, y = *inp++, v = *inp++;",
      "w": " *out++ = u; *out++ = y; *out++ = v;",
    },
    "i16": {
      "r": " int32_t u = *inp++<<8, iy = *inp++, y = (iy<<8) | iy, v = *inp++<<8;",
      "w": " *out++ = u>>8; *out++ = y>>8; *out++ = v>>8;",
    },
  },

  "yuv420p": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j/2] = u;  vop[j/2] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j/2] = u>>8;  vop[j/2] = v>>8;",
    },
  },
  "yuv420pi": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j/2] = u;  vop[j/2] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j/2] = u>>8;  vop[j/2] = v>>8;",
    },
  },

  "yuv422p": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j/2] = u;  vop[j/2] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j/2] = u>>8;  vop[j/2] = v>>8;",
    },
  },

  "yuv444p": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j] = u;  vop[j] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j] = u>>8;  vop[j] = v>>8;",
    },
  },

  "yuv411p": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j/4] = u;  vop[j/4] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j/4] = u>>8;  vop[j/4] = v>>8;",
    },
  },

  "yuv410p": {
    "i8": {
      "r": " int32_t y = *yip, u = *uip, v = *vip;",
      "w": " yop[j] = y;  uop[j/4] = u;  vop[j/4] = v;",
    },
    "i16": {
      "r": " int32_t iy = *yip, y = (iy<<8) | iy, u = *uip<<8, v = *vip<<8;",
      "w": " yop[j] = y>>8;  uop[j/4] = u>>8;  vop[j/4] = v>>8;",
    },
  },

  "rgbfltp": {
    "i8": {
      "r": " int r = clp(256,*rip++), g = clp(256,*gip++), b = clp(256,*bip++);",
      "w": " *rop++ = fclp(r,256); *gop++ = fclp(g,256); *bop++ = fclp(b,256);",
    },
    "i16": {
      "r": " int r = clp(65536,*rip++), g = clp(65536,*gip++), b = clp(65536,*bip++);",
      "w": " *rop++ = fclp(r,65536); *gop++ = fclp(g,65536); *bop++ = fclp(b,65536);",
    },
    "fp": {
      "r": " float r = *rip++, g = *gip++, b = *bip++;",
      "w": " *rop++ = r; *gop++ = g; *bop++ = b;",
    },
  },

  "gbrp": {
    "i8": {
      "r": " int g = *rip++, b = *gip++, r = *bip++;",
      "w": " *rop++ = g; *gop++ = b; *bop++ = r;",
    },
    "i16": {
      "r": " int ig = *rip++, g = (ig<<8) | ig, ib = *gip++, b = (ib<<8) | ib," +
           " ir = *bip++, r = (ir<<8) | ir;",
      "w": " *rop++ = g >> 8; *gop++ = b >> 8; *bop++ = r >> 8;",
    },
    "fp": {
      "r": " float g = *rip++/255.f, b = *gip++/255.f, r = *bip++/255.f;",
      "w": " *rop++ = clp(256,g); *gop++ = clp(256,b); *bop++ = clp(256,r);",
    },
  },

  "grey8": {
    "i8": {
      "r": " int32_t y = *inp++, u = 0x80, v = 0x80;",
      "w": " *out++ = y; (void)u; (void)v;",
    },
    "i16": {
      "r": " int32_t iy = *inp++, y = (iy<<8) | iy, u = 0x8000, v = 0x8000;",
      "w": " *out++ = y>>8; (void)u; (void)v;",
    },
  },

  "grey16": {
    "i8": {
      "r": " int32_t y = *inp++>>8, u = 0x80, v = 0x80;",
      "w": " *out++ = (y<<8) | y; (void)u; (void)v;",
    },
    "i16": {
      "r": " int32_t y = *inp++, u = 0x8000, v = 0x8000;",
      "w": " *out++ = y; (void)u; (void)v;",
    },
  },

  # alpha component
  "a8": {
    "i8": {
      "r": " z_int a = *inp++;",
      "w": " *out++ = a;",
    },
    "i16": {
      "r": " z_int a = *inp++; a = (a<<8) | a;",
      "w": " *out++ = a>>8;",
    },
    "fp": {
      "r": " z_float fa = fclp(*inp++,256);",
      "w": " *out++ = clp(256,a)",
    },
  },
  "a16": {
    "i8": {
      "r": " z_int a = *inp++>>8;",
      "w": " *out++ = (a<<8) | a;",
    },
    "i16": {
      "r": " z_int a = *inp++;",
      "w": " *out++ = a;",
    },
    "fp": {
      "r": " z_float fa = fclp(*inp++,65536);",
      "w": " *out++ = clp(65536,a);",
    },
  },
  "afp": {
    "i8": {
      "r": " z_int a = clp(256,*inp++);",
      "w": " *out++ = fclp(a,256);",
    },
    "i16": {
      "r": " z_int a = clp(65536,*inp++);",
      "w": " *out++ = fclp(a,65536);",
    },
    "fp": {
      "r": " z_float fa = *inp++;",
      "w": " *out++ = fa;",
    },
  },
  "afpp": {
    "i8": {
      "r": " z_int a = clp(256,*aip++);",
      "w": " *aop++ = fclp(a,256);",
    },
    "i16": {
      "r": " z_int a = clp(65536,*aip++);",
      "w": " *aop++ = fclp(a,65536);",
    },
    "fp": {
      "r": " z_float fa = *aip++;",
      "w": " *aop++ = fa;",
    },
  },
  # no src alpha blend
  "x8": {
    "i8": {
      "r": " ++inp;",
      "w": " *out++ = 0xff;",
    },
    "i16": {
      "r": " ++inp;",
      "w": " *out++ = 0xff;",
    },
    "fp": {
      "r": " ++inp;",
      "w": " *out++ = 0xff;",
    },
  },
  "x16": {
    "i8": {
      "r": " ++inp;",
      "w": " *out++ = 0xffff;",
    },
    "i16": {
      "r": " ++inp;",
      "w": " *out++ = 0xffff;",
    },
    "fp": {
      "r": " ++inp;",
      "w": " *out++ = 0xffff;",
    },
  },
  "xfp": {
    "i8": {
      "r": " ++inp;",
      "w": " *out++ = 1.f;",
    },
    "i16": {
      "r": " ++inp;",
      "w": " *out++ = 1.f;",
    },
    "fp": {
      "r": " ++inp;",
      "w": " *out++ = 1.f;",
    },
  },
  # alpha blend rgb/black, yuv/black, rgb/bg_color
  "brgb": {
    "i8": " r = r*a/0xffu; g = g*a/0xffu; b = b*a/0xffu;",
    "i16": " r = r*a/0xffffu; g = g*a/0xffffu; b = b*a/0xffffu;",
    "fp": " r *= fa; g *= fa; b *= fa;",
  },
  "byuv": {
    "i8": " z_int a1 = 0xffu-a;\n" +
          " y = y*a/0xffu; u = (u*a + 0x80u*a1)/0xffu; v = (v*a + 0x80u*a1)/0xffu;",
    "i16": " z_int a1 = 0xffffu-a;\n" +
           " y = y*a/0xffffu; u = (u*a + 0x8000u*a1)/0xffffu; v = (v*a + 0x8000u*a1)/0xffffu;",
  },
  "bbg": {
    "i8": " int a1 = 0xffu-a;\n" +
        " r = (r*a + bg_r*a1)/0xffu; g = (g*a + bg_g*a1)/0xffu; b = (b*a + bg_b*a1)/0xffu;",
    "i16": "int a1 = 0xffffu-a;\n" +
        " r = (r*a + bg_r*a1)/0xffffu; g = (g*a + bg_g*a1)/0xffffu; b = (b*a + bg_b*a1)/0xffffu;",
    "fp": " float a1 = 1-fa;\n" +
        " r = (r*fa + bg_r*a1); g = (g*fa + bg_g*a1); b = (b*fa + bg_b*a1);",
  },
}

cmodels = []
bcmodels = {}
layout = {}
dtype = { None: None }
special = {}
mx_bcmdl = -1

def add_cmodel(n, nm, typ=None, *args):
  global cmodels, bcmodels, layout, dtype, mx_bcmdl
  cmodels += [nm,]
  bcmodels[n] = nm
  if( n > mx_bcmdl ): mx_bcmdl = n
  dtype[nm] = typ
  layout[nm] = args

def specialize(fr_cmdl, to_cmdl, fn):
  global special
  special[(fr_cmdl, to_cmdl)] = fn

add_cmodel( 0, "bc_transparency")
add_cmodel( 1, "bc_compressed")

add_cmodel( 2, "bc_rgb8", "i8", "rgb8")
add_cmodel( 3, "bc_rgb565", "i8", "rgb565")
add_cmodel( 4, "bc_bgr565", "i8", "bgr565")
add_cmodel( 5, "bc_bgr888", "i8", "bgr888")
add_cmodel( 6, "bc_bgr8888", "i8", "bgr8888")

add_cmodel( 9, "bc_rgb888", "i8", "rgb888")
add_cmodel(10, "bc_rgba8888", "i8", "rgb888", "a8")
add_cmodel(20, "bc_argb8888", "i8", "a8", "rgb888")
add_cmodel(21, "bc_abgr8888", "i8", "a8", "bgr888")
add_cmodel(11, "bc_rgb161616", "i16", "rgb161616")
add_cmodel(12, "bc_rgba16161616", "i16", "rgb161616", "a16")
add_cmodel(13, "bc_yuv888", "i8", "yuv888")
add_cmodel(14, "bc_yuva8888", "i8", "yuv888", "a8")
add_cmodel(15, "bc_yuv161616", "i16", "yuv161616")
add_cmodel(16, "bc_yuva16161616", "i16", "yuv161616", "a16")
add_cmodel(35, "bc_ayuv16161616", "i16", "a16", "yuv161616")

add_cmodel(18, "bc_uvy422", "i8", "uyvy8888")
add_cmodel(19, "bc_yuv422", "i8", "yuyv8888")
add_cmodel(22, "bc_a8")
add_cmodel(23, "bc_a16")
add_cmodel(31, "bc_a_float")
add_cmodel(24, "bc_yuv101010", "i16", "yuv10101010")
add_cmodel(25, "bc_vyu888", "i8", "vyu888")
add_cmodel(26, "bc_uyva8888", "i8", "uyv888", "a8")
add_cmodel(29, "bc_rgb_float", "fp", "rgbfloat")
add_cmodel(30, "bc_rgba_float", "fp", "rgbfloat", "afp")

add_cmodel( 7, "bc_yuv420p", "i8", "yuv420p")
add_cmodel( 8, "bc_yuv422p", "i8", "yuv422p")
add_cmodel(27, "bc_yuv444p", "i8", "yuv444p")
add_cmodel(17, "bc_yuv411p", "i8", "yuv411p")
add_cmodel(28, "bc_yuv410p", "i8", "yuv410p")
add_cmodel(32, "bc_rgb_floatp", "fp", "rgbfltp")
add_cmodel(33, "bc_rgba_floatp", "fp", "rgbfltp", "afpp")
add_cmodel(34, "bc_yuv420pi", "i8", "yuv420pi")

add_cmodel(36, "bc_grey8", "i8", "grey8")
add_cmodel(37, "bc_grey16", "i16", "grey16")
add_cmodel(38, "bc_gbrp", "i8", "gbrp")

add_cmodel(39, "bc_rgbx8888", "i8", "rgb888", "x8")
add_cmodel(40, "bc_rgbx16161616", "i16", "rgb161616", "x16")
add_cmodel(41, "bc_yuvx8888", "i8", "yuv888", "x8")
add_cmodel(42, "bc_yuvx16161616", "i16", "yuv161616", "x16")
add_cmodel(43, "bc_rgbx_float", "fp", "rgbfloat", "xfp")

specialize("bc_rgba8888", "bc_transparency", "XFER_rgba8888_to_transparency")

ctype = {
  "i8": "uint8_t",
  "i16": "uint16_t",
  "fp": "float",
}

adata = {
  "i8": " z_int a=0xff;",
  "i16": " z_int a=0xffff;",
  "fp": " z_float fa=1;",
}

def has_alpha(nm):
  return nm in ["bc_rgba8888", "bc_argb8888", "bc_abgr8888", \
    "bc_rgba16161616", "bc_yuva8888", "bc_yuva16161616", "bc_ayuv16161616", \
    "bc_uyva8888", "bc_rgba_float", "bc_rgba_floatp",]

def has_bgcolor(fr_cmdl,to_cmdl):
  return fr_cmdl in ["bc_rgba8888"] and \
    to_cmdl in ["bc_rgba888", "bc_bgr565", "bc_rgb565", \
     "bc_bgr888", "bc_rgb888", "bc_bgr8888"]

def is_specialized(fr_cmdl,to_cmdl):
  global special
  return special.get((fr_cmdl, to_cmdl)) is not None

def is_rgb(nm):
  return nm in [ "bc_rgb8", "bc_rgb565", "bc_bgr565", \
    "bc_bgr888", "bc_bgr8888", "bc_rgb888", "bc_rgba8888", \
    "bc_argb8888", "bc_abgr8888", "bc_rgb", "bc_rgb161616", \
    "bc_rgba16161616", "bc_rgb_float", "bc_rgba_float", \
    "bc_rgb_floatp", "bc_rgba_floatp", "bc_gbrp", \
    "bc_rgbx8888", "bc_rgbx16161616", "bc_rgbx_float", ]

def is_yuv(nm):
  return nm in [ "bc_yuv888", "bc_yuva8888", "bc_yuv161616", \
    "bc_yuva16161616", "bc_ayuv16161616", "bc_yuv422", "bc_uvy422", "bc_yuv101010", \
    "bc_vyu888", "bc_uyva8888", "bc_yuv420p", "bc_yuv420pi", "bc_yuv422p", \
    "bc_yuv444p", "bc_yuv411p", "bc_yuv410p", "bc_grey8", "bc_grey16", \
    "bc_yuvx8888", "bc_yuvx16161616", ]

def is_planar(nm):
  return nm in [ "bc_yuv420p", "bc_yuv420pi", "bc_yuv422p", "bc_yuv444p", \
    "bc_yuv411p", "bc_yuv410p", "bc_rgb_floatp", "bc_rgba_floatp", "bc_gbrp", ]

def is_float(nm):
  return nm in [ "bc_rgb_float", "bc_rgba_float", "bc_rgbx_float", \
    "bc_rgb_floatp", "bc_rgba_floatp", ]

def gen_xfer_proto(fd, pfx, cls, fr_cmdl, to_cmdl):
  global dtype, ctype
  print("%svoid %sxfer_%s_to_%s" % (pfx, cls, fr_cmdl[3:], to_cmdl[3:]), end=' ', file=fd)
  ityp = dtype[fr_cmdl];  fr_typ = ctype[ityp];
  otyp = dtype[to_cmdl];  to_typ = ctype[otyp];
  print("(unsigned y0, unsigned y1)", end=' ', file=fd)

def gen_xfer_fn(fd, fr_cmdl, to_cmdl):
  global layout, dtype, adata
  ityp = dtype[fr_cmdl];  otyp = dtype[to_cmdl]
  if( ityp is None or otyp is None ): return
  # xfr fn header
  gen_xfer_proto(fd, "", class_qual, fr_cmdl, to_cmdl);
  # xfr fn body
  print("{", file=fd)
  # loops / pointer preload
  in_xfer = "flat" if not is_planar(fr_cmdl) else \
    fr_cmdl[3:] if is_yuv(fr_cmdl) else \
    "rgbp" if not has_alpha(fr_cmdl) else "rgbap"
  out_xfer = "flat" if not is_planar(to_cmdl) else \
    to_cmdl[3:] if is_yuv(to_cmdl) else \
    "rgbp" if not has_alpha(to_cmdl) else "rgbap"
  print(" xfer_%s_row_out(%s) xfer_%s_row_in(%s)" % (out_xfer, ctype[otyp], in_xfer, ctype[ityp]), file=fd)

  # load inp
  if( is_float(to_cmdl) and is_yuv(fr_cmdl) ):
    for ic in layout[fr_cmdl]: print("%s" % (base[ic][ityp]['r']), end=' ', file=fd)
    if( ityp == "i8" ):
      print("\n float r, g, b; YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v);", end=' ', file=fd)
    elif( ityp == "i16" ):
      print("\n float r, g, b; YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);", end=' ', file=fd)
    if( has_alpha(fr_cmdl) or has_alpha(to_cmdl) ):
      if( not has_alpha(fr_cmdl) ):
        print(" z_float fa = 1;", end=' ', file=fd)
      elif( ityp == "i8" ):
        print(" float fa = fclp(a,256);", end=' ', file=fd)
      elif( ityp == "i16" ):
        print(" float fa = fclp(a,65536);", end=' ', file=fd)
  else:
    for ic in layout[fr_cmdl]: print("%s" % (base[ic][otyp]['r']), end=' ', file=fd)
    if( has_alpha(to_cmdl) and not has_alpha(fr_cmdl) ):
      print("%s" % (adata[otyp]), end=' ', file=fd)
  print("", file=fd)
  # xfer
  if( is_rgb(fr_cmdl) and is_yuv(to_cmdl) ):
    if( otyp == "i8" ):
      print(" int32_t y, u, v;  YUV::yuv.rgb_to_yuv_8(r, g, b, y, u, v);", file=fd)
    elif( otyp == "i16" ):
      print(" int32_t y, u, v;  YUV::yuv.rgb_to_yuv_16(r, g, b, y, u, v);", file=fd)
  elif( is_yuv(fr_cmdl) and is_rgb(to_cmdl)):
    if( otyp == "i8" ):
      print(" int32_t r, g, b;  YUV::yuv.yuv_to_rgb_8(r, g, b, y, u, v);", file=fd)
    elif( otyp == "i16" ):
      print(" int32_t r, g, b;  YUV::yuv.yuv_to_rgb_16(r, g, b, y, u, v);", file=fd)
  # blend
  if( has_bgcolor(fr_cmdl,to_cmdl) ):
    print("%s" % (base["bbg"][otyp]), file=fd)
  elif( has_alpha(fr_cmdl) and not has_alpha(to_cmdl) ):
    if( is_rgb(to_cmdl) ):
      print("%s" % (base["brgb"][otyp]), file=fd)
    elif( is_yuv(to_cmdl) ):
      print("%s" % (base["byuv"][otyp]), file=fd)
  # store out
  for oc in layout[to_cmdl]:
    print("%s" % (base[oc][otyp]['w']), end=' ', file=fd)
  print("xfer_end", file=fd)
  print("}", file=fd)
  print("", file=fd)

# output code file
class_qual = "BC_Xfer::"
xfn = "xfer/xfer.h"
fd = open(xfn, "w")
xid = "".join([chr(x) if chr(x).isalnum() else '_' for x in range(256)])
xid = "__" + xfn.upper()[xfn.rfind("/")+1:].translate(xid) + "__"
print("#ifndef %s" % xid, file=fd)
print("#define %s" % xid, file=fd)
print("", file=fd)
xfd = open("bcxfer.h")
fd.write(xfd.read())
xfd.close()

for fr_cmdl in cmodels:
  ityp = dtype[fr_cmdl]
  for to_cmdl in cmodels:
    otyp = dtype[to_cmdl]
    if( is_specialized(fr_cmdl, to_cmdl) ):
      print("  void %s(unsigned y0, unsigned y1);" % (special[(fr_cmdl, to_cmdl)]), file=fd)
      continue
    if( ityp is None or otyp is None ): continue
    gen_xfer_proto(fd, "  ", "", fr_cmdl, to_cmdl);
    print(";", file=fd)
# end of class definition
print("};", file=fd)
print("", file=fd)
print("#endif", file=fd)
fd.close()
xfn = xfn[:xfn.rfind(".h")]

# xfer functions
for fr_cmdl in cmodels:
  fd = open(xfn + "_" + fr_cmdl + ".C", "w")
  print("#include \"xfer.h\"", file=fd)
  print("", file=fd)
  for to_cmdl in cmodels:
    gen_xfer_fn(fd, fr_cmdl, to_cmdl)
  fd.close()

fd = open(xfn + ".C", "w")
# transfer switch
print("#include \"xfer.h\"", file=fd)
print("", file=fd)
print("void %sxfer()" % class_qual, file=fd)
print("{", file=fd)
mx_no = mx_bcmdl + 1
print("  static xfer_fn xfns[%d][%d] = {" % (mx_no, mx_no), file=fd)
for fr_no in range(mx_no):
  fr_cmdl = bcmodels.get(fr_no)
  ityp = dtype[fr_cmdl]
  print("  { // %s" % (fr_cmdl.upper() if ityp else "None"), file=fd)
  n = 0
  for to_no in range(mx_no):
    to_cmdl = bcmodels.get(to_no)
    otyp = dtype[to_cmdl]
    xfn = special[(fr_cmdl, to_cmdl)] if( is_specialized(fr_cmdl, to_cmdl) ) else \
      "xfer_%s_to_%s" % (fr_cmdl[3:], to_cmdl[3:]) if ( ityp and otyp ) else None
    if( n > 72 ): print("", file=fd); n = 0
    if( n == 0 ): print("   ", end=' ', file=fd); n += 4
    fn = "&%s%s" % (class_qual, xfn) if( xfn ) else "0"
    print("%s, " % (fn), end=' ', file=fd)
    n += len(fn) + 3
  print("}, ", file=fd)
print("  }; ", file=fd)
print("  xfn = xfns[in_colormodel][out_colormodel];", file=fd)
print("  xfer_slices(out_w*out_h/0x80000+1);", file=fd)
print("}", file=fd)
print("", file=fd)
print("#include \"bcxfer.C\"", file=fd)
print("", file=fd)
fd.close()
