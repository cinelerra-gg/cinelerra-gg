
/*
 * CINELERRA
 * Copyright (C) 2008 Adam Williams <broadcast at earthling dot net>
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

#ifndef FILEXML_H
#define FILEXML_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "arraylist.h"
#include "mutex.h"
#include "keyframe.inc"
#include "filexml.inc"
#include "sizes.h"

#define MAX_TITLE 256

class XMLBuffer
{
	long bsz, isz;
	unsigned char *inp, *outp, *bfr, *lmt;
	int destroy;
	Mutex *share_lock;

	int demand(long len);
	friend class KeyFrame;
	friend class FileXML;
public:
	XMLBuffer(long buf_size=0x1000, long max_size=LONG_MAX, int del=1);
	XMLBuffer(long buf_size, char *buf, int del=0); // writing
	XMLBuffer(const char *buf, long buf_size, int del=0); // reading
	~XMLBuffer();

	long otell() { return inp - bfr; }
	long itell() { return outp - bfr; }
	void oseek(long pos) { inp = bfr + pos; }
	void iseek(long pos) { outp = bfr + pos; }
	unsigned char *pos(long ofs=0) { return bfr+ofs; }
	char *cstr() { if( demand(otell()+1) ) *inp = 0; return (char*)bfr; }
	int read(char *bp, int n);
	int write(const char *bp, int n);
	void copy_from(XMLBuffer *xbfr);

	int cur()  { return outp>=inp ? -1 : *outp; }
	int next() { return outp>=inp ? -1 : *outp++; }
	int next(int ch) { return !demand(otell()+1) ? -1 : *inp++ = ch; }

	static char *decode_data(char *bp, const char *sp, int n=-1);
	static char *encode_data(char *bp, const char *sp, int n=-1);
	static char *copy_data(char *bp, const char *sp, int n=-1);
	static long encoded_length(const char *sp, int n=-1);
	static long copy_length(const char *sp, int n=-1);
};

class XMLTag
{
	XMLBuffer *buffer;
	class Property {
	public:
		const char *prop;
		const char *value;

		Property(const char *pp, const char *vp);
		~Property();
	};
	bool ws(char ch) { return ch==' ' || ch=='\n'; }

public:
	XMLTag();
	~XMLTag();

	int reset_tag();

	int title_is(const char *title);
	char *get_title();
	int get_title(char *value);
	int test_property(char *property, char *value);
	const char *get_property_text(int number);
	int get_property_int(int number);
	float get_property_float(int number);
	const char *get_property(const char *property);
	const char* get_property(const char *property, char *value);
	int32_t get_property(const char *property, int32_t default_);
	int64_t get_property(const char *property, int64_t default_);
	float get_property(const char *property, float default_);
	double get_property(const char *property, double default_);

	int set_title(const char *text);
	int set_property(const char *text, const char *value);
	int set_property(const char *text, int32_t value);
	int set_property(const char *text, int64_t value);
	int set_property(const char *text, float value);
	int set_property(const char *text, double value);

	int write_tag(FileXML *xml);
	int read_tag(FileXML *xml);

	char title[MAX_TITLE];
	ArrayList<Property*> properties;
	char *string;
	long used, avail;
};


class FileXML
{
public:
	FileXML(int coded=1);
	~FileXML();

	int terminate_string();
	int append_newline();
	int append_tag();
	int append_text(const char *text);
	int append_text(const char *text, long len);
	int append_data(const char *text);
	int append_data(const char *text, long len);

	char *read_text();
	int read_data_until(const char *tag_end, XMLBuffer *xbfr, int skip=0);
	int read_text_until(const char *tag_end, XMLBuffer *xbfr, int skip=0);
	int read_tag();
	int skip_tag();
	int write_to_file(const char *filename);
	int write_to_file(FILE *file, const char *filename="");
	int read_from_file(const char *filename, int ignore_error = 0);
	int read_from_string(char *string);
	char *(*decode)(char *bp, const char *sp, int n);
	char *(*encode)(char *bp, const char *sp, int n);
	long (*coded_length)(const char *sp, int n);

	void set_coding(int coding);
	int get_coding();
	int set_shared_input(XMLBuffer *xbfr);
	int set_shared_output(XMLBuffer *xbfr);

	int rewind();
	char *get_data();
	char *string();
	long length();

	XMLBuffer *buffer;
	int coded, shared;

	XMLTag tag;
	long output_length;
	char *output;
	char left_delimiter, right_delimiter;
	char filename[MAX_TITLE];
	static const char *xml_header;
	static const int xml_header_size;
};

#endif
