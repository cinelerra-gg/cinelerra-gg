#include "overlayframe.h"

/* Fully resampled scale / translate / blend ******************************/
/* resample into a temporary row vector, then blend */

#define XSAMPLE(FN, temp_type, type, max, components, ofs, round) { \
	float temp[oh*components]; \
	temp_type opcty = fade * max + round, trnsp = max - opcty; \
	type **output_rows = (type**)voutput->get_rows() + o1i; \
	type **input_rows = (type**)vinput->get_rows(); \
 \
	for(int i = pkg->out_col1; i < pkg->out_col2; i++) { \
		type *input = input_rows[i - engine->col_out1 + engine->row_in]; \
		float *tempp = temp; \
		if( !k ) { /* direct copy case */ \
			type *ip = input + i1i * components; \
			for(int j = 0; j < oh; j++) { \
				*tempp++ = *ip++; \
				*tempp++ = *ip++ - ofs; \
				*tempp++ = *ip++ - ofs; \
				if( components == 4 ) *tempp++ = *ip++; \
			} \
		} \
		else { /* resample */ \
			for(int j = 0; j < oh; j++) { \
				float racc=0.f, gacc=0.f, bacc=0.f, aacc=0.f; \
				int ki = lookup_sk[j], x = lookup_sx0[j]; \
				type *ip = input + x * components; \
				while(x < lookup_sx1[j]) { \
					float kv = k[abs(ki >> INDEX_FRACTION)]; \
					/* handle fractional pixels on edges of input */ \
					if(x == i1i) kv *= i1f; \
					if(++x == i2i) kv *= i2f; \
					racc += kv * *ip++; \
					gacc += kv * (*ip++ - ofs); \
					bacc += kv * (*ip++ - ofs); \
					if( components == 4 ) { aacc += kv * *ip++; } \
					ki += kd; \
				} \
				float wacc = lookup_wacc[j]; \
				*tempp++ = racc * wacc; \
				*tempp++ = gacc * wacc; \
				*tempp++ = bacc * wacc; \
				if( components == 4 ) { *tempp++ = aacc * wacc; } \
			} \
		} \
 \
		/* handle fractional pixels on edges of output */ \
		temp[0] *= o1f;   temp[1] *= o1f;   temp[2] *= o1f; \
		if( components == 4 ) temp[3] *= o1f; \
		tempp = temp + (oh-1)*components; \
		tempp[0] *= o2f;  tempp[1] *= o2f;  tempp[2] *= o2f; \
		if( components == 4 ) tempp[3] *= o2f; \
		tempp = temp; \
		/* blend output */ \
		for(int j = 0; j < oh; j++) { \
			type *output = output_rows[j] + i * components; \
			if( components == 4 ) { \
				temp_type r, g, b, a; \
				ALPHA4_BLEND(FN, temp_type, tempp, output, max, 0, ofs, round); \
				ALPHA4_STORE(output, ofs, max); \
			} \
			else { \
				temp_type r, g, b; \
				ALPHA3_BLEND(FN, temp_type, tempp, output, max, 0, ofs, round); \
				ALPHA3_STORE(output, ofs, max); \
			} \
			tempp += components; \
		} \
	} \
	break; \
}

#define XBLEND_SAMPLE(FN) { \
        switch(vinput->get_color_model()) { \
        case BC_RGB_FLOAT:      XSAMPLE(FN, z_float,   z_float,    1.f,    3, 0.f,    0.f); \
        case BC_RGBA_FLOAT:     XSAMPLE(FN, z_float,   z_float,    1.f,    4, 0.f,    0.f); \
        case BC_RGB888:         XSAMPLE(FN, z_int32_t, z_uint8_t,  0xff,   3, 0,      .5f); \
        case BC_YUV888:         XSAMPLE(FN, z_int32_t, z_uint8_t,  0xff,   3, 0x80,   .5f); \
        case BC_RGBA8888:       XSAMPLE(FN, z_int32_t, z_uint8_t,  0xff,   4, 0,      .5f); \
        case BC_YUVA8888:       XSAMPLE(FN, z_int32_t, z_uint8_t,  0xff,   4, 0x80,   .5f); \
        case BC_RGB161616:      XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0,      .5f); \
        case BC_YUV161616:      XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0x8000, .5f); \
        case BC_RGBA16161616:   XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0,      .5f); \
        case BC_YUVA16161616:   XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0x8000, .5f); \
        } \
        break; \
}


SamplePackage::SamplePackage()
{
}

