#include "libzmpeg3.h"

ztitle_t::
title_t(zmpeg3_t *zsrc, char *fpath)
{
  fs = new fs_t(zsrc, fpath);
  src = zsrc;
}

ztitle_t::
title_t(zmpeg3_t *zsrc)
{
  fs = new fs_t(*zsrc->fs);
  src = zsrc;
}

ztitle_t::
~title_t()
{
  delete fs;
  if( cell_table_allocation )
    delete [] cell_table;
}

ztitle_t::
title_t(ztitle_t &title)
{
  src = title.src;
  fs = new fs_t(*title.fs);
  total_bytes = title.total_bytes;
  start_byte = title.start_byte;
  end_byte = title.end_byte;

  if( title.cell_table_size && title.cell_table ) {
    cell_table_allocation = title.cell_table_allocation;
    cell_table = new cell_t[cell_table_allocation];
    cell_table_size = title.cell_table_size;
    for( int i=0; i<cell_table_size; ++i ) {
      cell_table[i] = title.cell_table[i];
    }
  }
}

int ztitle_t::
dump_title()
{
  printf("path %s 0x%jx-0x%jx cell_table_size %d\n", 
    fs->path, start_byte, end_byte, cell_table_size);
  for( int i=0; i<cell_table_size; ++i ) {
    printf("0x%jx-0x%jx 0x%jx-0x%jx\n", 
      cell_table[i].title_start, cell_table[i].title_end, 
      cell_table[i].program_start, cell_table[i].program_end);
  }
  return 0;
}

void ztitle_t::
extend_cell_table()
{
  if( !cell_table || cell_table_allocation <= cell_table_size ) {
    long new_allocation = cell_table_allocation ?
       2*cell_table_size : 64;
    cell_t *new_table = new cell_t[new_allocation];
    for( int i=0; i<cell_table_size; ++i )
      new_table[i] = cell_table[i];
    delete [] cell_table;
    cell_table = new_table;
    cell_table_allocation = new_allocation;
  }
}

void ztitle_t::
new_cell(int cell_no, int64_t title_start, int64_t title_end,
  int64_t program_start, int64_t program_end, int discontinuity)
{
  extend_cell_table();
  cell_t *cell = &cell_table[cell_table_size];
  cell->cell_no = cell_no;
  cell->title_start = title_start;
  cell->title_end = title_end;
  cell->program_start = program_start;
  cell->program_end = program_end;
  cell->cell_time = -1.;
  cell->discontinuity = discontinuity;
  ++cell_table_size;
}

int ztitle_t::
print_cells(FILE *output)
{
  if( cell_table ) {
    for( int i=0; i<cell_table_size; ++i ) {
      cell_t *cell = &cell_table[i];
      fprintf(output, "REGION: 0x%jx-0x%jx 0x%jx-0x%jx\n",
        cell->program_start, cell->program_end,
        cell->title_start, cell->title_end);
    }
  }
  return 0;
}

