
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bcsignals.h"
#include "arraylist.h"
#include "cstrdup.h"
#include "filexml.h"
#include "mainerror.h"

// messes up cutads link
#undef eprintf
#define eprintf printf

static const char left_delm = '<', right_delm = '>';
const char *FileXML::xml_header = "<?xml version=\"1.0\"?>\n";
const int FileXML::xml_header_size = strlen(xml_header);

XMLBuffer::XMLBuffer(long buf_size, long max_size, int del)
{
	bsz = buf_size;
	bfr = new unsigned char[bsz];
	inp = outp = bfr;
	lmt = bfr + bsz;
	isz = max_size;
	destroy = del;
	share_lock = new Mutex("XMLBuffer::share_lock");
}

XMLBuffer::XMLBuffer(const char *buf, long buf_size, int del)
{	// reading
	bfr = (unsigned char *)buf;
	bsz = buf_size;
	outp = bfr;
	lmt = inp = bfr+bsz;
	isz = bsz;
	destroy = del;
	share_lock = new Mutex("XMLBuffer::share_lock");
}

XMLBuffer::XMLBuffer(long buf_size, char *buf, int del)
{	// writing
	bfr = (unsigned char *)buf;
	bsz = buf_size;
	inp = bfr;
	lmt = outp = bfr+bsz;
	isz = bsz;
	destroy = del;
	share_lock = new Mutex("XMLBuffer::share_lock");
}

XMLBuffer::~XMLBuffer()
{
	if( destroy ) delete [] bfr;
	delete share_lock;
}

int XMLBuffer::demand(long len)
{
	if( len > bsz ) {
		if( !destroy ) return 0;
		long sz = inp-bfr;
		len += sz/2 + BCTEXTLEN;
		unsigned char *np = new unsigned char[len];
		if( sz > 0 ) memcpy(np,bfr,sz);
		inp = np + (inp-bfr);
		outp = np + (outp-bfr);
		lmt = np + len;  bsz = len;
		delete [] bfr;   bfr = np;
	}
	return 1;
}

int XMLBuffer::write(const char *bp, int len)
{
	if( len <= 0 ) return 0;
	if( !destroy && lmt-inp < len ) len = lmt-inp;
	demand(otell()+len);
	memmove(inp,bp,len);
	inp += len;
	return len;
}

int XMLBuffer::read(char *bp, int len)
{
	long size = inp - outp;
	if( size <= 0 && len > 0 ) return -1;
	if( len > size ) len = size;
	memmove(bp,outp,len);
	outp += len;
	return len;
}

void XMLBuffer::copy_from(XMLBuffer *xbuf)
{
	xbuf->share_lock->lock("XMLBuffer::copy_from");
	share_lock->lock("XMLBuffer::copy_from");
	oseek(0);
	write((const char*)xbuf->pos(0), xbuf->otell());
	iseek(xbuf->itell());
	xbuf->share_lock->unlock();
	share_lock->unlock();
}


// Precision in base 10
// for float is 6 significant figures
// for double is 16 significant figures

XMLTag::Property::Property(const char *pp, const char *vp)
{
//printf("Property %s = %s\n",pp, vp);
	prop = cstrdup(pp);
	value = cstrdup(vp);
}

XMLTag::Property::~Property()
{
	delete [] prop;
	delete [] value;
}


XMLTag::XMLTag()
{
	string = 0;
	avail = used = 0;
}

XMLTag::~XMLTag()
{
	properties.remove_all_objects();
	delete [] string;
}

const char *XMLTag::get_property(const char *property, char *value)
{
	int i = properties.size();
	while( --i >= 0 && strcasecmp(properties[i]->prop, property) );
	if( i >= 0 )
		strcpy(value, properties[i]->value);
	else
		*value = 0;
	return value;
}

