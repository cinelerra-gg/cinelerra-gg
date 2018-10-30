#include "libzmpeg3.h"
// dvd sector size
#if defined(__x86_64__)
#define DVD_VIDEO_LB_LEN 2048L
#else
#define DVD_VIDEO_LB_LEN 2048LL
#endif

int zifo_t::
ifo_read(long pos, long count, uint8_t *data)
{
  int64_t ret = 0;
  int retry = 5;
  while( retry > 0 ) {
    ret = lseek(fd, pos, SEEK_SET);
    if( ret >= 0 ) break;
    perr("ifo_seek");
    sleep(1);
    --retry;
  }
  while( retry > 0 ) {
    ret = read(fd, data, count); 
    if( ret > 0 ) break;
   if( ret == 0 ) {
      zerr("unable to read ifo\n");
      return -1;
    }
    if( errno != EAGAIN ) {
      perr("read error");
      return -1;
    }
    sleep(1);
    --retry;
  }
  if( retry <= 0 ) {
    perr("timeout");
    return -1;
  }
  return ret;
}

int zifo_t::
get_table(int64_t offset, unsigned long tbl_id)
{
  int64_t len = 0;
  if( !offset ) return -1;
  uint8_t *zdata = new uint8_t[DVD_VIDEO_LB_LEN];
  uint64_t ipos = pos + offset*DVD_VIDEO_LB_LEN;
  int ret = ifo_read(ipos, DVD_VIDEO_LB_LEN, zdata);
  if( ret < 0 ) return -1;

  switch( tbl_id ) {
  case id_TITLE_VOBU_ADDR_MAP:
  case id_MENU_VOBU_ADDR_MAP:
    len = get4bytes(zdata) + 1;
    break;

  default: 
    len = hdr_length(zdata);
  }

  int ilen = len - DVD_VIDEO_LB_LEN;
  if( ilen > 0 ) {
    uint8_t *new_zdata = new uint8_t[len];
    memcpy(new_zdata,zdata,DVD_VIDEO_LB_LEN);
    memset(new_zdata+DVD_VIDEO_LB_LEN,0,len-DVD_VIDEO_LB_LEN);
    delete zdata;
    zdata = new_zdata;
    ipos += DVD_VIDEO_LB_LEN;
    ret = ifo_read(ipos, ilen, zdata+DVD_VIDEO_LB_LEN);
    if( ret < 0 ) return -1;
  }

  data[tbl_id] = zdata;

  if( tbl_id == id_TMT ) {
    uint32_t *ptr = (uint32_t*)zdata;
    len /= sizeof(*ptr);
    for( int i=0; i < len; ++i )
      ptr[i] = bswap_32(ptr[i]);
  }

  return 0;
}

zifo_t::
ifo_t(int zfd, long zpos)
{
  pos = zpos; 
  fd = zfd;
  char *cp = getenv("IFO_STREAM_PROBE");
  if( cp ) empirical = atoi(cp);
}

zifo_t::
~ifo_t()
{
  if( data[id_MAT] )                 delete data[id_MAT];
  if( data[id_PTT] )                 delete data[id_PTT];
  if( data[id_TITLE_PGCI] )          delete data[id_TITLE_PGCI];
  if( data[id_MENU_PGCI] )           delete data[id_MENU_PGCI];
  if( data[id_TMT] )                 delete data[id_TMT];
  if( data[id_MENU_CELL_ADDR] )      delete data[id_MENU_CELL_ADDR];
  if( data[id_MENU_VOBU_ADDR_MAP] )  delete data[id_MENU_VOBU_ADDR_MAP];
  if( data[id_TITLE_CELL_ADDR] )     delete data[id_TITLE_CELL_ADDR];
  if( data[id_TITLE_VOBU_ADDR_MAP] ) delete data[id_TITLE_VOBU_ADDR_MAP];
}

