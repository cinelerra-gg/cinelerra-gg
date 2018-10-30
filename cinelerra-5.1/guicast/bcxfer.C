
void BC_Xfer::init(
	uint8_t **output_rows, int out_colormodel, int out_x, int out_y, int out_w, int out_h,
		uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp, uint8_t *out_ap, int out_rowspan,
	uint8_t **input_rows, int in_colormodel, int in_x, int in_y, int in_w, int in_h,
		uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp, uint8_t *in_ap, int in_rowspan,
	int bg_color)
{
	this->bg_color = bg_color;
	if( bg_color >= 0 ) {
		this->bg_r = (bg_color>>16) & 0xff;
		this->bg_g = (bg_color>>8) & 0xff;
		this->bg_b = (bg_color>>0) & 0xff;
	}
	else { // bg_color < 0, no src blending
		switch( in_colormodel ) {
		case BC_RGBA8888:	in_colormodel = BC_RGBX8888;		break;
		case BC_RGBA16161616:	in_colormodel = BC_RGBX16161616;	break;
		case BC_YUVA8888:	in_colormodel = BC_YUVX8888;		break;
		case BC_YUVA16161616:	in_colormodel = BC_YUVX16161616;	break;
		case BC_RGBA_FLOAT:	in_colormodel = BC_RGBX_FLOAT;		break;
		}
	}
	// prevent bounds errors on poorly dimensioned macro pixel formats
	switch( in_colormodel ) {
	case BC_UVY422:
	case BC_YUV422:   in_w &= ~1;               break;  // 2x1
	case BC_YUV420P:
	case BC_YUV420PI: in_w &= ~1;  in_h &= ~1;  break;  // 2x2
	case BC_YUV422P:  in_w &= ~1;               break;  // 2x1
	case BC_YUV410P:  in_w &= ~3;  in_h &= ~3;  break;  // 4x4
	case BC_YUV411P:  in_w &= ~3;               break;  // 4x1
	}
	switch( out_colormodel ) {
	case BC_UVY422:
	case BC_YUV422:  out_w &= ~1;               break;
	case BC_YUV420P:
	case BC_YUV420PI: out_w &= ~1; out_h &= ~1; break;
	case BC_YUV422P: out_w &= ~1;               break;
	case BC_YUV410P: out_w &= ~3; out_h &= ~3;  break;
	case BC_YUV411P: out_w &= ~3;               break;
	}
	this->output_rows = output_rows;
	this->input_rows = input_rows;
	this->out_yp = out_yp;
	this->out_up = out_up;
	this->out_vp = out_vp;
	this->out_ap = out_ap;
	this->in_yp = in_yp;
	this->in_up = in_up;
	this->in_vp = in_vp;
	this->in_ap = in_ap;
	this->in_x = in_x;
	this->in_y = in_y;
	this->in_w = in_w;
	this->in_h = in_h;
	this->out_x = out_x;
	this->out_y = out_y;
	this->out_w = out_w;
	this->out_h = out_h;
	this->in_colormodel = in_colormodel;
	switch( in_colormodel ) {
	case BC_GBRP:
		in_rowspan = in_w * sizeof(uint8_t);
		break;
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		if( !BC_CModels::has_alpha(out_colormodel) )
			this->in_colormodel = BC_RGB_FLOATP;
		in_rowspan = in_w * sizeof(float);
		break;
	}
	this->total_in_w = in_rowspan;
	this->out_colormodel = out_colormodel;
	switch( out_colormodel ) {
	case BC_GBRP:
		out_rowspan = out_w * sizeof(uint8_t);
		break;
	case BC_RGB_FLOATP:
	case BC_RGBA_FLOATP:
		out_rowspan = out_w * sizeof(float);
		break;
	}
	this->total_out_w = out_rowspan;
	this->in_pixelsize = BC_CModels::calculate_pixelsize(in_colormodel);
	this->out_pixelsize = BC_CModels::calculate_pixelsize(out_colormodel);
	this->scale = (out_w != in_w) || (in_x != 0);

/* + 1 so we don't overflow when calculating in advance */
	column_table = new int[out_w+1];
        double hscale = (double)in_w/out_w;
	for( int i=0; i<out_w; ++i ) {
			column_table[i] = ((int)(hscale * i) + in_x) * in_pixelsize;
			if( in_colormodel == BC_YUV422 || in_colormodel == BC_UVY422 )
				column_table[i] &= ~3;
	}
        double vscale = (double)in_h/out_h;
	row_table = new int[out_h];
        for( int i=0; i<out_h; ++i )
                row_table[i] = (int)(vscale * i) + in_y;
}