//getters
const char *XMLTag::get_property_text(int i) {
	return i < properties.size() ? properties[i]->prop : "";
}
int XMLTag::get_property_int(int i) {
	return i < properties.size() ? atol(properties[i]->value) : 0;
}
float XMLTag::get_property_float(int i) {
	return i < properties.size() ? atof(properties[i]->value) : 0.;
}
const char* XMLTag::get_property(const char *prop) {
	for( int i=0; i<properties.size(); ++i ) {
		if( !strcasecmp(properties[i]->prop, prop) )
			return properties[i]->value;
	}
	return 0;
}
int32_t XMLTag::get_property(const char *prop, int32_t dflt) {
	const char *cp = get_property(prop);
	return !cp ? dflt : atol(cp);
}
int64_t XMLTag::get_property(const char *prop, int64_t dflt) {
	const char *cp = get_property(prop);
	return !cp ? dflt : strtoll(cp,0,0);
}
float XMLTag::get_property(const char *prop, float dflt) {
	const char *cp = get_property(prop);
	return !cp ? dflt : atof(cp);
}
double XMLTag::get_property(const char *prop, double dflt) {
	const char *cp = get_property(prop);
	return !cp ? dflt : atof(cp);
}

//setters
int XMLTag::set_title(const char *text) {
	strcpy(title, text);
	return 0;
}
int XMLTag::set_property(const char *text, const char *value) {
	properties.append(new XMLTag::Property(text, value));
	return 0;
}
int XMLTag::set_property(const char *text, int32_t value)
{
	char text_value[BCSTRLEN];
	sprintf(text_value, "%d", value);
	set_property(text, text_value);
	return 0;
}
int XMLTag::set_property(const char *text, int64_t value)
{
	char text_value[BCSTRLEN];
	sprintf(text_value, "%jd", value);
	set_property(text, text_value);
	return 0;
}
int XMLTag::set_property(const char *text, float value)
{
	char text_value[BCSTRLEN];
	if (value - (float)((int64_t)value) == 0)
		sprintf(text_value, "%jd", (int64_t)value);
	else
		sprintf(text_value, "%.6e", value);
	set_property(text, text_value);
	return 0;
}
int XMLTag::set_property(const char *text, double value)
{
	char text_value[BCSTRLEN];
	if (value - (double)((int64_t)value) == 0)
		sprintf(text_value, "%jd", (int64_t)value);
	else
		sprintf(text_value, "%.16e", value);
	set_property(text, text_value);
	return 0;
}


int XMLTag::reset_tag()
{
	used = 0;
	properties.remove_all_objects();
	return 0;
}

int XMLTag::write_tag(FileXML *xml)
{
	XMLBuffer *buf = xml->buffer;
// title header
	buf->next(left_delm);
	buf->write(title, strlen(title));

// properties
	for( int i=0; i<properties.size(); ++i ) {
		const char *prop = properties[i]->prop;
		const char *value = properties[i]->value;
		int plen = strlen(prop), vlen = strlen(value);
		bool need_quotes = !vlen || strchr(value,' ');
		buf->next(' ');
		xml->append_text(prop, plen);
		buf->next('=');
		if( need_quotes ) buf->next('\"');
		xml->append_text(value, vlen);
		if( need_quotes ) buf->next('\"');
	}

	buf->next(right_delm);
	return 0;
}

#define ERETURN(s) do { \
  printf("XMLTag::read_tag:%d tag \"%s\"%s\n",__LINE__,title,s);\
  return 1; } while(0)
#define EOB_RETURN(s) ERETURN(", end of buffer");