int zifo_t::
init_tables()
{
  num_menu_vobs  = vtsm_vobs();
  vob_start = vtstt_vobs();
//zmsgs("num of vobs: %x vob_start %x\n", num_menu_vobs, vob_start);
  if( !type_vts() ) {
    if( get_table(vts_ptt_srpt(), id_PTT) < 0 ||
        get_table(vts_pgcit(), id_TITLE_PGCI) < 0 ||
//        get_table(vtsm_pgci_ut(), id_MENU_PGCI) < 0 ||
//        get_table(vts_tmapt(), id_TMT) < 0 ||
//        get_table(vtsm_c_adt(), id_MENU_CELL_ADDR) < 0 ||
//        get_table(vtsm_vobu_admap(), id_MENU_VOBU_ADDR_MAP) < 0 ||
        get_table(vts_c_adt(), id_TITLE_CELL_ADDR) < 0 ||
        get_table(vts_vobu_admap(), id_TITLE_VOBU_ADDR_MAP) < 0 )
      return -1;
  } 
  else if( !type_vmg() ) {
    if( get_table(tt_srpt(), id_TSP) < 0 ||
        get_table(vmgm_pgci_ut(), id_MENU_PGCI) < 0 ||
        get_table(vts_atrt(), id_TMT) < 0 ||
        get_table(vmgm_c_adt(), id_TITLE_CELL_ADDR) < 0 ||
        get_table(vmgm_vobu_admap(), id_TITLE_VOBU_ADDR_MAP) < 0 )
      return -1;
  }
  return 0;
}

int zifo_t::
read_mat()
{
  data[id_MAT] = new uint8_t[DVD_VIDEO_LB_LEN];
  memset(data[id_MAT],0,DVD_VIDEO_LB_LEN);
  return ifo_read(pos, DVD_VIDEO_LB_LEN, data[id_MAT]) < 0 ? -1 : 0;
}

zifo_t* zmpeg3_t::
ifo_open(int fd, long pos)
{
  ifo_t *ifo = new ifo_t(fd,pos); 
  if( ifo->read_mat() || ifo->init_tables() ) {
    delete ifo;
    ifo = NULL;
  }
  return ifo;
}

int zifo_t::
ifo_close()
{
  delete this;
  return 0;
}

void zifo_t::
get_palette(zmpeg3_t *zsrc)
{
  if( !zsrc->have_palette ) {
    /* subtitle color palette */
    int ofs = 0;
    uint8_t *ptr = palette();
    for( int i=0; i<16; ++i ) {
      int y = (int)ptr[ofs+1];
      int u = (int)ptr[ofs+2];
      int v = (int)ptr[ofs+3];
      int k = i*4;
      zsrc->palette[k+0] = y;
      zsrc->palette[k+1] = u;
      zsrc->palette[k+2] = v;
//zmsgs("color %02d: 0x%02x 0x%02x 0x%02x\n", i, y, u, v);
      ofs += 4;
    }
    zsrc->have_palette = 1;
  }
}