BC_Xfer::BC_Xfer(uint8_t **output_rows, uint8_t **input_rows,
	uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp,
	uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp,
	int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
	int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan)
{
	init(output_rows, out_colormodel, out_x, out_y, out_w, out_h,
		out_yp, out_up, out_vp, 0, out_rowspan,
	     input_rows, in_colormodel, in_x, in_y, in_w, in_h,
		in_yp, in_up, in_vp, 0, in_rowspan,  bg_color);
}

BC_Xfer::BC_Xfer(
	uint8_t **output_ptrs, int out_colormodel,
		int out_x, int out_y, int out_w, int out_h, int out_rowspan,
	uint8_t **input_ptrs, int in_colormodel,
		int in_x, int in_y, int in_w, int in_h, int in_rowspan,
	int bg_color)
{
	uint8_t *out_yp = 0, *out_up = 0, *out_vp = 0, *out_ap = 0;
	uint8_t *in_yp = 0,  *in_up = 0,  *in_vp = 0,  *in_ap = 0;
	if( BC_CModels::is_planar(in_colormodel) ) {
		in_yp = input_ptrs[0]; in_up = input_ptrs[1]; in_vp = input_ptrs[2];
		if( BC_CModels::has_alpha(in_colormodel) ) in_ap = input_ptrs[3];
	}
	if( BC_CModels::is_planar(out_colormodel) ) {
		out_yp = output_ptrs[0]; out_up = output_ptrs[1]; out_vp = output_ptrs[2];
		if( BC_CModels::has_alpha(out_colormodel) ) out_ap = output_ptrs[3];
	}
	init(output_ptrs, out_colormodel, out_x, out_y, out_w, out_h,
		 out_yp, out_up, out_vp, out_ap, out_rowspan,
	     input_ptrs, in_colormodel, in_x, in_y, in_w, in_h,
		 in_yp, in_up, in_vp, in_ap, in_rowspan,  bg_color);
}

BC_Xfer::~BC_Xfer()
{
	delete [] column_table;
	delete [] row_table;
}

void BC_CModels::transfer(unsigned char **output_rows, unsigned char **input_rows,
        unsigned char *out_yp, unsigned char *out_up, unsigned char *out_vp,
        unsigned char *in_yp, unsigned char *in_up, unsigned char *in_vp,
        int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
        int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan)
{
	BC_Xfer xfer(output_rows, input_rows,
		out_yp, out_up, out_vp, in_yp, in_up, in_vp,
		in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h,
		in_colormodel, out_colormodel, bg_color, in_rowspan, out_rowspan);
	xfer.xfer();
}

void BC_CModels::transfer(
	uint8_t **output_ptrs, int out_colormodel,
		int out_x, int out_y, int out_w, int out_h, int out_rowspan,
	uint8_t **input_ptrs, int in_colormodel,
		int in_x, int in_y, int in_w, int in_h, int in_rowspan,  int bg_color)
{
	BC_Xfer xfer(
		output_ptrs, out_colormodel, out_x, out_y, out_w, out_h, out_rowspan,
		input_ptrs, in_colormodel, in_x, in_y, in_w, in_h, in_rowspan,  bg_color);
	xfer.xfer();
}

