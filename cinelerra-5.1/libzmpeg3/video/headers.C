#include "../libzmpeg3.h"

int zvideo_t::
get_seq_hdr()
{
  int i;
  int load_intra_quantizer_matrix, load_non_intra_quantizer_matrix;

//zmsg("1\n");
  horizontal_size = vstream->get_bits(12);
  vertical_size = vstream->get_bits(12);
  //int aspect_ratio =
    vstream->get_bits(4);
  framerate_code = vstream->get_bits(4);
  bitrate = vstream->get_bits(18);
  vstream->get_bit_noptr(); /* marker bit (=1) */
  //int vbv_buffer_size =
    vstream->get_bits(10);
  //int constrained_parameters_flag =
    vstream->get_bit_noptr();
  frame_rate = frame_rate_table[framerate_code];

  load_intra_quantizer_matrix = vstream->get_bit_noptr();
  if( load_intra_quantizer_matrix ) {
    for( i=0; i<64; ++i )
      intra_quantizer_matrix[zigzag_scan_table[i]] = vstream->get_byte_noptr();
  }
  else {
    for( i=0; i<64; ++i )
      intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
  }

  load_non_intra_quantizer_matrix = vstream->get_bit_noptr();
  if( load_non_intra_quantizer_matrix ) {
    for( i=0; i<64; ++i )
      non_intra_quantizer_matrix[zigzag_scan_table[i]] = vstream->get_byte_noptr();
  }
  else {
    for( i=0; i<64; ++i )
      non_intra_quantizer_matrix[i] = 16;
  }

/* copy luminance to chrominance matrices */
  for( i=0; i<64; ++i ) {
    chroma_intra_quantizer_matrix[i] = intra_quantizer_matrix[i];
    chroma_non_intra_quantizer_matrix[i] = non_intra_quantizer_matrix[i];
  }

//zmsg("100\n");
  return 0;
}


/* decode sequence extension */

int zvideo_t::
sequence_extension()
{
  mpeg2 = 1;
  scalable_mode = slice_decoder_t::sc_NONE; /* unless overwritten by seq. scal. ext. */
  //int prof_lev =
    vstream->get_byte_noptr();
  prog_seq = vstream->get_bit_noptr();
  chroma_format = vstream->get_bits(2);
  int horizontal_size_extension = vstream->get_bits(2);
  int vertical_size_extension = vstream->get_bits(2);
  //int bit_rate_extension =
    vstream->get_bits(12);
  vstream->get_bit_noptr();
  //int vbv_buffer_size_extension =
    vstream->get_byte_noptr();
  //int low_delay =
    vstream->get_bit_noptr();
  //int frame_rate_extension_n =
    vstream->get_bits(2);
  //int frame_rate_extension_d =
    vstream->get_bits(5);
  horizontal_size = (horizontal_size_extension << 12) | (horizontal_size & 0x0fff);
  vertical_size = (vertical_size_extension << 12) | (vertical_size & 0x0fff);
  return 0;
}


/* decode sequence display extension */

int zvideo_t::
sequence_display_extension()
{
  // int video_format =
    vstream->get_bits(3);
  int colour_description = vstream->get_bit_noptr();

  if( colour_description ) {
    //int colour_primaries =
      vstream->get_byte_noptr();
    //int transfer_characteristics =
      vstream->get_byte_noptr();
    matrix_coefficients = vstream->get_byte_noptr();
  }

  //int display_horizontal_size =
    vstream->get_bits(14);
  vstream->get_bit_noptr();
  //int display_vertical_size =
    vstream->get_bits(14);
  return 0;
}


/* decode quant matrix entension */

int zvideo_t::
quant_matrix_extension()
{
  int i;
  int load_intra_quantiser_matrix, load_non_intra_quantiser_matrix;
  int load_chroma_intra_quantiser_matrix;
  int load_chroma_non_intra_quantiser_matrix;

  load_intra_quantiser_matrix = vstream->get_bit_noptr();
  if( load_intra_quantiser_matrix != 0 ) {
    for( i=0; i<64; ++i ) {
      chroma_intra_quantizer_matrix[zigzag_scan_table[i]]
        = intra_quantizer_matrix[zigzag_scan_table[i]]
        = vstream->get_byte_noptr();
    }
  }

  load_non_intra_quantiser_matrix = vstream->get_bit_noptr();
  if( load_non_intra_quantiser_matrix != 0 ) {
    for( i=0; i<64; ++i ) {
      chroma_non_intra_quantizer_matrix[zigzag_scan_table[i]]
        = non_intra_quantizer_matrix[zigzag_scan_table[i]]
        = vstream->get_byte_noptr();
    }
  }

  load_chroma_intra_quantiser_matrix = vstream->get_bit_noptr();
  if( load_chroma_intra_quantiser_matrix != 0 ) {
    for( i=0; i<64; ++i )
      chroma_intra_quantizer_matrix[zigzag_scan_table[i]] = vstream->get_byte_noptr();
  }

  load_chroma_non_intra_quantiser_matrix = vstream->get_bit_noptr();
  if( load_chroma_non_intra_quantiser_matrix != 0 ) {
    for( i=0; i<64; ++i )
      chroma_non_intra_quantizer_matrix[zigzag_scan_table[i]] = vstream->get_byte_noptr();
  }

  return 0;
}


