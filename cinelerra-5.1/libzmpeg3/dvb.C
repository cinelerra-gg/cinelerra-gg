#include "libzmpeg3.h"
#include "huf.h"

#ifdef ZDVB

//#define DEBUG

#ifdef DEBUG
static const char *desc_name1[] = {
/* 00 */  0,
/* 01 */  0,
/* 02 */  "video_stream_descr",
/* 03 */  "audio_stream_descr",
/* 04 */  "hierarchy_descr",
/* 05 */  "registration_descr",
/* 06 */  "data_stream_align_descr",
/* 07 */  "target_background_grid_descr",
/* 08 */  "video_window_descr",
/* 09 */  "CA_descr",
/* 0a */  "ISO_639_lang_descr",
/* 0b */  "syst_clk_descr",
/* 0c */  "multiplex_buf_utilization_descr",
/* 0d */  "copyright_descr",
/* 0e */  "max_bitrate_descr",
/* 0f */  "private_data_indicator_descr",
/* 10 */  "smoothing_buf_descr",
/* 11 */  "STD_descr",
/* 12 */  "IBP_descr",
/* 13 */ 0,
/* 14 */ "assoc_tag_descr",
};

static const char *desc_name2[] = {
/* 80 */  "stuffing_descr",
/* 81 */  "AC-3 audio descr",
/* 82 */ 0, 0, 0, 0, /* - 85 */
/* 86 */ "caption_serv_descr",
/* 87 */ "content_advisory_descr",
/* 88 */ 0, 0, 0, 0, 0, 0, 0, 0, /* - 8f */
/* 90 */ 0, 0, 0, 0, 0, 0, 0, 0, /* - 97 */
/* 98 */ 0, 0, 0, 0, 0, 0, 0, 0, /* - 9f */
/* a0 */ "extended_ch_name_descr",
/* a1 */ "serv_location_descr",
/* a2 */ "time_shifted_serv_descr",
/* a3 */ "component_name_descr",
/* a4 */ "data_service_descr",
/* a5 */ "PID_count_descr",
/* a6 */ "Download_descr",
/* a7 */ "multiprotocol_encapsulate_descr",
/* a8 */ "DCC_departing_event_descr",
/* a9 */ "DCC_arriving_event_descr",
/* aa */ "Content_Protect_Copy_Mngt_descr",
/* ab */ 0, 0, 0, 0, 0, 0, 0, 0, 0, /* - b3 */
/* b4 */ "Module_Link_descr",
/* b5 */ "CRC32_descr",
/* b6 */ 0, 0, /* - b7 */
/* b8 */ "Group_Link_descr",
};

/* print descr tag info */

static void prt_descr(uint32_t tag, int len)
{
  int n;
  const char *nm = 0;
  printf("descr tag/len=0x%02x/%u (", tag, len);
  if( tag < lengthof(desc_name1) )
    nm = desc_name1[tag];
  else if( (n=tag-0x80) >= 0 && n<=lengthof(desc_name2) )
    nm = desc_name2[n];
  if( nm == 0 ) nm = "n/a";
  printf("%s)\n",nm);
}

static void prt_vct(zvct_t *vct)
{
  printf("VCT table -\n");
  printf(" ver=0x%04x\n", vct->version);
  printf(" items=0x%04x\n", vct->items);
  printf(" ts_stream_id=0x%04x\n", vct->transport_stream_id);
  for( int i=0; i<vct->items; ++i ) {
    zvitem_t *item = vct->vct_tbl[i];
    printf(" %2d. name= ", i);
    for( int j=0; j<7; ++j ) {
      int ch = item->short_name[j];
      printf("%c", ch>=0x20 && ch<0x80 ? ch : '.');
    }
    printf("\n");
    printf(" maj =0x%04x", item->major_channel_number);
    printf(" min =0x%04x", item->minor_channel_number);
    printf(" mod =0x%04x", item->modulation_mode     );
    printf(" freq=0x%04x", item->carrier_frequency   );
    printf(" TSid=0x%04x", item->channel_TSID        );
    printf(" prog=0x%04x\n", item->program_number      );
    printf(" etm =0x%04x", item->etm_location        );
    printf(" acc =0x%04x", item->access_controlled   );
    printf(" hide=0x%04x", item->hidden              );
    printf(" serv=0x%04x", item->service_type        );
    printf(" s_id=0x%04x", item->source_id           );
    printf(" pcr =0x%04x\n", item->pcr_pid             );
    for( int k=0; k<item->num_ch_elts; ++k ) {
      printf("   %2d.", k);
      printf(" stream=0x%04x",item->elts[k].stream_id  );
      printf(" pid=0x%04x",item->elts[k].pes_pid    );
      int ch0 = item->elts[k].code_639[0];
      int ch1 = item->elts[k].code_639[1];
      int ch2 = item->elts[k].code_639[2];
      printf(" lang_code=%c",ch0>=0x20 && ch0<0x80 ? ch0 : '.');
      printf("%c",ch1>=0x20 && ch1<0x80 ? ch1 : '.');
      printf("%c",ch2>=0x20 && ch2<0x80 ? ch2 : '.');
      printf("\n");
    }
  }
}