int XMLTag::read_tag(FileXML *xml)
{
	XMLBuffer *buf = xml->buffer;
	int len, term;
	long prop_start, prop_end;
	long value_start, value_end;
	long ttl;
	int ch = buf->next();
	title[0] = 0;
// skip ws
	while( ch>=0 && ws(ch) ) ch = buf->next();
	if( ch < 0 ) EOB_RETURN();

// read title
	ttl = buf->itell() - 1;
	for( int i=0; i<MAX_TITLE && ch>=0; ++i, ch=buf->next() ) {
		if( ch == right_delm || ch == '=' || ws(ch) ) break;
	}
	if( ch < 0 ) EOB_RETURN();
	len = buf->itell()-1 - ttl;
	if( len >= MAX_TITLE ) ERETURN(", title too long");
// if title
	if( ch != '=' ) {
		memmove(title, buf->pos(ttl), len);
		title[len] = 0;
	}
// no title but first property.
	else {
		title[0] = 0;
		buf->iseek(ttl);
		ch = buf->next();
	}
// read properties
	while( ch >= 0 && ch != right_delm ) {
// find tag start, skip header leadin
		while( ch >= 0 && (ch==left_delm || ws(ch)) )
			ch = buf->next();
// find end of property name
		prop_start = buf->itell()-1;
		while( ch >= 0 && (ch!=right_delm && ch!='=' && !ws(ch)) )
			ch = buf->next();
		if( ch < 0 ) EOB_RETURN();
		prop_end = buf->itell()-1;
// skip ws = ws
		while( ch >= 0 && ws(ch) )
			ch = buf->next();
		if( ch == '=' ) ch = buf->next();
		while( ch >= 0 && ws(ch) )
			ch = buf->next();
		if( ch < 0 ) EOB_RETURN();
// set terminating char
		if( ch == '\"' ) {
			term = ch;
			ch = buf->next();
		}
		else
			term = ' ';
		value_start = buf->itell()-1;
		while( ch >= 0 ) {
// old edl bug work-around, allow nl in quoted string
			if( ch==term || ch==right_delm ) break;
			if( ch=='\n' && term!='\"' ) break;
			ch = buf->next();
		}
		if( ch < 0 ) EOB_RETURN();
		value_end = buf->itell()-1;
// add property
		int plen = prop_end-prop_start;
		if( !plen ) continue;
		int vlen = value_end-value_start;
		char prop[plen+1], value[vlen+1];
		const char *coded_prop = (const char *)buf->pos(prop_start);
		const char *coded_value = (const char *)buf->pos(value_start);
// props should not have coded text
		memcpy(prop, coded_prop, plen);
		prop[plen] = 0;
		xml->decode(value, coded_value, vlen);
		if( prop_end > prop_start ) {
			Property *property = new Property(prop, value);
			properties.append(property);
		}
// skip the terminating char
		if( ch != right_delm ) ch = buf->next();
	}
	if( !properties.size() && !title[0] ) ERETURN(", emtpy");
	return 0;
}



FileXML::FileXML(int coded)
{
	output = 0;
	output_length = 0;
	buffer = new XMLBuffer();
	set_coding(coded);
	shared = 0;
}

FileXML::~FileXML()
{
	if( !shared ) delete buffer;
	else buffer->share_lock->unlock();
	delete [] output;
}


int FileXML::terminate_string()
{
	append_data("", 1);
	return 0;
}

int FileXML::rewind()
{
	terminate_string();
	buffer->iseek(0);
	return 0;
}


int FileXML::append_newline()
{
	append_data("\n", 1);
	return 0;
}

int FileXML::append_tag()
{
	tag.write_tag(this);
	append_text(tag.string, tag.used);
	tag.reset_tag();
	return 0;
}

int FileXML::append_text(const char *text)
{
	if( text != 0 )
		append_text(text, strlen(text));
	return 0;
}

int FileXML::append_data(const char *text)
{
	if( text != 0 )
		append_data(text, strlen(text));
	return 0;
}

int FileXML::append_data(const char *text, long len)
{
	if( text != 0 && len > 0 )
		buffer->write(text, len);
	return 0;
}

int FileXML::append_text(const char *text, long len)
{
	if( text != 0 && len > 0 ) {
		int size = coded_length(text, len);
		char coded_text[size+1];
		encode(coded_text, text, len);
		buffer->write(coded_text, size);
	}
	return 0;
}


char* FileXML::get_data()
{
	long ofs = buffer->itell();
	return (char *)buffer->pos(ofs);
}
char* FileXML::string()
{
	return (char *)buffer->cstr();
}

long FileXML::length()
{
	return buffer->otell();
}

char* FileXML::read_text()
{
	int ch = buffer->next();
// filter out first char is new line
	if( ch == '\n' ) ch = buffer->next();
	long ipos = buffer->itell()-1;
// scan for delimiter
	while( ch >= 0 && ch != left_delm ) ch = buffer->next();
	long pos = buffer->itell()-1;
	if( ch >= 0 ) buffer->iseek(pos);
	long len = pos - ipos;
	if( len >= output_length ) {
		delete [] output;
		output_length = len+1;
		output = new char[output_length];
	}
	decode(output,(const char *)buffer->pos(ipos), len);
	return output;
}

