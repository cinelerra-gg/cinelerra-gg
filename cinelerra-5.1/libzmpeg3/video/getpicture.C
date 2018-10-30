#include "../libzmpeg3.h"

zslice_buffer_t *zvideo_t::
get_slice_buffer()
{
  slice_lock.lock();
  while( avail_slice_buffers == 0 ) {
    ++slice_wait_locked;
    slice_lock.unlock();
    slice_wait.lock();
    slice_lock.lock();
  }
  zslice_buffer_t *buffer = avail_slice_buffers;
  avail_slice_buffers = buffer->next;
  buffer->video = this;
  buffer->buffer_position = 0;
  buffer->bits_size = 0;
  buffer->buffer_size = 0;
  if( !slice_active_locked++ )
    slice_active.lock();
  slice_lock.unlock();
  return buffer;
}

void zvideo_t::
put_slice_buffer(zslice_buffer_t* buffer)
{
  slice_lock.lock();
  buffer->next = avail_slice_buffers;
  avail_slice_buffers = buffer;
  if( slice_wait_locked ) {
    --slice_wait_locked;
    slice_wait.unlock();
  }
  if( !--slice_active_locked )
    slice_active.unlock();
  slice_lock.unlock();
}


/* decode all macroblocks of the current picture */
int zvideo_t::
get_macroblocks()
{
  uint32_t zcode;
/* Load every slice into a buffer array */
  while( !eof() ) {
    zcode = vstream->show_bits32_noptr();
    if( zcode < SLICE_MIN_START || zcode > SLICE_MAX_START ) break;
    zslice_buffer_t *slice = get_slice_buffer();
    slice->fill_buffer(vstream);
    src->decode_slice(slice);
  }
  slice_active.lock();
  slice_active.unlock();
  return 0;
}

int zvideo_t::
display_second_field()
{
  return 0; /* Not used */
}

/* decode one frame or field picture */

int zvideo_t::
get_picture()
{
  uint8_t **ip_frame;
  if( pict_type != pic_type_B ) {
    if( !secondfield ) {
      for( int i=0; i<3; ++i ) {
        /* Swap refframes */
        uint8_t *tmp = oldrefframe[i];
        oldrefframe[i] = refframe[i];
        refframe[i] = tmp;
      }
    }
    ip_frame = refframe;
  }
  else
    ip_frame = auxframe;

  for( int i=0; i<3; ++i )
    newframe[i] = ip_frame[i];

  if( pict_struct == pics_BOTTOM_FIELD ) {
    newframe[0] += coded_picture_width;
    newframe[1] += chrom_width;
    newframe[2] += chrom_width;
  }

  if( src->cpus != src->total_slice_decoders )
    src->reallocate_slice_decoders();
  if( 2*src->cpus != total_slice_buffers )
    reallocate_slice_buffers();

  int result = get_macroblocks();

  /* Set the frame to display */
  output_src[0] = output_src[1] = output_src[2] = 0;

  if( !result && thumb ) {
    switch( pict_type ) {
    case pic_type_P:
      if( !ithumb ) break;
      ithumb = 0;
      src->thumbnail_fn(src->thumbnail_priv, track->number);
      break;
    case pic_type_I:
      ithumb = 1;
      break;
    }
  }
  if( !result && !skim && framenum >= 0 ) {
    if( pict_struct == pics_FRAME_PICTURE || secondfield ) {
      if( pict_type == pic_type_B ) {
        output_src[0] = auxframe[0];
        output_src[1] = auxframe[1];
        output_src[2] = auxframe[2];
      }
      else {
        output_src[0] = oldrefframe[0];
        output_src[1] = oldrefframe[1];
        output_src[2] = oldrefframe[2];
      }
    }
    else {
      display_second_field();
    }
  }

  return result;
}