void zifo_t::
get_playlist(zmpeg3_t *zsrc)
{
  DIR *dirstream;
  char directory[STRLEN];
  char filename[STRLEN];
  char full_path[STRLEN];
  char title_path[STRLEN];
  char vob_prefix[STRLEN];
  struct dirent *new_filename;
  demuxer_t *demux = zsrc->demuxer;

  /* Get titles matching ifo file */
  complete_path(full_path, zsrc->fs->path);
  get_directory(directory, full_path);
  get_filename(filename, full_path);
  strncpy(vob_prefix, filename, 6);

  dirstream = opendir(directory);
  while( (new_filename=readdir(dirstream)) != 0 ) {
    if( !strncasecmp(new_filename->d_name, vob_prefix, 6) ) {
      char *ptr = strrchr(new_filename->d_name, '.');
      if( ptr && !strncasecmp(ptr, ".vob", 4) ) {
        /* Got a title */
        if( atol(&new_filename->d_name[7]) > 0 ) {
          joinpath(title_path, directory, new_filename->d_name);
          demuxer_t::title_t *title = new demuxer_t::title_t(zsrc, title_path);
          demux->titles[demux->total_titles++] = title;
//zmsgs("title_path=%s\n", title_path);
        }
      }
    }
  }
  closedir(dirstream);

  int done = 0;
  while( !done ) {
    done = 1;
    for( int i=1; i<demux->total_titles; ++i ) {
      char *i0 = demux->titles[i-0]->fs->path;
      char *i1 = demux->titles[i-1]->fs->path;
      if( strcmp(i1, i0) > 0 ) {
        ztitle_t *title = demux->titles[i-0];
        demux->titles[i-0] = demux->titles[i-1];
        demux->titles[i-1] = title;
        done = 0;
      }
    }
  }

  int64_t total_bytes = 0;
  for( int i=0; i<demux->total_titles; ++i ) {
    ztitle_t *title = demux->titles[i-0];
    title->total_bytes = path_total_bytes(title->fs->path);
    title->start_byte = total_bytes;
    title->end_byte = total_bytes + title->total_bytes;
    total_bytes += title->total_bytes;
  }
}

/* major kludge, but some dvds have very damaged data */
#define TEST_START 0x1000000
#define TEST_LEN   0x1000000

void zifo_t::
get_header(zdemuxer_t *demux)
{
  int i;
  /* Video header */
  demux->vstream_table[0] = 1;
  demux->open_title(0);

  if( empirical ) {
    demux->seek_byte(TEST_START);
    demux->read_all = 1;
    int result = 0;
    while( !result && !demux->eof() &&
           demux->tell_byte() < TEST_START+TEST_LEN ) {
      result = demux->read_next_packet();
    }
    demux->seek_byte(0);
    demux->read_all = 0;
  }
  else {
    /* Audio header */
    if( !type_vts() ) {
      int atracks = nr_of_vts_audio_streams();
      if( atracks > 0 ) {
        for( i=0; i<atracks; ++i ) {
          uint8_t *audio_attr = vts_audio_attr(i);
          int audio_mode = afmt_IGNORE;
          switch( aud_audio_format(audio_attr) ) {
          case 0:  audio_mode = afmt_AC3;   break;
          case 1:  audio_mode = afmt_MPEG;  break;
          case 2:  audio_mode = afmt_MPEG;  break;
          case 3:  audio_mode = afmt_PCM;   break;
          }
          if( !demux->astream_table[i+0x80] )
            demux->astream_table[i+0x80] = audio_mode;
        }
      }
    }
    else if( !type_vmg() ) {
      int atracks = nr_of_vmgm_audio_streams();
      if( atracks > 1 ) zerr("too many atracks for vmgm header\n");
      if( atracks > 0 ) {
        uint8_t *audio_attr = vmgm_audio_attr();
        int audio_mode = afmt_IGNORE;
        switch( aud_audio_format(audio_attr) ) {
        case 0:  audio_mode = afmt_AC3;   break;
        case 1:  audio_mode = afmt_MPEG;  break;
        case 2:  audio_mode = afmt_MPEG;  break;
        case 3:  audio_mode = afmt_PCM;   break;
        }
        if( !demux->astream_table[0x80] )
          demux->astream_table[0x80] = audio_mode;
      }
    }
    /* subtitle header */
    if( !type_vts() ) {
      int stracks = nr_of_vts_subp_streams();
      if( stracks > 0 ) {
        for( i=0; i<stracks; ++i )
          demux->sstream_table[i] = 1;
      }
    }
    else if( !type_vmg() ) {
      int stracks = nr_of_vmgm_subp_streams();
      if( stracks > 1 ) zerr("too many stracks for vmgm header\n");
      if( stracks > 0 ) {
        demux->sstream_table[0] = 1;
      }
    }
  }
}

