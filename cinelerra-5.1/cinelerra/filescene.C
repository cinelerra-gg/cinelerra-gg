/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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
#include "bcsignals.h"
#include "clip.h"
#include "file.h"
#include "filescene.h"
#include "filesystem.h"
#include "libmjpeg.h"
#include "scenegraph.h"


#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


extern "C"
{
#include <uuid.h>
}


// Paths relative to the exe path
#ifdef HAVE_FESTIVAL_BUILTIN
#define FESTIVAL_PATH "/festival"
#define FESTIVAL_LIB_PATH "/lib/"
#endif
#define ASSET_PATH "/models/"
#define FREAD_SIZE 0x10000
#define WAVHEADER 44
#define FESTIVAL_SAMPLERATE 16000
#define PAUSE_SAMPLES FESTIVAL_SAMPLERATE
// Amount to truncate if ...
#define ADVANCE_SAMPLES FESTIVAL_SAMPLERATE

// Maximum characters in a line of dialog is limited by festival
#define MAX_CHARS 65536

#define STRIP_LINE(string) \
/* Strip linefeeds */ \
	while(len > 0 && (string[len - 1] == '\n' || \
		string[len - 1] == ' ')) \
	{ \
		string[len - 1] = 0; \
		len--; \
	} \
 \
/* Strip comments */ \
	for(i = 0; i < len; i++) \
	{ \
		if(string[i] == '#') \
		{ \
			string[i] = 0; \
			i = len; \
		} \
	}




#define STRING_PARAMETER(title, need_char, dst) \
if(!strncmp(command, title, strlen(title))) \
{ \
/* advance to argument */ \
	i += strlen(title); \
	while(string[i] != 0 && string[i] == ' ') \
		i++; \
 \
	if(current_char || !need_char) \
	{ \
/* printf("STRING_PARAMETER %s %s %p\n", title, string + i, dst); */ \
		strcpy(dst, string + i); \
	} \
	else \
	{ \
		printf("FileScene::read_script %d Line %d: %s but no current character\n",  \
			__LINE__, \
			current_line, \
			title); \
	} \
 \
	i = len; \
}

static int read_parameter(char *string,
	int *i,
	const char *title,
	char *dst_string,
	float *dst_float0,
	float *dst_float1,
	float *dst_float2)
{
	char *command = string + *i;
	char *arg_text[3];

	if(!strncmp(command, title, strlen(title)))
	{
		*i += strlen(title);

		for(int j = 0; j < 4; j++)
		{
/* skip to start of argument */
			while(string[*i] != 0 && string[*i] == ' ')
				(*i)++;

			if(string[*i] != 0)
			{
				arg_text[j] = string + *i;
				while(string[*i] != 0 && string[*i] != ' ')
					(*i)++;
			}
			else
			{
				arg_text[j] = 0;
			}
 		}

// printf("read_parameter %d %s %s %s %s\n",
// __LINE__,
// title,
// arg_text[0],
// arg_text[1],
// arg_text[2]);

		if(arg_text[0])
		{
			if(dst_string)
			{
				char *ptr1 = dst_string;
				char *ptr2 = arg_text[0];
				while(*ptr2 != 0 && *ptr2 != ' ')
					*ptr1++ = *ptr2++;
				*ptr1 = 0;
			}
			else
			if(dst_float0)
			{
				*dst_float0 = atof(arg_text[0]);
			}
		}

		if(arg_text[1])
		{
			if(dst_float1)
			{
				*dst_float1 = atof(arg_text[1]);
			}
		}

		if(arg_text[2])
		{
			if(dst_float2)
			{
				*dst_float2 = atof(arg_text[2]);
			}
		}

		return 1;
	}

	return 0;
}













FileScene::FileScene(Asset *asset, File *file)
 : FileBase(asset, file)
{
	reset_parameters();
	strncpy(exec_path, File::get_cinlib_path(), sizeof(exec_path));
}


FileScene::~FileScene()
{
	close_file();
}



int FileScene::open_file(int rd, int wr)
{
// Load the script to get character count
	read_script();



// Set asset format
	asset->format = FILE_SCENE;
	asset->video_data = 1;
// Should be set in the scene file
	asset->layers = 1;
	asset->width = 1280;
	asset->height = 720;
// Dictated by character animations
	asset->frame_rate = (double)30000 / 1001;
// Some arbitrary default length.
// Hard to calculate without rendering it.
	asset->video_length = -1;



	asset->audio_data = 1;
// The speech synthesizer outputs different samplerates depending on the speaker.
// The heroine voices are all 16khz
	asset->sample_rate = FESTIVAL_SAMPLERATE;
// Mono voice placement for now
// Maybe a different track for each voice in the future or 3D positioning
	asset->channels = script->total_characters();
	asset->audio_length = -1;


	return 0;
}

int FileScene::close_file()
{
	delete script;
	delete [] audio_temp;
//	delete overlayer;
//	delete affine;
	FileBase::close_file();
	return 0;
}


int FileScene::check_sig(Asset *asset, char *test)
{
	if(!strncmp(test, "TEXT2MOVIE", 10)) return 1;

	return 0;
}



int FileScene::set_video_position(int64_t x)
{
	return 0;
}


int FileScene::set_audio_position(int64_t x)
{
	return 0;
}