static void prt_mgt(zmgt_t *mgt)
{
  printf("MGT table - items=%d\n", mgt->items);
  zmitem_t *item = mgt->mitems;
  for( int i=0; i<mgt->items; ++i,++item ) {
    printf("   %2d.", i);
    printf(" typ=0x%04x", item->type);
    char msg[16];
    uint16_t type = item->type;
     if( type >= 0x1500 ) sprintf(msg,"Reserved");
else if( type >= 0x1400 ) sprintf(msg,"DCCT %04x",type-0x1400);
else if( type >= 0x1000 ) sprintf(msg,"Reserved");
else if( type >= 0x0400 ) sprintf(msg,"Private%3d",type-0x0400);
else if( type >= 0x0301 ) sprintf(msg,"RRT %3d",type-0x0300);
else if( type >= 0x0280 ) sprintf(msg,"Reserved");
else if( type >= 0x0200 ) sprintf(msg,"ETT %3d",type-0x0200);
else if( type >= 0x0180 ) sprintf(msg,"Reserved");
else if( type >= 0x0100 ) sprintf(msg,"EIT %3d",type-0x0100);
else if( type >= 0x0006 ) sprintf(msg,"Reserved");
else if( type == 0x0005 ) sprintf(msg,"DCCSCT");
else if( type == 0x0004 ) sprintf(msg,"ChanETT");
else if( type == 0x0003 ) sprintf(msg,"CableVCT0");
else if( type == 0x0002 ) sprintf(msg,"CableVCT1");
else if( type == 0x0001 ) sprintf(msg,"TerraVCT0");
else if( type == 0x0000 ) sprintf(msg,"TerraVCT1");
    printf("(%-10s)",msg);
    printf(" pid=0x%04x", item->pid);
    printf(" ver=0x%04x", item->version);
    printf(" size=0x%04x", item->size);
    printf("\n");
  }
}

static void prt_stt(zdvb_t *dvb, zstt_t *stt)
{
  time_t tm = stt->system_time + dvb->stt_offset();
  printf("STT table  table_id=0x%04x - %s\n", stt->table_id, ctime(&tm));
}

static void prt_huf(zdvb_t *dvb,uint8_t *bp, int len)
{
  char *text = dvb->mstring(bp,len);
  if( text ) printf("%s",text);
  delete [] text;
}

static void prt_rrt(zrrt_t *rrt)
{
  int items = rrt->items;
  printf("RRT table - items=%d\n      region=", items);
  prt_huf(rrt->dvb,rrt->region_name,rrt->region_nlen);  printf("\n");
  zritem_t *rp = rrt->ratings;
  for( int i=0; i<items; ++i,++rp ) {
    printf("  %d  ",i);
    prt_huf(rrt->dvb,rp->dim_name,rp->dim_nlen);  printf("\n");
    printf("   grad %d, num=%d\n", rp->graduated_scale, rp->num_values);
    zrating_vt *ap = rp->abrevs;
    int values = rp->num_values;
    for( int j=0; j<values; ++j,++ap ) {
      printf("   %3d ", j);  prt_huf(rrt->dvb,ap->rating_name,ap->rating_nlen);
      printf(" = ");  prt_huf(rrt->dvb,ap->rating_text,ap->rating_tlen); printf("\n");
    }
  }
}

static void prt_ett(zdvb_t *dvb,zetext_t *etxt)
{
  printf("ETT table - table_id=0x%04x, etm_id=%08x, ",etxt->table_id, etxt->etm_id);
  printf("source_id=%04x, ", etxt->etm_id>>16);
  printf("event_id=%04x\n    ", (etxt->etm_id & 0xffff) >> 2);
  prt_huf(dvb,etxt->msg_text,etxt->msg_tlen); printf("\n");
}

