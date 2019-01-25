#include "clip.h"
#include "bccmodels.h"
#include "bchash.h"
#include "filexml.h"
#include "yuv411.h"
#include "yuv411win.h"
#include "language.h"

#include <stdint.h>
#include <string.h>

REGISTER_PLUGIN(yuv411Main)


yuv411Config::yuv411Config()
{
	reset();
}

void yuv411Config::reset()
{
	int_horizontal = 0;
	avg_vertical = 0;
	inpainting = 0;
	offset = 1;
	thresh = 4;
	bias = 1;
}

void yuv411Config::copy_from(yuv411Config &that)
{
	int_horizontal = that.int_horizontal;
	avg_vertical = that.avg_vertical;
	inpainting = that.inpainting;
	offset = that.offset;
	thresh = that.thresh;
	bias = that.bias;
}

int yuv411Config::equivalent(yuv411Config &that)
{
	return int_horizontal == that.int_horizontal &&
		avg_vertical == that.avg_vertical &&
		inpainting == that.inpainting &&
		offset == that.offset &&
		thresh == that.thresh &&
		bias == that.bias;
}

void yuv411Config::interpolate(yuv411Config &prev,
	yuv411Config &next,
	long prev_frame,
	long next_frame,
	long current_frame)
{
	this->int_horizontal = prev.int_horizontal;
	this->avg_vertical = prev.avg_vertical;
	this->inpainting = prev.inpainting;
	this->offset = prev.offset;
	this->thresh = prev.thresh;
	this->bias = prev.bias;
}


yuv411Main::yuv411Main(PluginServer *server)
 : PluginVClient(server)
{
	this->server = server;
	this->temp_frame = 0;
	colormodel = -1;
}

yuv411Main::~yuv411Main()
{
	delete temp_frame;
}

const char *yuv411Main::plugin_title() { return N_("YUV411"); }
int yuv411Main::is_realtime() { return 1; }

#define YUV411_MACRO(type, components) \
{ \
    type **input_rows = ((type**)input_ptr->get_rows()), **in_rows = input_rows; \
    type **output_rows = ((type**)output_ptr->get_rows()), **out_rows = output_rows; \
    if( config.avg_vertical ) { \
        if( config.int_horizontal ) \
            in_rows = out_rows = ((type**)temp_frame->get_rows()); \
        for( int i=0,h3=h-3; i<h3; i+=4 ) { \
            type *in_row0 = input_rows[i+0], *in_row1 = input_rows[i+1]; \
            type *in_row2 = input_rows[i+2], *in_row3 = input_rows[i+3]; \
            type *out_row0 = out_rows[i+0], *out_row1 = out_rows[i+1]; \
            type *out_row2 = out_rows[i+2], *out_row3 = out_rows[i+3]; \
            for( int k = 0; k<w; ++k ) { \
                for( int uv=1; uv<=2; ++uv ) { \
                   out_row0[uv] = out_row2[uv] = (in_row0[uv]+in_row2[uv]+1)/2; \
                   out_row1[uv] = out_row3[uv] = (in_row1[uv]+in_row3[uv]+1)/2; \
                } \
                in_row0 += components; in_row1 += components; \
                in_row2 += components; in_row3 += components; \
                out_row0 += components; out_row1 += components; \
                out_row2 += components; out_row3 += components; \
            } \
        } \
    } \
 \
    if( config.int_horizontal ) { \
        if( config.inpainting ) { \
            int kmax = w-7+config.offset; \
            for( int i=0; i<h; ++i ) { \
                type *in_row0 = in_rows[i+0], *out_row0 = output_rows[i+0]; \
                for( int k=config.offset; k<kmax; k+=4 ) { \
                    int k0 = (k+0) * components, a = in_row0[k0]; \
                    int b, d = 0, k4 = (k+4) * components; \
                    for( int jk=k0,n=4; --n>=0; a=b ) { \
                        b = in_row0[jk+=components]; \
                        d += a<b ? b-a : a-b;  d += config.bias; \
                    } \
                    if( d < config.thresh ) { \
                        for( int jk=k0,n=4; --n>0; ) { \
                            jk += components; \
                            for( int uv=1; uv<=2; ++uv ) { \
                                out_row0[jk+uv] = (n*in_row0[jk+uv] + (4-n)*in_row0[k4+uv]+2)/4; \
                            } \
                        } \
                    } \
                    else { \
                        int t = 0;  a = in_row0[k0]; \
                        for( int jk=k0,n=4; --n>0; a=b ) { \
                            b = in_row0[jk+=components]; \
                            t += a<b ? b-a : a-b;  t += config.bias; \
                            for( int uv=1; uv<=2; ++uv ) { \
                              out_row0[jk+uv] = (2*((d-t)*in_row0[k0+uv] + t*in_row0[k4+uv])+d)/(2*d); \
                            } \
                        } \
                    } \
                } \
            } \
        } \
        else { \
            int kmax = w-7+config.offset; \
            for( int i=0; i<h; ++i ) { \
                type *in_row0 = in_rows[i]; \
                type *out_row0 = output_rows[i]; \
                for( int k=config.offset; k<kmax; k+=4 ) { \
                    for( int uv=1; uv<=2; ++uv ) { \
                        int sum, avg, k0 = (k+0) * components, k4 = (k+4) * components; \
                        sum  = in_row0[k0 + uv]; sum += in_row0[(k0+=components) + uv]; \
                        sum += in_row0[k4 + uv]; sum += in_row0[(k4+=components) + uv]; \
                        avg = (sum + 2) / 4; \
                        out_row0[(k0+=components)+uv] = avg; \
                        out_row0[(k0+=components)+uv] = avg; \
                    } \
                } \
            } \
        } \
    } \
}