int FileScene::read_frame(VFrame *frame)
{
// Everything is timed based on speech, so render the audio for this frame.
	const int debug = 0;


	frame->clear_frame();
	int64_t audio_position1 = (int64_t)(file->current_frame *
		asset->sample_rate /
		asset->frame_rate);
	int64_t audio_position2 = (int64_t)(audio_position1 +
		asset->sample_rate /
		asset->frame_rate);

	if(debug) printf("FileScene::read_frame %d frame=%jd"
		" frame_rate=%f sample_rate=%d audio_position1=%jd"
		" audio_position2=%jd\n", __LINE__,
		file->current_frame, asset->frame_rate,
		asset->sample_rate, audio_position1,
		audio_position2);

	render_chunks(audio_position1,
		audio_position2 - audio_position1,
		1);
	if(!script) return 1;
	if(debug) printf("FileScene::read_frame %d\n", __LINE__);

//	script->dump();


// Determine lip position from amplitude
	double accum = 0;
	for(int i = 0; i < audio_position2 - audio_position1; i++)
	{
		double sample_float = fabs((double)audio_temp[i] / 32768);
		if(sample_float > accum) accum = sample_float;
	}
	if(debug) printf("FileScene::read_frame %d accum=%f\n", __LINE__, accum);

// Reset cameras
	for(int i = 0; i < script->total_characters(); i++)
	{
		SceneChar *character = script->get_character(i);
		character->current_camera = SceneChar::CAMERA_WIDE;
		character->is_speeking = 0;
		character->max = 0;
	}

// Now determine most recent character which is speaking from the sample times.
	int64_t current_sample = 0;
	SceneChar *speeking_character = 0;
// Sample relative to start of chunk
	int64_t chunk_sample = 0;
	for(int i = 0;
		i < script->total_chunks() && current_sample < audio_position2;
		i++)
	{
		SceneChunk *chunk = script->get_chunk(i);
		int samples = chunk->audio_size / 2;


		if(audio_position1 >= current_sample &&
			audio_position1 < current_sample + samples)
		{
			speeking_character = chunk->character;
			speeking_character->max = chunk->max;
			chunk->character->is_speeking = 1;
			chunk_sample = audio_position1 - current_sample;
//			break;
		}
		else
		if(!chunk->character->is_speeking)
		{
			chunk->character->increment_camera();
		}

		current_sample += chunk->advance_samples;
	}



	if(debug) printf("FileScene::read_frame %d\n", __LINE__);

// Render the scene.
// Store component placement in a scene graph.
	SceneGraph scene;
	//SceneNode *speeking_node = 0;

// Scale for the entire scene
	SceneCamera *camera = new SceneCamera;
	scene.append_camera(camera);
	camera->at_x = 0;
	camera->at_y = 0;

// Render background
	if(script->background[0])
	{
		script->render_background(&scene);
	}

	if(debug) printf("FileScene::read_frame %d\n", __LINE__);

// Render characters
	for(int i = 0; i < script->total_characters(); i++)
	{
		SceneChar *character = script->get_character(i);

		character->read_model();

// Nodes for the character
		SceneNode *character_node = new SceneNode(character->name);
		scene.append(character_node);
		character_node->sx = character_node->sy = character->scale;

		SceneNode *head_node = new SceneNode("head");

		SceneNode *body_node = new SceneNode("body");

// Render in the order listed in the file
		if(debug) printf("FileScene::read_frame %d\n", __LINE__);
		for(int j = 0; j < 2; j++)
		{
			if(debug) printf("FileScene::read_frame %d j=%d head_order=%d body_order=%d\n",
				__LINE__,
				j,
				character->head_order,
				character->body_order);
			if(j == character->head_order) character_node->append(head_node);
			else
			if(j == character->body_order) character_node->append(body_node);
		}

		SceneNode *eye_node = 0;
		if(character->eyes.size())
		{
			eye_node = new SceneNode("eyes");
			head_node->append(eye_node);
		}

		SceneNode *mouth_node = new SceneNode("mouth");
		head_node->append(mouth_node);

// Logical character placement
		switch(script->total_characters())
		{
			case 1:
				if(speeking_character == character &&
					speeking_character->current_camera == SceneChar::CAMERA_CU)
				{
					camera->at_x = asset->width / 4;
				}

				character_node->x = (int)(asset->width / 2 -
					character->w * character->scale / 2);
				character_node->y = (int)(asset->height -
					character->h * character->scale);
				break;

			case 2:
				if(i == 0)
				{
					character_node->x = 0;
					character_node->y = (int)(asset->height -
						character->h * character->scale);

					if(speeking_character == character &&
						speeking_character->current_camera == SceneChar::CAMERA_CU)
					{
						camera->at_y = character_node->y;
						camera->at_x = character_node->x +
							(character->head->x + character->head->image->get_w() / 2) * character->scale -
							asset->width / 4;
						CLAMP(camera->at_x, 0, asset->width / 2);
						CLAMP(camera->at_y, 0, asset->height);
//printf("FileScene::read_frame %d camera->at_x=%f camera->at_y=%f\n", __LINE__, camera->at_x, camera->at_y);
					}

					if(character->faces_left)
					{
						character_node->flip = 1;
						character_node->x = asset->width -
							character->w * character->scale;
					}
				}
				else
				{

					character_node->x = (int)(asset->width -
						character->w * character->scale);
					character_node->y = (int)(asset->height -
						character->h * character->scale);

					if(speeking_character == character &&
						speeking_character->current_camera == SceneChar::CAMERA_CU)
					{
						camera->at_x = character_node->x +
							character->head->x * character->scale -
							asset->width / 4;
						CLAMP(camera->at_x, 0, asset->width / 2);
					}

					if(!character->faces_left)
					{
						character_node->flip = 1;
						character_node->x = 0;
					}
				}
				break;

			case 3:
				if(i == 0)
				{
					character_node->x = 0;
					character_node->y = (int)(asset->height -
						character->h * character->scale);
					if(character->faces_left)
					{
						character_node->flip = 1;
					}
				}
				else
				if(i == 1)
				{
					if(speeking_character == character &&
						speeking_character->current_camera == SceneChar::CAMERA_CU)
					{
						camera->at_x = asset->width / 4;
					}
					character_node->x = (int)(asset->width / 2 -
						character->w * character->scale / 2);
					character_node->y = (int)(asset->height -
						character->h * character->scale);
					if(character->faces_left)
					{
						character_node->flip = 1;
					}
				}
				else
				{
					if(speeking_character == character &&
						speeking_character->current_camera == SceneChar::CAMERA_CU)
					{
						camera->at_x = asset->width / 2;
					}
					character_node->x = (int)(asset->width -
						character->w * character->scale);
					character_node->y = (int)(asset->height -
						character->h * character->scale);
					if(!character->faces_left)
					{
						character_node->flip = 1;
						character_node->x = 0;
					}
				}
				break;
		}

// Clamp the head
		if(character_node->y < 0) character_node->y = 0;

// Add remaining parts
		body_node->copy_ref(character->body);
		head_node->copy_ref(character->head);

// Speeker head rotation
		if(speeking_character == character)
		{
			//speeking_node = character_node;

			int head_time = (chunk_sample / asset->sample_rate / 2) % 2;

			if(head_time > 0)
			{
				double temp;
				double anim_position = modf((double)chunk_sample / asset->sample_rate / 2, &temp);
				double anim_length = 0.1;
// printf("FileScene::read_frame %d %d %f\n",
// __LINE__,
// head_time,
// anim_position);

				if(anim_position < anim_length)
					head_node->ry = -5 * anim_position / anim_length;
				else
				if(anim_position > 1.0 - anim_length)
					head_node->ry = -5 * (1.0 - anim_position) / anim_length;
				else
					head_node->ry = -5;
			}
		}

//head_node->ry = -5;

// Eyes
		double intpart;
		if(character->eyes.size())
		{
			if(modf((file->current_frame / asset->frame_rate + script->get_char_number(character)) / 5, &intpart) <=
				0.1 / 5 &&
				file->current_frame / asset->frame_rate > 1)
			{
				eye_node->copy_ref(character->eyes.get(0));
			}
			else
				eye_node->copy_ref(character->eyes.get(1));
		}

// Compute the mouth image
		int fraction = 0;
		if(character->is_speeking && character->max > 0)
		{
			fraction = (int)(accum * character->mouths.size() / character->max);
			if(fraction >= character->mouths.size())
				fraction = character->mouths.size() - 1;
		}

		mouth_node->copy_ref(character->mouths.get(fraction));


// camera->scale = 2;
// camera->at_x = asset->width / 2;
// camera->at_y = 0;

// Compute camera
		if(speeking_character == character &&
			speeking_character->current_camera == SceneChar::CAMERA_CU)
		{
// If closeup, increase scene scale
			camera->scale = 2;
		}
	}




	if(debug) printf("FileScene::read_frame %d\n", __LINE__);



// Render scene graph
	scene.render(frame, file->cpus);



	if(debug) printf("FileScene::read_frame %d\n", __LINE__);


	return 0;
}


