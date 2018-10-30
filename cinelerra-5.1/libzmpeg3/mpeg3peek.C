#include <stdio.h>
#include <stdlib.h>
#include "libzmpeg3.h"




int main(int argc, char *argv[])
{
  zmpeg3_t *file;
  int64_t frame_number;
  int error = 0;

  if(argc < 3) {
    printf("Usage: mpeg3peek <table of contents> <frame number>\n");
    printf("       mpeg3peek <table of contents> <sample number>\n");
    printf("Print the byte offset of a given frame or sample.\n");
    printf("If the file has no video, the sample number is located.\n");
    printf("Requires table of contents.\n");
    printf("Example: mpeg3peek heroine.toc 123\n");
    exit(1);
  }

  sscanf(argv[2], "%jx", &frame_number);
  if(frame_number < 0) frame_number = 0;

  file = mpeg3_open(argv[1], &error);
  if(file) {
    if(!mpeg3_total_vstreams(file)) {
      if(!mpeg3_total_astreams(file)) {
        printf("Need a video stream.\n");
        exit(1);
      }
      
      if(!file->atrack[0]->total_sample_offsets) {
        printf("Zero length track.  Did you load a table of contents?\n");
        exit(1);
      }
      
      int64_t chunk_number = frame_number / zmpeg3_t::AUDIO_CHUNKSIZE;
      if(chunk_number >= file->atrack[0]->total_sample_offsets)
        chunk_number = file->atrack[0]->total_sample_offsets - 1;
      printf("sample=%jx offset=0x%jx\n",
        frame_number,
        file->atrack[0]->sample_offsets[chunk_number]);
      exit(0);
    }

    if(!file->vtrack[0]->total_frame_offsets) {
      printf("Zero length track.  Did you load a table of contents?\n");
      exit(1);
    }

    if(frame_number >= file->vtrack[0]->total_frame_offsets)
      frame_number = file->vtrack[0]->total_frame_offsets - 1;
    printf("frame=%jx offset=0x%jx\n", 
      frame_number,
      file->vtrack[0]->frame_offsets[frame_number]);
  }
}