zicell_t* zicell_table_t::
append_cell()
{
  if( !cells || total_cells >= cells_allocated ) {
    int new_allocation = ZMAX(cells_allocated*2, 64);
    icell_t *new_cells = new icell_t[new_allocation];
    for( int i=0; i<total_cells; ++i )
      new_cells[i] = cells[i];
    delete [] cells;
    cells = new_cells;
    cells_allocated = new_allocation;
  }
  return &cells[total_cells++];
}

zicell_table_t::
~icell_table_t()
{
  if( cells ) delete [] cells;
}

void zifo_t::
get_playinfo(zmpeg3_t *zsrc, icell_table_t *icell_addrs)
{
  icell_table_t *pcells = new icell_table_t();
  if( zsrc->playinfo ) delete zsrc->playinfo;
  zsrc->playinfo = pcells;
  int total_pcells = nr_of_cells();
  int cur_angle = -1;
  double cur_time = 0.;
//zmsgs("total_pcells %d\n", total_pcells);

  for( int pcell_no=0; pcell_no<total_pcells; ++pcell_no ) {
    use_playback(pcell_no);
    int phr = bcd(cell_playback_time()[0]);
    int pmn = bcd(cell_playback_time()[1]);
    int psc = bcd(cell_playback_time()[2]);
    int pfm = bcd(cell_playback_time()[3] & 0x3f);
    int prt = cell_playback_time()[3] >> 6;
#if 0
zmsgs(" cell: %d\n"
  "   blk_ty %u, seemless %u, interlv %u, discon %u, angle %u @secs %f\n"
  "   mode %u, restrict %u, still %u, cmd_nr %u, ptime %02u:%02u:%02u.%02u %5.5s\n"
  "   1st sect %jx, end sect %jx, start sect %jx, last sect %jx\n",
  pcell_no+1,
  cell_block_type(), cell_seamless_play(), cell_interleaved(),
  cell_stc_discontinuity(), cell_seamless_angle(), cur_time,
  cell_playback_mode(), cell_restricted(), cell_still_time(),
  cell_cmd_nr(), phr, pmn, psc, pfm, &"0.0  25.00     29.97"[5*prt],
  cell_first_sector()*DVD_VIDEO_LB_LEN,
  cell_first_ilvu_end_sector()*DVD_VIDEO_LB_LEN,
  cell_last_vobu_start_sector()*DVD_VIDEO_LB_LEN,
  cell_last_sector()*DVD_VIDEO_LB_LEN);
#endif
    icell_t *pcell = pcells->append_cell();
    pcell->start_byte = cell_first_sector()*DVD_VIDEO_LB_LEN;
    pcell->end_byte = (cell_last_sector()+1)*DVD_VIDEO_LB_LEN;
    pcell->discon = cell_stc_discontinuity();
    pcell->inlv = cell_interleaved();
    if( cell_block_type() == BTY_ANGLE ) {
      switch( cell_block_mode() ) {
      case BMD_FIRST_CELL: cur_angle=0; break;
      case BMD_IN_BLOCK:
      case BMD_LAST_CELL:  ++cur_angle; break;
      }
    }
    else
      cur_angle = -1;
    pcell->angle = cur_angle;
    if( !pcell->inlv || cur_angle == 0 )
      cur_time += phr*3600 + pmn*60 + psc + pfm/(prt==1 ? 25.0 : 29.97);
    use_position(pcell_no);
    pcell->vob_id = vob_id_nr();
    pcell->cell_id = cell_id_nr();
//zmsgs(" %3d vob/cell %d/%d  start: %jx end: %jx discon %d inlv/ang %d/%d\n",
//  pcell_no+1, pcell->vob_id, pcell->cell_id,
//  pcell->start_byte, pcell->end_byte, pcell->discon, pcell->inlv,pcell->angle);
  }
}