int FileScene::read_samples(double *buffer, int64_t len)
{
// Speech rendering
// Everything is timed based on speech, so we have to do this for video, too.
	render_chunks(file->current_sample, len, 0);

//	script->dump();


// Convert temp to output
	for(int i = 0; i < len; i++)
	{
		buffer[i] = (double)audio_temp[i] / 32768;
	}

	return 0;
}


int64_t FileScene::get_memory_usage()
{
//PRINT_TRACE
	int total = 0x100000;
	if(script)
	{
		total += script->get_memory_usage();

	}
//PRINT_TRACE
	return total;
}



int FileScene::get_best_colormodel(Asset *asset, int driver)
{
	return 0;
}


int FileScene::colormodel_supported(int colormodel)
{
	return BC_RGBA8888;
}


int FileScene::can_copy_from(Asset *asset, int64_t position)
{
	return 0;
}


int FileScene::reset_parameters_derived()
{
	script = 0;
	audio_temp = 0;
	temp_allocated = 0;
//	overlayer = 0;
//	affine = 0;
	return 0;
}



void FileScene::render_chunks(int64_t start_position,
	int64_t len,
	int all_channels)
{
	int64_t end_position = start_position + len;
	const int debug = 0;


	if(debug) printf("FileScene::render_chunks %d start_position=%jd"
		" len=%jd\n", __LINE__, start_position, len);

// Update script
	read_script();

	if(!script) return;

	if(debug) PRINT_TRACE

// Reallocate temp buffer
	if(len > temp_allocated)
	{
		delete [] audio_temp;
		audio_temp = new int16_t[len];
		temp_allocated = len;
	}
	bzero(audio_temp, sizeof(int16_t) * len);

	if(debug) PRINT_TRACE




// Find start_position in script output.
// Must know length of every chunk before end of buffer
	int64_t current_sample = 0;
	for(int i = 0; i < script->total_chunks() &&
		current_sample < end_position; i++)
	{
		SceneChunk *chunk = script->get_chunk(i);
		chunk->used = 0;
		if(debug) printf("FileScene::render_chunks %d\n", __LINE__);

// If extent of audio output hasn't been calculated, render it
		if(!chunk->audio_size)
		{
			if(debug) printf("FileScene::render_chunks %d i=%d\n", __LINE__, i);
			chunk->render();
		}

// Dialog chunk is inside output buffer
		if(current_sample + chunk->audio_size / 2 > start_position &&
			current_sample < end_position)
		{
			if(debug) printf("FileScene::render_chunks %d\n", __LINE__);

// If no audio output exists, render it
			if(!chunk->audio)
			{
				if(debug) printf("FileScene::render_chunks %d rerendering audio\n", __LINE__);
				chunk->render();
			}

			if(debug) printf("FileScene::render_chunks %d: Using \"%s\" samples=%d\n",
				__LINE__,
				chunk->text,
				chunk->audio_size / 2);
			if(debug) printf("FileScene::render_chunks %d: start_position=%jd"
					" current_sample=%jd\n", __LINE__,
					start_position, current_sample);

// Memcpy it.
// TODO: allow characters to talk simultaneously
			int64_t src_offset = start_position - current_sample;
			int64_t dst_offset = 0;
			int64_t src_len = chunk->audio_size / 2 - src_offset;

			if(debug) printf("FileScene::render_chunks %d: src_offset=%jd"
				" dst_offset=%jd src_len=%jd\n", __LINE__,
				src_offset, dst_offset, src_len);

			if(src_offset < 0)
			{
				dst_offset -= src_offset;
				src_len += src_offset;
				src_offset = 0;
			}

			if(dst_offset + src_len > len)
			{
				src_len = len - dst_offset;
			}

			if(debug) printf("FileScene::render_chunks %d: src_offset=%jd"
				" dst_offset=%jd src_len=%jd\n", __LINE__,
				src_offset, dst_offset, src_len);

// Transfer if right channel
			if(all_channels ||
				file->current_channel == script->get_char_number(chunk->character))
			{
				for(int j = 0; j < src_len; j++)
				{
					audio_temp[dst_offset + j] =
						chunk->audio[(src_offset + j) * 2] |
						(chunk->audio[(src_offset + j) * 2 + 1] << 8);
				}
			}

			if(debug) printf("FileScene::render_chunks %d\n", __LINE__);
			chunk->used = 1;
		}

		current_sample += chunk->advance_samples;
	}

// Erase unused dialog chunks
	if(debug) printf("FileScene::render_chunks %d\n", __LINE__);
	for(int i = 0; i < script->total_chunks(); i++)
	{
		SceneChunk *chunk = script->get_chunk(i);
		if(!chunk->used && chunk->audio)
		{
			if(debug) printf("FileScene::render_chunks %d erasing unused audio\n", __LINE__);
			delete [] chunk->audio;
			chunk->audio = 0;
			chunk->audio_allocated = 0;
		}
	}
	if(debug) printf("FileScene::render_chunks %d\n", __LINE__);


}



