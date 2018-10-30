#include "../libzmpeg3.h"

zcc_t::
cc_t(video_t *video)
 : svc(this)
{
  this->video = video;
  reset();
}

zcc_t::
~cc_t()
{
}

void zcc_t::
reset()
{
//zmsg(" cc\n");
  frs_history = 0;
  text_cb = 0;
  font_size = fsz_normal;
  video->reset_subtitles();
  int sid = video->subtitle_track+1;
  if( sid <= 0 || sid > MX_SVC ) sid = 1;
  svc.reset(sid, this);
}

void zcc_t::
get_atsc_data(zbits_t *s)
{
  /* xfer data to data[0], first byte = total size */
  /*   <dat[1]=isz,dat[2..isz+1]=input, ...,0> */
  int b = s->get_bits(8) & 0x1f;
  int len = b & 0x1f;
  uint8_t *dat = &data[0][0];
  dat[0] = 0;
  uint8_t *bp = dat+1;
  uint8_t *cp = bp+1;
//zmsgs(" len=%d\n",len);
  s->get_bits(8); /* rsvd */

  for( int i=0; i<len; ++i ) {
    int typ = s->get_bits(8);
    if( (typ & 0xf8) != 0xf8 ) return;
    int valid = typ & 0x04;
    int ch1 = s->get_bits(8);
    int ch2 = s->get_bits(8);
//zmsgs("*** typ %02x  %02x %02x\n",typ,ch1,ch2);
    int isz = cp-bp - 1;
    switch( typ &= 0x03 ) {
    case 0x02: /* dvb_data */
      if( isz == 0 ) continue; /* missed start */
      break;
    case 0x03: /* dvb_start */
      if( isz > 0 ) { *bp = isz;  bp = cp++; }
      break;
    case 0x00: /* ntsc_f1 */
    case 0x01: /* ntsc_f2 */
      continue;
    }
    if( !valid ) continue;
    if( cp-dat+2 > 128 ) {
      zerr("closed caption data ovfl\n");
      return;
    }
    *cp++ = ch1;
    *cp++ = ch2;
  }
  /* terminate last recd */
  if( (*bp = cp-bp - 1) > 0 ) *cp++ = 0;
  /* dat[0] = total size */
  dat[0] = cp - dat;
//zmsgs("***** size %d\n",dat[0]);
  if( s->get_bits(8) == 0xff ) decode();
}

void zcc_t::
decode()
{
  int isz = *data[0];
//zmsgs("seq %d, isz=%d, pict_type %c, pict_struct %3.3s %d\n",
//  data[0][1]>>6,isz," IPBD"[video->pict_type],
//  &"    TOP BTM FRM "[4*video->pict_struct],video->framenum);
  switch( video->pict_type ) {
  case pic_type_I:
  case pic_type_P:
    switch( video->pict_struct ) {
    case pics_TOP_FIELD:
      if( (frs_history & (frs_tf1 | frs_frm)) ) {
        decode(1);
        frs_history &= ~(frs_tf1 | frs_frm);
      }
      memcpy(&data[1],&data[0],isz);
      frs_history |= frs_tf1;
      break;
    case pics_BOTTOM_FIELD:
      if( (frs_history & frs_frm) ) {
        decode(1);
        frs_history &= ~frs_frm;
      }
      else if( (frs_history & frs_bf2) ) {
        decode(2);
        frs_history &= ~frs_bf2;
      }
      memcpy(&data[2],&data[1],isz);
      frs_history |= frs_bf2;
      break;
    case pics_FRAME_PICTURE:
      if( (frs_history & (frs_tf1 | frs_frm)) ) {
        decode(1);
        frs_history &= ~(frs_tf1 | frs_frm);
      }
      if( frs_history & frs_bf2 ) {
        decode(2);
        frs_history &= ~frs_bf2;
      }
      memcpy(&data[1],&data[0],isz);
      frs_history = frs_frm;
      break;
    }
    break;
  case pic_type_B:
    if( frs_history == frs_tf1 || frs_history == frs_bf2 )
      frs_history = 0;
    decode(0);
    break;
  }
  frs_history |= frs_prc;
  *data[0] = 0;
}

void zcc_t::
reorder()
{
  if( frs_history ) {
    if( !(frs_history & frs_prc) )
      decode();
    frs_history &= ~frs_prc;
  }
}

void zcc_t::
decode(int k)
{
  int isz;
  uint8_t *dat = &data[k][1];
  while( (isz = *dat++) > 0 ) {
    uint8_t *bp = dat;
    uint8_t b = *bp;
    int sz = 2 * (b & 0x03f);
#if 0
    int seq = b >> 6;
    zmsgs("cc decode seq %d size %d/%d -",seq,isz,sz);
    int n = isz;
    for( int i=0; i<n; ++i ) printf(" %02x",dat[i]);
    printf("  ");
    for( int i=0; i<n; ++i ) printf(" %c",dat[i]>=0x20&&dat[i]<0x7f?dat[i]:'.');
    printf("\n");
#endif
    if( sz > isz ) return;
    uint8_t *ep = sz + bp++;
    while( bp < ep ) {
      b = *bp++;
      if( !b ) continue;
      int svc_nr = b >> 5, blksz = b & 0x1f;
      if( svc_nr == 0x07 ) {
        if( bp >= ep ) break;
        svc_nr = *bp++;
        if( svc_nr < 7 || svc_nr > 63 ) break;
      }
      if( svc_nr == svc.id() && bp+blksz <= ep )
        svc.append(bp,blksz);
      bp += blksz;
    }
    dat += isz;
  }
  svc.decode();
}

zsvc_t::
svc_t(zcc_t *cc)
{
  size = 0;
  bfrs = 0;
  sid = 1;
  this->cc = cc;
};

zsvc_t::
~svc_t()
{
  reset(sid, cc);
}

void zsvc_t::
reset(int sid, zcc_t *cc)
{
//zmsgs(" svc %d\n",sid);
  size = 0;
  this->sid = sid;
  int ctrk = sid-1;
  if( ctrk < 0 ) ctrk = MX_SVC-1;
  this->ctrk = ctrk;
  this->cc = cc;
  for( int sid=0; sid<MX_WIN; ++sid )
    win[sid].reset(sid, this);
  for( chr_t *bfr; (bfr=get_bfr()); ) delete [] bfr;
//  win[0].default_window();
//  curw = &win[0];
  curw = 0;
}

zchr_t *zsvc_t::
get_bfr()
{
  chr_t *bfr = bfrs;
  if( bfr ) bfrs = *(chr_t **)bfr;
  return bfr;
}

void zsvc_t::
bfr_put(chr_t *&bfr)
{
  if( !bfr ) return;
  *(chr_t **)bfr = bfrs;
  bfrs = bfr;
  bfr = 0;
}

void zsvc_t::
append(uint8_t *bp, int len)
{
  int n = size+len;
  if( n > isizeof(data) ) n = isizeof(data);
  for( int i=size; i<n; data[i++]=*bp++);
  size = n;
}

