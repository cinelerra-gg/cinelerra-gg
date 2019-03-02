#include "overlayframe.h"
#include "overlaydirect.h"

/* Direct translate / blend **********************************************/

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
	pkg = (DirectPackage*)package;
	output = engine->output;
	input = engine->input;
	mode = engine->mode;
	fade = BC_CModels::has_alpha(input->get_color_model()) &&
		mode == TRANSFER_REPLACE ? 1.f : engine->alpha;
	ix = engine->in_x1;
	ox = engine->out_x1;
	ow = engine->out_x2 - ox;
	iy = engine->in_y1 - engine->out_y1;

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