int FileScene::read_script()
{
	const int debug = 0;
	struct stat ostat;
	if(stat(asset->path, &ostat))
	{
		printf("FileScene::read_script %d: %s\n", __LINE__, strerror(errno));
		return 1;
	}

// Test timestamp
	if(script)
	{
		if(ostat.st_mtime == script->timestamp)
		{
			if(debug) printf("FileScene::read_script %d: script unchanged\n", __LINE__);
			return 0;
		}
	}


// Read new script
	delete script;
	script = 0;
	if(!script) script = new SceneTokens(this, file->cpus);
	script->timestamp = ostat.st_mtime;

	script->read_script(asset->path);

	return 1;
}















SceneChar::SceneChar(SceneTokens *script)
{
	this->script = script;
	name[0] = 0;
	voice[0] = 0;
	body = 0;
	head = 0;
	body_order = 0;
	head_order = 1;
	sprintf(model, "heroine01");
	current_camera = CAMERA_WIDE;
	faces_left = 0;
	scale = 1;
	max = 0;
	is_speeking = 0;
}

SceneChar::~SceneChar()
{
	mouths.remove_all_objects();
	eyes.remove_all_objects();
	delete body;
	delete head;
}


void SceneChar::increment_camera()
{
	current_camera++;
	if(current_camera >= CAMERA_TOTAL)
		current_camera = 0;
}


