
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

#include "audiodevice.h"
#include "audioalsa.h"
#include "bcsignals.h"
#include "language.h"
#include "mutex.h"
#include "playbackconfig.h"
#include "preferences.h"
#include "recordconfig.h"

#include <errno.h>

#ifdef HAVE_ALSA

AudioALSA::AudioALSA(AudioDevice *device)
 : AudioLowLevel(device)
{
	buffer_position = 0;
	samples_written = 0;
	timer = new Timer;
	delay = 0;
	period_size = 0;
	timer_lock = new Mutex("AudioALSA::timer_lock");
	interrupted = 0;
	dsp_in = 0;
	dsp_out = 0;
}

AudioALSA::~AudioALSA()
{
	delete timer_lock;
	delete timer;
}

// leak checking
static class alsa_leaks
{
public:
// This is required in the top thread for Alsa to work
	alsa_leaks() {
		ArrayList<char*> *alsa_titles = new ArrayList<char*>;
		alsa_titles->set_array_delete();
		AudioALSA::list_devices(alsa_titles, 0, MODEPLAY);
		alsa_titles->remove_all_objects();
		delete alsa_titles;
	}
	~alsa_leaks() { snd_config_update_free_global(); }
} alsa_leak;

void AudioALSA::list_devices(ArrayList<char*> *devices, int pcm_title, int mode)
{
	snd_ctl_t *handle;
	int card, err, dev;
	snd_ctl_card_info_t *info;
	snd_pcm_info_t *pcminfo;
	char string[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;

	switch(mode)
	{
		case MODERECORD:
			stream = SND_PCM_STREAM_CAPTURE;
			break;
		case MODEPLAY:
			stream = SND_PCM_STREAM_PLAYBACK;
			break;
	}


	snd_ctl_card_info_alloca(&info);
	snd_pcm_info_alloca(&pcminfo);

	card = -1;
#define DEFAULT_DEVICE "default"
	char *result = new char[strlen(DEFAULT_DEVICE) + 1];
	devices->append(result);
	strcpy(result, DEFAULT_DEVICE);

	while(snd_card_next(&card) >= 0)
	{
		char name[BCTEXTLEN];
		if(card < 0) break;
		sprintf(name, "hw:%i", card);

		if((err = snd_ctl_open(&handle, name, 0)) < 0)
		{
			printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
			continue;
		}

		if((err = snd_ctl_card_info(handle, info)) < 0)
		{
			printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
			snd_ctl_close(handle);
			continue;
		}

		dev = -1;

		while(1)
		{
			if(snd_ctl_pcm_next_device(handle, &dev) < 0)
				printf("AudioALSA::list_devices: snd_ctl_pcm_next_device\n");

			if (dev < 0)
				break;

			snd_pcm_info_set_device(pcminfo, dev);
			snd_pcm_info_set_subdevice(pcminfo, 0);
			snd_pcm_info_set_stream(pcminfo, stream);

			if((err = snd_ctl_pcm_info(handle, pcminfo)) < 0)
			{
				if(err != -ENOENT)
					printf("AudioALSA::list_devices card=%i: %s\n", card, snd_strerror(err));
				continue;
			}

			if(pcm_title)
			{
				sprintf(string, "plughw:%d,%d", card, dev);
//				strcpy(string, "cards.pcm.front");
			}
			else
			{
				sprintf(string, "%s #%d",
					snd_ctl_card_info_get_name(info),
					dev);
			}

			char *result = devices->append(new char[strlen(string) + 1]);
			strcpy(result, string);
		}



		snd_ctl_close(handle);
	}

#ifdef HAVE_PACTL
// attempt to add pulseaudio "monitor" devices
//  run: pactl list <sources>|<sinks>
//   scan output for <Source/Sink> #n,  Name: <device>
//   build alsa device config and add to alsa snd_config

	const char *arg = 0;
	switch( mode ) {
		case MODERECORD:
			arg = "source";
			break;
		case MODEPLAY:
			arg = "sink";
			break;
	}
	FILE *pactl = 0;
	char line[BCTEXTLEN];
	if( arg ) {
		sprintf(line, "pactl list %ss", arg);
		pactl = popen(line,"r");
	}
	if( pactl ) {
		if( pcm_title ) snd_config_update();
		char name[BCTEXTLEN], pa_name[BCTEXTLEN], device[BCTEXTLEN];
		name[0] = pa_name[0] = device[0] = 0;
		int arg_len = strlen(arg);
		while( fgets(line, sizeof(line), pactl) ) {
			if( !strncasecmp(line, arg, arg_len) ) {
				char *sp = name, *id = pa_name;
				for( char *cp=line; *cp && *cp!='\n'; *sp++=*cp++ )
					*id++ = (*cp>='A' && *cp<='Z') ||
						(*cp>='a' && *cp<='z') ||
						(*cp>='0' && *cp<='9') ? *cp : '_';
				*sp++ = 0;  *id = 0;
				if( !pcm_title )
					devices->append(strcpy(new char[sp-name], name));
				continue;
			}
			if( !pcm_title ) continue;
			if( sscanf(line, " Name: %s", device) != 1 ) continue;
			int len = strlen(pa_name);
			devices->append(strcpy(new char[len+1], pa_name));
			char alsa_config[BCTEXTLEN];
			len = snprintf(alsa_config, sizeof(alsa_config),
				"pcm.!%s {\n type pulse\n device %s\n}\n"
				"ctl.!%s {\n type pulse\n device %s\n}\n",
				pa_name, device, pa_name, device);
			FILE *fp = fmemopen(alsa_config,len,"r");
			snd_input_t *inp;
			snd_input_stdio_attach(&inp, fp, 1);
			snd_config_load(snd_config, inp);
			name[0] = pa_name[0] = device[0] = 0;
			snd_input_close(inp);
		}
		pclose(pactl);
	}
#endif
}

void AudioALSA::translate_name(char *output, char *input, int mode)
{
	ArrayList<char*> titles;
	titles.set_array_delete();

	ArrayList<char*> pcm_titles;
	pcm_titles.set_array_delete();

	list_devices(&titles, 0, mode);
	list_devices(&pcm_titles, 1, mode);

	sprintf(output, "default");
	for(int i = 0; i < titles.total; i++)
	{
//printf("AudioALSA::translate_name %s %s\n", titles.values[i], pcm_titles.values[i]);
		if(!strcasecmp(titles.values[i], input))
		{
			strcpy(output, pcm_titles.values[i]);
			break;
		}
	}

	titles.remove_all_objects();
	pcm_titles.remove_all_objects();
}

snd_pcm_format_t AudioALSA::translate_format(int format)
{
	switch(format)
	{
	case 8:
		return SND_PCM_FORMAT_S8;
		break;
	case 16:
		return SND_PCM_FORMAT_S16_LE;
		break;
	case 24:
		return SND_PCM_FORMAT_S24_LE;
		break;
	case 32:
		return SND_PCM_FORMAT_S32_LE;
		break;
	}
	return SND_PCM_FORMAT_UNKNOWN;
}

int AudioALSA::set_params(snd_pcm_t *dsp, int mode,
	int channels, int bits, int samplerate, int samples)
{
	snd_pcm_hw_params_t *params;
	snd_pcm_sw_params_t *swparams;
	int err;

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_sw_params_alloca(&swparams);
	err = snd_pcm_hw_params_any(dsp, params);

	if (err < 0) {
		fprintf(stderr, "AudioALSA::set_params: ");
		fprintf(stderr, _("no PCM configurations available\n"));
		return 1;
	}

	err=snd_pcm_hw_params_set_access(dsp,
		params,
		SND_PCM_ACCESS_RW_INTERLEAVED);
        if(err) {
		fprintf(stderr, "AudioALSA::set_params: ");
		fprintf(stderr, _("failed to set up interleaved device access.\n"));
		return 1;
        }

	err=snd_pcm_hw_params_set_format(dsp,
		params,
		translate_format(bits));
        if(err) {
		fprintf(stderr, "AudioALSA::set_params: ");
		fprintf(stderr, _("failed to set output format.\n"));
		return 1;
        }

	err=snd_pcm_hw_params_set_channels(dsp,
		params,
		channels);
        if(err) {
		fprintf(stderr, "AudioALSA::set_params: ");
		fprintf(stderr, _("Configured ALSA device does not support %d channel operation.\n"),
			channels);
		return 1;
        }

	err=snd_pcm_hw_params_set_rate_near(dsp,
		params,
		(unsigned int*)&samplerate,
		(int*)0);
        if(err) {
		fprintf(stderr, "AudioALSA::set_params: ");
		fprintf(stderr, _(" Configured ALSA device does not support %u Hz playback.\n"),
			(unsigned int)samplerate);
		return 1;
        }

// Buffers written must be equal to period_time
	int buffer_time = 0;
	int period_time = (int)(1000000 * (double)samples / samplerate);
	switch( mode ) {
	case MODERECORD:
		buffer_time = 10000000;
		break;
	case MODEPLAY:
		buffer_time = 2 * period_time;
		break;
	}

//printf("AudioALSA::set_params 1 %d %d %d\n", samples, buffer_time, period_time);
	snd_pcm_hw_params_set_buffer_time_near(dsp,
		params,
		(unsigned int*)&buffer_time,
		(int*)0);
	snd_pcm_hw_params_set_period_time_near(dsp,
		params,
		(unsigned int*)&period_time,
		(int*)0);
//printf("AudioALSA::set_params 5 %d %d\n", buffer_time, period_time);
	err = snd_pcm_hw_params(dsp, params);
	if(err < 0) {
		fprintf(stderr, "AudioALSA::set_params: hw_params failed\n");
		return 1;
	}

	snd_pcm_uframes_t chunk_size = 1024;
	snd_pcm_uframes_t buffer_size = 262144;
	snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
	snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
//printf("AudioALSA::set_params 10 %d %d\n", chunk_size, buffer_size);

	snd_pcm_sw_params_current(dsp, swparams);
	//snd_pcm_uframes_t xfer_align = 1;
	//snd_pcm_sw_params_get_xfer_align(swparams, &xfer_align);
	//unsigned int sleep_min = 0;
	//err = snd_pcm_sw_params_set_sleep_min(dsp, swparams, sleep_min);
	period_size = chunk_size;
	err = snd_pcm_sw_params_set_avail_min(dsp, swparams, period_size);
	//err = snd_pcm_sw_params_set_xfer_align(dsp, swparams, xfer_align);
	if(snd_pcm_sw_params(dsp, swparams) < 0) {
                /* we can continue staggering along even if this fails */
		fprintf(stderr, "AudioALSA::set_params: snd_pcm_sw_params failed\n");
	}

	device->device_buffer = samples * bits / 8 * channels;
	period_size /= 2;
//printf("AudioALSA::set_params 100 %d %d\n", samples,  device->device_buffer);

//	snd_pcm_hw_params_free(params);
//	snd_pcm_sw_params_free(swparams);
	return 0;
}

int AudioALSA::open_input()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_CAPTURE;
	int open_mode = 0;
	int err;

	device->in_channels = device->get_ichannels();
	device->in_bits = device->in_config->alsa_in_bits;

	translate_name(pcm_name, device->in_config->alsa_in_device,MODERECORD);
//printf("AudioALSA::open_input %s\n", pcm_name);

	err = snd_pcm_open(&dsp_in, pcm_name, stream, open_mode);

	if(err < 0) {
		dsp_in = 0;
		printf("AudioALSA::open_input: %s\n", snd_strerror(err));
		return 1;
	}

	err = set_params(dsp_in, MODERECORD,
		device->get_ichannels(),
		device->in_config->alsa_in_bits,
		device->in_samplerate,
		device->in_samples);
	if(err) {
		fprintf(stderr, "AudioALSA::open_input: set_params failed.  Aborting sampling.\n");
		close_input();
		return 1;
	}

	return 0;
}

