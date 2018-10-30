#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include "libzmpeg3.h"

/* use builtin data, or dynamically load font data from X instead */
//#define BUILTIN_FONT_DATA
/* generate static font data code */
//#define WRITE_FONT_DATA
#define MAX_SIZE 4

#ifdef WRITE_FONT_DATA
#undef BUILTIN_FONT_DATA
#endif

int zbitfont_t::font_refs = 0;


static inline int
font_index(int style, int pen_size, int italics, int size)
{
  if( pen_size > 0 ) --pen_size;
  return ((size*2 + italics)*2 + pen_size)*7 + style;
}

#if defined(BUILTIN_FONT_DATA)
/* using static builtin font data */

zbitfont_t *zmpeg3_t::
bitfont(int style,int pen_size,int italics,int size)
{
  int k = font_index(style,pen_size,italics,size);
  return zbitfont_t::fonts[k];
}

zbitfont_t *zwin_t::
chr_font(zchr_t *bp, int scale)
{
  int size = bp->font_size + scale-1;
  return bitfont(bp->font_style,bp->pen_size,bp->italics,size);
}

/* stubs */
void zbitfont_t::init_fonts() {}
void zbitfont_t::destroy_fonts() {}

#else
/* loading font data on the fly */

static int
init_xfont(zbitfont_t *font,char *font_name)
{
  const char *display_name = ":0";
  Display *display = XOpenDisplay(display_name);
  if( !display ) return 1;
  Font font_id = XLoadFont(display,font_name);
  int ret = 0;
  XFontStruct *font_struct = XQueryFont(display,font_id);
  if( !font_struct ) ret = 1;
  if( !ret ) {
    int row_first = font_struct->min_char_or_byte2;
    int row_last  = font_struct->max_char_or_byte2;
    int row_count  = row_last-row_first + 1;
    int col_first = font_struct->min_byte1;
    int col_last  = font_struct->max_byte1;
    XCharStruct *per_char = font_struct->per_char;
    XCharStruct *bounds = per_char == NULL ? NULL : &font_struct->max_bounds;
    int row_sz, sizeof_data;
    Screen *screen = DefaultScreenOfDisplay(display);
    int depth = DefaultDepthOfScreen(screen);
    Visual *visual = DefaultVisualOfScreen(screen);
    Window w = DefaultRootWindow(display);
    font->nch = (col_last-col_first+1) * (row_last-row_first+1);
    font->chrs = new zbitchar_t[font->nch];
    font->idx = font->idy = font->imy = 0;
    int inx = 0, iny = 0;
  
    XImage *image;
    Pixmap pixmap;
    int b1, b2, x, y, n;
    unsigned int width, height;
    GC gcb, gcw;
    XGCValues gcv;
  
    gcv.function = GXcopy;
    gcv.line_width = 1;
    gcv.line_style = LineSolid;
    gcv.font = font_id;
    n = GCFunction|GCForeground|GCLineWidth|GCLineStyle|GCFont;
    gcv.foreground = BlackPixelOfScreen(DefaultScreenOfDisplay(display));
    gcb = XCreateGC(display,w,n,&gcv);
    gcv.foreground = WhitePixelOfScreen(DefaultScreenOfDisplay(display));
    gcw = XCreateGC(display,w,n,&gcv);
  
    for( b1=col_first; b1<=col_last; ++b1 ) {
      for( b2=row_first; b2<=row_last; ++b2 ) {
        int i = (b1-col_first)*row_count + b2-row_first;
        zbitchar_t *cp = &font->chrs[i];
        XChar2b ch;
        ch.byte1 = b1;
        ch.byte2 = b2;
        cp->ch = (b1<<8) + b2;
        if( per_char != NULL )
          bounds = &per_char[i];
        cp->left = bounds->lbearing;
        cp->right = bounds->rbearing;
        cp->ascent = bounds->ascent;
        cp->decent = bounds->descent;
        width  = cp->right - cp->left;
        height = cp->ascent + cp->decent;
        if( cp->ascent > font->imy ) font->imy = cp->ascent;
        if( cp->decent > iny ) iny = cp->decent;
        row_sz = (width+7) / 8;
        sizeof_data = row_sz * height;
        if( sizeof_data > 0 ) {
          char *data = (char*)malloc(sizeof_data);
          if( width > 0 ) { font->idx += width;  ++inx; }
          memset(&data[0],0,sizeof_data);
          x = -cp->left;  y = cp->ascent;
          pixmap = XCreatePixmap(display,w,width,height,depth);
          XFillRectangle(display,pixmap,gcb,0,0,width,height);
          XDrawString16(display,pixmap,gcw,x,y,&ch,1);
          image = XCreateImage(display,visual,1,XYBitmap,0,
             data,width,height,8,row_sz);
          XGetSubImage(display,pixmap,0,0,width,height,1,
             XYPixmap,image,0,0);
          cp->bitmap = new uint8_t [sizeof_data];
          memcpy(&cp->bitmap[0],&data[0],sizeof_data);
          XFreePixmap(display,pixmap);
          XDestroyImage(image);
        }
        else
          cp->bitmap = 0;
      }
    }
    XFreeGC(display,gcb);
    XFreeGC(display,gcw);
    XFreeFont(display,font_struct);
    if( inx == 0 ) inx = 1;
    font->idx = (font->idx + inx-1) / inx;
    font->idy = font->imy + iny;
  }
  XCloseDisplay(display);
  return ret;
}

