
/*
 * CINELERRA
 * Copyright (C) 2008-2017 Adam Williams <broadcast at earthling dot net>
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

#include "assets.h"
#include "autos.h"
#include "bccapture.h"
#include "bccmodels.h"
#include "bcsignals.h"
#include "canvas.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mwindow.h"
#include "mwindowgui.h"
#include "playback3d.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"
#include "strategies.inc"
#include "vdevicex11.h"
#include "vframe.h"
#include "videodevice.h"
#include "videowindow.h"
#include "videowindowgui.h"

#include <string.h>
#include <unistd.h>

VDeviceX11::VDeviceX11(VideoDevice *device, Canvas *output)
 : VDeviceBase(device)
{
	reset_parameters();
	this->output = output;
}

VDeviceX11::~VDeviceX11()
{
	close_all();
}

int VDeviceX11::reset_parameters()
{
	output_frame = 0;
	window_id = 0;
	bitmap = 0;
	bitmap_type = 0;
	bitmap_w = 0;
	bitmap_h = 0;
	output_x1 = 0;
	output_y1 = 0;
	output_x2 = 0;
	output_y2 = 0;
	canvas_x1 = 0;
	canvas_y1 = 0;
	canvas_x2 = 0;
	canvas_y2 = 0;
	capture_bitmap = 0;
	color_model_selected = 0;
	is_cleared = 0;

	for( int i = 0; i < SCREENCAP_BORDERS; i++ ) {
		screencap_border[i] = 0;
	}
	return 0;
}

int VDeviceX11::open_input()
{
//printf("VDeviceX11::open_input 1\n");
	capture_bitmap = new BC_Capture(device->in_config->w,
		device->in_config->h,
		device->in_config->screencapture_display);
//printf("VDeviceX11::open_input 2\n");

// create overlay
	device->mwindow->gui->lock_window("VDeviceX11::close_all");

	screencap_border[0] = new BC_Popup(device->mwindow->gui,
			device->input_x - SCREENCAP_PIXELS, device->input_y - SCREENCAP_PIXELS,
			device->in_config->w + SCREENCAP_PIXELS * 2, SCREENCAP_PIXELS,
			SCREENCAP_COLOR, 1);
	screencap_border[1] = new BC_Popup(device->mwindow->gui,
			device->input_x - SCREENCAP_PIXELS, device->input_y,
			SCREENCAP_PIXELS, device->in_config->h,
			SCREENCAP_COLOR, 1);
	screencap_border[2] = new BC_Popup(device->mwindow->gui,
			device->input_x - SCREENCAP_PIXELS, device->input_y + device->in_config->h,
			device->in_config->w + SCREENCAP_PIXELS * 2, SCREENCAP_PIXELS,
			SCREENCAP_COLOR, 1);
	screencap_border[3] = new BC_Popup(device->mwindow->gui,
			device->input_x + device->in_config->w, device->input_y,
			SCREENCAP_PIXELS, device->in_config->h,
			SCREENCAP_COLOR, 1);
usleep(500000);	// avoids a bug in gnome-shell 2017/10/19

	for( int i=0; i<SCREENCAP_BORDERS; ++i )
		screencap_border[i]->show_window(0);

	device->mwindow->gui->flush();
	device->mwindow->gui->unlock_window();

	return 0;
}

int VDeviceX11::open_output()
{
	if( output ) {
		output->lock_canvas("VDeviceX11::open_output");
		output->get_canvas()->lock_window("VDeviceX11::open_output");
		if( !device->single_frame )
			output->start_video();
		else
			output->start_single();
		output->get_canvas()->unlock_window();

// Enable opengl in the first routine that needs it, to reduce the complexity.
		output->unlock_canvas();
	}
	return 0;
}


int VDeviceX11::output_visible()
{
	if( !output ) return 0;

	output->lock_canvas("VDeviceX11::output_visible");
	if( output->get_canvas()->get_hidden() ) {
		output->unlock_canvas();
		return 0;
	}
	else {
		output->unlock_canvas();
		return 1;
	}
}


int VDeviceX11::close_all()
{
	if( output ) {
		output->lock_canvas("VDeviceX11::close_all 1");
		output->get_canvas()->lock_window("VDeviceX11::close_all 1");
		int video_on = output->get_canvas()->get_video_on();
// Update the status bug
		if( !device->single_frame ) {
			output->stop_video();
		}
		else {
			output->stop_single();
		}
		if( output_frame ) {
			output->update_refresh(device, output_frame);
// if the last frame is good, don't draw over it
			if( !video_on || output->need_overlays() )
				output->draw_refresh(1);
		}
	}

	delete bitmap;          bitmap = 0;
	delete output_frame;    output_frame = 0;
	delete capture_bitmap;  capture_bitmap = 0;

	if( output ) {
		output->get_canvas()->unlock_window();
		output->unlock_canvas();
	}

	if( device->mwindow ) {
		device->mwindow->gui->lock_window("VDeviceX11::close_all");
		for( int i=0; i<SCREENCAP_BORDERS; ++i ) {
			delete screencap_border[i];
			screencap_border[i] = 0;
		}
		device->mwindow->gui->unlock_window();
	}

	reset_parameters();

	return 0;
}

int VDeviceX11::read_buffer(VFrame *frame)
{
//printf("VDeviceX11::read_buffer %d colormodel=%d\n", __LINE__, frame->get_color_model());
	device->mwindow->gui->lock_window("VDeviceX11::close_all");

	screencap_border[0]->reposition_window(device->input_x - SCREENCAP_PIXELS,
			device->input_y - SCREENCAP_PIXELS);
	screencap_border[1]->reposition_window(device->input_x - SCREENCAP_PIXELS,
			device->input_y);
	screencap_border[2]->reposition_window(device->input_x - SCREENCAP_PIXELS,
			device->input_y + device->in_config->h);
	screencap_border[3]->reposition_window(device->input_x + device->in_config->w,
			device->input_y);
	device->mwindow->gui->flush();
	device->mwindow->gui->unlock_window();


	capture_bitmap->capture_frame(frame,
		device->input_x, device->input_y, device->do_cursor);
	return 0;
}


int VDeviceX11::get_best_colormodel(Asset *asset)
{
	return File::get_best_colormodel(asset, SCREENCAPTURE);
//	return BC_RGB888;
}


int VDeviceX11::get_display_colormodel(int file_colormodel)
{
	int result = -1;

	if( device->out_config->driver == PLAYBACK_X11_GL ) {
		if( file_colormodel == BC_RGB888 ||
		    file_colormodel == BC_RGBA8888 ||
		    file_colormodel == BC_YUV888 ||
		    file_colormodel == BC_YUVA8888 ||
		    file_colormodel == BC_RGB_FLOAT ||
		    file_colormodel == BC_RGBA_FLOAT ) {
			return file_colormodel;
		}

		return BC_RGB888;
	}

	if( !device->single_frame ) {
		switch( file_colormodel ) {
		case BC_YUV420P:
		case BC_YUV422P:
		case BC_YUV422:
			result = file_colormodel;
			break;
		}
	}

	if( result < 0 ) {
		switch( file_colormodel ) {
		case BC_RGB888:
		case BC_RGBA8888:
		case BC_YUV888:
		case BC_YUVA8888:
		case BC_RGB_FLOAT:
		case BC_RGBA_FLOAT:
			result = file_colormodel;
			break;

		default:
			output->lock_canvas("VDeviceX11::get_display_colormodel");
			result = output->get_canvas()->get_color_model();
			output->unlock_canvas();
			break;
		}
	}

	return result;
}


void VDeviceX11::new_output_buffer(VFrame **result, int file_colormodel, EDL *edl)
{
// printf("VDeviceX11::new_output_buffer %d hardware_scaling=%d\n",
// __LINE__, bitmap ? bitmap->hardware_scaling() : 0);
	output->lock_canvas("VDeviceX11::new_output_buffer");
	output->get_canvas()->lock_window("VDeviceX11::new_output_buffer 1");

// Get the best colormodel the display can handle.
	int display_colormodel = get_display_colormodel(file_colormodel);

//printf("VDeviceX11::new_output_buffer %d file_colormodel=%d display_colormodel=%d\n",
// __LINE__, file_colormodel, display_colormodel);
// Only create OpenGL Pbuffer and texture.
	if( device->out_config->driver == PLAYBACK_X11_GL ) {
// Create bitmap for initial load into texture.
// Not necessary to do through Playback3D.....yet
		if( !output_frame ) {
			output_frame = new VFrame(device->out_w, device->out_h, file_colormodel);
		}

		window_id = output->get_canvas()->get_id();
		output_frame->set_opengl_state(VFrame::RAM);
	}
	else {
		output->get_transfers(edl,
			output_x1, output_y1, output_x2, output_y2,
			canvas_x1, canvas_y1, canvas_x2, canvas_y2,
// Canvas may be a different size than the temporary bitmap for pure software
			-1, -1);
		canvas_w = canvas_x2 - canvas_x1;
		canvas_h = canvas_y2 - canvas_y1;
// can the direct frame be used?
		int direct_supported =
			device->out_config->use_direct_x11 &&
			!output->xscroll && !output->yscroll &&
			output_x1 == 0 && output_x2 == device->out_w &&
			output_y1 == 0 && output_y2 == device->out_h;

// file wants direct frame but we need a temp
		if( !direct_supported && file_colormodel == BC_BGR8888 )
			file_colormodel = BC_RGB888;

// Conform existing bitmap to new colormodel and output size
		if( bitmap ) {
// printf("VDeviceX11::new_output_buffer %d bitmap=%dx%d canvas=%dx%d canvas=%dx%d\n",
// __LINE__, bitmap->get_w(), bitmap->get_h(), canvas_w, canvas_h,);
			int size_change = (
				bitmap->get_w() != canvas_w ||
				bitmap->get_h() != canvas_h );

// Restart if output size changed or output colormodel changed.
// May have to recreate if transferring between windowed and fullscreen.
			if( !color_model_selected ||
			    file_colormodel != output_frame->get_color_model() ||
			    (!bitmap->hardware_scaling() && size_change) ) {
//printf("VDeviceX11::new_output_buffer %d file_colormodel=%d prev "
// "file_colormodel=%d bitmap=%p output_frame=%p\n", __LINE__,
// file_colormodel, output_frame->get_color_model(), bitmap, output_frame);
				delete bitmap;        bitmap = 0;
				delete output_frame;  output_frame = 0;
// Clear borders if size changed
				if( size_change ) {
//printf("VDeviceX11::new_output_buffer %d w=%d h=%d "
// "canvas_x1=%d canvas_y1=%d canvas_x2=%d canvas_y2=%d\n",
// __LINE__, // (int)output->w, (int)output->h,
// (int)canvas_x1, (int)canvas_y1, (int)canvas_x2, (int)canvas_y2);
					output->get_canvas()->set_color(BLACK);

					if( canvas_y1 > 0 ) {
						output->get_canvas()->draw_box(0, 0, output->w, canvas_y1);
						output->get_canvas()->flash(0, 0, output->w, canvas_y1);
					}

					if( canvas_y2 < output->h ) {
						output->get_canvas()->draw_box(0, canvas_y2, output->w, output->h - canvas_y2);
						output->get_canvas()->flash(0, canvas_y2, output->w, output->h - canvas_y2);
					}

					if( canvas_x1 > 0 ) {
						output->get_canvas()->draw_box(0, canvas_y1, canvas_x1, canvas_y2 - canvas_y1);
						output->get_canvas()->flash(0, canvas_y1, canvas_x1, canvas_y2 - canvas_y1);
					}

					if( canvas_x2 < output->w ) {
						output->get_canvas()->draw_box(canvas_x2, canvas_y1,
							output->w - canvas_x2, canvas_y2 - canvas_y1);
						output->get_canvas()->flash(canvas_x2, canvas_y1,
							output->w - canvas_x2, canvas_y2 - canvas_y1);
					}
				}
			}
		}

// Create new bitmap
		if( !bitmap ) {
			int use_direct = 0;
			bitmap_type = BITMAP_TEMP;
//printf("VDeviceX11::new_output_buffer %d file_colormodel=%d display_colormodel=%d\n",
// __LINE__, file_colormodel, display_colormodel);

// Try hardware accelerated
			switch( display_colormodel ) {
// blit from the codec directly to the window, using the standard X11 color model.
// Must scale in the codec.  No cropping
			case BC_BGR8888:
				if( direct_supported ) {
					bitmap_type = BITMAP_PRIMARY;
					use_direct = 1;
				}
				break;

			case BC_YUV420P:
				if( device->out_config->driver == PLAYBACK_X11_XV &&
				    output->get_canvas()->accel_available(display_colormodel, 0) &&
				    !output->use_scrollbars )
					bitmap_type = BITMAP_PRIMARY;
				break;

			case BC_YUV422P:
				if( device->out_config->driver == PLAYBACK_X11_XV &&
				    output->get_canvas()->accel_available(display_colormodel, 0) &&
				    !output->use_scrollbars )
					bitmap_type = BITMAP_PRIMARY;
				else if( device->out_config->driver == PLAYBACK_X11_XV &&
				    output->get_canvas()->accel_available(BC_YUV422, 0) ) {
					bitmap = new BC_Bitmap(output->get_canvas(),
						device->out_w, device->out_h, BC_YUV422, 1);
				}
				break;

			case BC_YUV422:
				if( device->out_config->driver == PLAYBACK_X11_XV &&
				    output->get_canvas()->accel_available(display_colormodel, 0) &&
				    !output->use_scrollbars ) {
					bitmap_type = BITMAP_PRIMARY;
				}
				else if( device->out_config->driver == PLAYBACK_X11_XV &&
				    output->get_canvas()->accel_available(BC_YUV422P, 0) ) {
					bitmap = new BC_Bitmap(output->get_canvas(),
						device->out_w, device->out_h, BC_YUV422P, 1);
				}
				break;
			}
			if( bitmap_type == BITMAP_PRIMARY ) {
				int bitmap_w = use_direct ? canvas_w : device->out_w;
				int bitmap_h = use_direct ? canvas_h : device->out_h;
				bitmap = new BC_Bitmap(output->get_canvas(),
					bitmap_w, bitmap_h,  display_colormodel, -1);
				output_frame = new VFrame(bitmap,
					bitmap_w, bitmap_h, display_colormodel, -1);
			}
// Make an intermediate frame
			if( !bitmap ) {
				display_colormodel = output->get_canvas()->get_color_model();
// printf("VDeviceX11::new_output_buffer %d creating temp display_colormodel=%d "
// "file_colormodel=%d %dx%d %dx%d %dx%d\n", __LINE__,
// display_colormodel, file_colormodel, device->out_w, device->out_h,
// output->get_canvas()->get_w(), output->get_canvas()->get_h(), canvas_w, canvas_h);
				bitmap = new BC_Bitmap(output->get_canvas(),
					canvas_w, canvas_h, display_colormodel, 1);
				bitmap_type = BITMAP_TEMP;
			}

			if( bitmap_type == BITMAP_TEMP ) {
// Intermediate frame
//printf("VDeviceX11::new_output_buffer %d creating output_frame\n", __LINE__);
				output_frame = new VFrame(device->out_w, device->out_h, file_colormodel);
			}
			color_model_selected = 1;
		}
		else if( bitmap_type == BITMAP_PRIMARY ) {
			output_frame->set_memory(bitmap);
		}
	}

	*result = output_frame;
//printf("VDeviceX11::new_output_buffer 10 %d\n", output->get_canvas()->get_window_lock());

	output->get_canvas()->unlock_window();
	output->unlock_canvas();
}


int VDeviceX11::start_playback()
{
// Record window is initialized when its monitor starts.
	if( !device->single_frame )
		output->start_video();
	return 0;
}

int VDeviceX11::stop_playback()
{
	if( !device->single_frame )
		output->stop_video();
// Record window goes back to monitoring
// get the last frame played and store it in the video_out
	return 0;
}

int VDeviceX11::write_buffer(VFrame *output_channels, EDL *edl)
{
	output->lock_canvas("VDeviceX11::write_buffer");
	output->get_canvas()->lock_window("VDeviceX11::write_buffer 1");
//	if( device->out_config->driver == PLAYBACK_X11_GL &&
//	    output_frame->get_color_model() != BC_RGB888 ) {
// this is handled by overlay call in virtualvnode, using flatten alpha
// invoked when is_nested = -1 is passed to vdevicex11->overlay(...)
//	}

// printf("VDeviceX11::write_buffer %d %d bitmap_type=%d\n",
// __LINE__,
// output->get_canvas()->get_video_on(),
// bitmap_type);

// 	int use_bitmap_extents = 0;
// 	canvas_w = -1;  canvas_h = -1;
// // Canvas may be a different size than the temporary bitmap for pure software
// 	if( bitmap_type == BITMAP_TEMP && !bitmap->hardware_scaling() ) {
// 		canvas_w = bitmap->get_w();
// 		canvas_h = bitmap->get_h();
// 	}
//
// 	output->get_transfers(edl,
// 		output_x1, output_y1, output_x2, output_y2,
// 		canvas_x1, canvas_y1, canvas_x2, canvas_y2,
// 		canvas_w, canvas_h);

// Convert colormodel
	if( bitmap_type == BITMAP_TEMP ) {
// printf("VDeviceX11::write_buffer 1 %d %d, %d %d %d %d -> %d %d %d %d\n",
//   output->w, output->h, in_x, in_y, in_w, in_h, out_x, out_y, out_w, out_h);
// fflush(stdout);

// printf("VDeviceX11::write_buffer %d output_channels=%p\n", __LINE__, output_channels);
// printf("VDeviceX11::write_buffer %d input color_model=%d output color_model=%d\n",
// __LINE__, output_channels->get_color_model(), bitmap->get_color_model());
		if( bitmap->hardware_scaling() ) {
			BC_CModels::transfer(bitmap->get_row_pointers(), output_channels->get_rows(), 0, 0, 0,
				output_channels->get_y(), output_channels->get_u(), output_channels->get_v(),
				0, 0, output_channels->get_w(), output_channels->get_h(),
				0, 0, bitmap->get_w(), bitmap->get_h(),
				output_channels->get_color_model(), bitmap->get_color_model(),
				-1, output_channels->get_w(), bitmap->get_w());
		}
		else {
			BC_CModels::transfer(bitmap->get_row_pointers(), output_channels->get_rows(), 0, 0, 0,
				output_channels->get_y(), output_channels->get_u(), output_channels->get_v(),
				(int)output_x1, (int)output_y1, (int)(output_x2 - output_x1), (int)(output_y2 - output_y1),
				0, 0, (int)(canvas_x2 - canvas_x1), (int)(canvas_y2 - canvas_y1),
				output_channels->get_color_model(), bitmap->get_color_model(),
				-1, output_channels->get_w(), bitmap->get_w());
		}
	}

//printf("VDeviceX11::write_buffer 4 %p\n", bitmap);
//for( i = 0; i < 1000; i += 4 ) bitmap->get_data()[i] = 128;
//printf("VDeviceX11::write_buffer 2 %d %d %d\n", bitmap_type,
//	bitmap->get_color_model(),
//	output->get_color_model());fflush(stdout);

// printf("VDeviceX11::write_buffer %d %dx%d %f %f %f %f -> %f %f %f %f\n",
// __LINE__, // output->w, output->h,
// output_x1, output_y1, output_x2, output_y2,
// canvas_x1, canvas_y1, canvas_x2, canvas_y2);

// Cause X server to display it
	if( device->out_config->driver == PLAYBACK_X11_GL ) {
// Output is drawn in close_all if no video.
		if( output->get_canvas()->get_video_on() ) {
			canvas_w = -1;  canvas_h = -1;
// Canvas may be a different size than the temporary bitmap for pure software
			if( bitmap_type == BITMAP_TEMP &&
				!bitmap->hardware_scaling() ) {
				canvas_w = bitmap->get_w();
				canvas_h = bitmap->get_h();
			}

			output->get_transfers(edl,
				output_x1, output_y1, output_x2, output_y2,
				canvas_x1, canvas_y1, canvas_x2, canvas_y2,
				canvas_w, canvas_h);

//printf("VDeviceX11::write_buffer %d\n", __LINE__);
// Draw output frame directly.  Not used for compositing.
			output->get_canvas()->unlock_window();
			output->unlock_canvas();
			output->mwindow->playback_3d->write_buffer(output, output_frame,
				output_x1, output_y1, output_x2, output_y2,
				canvas_x1, canvas_y1, canvas_x2, canvas_y2,
				is_cleared);
			is_cleared = 0;
			output->lock_canvas("VDeviceX11::write_buffer 2");
			output->get_canvas()->lock_window("VDeviceX11::write_buffer 2");
		}
	}
	else {
		if( bitmap->hardware_scaling() ) {
			output->get_canvas()->draw_bitmap(bitmap, !device->single_frame,
				(int)canvas_x1, (int)canvas_y1,
				(int)(canvas_x2 - canvas_x1), (int)(canvas_y2 - canvas_y1),
				(int)output_x1, (int)output_y1,
				(int)(output_x2 - output_x1), (int)(output_y2 - output_y1),
				0);
		}
		else {
//printf("VDeviceX11::write_buffer %d x=%d y=%d w=%d h=%d\n",
// __LINE__, (int)canvas_x1, (int)canvas_y1,
// output->get_canvas()->get_w(), output->get_canvas()->get_h());
			output->get_canvas()->draw_bitmap(bitmap, !device->single_frame,
				(int)canvas_x1, (int)canvas_y1,
				(int)(canvas_x2 - canvas_x1), (int)(canvas_y2 - canvas_y1),
				0, 0,
				(int)(canvas_x2 - canvas_x1), (int)(canvas_y2 - canvas_y1),
				0);
//printf("VDeviceX11::write_buffer %d bitmap=%p\n", __LINE__, bitmap);
		}
		if( !output->get_canvas()->get_video_on() )
			output->get_canvas()->flash(0);
	}

	output->get_canvas()->unlock_window();
	output->unlock_canvas();
	return 0;
}


void VDeviceX11::clear_output()
{
	is_cleared = 1;
	output->mwindow->playback_3d->clear_output(output, 0);
	output->mwindow->playback_3d->clear_output(output, output_frame);
}


void VDeviceX11::clear_input(VFrame *frame)
{
	this->output->mwindow->playback_3d->clear_input(this->output, frame);
}

void VDeviceX11::convert_cmodel(VFrame *output, int dst_cmodel)
{
	this->output->mwindow->playback_3d->convert_cmodel(this->output,
		output, 
		dst_cmodel);
}

void VDeviceX11::do_camera(VFrame *output, VFrame *input,
	float in_x1, float in_y1, float in_x2, float in_y2,
	float out_x1, float out_y1, float out_x2, float out_y2)
{
	this->output->mwindow->playback_3d->do_camera(this->output,
		output, input,
		in_x1, in_y1, in_x2, in_y2,
		out_x1, out_y1, out_x2, out_y2);
}


void VDeviceX11::do_fade(VFrame *output_temp, float fade)
{
	this->output->mwindow->playback_3d->do_fade(this->output, output_temp, fade);
}

bool VDeviceX11::can_mask(int64_t start_position_project, MaskAutos *keyframe_set)
{
	Auto *current = 0;
	MaskAuto *keyframe = (MaskAuto*)keyframe_set->
		get_prev_auto(start_position_project, PLAY_FORWARD, current);
	return keyframe->disable_opengl_masking ? 0 : 1;
}

void VDeviceX11::do_mask(VFrame *output_temp, int64_t start_position_project,
		MaskAutos *keyframe_set, MaskAuto *keyframe, MaskAuto *default_auto)
{
	this->output->mwindow->playback_3d->do_mask(output, output_temp,
		start_position_project, keyframe_set, keyframe, default_auto);
}

void VDeviceX11::overlay(VFrame *output_frame, VFrame *input,
		float in_x1, float in_y1, float in_x2, float in_y2,
		float out_x1, float out_y1, float out_x2, float out_y2,
		float alpha, int mode, EDL *edl, int is_nested)
{
	int interpolation_type = edl->session->interpolation_type;
	output->mwindow->playback_3d->overlay(output, input,
		in_x1, in_y1, in_x2, in_y2,
		out_x1, out_y1, out_x2, out_y2, alpha, // 0 - 1
		mode, interpolation_type, output_frame, is_nested);
}

void VDeviceX11::run_plugin(PluginClient *client)
{
	output->mwindow->playback_3d->run_plugin(output, client);
}

void VDeviceX11::copy_frame(VFrame *dst, VFrame *src)
{
	output->mwindow->playback_3d->copy_from(output, dst, src, 1);
}