void zsvc_t::
decode()
{
  if( size == 0 ) return;
  bp = data;  ep = bp+size;  size = 0;
//zmsgs(" size=%d,sid=%d\n",(int)(ep-bp),sid);
  while( bp < ep ) {
    int b = *bp++;
    if( (b&0x7f) >= 0x20 ) { // group G0/G1 ascii/latin chars
      if( curw && curw->st > st_unmap ) curw->store(b);
      continue;
    }
    if( b < 0x10 || b >= 0x80 ) { // group C0/C1
      if( command(b) ) break;
      continue;
    }
    if( b == 0x10 ) {  // ETX1 escape C2/C3, G2/G3
      if( bp >= ep ) break;
      b = *bp++;       // no valid codes, just skip rsvd bytes
      if( b < 0x08 ) continue;                 // 0x00-0x07 C2
      if( b < 0x10 ) { bp += 1; continue; }    // 0x08-0x0f C2
      if( b < 0x18 ) { bp += 2; continue; }    // 0x10-0x17 C2
      if( b < 0x20 ) { bp += 3;  continue; }   // 0x18-0x1f C2
      if( b < 0x80 ) continue;  /* misc */     // 0x20-0x7f G2
      if( b < 0x88 ) { bp += 4;  continue; }   // 0x80-0x87 C3
      if( b < 0x90 ) { bp += 5;  continue; }   // 0x88-0x8f C3
      continue;
    }
    int bb = b & 0x07;          // one byte >ETX1 0x11-0x17
    if( bp >= ep ) break;
    bb = (bb << 8) | *bp++;
    if( b >= 0x18 ) {           // two byte >=P16 0x18-0x20
      if( bp >= ep ) break;
      bb = (bb << 8) | *bp++;
    }
    if( curw && curw->st > st_unmap ) curw->store(bb);
  }
  render();
}

int zsvc_t::
render()
{
  /* render dirty windows */
  for( int iw=0; iw<MX_WIN; ++iw ) {
    win_t *wp = &win[iw];
    if( wp->dirty ) {
      wp->render();
      wp->dirty = 0;
    }
  }
  return 0;
}

int zsvc_t::
command(int cmd)
{
  if( cmd == 0x00 || cmd == 0x03 || // nuisance
      cmd == 0x8d || cmd == 0x8e ) return 0;
#define CMD(fn,args...) return fn(args)
//zmsgs("cmd %02x svc %d ",cmd,sid);
//#define CMD(fn,args...) printf("%s\n",#fn); return fn(args)
  switch( cmd ) {
  case 0x03: CMD(ETX);      /* End of text, render */
  case 0x08: CMD(BS);       /* Backspace */
  case 0x0C: CMD(FF);       /* Form Feed */
  case 0x0D: CMD(CR);       /* Carriage Return */
  case 0x0E: CMD(HCR);      /* Horizontal Carriage Return */
  case 0x80 ... 0x87:
    CMD(CWx,cmd & 0x07);    /* SetCurrentWindow */
  case 0x88: CMD(CLW);      /* ClearWindows */
  case 0x89: CMD(DSW);      /* DisplayWindows */
  case 0x8A: CMD(HDW);      /* HideWindows */
  case 0x8B: CMD(TGW);      /* ToggleWindows */
  case 0x8C: CMD(DLW);      /* DeleteWindows */
  case 0x8D: CMD(DLY);      /* Delay */
  case 0x8E: CMD(DLC);      /* Delay Cancel */
  case 0x8F: CMD(RST);      /* Reset */
  case 0x90: CMD(SPA);      /* SetPenAttributes */
  case 0x91: CMD(SPC);      /* SetPenColor */
  case 0x92: CMD(SPL);      /* SetPenLocation */
  case 0x93 ... 0x96:
    CMD(RSVD);              /* reserved */
  case 0x97: CMD(SWA);      /* SetWindowAttributes */
  case 0x98 ... 0x9F:
    CMD(DFx,cmd & 0x07);    /* DFx DefineWindow */
  }
  zerrs("unkn %02x\n",cmd);
  return 1;
}

static inline int RGB2Y(int r,int g,int b)
{
  return ( 257*r+504*g+ 98*b+ 16500)/1000;
}

static inline int RGB2Cr(int r,int g,int b)
{
  return ( 439*r-368*g- 71*b+128500)/1000;
}

static inline int RGB2Cb(int r,int g,int b)
{
  return (-148*r-291*g+439*b+128500)/1000;
}

uint32_t zsvc_t::
pen_yuva(int color, int opacity)
{
  int r, g, b, y, u, v, a;
  r = ((color>>4) & 0x03) * 255 / 3;
  g = ((color>>2) & 0x03) * 255 / 3;
  b = ((color>>0) & 0x03) * 255 / 3;
  a = 255;
  switch( opacity ) {
  case oty_transparent:  a =   0;  break;
  case oty_translucent:  a = 128;  break;
  case oty_flash:        a = 208;  break;
  case oty_solid:        a = 255;  break;
  }
  y = RGB2Y(r,g,b);
  u = RGB2Cr(r,g,b);
  v = RGB2Cb(r,g,b);
  return (y<<24) | (u<<16) | (v<<8) | (a<<0);
}

static inline int put_utf8(unsigned int v, uint8_t *cp, int n)
{
  if( v >= 0x80 ) {
    int l = v < 0x0800 ? 2 : v < 0x00010000 ? 3 :
      v < 0x00200000 ? 4 : v < 0x04000000 ? 5 : 6;
    if( l > n ) return -1;
    int m = 0xff00 >> l, i = l;
    *cp++ = (v>>(6*--i)) | m;
    for( ; --i>=0; ++cp ) *cp = ((v>>(6*i)) & 0x3f) | 0x80;
    return l;
  }
  if( n < 1 ) return -1;
  *cp = v;
  return 1;
}

zwin_t::
win_t()
{
  bfr = 0;
  dirty = 0;
  subtitle = 0;
}

zwin_t::
~win_t()
{
  delete [] bfr;
  delete subtitle;
}

void zwin_t::
output_text()
{
  if( st < st_visible ) return;
  end_frame = svc->cc->video->framenum;
  if( !svc->cc->text_cb ) return;
  if( !bfr ) return;
  char txt[1024];
  uint8_t *cp = (uint8_t *)txt, *ep = cp + sizeof(txt)-1;
  for( int iy=0; iy<ch; ++iy ) {
    if( iy && cp < ep ) *cp++ = '|';
    chr_t *bp = &bfr[iy * MX_CX];
    for( int ix=0; ix<cw && cp<ep; ++ix,++bp ) {
      if( !bp->glyph ) continue;
      int n = put_utf8(bp->glyph, cp, ep-cp);
      if( n < 0 ) break;
      cp += n;
    }
  }
  *cp = 0;
  svc->cc->text_cb(svc->sid, id, start_frame, end_frame, txt);
  start_frame = end_frame;
}