int SceneChar::read_model()
{
// Read descriptor file
	const int debug = 0;
	int current_line = 0;
	float x, y;
	int i;
	int current_order = 0;
	char path[BCTEXTLEN];

// Already read it
	if(body) return 0;

	script->convert_path(path, model);
	FILE *fd = fopen(path, "r");


// Read assets
	if(fd)
	{
// Read 1 line
		char string[BCTEXTLEN];
		char string2[BCTEXTLEN];

		while(!feof(fd))
		{
			char *result = fgets(string, BCTEXTLEN, fd);

			if(result)
			{
				int len = strlen(string);

				STRIP_LINE(string);

				if(debug) printf("SceneChar::read_model %d: %s\n",
					__LINE__,
					string);

				for(i = 0; i < len; i++)
				{
					if(isalnum(string[i]))
					{
						string2[0] = 0;

						if(read_parameter(string,
							&i,
							"width:",
							0,
							&w,
							0,
							0))
						{
						}
						else
						if(read_parameter(string,
							&i,
							"height:",
							0,
							&h,
							0,
							0))
						{
						}
						else
						if(read_parameter(string,
							&i,
							"body:",
							string2,
							0,
							&x,
							&y))
						{
// Load image
							body = new SceneNode(script->load_image(string2), 1, x, y);
							body_order = current_order++;
						}
						else
						if(read_parameter(string,
							&i,
							"head:",
							string2,
							0,
							&x,
							&y))
						{
// Load image
							head = new SceneNode(script->load_image(string2), 1, x, y);
							head_order = current_order++;
						}
						else
						if(read_parameter(string,
							&i,
							"mouth:",
							string2,
							0,
							&x,
							&y))
						{
// Load image
							SceneNode *mouth;
							mouths.append(mouth = new SceneNode(script->load_image(string2), 1, x, y));
// Make coordinates relative to head
							if(head)
							{
								mouth->x -= head->x;
								mouth->y -= head->y;
							}
						}
						else
						if(read_parameter(string,
							&i,
							"eyes:",
							string2,
							0,
							&x,
							&y))
						{
// Load image
							SceneNode *eye;
							eyes.append(eye = new SceneNode(script->load_image(string2), 1, x, y));
// Make coordinates relative to head
							if(head)
							{
								eye->x -= head->x;
								eye->y -= head->y;
							}
						}
						else
						if(read_parameter(string,
							&i,
							"faces_left",
							0,
							0,
							0,
							0))
						{
							faces_left = 1;
						}
						else
						if(read_parameter(string,
							&i,
							"scale:",
							0,
							&scale,
							0,
							0))
						{
						}

						i = len;
					}
				}
			}

			current_line++;
		}

		fclose(fd);
		if(debug) dump();
		return 0;
	}

	printf("SceneChar::read_model %d: %s %s\n", __LINE__, path, strerror(errno));
	return 1;
}

int SceneChar::get_memory_usage()
{
	int total = 0;
	if(body) total += body->get_memory_usage();
	if(head) total += head->get_memory_usage();
	for(int i = 0; i < mouths.size(); i++)
		total += mouths.get(i)->get_memory_usage();
	return total;
}

void SceneChar::dump()
{
	printf("SceneChar::dump %d: %p name=%s voice=%s model=%s body=%p eyes=%d mouths=%d\n",
		__LINE__,
		this,
		name,
		voice,
		model,
		body,
		eyes.size(),
		mouths.size());
	printf("SceneChar::dump %d: w=%f h=%f\n", __LINE__, w, h);


	for(int i = 0; i < mouths.size(); i++)
	{
		SceneNode *node = mouths.get(i);
		printf("    mouth=%p x=%f y=%f\n", node, node->x, node->y);
	}

	for(int i = 0; i < eyes.size(); i++)
	{
		SceneNode *node = eyes.get(i);
		printf("    eyes=%p x=%f y=%f\n", node, node->x, node->y);
	}
}









// Dialog from a single character
SceneChunk::SceneChunk(SceneTokens *script)
{
	text = 0;
	character = 0;
	audio = 0;
	audio_size = 0;
	audio_allocated = 0;
	advance_samples = 0;
	used = 0;
	max = 0;
	command = NO_COMMAND;
	this->script = script;
}

SceneChunk::~SceneChunk()
{
	delete [] text;
	delete [] audio;
}

void SceneChunk::dump()
{
	printf("SceneChunk::dump %d: character=%s command=%d text=%s\n",
		__LINE__,
		character->name,
		command,
		text);
	printf("SceneChunk::dump %d: audio=%p audio_size=%d advance_samples=%d\n",
		__LINE__,
		audio,
		audio_size,
		advance_samples);
}

int SceneChunk::get_memory_usage()
{
	return audio_allocated;
}


void SceneChunk::append_text(char *new_text)
{
	char string[BCTEXTLEN];

// Replace "
// Convert ' to \'
	char *ptr = string;
	int len = strlen(new_text);
	for(int i = 0; i < len; i++)
	{
// 		if(new_text[i] == '"')
// 			*ptr++ = ' ';
// 		else
// 		if(new_text[i] == '\'')
// 		{
// 			*ptr++ = '\\';
// 			*ptr++ = '\'';
// 			*ptr++ = '\'';
// 		}
// 		else
			*ptr++ = new_text[i];
	}
	*ptr++ = 0;

	int len2 = strlen(string);
	if(text)
	{
		int len1 = strlen(text);
		int len3 = 1;
		int need_space = 0;

//		if(len1 > 0 && isalnum(text[len1 - 1]))
		if(len1 > 0 && text[len1 - 1] != ' ')
		{
			need_space = 1;
			len3++;
		}

		text = (char*)realloc(text, len1 + len2 + len3);

// Append space
		if(need_space)
		{
			text[len1] = ' ';
			text[len1 + 1] = 0;
		}
	}
	else
	{
		text = new char[len2 + 1];
		text[0] = 0;
	}

	strcat(text, string);
}




