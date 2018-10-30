
/*
 * CINELERRA
 * Copyright (C) 2010 Adam Williams <broadcast at earthling dot net>
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

#include "bchash.h"
#include "bcsignals.h"
#include "clip.h"
#include "cstrdup.h"
#include "filexml.h"
#include "keyframe.h"

#include <stdio.h>
#include <string.h>



KeyFrame::KeyFrame()
 : Auto()
{
	xbuf = new XMLBuffer();
}

KeyFrame::KeyFrame(EDL *edl, KeyFrames *autos)
 : Auto(edl, (Autos*)autos)
{
	xbuf = new XMLBuffer();
}

KeyFrame::~KeyFrame()
{
	delete xbuf;
}

void KeyFrame::load(FileXML *file)
{
//printf("KeyFrame::load 1\n");
// Shouldn't be necessary
//	position = file->tag.get_property((char*)"POSITION", position);
//printf("KeyFrame::load 1\n");
	xbuf->iseek(0);  xbuf->oseek(0);
	file->read_data_until((char*)"/KEYFRAME", xbuf, 1);
}

void KeyFrame::copy(int64_t start, int64_t end, FileXML *file, int default_auto)
{
	file->tag.set_title((char*)"KEYFRAME");
	if(default_auto)
		file->tag.set_property((char*)"POSITION", 0);
	else
		file->tag.set_property((char*)"POSITION", position - start);
// default_auto converts a default auto to a normal auto
	if(is_default && !default_auto)
		file->tag.set_property((char*)"DEFAULT", 1);
	file->append_tag();
// Can't put newlines in because these would be reimported and resaved along
// with new newlines.
	char *data = (char*)xbuf->cstr();
	file->append_data(data, strlen(data));

	file->tag.set_title((char*)"/KEYFRAME");
	file->append_tag();
	file->append_newline();
}


void KeyFrame::copy_from(Auto *that)
{
	copy_from((KeyFrame*)that);
}

void KeyFrame::copy_data(KeyFrame *src)
{
	xbuf->copy_from(src->xbuf);
}

void KeyFrame::copy_from(KeyFrame *that)
{
	Auto::copy_from(that);
	copy_data(that);
	position = that->position;
}

int KeyFrame::identical(KeyFrame *src)
{
	return !strcasecmp(xbuf->cstr(), src->xbuf->cstr());
}

void KeyFrame::get_contents(BC_Hash *ptr, char **text, char **extra)
{
	FileXML input;
	input.set_shared_input(xbuf);
	char *this_text = 0, *this_extra = 0;
	if( !input.read_tag() ) {
		for( int i=0; i<input.tag.properties.size(); ++i ) {
			const char *key = input.tag.get_property_text(i);
			const char *value = input.tag.get_property(key);
			ptr->update(key, value);
		}

// Read any text after tag
		this_text = input.read_text();
		(*text) = cstrdup(this_text);

// Read remaining data
		this_extra = input.get_data();
		(*extra) = cstrdup(this_extra);
	}
}

void KeyFrame::update_parameter(BC_Hash *params,
	const char *text, const char *extra)
{
	BC_Hash this_params;
	char *this_text = 0, *this_extra = 0;
	get_contents(&this_params, &this_text, &this_extra);

	FileXML input, output;
	input.set_shared_input(xbuf);
	if( !input.read_tag() ) {
// Replicate tag
		output.tag.set_title(input.tag.get_title());

// Get each parameter from this keyframe
		for( int i=0; i<this_params.size(); ++i ) {
			const char *key = this_params.get_key(i);
			const char *value = this_params.get_value(i);
// Get new value from the params argument
			if( params ) {
				for( int j=0; j<params->size(); ++j ) {
					if( !strcmp(params->get_key(j), key) ) {
						value = params->get_value(j);
						break;
					}
				}
			}

			output.tag.set_property(key, value);
		}

// Get each parameter from params argument
		if( params ) {
			for( int i=0; i<params->size(); ++i ) {
				const char *key = params->get_key(i);
				int got_it = 0;
				for( int j=0; j<this_params.size(); ++j ) {
					if( !strcmp(this_params.get_key(j), key) ) {
						got_it = 1;
						break;
					}
				}

// If it wasn't found in output, set new parameter in output.
				if( !got_it ) {
					output.tag.set_property(key, params->get_value(i));
				}
			}
		}

		output.append_tag();
// Write anonymous text & duplicate the rest
		output.append_text(text ? text : this_text);
		output.append_data(extra ? extra : this_extra);
		output.terminate_string();
// Move output to input
		xbuf->oseek(0);
		xbuf->write(output.string(), output.length());
	}

	delete [] this_text;
	delete [] this_extra;
}


void KeyFrame::get_diff(KeyFrame *src,
		BC_Hash **params, char **text, char **extra)
{
	BC_Hash this_params;  char *this_text = 0, *this_extra = 0;
	BC_Hash src_params;   char *src_text = 0,  *src_extra = 0;

	get_contents(&this_params, &this_text, &this_extra);
	src->get_contents(&src_params, &src_text, &src_extra);

// Capture changed parameters
	char this_value[BCTEXTLEN];
	int m = MIN(this_params.size(), src_params.size());
	for( int i=0; i<m; ++i ) {
		const char *src_key = src_params.get_key(i);
		const char *src_value = src_params.get_value(i);
		this_value[0] = 0;
		this_params.get(src_key, this_value);
// Capture values which differ
		if( strcmp(src_value, this_value) ) {
			if(!(*params)) (*params) = new BC_Hash;
			(*params)->update(src_key, src_value);
		}
	}


// Capture text	which differs
	if( !this_text || strcmp(this_text, src_text) )
		(*text) = cstrdup(src_text);

	if( !this_extra || strcmp(this_extra, src_extra) )
		(*extra) = cstrdup(src_extra);

	delete [] this_text;
	delete [] this_extra;
	delete [] src_text;
	delete [] src_extra;
}

int KeyFrame::operator==(Auto &that)
{
	return identical((KeyFrame*)&that);
}

int KeyFrame::operator==(KeyFrame &that)
{
	return identical(&that);
}

char *KeyFrame::get_data(int64_t sz)
{
	if( sz >= 0 ) xbuf->demand(sz);
	return xbuf->cstr();
}

void KeyFrame::set_data(char *data)
{
	xbuf->oseek(0);
	xbuf->write(data, strlen(data));
}

void KeyFrame::dump(FILE *fp)
{
	fprintf(fp,"     position: %jd\n", position);
	fprintf(fp,"     data: %s\n", xbuf->cstr());
}