void zwin_t::
set_state(int8_t st)
{
  if( this->st == st ) return;
//zmsgs("win %d was %d, now %d %s\n", id, this->st, st,
//  st==st_unmap ? "unmapped" : st==st_hidden ? "hidden" :
//  st==st_visible ? "visible" : "err");
  output_text();
  if( this->st < st_visible && st >= st_visible )
    start_frame = svc->cc->video->framenum;
  this->st = st;
  dirty = 1;
}

void zwin_t::
reset(int i, zsvc_t *svc)
{
  this->id = i;
  this->svc = svc;
//zmsgs("win %d\n",id);
  scale = 1;
  cx = cy = cw = ch = 0;
  x = y = 0;
  pdir = dir_rt;
  sdir = dir_up;
  svc->bfr_put(bfr);
  st = st_unmap;
}

int zwin_t::
is_breakable(int b)
{
  return b == ' ' || b == '-' || b == '\r';
}

int zwin_t::
wrap()
{
  switch( pdir ) {
  case dir_rt: {
    chr_t line[MX_CX];  int ix = cx-1;
    chr_t *cp = &line[ix], *sp = cp;
    while( ix >= 0 ) {
      chr_t *bp = &bfr[cy*MX_CX + ix];
      if( is_breakable(bp->glyph) ) break;
      *cp-- = *bp;  bp->glyph = 0;  --ix;
    }
    if( ix >= 0 ) CR();
    for( ; cx<cw && cp<sp; ++cx )
      bfr[cy*MX_CX + cx] = *++cp;
    if( ix < 0 ) CR();
    break; }

  case dir_lt: {
    chr_t line[MX_CX];  int ix = cx+1;
    chr_t *cp = &line[ix], *sp = cp;
    while( ix < cw ) {
      chr_t *bp = &bfr[cy*MX_CX + ix];
      if( is_breakable(bp->glyph) ) break;
      *cp++ = *bp;  bp->glyph = 0;  ++ix;
    }
    if( ix < cw ) CR();
    for( ; cx>=0 && cp>sp; --cx )
      bfr[cy*MX_CX + cx] = *--cp;
    if( ix >= cw ) CR();
    break; }

  case dir_dn: {
    chr_t line[MX_CY];  int iy = cy-1;
    chr_t *cp = &line[iy], *sp = cp;
    while( iy >= 0 ) {
      chr_t *bp = &bfr[iy*MX_CX + cx];
      if( is_breakable(bp->glyph) ) break;
      *cp-- = *bp;  bp->glyph = 0;  --iy;
    }
    if( iy >= 0 ) CR();
    for( ; cy<ch && cp<sp; ++cy )
      bfr[cy*MX_CX + cx] = *++cp;
    if( iy < 0 ) CR();
    break; }

  case dir_up: {
    chr_t line[MX_CY];  int iy = cy+1;
    chr_t *cp = &line[iy], *sp = cp;
    while( iy < ch ) {
      chr_t *bp = &bfr[iy*MX_CX + cx];
      if( is_breakable(bp->glyph) ) break;
      *cp++ = *bp;  bp->glyph = 0;  ++iy;
    }
    if( iy < ch ) CR();
    for( ; cy>=0 && cp>sp; --cy )
      bfr[cy*MX_CX + cx] = *--cp;
    if( iy >= ch ) CR();
    break; }
  }

  return 0;
}

int zwin_t::
ovfl()
{
  if( !col_lock && !resize(cx+1, ch) ) return 0;
  if( !row_lock ) return 1;
  if( wordwrap ) return wrap();
  return CR();
}

int zwin_t::
store(int b)
{
//zmsgs("%02x ** \"%c\" at %d,%d win %d\n",
//  b, b>=0x20 && b<0x7f ? b : '.',cx,cy,id);
  if( !bfr ) return 1;
  switch( pdir ) {
  case dir_rt:  if( cx >= cw && ovfl() ) return 1;  break;
  case dir_lt:  if( cx < 0 && ovfl() )   return 1;  break;
  case dir_dn:  if( cy >= ch && ovfl() ) return 1;  break;
  case dir_up:  if( cy < 0 && ovfl() )   return 1;  break;
  }
  if( cx >= 0 && cx < cw && cy >= 0 && cy < ch ) {
    chr_t *chr = &bfr[cy*MX_CX + cx];
    chr->glyph = b;
    chr->fg_opacity = fg_opacity;
    chr->fg_color = fg_color;
    chr->bg_opacity = bg_opacity;
    chr->bg_color = bg_color;
    chr->pen_size = pen_size;
    chr->edge_color = edge_color;
    chr->italics = italics;
    chr->edge_type = edge_type;
    chr->underline = underline;
    chr->font_style = font_style;
    chr->font_size = svc->cc->font_size;
    chr->offset = offset;
    chr->text_tag = text_tag;
    dirty = 1;
  }
  switch( pdir ) {
  case dir_rt:  ++cx;  break;
  case dir_lt:  --cx;  break;
  case dir_dn:  ++cy;  break;
  case dir_up:  --cy;  break;
  }
  return 0;
}

int zsvc_t::
ETX() /* end of text */
{
//zmsgs("svc %d,win %d\n",sid, curw ? curw->id : -1);
//  if( curw ) curw->render();
  return 0;
}

int zwin_t::
BS() /* Backspace */
{
  switch( pdir ) {
  case dir_rt:  if( cx > 0 ) --cx;   break;
  case dir_lt:  if( cx < cw-1 ) ++cx;  break;
  case dir_dn:  if( cy > 0 ) --cy;   break;
  case dir_up:  if( cy < ch-1 ) ++cy;  break;
  }
  return 0;
}

int zsvc_t::
BS() /* Backspace */
{
  if( !curw || curw->st == st_unmap ) return 0;
//zmsgs("svc %d,win %d\n",sid,curw->id);
  return curw->BS();
}

int zwin_t::
FF() /* Form Feed */
{
  int n = cw*sizeof(*bfr);
  for( int iy=0; iy<ch; ++iy )
    memset(&bfr[iy*MX_CX],0,n);
  cx = cy = 0;
  dirty = 1;
  return 0;
}

int zsvc_t::
FF() /* Form Feed */
{
  if( !curw || curw->st == st_unmap ) return 0;
//zmsgs("svc %d,win %d\n",sid,curw->id);
  return curw->FF();
}