int yuv411Main::process_realtime(VFrame *input_ptr, VFrame *output_ptr)
{
	load_configuration();
	int w = input_ptr->get_w();
	int h = input_ptr->get_h();
	int colormodel = input_ptr->get_color_model();

	if( input_ptr == output_ptr ||
	    ( config.avg_vertical && config.int_horizontal ) ) {
		if( temp_frame && (temp_frame->get_color_model() != colormodel ||
		    temp_frame->get_w() != w || temp_frame->get_h() != h) ) {
			delete temp_frame;
			temp_frame = 0;
		}
		if( !temp_frame )
			temp_frame = new VFrame(w, h, colormodel, 0);
    		if( input_ptr == output_ptr ) {
			temp_frame->copy_from(input_ptr);
			input_ptr = temp_frame;
		}
	}

	switch( colormodel ) {
	case BC_YUV888:
		YUV411_MACRO(unsigned char, 3);
		break;
	case BC_YUVA8888:
		YUV411_MACRO(unsigned char, 4);
		break;
	}

	if( this->colormodel != colormodel ) {
		this->colormodel = colormodel;
		send_render_gui(this);
	}

	return 0;
}


NEW_WINDOW_MACRO(yuv411Main, yuv411Window)

LOAD_CONFIGURATION_MACRO(yuv411Main, yuv411Config)

void yuv411Main::update_gui()
{
	if(thread)
	{
		load_configuration();
		thread->window->lock_window();
		yuv411Window *window = (yuv411Window *)thread->window;
		window->update_enables();
		window->avg_vertical->update((int)config.avg_vertical);
		window->int_horizontal->update((int)config.int_horizontal);
		window->inpainting->update((int)config.inpainting);
		window->offset->update((int)config.offset);
		window->thresh->update((int)config.thresh);
		window->bias->update((int)config.bias);
		window->unlock_window();
	}
}

void yuv411Main::render_gui(void *data)
{
	if(thread) {
		thread->window->lock_window();
		yuv411Window *window = (yuv411Window *)thread->window;
		yuv411Main *client = (yuv411Main *)data;
		switch( client->colormodel ) {
		case BC_YUV888:
		case BC_YUVA8888:
			window->show_warning(0);
			break;
		default:
			window->show_warning(1);
			break;
		}
		window->unlock_window();
	}
}

void yuv411Main::save_data(KeyFrame *keyframe)
{
	FileXML output;

// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("YUV411");
	output.tag.set_property("OFFSET",config.offset);
	output.tag.set_property("THRESH",config.thresh);
	output.tag.set_property("BIAS",config.bias);
	output.append_tag();
	if(config.avg_vertical) {
		output.tag.set_title("VERTICAL");
		output.append_tag();
		output.tag.set_title("/VERTICAL");
		output.append_tag();
	}
	if(config.int_horizontal) {
		output.tag.set_title("HORIZONTAL");
		output.append_tag();
		output.tag.set_title("/HORIZONTAL");
		output.append_tag();
	}
	if(config.inpainting ) {
		output.tag.set_title("INPAINTING");
		output.append_tag();
		output.tag.set_title("/INPAINTING");
		output.append_tag();
	}
	output.tag.set_title("/YUV411");
	output.append_tag();
	output.terminate_string();
}

void yuv411Main::read_data(KeyFrame *keyframe)
{
	FileXML input;

	input.set_shared_input(keyframe->xbuf);

	int result = 0;
	config.avg_vertical = config.int_horizontal = 0;
	config.inpainting = 0;
	config.offset = 1;
	config.thresh = 4;
	config.bias = 1;

	while(!(result = input.read_tag()) ) {
		if( input.tag.title_is("YUV411") ) {
			config.offset = input.tag.get_property("OFFSET",config.offset);
			config.thresh = input.tag.get_property("THRESH",config.thresh);
			config.bias = input.tag.get_property("BIAS",config.bias);
		}
		else if(input.tag.title_is("VERTICAL")) {
			config.avg_vertical = 1;
		}
		else if(input.tag.title_is("HORIZONTAL")) {
			config.int_horizontal = 1;
		}
		else if(input.tag.title_is("INPAINTING")) {
			config.inpainting = 1;
		}
	}
}

