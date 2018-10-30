
/*
 * CINELERRA
 * Copyright (C) 2009-2013 Adam Williams <broadcast at earthling dot net>
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

#include "asset.h"
#include "bchash.h"
#include "bcpbuffer.h"
#include "bcsignals.h"
#include "cache.h"
#include "clip.h"
#include "commonrender.h"
#include "edits.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filexml.h"
#include "floatautos.h"
#include "maskauto.h"
#include "maskautos.h"
#include "mwindow.h"
#include "overlayframe.h"
#include "patch.h"
#include "pluginarray.h"
#include "preferences.h"
#include "renderengine.h"
#include "sharedlocation.h"
#include "tracks.h"
#include "transition.h"
#include "transportque.h"
#include "units.h"
#include "vattachmentpoint.h"
#include "vdevicex11.h"
#include "vedit.h"
#include "vframe.h"
#include "videodevice.h"
#include "virtualvconsole.h"
#include "vmodule.h"
#include "vrender.h"
#include "vplugin.h"
#include "vtrack.h"
#include <string.h>
#include "interlacemodes.h"
#include "maskengine.h"
#include "automation.h"

VModule::VModule(RenderEngine *renderengine,
	CommonRender *commonrender,
	PluginArray *plugin_array,
	Track *track)
 : Module(renderengine, commonrender, plugin_array, track)
{
	data_type = TRACK_VIDEO;
	overlay_temp = 0;
	input_temp = 0;
	transition_temp = 0;
	masker = 0;
}

VModule::~VModule()
{
	if(overlay_temp) delete overlay_temp;
	if(input_temp) delete input_temp;
	if(transition_temp) delete transition_temp;
	delete masker;
}


AttachmentPoint* VModule::new_attachment(Plugin *plugin)
{
	return new VAttachmentPoint(renderengine, plugin);
}

int VModule::get_buffer_size()
{
	return 1;
}

CICache* VModule::get_cache()
{
	if(renderengine)
		return renderengine->get_vcache();
	else
		return cache;
}






int VModule::import_frame(VFrame *output, VEdit *current_edit,
	int64_t input_position, double frame_rate, int direction, int use_opengl)
{
	int64_t direction_position;
// Translation of edit
	float in_x, in_y, in_w, in_h;
	float out_x, out_y, out_w, out_h;
	int result = 0;
	const int debug = 0;
	double edl_rate = get_edl()->session->frame_rate;

	int64_t input_position_project = Units::to_int64(input_position *
		edl_rate / frame_rate + 0.001);

	if(!output) printf("VModule::import_frame %d output=%p\n", __LINE__, output);
	//output->dump_params();

	if(debug) printf("VModule::import_frame %d this=%p input_position=%lld direction=%d\n",
		__LINE__, this, (long long)input_position, direction);

// Convert to position corrected for direction
	direction_position = input_position;
	if(direction == PLAY_REVERSE) {
		if( direction_position > 0 ) direction_position--;
		if( input_position_project > 0 ) input_position_project--;
	}
	if(!output) printf("VModule::import_frame %d output=%p\n", __LINE__, output);

	VDeviceX11 *x11_device = 0;
	if(use_opengl)
	{
		if(renderengine && renderengine->video)
		{
			x11_device = (VDeviceX11*)renderengine->video->get_output_base();
			output->set_opengl_state(VFrame::RAM);
			if(!x11_device) use_opengl = 0;
		}
	}

	if(!output) printf("VModule::import_frame %d output=%p x11_device=%p nested_edl=%p\n",
		__LINE__, output, x11_device, nested_edl);

	if(debug) printf("VModule::import_frame %d current_edit=%p\n",
		__LINE__, current_edit);

// Load frame into output

// Create objects for nested EDL
	if(current_edit && current_edit->nested_edl) {
		int command;
		if(debug) printf("VModule::import_frame %d nested_edl=%p current_edit->nested_edl=%p\n",
			__LINE__, nested_edl, current_edit->nested_edl);

// Convert requested direction to command
		if( renderengine->command->command == CURRENT_FRAME ||
		    renderengine->command->command == LAST_FRAME )
		{
			command = renderengine->command->command;
		}
		else
		if(direction == PLAY_REVERSE)
		{
			if(renderengine->command->single_frame())
				command = SINGLE_FRAME_REWIND;
			else
				command = NORMAL_REWIND;
		}
		else
		{
			if(renderengine->command->single_frame())
				command = SINGLE_FRAME_FWD;
			else
				command = NORMAL_FWD;
		}

		if(!nested_edl || nested_edl->id != current_edit->nested_edl->id)
		{
			nested_edl = current_edit->nested_edl;
			if(nested_renderengine)
			{
				delete nested_renderengine;
				nested_renderengine = 0;
			}

			if(!nested_command)
			{
				nested_command = new TransportCommand;
			}


			if(!nested_renderengine)
			{
				nested_command->command = command;
				nested_command->get_edl()->copy_all(nested_edl);
				nested_command->change_type = CHANGE_ALL;
				nested_command->realtime = renderengine->command->realtime;
				nested_renderengine = new RenderEngine(0, get_preferences(), 0, 1);
				nested_renderengine->set_vcache(get_cache());
				nested_renderengine->arm_command(nested_command);
			}
		}
		else
		{

// Update nested command
			nested_renderengine->command->command = command;
			nested_command->realtime = renderengine->command->realtime;
		}

// Update nested video driver for opengl
		nested_renderengine->video = renderengine->video;
	}
	else
	{
		nested_edl = 0;
	}
	if(debug) printf("VModule::import_frame %d\n", __LINE__);

	if( output ) {
		if( use_opengl )
			x11_device->clear_input(output);
		else
			output->clear_frame();
	}
	else
		printf("VModule::import_frame %d output=%p\n", __LINE__, output);

	if(current_edit &&
		(current_edit->asset ||
		(current_edit->nested_edl && nested_renderengine->vrender)))
	{
		File *file = 0;

//printf("VModule::import_frame %d cache=%p\n", __LINE__, get_cache());
		if( current_edit->asset ) {
			get_cache()->age();
			file = get_cache()->check_out(current_edit->asset,
				get_edl());
//			get_cache()->dump();
		}

// File found
		if(file || nested_edl)
		{
// Make all positions based on requested frame rate.
			int64_t edit_startproject = Units::to_int64(current_edit->startproject *
				frame_rate /
				edl_rate);
			int64_t edit_startsource = Units::to_int64(current_edit->startsource *
				frame_rate /
				edl_rate);
// Source position going forward
			uint64_t position = direction_position -
				edit_startproject +
				edit_startsource;
			int64_t nested_position = 0;





// apply speed curve to source position so the timeline agrees with the playback
			if(track->has_speed())
			{
// integrate position from start of edit.
				double speed_position = edit_startsource;
				FloatAutos *speed_autos = (FloatAutos*)track->automation->autos[AUTOMATION_SPEED];
				speed_position += speed_autos->automation_integral(edit_startproject,
						direction_position-edit_startproject, PLAY_FORWARD);
//printf("VModule::import_frame %d %lld %lld\n", __LINE__, position, (int64_t)speed_position);
				position = (int64_t)speed_position;
			}





			int asset_w;
			int asset_h;
			if(debug) printf("VModule::import_frame %d\n", __LINE__);


// maybe apply speed curve here, so timeline reflects actual playback



// if we hit the end of stream, freeze at last frame
			uint64_t max_position = 0;
			if(file)
			{
				max_position = Units::to_int64((double)file->get_video_length() *
					frame_rate /
					current_edit->asset->frame_rate - 1);
			}
			else
			{
				max_position = Units::to_int64(nested_edl->tracks->total_length() *
					frame_rate - 1);
			}


			if(position > max_position) position = max_position;
			else
			if(position < 0) position = 0;

			int use_cache = renderengine &&
				renderengine->command->single_frame();
//			int use_asynchronous = !use_cache &&
//				renderengine &&
// Try to make rendering go faster.
// But converts some formats to YUV420, which may degrade input format.
////				renderengine->command->realtime &&
//				renderengine->get_edl()->session->video_asynchronous;

			if(file)
			{
				if(debug) printf("VModule::import_frame %d\n", __LINE__);
//				if(use_asynchronous)
//					file->start_video_decode_thread();
//				else
					file->stop_video_thread();

				int64_t normalized_position = Units::to_int64(position *
					current_edit->asset->frame_rate /
					frame_rate);
// printf("VModule::import_frame %d %lld %lld\n",
// __LINE__,
// position,
// normalized_position);
				file->set_layer(current_edit->channel);
				file->set_video_position(normalized_position,
					0);
				asset_w = current_edit->asset->width;
				asset_h = current_edit->asset->height;
//printf("VModule::import_frame %d normalized_position=%lld\n", __LINE__, normalized_position);
			}
			else
			{
				if(debug) printf("VModule::import_frame %d\n", __LINE__);
				asset_w = nested_edl->session->output_w;
				asset_h = nested_edl->session->output_h;
// Get source position in nested frame rate in direction of playback.
				nested_position = Units::to_int64(position *
					nested_edl->session->frame_rate /
					frame_rate);
				if(direction == PLAY_REVERSE)
					nested_position++;
			}


// Auto scale if required
			if(output->get_params()->get("AUTOSCALE", 0))
			{
				float autoscale_w = output->get_params()->get("AUTOSCALE_W", 1024);
				float autoscale_h = output->get_params()->get("AUTOSCALE_H", 1024);
				float x_scale = autoscale_w / asset_w;
				float y_scale = autoscale_h / asset_h;

// Overriding camera
				in_x = 0;
				in_y = 0;
				in_w = asset_w;
				in_h = asset_h;

				if(x_scale < y_scale)
				{
					out_w = in_w * x_scale;
					out_h = in_h * x_scale;
				}
				else
				{
					out_w = in_w * y_scale;
					out_h = in_h * y_scale;
				}

				out_x = track->track_w / 2 - out_w / 2;
				out_y = track->track_h / 2 - out_h / 2;
			}
			else
// Apply camera
			{
				((VTrack*)track)->calculate_input_transfer(asset_w,
					asset_h,
					input_position_project,
					direction,
					in_x,
					in_y,
					in_w,
					in_h,
					out_x,
					out_y,
					out_w,
					out_h);
			}

// printf("VModule::import_frame %d %f %d %f %d\n",
// __LINE__,
// in_w,
// asset_w,
// in_h,
// asset_h);
//
//			printf("VModule::import_frame 1 [ilace] Project: mode (%d) Asset: autofixoption (%d), mode (%d), method (%d)\n",
//			get_edl()->session->interlace_mode,
//			current_edit->asset->interlace_autofixoption,
//			current_edit->asset->interlace_mode,
//			current_edit->asset->interlace_fixmethod);

			// Determine the interlacing method to use.
			int interlace_fixmethod = !current_edit->asset ? ILACE_FIXMETHOD_NONE :
				 ilaceautofixmethod2(get_edl()->session->interlace_mode,
					current_edit->asset->interlace_autofixoption,
					current_edit->asset->interlace_mode,
					current_edit->asset->interlace_fixmethod);
//
//			char string[BCTEXTLEN];
//			ilacefixmethod_to_text(string,interlace_fixmethod);
//			printf("VModule::import_frame 1 [ilace] Compensating by using: '%s'\n",string);

			// Compensate for the said interlacing...
			switch (interlace_fixmethod) {
				case ILACE_FIXMETHOD_NONE:

				break;
				case ILACE_FIXMETHOD_UPONE:
					out_y--;
				break;
				case ILACE_FIXMETHOD_DOWNONE:
					out_y++;
				break;
				default:
					printf("vmodule::importframe WARNING - unknown fix method for interlacing, no compensation in effect\n");
			}

// file -> temp -> output
			if( !EQUIV(in_x, 0) ||
				!EQUIV(in_y, 0) ||
				!EQUIV(in_w, track->track_w) ||
				!EQUIV(in_h, track->track_h) ||
				!EQUIV(out_x, 0) ||
				!EQUIV(out_y, 0) ||
				!EQUIV(out_w, track->track_w) ||
				!EQUIV(out_h, track->track_h) ||
				!EQUIV(in_w, asset_w) ||
				!EQUIV(in_h, asset_h))
			{
//printf("VModule::import_frame %d file -> temp -> output\n", __LINE__);




// Get temporary input buffer
				VFrame **input = 0;
// Realtime playback
				if(commonrender)
				{
					VRender *vrender = (VRender*)commonrender;
//printf("VModule::import_frame %d vrender->input_temp=%p\n", __LINE__, vrender->input_temp);
					input = &vrender->input_temp;
				}
				else
// Menu effect
				{
					input = &input_temp;
				}


				if((*input) &&
					((*input)->get_w() != asset_w ||
					(*input)->get_h() != asset_h))
				{
					delete (*input);
					(*input) = 0;
				}





				if(!(*input))
				{
					(*input) =
						new VFrame(asset_w, asset_h,
							get_edl()->session->color_model);
				}



				(*input)->copy_stacks(output);

// file -> temp
// Cache for single frame only
				if(file)
				{
					if(debug) printf("VModule::import_frame %d this=%p file=%s\n",
						__LINE__,
						this,
						current_edit->asset->path);
					if(use_cache) file->set_cache_frames(1);
					result = file->read_frame((*input));
					if(use_cache) file->set_cache_frames(0);
					(*input)->set_opengl_state(VFrame::RAM);
				}
				else
				if(nested_edl)
				{
// If the colormodels differ, change input to nested colormodel
					int nested_cmodel = nested_renderengine->get_edl()->session->color_model;
					int current_cmodel = output->get_color_model();
					int output_w = output->get_w();
					int output_h = output->get_h();
					VFrame *input2 = (*input);

					if(nested_cmodel != current_cmodel)
					{
// If opengl, input -> input -> output
						if(use_opengl)
						{
						}
						else
						{
// If software, input2 -> input -> output
// Use output as a temporary.
							input2 = output;
						}

						if(debug) printf("VModule::import_frame %d this=%p nested_cmodel=%d\n",
							__LINE__,
							this,
							nested_cmodel);
						input2->dump();
						input2->reallocate(0,
							-1,
							0,
							0,
							0,
							(*input)->get_w(),
							(*input)->get_h(),
							nested_cmodel,
							-1);
						input2->dump();
					}


					if(debug) printf("VModule::import_frame %d this=%p nested_edl=%s input2=%p\n",
						__LINE__,
						this,
						nested_edl->path,
						input2);

					result = nested_renderengine->vrender->process_buffer(
						input2,
						nested_position,
						use_opengl);

					if(debug) printf("VModule::import_frame %d this=%p nested_edl=%s\n",
						__LINE__,
						this,
						nested_edl->path);

					if(nested_cmodel != current_cmodel)
					{
						if(debug) printf("VModule::import_frame %d\n", __LINE__);
						if(use_opengl)
						{
// Change colormodel in hardware.
							if(debug) printf("VModule::import_frame %d\n", __LINE__);
							x11_device->convert_cmodel(input2,
								current_cmodel);

// The converted color model is now in hardware, so return the input2 buffer
// to the expected color model.
							input2->reallocate(0,
								-1,
								0,
								0,
								0,
								(*input)->get_w(),
								(*input)->get_h(),
								current_cmodel,
								-1);
						}
						else
						{
// Transfer from input2 to input
if(debug) printf("VModule::import_frame %d nested_cmodel=%d current_cmodel=%d input2=%p input=%p output=%p\n",
__LINE__,
nested_cmodel,
current_cmodel,
input2,
(*input),
output);
							BC_CModels::transfer((*input)->get_rows(),
								input2->get_rows(),
								0,
								0,
								0,
								0,
								0,
								0,
								0,
								0,
							    input2->get_w(),
							    input2->get_h(),
								0,
								0,
								(*input)->get_w(),
								(*input)->get_h(),
								nested_cmodel,
								current_cmodel,
								0,
								input2->get_w(),
								(*input)->get_w());
//printf("VModule::import_frame %d\n", __LINE__);

// input2 was the output buffer, so it must be restored
						input2->reallocate(0,
							-1,
							0,
							0,
							0,
							output_w,
							output_h,
							current_cmodel,
							-1);
//printf("VModule::import_frame %d\n", __LINE__);
						}
					}

				}

// Find an overlayer object to perform the camera transformation
				OverlayFrame *overlayer = 0;

// OpenGL playback uses hardware
				if(use_opengl)
				{
//printf("VModule::import_frame %d\n", __LINE__);
				}
				else
// Realtime playback
				if(commonrender)
				{
					VRender *vrender = (VRender*)commonrender;
					overlayer = vrender->overlayer;
				}
				else
// Menu effect
				{
					if(!plugin_array)
						printf("VModule::import_frame neither plugin_array nor commonrender is defined.\n");
					if(!overlay_temp)
					{
						overlay_temp = new OverlayFrame(plugin_array->mwindow->preferences->processors);
					}

					overlayer = overlay_temp;
				}
// printf("VModule::import_frame 1 %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n",
// 	in_x,
// 	in_y,
// 	in_w,
// 	in_h,
// 	out_x,
// 	out_y,
// 	out_w,
// 	out_h);

// temp -> output
// for(int j = 0; j < output->get_w() * 3 * 5; j++)
// 	output->get_rows()[0][j] = 255;

				if(use_opengl)
				{
					x11_device->do_camera(output,
						(*input),
						in_x,
						in_y,
						in_x + in_w,
						in_y + in_h,
						out_x,
						out_y,
						out_x + out_w,
						out_y + out_h);
if(debug) printf("VModule::import_frame %d %d %d\n",
__LINE__,
output->get_opengl_state(),
(*input)->get_opengl_state());
				}
				else
				{



					output->clear_frame();


// get_cache()->check_in(current_edit->asset);
// return;

// TRANSFER_REPLACE is the fastest transfer mode but it has the disadvantage
// of producing green borders in floating point translation of YUV
					int mode = TRANSFER_REPLACE;
					if(get_edl()->session->interpolation_type != NEAREST_NEIGHBOR &&
						BC_CModels::is_yuv(output->get_color_model()))
						mode = TRANSFER_NORMAL;

					if(debug) printf("VModule::import_frame %d temp -> output\n", __LINE__);
					overlayer->overlay(output,
						(*input),
						in_x,
						in_y,
						in_x + in_w,
						in_y + in_h,
						out_x,
						out_y,
						out_x + out_w,
						out_y + out_h,
						1,
						mode,
						get_edl()->session->interpolation_type);
				}
				result = 1;
				
				output->copy_stacks((*input));
				
				
//printf("VModule::import_frame %d\n", __LINE__); 
//(*input)->dump_params();
//output->dump_params();
			}
			else
// file -> output
			{
				if(debug) printf("VModule::import_frame %d file -> output nested_edl=%p file=%p\n",
					__LINE__,
					nested_edl,
					file);
				if(nested_edl)
				{
					VFrame **input = &output;
// If colormodels differ, reallocate output in nested colormodel.
					int nested_cmodel = nested_renderengine->get_edl()->session->color_model;
					int current_cmodel = output->get_color_model();

if(debug) printf("VModule::import_frame %d nested_cmodel=%d current_cmodel=%d\n",
__LINE__,
nested_cmodel,
current_cmodel);

					if(nested_cmodel != current_cmodel)
					{
						if(use_opengl)
						{
						}
						else
						{
							if(commonrender)
							{
								input = &((VRender*)commonrender)->input_temp;
							}
							else
							{
								input = &input_temp;
							}

							if(!(*input)) (*input) = new VFrame;
						}

						(*input)->reallocate(0,
							-1,
							0,
							0,
							0,
							output->get_w(),
							output->get_h(),
							nested_cmodel,
							-1);
if(debug) printf("VModule::import_frame %d\n",
__LINE__);
//(*input)->dump();
//(*input)->clear_frame();
					}

if(debug) printf("VModule::import_frame %d %p %p\n",
__LINE__,
(*input)->get_rows(),
(*input));
					result = nested_renderengine->vrender->process_buffer(
						(*input),
						nested_position,
						use_opengl);
if(debug) printf("VModule::import_frame %d\n",
__LINE__);

// If colormodels differ, change colormodels in opengl if possible.
// Swap output for temp if not possible.
					if(nested_cmodel != current_cmodel)
					{
						if(use_opengl)
						{
							x11_device->convert_cmodel(output,
								current_cmodel);

// The color model was changed in place, so return output buffer
							output->reallocate(0,
								-1,
								0,
								0,
								0,
								output->get_w(),
								output->get_h(),
								current_cmodel,
								-1);
						}
						else
						{
// Transfer from temporary to output
if(debug) printf("VModule::import_frame %d %d %d %d %d %d %d\n",
__LINE__,
(*input)->get_w(),
(*input)->get_h(),
output->get_w(),
output->get_h(),
nested_cmodel,
current_cmodel);
							BC_CModels::transfer(output->get_rows(),
								(*input)->get_rows(),
								0,
								0,
								0,
								0,
								0,
								0,
								0,
								0,
								(*input)->get_w(),
								(*input)->get_h(),
								0,
								0,
								output->get_w(),
								output->get_h(),
								nested_cmodel,
								current_cmodel,
								0,
								(*input)->get_w(),
								output->get_w());
						}

					}

				}
				else
				if(file)
				{
// Cache single frames
//memset(output->get_rows()[0], 0xff, 1024);
					if(use_cache) file->set_cache_frames(1);
					result = file->read_frame(output);
					if(use_cache) file->set_cache_frames(0);
					output->set_opengl_state(VFrame::RAM);
				}
			}

			if(file)
			{
				get_cache()->check_in(current_edit->asset);
				file = 0;
			}
		}
		else
// Source not found
		{
			result = 1;
		}

// 		printf("VModule::import_frame %d cache=%p\n", 
// 			__LINE__,
// 			get_cache());

	}
	else
// Source is silence
	{
		if(debug) printf("VModule::import_frame %d\n", __LINE__);
		if(use_opengl)
		{
			x11_device->clear_input(output);
		}
		else
		{
			output->clear_frame();
		}
	}

	if(debug) printf("VModule::import_frame %d done\n", __LINE__);

	return result;
}



int VModule::render(VFrame *output,
	int64_t start_position,
	int direction,
	double frame_rate,
	int use_nudge,
	int debug_render,
	int use_opengl)
{
	int result = 0;
	double edl_rate = get_edl()->session->frame_rate;

//printf("VModule::render %d %ld\n", __LINE__, start_position);

	if(use_nudge) start_position += Units::to_int64(track->nudge *
		frame_rate / edl_rate);

	int64_t start_position_project = Units::to_int64(start_position *
		edl_rate / frame_rate + 0.5);

	update_transition(start_position_project,
		direction);

	VEdit* current_edit = (VEdit*)track->edits->editof(start_position_project,
		direction,
		0);
	VEdit* previous_edit = 0;
//printf("VModule::render %d %p %ld %d\n", __LINE__, current_edit, start_position_project, direction);

	if(debug_render)
		printf("    VModule::render %d %d %jd %s transition=%p opengl=%d current_edit=%p output=%p\n",
			__LINE__,
			use_nudge,
			start_position_project,
			track->title,
			transition,
			use_opengl,
			current_edit,
			output);

	if(!current_edit)
	{
		output->clear_frame();
		// We do not apply mask here, since alpha is 0, and neither substracting nor multypling changes it
		// Another mask mode - "addition" should be added to be able to create mask from empty frames
		// in this case we would call masking here too...
		return 0;
	}




// Process transition
	if(transition && transition->on)
	{

// Get temporary buffer
		VFrame **transition_input = 0;
		if(commonrender)
		{
			VRender *vrender = (VRender*)commonrender;
			transition_input = &vrender->transition_temp;
		}
		else
		{
			transition_input = &transition_temp;
		}

		if((*transition_input) &&
			((*transition_input)->get_w() != track->track_w ||
			(*transition_input)->get_h() != track->track_h))
		{
			delete (*transition_input);
			(*transition_input) = 0;
		}

// Load incoming frame
		if(!(*transition_input))
		{
			(*transition_input) =
				new VFrame(track->track_w, track->track_h,
					get_edl()->session->color_model);
		}

		(*transition_input)->copy_stacks(output);

//printf("VModule::render %d\n", __LINE__);
		result = import_frame((*transition_input),
			current_edit,
			start_position,
			frame_rate,
			direction,
			use_opengl);


// Load transition buffer
		previous_edit = (VEdit*)current_edit->previous;

		result |= import_frame(output,
			previous_edit,
			start_position,
			frame_rate,
			direction,
			use_opengl);
//printf("VModule::render %d\n", __LINE__);

// printf("VModule::render %d %p %p %p %p\n",
// __LINE__,
// (*transition_input),
// (*transition_input)->get_pbuffer(),
// output,
// output->get_pbuffer());


// Execute plugin with transition_input and output here
		if(renderengine)
			transition_server->set_use_opengl(use_opengl, renderengine->video);
		transition_server->process_transition((*transition_input),
			output,
			(direction == PLAY_FORWARD) ?
				(start_position_project - current_edit->startproject) :
				(start_position_project - current_edit->startproject - 1),
			transition->length);
	}
	else
	{
// Load output buffer
		result = import_frame(output,
			current_edit,
			start_position,
			frame_rate,
			direction,
			use_opengl);
	}

	Auto *current = 0;
	MaskAutos *keyframe_set =
		(MaskAutos*)track->automation->autos[AUTOMATION_MASK];
	int64_t mask_position = !renderengine ? start_position :
		renderengine->vrender->current_position;
       	MaskAuto *keyframe =
		(MaskAuto*)keyframe_set->get_prev_auto(mask_position, direction, current);

	if( keyframe->apply_before_plugins ) {
		VDeviceX11 *x11_device = 0;
		if(use_opengl && renderengine && renderengine->video) {
			x11_device = (VDeviceX11*)renderengine->video->get_output_base();
			if( !x11_device->can_mask(mask_position, keyframe_set) )
				use_opengl = 0;
		}
		if( use_opengl && x11_device ) {
			x11_device->do_mask(output, mask_position, keyframe_set,
					keyframe, keyframe);
		}
		else {
			if( !masker ) {
				int cpus = renderengine ?
					renderengine->preferences->processors :
					plugin_array->mwindow->preferences->processors;
				masker = new MaskEngine(cpus);
			}
			masker->do_mask(output, mask_position, keyframe_set, keyframe, keyframe);
		}
	}

	return result;
}






void VModule::create_objects()
{
	Module::create_objects();
}