int zwin_t::
CR() /* Carriage Return */
{
  int n = (cw-1)*sizeof(*bfr);
  int m = cw*sizeof(*bfr);
  int scrolled = 0;
  switch( sdir ) {
  case dir_rt:
    cy = 0;
    if( cx > 0 ) { --cx;  break; }
    for( int i=0; i<ch; ++i ) {
      memmove(&bfr[i*MX_CX+1],&bfr[i*MX_CX],n);
      memset(&bfr[i*MX_CX],0,sizeof(*bfr));
    }
    cx = 0;
    scrolled = 1;
    break;
  case dir_lt:
    cy = 0;
    if( cx+1 < cw ) { ++cx;  break; }
    for( int i=0; i<ch; ++i ) {
      memmove(&bfr[i*MX_CX],&bfr[i*MX_CX+1],n);
      memset(&bfr[i*MX_CX+cw-1],0,sizeof(*bfr));
    }
    cx = cw-1;
    scrolled = 1;
    break;
  case dir_dn:
    cx = 0;
    if( cy > 0 ) { --cy;  break; }
    for( int i=ch; --i>0; )
      memmove(&bfr[i*MX_CX],&bfr[(i-1)*MX_CX],m);
    memset(&bfr[0],0,m);
    cy = 0;
    scrolled = 1;
    break;
  case dir_up:
    cx = 0;
    if( cy+1 < ch ) { ++cy;  break; }
    for( int i=1; i<ch; ++i )
      memmove(&bfr[(i-1)*MX_CX],&bfr[i*MX_CX],m);
    memset(&bfr[(ch-1)*MX_CX],0,m);
    cy = ch-1;
    scrolled = 1;
    break;
  }
  if( scrolled ) {
    output_text();
    dirty = 1;
  }
  return 0;
}

int zsvc_t::
CR() /* Carriage Return */
{
  if( !curw || curw->st == st_unmap ) return 0;
//zmsgs("svc %d,win %d\n",sid,curw->id);
  return curw->CR();
}

int zwin_t::
HCR() /* Horizontal Carriage Return */
{
  switch( pdir ) {
  case dir_rt:
  case dir_lt:
    memset(&bfr[cy*MX_CX],0,cw*sizeof(*bfr));
    cy = pdir == dir_rt ? 0 : cw-1;
    dirty = 1;
    break;
  case dir_up:
  case dir_dn:
    for( int i=0; i<ch; ++i ) memset(&bfr[i*MX_CX+cx],0,sizeof(*bfr));
    cx = pdir == dir_dn ? 0 : ch-1;
    dirty = 1;
    break;
  }
  return 0;
}

int zsvc_t::
HCR() /* Horizontal Carriage Return */
{
  if( !curw || curw->st == st_unmap ) return 0;
//zmsgs("svc %d,win %d\n",sid,curw->id);
  return curw->HCR();
}

int zsvc_t::
CLW()  /* ClearWindows */
{
  if( bp >= ep ) return 1;
  int map = *bp++;  // 1
//zmsgs("map %02x svc %d\n",map,sid);
  for( int iw=0; iw<MX_WIN; ++iw ) {
    if( !(map & (1<<iw)) ) continue;
    win_t *wp = &win[iw];
    if( wp->st > st_unmap ) {
      wp->output_text();
//zmsgs("  clear win %d\n",wp->id);
      int n = wp->cw*sizeof(*wp->bfr);
      for( int iy=0; iy<wp->ch; ++iy )
        memset(&wp->bfr[iy*MX_CX],0,n);
      wp->dirty = 1;
    }
  }
  return 0;
}

int zsvc_t::
DSW() /* DisplayWindows */
{
  if( bp >= ep ) return 1;
  int map = *bp++;  // 1
//zmsgs("map %02x svc %d\n",map,sid);
  for( int iw=0; iw<MX_WIN; ++iw ) {
    if( !(map & (1<<iw)) ) continue;
    win_t *wp = &win[iw];
    if( wp->st > st_unmap ) {
      wp->set_state(st_visible);
    }
  }
  return 0;
}

int zsvc_t::
HDW() /* HideWindows */
{
  if( bp >= ep ) return 1;
  int map = *bp++;  // 1
//zmsgs("map %02x svc %d\n",map,sid);
  for( int iw=0; iw<MX_WIN; ++iw ) {
    if( !(map & (1<<iw)) ) continue;
    win_t *wp = &win[iw];
    if( wp->st > st_unmap ) {
      wp->set_state(st_hidden);
    }
  }
  return 0;
}

int zsvc_t::
TGW() /* ToggleWindows */
{
  if( bp >= ep ) return 1;
  int map = *bp++;  // 1
//zmsgs("map %02x svc %d\n",map,sid);
  for( int iw=0; iw<MX_WIN; ++iw ) {
    if( !(map & (1<<iw)) ) continue;
    win_t *wp = &win[iw];
    if( wp->st > st_unmap ) {
      int8_t st = wp->st == st_visible ? st_hidden : st_visible;
      wp->set_state(st);
    }
  }
  return 0;
}

int zsvc_t::
DLW() /* DeleteWindows */
{
  if( bp >= ep ) return 1;
  int map = *bp++;  // 1
//zmsgs("map %02x svc %d\n",map,sid);
  for( int iw=0; iw<MX_WIN; ++iw ) {
    if( !(map & (1<<iw)) ) continue;
    win_t *wp = &win[iw];
    if( wp->st == st_unmap ) continue;
//zmsgs("  delete %d\n",wp->id);
    wp->set_state(st_unmap);
    if( wp == curw ) curw = 0;
    bfr_put(wp->bfr);
  }
  return 0;
}

int zsvc_t::
DLY() /* delay */
{
  if( bp >= ep ) return 1;
//zmsgs(" svc %d (%d)\n",sid, *bp);
  ++bp;  // 1
  return 0;
}

int zsvc_t::
DLC() /* delay cancel */
{
//zmsg("\n");
  return 0;
}

int zsvc_t::
RST() /* Reset */
{
//zmsgs("svc %d\n",sid);
  reset(sid, cc);
  return 0;
}

int zsvc_t::
SPA() /* SetPenAttributes */
{
  if( bp+2 > ep ) return 1;
  int b = *bp++;  // 1
  int text_tag = b>>4;
  int offset = (b>>2) & 0x03;
  int pen_size = b & 0x03;
  b = *bp++;  // 2
  int italics = b>>7;
  int underline = (b>>6) & 0x01;
  int edge_type = (b>>3) & 0x07;
  int font_style = b & 0x07;
  if( pen_size >= 3 ) return 0;
  if( offset >= 3 ) return 0;
  if( edge_type >= 6 ) return 0;
  win_t *wp = curw;
  if( wp && wp->st > st_unmap ) {
    wp->text_tag = text_tag;
    wp->offset = offset;
    wp->pen_size = pen_size;
    wp->italics = italics;
    wp->underline = underline;
    wp->edge_type = edge_type;
    wp->font_style = font_style;
//zmsgs("txttag=%d,offset=%d,pen_size=%d,ital=%d,undl=%d,"
//  "edge=%d,ftstyl=%d,win %d,svc %d\n",text_tag,offset,
//  pen_size,italics,underline,edge_type,font_style,wp->id,sid);
  }
  return 0;
}

