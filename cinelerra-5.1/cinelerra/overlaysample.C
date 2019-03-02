#include "overlayframe.h"
#include "overlaysample.h"

/* Fully resampled scale / translate / blend ******************************/
/* resample into a temporary row vector, then blend */

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
	pkg = (SamplePackage*)package;

	float i1  = engine->in1;
	float i2  = engine->in2;
	float o1  = engine->out1;
	float o2  = engine->out2;

	if(i2 - i1 <= 0 || o2 - o1 <= 0)
		return;

	voutput = engine->output;
	vinput = engine->input;
	mode = engine->mode;
	fade = BC_CModels::has_alpha(vinput->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;

	//iw  = vinput->get_w();
	i1i = floor(i1);
	i2i = ceil(i2);
	i1f = 1.f - i1 + i1i;
	i2f = 1.f - i2i + i2;

	o1i = floor(o1);
	o2i = ceil(o2);
	o1f = 1.f - o1 + o1i;
	o2f = 1.f - o2i + o2;
	oh  = o2i - o1i;

	k  = engine->kernel->lookup;
	//kw  = engine->kernel->width;
	//kn  = engine->kernel->n;
	kd = engine->kd;

	lookup_sx0 = engine->lookup_sx0;
	lookup_sx1 = engine->lookup_sx1;
	lookup_sk = engine->lookup_sk;
	lookup_wacc = engine->lookup_wacc;

	switch( vinput->get_color_model() ) {
	case BC_RGB_FLOAT:	rgb_float();	break;
	case BC_RGBA_FLOAT:	rgba_float();	break;
	case BC_RGB888:		rgb888();	break;
	case BC_YUV888:		yuv888();	break;
	case BC_RGBA8888:	rgba8888();	break;
	case BC_YUVA8888:	yuva8888();	break;
	case BC_RGB161616:	rgb161616();	break;
	case BC_YUV161616:	yuv161616();	break;
	case BC_RGBA16161616:	rgba16161616();	break;
	case BC_YUVA16161616:	yuva16161616();  break;
	}
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