int AudioALSA::open_output()
{
	char pcm_name[BCTEXTLEN];
	snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
	int open_mode = SND_PCM_NONBLOCK;
	int err;

	device->out_channels = device->get_ochannels();
	device->out_bits = device->out_config->alsa_out_bits;

//printf("AudioALSA::open_output out_device %s\n", device->out_config->alsa_out_device);
	translate_name(pcm_name, device->out_config->alsa_out_device,MODEPLAY);
//printf("AudioALSA::open_output pcm_name %s\n", pcm_name);

	err = snd_pcm_open(&dsp_out, pcm_name, stream, open_mode);

	if(err < 0)
	{
		dsp_out = 0;
		printf("AudioALSA::open_output %s: %s\n", pcm_name, snd_strerror(err));
		return 1;
	}

	err = set_params(dsp_out, MODEPLAY,
		device->get_ochannels(),
		device->out_config->alsa_out_bits,
		device->out_samplerate,
		device->out_samples);
	if(err) {
		fprintf(stderr, "AudioALSA::open_output: set_params failed.  Aborting playback.\n");
		close_output();
		return 1;
	}

	timer->update();
	return 0;
}

int AudioALSA::stop_output()
{
//printf("AudioALSA::stop_output\n");
	if(!device->out_config->interrupt_workaround)
	{
		if( get_output() )
			snd_pcm_drop(get_output());
	}
	else
		flush_device();
	return 0;
}

