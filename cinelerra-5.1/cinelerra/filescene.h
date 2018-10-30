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



#ifndef FILESCENE_H
#define FILESCENE_H


#include "filebase.h"
#include "scenegraph.inc"

// Simple text to movie converter.
// Initially we're trying to render the entire movie in the file handler.
// Ideally the file handler would just translate the script into scene graphs &
// audio with the 3D pipeline integrated into Cinelerra.


class SceneTokens;
class FileScene;




// Parsed script





class SceneChar
{
public:
	SceneChar(SceneTokens *script);
	~SceneChar();

// called by renderer to keep track of cameras
	void increment_camera();
	int read_model();
	void dump();
	int get_memory_usage();

// Name of character in script
	char name[BCTEXTLEN];
// Festival voice for character
	char voice[BCTEXTLEN];
// Name of model
	char model[BCTEXTLEN];


	SceneTokens *script;
	SceneNode *body;
	SceneNode *head;
	int body_order;
	int head_order;
	ArrayList<SceneNode *> mouths;
	ArrayList<SceneNode *> eyes;

// the model faces left instead of right
	int faces_left;

// automated camera when this character is speaking
// increments for each chunk
	int current_camera;
	enum
	{
		CAMERA_WIDE,
		CAMERA_CU,
		CAMERA_TOTAL
	};

	int is_speeking;
// Volume for mouth image
	double max;

// Extents are all floating point for 3D
// Dimensions of model
	float w, h;
	float scale;
};


// Dialog from a single character
class SceneChunk
{
public:
	SceneChunk(SceneTokens *script);
	~SceneChunk();

	void dump();
	void append_text(char *new_chunk);
// Render the audio
	void render();
	int get_memory_usage();

// Rendered output
	unsigned char *audio;
// Nonzero if it has been rendered
// Units are bytes since we have WAV headers
	int audio_size;
	int audio_allocated;
// Number of samples to advance dialog playback.
// May be shorter than the audio to get characters to talk simultaneously.
	int advance_samples;
// If it was used in the last buffer read
	int used;
// Loudest audio segment
	double max;

// Dialogue
	char *text;
// Command
	int command;
	enum
	{
		NO_COMMAND,
		PAUSE_COMMAND,
	};

// Pointer to character
	SceneChar *character;
	SceneTokens *script;
};

// Entire script
class SceneTokens
{
public:
	SceneTokens(FileScene *file, int cpus);
	~SceneTokens();

	int read_script(char *path);
// Get character or create new one if it doesn't exist
	SceneChar* get_character(char *name);
	SceneChar* get_character(int number);
	int get_char_number(SceneChar *ptr);
// Create a new text token & return it
	SceneChunk* new_chunk();
	SceneChunk* get_chunk(int number);
	int get_memory_usage();
// Number of text objects
	int total_chunks();
// Number of characters
	int total_characters();
	void dump();
// Convert asset path to path relatiive to script
	void convert_path(char *dst, char *src);
// Load image with path completion
	VFrame* load_image(char *path);
	void render_background(SceneGraph *scene);

	ArrayList<SceneChunk*> chunks;
	ArrayList<SceneChar*> characters;
	char background[BCTEXTLEN];
// Path the script was read from
	char path[BCTEXTLEN];
// Decompressed assets
	VFrame *background_image;
//	OverlayFrame *overlayer;
	int64_t timestamp;
	int cpus;
	FileScene *file;
};


class FileScene : public FileBase
{
public:
	FileScene(Asset *asset, File *file);
	~FileScene();

	int open_file(int rd, int wr);
	int close_file();
	static int check_sig(Asset *asset, char *test);

	int set_video_position(int64_t x);
	int set_audio_position(int64_t x);
	int read_frame(VFrame *frame);
	int read_samples(double *buffer, int64_t len);
	int64_t get_memory_usage();
// Direct copy routines
	static int get_best_colormodel(Asset *asset, int driver);
	int colormodel_supported(int colormodel);
	int can_copy_from(Asset *asset, int64_t position); // This file can copy frames directly from the asset
	int reset_parameters_derived();

// Path to all prepackaged assets
	char exec_path[BCTEXTLEN];


private:
	int read_script();
	void render_chunks(int64_t start_position, int64_t len, int all_channels);

// Temporary buffer for speech output
	int16_t *audio_temp;
// In samples
	int temp_allocated;

// Parsed script
	SceneTokens *script;
//	OverlayFrame *overlayer;
//	AffineEngine *affine;
};







#endif