int zsvc_t::
SPC() /* SetPenColor */
{
  if( bp+3 > ep ) return 1;
  int b = *bp++;  // 1
  int fg_opacity = b >> 6;
  int fg_color = b & 0x3f;
  b = *bp++;  // 2
  int bg_opacity = b >> 6;
  int bg_color = b & 0x3f;
  b = *bp++;  // 3
  int edge_color = b;
  win_t *wp = curw;
  if( wp && wp->st > st_unmap ) {
    wp->fg_opacity = fg_opacity;
    wp->fg_color = fg_color;
    wp->edge_color = edge_color;
    wp->bg_opacity = bg_opacity;
    wp->bg_color = bg_color;
    wp->fg_yuva = pen_yuva(fg_color,fg_opacity);
    wp->edge_yuva = pen_yuva(edge_color,fg_opacity);
    wp->bg_yuva = pen_yuva(bg_color,bg_opacity);
//zmsgs("fgopc=%d,fgclr=%02x,bgopc=%d,bgclr=%02x,egclr=%02x,"
//  "win %d,svc %d\n",fg_opacity,fg_color,bg_opacity,bg_color,
//  edge_color,wp->id,sid);
  }
  return 0;
}

int zsvc_t::
SPL() /* SetPenLocation */
{
  if( bp+2 > ep ) return 1;
  int b = *bp++;  // 1
  int row = b;
  b = *bp++;  // 2
  int column = b;
  if( row >= MX_CY ) return 0;
  if( column >= MX_CX ) return 0;
  win_t *wp = curw;
  if( wp && wp->st > st_unmap ) {
    if( column >= wp->cw || row >= wp->ch ) return 1;
    wp->cx = column;  wp->cy = row;
//zmsgs("row=%d,col=%d,win %d,svc %d\n",row,column,wp->id,sid);
  }
  return 0;
}

int zsvc_t::
RSVD() /* reserved */
{
//zmsgs("sid %d\n",sid);
  return 0;
}

int zsvc_t::
SWA() /* SetWindowAttributes */
{
  if( bp+4 > ep ) return 1;
  int b = *bp++;  // 1
  int fill_opacity = b>>6;
  int fill_color = b & 0x3f;
  b = *bp++;  // 2
  int border_color = b & 0x3f;
  int border_type = b>>6;
  b = *bp++;  // 3
  int wordwrap = (b>>6) & 0x01;
  border_type |= (b>>5) & 0x04;
  int print_direction = (b>>4) & 0x03;
  int scroll_direction = (b>>2) & 0x03;
  int justify = b & 0x03;
  b = *bp++;  // 4
  int effect_speed = b>>4;
  int effect_direction = (b>>2) & 0x3;
  int display_effect = b & 0x03;
  if( border_type >= 6 ) return 0;
  if( display_effect >= 3 ) return 0;

  win_t *wp = curw;
  if( wp && wp->st > st_unmap ) {
    wp->fill_opacity = fill_opacity;
    wp->fill_color = fill_color;
    wp->border_color = border_color;
    wp->fill_yuva = pen_yuva(fill_color,fill_opacity);
    wp->border_yuva = pen_yuva(border_color,fill_opacity);
    wp->wordwrap = wordwrap;
    wp->pdir = print_direction;
    wp->sdir = scroll_direction;
    wp->justify = justify;
    wp->effect_speed = effect_speed;
    wp->effect_direction = effect_direction;
    wp->display_effect = display_effect;
    wp->dirty = 1;
//zmsgs("fopc=%d,fclr=%02x,bclr=%02x,wrap=%d,pdir=%d,sdir=%d,just=%d"
//  "espd=%d,edir=%d,deft=%d,win %d,svc %d\n",fill_opacity,fill_color,
//  border_color,wordwrap,print_direction,scroll_direction,justify,
//  effect_speed,effect_direction,display_effect,wp->id,sid);
  }
  return 0;
}

int zsvc_t::
CWx(int id) /* SetCurrentWindow */
{
  win_t *wp = &win[id];
  curw = wp->st == st_unmap ? 0 : wp;
//zmsgs("== win %d,svc %d\n",wp->id,sid);
  return 0;
}

int zsvc_t::
DFz(int id) /* DFx DefineWindow */
{
  if( bp+6 > ep ) return 1;
  win_t *wp = &win[id];
  int b = *bp++; // 1
  int visible = (b>>5) & 0x01;
  int row_lock = (b>>4) & 0x01;
  int col_lock = (b>>3) & 0x01;
  int priority = b & 0x07;
  b = *bp++; // 2
  int anchor_relative = (b>>7) & 0x01;
  int anchor_vertical = b & 0x7f;
  b = *bp++; // 3
  int anchor_horizontal = b;
  b = *bp++; // 4
  int anchor_point = b>>4;
  int ch = (b & 0x0f) + 1;
  b = *bp++; // 5
  int cw = b + 1;
  b = *bp++; // 6
  int window_style = (b>>3) & 0x07;
  int pen_style = b & 0x07;
//zmsgs("%dx%d vis=%d,rowlk=%d,collk=%d,pri=%d,an_rel=%d,\n"
//  "an_hor=%d,an_vert=%d,an_pnt=%d,wsty=%d,psty=%d,win %d,svc %d\n",
//  cw,ch,visible,row_lock,col_lock,priority,anchor_relative,
//  anchor_horizontal,anchor_vertical,anchor_point,window_style,
//  pen_style,wp->id,sid);
  if( anchor_relative ) {
    if( anchor_vertical >= 75 ) return -1;
    if( anchor_horizontal >= 210 ) return -1;
  }
  else {
    if( anchor_vertical >= 100 ) return -1;
    if( anchor_horizontal >= 100 ) return -1;
  }
  if( anchor_point >= 9 ) return -1;
  if( cw > MX_CX ) return -1;
  if( ch > MX_CY ) return -1;

  wp->row_lock = row_lock;
  wp->col_lock = col_lock;
  wp->priority = priority;
  wp->anchor_relative = anchor_relative;
  wp->anchor_vertical = anchor_vertical;
  wp->anchor_horizontal = anchor_horizontal;
  wp->anchor_point = anchor_point;
  if( wp->st == st_unmap )
    wp->set_window_style(window_style);
  if( wp->st == st_unmap )
    wp->set_pen_style(pen_style);
  int bw = wp->border_type != edg_none ? bdr_width : 0;
  wp->bw = (bw+3) & ~4;  // yuv 422 macro pixels
  if( wp->st == st_unmap )
    wp->init(cw, ch);
  else
    wp->resize(cw, ch);
  wp->set_state(visible ? st_visible : st_hidden);
  curw = wp;
  wp->dirty = 1;
  int scale = cc->video->coded_picture_height/360;
  if( scale < 1 ) scale = 1;
  wp->scale = scale;
  return 0;
}

int zsvc_t::
DFx(int id) /* DFx DefineWindow wrapper */
{
  int ret = DFz(id);
  if( ret < 0 ) {
    zmsgs("failed to validate cc win %d\n",id);
    ret = 0;
  }
  return ret;
}