/* decode sequence scalable extension */

int zvideo_t::
sequence_scalable_extension()
{
  scalable_mode = vstream->get_bits(2) + 1; /* add 1 to make sc_DP != sc_NONE */
  //int layer_id =
    vstream->get_bits(4);

  if( scalable_mode == slice_decoder_t::sc_SPAT ) {
    llw = vstream->get_bits(14); /* lower_layer_prediction_horizontal_size */
    vstream->get_bit_noptr();
    llh = vstream->get_bits(14); /* lower_layer_prediction_vertical_size */
    hm = vstream->get_bits(5);
    hn = vstream->get_bits(5);
    vm = vstream->get_bits(5);
    vn = vstream->get_bits(5);
  }

  if( scalable_mode == slice_decoder_t::sc_TEMP )
    zerr("temporal scalability not implemented\n");
  return 0;
}


/* decode picture display extension */

int zvideo_t::
picture_display_extension()
{
//  short frame_centre_horizontal_offset[3];
//  short frame_centre_vertical_offset[3];
/* per spec */
  int n = prog_seq ? (!repeatfirst ? 1 : (!topfirst ? 2 : 3)) :
     pict_struct != pics_FRAME_PICTURE ? 1 : (!repeatfirst ? 2 : 3) ;

  for( int i=0; i<n; ++i ) {
    //frame_centre_horizontal_offset[i] = (short)
      vstream->get_bits(16);
    vstream->get_bit_noptr();
    //frame_centre_vertical_offset[i] = (short)
      vstream->get_bits(16);
    vstream->get_bit_noptr();
  }
  return 0;
}


/* decode picture coding extension */

int zvideo_t::
picture_coding_extension()
{
  h_forw_r_size = vstream->get_bits(4) - 1;
  v_forw_r_size = vstream->get_bits(4) - 1;
  h_back_r_size = vstream->get_bits(4) - 1;
  v_back_r_size = vstream->get_bits(4) - 1;
  dc_prec = vstream->get_bits(2);
  pict_struct = vstream->get_bits(2);
  topfirst = vstream->get_bit_noptr();
  frame_pred_dct = vstream->get_bit_noptr();
  conceal_mv = vstream->get_bit_noptr();
  qscale_type = vstream->get_bit_noptr();
  intravlc = vstream->get_bit_noptr();
  altscan = vstream->get_bit_noptr();
  repeatfirst = vstream->get_bit_noptr();
  //int chroma_420_type =
    vstream->get_bit_noptr();
  prog_frame = vstream->get_bit_noptr();
  if( repeat_fields > 2 ) repeat_fields = 0;
  repeat_fields += 2;
  if( repeatfirst ) {
    if( prog_seq ) repeat_fields += !topfirst ? 2 : 4;
    else if( prog_frame ) ++repeat_fields;
  }
  current_field = 0;

//static const char *pstruct[] = { "nil", "top", "bot", "fld" };
//zmsgs("pstruct %s prog_seq%d prog_frame%d topfirst%d repeatfirst%d repeat_fields%d\n", 
//  pstruct[pict_struct], prog_seq , prog_frame, topfirst, repeatfirst, repeat_fields);

  int composite_display_flag = vstream->get_bit_noptr();
  if( composite_display_flag ) {
    //int v_axis =
      vstream->get_bit_noptr();
    field_sequence = vstream->get_bits(3);
    //int sub_carrier =
      vstream->get_bit_noptr();
    //int burst_amplitude =
      vstream->get_bits(7);
    //int sub_carrier_phase =
      vstream->get_byte_noptr();
  }
  return 0;
}

/* decode picture spatial scalable extension */

int zvideo_t::
picture_spatial_scalable_extension()
{
  pict_scal = 1; /* use spatial scalability in this picture */
  lltempref = vstream->get_bits(10);
  vstream->get_bit_noptr();
  llx0 = vstream->get_bits(15);
  if(llx0 >= 16384) llx0 -= 32768;
  vstream->get_bit_noptr();
  lly0 = vstream->get_bits(15);
  if(lly0 >= 16384) lly0 -= 32768;
  stwc_table_index = vstream->get_bits(2);
  llprog_frame = vstream->get_bit_noptr();
  llfieldsel = vstream->get_bit_noptr();
  return 0;
}


/* decode picture temporal scalable extension
 *
 * not implemented
 *
 */

int zvideo_t::
picture_temporal_scalable_extension()
{
  zerr("temporal scalability not supported\n");
  return 0;
}


/* decode extension and user data */

