#include "libzmpeg3.h"

void usage()
{
  fprintf(stderr, "Show Table Of Contents\n"
    "Usage: mpeg3show <opts> <path>\n"
    "  opts:  -a show audio sample offsets\n"
    "         -i show audio index\n"
    "         -v show video frame offsets\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  if(argc < 2) {
    fprintf(stderr, "Show Table Of Contents\n"
      "Usage: mpeg3show <path>\n");
    exit(1);
  }

  int i;
  int flags = 0;
  for( i=1; i < argc; ++i ) {
    char *cp = argv[i];
    if( *cp++ != '-' ) break;
    while( *cp ) {
      switch( *cp ) {
      case 'a':  flags |= TOC_SAMPLE_OFFSETS;  break;
      case 'i':  flags |= TOC_AUDIO_INDEX;     break;
      case 'v':  flags |= TOC_FRAME_OFFSETS;   break;
      default:
        usage();
      }
      ++cp;
    }
  }

  zmpeg3_t *file = new zmpeg3_t(argv[i]);
  file->show_toc(flags);
  delete file;

  return 0;
}

