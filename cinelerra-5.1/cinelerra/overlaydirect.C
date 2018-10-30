#include "overlayframe.h"

/* Direct translate / blend **********************************************/

#define XBLEND(FN, temp_type, type, max, components, ofs, round) { \
	temp_type opcty = fade * max + round, trnsp = max - opcty; \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ix *= components;  ox *= components; \
 \
	for(int i = pkg->out_row1; i < pkg->out_row2; i++) { \
		type* in_row = input_rows[i + iy] + ix; \
		type* output = output_rows[i] + ox; \
		for(int j = 0; j < ow; j++) { \
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
			in_row += components;  output += components; \
		} \
	} \
	break; \
}

#define XBLEND_ONLY(FN) { \
	switch(input->get_color_model()) { \
	case BC_RGB_FLOAT:	XBLEND(FN, z_float,   z_float,    1.f,    3, 0,      0.f); \
	case BC_RGBA_FLOAT:	XBLEND(FN, z_float,   z_float,    1.f,    4, 0,      0.f); \
	case BC_RGB888:		XBLEND(FN, z_int32_t, z_uint8_t,  0xff,   3, 0,      .5f); \
	case BC_YUV888:		XBLEND(FN, z_int32_t, z_uint8_t,  0xff,   3, 0x80,   .5f); \
	case BC_RGBA8888:	XBLEND(FN, z_int32_t, z_uint8_t,  0xff,   4, 0,      .5f); \
	case BC_YUVA8888:	XBLEND(FN, z_int32_t, z_uint8_t,  0xff,   4, 0x80,   .5f); \
	case BC_RGB161616:	XBLEND(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0,      .5f); \
	case BC_YUV161616:	XBLEND(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0x8000, .5f); \
	case BC_RGBA16161616:	XBLEND(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0,      .5f); \
	case BC_YUVA16161616:	XBLEND(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0x8000, .5f); \
	} \
	break; \
}

DirectPackage::DirectPackage()
{
}

DirectUnit::DirectUnit(DirectEngine *server)
 : LoadClient(server)
{
	this->engine = server;
}

DirectUnit::~DirectUnit()
{
}

void DirectUnit::process_package(LoadPackage *package)
{
	DirectPackage *pkg = (DirectPackage*)package;

	VFrame *output = engine->output;
	VFrame *input = engine->input;
	int mode = engine->mode;
	float fade =
		BC_CModels::has_alpha(input->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;

	int ix = engine->in_x1;
	int ox = engine->out_x1;
	int ow = engine->out_x2 - ox;
	int iy = engine->in_y1 - engine->out_y1;

	BLEND_SWITCH(XBLEND_ONLY);
}

DirectEngine::DirectEngine(int cpus)
 : LoadServer(cpus, cpus)
{
}

DirectEngine::~DirectEngine()
{
}

void DirectEngine::init_packages()
{
	if(in_x1 < 0) { out_x1 -= in_x1; in_x1 = 0; }
	if(in_y1 < 0) { out_y1 -= in_y1; in_y1 = 0; }
	if(out_x1 < 0) { in_x1 -= out_x1; out_x1 = 0; }
	if(out_y1 < 0) { in_y1 -= out_y1; out_y1 = 0; }
	if(out_x2 > output->get_w()) out_x2 = output->get_w();
	if(out_y2 > output->get_h()) out_y2 = output->get_h();
	int out_w = out_x2 - out_x1;
	int out_h = out_y2 - out_y1;
	if( !out_w || !out_h ) return;

	int rows = out_h;
	int pkgs = get_total_packages();
	int row1 = out_y1, row2 = row1;
	for(int i = 0; i < pkgs; row1=row2 ) {
		DirectPackage *package = (DirectPackage*)get_package(i);
		row2 = ++i * rows / pkgs + out_y1;
		package->out_row1 = row1;
		package->out_row2 = row2;
	}
}

LoadClient* DirectEngine::new_client()
{
	return new DirectUnit(this);
}

LoadPackage* DirectEngine::new_package()
{
	return new DirectPackage;
}


