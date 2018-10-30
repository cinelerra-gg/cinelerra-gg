#include "overlayframe.h"

/* Nearest Neighbor scale / translate / blend ********************/

#define XBLEND_3NN(FN, temp_type, type, max, components, ofs, round) { \
	temp_type opcty = fade * max + round, trnsp = max - opcty; \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) { \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i] + ox; \
		for(int j = 0; j < ow; j++) { \
			in_row += *lx++; \
			if( components == 4 ) { \
				temp_type r, g, b, a; \
				ALPHA4_BLEND(FN, temp_type, in_row, output, max, ofs, ofs, round); \
				ALPHA4_STORE(output, ofs, max); \
			} \
			else { \
				temp_type r, g, b; \
				ALPHA3_BLEND(FN, temp_type, in_row, output, max, ofs, ofs, round); \
				ALPHA3_STORE(output, ofs, max); \
			} \
			output += components; \
		} \
	} \
	break; \
}

#define XBLEND_NN(FN) { \
	switch(input->get_color_model()) { \
	case BC_RGB_FLOAT:	XBLEND_3NN(FN, z_float,   z_float,    1.f,    3, 0,       0.f); \
	case BC_RGBA_FLOAT:	XBLEND_3NN(FN, z_float,   z_float,    1.f,    4, 0,       0.f); \
	case BC_RGB888:		XBLEND_3NN(FN, z_int32_t, z_uint8_t,  0xff,   3, 0,      .5f); \
	case BC_YUV888:		XBLEND_3NN(FN, z_int32_t, z_uint8_t,  0xff,   3, 0x80,   .5f); \
	case BC_RGBA8888:	XBLEND_3NN(FN, z_int32_t, z_uint8_t,  0xff,   4, 0,      .5f); \
	case BC_YUVA8888:	XBLEND_3NN(FN, z_int32_t, z_uint8_t,  0xff,   4, 0x80,   .5f); \
	case BC_RGB161616:	XBLEND_3NN(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0,      .5f); \
	case BC_YUV161616:	XBLEND_3NN(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0x8000, .5f); \
	case BC_RGBA16161616:	XBLEND_3NN(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0,      .5f); \
	case BC_YUVA16161616:	XBLEND_3NN(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0x8000, .5f); \
	} \
	break; \
}

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
	NNPackage *pkg = (NNPackage*)package;
	VFrame *output = engine->output;
	VFrame *input = engine->input;
	int mode = engine->mode;
	float fade =
		BC_CModels::has_alpha(input->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;

	int ox = engine->out_x1i;
	int ow = engine->out_x2i - ox;
	int *ly = engine->in_lookup_y + pkg->out_row1;

	BLEND_SWITCH(XBLEND_NN);
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