static void prt_eit(zdvb_t *dvb,zeit_t *eit)
{
  printf("EIT table - source_id=0x%04x, items=%d\n", eit->source_id, eit->items);
  zeinfo_t *ep = eit->infos;
  for( int i=0; i<eit->items; ++i,++ep ) {
    time_t tm = ep->start_time + dvb->stt_offset();
    printf(" %2d.  id=%04x  %02d:%02d %s    | ", i, ep->event_id,
      ep->seconds/3600, (ep->seconds/60)%60, ctime(&tm));
    prt_huf(eit->dvb,ep->title_text,ep->title_tlen); printf("\n");
  }
}

#endif
// comment/uncomment to enable/disable prt
//#define dprintf(s...) printf(s)
#define dprintf(s...) do {} while(0)
#define prt_descr(s,t) do {} while(0)
#define prt_vct(s) do {} while(0)
#define prt_mgt(s) do {} while(0)
#define prt_stt(d,s) do {} while(0)
#define prt_rrt(s) do {} while(0)
#define prt_eit(d,s) do {} while(0)
#define prt_ett(d,s) do {} while(0)

zdvb_t::
dvb_t(zmpeg3_t *zsrc)
: src(zsrc),
  base_pid(0x1ffb)
{
  char *cp = getenv("DVB_STREAM_PROBE");
  empirical = cp ? atoi(cp) : 0;
  active = 0;
}

zdvb_t::
~dvb_t()
{
  reset();
}

void zdvb_t::
reset()
{
  stt_start_time = 0;
  delete mgt;  mgt = 0;
  while( eit ) { eit_t *nxt = eit->next;  delete eit;  eit = nxt; }
  while( ett ) { ett_t *nxt = ett->next;  delete ett;  ett = nxt; }
  delete tvct; tvct = 0;
  delete cvct; cvct = 0;
  delete rrt;  rrt = 0;
  delete stt;  stt = 0;
  base_pid.clear();
  active = 0;
}

void zdvb_t::
skip_bfr(int n)
{
  int len = bfr_len();
  if( n > len ) n = len;
  xbfr += n;
}

int zdvb_t::
get_text(uint8_t *dat, int bytes)
{
  int len = get8bits();
  int count = len < bytes ? len : bytes;
  for( int i=0; i<count; ++i ) *dat++ = get8bits();
  skp_bfr(len-count);
  return count;
}

void zdvb_t::
append_text(char *msg, int n)
{
  int len = strnlen(msg, n);
  int m = text_length + len + 1;
  if( m > text_allocated ) {
    int allocated = 2*text_allocated + len + 0x100;
    char *new_text = new char[allocated];
    text_allocated = allocated;
    memcpy(new_text, text, text_length);
    delete text;  text = new_text;
  }
  memmove(text+text_length,msg,len);
  text[text_length += len] = 0;
}

char *zdvb_t::
mstring(uint8_t *bp, int len)
{
  char msg[4096];
  text_length = text_allocated = 0;
  text = 0;  uint8_t *ep = bp + len;
  int num = !len ? 0 : *bp++;
  for( int i=0; i<num && bp<ep; ++i ) {
    //uint8_t enc[4];
    //enc[0] = *bp++; enc[1] = *bp++;
    //enc[2] = *bp++; enc[3] = 0;
    bp += 3; // enc
    int segs = *bp++;
    for( int j=0; j<segs; ++j ) {
      int type = *bp++;
      int mode = *bp++;  (void)mode;
      int bytes = *bp++;
      uint8_t *dat = bp;
      bp += bytes;
      switch( type ) {
      case 0x01: { // huffman 1
        int len = huf::huf1_decode(dat,bytes*8,msg,sizeof(msg));
        append_text(msg,len);
        break; }
      case 0x02: { // huffman 2
        int len = huf::huf2_decode(dat,bytes*8,msg,sizeof(msg));
        append_text(msg,len);
        break; }
      default: { // no compression
        append_text((char*)&dat[0],bytes);
        break; }
      }
    }
  }
  return text;
}

void zdvb_t::
skip_descr(int descr_length)
{
  if( descr_length > 0 && descr_length <= bfr_len() ) {
    int descr_bytes = 0;
    while( descr_bytes < descr_length ) {
      uint32_t tag = get8bits();
     (void)tag; /* avoid cmplr msg when debug off */
      int len = get8bits() + 2;
      int count = 2;
      prt_descr(tag, len);
      skp_bfr(len-count);
      descr_bytes += len;
    }
  }
}

void zmitem_t::
init(int len)
{
  if( len > bfr_alloc ) {
    delete [] bfr;
    bfr = new uint8_t[len];
    bfr_alloc = len;
  }
  bfr_size = 0;
  tbl_len = len;
}