int FileXML::read_tag()
{
	int ch = buffer->next();
// scan to next tag
	while( ch >= 0 && ch != left_delm ) ch = buffer->next();
	if( ch < 0 ) return 1;
	tag.reset_tag();
	return tag.read_tag(this);
}

int FileXML::skip_tag()
{
	char tag_title[sizeof(tag.title)];
	strcpy(tag_title, tag.title);
	int n = 1;
	while( !read_tag() ) {
		if( tag.title[0] == tag_title[0] ) {
			if( !strcasecmp(&tag_title[1], &tag.title[1]) ) ++n;
		}
		else if( tag.title[0] != '/' ) continue;
		else if( strcasecmp(&tag_title[0], &tag.title[1]) ) continue;
		else if( --n <= 0 ) return 0;
	}
	return 1;
}

int FileXML::read_data_until(const char *tag_end, XMLBuffer *xbuf, int skip)
{
	long ipos = buffer->itell();
	int pos = -1;
	for( int ch=buffer->next(); ch>=0; ch=buffer->next() ) {
		if( pos < 0 ) { // looking for next tag
			if( ch == left_delm ) {
				ipos = buffer->itell()-1;
				pos = 0;
			}
			else
				xbuf->next(ch);
			continue;
		}
		// check for end of match
		if( !tag_end[pos] && ch == right_delm ) break;
		// if mismatched, copy prefix to out
		if( tag_end[pos] != ch ) {
			xbuf->next(left_delm);
			for( int i=0; i<pos; ++i )
				xbuf->next(tag_end[i]);
			pos = -1;
			xbuf->next(ch);
			continue;
		}
		++pos;
	}
// if end tag is reached, pos is left on the < of the end tag
	if( !skip && pos >= 0 && !tag_end[pos] )
		buffer->iseek(ipos);
	return xbuf->otell();
}

int FileXML::read_text_until(const char *tag_end, XMLBuffer *xbuf, int skip)
{
	int len = read_data_until(tag_end, xbuf, skip);
	char *cp = xbuf->cstr();
	decode(cp, cp, len);
	return 0;
}

int FileXML::write_to_file(const char *filename)
{
	FILE *out = fopen(filename, "wb");
	if( !out ) {
		eprintf("write_to_file %d \"%s\": %m\n", __LINE__, filename);
		return 1;
	}
	int ret = write_to_file(out, filename);
	fclose(out);
	return ret;
}

int FileXML::write_to_file(FILE *file, const char *filename)
{
	strcpy(this->filename, filename);
	const char *str = string();
	long len = strlen(str);
// Position may have been rewound after storing
	if( !fwrite(xml_header, xml_header_size, 1, file) ||
	    ( len > 0 && !fwrite(str, len, 1, file) ) ) {
		eprintf("\"%s\": %m\n", filename);
		return 1;
	}
	return 0;
}

int FileXML::read_from_file(const char *filename, int ignore_error)
{

	strcpy(this->filename, filename);
	FILE *in = fopen(filename, "rb");
	if( !in ) {
		if(!ignore_error)
			eprintf("\"%s\" %m\n", filename);
		return 1;
	}
	fseek(in, 0, SEEK_END);
	long length = ftell(in);
	fseek(in, 0, SEEK_SET);
	char *fbfr = new char[length+1];
	delete buffer;
	(void)fread(fbfr, length, 1, in);
	fbfr[length] = 0;
	buffer = new XMLBuffer(fbfr, length, 1);
	fclose(in);
	return 0;
}

int FileXML::read_from_string(char *string)
{
        strcpy(this->filename, "");
	long length = strlen(string);
	char *sbfr = new char[length+1];
	strcpy(sbfr, string);
	delete buffer;
	buffer = new XMLBuffer(sbfr, length, 1);
        return 0;
}

void FileXML::set_coding(int coding)
{
	coded = coding;
	if( coded ) {
		decode = XMLBuffer::decode_data;
		encode = XMLBuffer::encode_data;
		coded_length = XMLBuffer::encoded_length;
	}
	else {
		decode = XMLBuffer::copy_data;
		encode = XMLBuffer::copy_data;
		coded_length = XMLBuffer::copy_length;
	}
}

int FileXML::get_coding()
{
	return coded;
}

