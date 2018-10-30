/* readpic.c, read source pictures                                          */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "global.h"


static void read_mpeg(long number, unsigned char *frame[])
{
	long real_number;

// Normalize frame_rate
	real_number = (long)((double)mpeg3_frame_rate(mpeg_file, 0) / 
		frame_rate * 
		number + 
		0.5);

	while(mpeg3_get_frame(mpeg_file, 0) <= real_number)
		mpeg3_read_yuvframe(mpeg_file,
			(char*)frame[0],
			(char*)frame[1],
			(char*)frame[2],
			0,
			0,
			horizontal_size,
			vertical_size,
			0);
/* Terminate encoding after processing this frame */
	if(mpeg3_end_of_video(mpeg_file, 0)) frames_scaled = 1; 
}

static void read_stdin(long number, unsigned char *frame[])
{
	int chroma_denominator = 1;
	unsigned char data[5];


	if(chroma_format == 1) chroma_denominator = 2;

	fread(data, 4, 1, stdin_fd);

// Terminate encoding before processing this frame
	if(data[0] == 0xff && data[1] == 0xff && data[2] == 0xff && data[3] == 0xff)
	{
		frames_scaled = 0;
		return;
	}

	fread(frame[0], width * height, 1, stdin_fd);
	fread(frame[1], width / 2 * height / chroma_denominator, 1, stdin_fd);
	fread(frame[2], width / 2 * height / chroma_denominator, 1, stdin_fd);
}

static void read_buffers(long number, unsigned char *frame[])
{
	int chroma_denominator = 1;

	if(chroma_format == 1) chroma_denominator = 2;

	pthread_mutex_lock(&input_lock);

	if(input_buffer_end) 
	{
		frames_scaled = 0;
		pthread_mutex_unlock(&copy_lock);
		pthread_mutex_unlock(&output_lock);
		return;
	}

	memcpy(frame[0], input_buffer_y, width * height);
	memcpy(frame[1], input_buffer_u, width / 2 * height / chroma_denominator);
	memcpy(frame[2], input_buffer_v, width / 2 * height / chroma_denominator);
	pthread_mutex_unlock(&copy_lock);
	pthread_mutex_unlock(&output_lock);
}

void readframe(int frame_num, uint8_t *frame[])
{
	int n;
	n = frame_num % (2 * READ_LOOK_AHEAD);

	frame[0] = frame_buffers[n][0];
	frame[1] = frame_buffers[n][1];
	frame[2] = frame_buffers[n][2];
	

	switch (inputtype)
	{
		case T_MPEG:
			read_mpeg(frame_num, frame);
			break;
		case T_STDIN:
			read_stdin(frame_num, frame);
			break;
		case T_BUFFERS:
			read_buffers(frame_num, frame);
			break;
		default:
			break;
  }
}
