
#include <stdio.h>
#include "libzmpeg3.h"

/* reads a ifo file which must be a dvd, and vaidates the title data */
/*   reports title/program sets which appear to be usable */

int usage(const char *nm)
{
  printf("usage: %s <path>\n"
    "  <path> = path to dvd mount point\n", nm);
  return 0;
}

int main(int argc, char **argv)
{
  if( argc < 2 ) 
    return usage(argv[0]);

  int verbose = 0;
  int program = -1;
  int limit = 3;
  const char *path = 0;

  int i;
  for( i=1; i<argc; ++i ) {
    if( !strcmp(argv[i], "-v") ) {
      verbose = 1;
    }
    else if( !strcmp(argv[i], "-p") ) {
      if( ++i < argc ) program = atoi(argv[i]);
    }
    else if( !strcmp(argv[i], "-l") ) {
      if( ++i < argc ) limit = atoi(argv[i]);
    }
    else if(argv[i][0] == '-') {
      fprintf(stderr, "Unrecognized command %s\n", argv[i]);
      exit(1);
    }
    else if(!path) {
      path = argv[i];
    }
    else {
      fprintf(stderr, "Unknown argument \"%s\"\n", argv[i]);
      exit(1);
    }
  }

  for( i=1; i<100; ++i ) {
    char ifo_path[PATH_MAX];
    strncpy(&ifo_path[0], path, sizeof(ifo_path));
    int l = strnlen(&ifo_path[0],sizeof(ifo_path));
    snprintf(&ifo_path[l],sizeof(ifo_path)-l,"/VIDEO_TS/VTS_%02d_0.IFO",i);
    if( access(&ifo_path[0],R_OK) ) break;

    zmpeg3_t *src = new zmpeg3_t(&ifo_path[0]);
    if( !src ) break;
    uint32_t bits = src->fs->read_uint32();
    if( bits == zmpeg3_t::IFO_PREFIX ) {
      int fd = src->fs->get_fd();
      zifo_t *ifo = src->ifo_open(fd, 0);
      if( !ifo ) {
        fprintf(stderr,"Error opening ifo in %s\n", &ifo_path[0]);
        continue;
      }

      zicell_table_t icell_addrs;
      ifo->icell_addresses(&icell_addrs);
      int programs = ifo->max_inlv+1;
      int angles = -1;
      if( verbose )
        printf("processing: %s, %d programs\n",&ifo_path[0], programs);

      for( int title=0; title<100; ++title ) {
        int listed = 0;
        for( int pgm=0; pgm<programs; ++pgm ) {
          int sectors = 0, pcells = 0;
          int ret = ifo->chk(title, 0, pgm, 0, &icell_addrs, sectors, pcells, angles);
          if( ret < 0 ) break;
          if( !ret ) continue;
          if( pcells < limit ) continue;
          if( program >= 0 && pgm != program ) continue;
          if( !listed ) {
            listed = 1;
            printf("%s, Title = %d, pcells=%d, sectors = %d, angles %d, program ids=%d",
              &ifo_path[0], title+1, pcells, sectors, angles+1, title*100+pgm);
          }
          else
            printf(", %d",title*100+pgm);
        }
        if( listed ) printf("\n");
      }
      ifo->ifo_close();
    }

    delete src;
  }

  if( i == 1 ) {
    printf("cant open %s/VIDEO_TS/VTS_01_0.IFO\n",path);
    exit(1);
  }

  return 0;
}