void SceneChunk::render()
{
	const int debug = 0;
	int len = 0;
	char command_line[BCTEXTLEN];
	char string2[MAX_CHARS];
	char script_path[BCTEXTLEN];

	//int total_args = 0;
	if(text) len = strlen(text);
	char *text_end = text + len;
	char *text_ptr = text;

	audio_size = 0;

	if(!character)
	{
		printf("SceneChunk::render %d: no character defined.\n", __LINE__);
	}

	if(len > MAX_CHARS)
	{
		printf("SceneChunk::render %d: text '%s' exceeds festival's maximum line length of %d chars.\n",
			__LINE__,
			text,
			MAX_CHARS);
	}

// Process command
	switch(command)
	{
		case SceneChunk::PAUSE_COMMAND:
			audio_allocated = PAUSE_SAMPLES * 2;
			audio_size = PAUSE_SAMPLES * 2;
			advance_samples = PAUSE_SAMPLES;
			audio = (unsigned char*)realloc(audio, audio_allocated);
			bzero(audio, audio_size);
			max = 0;
			break;
	}

	while(text && text_ptr < text_end)
	{
// Copy at most MAX_CHARS of data into string2
		char *ptr = string2;
		for(int i = 0; i < MAX_CHARS && text_ptr < text_end; i++)
		{
			*ptr++ = *text_ptr++;
		}
		*ptr++ = 0;


// Rewind to white space if still more text
		if(text_ptr < text_end)
		{
			ptr--;
			text_ptr--;
			while(*text_ptr != ' ' &&
				*text_ptr != '\n' &&
				text_ptr > text)
			{
				text_ptr--;
				ptr--;
			}


// Truncate string2 at white space
			*ptr = 0;
// If no white space, abort.
			if(text_ptr <= text)
			{
				break;
			}

		}

		uuid_t temp_id;
		sprintf(script_path, "/tmp/cinelerra.");
		uuid_generate(temp_id);
		uuid_unparse(temp_id, script_path + strlen(script_path));
		FILE *script_fd = fopen(script_path, "w");

#ifdef HAVE_FESTIVAL_BUILTIN
		sprintf(command_line, "%s%s --libdir %s%s -b %s",
			script->file->exec_path, FESTIVAL_PATH,
			script->file->exec_path, FESTIVAL_LIB_PATH,
			script_path);
#else
		sprintf(command_line, "festival -b %s", script_path);
#endif

// Create script.
// The maximum text length is limited with the command line

		fprintf(script_fd,
			"(voice_%s)\n"
			"(set! text (Utterance Text \"%s\"))\n"
			"(utt.synth text)"
			"(utt.save.wave text \"-\")",
			character->voice,
			string2);
		fclose(script_fd);


		if(debug)
		{
			printf("SceneChunk::render %d %s\n",
				__LINE__,
				command_line);

			FILE *script_fd = fopen(script_path, "r");
			while(!feof(script_fd))
				fputc(fgetc(script_fd), stdout);
			printf("\n");
			fclose(script_fd);
		}

// popen only does half duplex
		FILE *fd = popen(command_line, "r");

// Capture output
		if(fd)
		{
			int audio_start = audio_size;


			if(debug) printf("SceneChunk::render %d\n",
				__LINE__);
			while(!feof(fd))
			{
				if(debug) printf("SceneChunk::render %d\n",
					__LINE__);
				if(audio_size + FREAD_SIZE > audio_allocated)
				{
					audio_allocated += FREAD_SIZE;
					audio = (unsigned char*)realloc(audio, audio_allocated);
				}


				if(debug) printf("SceneChunk::render %d audio=%p audio_size=%d\n",
					__LINE__,
					audio,
					audio_size);

				int bytes_read = fread(audio + audio_size, 1, FREAD_SIZE, fd);
				if(debug) printf("SceneChunk::render %d bytes_read=%d\n",
					__LINE__,
					bytes_read);
				audio_size += bytes_read;
 				if(bytes_read < FREAD_SIZE)
 				{
 					break;
 				}
			}


			pclose(fd);

			if(debug) printf("SceneChunk::render %d audio=%p audio_size=%d audio_allocated=%d\n",
				__LINE__,
				audio,
				audio_size,
				audio_allocated);

// Strip WAV header

			if(audio_size - audio_start > WAVHEADER)
			{
// for(int i = 0; i < 128; i++)
// {
// printf("%c ", audio[audio_start + i]);
// if(!((i + 1) % 16)) printf("\n");
// }
// printf("\n");
// Find header after error messages
				int header_size = WAVHEADER;
				for(int i = audio_start; i < audio_start + audio_size - 4; i++)
				{
					if(audio[i] == 'R' &&
						audio[i + 1] == 'I' &&
						audio[i + 2] == 'F' &&
						audio[i + 3] == 'F')
					{
						header_size += i - audio_start;
						break;
					}
				}

				memcpy(audio + audio_start,
					audio + audio_start + header_size,
					audio_size - audio_start - header_size);
				audio_size -= header_size;
				if(debug) printf("SceneChunk::render %d: audio_size=%d\n",
					__LINE__,
					audio_size);
			}

			advance_samples = audio_size / 2;
		}
		else
		{
			printf("SceneChunk::render %d: Couldn't run %s: %s\n",
				__LINE__,
				command_line,
				strerror(errno));
		}

		remove(script_path);
		if(debug) printf("SceneChunk::render %d max=%f\n",
			__LINE__,
			max);
	}


	if(text)
	{

// Truncate if ...
		text_ptr = text + len - 1;
		while(text_ptr > text &&
			(*text_ptr == ' ' ||
			*text_ptr == '\n'))
			text_ptr--;
		if(text_ptr > text + 3 &&
			*(text_ptr - 0) == '.' &&
			*(text_ptr - 1) == '.' &&
			*(text_ptr - 2) == '.')
		{
			advance_samples -= ADVANCE_SAMPLES;
			if(advance_samples < 0) advance_samples = 0;
		}

// Calculate loudest part
		max = 0;
		for(int i = 0; i < audio_size; i += 2)
		{
			int16_t sample = audio[i] |
				(audio[i + 1] << 8);
			double sample_float = fabs((double)sample / 32768);
			if(sample_float > max) max = sample_float;
		}
	}
}