void zifo_t::
icell_addresses(zicell_table_t *icell_addrs)
{
  int i, j;
//zmsg("icell_addresses\n");
  int length = cadr_length();
//zmsgs("icells length %d\n",length);

  for( i=0; i<length; ++i ) {
    use_cell(i);
    icell_t *cell = icell_addrs->append_cell();
    cell->start_byte = cadr_start_sector() * DVD_VIDEO_LB_LEN;
    cell->end_byte = (cadr_last_sector()+1) * DVD_VIDEO_LB_LEN;
    cell->vob_id = cadr_vob_id();
    cell->cell_id = cadr_cell_id();
    cell->discon = i;
    cell->inlv = 0;
    cell->angle = 0;
//zmsgs("cell %d, vob_id %u, cell_id %u, start %lu/%lx, last %lu/%lx\n",
//  i, cell->vob_id, cell->cell_id, cell->start_byte, cell->start_byte,
//  cell->end_byte, cell->end_byte);
  }

  int total_icells = icell_addrs->total_cells;
  icell_t *icells[total_icells];
  for( i=total_icells; --i>=0; ) icells[i] = &icell_addrs->cells[i];

// Sort addresses by addr/vob_id/cell_id instead of vob id
  int done = 0;
  while( !done ) {
    done = 1;
    for( i=1; i<total_icells; ++i ) {
      icell_t *icell0 = icells[i-0];
      icell_t *icell1 = icells[i-1];
      // key addr/vob_no/cell_id
      if( icell1->start_byte > icell0->start_byte ||
         (icell1->start_byte == icell0->start_byte &&
          (icell1->vob_id > icell0->vob_id ||
           (icell1->vob_id == icell0->vob_id &&
            icell1->cell_id > icell0->cell_id))) ) {
        icells[i-0] = icell1;
        icells[i-1] = icell0;
        done = 0;
      }
    }
  }

#if 0
  int cur_vobid = 0x10000;
  /* label interleave */
  for( i=total_icells; --i>=0; ) {
    icell_t *icell = icells[i];
    int cur_inlv = icell->vob_id - cur_vobid;
    /* Reduce current vobid */
    if( cur_inlv < 0 ) {
      cur_vobid = icell->vob_id;
      cur_inlv = 0;
    }
    icell->inlv = cur_inlv;
    if( max_inlv < cur_inlv )
      max_inlv = cur_inlv;
    if( cur_inlv > 0 ) {
      int cell_id = icell->cell_id;
      /* Get the last interleave by brute force */
      for( j=i+1; j<total_icells && cell_id==icells[j]->cell_id; ++j ) {
        int inlv = icells[j]->vob_id - cur_vobid;
        if( inlv > cur_inlv ) continue;
        icells[j]->inlv = inlv;
      }
    }
    else if( cur_inlv == 0 ) {
      int64_t start_byte = icells[i]->start_byte;
      for( j=i+1; j<total_icells && start_byte==icells[j]->start_byte; ++j )
        icells[j]->inlv = j-i;
    }
  }

  /* convert int labels to bit vector */
  i = 0;
  while( i < total_icells ) {
    j = i;
    int64_t start_byte = icells[j]->start_byte;
    uint32_t inlv = 1 << icells[j]->inlv;
    while( ++j<total_icells && start_byte==icells[j]->start_byte )
      inlv |= 1 << icells[j]->inlv;
    while( i < j ) icells[i++]->inlv = inlv;
  }

#else
  int istart = -1, istop = -1;
// Assign interleave:
//  there are 2 cases,
//    interleave is known, using vob_id offset,
//    interleave is unknown, cell may be in any program
//  unsorted vob_ids:   1111122233
//    sorted vob_ids:   1112121233
//  equal start_byte:   ***xxxx***
//        interleave:   xx010101xx
  i = 0;
  while( i < total_icells ) {
    istart = i;
    int cur_vob_id = icells[istart]->vob_id;
    while( ++i < total_icells ) {
      if( icells[i]->vob_id > cur_vob_id ) {
        if( icells[i]->start_byte == icell_addrs->cells[i].start_byte ) break;
        cur_vob_id = icells[i]->vob_id;
      }
    }
    istop = i;
    if( istop == istart+1 ) {
      icells[istart]->inlv = ~0;
      continue;
    }
    int min_vob_id = icells[istart]->vob_id;
    int cur_inlv = cur_vob_id - min_vob_id;
    if( cur_inlv > max_inlv ) max_inlv = cur_inlv;
    int inlv = 0;
    j = istart;
    while( j < istop ) {
      int64_t start_byte = icells[j]->start_byte;
      if( j+1 < istop && icells[j+1]->start_byte == start_byte ) {
        inlv = -1;
        while( j < istop && icells[j]->start_byte == start_byte )
          icells[j++]->inlv = ~0;
      }
      else {
        int k = icells[j]->vob_id - min_vob_id;
        if( inlv >= 0 && k != inlv )
          zerrs("bolixed: j %d, inlv %d, k %d\n", j, inlv, k);
        icells[j++]->inlv = 1 << k;
        inlv = k;
        if( ++inlv > cur_inlv ) inlv = 0;
      }
    }
    if( inlv > 0 ) {
      zerrs("bungled: istart %d, istop %d, cur_inlv %d\n",
        istart, istop, cur_inlv);
    }
  }
#endif

#if 0
zmsgs("sorted labeled icells %d\n",total_icells);
  for( i=0; i<total_icells; ++i ) {
    icell_t *cell = icells[i];
    zmsgs("cell %d, vob_id %u, cell_id %u, start %jx, last %jx inlv %04x\n",
      i, cell->vob_id, cell->cell_id, cell->start_byte, cell->end_byte, cell->inlv);
  }
#endif
}