int AudioALSA::close_output()
{
//printf("AudioALSA::close_output\n");
	if(device->w && dsp_out) {
		stop_output();
		snd_pcm_close(dsp_out);
		dsp_out = 0;
	}
	return 0;
}

int AudioALSA::close_input()
{
//printf("AudioALSA::close_input\n");
	if(device->r && dsp_in) {
//		snd_pcm_reset(dsp_in);
		snd_pcm_drop(dsp_in);
		snd_pcm_drain(dsp_in);
		snd_pcm_close(dsp_in);
		dsp_in = 0;
	}
	return 0;
}

int AudioALSA::close_all()
{
//printf("AudioALSA::close_all\n");
	close_input();
	close_output();
	buffer_position = 0;
	samples_written = 0;
	delay = 0;
	interrupted = 0;
	return 0;
}

// Undocumented
int64_t AudioALSA::device_position()
{
	timer_lock->lock("AudioALSA::device_position");
	int64_t delta = timer->get_scaled_difference(device->out_samplerate);
	int64_t result = buffer_position - delay + delta;
//printf("AudioALSA::device_position 1 w=%jd dt=%jd dly=%d pos=%jd\n",
// buffer_position, delta, delay, result);
	timer_lock->unlock();
	return result;
}

int AudioALSA::read_buffer(char *buffer, int size)
{
//printf("AudioALSA::read_buffer 1\n");
	int attempts = 0;
	int done = 0;
	int frame_size = (device->in_bits / 8) * device->get_ichannels();
	int result = 0;

	if(!get_input())
	{
		sleep(1);
		return 0;
	}

	while(attempts < 1 && !done)
	{
		snd_pcm_uframes_t frames = size / frame_size;
		result = snd_pcm_readi(get_input(), buffer, frames);
		if( result < 0)
		{
			printf("AudioALSA::read_buffer overrun at sample %jd\n",
				device->total_samples_read);
//			snd_pcm_resume(get_input());
			close_input();  open_input();
			attempts++;
		}
		else
			done = 1;
//printf("AudioALSA::read_buffer %d result=%d done=%d\n", __LINE__, result, done);
	}
	return 0;
}