void zmitem_t::
extract(uint8_t *dat, int len)
{
  if( bfr_size+len <= bfr_alloc || (len=bfr_alloc-bfr_size) > 0 ) {
    memcpy(bfr+bfr_size, dat, len);
    bfr_size += len;
  }
}

void zmitem_t::
clear()
{
  delete [] bfr;
  bfr = 0;
  bfr_size = bfr_alloc = 0;
}

void zmgt_t::
extract()
{
  clear();
  items = dvb->get16bits();
  mitems = new mitem_t[items];
  mitem_t *item = mitems;
  for( int i=0; i<items; ++i,++item ) {
    item->type = dvb->get16bits();
    item->pid = dvb->get16bits() & 0x1fff;
    item->version = dvb->get8bits() & 0x1f;
    item->size = dvb->get32bits();
    int descr_len = dvb->get16bits() & 0xfff;
    dvb->skip_descr(descr_len);
  }
  int descr_len = dvb->get16bits() & 0xfff;
  dvb->skip_descr(descr_len);
  prt_mgt(this);
}

void zmgt_t::
clear()
{
  delete [] mitems;  mitems = 0;
  items = 0;
}

zmitem_t *zmgt_t::
search(uint16_t pid)
{
  mitem_t *item = mitems;
  int i = items;
  while( --i>=0 && item->pid!=pid ) ++item;
  return i < 0 ? 0 : item;
}

void zch_elts_t::
extract(dvb_t *dvb)
{
  stream_id = dvb->get8bits();
  pes_pid = dvb->get16bits() & 0x1fff;
  code_639[0] = dvb->get8bits();
  code_639[1] = dvb->get8bits();
  code_639[2] = dvb->get8bits();
}

void zvitem_t::
extract(dvb_t *dvb)
{
  for( int i=0; i<7 ; ++i ) short_name[i] = dvb->get16bits();
  uint8_t b0 = dvb->get8bits();
  uint8_t b1 = dvb->get8bits();
  uint8_t b2 = dvb->get8bits();
  major_channel_number = ((b0&0x0f)<<6) | ((b1>>2)&0x3f);
  minor_channel_number = ((b1&0x03)<<8) | b2;
  modulation_mode = dvb->get8bits();
  carrier_frequency = dvb->get32bits();
  channel_TSID = dvb->get16bits();
  program_number = dvb->get16bits();
  b0 = dvb->get8bits();
  etm_location = (b0 >> 6) & 0x03;
  access_controlled = (b0 >> 5) & 1;
  hidden = (b0 >> 4) & 1;
  path_select = (b0 >> 3) & 1;
  out_of_band = (b0 >> 2) & 1;
  service_type = dvb->get8bits() & 0x3f;
  source_id = dvb->get16bits();
  num_ch_elts = 0;

  int descr_length = dvb->get16bits() & 0x3ff;
  int descr_bytes = 0;

  while( descr_bytes < descr_length ) {
    if( dvb->bfr_len() < 2 ) return;
    uint32_t tag = dvb->get8bits();
    int len = dvb->get8bits() + 2;
    prt_descr(tag, len);
    int count = 2;
    if( tag == 0xa1 ) {
      /* "Service Location Descriptor", extract common info */
      pcr_pid = dvb->get16bits() & 0x1fff;
      int num_elts = dvb->get8bits();
      count += 3;
      int n = num_elts;
      if( n > lengthof(elts) ) {
        n = lengthof(elts);
        printf("num of pids = %d -- truncated to %d\n", num_elts,n);
      }
      /* step thru each channel element & extract info into the VCT */
      num_ch_elts += n;
      for( int j=0; j<n; ++j )
        elts[j].extract(dvb);
      count += n*6;
    }
    dvb->skp_bfr(len-count);
    descr_bytes += len;
  }
}

int zvct_t::
search(zvitem_t &new_item)
{
  uint32_t maj = new_item.major_channel_number;
  uint32_t min = new_item.minor_channel_number;
  for( int i=items; --i>=0; ) {
    vitem_t *item = vct_tbl[i];
    if( maj == item->major_channel_number &&
        min == item->minor_channel_number ) return 1;
  }
  return 0;
}

void zvct_t::
append(zvitem_t &new_vitem)
{
  if( items >= items_allocated ) {
    int allocated = 2*items_allocated + 16;
    vitem_t **new_vct_tbl = new vitem_t *[allocated];
    items_allocated = allocated;
    for( int i=0; i<items; ++i ) new_vct_tbl[i] = vct_tbl[i];
    delete [] vct_tbl;  vct_tbl = new_vct_tbl;
  }
  vct_tbl[items++] = new vitem_t(new_vitem);
}

