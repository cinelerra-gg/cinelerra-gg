#include <stdio.h>
#include <math.h>
#include "bccolors.C"

//c++ -g -I../guicast test5.C ../guicast/x86_64/libguicast.a \
// -DHAVE_GL -DHAVE_XFT -I/usr/include/freetype2 -lGL -lX11 -lXext \
// -lXinerama -lXv -lpng  -lfontconfig -lfreetype -lXft -pthread

int main(int ac, char **av)
{
  int rgb2yuv = atoi(av[1]);
  int color = atoi(av[2]);
  int range = atoi(av[3]);

  YUV::yuv.yuv_set_colors(color, range);
  if( !rgb2yuv ) {
    for( int y=0; y<0x100; ++y ) {
      for( int u=0; u<0x100; ++u ) {
        for( int v=0; v<0x100; ++v ) {
          float fy = y/255.f, fu = u/255.f - 0.5f, fv = v/255.f - 0.5f;
          float fr, fg, fb;  YUV::yuv.yuv_to_rgb_f(fr,fg,fb, fy,fu,fv);
          if( fr<0 || fr>1 || fg<0 || fg>1 || fb<0 || fb>1 )
		continue;
          float yy, uu, vv;  YUV::yuv.rgb_to_yuv_f(fr,fg,fb, yy,uu,vv);
          float d = fabsf(fy-yy) + fabsf(fu-uu) + fabsf(fv-vv);
          int rr = fr*256, gg = fg*256, bb = fb*256;
          CLAMP(rr,0,255); CLAMP(gg,0,255); CLAMP(bb,0,255);
          printf("yuv 0x%02x 0x%02x 0x%02x =%f,%f,%f, "
                 "rgb 0x%02x 0x%02x 0x%02x =%f,%f,%f, "
                 "  %f,%f,%f, %f\n",
            y,u,v, fy,fu,fv, rr,gg,bb, fr,fg,fb, yy,uu,vv, d);
        }
      }
    }
  }
  else {
    for( int r=0; r<0x100; ++r ) {
      for( int g=0; g<0x100; ++g ) {
        for( int b=0; b<0x100; ++b ) {
          float fr = r/255.f, fg = g/255.f, fb = b/255.f;
          float fy, fu, fv;  YUV::yuv.rgb_to_yuv_f(fr,fg,fb, fy,fu,fv);
          if( fy<0 || fy>1 || fu<0 || fu>1 || fv<0 || fv>1 )
		continue;
          float rr, gg, bb;  YUV::yuv.yuv_to_rgb_f(rr,gg,bb, fy,fu,fv);
          float d = fabsf(fr-rr) + fabsf(fg-gg) + fabsf(fb-bb);
          int yy = fy*256, uu = fu*256, vv = fv*256;
          CLAMP(yy,0,255); CLAMP(uu,0,255); CLAMP(vv,0,255);
          printf("rgb 0x%02x 0x%02x 0x%02x =%f,%f,%f, "
                 "yuv 0x%02x 0x%02x 0x%02x =%f,%f,%f, "
                 "  %f,%f,%f, %f\n",
            r,g,b, fr,fg,fb, yy,uu,vv, fy,fu,fv, rr,gg,bb, d);
        }
      }
    }
  }
  return 0;
}