static int pcmpr(const void *ap, const void *bp)
{
  zicell_t *a = *(zicell_t**)ap, *b = *(zicell_t**)bp;
  int v = a->vob_id - b->vob_id;
  if( !v ) v = a->cell_id - b->cell_id;
  return v;
}

void zifo_t::
icell_map(zmpeg3_t *zsrc, icell_table_t *icell_addrs)
{
  int inlv = zsrc->interleave;
  int angle = zsrc->angle;
  demuxer_t *demuxer = zsrc->demuxer;
  int64_t program_start = 0;
  int discon = 0;
  int total_icells = icell_addrs->total_cells;
  icell_t *icells = &icell_addrs->cells[0];
  int total_pcells = zsrc->playinfo->total_cells;
  icell_t *pcells[total_pcells];
  for( int i=0; i<total_pcells; ++i ) pcells[i] = &zsrc->playinfo->cells[i];
  qsort(pcells, total_pcells, sizeof(pcells[0]), pcmpr);

  for( int i=0; i<total_pcells; ++i ) {
    icell_t *pcell = pcells[i];
    int pcell_no = pcell - zsrc->playinfo->cells;
    if( pcell->angle >= 0 && pcell->angle != angle ) continue;
    int icell_no, pgm = pcell->angle >= 0 ? angle : inlv;
    icell_t *cell = 0;
    for( icell_no=0; icell_no<total_icells; ++icell_no ) {
      cell = &icells[icell_no];
      if( pcell->inlv && !cell->has_inlv(pgm) ) continue;
      if( cell->vob_id != pcell->vob_id ) continue;
      if( cell->cell_id == pcell->cell_id ) break;
    }
    if( !cell || icell_no >= total_icells ) {
      zerrs("vob/cell %d/%d missed in icells\n", pcell->vob_id, pcell->cell_id);
      continue;
    }

    if( current_vob_id != cell->vob_id ) {
      current_vob_id = cell->vob_id;
      // -1 resets pts to 0, 1 is just discontinuity
      // discon = cell->cell_id == 1 ? -1 : 1;
      discon = 1;
    }
    else if( pcell->discon )
      discon = 1;

    while( icell_no < total_icells ) {
      cell = &icells[icell_no++];
      if( pcell->inlv && !cell->has_inlv(pgm) ) continue;
      if( cell->vob_id != pcell->vob_id ) break;
      if( cell->cell_id != pcell->cell_id ) break;
      if( cell->start_byte >= pcell->end_byte ) continue;
      if( cell->end_byte <= pcell->start_byte ) continue;
      int64_t start_byte = cell->start_byte;
      if( start_byte < pcell->start_byte )
        start_byte = pcell->start_byte;
      int64_t end_byte = cell->end_byte;
      if( end_byte > pcell->end_byte )
        end_byte = pcell->end_byte;

      int64_t length = end_byte - start_byte;
      while( length > 0 ) {
        /* Cell may be split by a title so handle in fragments. */
        int title_no;
        for( title_no=0; title_no<demuxer->total_titles; ++title_no )
          if( demuxer->titles[title_no]->end_byte > start_byte ) break;
        if( title_no >= demuxer->total_titles ) {
          zerrs("cell map past titles %jx\n", start_byte);
          break;
        }
        ztitle_t *title = demuxer->titles[title_no];
        int64_t cell_size = length;
        /* Clamp length to current title */
        if( start_byte+length > title->end_byte )
          cell_size = title->end_byte - start_byte;
        int64_t title_start = start_byte - title->start_byte;
        int64_t title_end = title_start + cell_size;
        int64_t program_end = program_start + cell_size;
//zmsgs("  %d/%d. title/cell: %d/%-2d, vob/cell: %2d/%-2d,  title 0x%012lx-0x%012lx, "
//      " program 0x%012lx-0x%012lx discon %d\n",
//  pcell_no+1, icell_no, title_no, title->cell_table_size, cell->vob_id, cell->cell_id,
//  title_start, title_end, program_start, program_end, discon);
        title->new_cell(pcell_no, title_start, title_end,
                        program_start, program_end, discon);
        discon = 0;
        start_byte += cell_size;
        program_start += cell_size;
        length -= cell_size;
      }

      current_byte = end_byte;
    }
  }
}