int AudioALSA::write_buffer(char *buffer, int size)
{
//printf("AudioALSA::write_buffer %d\n",size);
// Don't give up and drop the buffer on the first error.
	int attempts = 0;
	int done = 0;
	int sample_size = (device->out_bits / 8) * device->get_ochannels();
	int samples = size / sample_size;
//printf("AudioALSA::write_buffer %d\n",samples);
	int count = samples;
 	snd_pcm_sframes_t delay = 0;

// static FILE *debug_fd = 0;
// if(!debug_fd)
// {
// 	debug_fd = fopen("/tmp/debug.pcm", "w");
// }
// fwrite(buffer, size, 1, debug_fd);
// fflush(debug_fd);


	if(!get_output()) return 0;
	if( buffer_position == 0 )
		timer->update();

	AudioThread *audio_out = device->audio_out;
	while( count > 0 && attempts < 2 && !done ) {
		if( device->playback_interrupted ) break;
// Buffers written must be equal to period_time
		audio_out->Thread::enable_cancel();
		int ret = snd_pcm_avail_update(get_output());
		if( ret >= period_size ) {
			if( ret > count ) ret = count;
//FILE *alsa_fp = 0;
//if( !alsa_fp ) alsa_fp = fopen("/tmp/alsa.raw","w");
//if( alsa_fp ) fwrite(buffer, sample_size, ret, alsa_fp);
//printf("AudioALSA::snd_pcm_writei start %d\n",count);
			ret = snd_pcm_writei(get_output(),buffer,ret);
//printf("AudioALSA::snd_pcm_writei done %d\n", ret);
		}
		else if( ret >= 0 || ret == -EAGAIN ) {
			ret = snd_pcm_wait(get_output(),15);
			if( ret > 0 ) ret = 0;
		}
		audio_out->Thread::disable_cancel();
		if( device->playback_interrupted ) break;
		if( ret == 0 ) continue;

		if( ret > 0 ) {
			samples_written += ret;
			if( (count-=ret) > 0 ) {
				buffer += ret * sample_size;
				attempts = 0;
			}
			else
				done = 1;
		}
		else {
			printf("AudioALSA::write_buffer err %d(%s) at sample %jd\n",
				ret, snd_strerror(ret), device->current_position());
			Timer::delay(50);
//			snd_pcm_resume(get_output());
			snd_pcm_recover(get_output(), ret, 1);
//			close_output();  open_output();
			attempts++;
		}
	}

	if( !interrupted && device->playback_interrupted )
	{
		interrupted = 1;
//printf("AudioALSA::write_buffer interrupted\n");
		stop_output();
	}

	if(done)
	{
		timer_lock->lock("AudioALSA::write_buffer");
		snd_pcm_delay(get_output(), &delay);
		this->delay = delay;
		timer->update();
		buffer_position += samples;
//printf("AudioALSA::write_buffer ** wrote %d, delay %d\n",samples,(int)delay);
		timer_lock->unlock();
	}
	return 0;
}