int zwin_t::
init(int ncw, int nch)
{
//zmsgs("  init %d, %dx%d\n", id, ncw, nch);
  if( ncw > MX_CX ) return 1;
  if( nch > MX_CY ) return 1;
  bfr = svc->get_bfr();
  if( !bfr )
    bfr = new chr_t[MX_CX*MX_CY];
  cw = ncw;  ch = nch;
  int n = cw*sizeof(*bfr);
  for( int iy=0; iy<ch; ++iy )
    memset(&bfr[iy*MX_CX],0,n);
  text_tag = 0;
  cx = cy = 0;
  x = y = 0;
  return 0;
}

int zwin_t::
resize(int ncw, int nch)
{
//zmsgs("  resize %d, %dx%d %s\n", id, ncw, nch, cw!=ncw || ch!=nch ? "*" : "");
  if( ncw > MX_CX ) return 1;
  if( nch > MX_CY ) return 1;
  int dx = ncw-cw, dy = nch-ch;
  if( !dx && !dy ) return 0;
  int n = cw*sizeof(*bfr);
  switch( sdir ) {
  case dir_rt:
    if( dx > 0 ) { // scroll right, expanding
      int m = dx*sizeof(*bfr);
      for( int iy=0; iy<ch; ++iy ) {
        chr_t *bp = &bfr[iy*MX_CX], *cp = bp+dx;
        memmove(cp,bp,n);
        memset(bp,0,m);
      }
      dx = 0;
    }
    break;
  case dir_lt:
    if( dx < 0 ) { // scroll left, contracting
      for( int iy=0; iy<ch; ++iy ) {
        chr_t *bp = &bfr[iy*MX_CX], *cp = bp-dx;
        memmove(bp,cp,n);
      }
      dx = 0;
    }
    break;
  case dir_dn:
    if( dy > 0 ) { // scroll down, expanding
      for( int iy=ch; --iy>=dy; ) {
        chr_t *bp = &bfr[iy*MX_CX], *cp = bp-dy*MX_CX;
        memmove(bp,cp,n);
      }
      memset(&bfr[0],0,dy*n);
      dy = 0;
    }
    break;
  case dir_up:
    if( dy < 0 ) { // scroll up, contracting
      for( int iy=dy; iy>ch; ++iy ) {
        chr_t *bp = &bfr[iy*MX_CX], *cp = bp+dy*MX_CX;
        memmove(cp,bp,n);
      }
      dy = 0;
    }
    break;
  }

  if( dx > 0 ) {
    int m = dx*sizeof(*bfr);
    for( int iy=0; iy<ch; ++iy ) {
      chr_t *bp = &bfr[iy*MX_CX] + cw;
      memset(bp,0,m);
    }
  }
  if( dy > 0 ) {
    n = ncw*sizeof(*bfr);
    for( int iy=ch; iy<nch; ++iy ) {
      chr_t *bp = &bfr[iy*MX_CX];
      memset(bp,0,n);
    }
  }
  cw = ncw;  ch = nch;
  return 0;
}

void zwin_t::
set_pen_style(int i)
{
  pen_style = i > 0 ? i : 1;
  pen_size = psz_standard;
  font_style = pen_style <= 5 ? pen_style-1 : pen_style-3;
  offset = ofs_normal;
  italics = underline = 0;
  fg_opacity = oty_solid;
  fg_color = 0x3f;
  edge_type = pen_style <= 5 ? edg_none : edg_uniform;
  edge_color = 0x00;
  bg_opacity = pen_style <= 5 ? oty_solid : oty_transparent;
  bg_color = 0x00;
  fg_yuva = pen_yuva(fg_color,fg_opacity);
  edge_yuva = pen_yuva(edge_color,fg_opacity);
  bg_yuva = pen_yuva(bg_color,bg_opacity);
  text_tag = tag_dialog;
}
 
void zwin_t::
set_window_style(int i)
{
  window_style = i > 0 ? i : 1;
  justify = ((0x48>>window_style) & 1) ? jfy_center : jfy_left;
  pdir = window_style < 7 ? dir_rt : dir_dn;
  sdir = window_style < 7 ? dir_up : dir_lt;
  wordwrap = (0x70>>window_style) & 1;
  display_effect = eft_snap;
  effect_speed = 0;
  effect_direction = dir_rt;
  fill_opacity = oty_solid;
  fill_color = 0x00;
  border_type = edg_none;
  border_color = 0x00;
  fill_yuva = pen_yuva(fill_color,fill_opacity);
  border_yuva = pen_yuva(border_color,fill_opacity);
}

#if 0
void zwin_t::
default_window()
{
  if( !bfr ) return;
  row_lock = col_lock = 0;
  priority = 0;
  anchor_relative = 0;
  anchor_vertical = 24;
  anchor_horizontal = 12;
  anchor_point = 0;
  set_window_style(1);
  set_pen_style(1);
  text_tag = 0;
  ch = 1;  cw = 32;
  int n = cw*sizeof(*bfr);
  for( int iy=0; iy<ch; ++iy)
    memset(&bfr[iy*MX_CX],0,n);
  cx = cy = 0;
  st = st_visible;
}
#endif

void zwin_t::
print_buffer()
{
  if( !bfr ) return;
  zmsgs("bfr %d\n",id);
  for( int iy=0; iy<ch; ++iy ) {
    int ofs = iy*MX_CX;
    for( int ix=0; ix<cw; ++ix ) {
      int b = bfr[ofs++].glyph;
      printf("%c", b>=0x20 && b<0x7f ? b : '.');
    }
    printf("\n");
  }
}