SampleUnit::SampleUnit(SampleEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

SampleUnit::~SampleUnit()
{
}

void SampleUnit::process_package(LoadPackage *package)
{
	SamplePackage *pkg = (SamplePackage*)package;

	float i1  = engine->in1;
	float i2  = engine->in2;
	float o1  = engine->out1;
	float o2  = engine->out2;

	if(i2 - i1 <= 0 || o2 - o1 <= 0)
		return;

	VFrame *voutput = engine->output;
	VFrame *vinput = engine->input;
	int mode = engine->mode;
	float fade =
		BC_CModels::has_alpha(vinput->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;

	//int   iw  = vinput->get_w();
	int   i1i = floor(i1);
	int   i2i = ceil(i2);
	float i1f = 1.f - i1 + i1i;
	float i2f = 1.f - i2i + i2;

	int   o1i = floor(o1);
	int   o2i = ceil(o2);
	float o1f = 1.f - o1 + o1i;
	float o2f = 1.f - o2i + o2;
	int   oh  = o2i - o1i;

	float *k  = engine->kernel->lookup;
	//float kw  = engine->kernel->width;
	//int   kn  = engine->kernel->n;
	int   kd = engine->kd;

	int *lookup_sx0 = engine->lookup_sx0;
	int *lookup_sx1 = engine->lookup_sx1;
	int *lookup_sk = engine->lookup_sk;
	float *lookup_wacc = engine->lookup_wacc;

	BLEND_SWITCH(XBLEND_SAMPLE);
}


SampleEngine::SampleEngine(int cpus)
 : LoadServer(cpus, cpus)
{
	lookup_sx0 = 0;
	lookup_sx1 = 0;
	lookup_sk = 0;
	lookup_wacc = 0;
	kd = 0;
}

SampleEngine::~SampleEngine()
{
	if(lookup_sx0) delete [] lookup_sx0;
	if(lookup_sx1) delete [] lookup_sx1;
	if(lookup_sk) delete [] lookup_sk;
	if(lookup_wacc) delete [] lookup_wacc;
}

/*
 * unlike the Direct and NN engines, the Sample engine works across
 * output columns (it makes for more economical memory addressing
 * during convolution)
 */
void SampleEngine::init_packages()
{
	int   iw  = input->get_w();
	int   i1i = floor(in1);
	int   i2i = ceil(in2);
	float i1f = 1.f - in1 + i1i;
	float i2f = 1.f - i2i + in2;

	int   oy  = floor(out1);
	float oyf = out1 - oy;
	int   oh  = ceil(out2) - oy;

	float *k  = kernel->lookup;
	float kw  = kernel->width;
	int   kn  = kernel->n;

	if(in2 - in1 <= 0 || out2 - out1 <= 0)
		return;

	/* determine kernel spatial coverage */
	float scale = (out2 - out1) / (in2 - in1);
	float iscale = (in2 - in1) / (out2 - out1);
	float coverage = fabs(1.f / scale);
	float bound = (coverage < 1.f ? kw : kw * coverage) - (.5f / TRANSFORM_SPP);
	float coeff = (coverage < 1.f ? 1.f : scale) * TRANSFORM_SPP;

	delete [] lookup_sx0;
	delete [] lookup_sx1;
	delete [] lookup_sk;
	delete [] lookup_wacc;

	lookup_sx0 = new int[oh];
	lookup_sx1 = new int[oh];
	lookup_sk = new int[oh];
	lookup_wacc = new float[oh];

	kd = (double)coeff * (1 << INDEX_FRACTION) + .5;

	/* precompute kernel values and weight sums */
	for(int i = 0; i < oh; i++) {
		/* map destination back to source */
		double sx = (i - oyf + .5) * iscale + in1 - .5;

		/*
		 * clip iteration to source area but not source plane. Points
		 * outside the source plane count as transparent. Points outside
		 * the source area don't count at all.  The actual convolution
		 * later will be clipped to both, but we need to compute
		 * weights.
		 */
		int sx0 = mmax((int)floor(sx - bound) + 1, i1i);
		int sx1 = mmin((int)ceil(sx + bound), i2i);
		int ki = (double)(sx0 - sx) * coeff * (1 << INDEX_FRACTION)
				+ (1 << (INDEX_FRACTION - 1)) + .5;
		float wacc=0.;

		lookup_sx0[i] = -1;
		lookup_sx1[i] = -1;

		for(int j= sx0; j < sx1; j++) {
			int kv = (ki >> INDEX_FRACTION);
			if(kv > kn) break;
			if(kv >= -kn) {
				/*
				 * the contribution of the first and last input pixel (if
				 * fractional) are linearly weighted by the fraction
				 */
				float fk = k[abs(kv)];
				wacc += j == i1i ? fk * i1f : j+1 == i2i ? fk * i2f : fk;

				/* this is where we clip the kernel convolution to the source plane */
				if(j >= 0 && j < iw) {
					if(lookup_sx0[i] == -1) {
						lookup_sx0[i] = j;
						lookup_sk[i] = ki;
					}
					lookup_sx1[i] = j + 1;
				}
			}
			ki += kd;
		}
		lookup_wacc[i] = wacc > 0. ? 1. / wacc : 0.;
	}

	int cols = col_out2 - col_out1;
	int pkgs = get_total_packages();
	int col1 = col_out1, col2 = col1;
	for(int i = 0; i < pkgs; col1=col2 ) {
		SamplePackage *package = (SamplePackage*)get_package(i);
		col2 = ++i * cols / pkgs + col_out1;
		package->out_col1 = col1;
		package->out_col2 = col2;
	}
}

LoadClient* SampleEngine::new_client()
{
	return new SampleUnit(this);
}

LoadPackage* SampleEngine::new_package()
{
	return new SamplePackage;
}


