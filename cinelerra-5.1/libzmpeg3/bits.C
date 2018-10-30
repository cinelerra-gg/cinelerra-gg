#include "libzmpeg3.h"

zbits_t::
bits_t(zmpeg3_t *zsrc, demuxer_t *demux)
{
  bfr = 0;
  bfr_size = 0;
  bit_number = 0;
  src = zsrc;
  demuxer = demux;
  input_ptr = 0;
}

zbits_t::
~bits_t()
{
}

/* Fill a buffer.  Only works if bit_number is on an 8 bit boundary */
int zbits_t::
read_buffer(uint8_t *buffer, int bytes)
{
  while( bit_number > 0 ) {
    bit_number -= 8;
    demuxer->read_prev_char();
  }

  bit_number = 0;
  bfr_size = 0;
  bfr = 0;
  int result = demuxer->read_data(buffer, bytes);
  return result;
}

/* For mp3 decompression use a pointer in a buffer for get_bits. */
void zbits_t::
use_ptr(uint8_t *buffer)
{
  bfr_size = bit_number = 0;
  bfr = 0;
  input_ptr = buffer;
}

/* Go back to using the demuxer for get_bits in mp3. */
void zbits_t::
use_demuxer()
{
  if( input_ptr ) {
    bfr_size = bit_number = 0;
    input_ptr = 0;
    bfr = 0;
  }
}

/* Reconfigure for reverse operation */
/* Default is forward operation */
void zbits_t::
start_reverse()
{
  if( input_ptr )
    input_ptr -= (bfr_size+7)/8;
  else for( int i=0; i<bfr_size; i+=8 )
    demuxer->read_prev_char();
}

/* Reconfigure for forward operation */
void zbits_t::
start_forward()
{
// If already at the bof, the buffer is already invalid.
  if( demuxer && bof() ) {
    bfr_size = 0;
    bit_number = 0;
    bfr = 0;
    input_ptr = 0;
    demuxer->read_char();
  }
  else if( input_ptr )
    input_ptr += (bfr_size+7)/8;
  else for( int i=0; i<bfr_size; i+=8 )
    demuxer->read_char();
}

/* Erase the buffer with the next 4 bytes in the file. */
int zbits_t::
refill_ptr()
{
  bit_number = 32;
  bfr_size = 32;
  bfr  = (uint32_t)(*input_ptr++) << 24;
  bfr |= (uint32_t)(*input_ptr++) << 16;
  bfr |= (uint32_t)(*input_ptr++) << 8;
  bfr |= (uint32_t)*input_ptr++;
  return demuxer->eof();
}

int zbits_t::
refill_noptr()
{
  bit_number = 32;
  bfr_size = 32;
  bfr  = (uint32_t)demuxer->read_char() << 24;
  bfr |= (uint32_t)demuxer->read_char() << 16;
  bfr |= (uint32_t)demuxer->read_char() << 8;
  bfr |= (uint32_t)demuxer->read_char();
  return demuxer->eof();
}

/* Erase the buffer with the previous 4 bytes in the file. */
int zbits_t::
refill_reverse()
{
  bit_number = 0;
  bfr_size = 32;
  bfr  = (uint32_t)demuxer->read_prev_char();
  bfr |= (uint32_t)demuxer->read_prev_char() << 8;
  bfr |= (uint32_t)demuxer->read_prev_char() << 16;
  bfr |= (uint32_t)demuxer->read_prev_char() << 24;
  return demuxer->eof();
}

int zbits_t::
open_title(int title)
{
  bfr_size = bit_number = 0;
  return demuxer->open_title(title);
}

/*
 * int zbits_t::
 * seek_time(double time_position)
 * {
 *   bfr_size = bit_number = 0;
 *   return demuxer->seek_time(time_position);
 * }
 */

int zbits_t::
seek_byte(int64_t position)
{
  bfr_size = bit_number = 0;
  return demuxer->seek_byte(position);
}