//this delay seems to prevent a problem where the sound system outputs
//a lot of silence while waiting for the device drain to happen.
int AudioALSA::output_wait()
{
	snd_pcm_sframes_t delay = 0;
	snd_pcm_delay(get_output(), &delay);
	if( delay <= 0 ) return 0;
	int64_t udelay = 1e6 * delay / device->out_samplerate;
	// don't allow more than 10 seconds
	if( udelay > 10000000 ) udelay = 10000000;
	while( udelay > 0 && !device->playback_interrupted ) {
		int64_t usecs = udelay;
		if( usecs > 100000 ) usecs = 100000;
		usleep(usecs);
		udelay -= usecs;
	}
	if( device->playback_interrupted &&
	    !device->out_config->interrupt_workaround )
		snd_pcm_drop(get_output());
	return 0;
}

int AudioALSA::flush_device()
{
//printf("AudioALSA::flush_device\n");
	if(get_output())
	{
		output_wait();
		//this causes the output to stutter.
		//snd_pcm_nonblock(get_output(), 0);
		snd_pcm_drain(get_output());
		//snd_pcm_nonblock(get_output(), 1);
	}
	return 0;
}

int AudioALSA::interrupt_playback()
{
//printf("AudioALSA::interrupt_playback *********\n");
//	if(get_output())
//	{
// Interrupts the playback but may not have caused snd_pcm_writei to exit.
// With some soundcards it causes snd_pcm_writei to freeze for a few seconds.
//		if(!device->out_config->interrupt_workaround)
//			snd_pcm_drop(get_output());

// Makes sure the current buffer finishes before stopping.
//		snd_pcm_drain(get_output());

// The only way to ensure snd_pcm_writei exits, but
// got a lot of crashes when doing this.
//		device->Thread::cancel();
//	}
	return 0;
}


snd_pcm_t* AudioALSA::get_output()
{
	return dsp_out;
}

snd_pcm_t* AudioALSA::get_input()
{
	return dsp_in;
}

#endif
