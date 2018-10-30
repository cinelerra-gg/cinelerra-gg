#include <stdio.h>
#include <stdlib.h>


#include "libzmpeg3.h"

#define PACKET_SIZE 2048

class multiplexer_t {
public:
	int derivative;
	unsigned char packet_buffer[PACKET_SIZE];
	FILE *out_file;
};

class track_t {
public:
	void *operator new(size_t n) {
		void *t = (void*) new char[n];
		memset(t,0,n);
		return t;
	}
	void operator delete(void *t,size_t n) {
		delete[](char*)t;
	}
	long bytes_decoded;
	long frames_decoded;
	long samples_decoded;
	mpeg3_t *file;
	FILE *raw_file;
	int stream_number;
	int end_of_data;
};

int write_pack_header(unsigned char *ptr, 
		multiplexer_t *mplex, 
		float seconds,
		int stream_id,
		int ac3)
{
	int packet_length;
// PACK START CODE
	*ptr++ = 0x00;
	*ptr++ = 0x00;
	*ptr++ = 0x01;
	*ptr++ = 0xba;

	if(mplex->derivative == 1)
	{
		unsigned long timestamp = (unsigned long)(seconds * 90000);
		*ptr = 0x20;
		*ptr++ |= ((timestamp & 0xc0000000) >> 29) | 1;
		*ptr++ =  (timestamp & 0x3fc00000) >> 22;
		*ptr++ =  ((timestamp & 0x003f8000) >> 14) | 1;
		*ptr++ =  (timestamp & 0x00007f80) >> 7;
		*ptr++ =  ((timestamp & 0x0000007f) << 1) | 1;

		*ptr++ = 0;
		*ptr++ = 0;
		*ptr++ = 0;
	}
	else
	if(mplex->derivative == 2)
	{
		*ptr = 0x40;
	}

	*ptr++ = 0x00;
	*ptr++ = 0x00;
	*ptr++ = 0x01;
	if(ac3) 
		*ptr++ = 0xbd;
	else
		*ptr++ = stream_id;

// Packet length
	packet_length = PACKET_SIZE - (ptr - mplex->packet_buffer) - 2;
	*ptr++ = (packet_length & 0xff00) >> 8;
	*ptr++ = packet_length & 0xff;

// Pts/dts flags
	*ptr++ = 0x0f;

// AC3 stream_id
	if(ac3)
	{
		*ptr++ = stream_id;
		*ptr++ = 0x00;
		*ptr++ = 0x00;
		*ptr++ = 0x00;
	}

	return PACKET_SIZE - (ptr - mplex->packet_buffer);
}

int write_packet(track_t *track, 
		float start_time, 
		float end_time, 
		multiplexer_t *mplex,
		int stream_id,
		int ac3)
{
	int result = 0;
	
	long bytes_needed = track->bytes_decoded - ftell(track->raw_file);
	long current_byte = 0;

	while(current_byte < bytes_needed)
	{
// Write packet header
		float current_time = start_time + (float)current_byte / bytes_needed * (end_time - start_time);
		int packet_bytes = write_pack_header(mplex->packet_buffer, 
			mplex, 
			current_time,
			stream_id,
			ac3);
		unsigned char *ptr = mplex->packet_buffer + PACKET_SIZE - packet_bytes;
		/*int bytes_read =*/ fread(ptr, 1, packet_bytes, track->raw_file);

		result = !fwrite(mplex->packet_buffer, PACKET_SIZE, 1, mplex->out_file);
		current_byte += packet_bytes;
	}
	return result;
}

int main(int argc, char *argv[])
{
	int streams = 0;
	int stream = 0;
	int result = 0;
	track_t *atracks[streams + 1];
	track_t *vtracks[streams + 1];
	int total_atracks = 0;
	int total_vtracks = 0;
	float frame_rate = 30000.0 / 1001.0;
	float sample_rate = 48000;
	long frames_decoded = 0;
	float *audio_temp = new float[1];
	long audio_temp_allocated = 0;
	float current_time = 0, previous_time = 0;
	int video_completed = 0;
	int audio_completed = 0;
	int i;
	int ac3 = 0;
	multiplexer_t mplex;
	char **path;
	char *output_path = 0;
	int old_percentage = 0;
	
	path = new char*[argc];
	mplex.derivative = 1;
	mplex.out_file = 0;

	if(argc < 4)
	{
		printf("Tiny MPLEX by Heroine Virtual Ltd.\n");
		printf("Usage: mplex [-a] <stream 1> <stream2> ... <output>\n");
		printf("	-a use ac3 packet headers\n");
		exit(1);
	}

	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-a"))
		{
			ac3 = 1;
		}
		else
		if(i == argc - 1)
		{
			output_path = argv[i];
		}
		else
		{
			path[stream] = new char[strlen(argv[i])+1];
			strcpy(path[stream], argv[i]);
			streams++;
			stream++;
		}
	}

