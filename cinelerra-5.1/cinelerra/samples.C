
/*
 * CINELERRA
 * Copyright (C) 2009 Adam Williams <broadcast at earthling dot net>
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



// An object which contains samples



#include "samples.h"
#include <stdio.h>
#include <sys/shm.h>
#include <unistd.h>


Samples::Samples()
{
	reset();
}

// Allocate number of samples
Samples::Samples(int samples)
{
	reset();
	allocate(samples, 1);
}

Samples::Samples(Samples *src)
{
	reset();
	share(src->get_shmid());
	set_allocated(src->get_allocated());
	set_offset(src->get_offset());
}

Samples::~Samples()
{
	clear_objects();
}

void Samples::reset()
{
	use_shm = 1;
	shmid = -1;
	data = 0;
	allocated = 0;
	offset = 0;
}

void Samples::clear_objects()
{
	if(use_shm)
	{
		if(data) shmdt(data);
	}
	else
	{
		delete [] data;
	}

	reset();
}


void Samples::share(int shmid)
{
	if(data)
	{
		if(use_shm)
			shmdt(data);
		else
			delete [] data;
	}

	this->use_shm = 1;
	data = (double*)shmat(shmid, NULL, 0);
	this->allocated = 0;
	this->shmid = shmid;
}

void Samples::allocate(int samples, int use_shm)
{
	if(data &&
		this->allocated >= samples &&
		this->use_shm == use_shm) return;

	if(data)
	{
		if(this->use_shm)
			shmdt(data);
		else
			delete [] data;
	}

	this->use_shm = use_shm;

	if(use_shm)
	{
		shmid = shmget(IPC_PRIVATE,
			(samples + 1) * sizeof(double),
			IPC_CREAT | 0777);
		data = (double*)shmat(shmid, NULL, 0);
	// This causes it to automatically delete when the program exits.
		shmctl(shmid, IPC_RMID, 0);
	}
	else
	{
		shmid = -1;
		data = new double[samples];
	}


	this->allocated = samples;




}

// Get the buffer
double* Samples::get_data()
{
	return data + offset;
}

// Get number of samples allocated
int Samples::get_allocated()
{
	return allocated;
}

// Set number of samples allocated
void Samples::set_allocated(int allocated)
{
	this->allocated = allocated;
}

int Samples::get_shmid()
{
	return shmid;
}

void Samples::set_offset(int offset)
{
	this->offset = offset;
}

int Samples::get_offset()
{
	return offset;
}

