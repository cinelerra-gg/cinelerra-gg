#include "../libzmpeg3.h"

#ifndef MM_ACCEL_DJBFFT
#define MM_ACCEL_DJBFFT 0x00000001
#endif

zaudio_decoder_ac3_t::
audio_decoder_ac3_t()
{
  stream = new bits_t(0, 0);
  state = a52_init(MM_ACCEL_DJBFFT);
  output = a52_samples(state);
}

zaudio_decoder_ac3_t::
~audio_decoder_ac3_t()
{
  delete stream;
  a52_free(state);
}


/* Return 1 if it isn't an AC3 header */
int zaudio_decoder_ac3_t::
ac3_check(uint8_t *header)
{
  int flags, samplerate, bitrate;
  return !a52_syncinfo(header, &flags, &samplerate, &bitrate);
}

/* Decode AC3 header */
int zaudio_decoder_ac3_t::
ac3_header(uint8_t *header)
{
  int result = 0;
  flags = 0;
//zmsgs("%02x%02x%02x%02x%02x%02x%02x%02x\n",
//  header[0], header[1], header[2], header[3],
//  header[4], header[5], header[6], header[7]);
  result = a52_syncinfo(header, &flags, &samplerate, &bitrate);

  if( result ) {
//zmsgs("%d\n", result);
    framesize = result;
    channels = 0;

    if( (flags & A52_LFE) != 0 )
      ++channels;
//zmsgs("%08x %08x\n", (flags & A52_LFE), (flags & A52_CHANNEL_MASK));
    switch( (flags & A52_CHANNEL_MASK) ) {
      case A52_CHANNEL: ++channels;  break;
      case A52_MONO:    ++channels;  break;
      case A52_STEREO:  channels += 2;  break;
      case A52_3F:      channels += 3;  break;
      case A52_2F1R:    channels += 3;  break;
      case A52_3F1R:    channels += 4;  break;
      case A52_2F2R:    channels += 4;  break;
      case A52_3F2R:    channels += 5;  break;
      case A52_DOLBY:   channels += 2;  break;
      default:
        zmsgs("unknown channel code: %08x\n", (flags & A52_CHANNEL_MASK));
        break;
    }
  }
//zmsgs("1 %d\n", channels);
  return result;
}


int zaudio_decoder_ac3_t::
do_ac3(uint8_t *zframe, int zframe_size, float **zoutput, int render)
{
  int output_position = 0;
  sample_t level = 1;
  int i, j, l;
//zmsgs("1\n");
  a52_frame(state, zframe, &flags, &level, 0);
//zmsgs("2\n");
  a52_dynrng(state, NULL, NULL);
//zmsgs("3\n");
  for( i=0; i < 6; ++i ) {
    if( !a52_block(state) ) {
      l = 0;
      if( render ) {
        /* Remap the channels to conform to encoders. */
        for( j=0; j < channels; ++j ) {
          int dst_channel = j;
          /* Make LFE last channel. */
          /* Shift all other channels down 1. */
          if( (flags & A52_LFE) != 0 )
            dst_channel = j == 0 ? channels-1 : dst_channel-1;
          /* Swap front left and center for certain configurations */
          switch( (flags & A52_CHANNEL_MASK) ) {
            case A52_3F:
            case A52_3F1R:
            case A52_3F2R:
              if( dst_channel == 0 ) dst_channel = 1;
              else if( dst_channel == 1 ) dst_channel = 0;
              break;
          }

          memmove(zoutput[dst_channel]+output_position,
              output+l, 256*sizeof(float));
          l += 256;
        }
      }
      output_position += 256;
    }
  }

  return output_position;
}