int FileXML::set_shared_input(XMLBuffer *xbuf)
{
	strcpy(this->filename, "");
	delete buffer;
	buffer = xbuf;
	xbuf->share_lock->lock("FileXML::set_shared_input");
	xbuf->iseek(0);
	shared = 1;
	set_coding(coded);
	return 0;
}

int FileXML::set_shared_output(XMLBuffer *xbuf)
{
	strcpy(this->filename, "");
	delete buffer;
	buffer = xbuf;
	xbuf->share_lock->lock("FileXML::set_shared_output");
	xbuf->oseek(0);
	shared = 1;
	set_coding(coded);
	return 0;
}


// ================================ XML tag

int XMLTag::title_is(const char *tp)
{
	return !strcasecmp(title, tp) ? 1 : 0;
}

char* XMLTag::get_title()
{
	return title;
}

int XMLTag::get_title(char *value)
{
	if( title[0] != 0 ) strcpy(value, title);
	return 0;
}

int XMLTag::test_property(char *property, char *value)
{
	for( int i=0; i<properties.size(); ++i ) {
		if( !strcasecmp(properties[i]->prop, property) &&
		    !strcasecmp(properties[i]->value, value) )
			return 1;
	}
	return 0;
}

static inline int xml_cmp(const char *np, const char *sp)
{
   const char *tp = ++np;
   while( *np ) { if( *np != *sp ) return 0;  ++np; ++sp; }
   return np - tp;
}

char *XMLBuffer::decode_data(char *bp, const char *sp, int n)
{
   char *ret = bp;
   if( n < 0 ) n = strlen(sp);
   const char *ep = sp + n;
   while( sp < ep ) {
      if( (n=*sp++) != '&' ) { *bp++ = n; continue; }
      switch( (n=*sp++) ) {
      case 'a': // &amp;
         if( (n=xml_cmp("amp;", sp)) ) { *bp++ = '&';  sp += n;  continue; }
         break;
      case 'g': // &gt;
         if( (n=xml_cmp("gt;", sp)) ) { *bp++ = '>';  sp += n;  continue; }
         break;
      case 'l': // &lt;
         if( (n=xml_cmp("lt;", sp)) ) { *bp++ = '<';  sp += n;  continue; }
         break;
      case 'q': // &quot;
         if( (n=xml_cmp("quot;", sp)) ) { *bp++ = '"';  sp += n;  continue; }
         break;
      case '#': { // &#<num>;
         char *cp = 0;  int v = strtoul(sp,&cp,10);
         if( *cp == ';' ) { *bp++ = (char)v;  sp = cp+1;  continue; }
         n = cp - sp; }
         break;
      default:
         *bp++ = '&';
         *bp++ = (char)n;
         continue;
      }
      sp -= 2;  n += 2;
      while( --n >= 0 ) *bp++ = *sp++;
   }
   *bp = 0;
   return ret;
}


char *XMLBuffer::encode_data(char *bp, const char *sp, int n)
{
	char *ret = bp;
	if( n < 0 ) n = strlen(sp);
	const char *cp, *ep = sp + n;
	while( sp < ep ) {
		int ch = *sp++;
		switch( ch ) {
		case '<':  cp = "&lt;";    break;
		case '>':  cp = "&gt;";    break;
		case '&':  cp = "&amp;";   break;
		case '"':  cp = "&quot;";  break;
		default:  *bp++ = ch;      continue;
		}
		while( *cp != 0 ) *bp++ = *cp++;
	}
	*bp = 0;
	return ret;
}

long XMLBuffer::encoded_length(const char *sp, int n)
{
	long len = 0;
	if( n < 0 ) n = strlen(sp);
	const char *ep = sp + n;
	while( sp < ep ) {
		int ch = *sp++;
		switch( ch ) {
		case '<':  len += 4;  break;
		case '>':  len += 4;  break;
		case '&':  len += 5;  break;
		case '"':  len += 6;  break;
		default:   ++len;     break;
		}
	}
	return len;
}

char *XMLBuffer::copy_data(char *bp, const char *sp, int n)
{
	int len = n < 0 ? strlen(sp) : n;
	if( bp != sp )
		memmove(bp,sp,len);
	bp[len] = 0;
	return bp;
}

long XMLBuffer::copy_length(const char *sp, int n)
{
	int len = n < 0 ? strlen(sp) : n;
	return len;
}