int zvideo_t::
ext_user_data()
{
  int zcode = vstream->next_start_code();
  if( zcode < 0 ) return 1;
  while( zcode == EXT_START_CODE || zcode == USER_START_CODE ) {
    vstream->refill();
    if( zcode == EXT_START_CODE ) {
      int ext_id = vstream->get_bits(4);
      switch(ext_id) {
      case ext_id_SEQ:
        sequence_extension();
        break;
      case ext_id_DISP:
        sequence_display_extension();
        break;
      case ext_id_QUANT:
        quant_matrix_extension();
        break;
      case ext_id_SEQSCAL:
        sequence_scalable_extension();
        break;
      case ext_id_PANSCAN:
        picture_display_extension();
        break;
      case ext_id_CODING:
        picture_coding_extension();
        break;
      case ext_id_SPATSCAL:
        picture_spatial_scalable_extension();
        break;
      case ext_id_TEMPSCAL:
        picture_temporal_scalable_extension();
        break;
      default:
        zerrs("reserved extension start code ID %d\n", ext_id);
        break;
      }
    }
    else if( subtitle_track >= 0 ) {
      int32_t id = vstream->show_bits(32);
      if( id == 0x47413934 ) { /* "GA94" - atsc user data */
        vstream->refill();
        int typ = vstream->get_bits(8);
        if( typ == 3 ) /* mpeg cc */
          get_cc()->get_atsc_data(vstream);
      }
    }
    zcode = vstream->next_start_code();
    if( zcode < 0 ) return 1;
  }
//zmsgs("prog_seq=%d prog_frame=%d\n", prog_seq,prog_frame);
  return 0;
}


/* decode group of pictures header */

int zvideo_t::
get_gop_header()
{
//zmsg("1\n");
  has_gops = 1;
  //int drop_flag =
    vstream->get_bit_noptr();
  gop_timecode.hour = vstream->get_bits(5);
  gop_timecode.minute = vstream->get_bits(6);
  vstream->get_bit_noptr();
  gop_timecode.second = vstream->get_bits(6);
  gop_timecode.frame = vstream->get_bits(6);
  //int closed_gop =
    vstream->get_bit_noptr();
  //int broken_link =
    vstream->get_bit_noptr();
//zmsg("100\n");
//zmsgs("%d:%d:%d:%d %d %d %d\n",
//  gop_timecode.hour, gop_timecode.minute, gop_timecode.second,
//  gop_timecode.frame, drop_flag, closed_gop, broken_link);
  return vstream->error();
}

/* decode picture header */

int zvideo_t::
get_picture_hdr()
{
  pict_scal = 0; /* unless overwritten by pict. spat. scal. ext. */
  //int temp_ref =
    vstream->get_bits(10);
  pict_type = vstream->get_bits(3);
  //int vbv_delay =
    vstream->get_bits(16);
  if( pict_type == pic_type_P || pict_type == pic_type_B ) {
    full_forw = vstream->get_bit_noptr();
    forw_r_size = vstream->get_bits(3) - 1;
  }

  if( pict_type == pic_type_B ) {
    full_back = vstream->get_bit_noptr();
    back_r_size = vstream->get_bits(3) - 1;
  }

/* get extra bit picture */
  while( !vstream->eof() && vstream->get_bit_noptr() )
    vstream->get_bits_noptr(8);
  return 0;
}


/* move to start of header. */
/*  this updates the demuxer times */
int zvideo_t::
find_header()
{
  for( int count=0x1000; --count>=0; ) {
    int zcode = vstream->next_start_code();
    if( zcode < 0 ) break;
    switch(zcode) {
    case SEQUENCE_START_CODE:
    case GOP_START_CODE:
    case PICTURE_START_CODE:
      return 0;
    }
    vstream->refill();
  }

  return 1;
}


int zvideo_t::
get_header()
{
// Stop search after this many start codes
  for( int count=0x1000; --count>=0; ) {
/* look for startcode */
    int zcode = vstream->next_start_code();
    if( zcode < 0 ) break;
    vstream->refill();

    switch(zcode) {
    case SEQUENCE_START_CODE:
/* a sequence header should be found before returning from get_header the */
/* first time (this is to set horizontal/vertical size properly) */
      found_seqhdr = 1;
      get_seq_hdr();  
      ext_user_data();
      break;

    case GOP_START_CODE:
      get_gop_header();
      ext_user_data();
      break;

    case PICTURE_START_CODE:
      get_picture_hdr();
      ext_user_data();
      if( cc ) cc->reorder();
      if( found_seqhdr )
        return 0;       /* Exit here */
      break;
    }
  }

  return 1;
}

int zslice_buffer_t::
ext_bit_info()
{
  while( get_bit() ) get_bits(8);
  return 0;
}

/* decode slice header */
int zslice_decoder_t::
get_slice_hdr()
{
  int slice_vertical_position_extension;
  int qs;
  slice_vertical_position_extension =
    (video->mpeg2 && video->vertical_size>2800) ?
      slice_buffer->get_bits(3) : 0;

  if( video->scalable_mode == sc_DP )
    pri_brk = slice_buffer->get_bits(7);

  qs = slice_buffer->get_bits(5);
  quant_scale = video->mpeg2 ?
    (video->qscale_type ? non_linear_mquant_table[qs] : (qs << 1)) :
    qs;

//int intra_slice = 0;
  if( slice_buffer->get_bit() ) {
    //intra_slice =
      slice_buffer->get_bit();
    slice_buffer->get_bits(7);
    slice_buffer->ext_bit_info();
  }

  return slice_vertical_position_extension;
}