void zvct_t::
extract()
{
  transport_stream_id = dvb->stream_id;
  version = dvb->version;
  int nitems = dvb->get8bits();
  /* append new entries */
  for( int i=0; i<nitems; ++i ) {
    vct_t::vitem_t new_item;
    new_item.extract(dvb);
    if( !search(new_item) )
      append(new_item);
  }
  int descr_len = dvb->get16bits() & 0x3ff;
  dvb->skip_descr(descr_len);
  prt_vct(this);
}

void zvct_t::
clear()
{
  for( int i=0; i<items; ++i ) delete vct_tbl[i];
  delete [] vct_tbl;  vct_tbl = 0;
  items = items_allocated = 0;
}

void zrating_vt::
extract(dvb_t *dvb)
{
  rating_nlen = dvb->get_text(rating_name, lengthof(rating_name));
  rating_tlen = dvb->get_text(rating_text, lengthof(rating_text));
}

void zritem_t::
extract(dvb_t *dvb)
{
  clear();
  dim_nlen = dvb->get_text(dim_name,lengthof(dim_name));
  int len = dvb->get8bits();
  graduated_scale = (len>>4) & 1;
  num_values = len & 0x0f;
  abrevs = new rating_vt[num_values];
  for( int i=0; i<num_values; ++i )
    abrevs[i].extract(dvb);
}

void zritem_t::
clear()
{
  delete [] abrevs;  abrevs = 0;
  num_values = 0;
}

void zrrt_t::
extract()
{
  clear();
  version = dvb->version;
  region_nlen = dvb->get_text(region_name,lengthof(region_name));
  items = dvb->get8bits();
  ratings = new ritem_t[items];
  for( int i=0; i<items; ++i )
    ratings[i].extract(dvb);
  int descr_len = dvb->get16bits() & 0x3ff;
  dvb->skip_descr(descr_len);
  prt_rrt(this);
}

void zrrt_t::
clear()
{
  delete [] ratings;  ratings = 0;
  items = 0;
}


void zeinfo_t::
extract(dvb_t *dvb)
{
  event_id = dvb->get16bits() & 0x3fff;
  start_time = dvb->get32bits();
  uint32_t loc = dvb->get24bits();
  location = (loc>>20) & 0x3;
  seconds = loc & 0x0fffff;
  title_tlen = dvb->get8bits();
  title_text = new uint8_t[title_tlen];
  for( int i=0; i<title_tlen; ++i )
    title_text[i] = dvb->get8bits();
  int descr_len = dvb->get16bits() & 0xfff;
  dvb->skip_descr(descr_len);
}

void zeinfo_t::
clear()
{
  delete [] title_text;  title_text = 0;
  title_tlen = 0;
}

void zeit_t::
extract()
{
  clear();
  version = dvb->version;
  source_id = dvb->stream_id;
  nitems = dvb->get8bits();
  infos = new einfo_t[nitems];
  items = 0;
  for( int i=0; i<nitems; ++i ) {
    infos[items].extract(dvb);
    if( !search(infos[items].event_id) ) ++items;
    else infos[items].clear();
  }
  prt_eit(dvb,this);
}

void zeit_t::
clear()
{
  for( int i=0; i<items; ++i ) infos[i].clear();
  delete [] infos;  infos = 0;
  items = nitems = 0;
}

int zeit_t::
search(uint16_t evt_id)
{
  uint16_t src_id = id >> 16;
  for( eit_t *ep=dvb->eit; ep; ep=ep->next ) {
    if( (ep->id>>16) != src_id ) continue;
    for( int i=0; i<ep->items; ++i )
      if( ep->infos[i].event_id == evt_id ) return 1;
  }
  return 0;
}

void zetext_t::
extract(dvb_t *dvb, uint32_t eid)
{
  clear();
  version = dvb->version;
  table_id = dvb->stream_id;
  etm_id = eid;
  int tlen = dvb->bfr_len() - sizeof(uint32_t);
  msg_text = new uint8_t[tlen];
  msg_tlen = tlen;
  for( int i=0; i<msg_tlen; ++i )
    msg_text[i] = dvb->get8bits();
  prt_ett(dvb, this);
}

void zetext_t::
clear()
{
  delete [] msg_text;  msg_text = 0;
  msg_tlen = 0;
}