void zwin_t::
put_chr(chr_t *chr)
{
  if( !subtitle ) return;
  uint16_t bb = chr->glyph;
  bitfont_t *font = chr_font(chr, scale);
  if( bb >= font->nch ) return;
//zmsgs("%02x \"%c\" at %d,%d in %d\n",bb,bb>=0x20&&bb<0x7f?bb:'.',x,y,id);
  int ww = subtitle->w - 2*bw;
  int hh = subtitle->h - 2*bw;
  int dx = 0, dy = 0;
  bitfont_t::bitchar_t *cp = &font->chrs[bb];
  uint8_t *bmap = cp->bitmap;
  if( bmap || cp->ch == ' ' ) {
    /* cell origin = (x,y) addressed (0,0) to (x2,y2) */
    /* char data box (x0,y0) to (x1,y1), may exceed cell box */
    int x0, ix0, y0, iy0;
    int x1, x2, y1, y2;
    x0 = ix0 = cp->left;
    y0 = iy0 = font->imy - cp->ascent;
    x1 = cp->right;
    y1 = font->imy + cp->decent;
    x2 = font->idx;  y2 = font->idy;
    if( bmap ) {
      switch( pdir ) {
      case dir_rt:  x2 = dx = cp->right;  break;
      case dir_lt:  x2 = dx = cp->left;   break;
      case dir_dn:  y2 = dy = cp->decent;  break;
      case dir_up:  y2 = dy = cp->ascent;  break;
      }
    }
    else { /* blank */
      switch( pdir ) {
      case dir_rt: case dir_lt:  dx = font->idx;  break;
      case dir_dn: case dir_up:  dy = font->idy;  break;
      }
    }
    if( x+x0 < 0 ) x0 = -x;
    if( y+y0 < 0 ) y0 = -y;
    int mx = ww - x;
    if( x0 > mx ) x0 = mx;
    if( x1 > mx ) x1 = mx;
    if( x2 > mx ) x2 = mx;
    int my = hh - y;
    if( y0 > my ) y0 = my;
    if( y1 > my ) y1 = my;
    if( y2 > my ) y2 = my;

    class pix_t {  /* address char cell */
      uint8_t *image_y, *image_u, *image_v, *image_a;
      uint8_t *img_y, *img_u, *img_v, *img_a;  int w;
    public:
      void set_line(int n) {
        int ofs = w * n;
        img_y = image_y + ofs;
        img_u = image_u + ofs;
        img_v = image_v + ofs;
        img_a = image_a + ofs;
      }
      pix_t(subtitle_t *subtitle, int x, int y) {
        w = subtitle->w;
        int ofs = y*w + x;
        image_y = subtitle->image_y + ofs;
        image_u = subtitle->image_u + ofs;
        image_v = subtitle->image_v + ofs;
        image_a = subtitle->image_a + ofs;
      }
      void write(int i, uint32_t yuva) {
        img_y[i] = yuva >> 24;
        img_u[i] = yuva >> 16;
        img_v[i] = yuva >> 8;
        img_a[i] = yuva >> 0;
      }
      void fill_box(uint32_t yuva, int x0,int y0, int x1,int y1) {
        if( x0 >= x1 ) return;
        while( y0 < y1 ) {
          set_line(y0++);
          for( int ix=x0; ix<x1; ++ix ) write(ix,yuva);
        }
      }
    } pix(subtitle, x+bw, y+bw); /* origin inside border */
    /* dont fill left of cell */
    pix.fill_box(bg_yuva,  0, 0, x2,y0);  /* fill top */
    pix.fill_box(bg_yuva,  0,y0, x0,y1);  /* fill left */
    pix.fill_box(bg_yuva, x1,y0, x2,y1);  /* fill right */
    pix.fill_box(bg_yuva,  0,y1, x2,y2);  /* fill bottom */

    int wsz = (cp->right-cp->left+7) / 8;
    int rx = x0 - ix0;
    int ry = y0 - iy0;
    /* render char box */
    for( int iy=y0; iy<y1; ++iy,++ry ) {
      pix.set_line(iy);
      uint8_t *row = &bmap[ry*wsz];
      int i = rx, ix = x0;
      /* dont render bg left of cell */
      while( ix < 0 ) {
        if( ((row[i>>3] >> (i&7)) & 1) ) pix.write(ix, fg_yuva);
        ++ix;  ++i;
      }
      /* render fg/bg inside of cell */
      while( ix < x1 ) {
        pix.write(ix, ((row[i>>3] >> (i&7)) & 1) ? fg_yuva : bg_yuva);
        ++ix;  ++i;
      }
    }
    if( chr->underline ) {
      int uy0 = font->imy;
      if( uy0 < y2 ) {
        int uy1 = uy0 + (y2+23)/24;
        if( uy1 > y2 ) uy1 = y2;
        pix.fill_box(fg_yuva, 0,uy0, x2,uy1);
      }
    }
  }
  else {
    /* no bitmap and not blank */
    switch( pdir ) {
    case dir_rt:  dx = font->idx;  dy = 0;  break;
    case dir_lt:  dx = font->idy;  dy = 0;  break;
    case dir_dn:  dx = 0;  dy = font->idy;  break;
    case dir_up:  dx = 0;  dy = font->idy;  break;
    }
  }
  x += dx;  y += dy;
}

void zwin_t::
hbar(int x, int y, int ww, int hh, int lm, int rm, int clr)
{
/* w,h = widget dims, ww,hh = bar dims */
/* x,y = widget coord, lm,rm = lt/rt miter, clr = color */
  int w = subtitle->w;
  int h = subtitle->h;
  if( y < 0 ) { hh += y;  y = 0; }
  if( y+hh > h ) hh = h - y;
  uint32_t yuva = border_yuva;
  for( int ih=hh; --ih>=0; ++y ) {
    int iw = ww, ix = x;
    if( ix < 0 ) { iw += ix;  ix = 0; }
    if( ix+iw > w ) iw = w - ix;
    int ofs = w*y + ix;
    uint8_t *img_y = &subtitle->image_y[ofs];
    uint8_t *img_u = &subtitle->image_u[ofs];
    uint8_t *img_v = &subtitle->image_v[ofs];
    uint8_t *img_a = &subtitle->image_a[ofs];
    while( --iw >= 0 ) {
      *img_y = clr;         ++img_y;
      *img_u = yuva >> 16;  ++img_u;
      *img_v = yuva >> 8;   ++img_v;
      *img_a = yuva >> 0;   ++img_a;
    }
    x += lm;  ww -= lm-rm;
  }
}

void zwin_t::
vbar(int x, int y, int ww, int hh, int tm, int bm, int clr)
{
/* w,h = widget dims, ww,hh = bar dims */
/* x,y = widget coord, lm,rm = lt/rt miter, clr = color */
  int w = subtitle->w;
  int h = subtitle->h;
  if( x < 0 ) { ww += x;  x = 0; }
  if( x+ww > w ) ww = w - x;
  uint32_t yuva = border_yuva;
  for( int iw=ww; --iw>=0; ++x ) {
    int ih = hh, iy = y;
    if( iy < 0 ) { ih += iy;  iy = 0; }
    if( iy+ih > h ) ih = h - iy;
    int ofs = w*iy + x;
    uint8_t *img_y = &subtitle->image_y[ofs];
    uint8_t *img_u = &subtitle->image_u[ofs];
    uint8_t *img_v = &subtitle->image_v[ofs];
    uint8_t *img_a = &subtitle->image_a[ofs];
    while( --ih >= 0 ) {
      *img_y = clr;         img_y += w;
      *img_u = yuva >> 16;  img_u += w;
      *img_v = yuva >> 8;   img_v += w;
      *img_a = yuva >> 0;   img_a += w;
    }
    y += tm;  hh -= tm-bm;
  }
}

void zwin_t::
border(int ilt, int olt, int irt, int ort,
       int iup, int oup, int idn, int odn)
{
/* i/o = inside/outside + lt/rt/up/dn = colors */
  if( !bw || !subtitle ) return;
  int w = subtitle->w;
  int h = subtitle->h;
  int sbw = bw * scale;
  int zo = sbw / 2;  // outside width
  int zi = sbw - zo;  // inside width
  hbar(0,0,w,zo,1,-1,oup);
  hbar(zo,zo,w-2*zo,zi,1,-1,iup);
  hbar(sbw-1,h-sbw,w-2*sbw+2,zi,-1,1,idn);
  hbar(zo-1,h-zo,w-2*zo+2,zo,-1,1,odn);
  vbar(0,1,zo,h-2,1,-1,olt);
  vbar(zo,zo+1,zi,h-2*zo-2,1,-1,ilt);
  vbar(w-sbw,sbw,zi,h-2*sbw,-1,1,irt);
  vbar(w-zo,zo,zo,h-2*zo,-1,1,ort);
}