// Open files
	for(stream = 0; stream < streams; stream++)
	{
		int is_audio, is_video;
		int error_return;
		mpeg3_t *file = mpeg3_open(path[stream], &error_return);

		if(!file)
		{
			printf("Couldn't open %s\n", path[stream]);
			result = 1;
		}
		else
		{
			is_audio = mpeg3_has_audio(file);
			is_video = mpeg3_has_video(file);
			mpeg3_set_cpus(file, 2);

			if(is_audio && is_video)
			{
				printf("%s: Can't multiplex a system stream.\n", path[stream]);
				result = 1;
			}
			else
			if(is_audio)
			{
				atracks[total_atracks] = new track_t();
				atracks[total_atracks]->file = file;
				sample_rate = mpeg3_sample_rate(file, 0);
				atracks[total_atracks]->raw_file = fopen(path[stream], "rb");
				atracks[total_atracks]->stream_number = total_atracks;
				total_atracks++;
			}
			else
			if(is_video)
			{
				vtracks[total_vtracks] = new track_t();
				vtracks[total_vtracks]->file = file;
				frame_rate = mpeg3_frame_rate(file, 0);
				vtracks[total_vtracks]->raw_file = fopen(path[stream], "rb");
				vtracks[total_vtracks]->stream_number = total_vtracks;
// Get the first frame
				mpeg3_skip_video_frame(vtracks[total_vtracks]->file,0);
				total_vtracks++;
			}
			else
			{
				printf("%s: Has neither audio or video.\n", path[stream]);
				result = 1;
			}
		}
	}

	if(!result)
	{
		if(!total_vtracks)
		{
			printf("You must supply at least 1 video track.\n");
			result = 1;
		}
	}

// Open output
	if(!result)
		if(!(mplex.out_file = fopen(output_path, "wb")))
		{
			printf("Couldn't open output file\n");
			result = 1;
		}

// Write multiplexed stream
	if(!result)
	{
		while(!result && !(video_completed && audio_completed))
		{
			previous_time = current_time;
// Want the frame to come before the audio that goes with it.
			for(stream = 0; stream < total_vtracks && !result; stream++)
			{
// Decode a frame and write it
				track_t *track = vtracks[stream];

				if(!track->end_of_data)
				{
					int percentage;
					mpeg3_skip_video_frame(track->file,0);
					track->frames_decoded++;
					track->bytes_decoded = mpeg3_video_tell_byte(track->file,0);
					if(track->frames_decoded > frames_decoded)
					{
						frames_decoded = track->frames_decoded;
						current_time = (float)frames_decoded / frame_rate;
					}

					result = write_packet(track, 
									previous_time, 
									current_time, 
									&mplex,
									0xe0 | track->stream_number,
									0);
				    track->end_of_data = mpeg3_end_of_video(track->file, 0);
					percentage = (int)(mpeg3_video_tell_byte(track->file,0) * 100 / 
						mpeg3_get_bytes(track->file));
					if(percentage - old_percentage >= 1)
					{
						printf("\t%d%% completed\r", percentage);
						old_percentage = percentage;
					}
					fflush(stdout);
				}
			}

// Decode audio until the last frame is covered
			for(stream = 0; stream < total_atracks && !result; stream++)
			{
				track_t *track = atracks[stream];
//printf("mplex decode audio 1\n");
				if(!track->end_of_data &&
					(track->samples_decoded < current_time * sample_rate ||
						video_completed))
				{
					if(!video_completed)
					{
						long samples_needed = (long)(current_time * sample_rate) - track->samples_decoded;
						if(audio_temp_allocated < samples_needed)
						{
							delete [] audio_temp;
							audio_temp = new float[samples_needed];
							audio_temp_allocated = samples_needed;
						}

						mpeg3_read_audio(track->file,
							audio_temp,
							0,
							0,
							samples_needed,
							0);
						track->bytes_decoded = mpeg3_audio_tell_byte(track->file,0);
						if(!track->end_of_data) track->bytes_decoded -= 2048;
						track->samples_decoded += samples_needed;
						track->end_of_data = mpeg3_end_of_audio(track->file, 0);
					}
					else
					{
						track->bytes_decoded = mpeg3_get_bytes(track->file);
						track->end_of_data = 1;
					}

//printf("mplex decode audio 2\n");
					result = write_packet(track, 
						previous_time, 
						current_time, 
						&mplex,
						ac3 ? track->stream_number : (0xc0 | track->stream_number),
						ac3);
				}
			}

			for(stream = 0; stream < total_vtracks; stream++)
			{
				if(vtracks[stream]->end_of_data) video_completed++;
			}
			if(video_completed < total_vtracks) video_completed = 0;

			for(stream = 0; stream < total_atracks; stream++)
			{
				if(atracks[stream]->end_of_data) audio_completed++;
			}
			if(audio_completed < total_atracks) audio_completed = 0;
		}
	}

// Close streams
	for(stream = 0; stream < total_atracks; stream++)
	{
		mpeg3_close(atracks[stream]->file);
		fclose(atracks[stream]->raw_file);
		delete atracks[stream];
	}

	for(stream = 0; stream < total_vtracks; stream++)
	{
		mpeg3_close(vtracks[stream]->file);
		fclose(vtracks[stream]->raw_file);
		delete vtracks[stream];
	}
	
	if(mplex.out_file) fclose(mplex.out_file);

	return result;
}