void zett_t::
extract()
{
  uint32_t eid = dvb->get32bits();
  int id = (eid & ~0xffff) | ((eid & 0xffff) >> 2);
  etext_t *etp = texts;
  while( etp && etp->id != id ) etp=etp->next;
  if( !etp ) { etp = new etext_t(id);  etp->next = texts;  texts = etp; }
  etp->extract(dvb, eid);
}

void zett_t::
clear()
{
  while( texts ) { etext_t *nxt = texts->next;  delete texts;  texts = nxt; }
}


void zstt_t::
extract()
{
  clear();
  version = dvb->version;
  table_id = dvb->stream_id;
  system_time = dvb->get32bits();
  utc_offset = dvb->get8bits();
  daylight_saving = dvb->get16bits();
  //doesnt seem to really be there
  //int descr_len = len - sizeof(uint32_t);
  //dvb->skip_descr(descr_len);
  dvb->skp_bfr(sizeof(uint32_t)); /*crc32*/
  prt_stt(dvb,this);
}

void zstt_t::
clear()
{
}


zmitem_t *zdvb_t::
atsc_pid(int pid)
{
  return pid==0x1ffb ? &base_pid : !mgt ? 0 : mgt->search(pid);
}

int zdvb_t::
atsc_tables(zdemuxer_t *demux, int pid)
{
  active = atsc_pid(pid);
  if( !active ) return 0;
  int dlen = demux->zdata.size;
//zmsgs("*** packet, pid = %04x, len=%04x\n", pid, dlen);
  uint8_t *dbfr = demux->zdata.buffer;
  uint8_t *dend = dbfr + dlen;
  while( dbfr < dend ) {
    if( demux->payload_unit_start_indicator ) {
      if( active->tbl_id ) {
        if( src->log_errs )
          zmsgs("atsc_tables, unfinished table %04x, len=%d\n",
            active->tbl_id, active->bfr_len());
//      extract();
      }
      xbfr = dbfr;  eob = dend;
      uint32_t tbl_id = get16bits();
      switch( tbl_id ) {
      case 0xc7: dprintf("MGT: ");     break;
      case 0xc8: dprintf("TVCT: ");    break;
      case 0xc9: dprintf("CVCT: ");    break;
      case 0xca: dprintf("RRT: ");     break;
      case 0xcb: dprintf("EIT: ");     break;
      case 0xcc: dprintf("ETT: ");     break;
      case 0xcd: dprintf("STT: ");     break;
      case 0xd3: dprintf("DCCT\n");    return 0;
      case 0xd4: dprintf("DCCSCT\n");  return 0;
      case 0xffff:                     return 0;
      default:   dprintf("tbl_id = 0x%02x\n",tbl_id);
        return 1;
      }
      /* read header */
      sect_len = get16bits() & 0x0fff;  /* 1,2 */
      if( (sect_len-=6) < 0 ) {
        zmsgs("sect_len=%d too small for header\n",sect_len);
        return 1;
      }
      stream_id = get16bits();          /* 3,4 */
      uint8_t byt = get8bits();         /* 5 */
      version = (byt >> 1) & 0x1f;
      cur_next = byt & 0x01;
      sect_num = get8bits();            /* 6 */
      proto_ver = get16bits() & 0xff;   /* 7,8 */
      dprintf("pid/tid=%04x/%02x  len=%04x strm=%04x ver=%d cur=%d sect=%d proto=%d\n",
        pid, tbl_id, sect_len, stream_id, version, cur_next, sect_num, proto_ver);
      if( proto_ver != 0 ) return 1;
      active->init(sect_len);
      active->tbl_id = tbl_id;
      active->src_id = stream_id;
      dbfr = xbfr;
    }
    else if( !active->tbl_id )
      return 1;

    /* append data to buffer */
    int len = dend - dbfr;
    int want = active->bfr_len();
    if( want < len ) len = want;
    if( len > 0 ) {
      active->extract(dbfr, len);
      dbfr += len;
    }
    dprintf("append tbl_id=%04x, len=%04x, size=%04x\n",
      active->tbl_id, len, active->bfr_size);

    /* if table assembled, extract data */
    if( !active->bfr_len() )
      extract();
  }

  return 0;
}