SceneTokens::SceneTokens(FileScene *file, int cpus)
{
	background[0] = 0;
	background_image = 0;
	timestamp = 0;
	this->cpus = cpus;
	this->file = file;
//	overlayer = 0;
}

SceneTokens::~SceneTokens()
{
	chunks.remove_all_objects();
	characters.remove_all_objects();
	delete background_image;
//	delete overlayer;
}

SceneChar* SceneTokens::get_character(char *name)
{
	const int debug = 0;
	if(debug) printf("SceneTokens::get_character %d %d\n",
		__LINE__,
		characters.size());

	for(int i = 0; i < characters.size(); i++)
	{
		if(!strcmp(characters.get(i)->name, name)) return characters.get(i);
	}
	if(debug) printf("SceneTokens::get_character %d %d\n",
		__LINE__,
		characters.size());

	SceneChar *result = new SceneChar(this);
	if(debug) printf("SceneTokens::get_character %d %d this=%p\n",
		__LINE__,
		characters.size(),
		&characters);

	characters.append(result);
	if(debug) printf("SceneTokens::get_character %d %d\n",
		__LINE__,
		characters.size());

	strcpy(result->name, name);
	if(debug) printf("SceneTokens::get_character %d %d\n",
		__LINE__,
		characters.size());

	if(debug) printf("SceneTokens::get_character %d %d\n",
		__LINE__,
		characters.size());

	return result;
}

SceneChar* SceneTokens::get_character(int number)
{
	return characters.get(number);
}

int SceneTokens::get_char_number(SceneChar *ptr)
{
	for(int i = 0; i < characters.size(); i++)
		if(ptr == characters.get(i)) return i;
	return 0;
}

SceneChunk* SceneTokens::new_chunk()
{
	SceneChunk *result = new SceneChunk(this);
	chunks.append(result);
	return result;
}

int SceneTokens::total_chunks()
{
	return chunks.size();
}

int SceneTokens::total_characters()
{
	return characters.size();
}

SceneChunk* SceneTokens::get_chunk(int number)
{
	return chunks.get(number);
}

int SceneTokens::read_script(char *path)
{
	const int debug = 0;

	strcpy(this->path, path);

	FILE *fd = fopen(path, "r");
	if(fd)
	{
// Read 1 line
		char string[BCTEXTLEN];
// Current character name
		char char_name[BCTEXTLEN];
		SceneChar *current_char = 0;
		char_name[0] = 0;
		SceneChunk *current_chunk = 0;
		int current_line = 0;
		int i;

		while(!feof(fd))
		{
			char *result = fgets(string, BCTEXTLEN, fd);
			current_line++;

			if(result)
			{
				int len = strlen(string);
				STRIP_LINE(string)

				if(debug) printf("SceneTokens::read_script %d: %s\n",
					__LINE__,
					string);

// Skip the file ID & empty lines
				if(string[0] == 0 ||
					!strncmp(string, "TEXT2MOVIE", 10))
					continue;

				int got_it = 0;
				for(i = 0; i < len; i++)
				{
					if(isalnum(string[i]))
					{
						got_it = 1;
						i = len;
					}
				}

				if(!got_it) continue;

// A line all in caps is a character name
				got_it = 1;
				for(i = 0; i < len; i++)
				{
					if(islower(string[i]))
					{
						got_it = 0;
						i = len;
					}
				}

				if(got_it)
				{
					strcpy(char_name, string);

					if(debug) printf("SceneTokens::read_script %d: char_name=%s\n",
						__LINE__,
						char_name);

					current_char = get_character(char_name);

					if(debug) printf("SceneTokens::read_script %d current_char=%p\n",
						__LINE__,
						current_char);

// Reset the current chunk pointer
					current_chunk = 0;
					i = len;
				}
				else
					i = 0;

// Certain words are commands
				for(; i < len; i++)
				{
					if(string[i] == '[' || isalnum(string[i]))
					{
						char *command = string + i;

						STRING_PARAMETER("voice:", 1, current_char->voice)
						else
						STRING_PARAMETER("model:", 1, current_char->model)
						else
						STRING_PARAMETER("background:", 0, background)
						else
// Default is dialogue
						{
							if(!current_char)
							{
								printf("SceneTokens::read_script %d Line %d: dialogue text but no current character\n",
									__LINE__,
									current_line);
							}
							else
							{
								if(!current_chunk)
								{
									current_chunk = new_chunk();
									current_chunk->character = current_char;
								}

// Append dialogue to current chunk
								current_chunk->append_text(string + i);
							}

							i = len;
						}
					}
				}
			}
		}



		fclose(fd);

		if(debug) printf("SceneTokens::read_script %d total_chunks=%d\n",
			__LINE__,
			total_chunks());
// Parse commands in dialogue
		for(i = 0; i < total_chunks(); i++)
		{
			SceneChunk *chunk = get_chunk(i);
			if(chunk->text)
			{
				char *ptr = chunk->text;
				char *end = chunk->text + strlen(chunk->text);
				int got_text = 0;
				if(debug) printf("SceneTokens::read_script %d %s\n", __LINE__, chunk->text);
				while(ptr < end)
				{
// Start of command
					if(*ptr == '[')
					{
						if(debug) printf("SceneTokens::read_script %d [\n", __LINE__);
// Split chunk
						if(got_text)
						{
							SceneChunk *new_chunk = new SceneChunk(this);
							new_chunk->character = chunk->character;
							chunks.insert(new_chunk, i + 1);

// Move text from start of command to new chunk.
							new_chunk->append_text(ptr);
// Truncate current chunk
							*ptr = 0;
// Advance to next chunk
							ptr = new_chunk->text;
							end = new_chunk->text + strlen(new_chunk->text);
							chunk = new_chunk;
							i++;
						}
						if(debug) printf("SceneTokens::read_script %d\n", __LINE__);

// Read command
						while(ptr < end &&
							(*ptr == '[' || *ptr == ' ' || *ptr == '\n'))
							ptr++;

						if(debug) printf("SceneTokens::read_script %d\n", __LINE__);

						char *ptr2 = string;
						char *string_end = string + BCTEXTLEN;
						while(*ptr != ']' &&
							*ptr != ' ' &&
							*ptr != '\n' &&
							ptr < end &&
							ptr2 < string_end - 1)
						{
							*ptr2++ = *ptr++;
						}
						*ptr2 = 0;
						if(debug) printf("SceneTokens::read_script %d command=%s\n", __LINE__, string);

// Convert command to code
						if(!strcasecmp(string, "pause"))
						{
							chunk->command = SceneChunk::PAUSE_COMMAND;
						}
						else
						{
// TODO: line numbers
							printf("SceneTokens::read_script %d: Unknown command '%s'\n",
								__LINE__,
								string);
						}
						if(debug) printf("SceneTokens::read_script %d\n", __LINE__);

// Search for more text
						while(ptr < end &&
							(*ptr == ']' || *ptr == ' ' || *ptr == '\n'))
							ptr++;

// Create new chunk for rest of text
						if(ptr < end)
						{
							SceneChunk *new_chunk = new SceneChunk(this);
							new_chunk->character = chunk->character;
							chunks.insert(new_chunk, i + 1);
// Move text from end of command to new chunk.
							new_chunk->append_text(ptr);
// Parse next chunk in next iteration
							ptr = end;
//							i++;
						}
						if(debug) printf("SceneTokens::read_script %d\n", __LINE__);

// Truncate current chunk
						chunk->text[0] = 0;
					}
					else
// Got non whitespace
					if(*ptr != ' ' &&
						*ptr != '\n')
					{
						got_text = 1;
					}

					ptr++;
				}
			}
		}


		if(debug) dump();
		return 0;
	}

	printf("SceneTokens::read_script %d: %s %s\n", __LINE__, path, strerror(errno));
	return 1;
}