static zbitfont_t *
get_xfont(int style, int pen_size, int italics, int size)
{
  int serif = 0, prop = 0, casual = 0; // fst_default
  switch( style ) {
  case 1: serif = 1;  prop = 0;  casual = 0;  break; // fst_mono_serif
  case 2: serif = 1;  prop = 1;  casual = 0;  break; // fst_prop_serif
  case 3: serif = 0;  prop = 0;  casual = 0;  break; // fst_mono_sans
  case 4: serif = 0;  prop = 1;  casual = 0;  break; // fst_prop_sans
  case 5: serif = 1;  prop = 1;  casual = 1;  break; // fst_casual
  }

  const char *family, *slant;
#if 1
  if( casual ) {
    family = "adobe-utopia";
    slant = italics ? "i" : "r";
  }
  if( serif == 0 ) {
   family = prop == 0 ?
     (slant = italics ? "o" : "r", "urw-nimbus mono l") :
     (slant = italics ? "i" : "r", "urw-nimbus sans l");
  }
  else {
   family = prop == 0 ?
     (slant = italics ? "i" : "r", "bitstream-courier 10 pitch"):
     (slant = italics ? "i" : "r", "bitstream-bitstream charter");
  }
#else
  if( casual ) {
    family = "bitstream-bitstream vera serif";
    slant = "r";
  }
  if( serif == 0 ) {
   family = prop == 0 ?
     (slant = italics ? "o" : "r", "bitstream-bitstream vera sans mono") :
     (slant = italics ? "o" : "r", "bitstream-bitstream vera sans");
  }
  else {
   family = prop == 0 ?
     (slant = italics ? "i" : "r", "bitstream-courier 10 pitch"):
     (slant = italics ? "i" : "r", "bitstream-bitstream charter");
  }
#endif
  const char *wght = pen_size > 1 ? "bold" : "medium";

  // range expanded to account for scale
  if( size < 0 ) size = 0;
  if( size > MAX_SIZE ) size = MAX_SIZE;
  int ptsz = 180;
  switch( size ) {
  case 0: ptsz =  48;  break; // very fsz_small
  case 1: ptsz = 100;  break; // fsz_small
  default:  case 2:    break; // fsz_normal
  case 3: ptsz = 320;  break; // fsz_large
  case 4: ptsz = 420;  break; // very fsz_large
  };

  char name[512];
  sprintf(&name[0],"-%s-%s-%s-*-*-*-0-%d-%d-*-*-iso8859-1",
    family, wght, slant, ptsz, ptsz);
  zbitfont_t *font = new zbitfont_t();
  if( init_xfont(font,&name[0]) ) {
    delete font;  font = 0;
  }
  return font;
}

/* italics=2, pen_size=2, font_size=5, font_style=7 */
int zbitfont_t::total_fonts = 140;
zbitfont_t *zbitfont_t::fonts[140];

zbitfont_t *zmpeg3_t::
bitfont(int style,int pen_size,int italics,int size)
{
  int k = font_index(style,pen_size,italics,size);
  bitfont_t *font = bitfont_t::fonts[k];
  if( !font ) {
    font = get_xfont(style,pen_size,italics,size);
    bitfont_t::fonts[k] = font;
  }
  return font;
}

zbitfont_t *zwin_t::
chr_font(zchr_t *bp, int scale)
{
  int size = bp->font_size + scale-1;
  return bitfont(bp->font_style,bp->pen_size,bp->italics,size);
}

#if 0
void zbitfont_t::
init_fonts()
{
  if( !font_refs )
    for( int i=0; i<total_fonts; ++i ) fonts[i] = 0;
  ++font_refs;
}

void zbitfont_t::
destroy_fonts()
{
  if( font_refs > 0 ) --font_refs;
  if( font_refs > 0 ) return;
  for( int i=0; i<total_fonts; ++i )
    if( fonts[i] ) { delete fonts[i];  fonts[i] = 0; }
}
#else
void zbitfont_t::init_fonts() {}
void zbitfont_t::destroy_fonts() {}

