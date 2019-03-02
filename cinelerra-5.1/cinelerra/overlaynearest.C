#include "overlayframe.h"
#include "overlaynearest.h"

/* Nearest Neighbor scale / translate / blend ********************/

NNPackage::NNPackage()
{
}

NNUnit::NNUnit(NNEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

NNUnit::~NNUnit()
{
}

void NNUnit::process_package(LoadPackage *package)
{
	pkg = (NNPackage*)package;
	output = engine->output;
	input = engine->input;
	mode = engine->mode;
	fade = BC_CModels::has_alpha(input->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;

	ox = engine->out_x1i;
	ow = engine->out_x2i - ox;
	ly = engine->in_lookup_y + pkg->out_row1;

	switch(input->get_color_model()) {
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

NNEngine::NNEngine(int cpus)
 : LoadServer(cpus, cpus)
{
	in_lookup_x = 0;
	in_lookup_y = 0;
}

NNEngine::~NNEngine()
{
	if(in_lookup_x)
		delete[] in_lookup_x;
	if(in_lookup_y)
		delete[] in_lookup_y;
}

void NNEngine::init_packages()
{
	int in_w = input->get_w();
	int in_h = input->get_h();
	int out_w = output->get_w();
	int out_h = output->get_h();

	float in_subw = in_x2 - in_x1;
	float in_subh = in_y2 - in_y1;
	float out_subw = out_x2 - out_x1;
	float out_subh = out_y2 - out_y1;
	int first, last, count, i;
	int components = 3;

	out_x1i = rint(out_x1);
	out_x2i = rint(out_x2);
	if(out_x1i < 0) out_x1i = 0;
	if(out_x1i > out_w) out_x1i = out_w;
	if(out_x2i < 0) out_x2i = 0;
	if(out_x2i > out_w) out_x2i = out_w;
	int out_wi = out_x2i - out_x1i;
	if( !out_wi ) return;

	delete[] in_lookup_x;
	in_lookup_x = new int[out_wi];
	delete[] in_lookup_y;
	in_lookup_y = new int[out_h];

	switch(input->get_color_model()) {
	case BC_RGBA_FLOAT:
	case BC_RGBA8888:
	case BC_YUVA8888:
	case BC_RGBA16161616:
		components = 4;
		break;
	}

	first = count = 0;

	for(i = out_x1i; i < out_x2i; i++) {
		int in = (i - out_x1 + .5) * in_subw / out_subw + in_x1;
		if(in < in_x1)
			in = in_x1;
		if(in > in_x2)
			in = in_x2;

		if(in >= 0 && in < in_w && in >= in_x1 && i >= 0 && i < out_w) {
			if(count == 0) {
				first = i;
				in_lookup_x[0] = in * components;
			}
			else {
				in_lookup_x[count] = (in-last)*components;
			}
			last = in;
			count++;
		}
		else if(count)
			break;
	}
	out_x1i = first;
	out_x2i = first + count;
	first = count = 0;

	for(i = out_y1; i < out_y2; i++) {
		int in = (i - out_y1+.5) * in_subh / out_subh + in_y1;
		if(in < in_y1) in = in_y1;
		if(in > in_y2) in = in_y2;
		if(in >= 0 && in < in_h && i >= 0 && i < out_h) {
			if(count == 0) first = i;
			in_lookup_y[i] = in;
			count++;
		}
		else if(count)
			break;
	}
	out_y1 = first;
	out_y2 = first + count;

	int rows = count;
	int pkgs = get_total_packages();
	int row1 = out_y1, row2 = row1;
	for(int i = 0; i < pkgs; row1=row2 ) {
		NNPackage *package = (NNPackage*)get_package(i);
		row2 = ++i * rows / pkgs + out_y1;
		package->out_row1 = row1;
		package->out_row2 = row2;
	}
}

LoadClient* NNEngine::new_client()
{
	return new NNUnit(this);
}

LoadPackage* NNEngine::new_package()
{
	return new NNPackage;
}