void SceneTokens::convert_path(char *dst, char *src)
{
// Absolute path in src
	if(src[0] == '/')
	{
		strcpy(dst, src);
	}
	else
// Relative path
	{
// Try directory of script
		FileSystem fs;
		fs.extract_dir(dst, path);
		strcat(dst, src);

		struct stat ostat;
		if(stat(dst, &ostat))
		{
// Try cinelerra directory
			strcpy(dst, File::get_cindat_path());
			strcat(dst, ASSET_PATH);
			strcat(dst, src);
//printf("SceneTokens::convert_path %d %s\n", __LINE__, dst);
		}
	}
}

VFrame* SceneTokens::load_image(char *path)
{
	VFrame *result = 0;
	char complete_path[BCTEXTLEN];
	convert_path(complete_path, path);

	int64_t size = FileSystem::get_size(complete_path);
	if(!size)
	{
		printf("SceneTokens::load_image %d: Couldn't open %s\n",
			__LINE__,
			complete_path);
		return 0;
	}

	unsigned char *data = new unsigned char[size + 4];
	FILE *fd = fopen(complete_path, "r");
	(void)fread(data + 4, 1, size, fd);
	data[0] = (size >> 24) & 0xff;
	data[1] = (size >> 16) & 0xff;
	data[2] = (size >> 8) & 0xff;
	data[3] = size & 0xff;
	result = new VFramePng(data, 1.);
	delete [] data;


	if(!BC_CModels::has_alpha(result->get_color_model()) )
		printf("SceneTokens::load_image %d: image %s has no alpha channel\n",
			__LINE__,
			path);
	return result;
}

void SceneTokens::render_background(SceneGraph *scene)
{
// Decompress background image
	if(!background_image)
	{
		background_image = load_image(background);
	}

	if(background_image)
	{
		SceneNode *node = new SceneNode("background");
		scene->append(node);
		node->image = background_image;
		node->x = 0;
		node->y = 0;
		node->sx = 1;
		node->sy = 1;
	}
}

int SceneTokens::get_memory_usage()
{
	int total = 0;
	if(background_image) total += background_image->get_memory_usage();
	for(int i = 0; i < total_chunks(); i++)
	{
		total += get_chunk(i)->get_memory_usage();
	}
	for(int i = 0; i < total_characters(); i++)
	{
		total += get_character(i)->get_memory_usage();
	}
	return total;
}


void SceneTokens::dump()
{
	printf("SceneTokens::dump %d background=%s\n", __LINE__, background);
	for(int i = 0; i < characters.size(); i++)
	{
		characters.get(i)->dump();
	}


	for(int i = 0; i < chunks.size(); i++)
	{
		chunks.get(i)->dump();
	}
}