zstatic_init_t::
static_init_t()
{
  for( int i=0; i<total_fonts; ++i ) fonts[i] = 0;
}
zstatic_init_t::
~static_init_t()
{
  for( int i=0; i<total_fonts; ++i )
    if( fonts[i] ) { delete fonts[i];  fonts[i] = 0; }
}

zstatic_init_t static_init;

#endif
#endif

#if defined(WRITE_FONT_DATA)
/* generate code for builtin font initialization */

static int
write_font(int fno, zbitfont_t *font)
{
  char font_var[512];
  
  sprintf(&font_var[0],"font%02d",fno);
  printf("#include \"libzmpeg3.h\"\n\n");
  zbitchar_t *cp = font->chrs;
  for( int i=0; i<font->nch; ++i,++cp ) {
    int sizeof_data = (cp->right-cp->left+7)/8 * (cp->ascent+cp->decent);
    if( sizeof_data > 0 ) {
      printf("static uint8_t %s_pix%04x[] = {\n",font_var,i);
      uint8_t *bp = cp->bitmap;
      int j = 0;
      while( j < sizeof_data ) {
        if( (j&7) == 0 ) printf("  ");
        printf("0x%02x,",*bp++);
        if( (++j&7) == 0 ) printf("\n");
      }
      if( (j&7) != 0 ) printf("\n");
      printf("};\n");
    }
  }
  printf("\n");

  printf("static zbitchar_t %s_chrs[] = {\n",font_var);
  cp = font->chrs;
  for( int i=0; i<font->nch; ++i,++cp ) {
    printf("  { 0x%04x, %2d, %2d, %2d, %2d, ",
      cp->ch,cp->left,cp->right,cp->ascent,cp->decent);
    int sizeof_data = (cp->right-cp->left+7)/8 * (cp->ascent+cp->decent);
    if( sizeof_data > 0 )
      printf("&%s_pix%04x[0] },\n",font_var,i);
    else
      printf("0 },\n");
  }
  printf("};\n");
  printf("\n");
  
  printf("static zbitfont_t %s = { &%s_chrs[0], %d, %d, %d, %d, };\n",
    font_var,font_var,font->nch,font->idx,font->idy,font->imy);
  printf("\n");
  return 0;
}

int main(int ac, char **av)
{
  int fno = 0;
  for( int size=0; size<=MAX_SIZE; ++size ) {
    for( int italics=0; italics<=1; ++italics ) {
      for( int pen_size=1; pen_size<=2; ++pen_size ) {
        for( int font_style=0; font_style<=6; ++font_style ) {
          zbitfont_t *font = get_xfont(font_style, pen_size, italics, size);
          write_font(++fno, font);
          delete font;
        }
      }
    }
  }
  fno = 0;
  printf("zbitfont_t *zbitfont_t::fonts[] = {\n");
  for( int size=0; size<=MAX_SIZE; ++size ) {
    for( int italics=0; italics<=1; ++italics ) {
      for( int pen_size=1; pen_size<=2; ++pen_size ) {
        for( int font_style=0; font_style<=6; ++font_style ) {
          if( (fno&7) == 0 ) printf("  ");
          printf("&font%02d,",++fno);
          if( (fno&7) == 0 ) printf("\n");
        }
      }
    }
  }
  if( (fno&7) != 0 ) printf("\n");
  printf("};\n");
  printf("\n");
  printf("int zbitfont_t::total_fonts = %d;\n",fno);
  printf("\n");
  return 0;
}

#endif

#if 0
/* attach to end of output for testing */
int main(int ac, char **av)
{
  int sz = 0;
  for( int i=0; i<zbitfont_t::total_fonts; ++i ) {
    zbitfont_t *font = zbitfont_t::fonts[i];
    for( int ch=0; ch<font->nch; ++ch ) {
      zbitchar_t *cp = &font->chrs[ch];
      sz += 16 + (cp->ascent+cp->decent)*((cp->right-cp->left+7)/8);
    }
    sz += 20;
  }
  printf(" %d bytes\n",sz);
  zbitchar_t *cp = &font84.chrs['y'];
  uint8_t *bmap = cp->bitmap;
  int wd = cp->right - cp->left;
  int ht = cp->ascent + cp->decent;
  int wsz = (wd+7) / 8;
  for( int iy=0; iy<ht; ++iy ) {
    uint8_t *row = &bmap[iy*wsz];
    for( int ix=0; ix<wd; ++ix ) {
      int ch = ((row[ix>>3] >> (ix&7)) & 1) ?  'x' : ' ';
      printf("%c",ch);
    }
    printf("\n");
  }
  return 0;
}
#endif