void zifo_t::
get_ititle(zmpeg3_t *zsrc, int chapter)
{
  int title = zsrc->vts_title;
  use_ptt_info(title);
  int program_chain = ptt_pgcn(chapter);
  use_program_chain(program_chain-1);
}

/* Read the title information from an ifo */
int zmpeg3_t::
read_ifo()
{
  ifo_t *ifo;
  icell_table_t icell_addrs;
  int result = 0;

  int fd = fs->get_fd();
  if( (ifo = ifo_open(fd, 0)) == 0 ) {
    zerr("Error opening ifo.\n");
    result = 1;
  }
  if( !result ) {
    ifo->get_playlist(this);
    if( !demuxer->total_titles ) {
      zerr("Error no titles in ifo.\n");
      result = 1;
    }
  }
  if( !result ) {
    total_vts_titles = ifo->nr_of_titles();
    if( vts_title >= total_vts_titles ) {
      zerrs("Error only %d titles in ifo, req %d\n",
        total_vts_titles, vts_title);
      result = 1;
    }
  }
  if( !result ) {
    ifo->get_header(demuxer);
    ifo->get_ititle(this);
    ifo->get_palette(this);
    ifo->icell_addresses(&icell_addrs);
    total_interleaves = ifo->max_inlv + 1;
    if( interleave >= total_interleaves ) {
      zerrs("Error only %d interleaves in ifo, req %d\n",
        total_interleaves, interleave);
      result = 1;
    }
  }
  if( !result ) {
    ifo->get_playinfo(this, &icell_addrs);
    ifo->icell_map(this, &icell_addrs);
  }

  if( ifo ) ifo->ifo_close();
  return result;
}

