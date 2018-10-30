/* Concatenate elementary streams */
/* Mpeg3cat is useful for extracting elementary streams from program streams. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "libzmpeg3.h"

#define BUFFER_SIZE 0x100000

unsigned char *output_buffer = 0;
int64_t output_buffer_size = 0;
int64_t output_scan_offset = 0;
FILE *out = 0;
int got_start = 0;
int do_audio = 0;
int do_video = 0;

// Check for first start code before writing out
static int write_output(unsigned char *data, int size, zmpeg3_t *file)
{
// Already got start code so write data directly
// Or user doesn't want to extract an elementary stream
  if( got_start || (!do_audio && !do_video) || file->is_bd() ) {
    return fwrite(data, size, 1, out);
  }
  else {
// Buffer until start code
    uint32_t zcode = 0xffffffff;
    int new_allocation = output_buffer_size + size;
    unsigned char *new_output_buffer = new unsigned char[new_allocation];
    memcpy(new_output_buffer, output_buffer, output_buffer_size);
    memcpy(new_output_buffer + output_buffer_size, data, size);
    if( output_buffer ) delete [] output_buffer;
    output_buffer = new_output_buffer;
    output_buffer_size = new_allocation;

    if( output_buffer_size >= 4 ) {
      if( do_video ) {
        while( output_scan_offset < output_buffer_size && !got_start ) {
          zcode = (zcode << 8) | output_buffer[output_scan_offset++];
          if( zcode == zmpeg3_t::SEQUENCE_START_CODE ) {
            got_start = 1;
          }
        }

        output_scan_offset -= 4;
      }
      else {
// Only scan for AC3 start code since we can't scan for mp2 start codes.
// It must occur in the first 2048 bytes or we give up.
        while( output_scan_offset < output_buffer_size && 
               output_scan_offset < 2048 && !got_start ) {
          zcode = ((zcode & 0xff) << 8) | output_buffer[output_scan_offset++];
          if( zcode == zmpeg3_t::AC3_START_CODE ) {
            got_start = 1;
          }
        }

        if( got_start )
          output_scan_offset -= 2;
        else if( output_scan_offset >= 2048 ) {
          output_scan_offset = 0;
          got_start = 1;
        }
        else
          output_scan_offset -= 2;
      }

      if( got_start ) {
        return fwrite(output_buffer + output_scan_offset, 
        output_buffer_size - output_scan_offset, 1, out);
      }
    }
  }

  return 1;
}

int main(int argc, char *argv[])
{
  char inpath[1024], outpath[1024], newpath[1024];
  char *inpaths[argc];
  int total_infiles = 0;
  zmpeg3_t *in;
  int out_counter = 0;
  int current_file, current_output_file = 0, i;
  unsigned char buffer[BUFFER_SIZE];
  long output_size;
  int result = 0;
  int64_t total_frames = 0;
  int stream = 0;
  int64_t total_written = 0;

  if( argc < 2 ) {
    fprintf(stderr, "Concatenate elementary streams or demultiplex a program stream.\n"
      "Usage: mpeg3cat -[av0123456789] <infile> [infile...] > <outfile>\n\n"
      "Example: Concatenate 2 video files: mpeg3cat xena1.m2v xena2.m2v > xena.m2v\n"
      "         Extract audio stream 0: mpeg3cat -a0 xena.vob > war_cry.ac3\n");
    exit(1);
  }

  outpath[0] = 0;
  for( i=1; i < argc; ++i ) {
    if( argv[i][0] == '-' ) {
      if( argv[i][1] != 'a' && argv[i][1] != 'v' && argv[i][1] != 'o' ) {
        fprintf(stderr, "invalid option %s\n", argv[i]);
        exit(1);
      }
      else
      if( argv[i][1] == 'o' ) {
// Check for filename
        if( i < argc - 1 ) {
          strcpy(outpath, argv[++i]);
        }
        else {
          fprintf(stderr, "-o requires an output file\n");
          exit(1);
        }

// Check if file exists
        if( (out = fopen(outpath, "r")) ) {
          fprintf(stderr, "%s exists.\n", outpath);
          exit(1);
        }
      }
      else {
        if( argv[i][1] == 'a' ) do_audio = 1;
        else if( argv[i][1] == 'v' ) do_video = 1;
        if( argv[i][2] != 0 ) {
          stream = argv[i][2] - '0';
        }
      }
    }
    else {
      inpaths[total_infiles++] = argv[i];
    }
  }

  if( outpath[0] ) {
    if( !(out = fopen(outpath, "wb")) ) {
      fprintf(stderr, "Failed to open %s for writing\n", outpath);
      exit(1);
    }
  }
  else
    out = stdout;

  for( current_file=0; current_file < total_infiles; ++current_file ) {
    strcpy(inpath, inpaths[current_file]);
    int error = 0;
    if( !(in = mpeg3_open(inpath, &error)) ) {
      fprintf(stderr, "Skipping %s\n", inpath);
      continue;
    }

/* output elementary audio stream */
    if( (mpeg3_has_audio(in) && in->is_audio_stream() ) || 
        (do_audio && !in->is_audio_stream() && !in->is_video_stream())) {
      do_audio = 1;
/* Add audio stream to end */
      if( stream >= in->total_astreams() ) {
        fprintf(stderr, "No audio stream %d\n", stream);
        exit(1);
      }

      in->atrack[stream]->demuxer->seek_byte(zmpeg3_t::START_BYTE);
//      in->atrack[stream]->audio->astream->refill();
//printf("mpeg3cat 1\n");
      while( !mpeg3_read_audio_chunk(in, buffer, 
                &output_size, BUFFER_SIZE, stream) ) {
//printf("mpeg3cat 2 0x%x\n", output_size);
        result = !write_output(buffer, output_size, in);
        if( result ) {
          perror("write audio chunk");
          break;
        }
      }
//printf("mpeg3cat 3\n");
    }