void zdvb_t::extract()
{
  xbfr = active->bfr;
  eob = xbfr + active->bfr_size;

  switch( active->tbl_id ) {
  case 0xc7:
    if( !mgt ) mgt = new mgt_t(this);
      mgt->extract();
      break;
  case 0xc8:
    if( !tvct ) tvct = new vct_t(this);
    tvct->extract();
    break;
  case 0xc9:
    if( !cvct ) cvct = new vct_t(this);
    cvct->extract();
    break;
  case 0xca:
    if( !rrt ) rrt = new rrt_t(this);
    rrt->extract();
    break;
  case 0xcb: { int id;
    if( (id=active->type-0x100) < 0 || id >= 0x80 ) break;
    id |= active->src_id << 16;
    eit_t *eip = eit;
    while( eip && eip->id != id ) eip = eip->next;
    if( !eip ) { eip = new eit_t(this, id);  eip->next = eit;  eit = eip; }
    eip->extract();
    break; }
  case 0xcc: { int id;
    if( active->type == 0x0004 ) id = -1; // channel ett
    else if( (id=active->type-0x200) < 0 || id >= 0x80 ) break;
    ett_t *etp = ett;
    while( etp && etp->id != id ) etp = etp->next;
    if( !etp ) { etp = new ett_t(this, id);  etp->next = ett;  ett = etp; }
    etp->extract();
    break; }
  case 0xcd:
    if( !stt ) stt = new stt_t(this);
    stt->extract();
    if( !stt_start_time )
      stt_start_time = stt->system_time;
    break;
  }

  active->tbl_id = 0;
}

int zdvb_t::
signal_time()
{
  return stt_start_time ? stt->system_time - stt_start_time : 0;
}

int zdvb_t::
channel_count()
{
  vct_t *vct = tvct ? tvct : cvct;
  return vct ? vct->items : -1;
}

int zdvb_t::
get_channel(int n, int &major, int &minor)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  major = item->major_channel_number;
  minor = item->minor_channel_number;
  return 0;
}

int zdvb_t::
get_station_id(int n, char *name)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  int i = 0;
  while( i < 7 ) {
    int ch = item->short_name[i];
    name[i] = ch >= 0x20 && ch < 0x80 ? ch : '_';
    ++i;
  }
  name[i] = 0;
  return 0;
}

int zdvb_t::
total_astreams(int n, int &count)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  count = 0;
  for( int i=0; i<item->num_ch_elts; ++i ) {
    int stream_id = item->elts[i].stream_id;
    if( stream_id == 0x81 ||  /* AC3-audio */
        stream_id == 0x03 ||  /* MPEG1-audio */
        stream_id == 0x04 ) { /* MPEG1-audio */
      ++count;
    }
  }
  return 0;
}

int zdvb_t::
astream_number(int n, int ord, int &stream, char *enc)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  vct_t::vitem_t::ch_elts_t *elt = 0;
  int count = 0;
  for( int i=0; i<item->num_ch_elts; ++i ) {
    int stream_id = item->elts[i].stream_id;
    if( stream_id == 0x81 || stream_id == 0x03 || stream_id == 0x04 ) {
      if( count == ord ) {
        elt = &item->elts[i];
        break;
      }
      ++count;
    }
  }
  if( !elt ) return 1;
  int pid = elt->pes_pid;
  int trk = 0;
  while( trk < src->total_atracks ) {
    if( src->atrack[trk]->pid == pid ) break;
    ++trk;
  }
  if( trk >= src->total_atracks ) return 1;
  stream = trk;
  if( enc ) {
    int ch0 = elt->code_639[0];
    int ch1 = elt->code_639[1];
    int ch2 = elt->code_639[2];
    enc[0] = ch0>=0x20 && ch0<0x80 ? ch0 : '.';
    enc[1] = ch1>=0x20 && ch1<0x80 ? ch1 : '.';
    enc[2] = ch2>=0x20 && ch2<0x80 ? ch2 : '.';
    enc[3] = 0;
  }
  return 0;
}

int zdvb_t::
total_vstreams(int n,int &count)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  count = 0;
  for( int i=0; i<item->num_ch_elts; ++i ) {
    int stream_id = item->elts[i].stream_id;
    if( stream_id == 0x02 || stream_id == 0x80 ) { /* video */
      ++count;
    }
  }
  return 0;
}

int zdvb_t::
vstream_number(int n, int ord, int &stream)
{
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return 1;
  vct_t::vitem_t *item = vct->vct_tbl[n];
  int pid = -1;
  int count = 0;
  for( int i=0; i<item->num_ch_elts; ++i ) {
    int stream_id = item->elts[i].stream_id;
    if( stream_id == 0x02 || stream_id == 0x80 ) {
      if( count == ord ) {
        pid = item->elts[i].pes_pid;
        break;
      }
      ++count;
    }
  }
  if( pid < 0 ) return 1;
  int trk = 0;
  while( trk < src->total_vtracks ) {
    if( src->vtrack[trk]->pid == pid ) break;
    ++trk;
  }
  if( trk >= src->total_vtracks ) return 1;
  stream = trk;
  return 0;
}