// specialized functions

//  in bccmdl.py:  specialize("bc_rgba8888", "bc_transparency", "XFER_rgba8888_to_transparency")
void BC_Xfer::XFER_rgba8888_to_transparency(unsigned y0, unsigned y1)
{
  for( unsigned i=y0; i<y1; ++i ) {
    uint8_t *outp = output_rows[i + out_y] + out_x * out_pixelsize;
    uint8_t *inp_row = input_rows[row_table[i]];
    int bit_no = 0, bit_vec = 0;
    for( unsigned j=0; j<out_w; ) {
      uint8_t *inp = inp_row + column_table[j];
      if( inp[3] < 127 ) bit_vec |= 0x01 << bit_no;
      bit_no = ++j & 7;
      if( !bit_no ) { *outp++ = bit_vec;  bit_vec = 0; }
    }
    if( bit_no ) *outp = bit_vec;
  }
}

BC_Xfer::SlicerList BC_Xfer::slicers;

BC_Xfer::SlicerList::SlicerList()
{
  count = 0;
}

BC_Xfer::SlicerList::~SlicerList()
{
  reset();
}

void BC_Xfer::SlicerList::reset()
{
  lock("BC_Xfer::SlicerList::reset");
  while( last ) remove(last);
  count = 0;
  unlock();
}

BC_Xfer::Slicer *BC_Xfer::SlicerList::get_slicer(BC_Xfer *xp)
{
  Slicer *slicer = first;
  if( !slicer ) {
    if( count < BC_Resources::machine_cpus ) {
      slicer = new Slicer(xp);
      ++count;
    }
  }
  else
    remove_pointer(slicer);
  return slicer;
}

void BC_Xfer::xfer_slices(int slices)
{
  if( !xfn ) return;
  int max_slices = BC_Resources::machine_cpus/2;
  if( slices > max_slices ) slices = max_slices;
  if( slices < 1 ) slices = 1;
  Slicer *active[slices];
  unsigned y0 = 0, y1 = out_h;
  int slices1 = slices-1;
  if( slices1 > 0 ) {
    slicers.lock("BC_Xfer::xfer_slices");
    for( int i=0; i<slices1; y0=y1 ) {
      Slicer *slicer = slicers.get_slicer(this);
      if( !slicer ) { slices1 = i;  break; }
      active[i] = slicer;
      y1 = out_h * ++i / slices;
      slicer->slice(this, y0, y1);
    }
    slicers.unlock();
  }
  (this->*xfn)(y0, out_h);
  if( slices1 > 0 ) {
    for( int i=0; i<slices1; ++i )
      active[i]->complete->lock("BC_Xfer::xfer_slices");
    slicers.lock("BC_Xfer::xfer_slices");
    for( int i=0; i<slices1; ++i )
      slicers.append(active[i]);
    slicers.unlock();
  }
}

BC_Xfer::Slicer::Slicer(BC_Xfer *xp)
{
  this->xp = xp;
  init = new Condition(0, "BC_Xfer::Slicer::init", 0);
  complete = new Condition(0, "BC_Xfer::Slicer::complete", 0);
  done = 0;
  start();
}
BC_Xfer::Slicer::~Slicer()
{
  done = 1;
  init->unlock();
  join();
  delete complete;
  delete init;
}

void BC_Xfer::Slicer::slice(BC_Xfer *xp, unsigned y0, unsigned y1)
{
  this->xp = xp;
  this->y0 = y0;
  this->y1 = y1;
  init->unlock();
}

void BC_Xfer::Slicer::run()
{
  while( !done ) {
    init->lock("Slicer::run");
    if( done ) break;
    xfer_fn xfn = xp->xfn;
    (xp->*xfn)(y0, y1);
    complete->unlock();
  }
}

void BC_CModels::bcxfer_stop_slicers()
{
  BC_Xfer::slicers.reset();
}