/* Output elementary video stream */
    else if( (mpeg3_has_video(in) && in->is_video_stream()) ||
             (do_video && !in->is_video_stream() && !in->is_audio_stream())) {
/* Add video stream to end */
      int64_t hour, minute, second, frame;
      uint32_t zcode;
      float carry;
      int i, offset;
      
      if( stream >= in->total_vstreams() ) {
        fprintf(stderr, "No video stream %d\n", stream);
        exit(1);
      }

      in->vtrack[stream]->demuxer->seek_byte(zmpeg3_t::START_BYTE);
      in->vtrack[stream]->video->vstream->refill();
      do_video = 1;
      while( !mpeg3_read_video_chunk(in, buffer, &output_size,
                 BUFFER_SIZE, stream) && output_size >= 4 ) {
        zcode = (uint32_t)buffer[output_size - 4] << 24; 
        zcode |= (uint32_t)buffer[output_size - 3] << 16; 
        zcode |= (uint32_t)buffer[output_size - 2] << 8; 
        zcode |= (uint32_t)buffer[output_size - 1]; 

/* Got a frame at the end of this buffer. */
        if( zcode == zmpeg3_t::PICTURE_START_CODE ) {
          total_frames++;
        }
        else if( zcode == zmpeg3_t::SEQUENCE_END_CODE ) {
/* Got a sequence end code at the end of this buffer. */
          output_size -= 4;
        }

        zcode = (uint32_t)buffer[0] << 24;
        zcode |= (uint32_t)buffer[1] << 16;
        zcode |= (uint32_t)buffer[2] << 8;
        zcode |= buffer[3];

        i = 0;
        offset = 0;
        if( zcode == zmpeg3_t::SEQUENCE_START_CODE && current_output_file > 0 ) {
/* Skip the sequence start code */
          i += 4;
          while( i < output_size && zcode != zmpeg3_t::GOP_START_CODE ) {
            zcode <<= 8;
            zcode |= buffer[i++];
          }
          i -= 4;
          offset = i;
        }

/* Search for GOP header to fix */
        zcode = (uint32_t)buffer[i++] << 24;
        zcode |= (uint32_t)buffer[i++] << 16;
        zcode |= (uint32_t)buffer[i++] << 8;
        zcode |= buffer[i++];
        while( i < output_size && zcode != zmpeg3_t::GOP_START_CODE ) {
          zcode <<= 8;
          zcode |= buffer[i++];
        }

        if( zcode == zmpeg3_t::GOP_START_CODE ) {
/* Get the time code */
          zcode = (uint32_t)buffer[i] << 24;
          zcode |= (uint32_t)buffer[i + 1] << 16;
          zcode |= (uint32_t)buffer[i + 2] << 8;
          zcode |= (uint32_t)buffer[i + 3];

          hour = zcode >> 26 & 0x1f;
          minute = zcode >> 20 & 0x3f;
          second = zcode >> 13 & 0x3f;
          frame = zcode >> 7 & 0x3f;

/*        int64_t gop_frame = (int64_t)(hour * 3600 * mpeg3_frame_rate(in, stream) +
              minute * 60 * mpeg3_frame_rate(in, stream) +
              second * mpeg3_frame_rate(in, stream) + 
              frame); */
/* fprintf(stderr, "old: %02d:%02d:%02d:%02d ", hour, minute, second, frame); */
/* Write a new time code */
          hour = (int64_t)((float)(total_frames - 1) / mpeg3_frame_rate(in, stream) / 3600);
          carry = hour * 3600 * mpeg3_frame_rate(in, stream);
          minute = (int64_t)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream) / 60);
          carry += minute * 60 * mpeg3_frame_rate(in, stream);
          second = (int64_t)((float)(total_frames - 1 - carry) / mpeg3_frame_rate(in, stream));
          carry += second * mpeg3_frame_rate(in, stream);
          frame = (total_frames - 1 - carry);

          buffer[i] = ((zcode >> 24) & 0x80) | (hour << 2) | (minute >> 4);
          buffer[i + 1] = ((zcode >> 16) & 0x08) | ((minute & 0xf) << 4) | (second >> 3);
          buffer[i + 2] = ((second & 0x7) << 5) | (frame >> 1);
          buffer[i + 3] = (zcode & 0x7f) | ((frame & 0x1) << 7);
/* fprintf(stderr, "new: %02d:%02d:%02d:%02d\n", hour, minute, second, frame); */
        }