void zwin_t::
render()
{
  if( st < st_visible || !bfr ) {
    if( subtitle ) subtitle->draw = -1;
    return;
  }
  /* get bounding box */
  int wd = 2*bw;
  int ht = 2*bw;
  int fw = 0, fh = 0, zz = 0;
  switch( pdir ) {
  case dir_rt:
  case dir_lt:
    for( int iy=0; iy<ch; ++iy ) {
      int ww = 0;
      for( int ix=0; ix<cw; ++ix ) {
        chr_t *chr = &bfr[iy*MX_CX + ix];
        bitfont_t *font = chr_font(chr, scale);
        if( font->idy > fh ) fh = font->idy;
        int b = chr->glyph;
        if( b >= font->nch ) continue;
        bitfont_t::bitchar_t *cp = &font->chrs[b];
        int sz = !cp->bitmap ? font->idx :
          pdir == dir_rt ? cp->right : cp->left;
        ww += sz;
      }
      if( ww > zz ) zz = ww;
    }
    wd += zz;
    ht += ch * fh;
    break;
  case dir_up:
  case dir_dn:
    for( int ix=0; ix<cw; ++ix ) {
      int hh = 0;
      for( int iy=0; iy<ch; ++iy ) {
        chr_t *chr = &bfr[iy*MX_CX + ix];
        bitfont_t *font = chr_font(chr, scale);
        if( font->idx > fw ) fw = font->idx;
        int b = chr->glyph;
        if( b >= font->nch ) continue;
        bitfont_t::bitchar_t *cp = &font->chrs[b];
        int sz = !cp->bitmap ? font->idy :
          pdir == dir_dn ? cp->decent : cp->ascent;
        hh += sz;
      }
      if( hh > zz ) zz = hh;
    }
    wd += cw * fw;
    ht += zz;
    break;
  }
  video_t *video = svc->cc->video;
//zmsgs(" an_horz=%d, an_vert=%d, an_rela=%d, %dx%d\n",
//  anchor_horizontal, anchor_vertical, anchor_relative,
//  video->coded_picture_width, video->coded_picture_height);
  int ax = ((anchor_horizontal+10) * video->coded_picture_width) / 200;
  int ay = ((anchor_vertical+10) * video->coded_picture_height) / 100;
  if( anchor_relative ) { ax += ax; ay += ay; }
  switch( anchor_point ) {
  case 0x00:                            break;  /* nw */
  case 0x01:  ax -= wd/2;               break;  /* n */
  case 0x02:  ax -= wd;                 break;  /* ne */
  case 0x03:  ax -= wd;    ay -= ht/2;  break;  /* e */
  case 0x04:  ax -= wd;    ay -= ht;    break;  /* se */
  case 0x05:  ax -= wd/2;  ay -= ht;    break;  /* s */
  case 0x06:               ay -= ht;    break;  /* sw */
  case 0x07:               ay -= ht/2;  break;  /* w */
  case 0x08:  ax -= wd/2;  ay -= ht/2;  break;  /* center */
  }

  zmpeg3_t *src = video->src;
  strack_t *strack = src->create_strack(svc->ctrk, video);
  if( !subtitle )
    subtitle = new subtitle_t(id, wd, ht);
  if( subtitle->done < 0 ) {
    if( strack->append_subtitle(subtitle,0) ) {
      delete subtitle;  subtitle = 0;
      return;
    }
  }
  subtitle->set_image_size(wd, ht);
  subtitle->x1 = ax;  subtitle->x2 = ax + wd;
  subtitle->y1 = ay;  subtitle->y2 = ay + ht;
  subtitle->draw = 1;
  subtitle->frame_time = video->frame_time;
//print_buffer();
  uint8_t yy = fill_yuva >> 24;
  uint8_t uu = fill_yuva >> 16;
  uint8_t vv = fill_yuva >> 8;
  uint8_t aa = fill_yuva >> 0;
  aa = 0xff - aa;  // this is probably wrong
  int w = subtitle->w;
  int h = subtitle->h;
  int sz = w * h;
  memset(subtitle->image_y, yy, sz);
  memset(subtitle->image_u, uu, sz);
  memset(subtitle->image_v, vv, sz);
  memset(subtitle->image_a, aa, sz);
  yy = border_yuva >> 24;
  int hi_y, lo_y;
  if( (hi_y = yy + 0x60) > 0xff ) hi_y = 0xff;
  if( (lo_y = yy - 0x60) < 0x10 ) lo_y = 0x10;
  switch( border_type ) {
  case 0x01: /* raised */
    border(hi_y, yy, hi_y, yy, hi_y, yy, hi_y, yy);
    break;
  case 0x02: /* depressed */
    border(lo_y, yy, lo_y, yy, lo_y, yy, lo_y, yy);
    break;
  case 0x03: /* uniform */
    border(yy, yy, yy, yy, yy, yy, yy, yy);
    break;
  case 0x04: /* shadowl */
    border(lo_y, lo_y, hi_y, hi_y, hi_y, lo_y, hi_y, lo_y);
    break;
  case 0x05: /* shadowr */
    border(hi_y, hi_y, lo_y, lo_y, hi_y, lo_y, hi_y, lo_y);
    break;
  default:
  case 0x00: /* none */
    break;
  }
  /* render character data */
  switch( pdir ) {
  case dir_rt:
    for( int iy=0; iy<ch; ++iy ) {
      x = 0;  y = iy * fh;
      chr_t *bp = &bfr[iy * MX_CX];
      for( int ix=0; ix<cw; ++ix, ++bp ) {
        put_chr(bp);
      }
    }
    break;
  case dir_lt:
    for( int iy=0; iy<ch; ++iy ) {
      x = w;  y = iy * fh;
      chr_t *bp = &bfr[iy*MX_CX + cw];
      for( int ix=cw; --ix>=0; )
        put_chr(--bp);
    }
    break;
  case dir_dn:
    for( int ix=0; ix<cw; ++ix ) {
      x = ix * fw;  y = 0;
      chr_t *bp = &bfr[ix];
      for( int iy=0; iy<ch; ++iy, bp+=MX_CX )
        put_chr(bp);
    }
    break;
  case dir_up:
    for( int ix=0; ix<cw; ++ix ) {
      x = ix * fw;  y = h;
      chr_t *bp = &bfr[ch*MX_CX - cw + ix];
      for( int iy=ch; --iy>=0; ) {
        put_chr(bp -= MX_CX);
      }
    }
    break;
  }
}