int zdvb_t::
read_dvb(zdemuxer_t *demux)
{
  vct_t *vct = tvct ? tvct : cvct;
  /* scan tables or use empirical data */
  if( vct && !empirical ) {
    memset(demux->astream_table,0,sizeof(demux->astream_table));
    memset(demux->vstream_table,0,sizeof(demux->vstream_table));
    /* mark active streams */
    for( int n=0; n<vct->items; ++n ) {
      vct_t::vitem_t *item = vct->vct_tbl[n];
      for( int i=0; i<item->num_ch_elts; ++i ) {
        int stream_id = item->elts[i].stream_id;
        int pid = item->elts[i].pes_pid;
//zmsgs("** dvb stream %02x pid %04x\n",stream_id,pid);
        switch( stream_id ) {
        case 0x80: /* video */
        case 0x02: /* video */
          demux->vstream_table[pid] = 1;
          break;
        case 0x03: /* MPEG1-audio */
        case 0x04: /* MPEG1-audio */
          demux->astream_table[pid] = afmt_MPEG;
          break;
        case 0x81: /* AC3-audio */
          demux->astream_table[pid] = afmt_AC3;
          break;
        }
      }
    }
  }
  return 0;
}

int zdvb_t::
get_chan_info(int n, int ord, int i, char *txt, int len)
{
  char *cp = txt;
  if( len > 0 ) *cp = 0;
  vct_t *vct = tvct ? tvct : cvct;
  if( !vct ) return -1;
  if( n < 0 || n >= vct->items ) return -1;
  zvitem_t *item = vct->vct_tbl[n];
  zeinfo_t *eip = 0;
  int eid = item->source_id << 16;
  if( ord >= 0 ) {  // not channel info
    int xid = eid | ord;
    eit_t *ep = eit;
    while( ep && ep->id != xid ) ep = ep->next;
    if( !ep || i >= ep->items ) return -1;
    eip = &ep->infos[i];
    time_t st = eip->start_time + stt_offset();
    time_t et = st + eip->seconds;
    struct tm stm;  localtime_r(&st, &stm);
    struct tm etm;  localtime_r(&et, &etm);
    if( len > 0 ) { // output start time
      n = snprintf(cp,len,"%02d:%02d:%02d-", stm.tm_hour, stm.tm_min, stm.tm_sec);
      cp += n;  len -= n;
    }
    if( len > 0 ) { // output end time
      n = snprintf(cp,len,"%02d:%02d:%02d  ", etm.tm_hour, etm.tm_min, etm.tm_sec);
      cp += n;  len -= n;
    }
    if( len > 0 ) { // output title text
      char *text = mstring(eip->title_text,eip->title_tlen);
      if( text ) {
        char *bp = text;
        while( *bp && len>0 ) {  // no new lines, messes up decoding title
          *cp++ = *bp != '\n' ? *bp : ';';
          --len;  ++bp;
        }
        if( len > 0 ) { *cp++ = '\n'; --len; }
        delete [] text;
      }
    }
    if( len > 0 ) { // output start weekday
      n = snprintf(cp,len,"(%3.3s) ",&"sunmontuewedthufrisat"[stm.tm_wday*3]);
      cp += n;  len -= n;
    }
    if( len > 0 ) { // output start date
      n = snprintf(cp,len,"%04d/%02d/%02d\n", stm.tm_year+1900, stm.tm_mon+1, stm.tm_mday);
      cp += n;  len -= n;
    }
    eid |= eip->event_id;
  }
  if( len > 0 && (eip ? eip->location : ord == -1) ) {
    ett_t *etp = ett;
    while( etp && etp->id != ord ) etp = etp->next;
    if( etp ) {    // output extended message text
      zetext_t *tp = etp->texts;
      while( tp && tp->id != eid ) tp = tp->next;
      if( tp ) {
        if( ord < 0 ) {  // channel ett exists
          n = snprintf(cp,len,"** ");
          cp += n;  len -= n;
        }
        char *text = mstring(tp->msg_text,tp->msg_tlen);
        if( text ) {
          n = snprintf(cp,len,"%s\n",text);
          cp += n;  len -= n;
          delete [] text;
        }
      }
    }
  }
  if( len > 0 ) *cp = 0;
  return cp - txt;
}

#endif