/* Test 32 bit overflow */
        if( outpath[0] ) {
          if( ftell(out) > 0x7f000000 ) {
            fclose(out);
            out_counter++;
            sprintf(newpath, "%s%03d", outpath, out_counter);
            if( !(out = fopen(newpath, "wb")) ) {
              fprintf(stderr, "Couldn't open %s for writing.\n", newpath);
              exit(1);
            }
          }
        }
/*
 * fprintf(stderr, "mpeg3cat 5 %02x %02x %02x %02x\n", 
 *   (buffer + offset)[0], 
 *   (buffer + offset)[1],
 *   (buffer + offset)[2], 
 *   (buffer + offset)[3]);
 */

/* Write the frame */
        result = !write_output(buffer + offset, output_size - offset, in);
        if( result ) {
          perror("write video chunk");
          break;
        }
      }
    }
    else
/* Output program stream */
/* In real life, program streams ended up having discontinuities in time codes */
/* so this isn't being maintained anymore. */
    if( in->is_program_stream() ) {
      zdemuxer_t *demuxer = in->vtrack[0]->demuxer;
      result = 0;

/* Append program stream with no changes */
      demuxer->read_all = 1;
      demuxer->seek_byte(zmpeg3_t::START_BYTE);

      while( !result ) {
        result = demuxer->seek_phys();
        if( !result ) {
          demuxer->zdata.size = 0;
          demuxer->zvideo.size = 0;
          demuxer->zaudio.size = 0;
          result = demuxer->read_program();
          if( result )
            fprintf(stderr, "Hit end of data in %s\n", inpath);
        }


// Read again and decrypt it
        unsigned char raw_data[0x10000];
        int raw_size = 0;
        if( !result ) {
          ztitle_t *title = demuxer->titles[demuxer->current_title];
          int64_t temp_offset = title->fs->tell();
          int64_t decryption_offset =
            demuxer->last_packet_decryption - demuxer->last_packet_start;
          raw_size = demuxer->last_packet_end - demuxer->last_packet_start;

          title->fs->seek(demuxer->last_packet_start);
          title->fs->read_data(raw_data, raw_size);
          title->fs->seek(temp_offset);

          if( decryption_offset > 0 && 
              decryption_offset < raw_size &&
              raw_data[decryption_offset] & 0x30 ) {
            if( title->fs->css.decrypt_packet(raw_data, 0) ) {
              fprintf(stderr, "get_ps_pes_packet: Decryption not available\n");
              return 1;
            }
            raw_data[decryption_offset] &= 0xcf;
          }
        }

// Write it
        if( !result ) {
          result = !write_output(raw_data, raw_size, in);
          total_written += raw_size;
          if( result) fprintf(stderr, "write program stream: %s\n", strerror(errno) );
        }
      }
    }
    else {
/* No transport stream support, since these can be catted */
      fprintf(stderr, "No catting of transport streams.\n");
      mpeg3_close(in);
      in = 0;
      continue;
    }

    mpeg3_close(in);
    in = 0;
    current_output_file++;
  }

#ifdef TODO
/* Terminate output */
  if( current_output_file > 0 && do_video ) {
/* Write new end of sequence */
/* Not very useful */
    buffer[0] = (zmpeg3_t::SEQUENCE_END_CODE >> 24) & 0xff;
    buffer[1] = (zmpeg3_t::SEQUENCE_END_CODE >> 16) & 0xff;
    buffer[2] = (zmpeg3_t::SEQUENCE_END_CODE >>  8) & 0xff;
    buffer[3] = (zmpeg3_t::SEQUENCE_END_CODE >>  0) & 0xff;
    result = !write_output(buffer, 4, in);
  }
#endif
  if( outpath[0]) fclose(out );
  exit(0);
}

