
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
 * Copyright (C) 2012 Monty <monty@xiph.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "clip.h"
#include "edl.inc"
#include "mutex.h"
#include "overlayframe.h"
#include "units.h"

/*
 * New resampler code; replace the original somehwat blurry engine
 * with a fairly standard kernel resampling core.  This could be used
 * for full affine transformation but only implements scale/translate.
 * Mostly reuses the old blending macro code.
 *
 * Pixel convention:
 *
 *  1) Pixels are points, not areas or squares.
 *
 *  2) To maintain the usual edge and scaling conventions, pixels are
 *     set inward from the image edge, eg, the left edge of an image is
 *     at pixel location x=-.5, not x=0.  Although pixels are not
 *     squares, the usual way of stating this is 'the pixel is located
 *     at the center of its square'.
 *
 *  3) Because of 1 and 2, we must truncate and weight the kernel
 *     convolution at the edge of the input area.  Otherwise, all
 *     resampled areas would be bordered by a transparency halo. E.g.
 *     in the old engine, upsampling HDV to 1920x1080 results in the
 *     left and right edges being partially transparent and underlying
 *     layers shining through.
 *
 *   4) The contribution of fractional pixels at the edges of input
 *     ranges are weighted according to the fraction.  Note that the
 *     kernel weighting is adjusted, not the opacity.  This is one
 *     exception to 'pixels have no area'.
 *
 *  5) The opacity of fractional pixels at the edges of the output
 *     range is adjusted according to the fraction. This is the other
 *     exception to 'pixels have no area'.
 *
 * Fractional alpha blending has been modified across the board from:
 *    output_alpha = input_alpha > output_alpha ? input_alpha : output_alpha;
 *  to:
 *    output_alpha = output_alpha + ((max - output_alpha) * input_alpha) / max;
 */

/* Sinc needed for Lanczos kernel */
static float sinc(const float x)
{
	if(fabsf(x) < TRANSFORM_MIN) return 1.0f;
	float y = x * M_PI;
	return sinf(y) / y;
}

/*
 * All resampling (except Nearest Neighbor) is performed via
 *   transformed 2D resampling kernels bult from 1D lookups.
 */
OverlayKernel::OverlayKernel(int interpolation_type)
{
	int i;
	this->type = interpolation_type;

	switch(interpolation_type)
	{
	case BILINEAR:
		width = 1.f;
		lookup = new float[(n = TRANSFORM_SPP) + 1];
		for (i = 0; i <= TRANSFORM_SPP; i++)
			lookup[i] = (float)(TRANSFORM_SPP - i) / TRANSFORM_SPP;
		break;

	/* Use a Catmull-Rom filter (not b-spline) */
	case BICUBIC:
		width = 2.;
		lookup = new float[(n = 2 * TRANSFORM_SPP) + 1];
		for(i = 0; i <= TRANSFORM_SPP; i++) {
			float x = i / (float)TRANSFORM_SPP;
			lookup[i] = 1.f - 2.5f * x * x + 1.5f * x * x * x;
		}
		for(; i <= 2 * TRANSFORM_SPP; i++) {
			float x = i / (float)TRANSFORM_SPP;
			lookup[i] = 2.f - 4.f * x  + 2.5f * x * x - .5f * x * x * x;
		}
		break;

	case LANCZOS:
		width = 3.;
		lookup = new float[(n = 3 * TRANSFORM_SPP) + 1];
		for (i = 0; i <= 3 * TRANSFORM_SPP; i++)
			lookup[i] = sinc((float)i / TRANSFORM_SPP) *
				sinc((float)i / TRANSFORM_SPP / 3.0f);
		break;

	default:
		width = 0.;
		lookup = 0;
		n = 0;
		break;
	}
}

OverlayKernel::~OverlayKernel()
{
	if(lookup) delete [] lookup;
}

OverlayFrame::OverlayFrame(int cpus)
{
	direct_engine = 0;
	nn_engine = 0;
	sample_engine = 0;
	temp_frame = 0;
	memset(kernel, 0, sizeof(kernel));
	this->cpus = cpus;
}

OverlayFrame::~OverlayFrame()
{
	if(temp_frame) delete temp_frame;

	if(direct_engine) delete direct_engine;
	if(nn_engine) delete nn_engine;
	if(sample_engine) delete sample_engine;

	if(kernel[NEAREST_NEIGHBOR]) delete kernel[NEAREST_NEIGHBOR];
	if(kernel[BILINEAR]) delete kernel[BILINEAR];
	if(kernel[BICUBIC]) delete kernel[BICUBIC];
	if(kernel[LANCZOS]) delete kernel[LANCZOS];
}

