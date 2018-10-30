
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

#ifndef GARBAGE_H
#define GARBAGE_H


#include "arraylist.h"
#include "garbage.inc"
#include "mutex.inc"

// Garbage collection
// The objects inherit from GarbageObject.
// The constructor sets users to 1 so the caller must call
// Garbage::remove_user when it's done.
// Other users of the object must call add_user to increment the user count
// and Garbage::remove_user to decriment the user count.
// The object is only deleted if a call to Garbage::delete_object is made and
// the user count is 0.  returns 0 when no more users, 1 if still in use.

// Can't make graphics elements inherit because they must be deleted
// when the window is locked and they change their parent pointers.

// Elements of link lists must first be unlinked with remove_pointer and then
// passed to delete_object.

// ArrayList objects must be deleted one at a time with delete_object.
// Then the pointers must be deleted with remove_all.
//
// NOTE: Garbage must be first base class or this!=0 test is broken

class Garbage
{
	Garbage(Garbage &v) {} //disallow copy constructor
	Garbage &operator=(Garbage &v) { return *this=v; } //disallow = operator
public:
	Garbage(const char *title);
	virtual ~Garbage();

// Called when user begins to use the object.
	void add_user();
	int remove_user();

	int users;
	int deleted;
	char *title;
	Mutex *lock;
};



// class Garbage
// {
// public:
// 	Garbage();
// 	~Garbage();
//
// // Called by GarbageObject constructor
// //	void add_object(GarbageObject *ptr);
//
// // Called by user to delete the object.
// // Flags the object for deletion as soon as it has no users.
// 	static void delete_object(GarbageObject *ptr);
//
//
// // Called by remove_user and delete_object
// //	static void remove_expired();
// 	Mutex *lock;
// //	ArrayList<GarbageObject*> objects;
//
// // Global garbage collector
// 	static Garbage *garbage;
// };



#endif