/* This is not useful, since title is based on the vgm tt_srpt table */
/*   and that table is not normally read.  written for testing purposes. */
/*   the orignal plan was to be able to put keyframe markers at chapters */
int64_t zifo_t::ifo_chapter_cell_time(zmpeg3_t *zsrc, int chapter)
{
  double result = -1;
  int title = zsrc->vts_title;
  if( chapter >= 0 && chapter < (int)nr_of_chapters(title) ) {
    get_ititle(zsrc, chapter);
    int pgn = ptt_pgn(chapter);
    uint8_t *pmap = pgc_program_map();
    int cell_id = pmap[pgn-1] - 1;
    int n = zsrc->playinfo->total_cells;
    int i = 0;
    while( i < n ) {
      if( zsrc->playinfo->cells[i].cell_id == cell_id ) break;
      ++i;
    }
    if( i < n ) {
      use_playback(i);
      result = cell_first_sector() * DVD_VIDEO_LB_LEN;
    }
  }
  return result;
}

/* check playcells */
int zifo_t::
chk(int vts_title, int chapter, int inlv, int angle,
    icell_table_t *icell_addrs, int &sectors, int &pcells, int &max_angle)
{
  if( type_vts() ) return -1;
  if( vts_title >= (int)nr_of_titles() ) return -1;
  if( chapter >= (int)nr_of_chapters(vts_title) ) return 0;
  use_ptt_info(vts_title);
  int program_chain = ptt_pgcn(chapter);
  use_program_chain(program_chain-1);
  int total_pcells = nr_of_cells();
  if( total_pcells <= 0 ) return 0;
  int pgn = ptt_pgn(chapter);
  uint8_t *pmap = pgc_program_map();
  int pcell_no = pmap[pgn-1] - 1;
  use_playback(pcell_no);
  uint32_t current_sector = cell_first_sector();
  int count = 0;

  int total_icells = icell_addrs->total_cells;
  icell_t *icells = &icell_addrs->cells[0];
  int last_vob_id = 0;
  int last_cell_id = 0;
  max_angle = -1;
  int cur_angle = -1;

  for( int pcell_no=0; pcell_no<total_pcells; ++pcell_no ) {
    // vob/cell id keys, must be increasing
    use_position(pcell_no);
    int vob_id = vob_id_nr();
    int cell_id = cell_id_nr();
    if( vob_id != last_vob_id )
      last_vob_id = vob_id;
    else if( cell_id < last_cell_id )
      return 0;
    last_cell_id = cell_id;

    use_playback(pcell_no);
    int interleaved = cell_interleaved();
    int pgm = inlv;
    if( cell_block_type() == BTY_ANGLE ) {
      switch( cell_block_mode() ) {
      case BMD_FIRST_CELL: cur_angle=0;  break;
      case BMD_IN_BLOCK:
      case BMD_LAST_CELL:  ++cur_angle;  break;
      }
      if( cur_angle > max_angle ) max_angle = cur_angle;
      if( cur_angle != angle ) continue;
      pgm = cur_angle;
    }
    else
      cur_angle = -1;
    // vob/cell/interleave must be in map
    icell_t *cell = 0;
    int icell_no;
    for( icell_no=0; icell_no<total_icells; ++icell_no ) {
      cell = &icells[icell_no];
      if( interleaved && !cell->has_inlv(pgm) ) continue;
      if( cell->vob_id != vob_id ) continue;
      if( cell->cell_id == cell_id ) break;
    }
    if( !cell || icell_no >= total_icells ) return 0;

    // check sectors, must be increasing
    uint32_t first_sector = cell_first_sector();
    if( first_sector && current_sector > first_sector ) return 0;
    uint32_t last_sector1 = cell_last_sector()+1;
    count += last_sector1 - first_sector;
    current_sector = last_sector1;

    // pcell must overlap cell
    if( first_sector*DVD_VIDEO_LB_LEN >= cell->end_byte ) return 0;
    if( cell->start_byte >= last_sector1*DVD_VIDEO_LB_LEN ) return 0;
  }

  sectors = count;
  pcells = total_pcells;

  return 1;
}