static float epsilon_snap(float f)
{
	return rintf(f * 1024) / 1024.;
}

int OverlayFrame::overlay(VFrame *output, VFrame *input,
	float in_x1, float in_y1, float in_x2, float in_y2,
	float out_x1, float out_y1, float out_x2, float out_y2,
	float alpha, int mode, int interpolation_type)
{
	in_x1 = epsilon_snap(in_x1);
	in_x2 = epsilon_snap(in_x2);
	in_y1 = epsilon_snap(in_y1);
	in_y2 = epsilon_snap(in_y2);
	out_x1 = epsilon_snap(out_x1);
	out_x2 = epsilon_snap(out_x2);
	out_y1 = epsilon_snap(out_y1);
	out_y2 = epsilon_snap(out_y2);

	if (isnan(in_x1) || isnan(in_x2) ||
		isnan(in_y1) || isnan(in_y2) ||
		isnan(out_x1) || isnan(out_x2) ||
		isnan(out_y1) || isnan(out_y2)) return 1;

	if( in_x2 <= in_x1 || in_y2 <= in_y1 ) return 1;
	if( out_x2 <= out_x1 || out_y2 <= out_y1 ) return 1;

	float xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
	float yscale = (out_y2 - out_y1) / (in_y2 - in_y1);
	int in_w = input->get_w(), in_h = input->get_h();
	int out_w = output->get_w(), out_h = output->get_h();

	if( in_x1 < 0 ) {
		out_x1 -= in_x1 * xscale;
		in_x1 = 0;
	}
	if( in_x2 > in_w ) {
		out_x2 -= (in_x2 - in_w) * xscale;
		in_x2 = in_w;
	}
	if( in_y1 < 0 ) {
		out_y1 -= in_y1 * yscale;
		in_y1 = 0;
	}
	if( in_y2 > in_h ) {
		out_y2 -= (in_y2 - in_h) * yscale;
		in_y2 = in_h;
	}

	if( out_x1 < 0 ) {
		in_x1 -= out_x1 / xscale;
		out_x1 = 0;
	}
	if( out_x2 > out_w ) {
		in_x2 -= (out_x2 - out_w) / xscale;
		out_x2 = out_w;
	}
	if( out_y1 < 0 ) {
		in_y1 -= out_y1 / yscale;
		out_y1 = 0;
	}
	if( out_y2 > out_h ) {
		in_y2 -= (out_y2 - out_h) / yscale;
		out_y2 = out_h;
	}

	if( in_x1 < 0) in_x1 = 0;
	if( in_y1 < 0) in_y1 = 0;
	if( in_x2 > in_w ) in_x2 = in_w;
	if( in_y2 > in_h ) in_y2 = in_h;
	if( out_x1 < 0) out_x1 = 0;
	if( out_y1 < 0) out_y1 = 0;
	if( out_x2 > out_w ) out_x2 = out_w;
	if( out_y2 > out_h ) out_y2 = out_h;

	if( in_x2 <= in_x1 || in_y2 <= in_y1 ) return 1;
	if( out_x2 <= out_x1 || out_y2 <= out_y1 ) return 1;
	xscale = (out_x2 - out_x1) / (in_x2 - in_x1);
	yscale = (out_y2 - out_y1) / (in_y2 - in_y1);

	/* don't interpolate integer translations, or scale no-ops */
	if(xscale == 1. && yscale == 1. &&
		(int)in_x1 == in_x1 && (int)in_x2 == in_x2 &&
		(int)in_y1 == in_y1 && (int)in_y2 == in_y2 &&
		(int)out_x1 == out_x1 && (int)out_x2 == out_x2 &&
		(int)out_y1 == out_y1 && (int)out_y2 == out_y2) {
		if(!direct_engine) direct_engine = new DirectEngine(cpus);

		direct_engine->output = output;   direct_engine->input = input;
		direct_engine->in_x1 = in_x1;     direct_engine->in_y1 = in_y1;
		direct_engine->out_x1 = out_x1;   direct_engine->out_x2 = out_x2;
		direct_engine->out_y1 = out_y1;   direct_engine->out_y2 = out_y2;
		direct_engine->alpha = alpha;     direct_engine->mode = mode;
		direct_engine->process_packages();
	}
	else if(interpolation_type == NEAREST_NEIGHBOR) {
		if(!nn_engine) nn_engine = new NNEngine(cpus);
		nn_engine->output = output;       nn_engine->input = input;
		nn_engine->in_x1 = in_x1;         nn_engine->in_x2 = in_x2;
		nn_engine->in_y1 = in_y1;         nn_engine->in_y2 = in_y2;
		nn_engine->out_x1 = out_x1;       nn_engine->out_x2 = out_x2;
		nn_engine->out_y1 = out_y1;       nn_engine->out_y2 = out_y2;
		nn_engine->alpha = alpha;         nn_engine->mode = mode;
		nn_engine->process_packages();
	}
	else {
		int xtype = BILINEAR;
		int ytype = BILINEAR;

		switch(interpolation_type)
		{
		case CUBIC_CUBIC: // Bicubic enlargement and reduction
			xtype = ytype = BICUBIC;
			break;
		case CUBIC_LINEAR: // Bicubic enlargement and bilinear reduction
			xtype = xscale > 1. ? BICUBIC : BILINEAR;
			ytype = yscale > 1. ? BICUBIC : BILINEAR;
			break;
		case LINEAR_LINEAR: // Bilinear enlargement and bilinear reduction
			xtype = ytype = BILINEAR;
			break;
		case LANCZOS_LANCZOS: // Because we can
			xtype = ytype = LANCZOS;
			break;
		}

		if(xscale == 1. && (int)in_x1 == in_x1 && (int)in_x2 == in_x2 &&
				(int)out_x1 == out_x1 && (int)out_x2 == out_x2)
			xtype = DIRECT_COPY;

		if(yscale == 1. && (int)in_y1 == in_y1 && (int)in_y2 == in_y2 &&
				(int)out_y1 == out_y1 && (int)out_y2 == out_y2)
			ytype = DIRECT_COPY;

		if(!kernel[xtype])
			kernel[xtype] = new OverlayKernel(xtype);
		if(!kernel[ytype])
			kernel[ytype] = new OverlayKernel(ytype);

/*
 * horizontal and vertical are separately resampled.  First we
 * resample the input along X into a transposed, temporary frame,
 * then resample/transpose the temporary space along X into the
 * output.  Fractional pixels along the edge are handled in the X
 * direction of each step
 */
		// resampled dimension matches the transposed output space
		float temp_y1 = out_x1 - floor(out_x1);
		float temp_y2 = temp_y1 + (out_x2 - out_x1);
		int temp_h = ceil(temp_y2);

		// non-resampled dimension merely cropped
		float temp_x1 = in_y1 - floor(in_y1);
		float temp_x2 = temp_x1 + (in_y2 - in_y1);
		int temp_w = ceil(temp_x2);

		if( temp_frame &&
		   (temp_frame->get_color_model() != input->get_color_model() ||
                    temp_frame->get_w() != temp_w || temp_frame->get_h() != temp_h) ) {
			delete temp_frame;
			temp_frame = 0;
		}

		if(!temp_frame) {
			temp_frame =
				new VFrame(temp_w, temp_h, input->get_color_model(), 0);
		}

		temp_frame->clear_frame();

		if(!sample_engine) sample_engine = new SampleEngine(cpus);

		sample_engine->output = temp_frame;
		sample_engine->input = input;
		sample_engine->kernel = kernel[xtype];
		sample_engine->col_out1 = 0;
		sample_engine->col_out2 = temp_w;
		sample_engine->row_in = floor(in_y1);

		sample_engine->in1 = in_x1;
		sample_engine->in2 = in_x2;
		sample_engine->out1 = temp_y1;
		sample_engine->out2 = temp_y2;
		sample_engine->alpha = 1.;
		sample_engine->mode = TRANSFER_REPLACE;
		sample_engine->process_packages();

		sample_engine->output = output;
		sample_engine->input = temp_frame;
		sample_engine->kernel = kernel[ytype];
		sample_engine->col_out1 = floor(out_x1);
		sample_engine->col_out2 = ceil(out_x2);
		sample_engine->row_in = 0;

		sample_engine->in1 = temp_x1;
		sample_engine->in2 = temp_x2;
		sample_engine->out1 = out_y1;
		sample_engine->out2 = out_y2;
		sample_engine->alpha = alpha;
		sample_engine->mode = mode;
		sample_engine->process_packages();
	}
	return 0;
}


