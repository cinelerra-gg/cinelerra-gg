
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

#ifndef BCHASH_H
#define BCHASH_H



// Key/value table with persistent storage in stringfiles.


#include "bcwindowbase.inc"
#include "bctextbox.inc"
#include "units.h"


class BC_Hash
{
public:
	BC_Hash();
	BC_Hash(const char *filename);
	virtual ~BC_Hash();

	int load_file(FILE *fp);
	int load();        // load from disk file
	int save_file(FILE *fp);
	int save();        // save to disk file
	int load_string(const char *string);        // load from string
	int save_string(char* &string);       // save to new string
	int update(const char *name, double value); // update a value if it exists
	int update(const char *name, float value); // update a value if it exists
	int update(const char *name, int32_t value); // update a value if it exists
	int update(const char *name, int64_t value); // update a value if it exists
	int update(const char *name, const char *value); // create it if it doesn't

	double get(const char *name, double default_);   // retrieve a value if it exists
	float get(const char *name, float default_);   // retrieve a value if it exists
	int32_t get(const char *name, int32_t default_);   // retrieve a value if it exists
	int64_t get(const char *name, int64_t default_);   // retrieve a value if it exists
	char* get(const char *name, char *default_); // return 1 if it doesn't

// same as above using vararg format to specify name
	int updatef(double value, const char *fmt, ...);
	int updatef(float value, const char *fmt, ...);
	int updatef(int32_t value, const char *fmt, ...);
	int updatef(int64_t value, const char *fmt, ...);
	int updatef(const char *value, const char *fmt, ...);

	double getf(double default_, const char *fmt, ...);
	float getf(float default_, const char *fmt, ...);
	int32_t getf(int32_t default_, const char *fmt, ...);
	int64_t getf(int64_t default_, const char *fmt, ...);
	char* getf(char *default_, const char *fmt, ...);

// Update values with values from another table.
// Adds values that don't exist and updates existing values.
	void copy_from(BC_Hash *src);
// Return 1 if the tables are equivalent
	int equivalent(BC_Hash *src);

	void dump();

	int size();
	char* get_key(int number);
	char* get_value(int number);
	void clear();

private:
// Reallocate table so at least total entries exist
	void reallocate_table(int total);

	char **names;  // list of string names
	char **values;    // list of values
	int total;             // number of defaults
	int allocated;         // allocated defaults
	char filename[BCTEXTLEN];        // filename the defaults are stored in
};

#endif
