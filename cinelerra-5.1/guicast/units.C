
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

#include "bcwindowbase.inc"
#include "units.h"
#include "language.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

float* DB::topower_base = 0;
int* Freq::freqtable = 0;


DB::DB(float infinitygain)
{
	this->infinitygain = infinitygain;
	this->db = 0;
	this->topower = topower_base + -INFINITYGAIN * 10;
}


float DB::fromdb_table()
{
	return db = topower[(int)(db*10)];
}

float DB::fromdb_table(float db)
{
	if(db > MAXGAIN) db = MAXGAIN;
	if(db <= INFINITYGAIN) return 0;
	return db = topower[(int)(db*10)];
}

float DB::fromdb()
{
	return pow(10, db/20);
}

float DB::fromdb(float db)
{
	return pow(10, db/20);
}

// set db to the power given using a formula
float DB::todb(float power)
{
	float db;
	if(power > 0) {
		db = 20 * log10(power);
		if(db < -100) db = -100;
	}
	else
		db = -100;
	return db;
}


Freq::Freq()
{
	freq = 0;
}

Freq::Freq(const Freq& oldfreq)
{
	this->freq = oldfreq.freq;
}

int Freq::fromfreq()
{
	int i = 0;
  	while( i<TOTALFREQS && freqtable[i]<freq ) ++i;
  	return i;
};

int Freq::fromfreq(int index)
{
	int i = 0;
  	while( i<TOTALFREQS && freqtable[i]<index ) ++i;
  	return i;
};

int Freq::tofreq(int index)
{
	if(index >= TOTALFREQS) index = TOTALFREQS - 1;
	return freqtable[index];
}

Freq& Freq::operator++()
{
	if(freq < TOTALFREQS) ++freq;
	return *this;
}

Freq& Freq::operator--()
{
	if(freq > 0) --freq;
	return *this;
}

int Freq::operator>(Freq &newfreq) { return freq > newfreq.freq; }
int Freq::operator<(Freq &newfreq) { return freq < newfreq.freq; }
Freq& Freq::operator=(const Freq &newfreq) { freq = newfreq.freq; return *this; }
int Freq::operator=(const int newfreq) { freq = newfreq; return newfreq; }
int Freq::operator!=(Freq &newfreq) { return freq != newfreq.freq; }
int Freq::operator==(Freq &newfreq) { return freq == newfreq.freq; }
int Freq::operator==(int newfreq) { return freq == newfreq; }



void Units::init()
{
	DB::topower_base = new float[(MAXGAIN - INFINITYGAIN) * 10 + 1];
	float *topower = DB::topower_base + -INFINITYGAIN * 10;
	for(int i = INFINITYGAIN * 10; i <= MAXGAIN * 10; i++)
		topower[i] = pow(10, (float)i / 10 / 20);
	topower[INFINITYGAIN * 10] = 0;   // infinity gain

	Freq::freqtable = new int[TOTALFREQS + 1];
// starting frequency
	double freq1 = 27.5, freq2 = 55;
// Some number divisable by three.  This depends on the value of TOTALFREQS
	int scale = 105;

	Freq::freqtable[0] = 0;
	for(int i = 1, j = 0; i <= TOTALFREQS; i++, j++) {
		Freq::freqtable[i] = (int)(freq1 + (freq2 - freq1) / scale * j + 0.5);
		if(j < scale) continue;
		freq1 = freq2;  freq2 *= 2;  j = 0;
	}
}
void Units::finit()
{
	delete [] DB::topower_base;  DB::topower_base = 0;
	delete [] Freq::freqtable;   Freq::freqtable = 0;
}

// give text representation as time
char* Units::totext(char *text, double seconds, int time_format,
			int sample_rate, float frame_rate, float frames_per_foot)
{
	int64_t hour, feet, frame;
	int minute, second, thousandths;

	switch(time_format) {
	case TIME_SECONDS: {
// add 1.0e-6 to prevent round off truncation from glitching a bunch of digits
		seconds = fabs(seconds) + 1.0e-6;
		second = seconds;
		seconds -= (int64_t)seconds;
		thousandths = (int64_t)(seconds*1000) % 1000;
		sprintf(text, "%04d.%03d", second, thousandths);
		break; }

	case TIME_HMS: {
		seconds = fabs(seconds) + 1.0e-6;
		hour = seconds/3600;
		minute = seconds/60 - hour*60;
		second = seconds - (hour*3600 + minute*60);
		seconds -= (int64_t)seconds;
		thousandths = (int64_t)(seconds*1000) % 1000;
		sprintf(text, "%d:%02d:%02d.%03d",
			(int)hour, minute, second, thousandths);
		break; }

	case TIME_HMS2: {
		seconds = fabs(seconds) + 1.0e-6;
		hour = seconds/3600;
		minute = seconds/60 - hour*60;
		second = seconds - (hour*3600 + minute*60);
		sprintf(text, "%d:%02d:%02d", (int)hour, minute, second);
		break; }

	case TIME_HMS3: {
		seconds = fabs(seconds) + 1.0e-6;
		hour = seconds/3600;
		minute = seconds/60 - hour*60;
		second = seconds - (hour*3600 + minute*60);
		sprintf(text, "%02d:%02d:%02d", (int)hour, minute, second);
		break; }

	case TIME_HMSF: {
		seconds = fabs(seconds) + 1.0e-6;
		hour = seconds/3600;
		minute = seconds/60 - hour*60;
		second = seconds - (hour*3600 + minute*60);
		seconds -= (int64_t)seconds;
//		frame = round(frame_rate * (seconds-(int)seconds));
		frame = frame_rate*seconds + 1.0e-6;
		sprintf(text, "%01d:%02d:%02d:%02d", (int)hour, minute, second, (int)frame);
		break; }

	case TIME_SAMPLES: {
		sprintf(text, "%09jd", to_int64(seconds * sample_rate));
		break; }

	case TIME_SAMPLES_HEX: {
		sprintf(text, "%08jx", to_int64(seconds * sample_rate));
		break; }

	case TIME_FRAMES: {
		frame = to_int64(seconds * frame_rate);
		sprintf(text, "%06jd", frame);
		break; }

	case TIME_FEET_FRAMES: {
		frame = to_int64(seconds * frame_rate);
		feet = (int64_t)(frame / frames_per_foot);
		sprintf(text, "%05jd-%02jd", feet,
			(int64_t)(frame - feet * frames_per_foot));
		break; }

	case TIME_MS1: {
		seconds = fabs(seconds) + 1.0e-6;
		minute = seconds/60;
		second = seconds - minute*60;
		sprintf(text, "%d:%02d", minute, second);
		break; }

	case TIME_MS2: {
		int sign = seconds >= 0 ? '+' : '-';
		seconds = fabs(seconds) + 1.0e-6;
		minute = seconds/60;
		second = seconds - minute*60;
		sprintf(text, "%c%d:%02d", sign, minute, second);
		break; }
	default: {
		*text = 0;
		break; }
	}
	return text;
}


// give text representation as time
char* Units::totext(char *text, int64_t samples, int samplerate,
		int time_format, float frame_rate, float frames_per_foot)
{
	return totext(text, (double)samples/samplerate, time_format,
			samplerate, frame_rate, frames_per_foot);
}

int64_t Units::get_int64(const char *&bp)
{
	char string[BCTEXTLEN], *sp=&string[0];
	const char *cp = bp;
	for( int j=0; j<10 && isdigit(*cp); ++j ) *sp++ = *cp++;
	*sp = 0;  bp = cp;
	return atol(string);
}

double Units::get_double(const char *&bp)
{
	char string[BCTEXTLEN], *sp=&string[0];
	const char *cp = bp;
	for( int j=0; j<10 && isdigit(*cp); ++j ) *sp++ = *cp++;
	if( *cp == '.' ) {
		*sp++ = *cp++;
		for( int j=0; j<10 && isdigit(*cp); ++j ) *sp++ = *cp++;
	}
	*sp = 0;  bp = cp;
	return strtod(string,0);
}

void Units::skip_seperators(const char *&bp)
{
	const char *cp = bp;
	for( int j=0; j<10 && *cp && !isdigit(*cp); ++j ) ++cp;
	bp = cp;
}

int64_t Units::fromtext(const char *text, int samplerate, int time_format,
			float frame_rate, float frames_per_foot)
{
	int64_t hours, total_samples;
	int minutes, frames, feet;
	double seconds, total_seconds;
	int sign = 1;

	switch(time_format) {
	case TIME_SECONDS: {
		total_seconds = get_double(text);
		break; }

	case TIME_HMS:
	case TIME_HMS2:
	case TIME_HMS3: {
		hours = get_int64(text);    skip_seperators(text);
		minutes = get_int64(text);  skip_seperators(text);
		seconds = get_int64(text);
		total_seconds = seconds + minutes*60 + hours*3600;
		break; }

	case TIME_HMSF: {
		hours = get_int64(text);    skip_seperators(text);
		minutes = get_int64(text);  skip_seperators(text);
		seconds = get_int64(text);  skip_seperators(text);
		frames = get_int64(text);
		total_seconds = frames/frame_rate + seconds + minutes*60 + hours*3600;
		break; }

	case TIME_SAMPLES: {
		return get_int64(text); }

	case TIME_SAMPLES_HEX: {
		sscanf(text, "%jx", &total_samples);
		return total_samples; }

	case TIME_FRAMES: {
		total_seconds = get_double(text) / frame_rate;
		break; }

	case TIME_FEET_FRAMES: {
		feet = get_int64(text);    skip_seperators(text);
		frames = get_int64(text);
		total_seconds = (feet*frames_per_foot + frames) / frame_rate;
		break; }

	case TIME_MS2: {
		switch( *text ) {
		case '+':  sign = 1;   ++text;  break;
		case '-':  sign = -1;  ++text;  break;
		} } // fall through
	case TIME_MS1: {
		minutes = get_int64(text);   skip_seperators(text);
		seconds = get_double(text);
		total_seconds = sign * (seconds + minutes*60);
		break; }
	default: {
		total_seconds = 0;
		break; }
	}

	total_samples = total_seconds * samplerate;
	return total_samples;
}

double Units::text_to_seconds(const char *text, int samplerate, int time_format,
				float frame_rate, float frames_per_foot)
{
	return (double)fromtext(text, samplerate, time_format,
				frame_rate, frames_per_foot) / samplerate;
}




int Units::timeformat_totype(char *tcf)
{
	if (!strcmp(tcf,TIME_SECONDS__STR)) return(TIME_SECONDS);
	if (!strcmp(tcf,TIME_HMS__STR)) return(TIME_HMS);
	if (!strcmp(tcf,TIME_HMS2__STR)) return(TIME_HMS2);
	if (!strcmp(tcf,TIME_HMS3__STR)) return(TIME_HMS3);
	if (!strcmp(tcf,TIME_HMSF__STR)) return(TIME_HMSF);
	if (!strcmp(tcf,TIME_SAMPLES__STR)) return(TIME_SAMPLES);
	if (!strcmp(tcf,TIME_SAMPLES_HEX__STR)) return(TIME_SAMPLES_HEX);
	if (!strcmp(tcf,TIME_FRAMES__STR)) return(TIME_FRAMES);
	if (!strcmp(tcf,TIME_FEET_FRAMES__STR)) return(TIME_FEET_FRAMES);
	return(-1);
}


float Units::toframes(int64_t samples, int sample_rate, float framerate)
{
	return (double)samples/sample_rate * framerate;
} // give position in frames

int64_t Units::toframes_round(int64_t samples, int sample_rate, float framerate)
{
// used in editing
	float result_f = (float)samples / sample_rate * framerate;
	int64_t result_l = (int64_t)(result_f + 0.5);
	return result_l;
}

double Units::fix_framerate(double value)
{
	if(value > 29.5 && value < 30)
		value = (double)30000 / (double)1001;
	else if(value > 59.5 && value < 60)
		value = (double)60000 / (double)1001;
	else if(value > 23.5 && value < 24)
		value = (double)24000 / (double)1001;
	return value;
}

double Units::atoframerate(const char *text)
{
	double result = get_double(text);
	return fix_framerate(result);
}


int64_t Units::tosamples(double frames, int sample_rate, float framerate)
{
	double result = frames/framerate * sample_rate;
	if(result - (int)result >= 1.0e-6 ) result += 1;
	return (int64_t)result;
} // give position in samples


float Units::xy_to_polar(int x, int y)
{
	float angle = 0.;
	if(x > 0 && y <= 0)
		angle = atan((float)-y / x) / (2*M_PI) * 360;
	else if(x < 0 && y <= 0)
		angle = 180 - atan((float)-y / -x) / (2*M_PI) * 360;
	else if(x < 0 && y > 0)
		angle = 180 - atan((float)-y / -x) / (2*M_PI) * 360;
	else if(x > 0 && y > 0)
		angle = 360 + atan((float)-y / x) / (2*M_PI) * 360;
	else if(x == 0 && y < 0)
		angle = 90;
	else if(x == 0 && y > 0)
		angle = 270;
	else if(x == 0 && y == 0)
		angle = 0;
	return angle;
}

void Units::polar_to_xy(float angle, int radius, int &x, int &y)
{
	if( angle < 0 )
		angle += ((int)(-angle)/360 + 1) * 360;
	else if( angle >= 360 )
		angle -= ((int)(angle)/360) * 360;

	x = (int)(cos(angle / 360 * (2*M_PI)) * radius);
	y = (int)(-sin(angle / 360 * (2*M_PI)) * radius);
}

int64_t Units::round(double result)
{
	return (int64_t)(result < 0 ? result - 0.5 : result + 0.5);
}

float Units::quantize10(float value)
{
	int64_t temp = (int64_t)(value*10 + 0.5);
	return temp/10.;
}

float Units::quantize(float value, float precision)
{
	int64_t temp = (int64_t)(value/precision + 0.5);
	return temp*precision;
}

int64_t Units::to_int64(double result)
{
// This must round up if result is one sample within ceiling.
// Sampling rates below 48000 may cause more problems.
	return (int64_t)(result < 0 ? (result - 0.005) : (result + 0.005));
}

const char* Units::print_time_format(int time_format, char *string)
{
	const char *fmt = "Unknown";
	switch(time_format) {
	case TIME_HMS:         fmt = TIME_HMS_TEXT;                   break;
	case TIME_HMSF:        fmt = TIME_HMSF_TEXT;                  break;
	case TIME_SAMPLES:     fmt = TIME_SAMPLES_TEXT;               break;
	case TIME_SAMPLES_HEX: fmt = TIME_SAMPLES_HEX_TEXT;           break;
	case TIME_FRAMES:      fmt = TIME_FRAMES_TEXT;                break;
	case TIME_FEET_FRAMES: fmt = TIME_FEET_FRAMES_TEXT;           break;
	case TIME_HMS2:
	case TIME_HMS3:        fmt = TIME_HMS3_TEXT;                  break;
	case TIME_SECONDS:     fmt = TIME_SECONDS_TEXT;               break;
	case TIME_MS1:
	case TIME_MS2:         fmt = TIME_MS2_TEXT;                   break;
	}
	return strcpy(string,fmt);
}

int Units::text_to_format(const char *string)
{
	if(!strcmp(string, TIME_HMS_TEXT)) return TIME_HMS;
	if(!strcmp(string, TIME_HMSF_TEXT)) return TIME_HMSF;
	if(!strcmp(string, TIME_SAMPLES_TEXT)) return TIME_SAMPLES;
	if(!strcmp(string, TIME_SAMPLES_HEX_TEXT)) return TIME_SAMPLES_HEX;
	if(!strcmp(string, TIME_FRAMES_TEXT)) return TIME_FRAMES;
	if(!strcmp(string, TIME_FEET_FRAMES_TEXT)) return TIME_FEET_FRAMES;
	if(!strcmp(string, TIME_HMS3_TEXT)) return TIME_HMS3;
	if(!strcmp(string, TIME_SECONDS_TEXT)) return TIME_SECONDS;
	if(!strcmp(string, TIME_MS2_TEXT)) return TIME_MS2;
	return TIME_HMS;
}

char* Units::size_totext(int64_t bytes, char *text)
{
	char string[BCTEXTLEN];
	static const char *sz[] = { "bytes", "KB", "MB", "GB", "TB" };

	int i = (sizeof(sz) / sizeof(sz[0]));
	while( --i > 0 && bytes < ((int64_t)1 << (10*i)) );

	if( i > 0 ) {
		bytes >>= 10*(i-1);
		int frac = bytes % 1000;
		sprintf(string, "%jd", bytes/1000);
		if( bytes > 1000 ) punctuate(string);
		sprintf(text, "%s.%03d %s", string, frac, sz[i]);
	}
	else {
		sprintf(string, "%jd", bytes);
		if( bytes > 1000 ) punctuate(string);
		sprintf(text, "%s %s", string, sz[i]);
	}
	return text;
}


#undef BYTE_ORDER
#define BYTE_ORDER ((*(const uint32_t*)"a   ") & 0x00000001)

void* Units::int64_to_ptr(uint64_t value)
{
	unsigned char *value_dissected = (unsigned char*)&value;
	void *result;
	unsigned char *data = (unsigned char*)&result;

// Must be done behind the compiler's back
	if(sizeof(void*) == 4) {
		if(!BYTE_ORDER) {
			data[0] = value_dissected[4];
			data[1] = value_dissected[5];
			data[2] = value_dissected[6];
			data[3] = value_dissected[7];
		}
		else {
			data[0] = value_dissected[0];
			data[1] = value_dissected[1];
			data[2] = value_dissected[2];
			data[3] = value_dissected[3];
		}
	}
	else {
		data[0] = value_dissected[0];
		data[1] = value_dissected[1];
		data[2] = value_dissected[2];
		data[3] = value_dissected[3];
		data[4] = value_dissected[4];
		data[5] = value_dissected[5];
		data[6] = value_dissected[6];
		data[7] = value_dissected[7];
	}
	return result;
}

uint64_t Units::ptr_to_int64(void *ptr)
{
	unsigned char *ptr_dissected = (unsigned char*)&ptr;
	int64_t result = 0;
	unsigned char *data = (unsigned char*)&result;
// Don't do this at home.
	if(sizeof(void*) == 4) {
		if(!BYTE_ORDER) {
			data[4] = ptr_dissected[0];
			data[5] = ptr_dissected[1];
			data[6] = ptr_dissected[2];
			data[7] = ptr_dissected[3];
		}
		else {
			data[0] = ptr_dissected[0];
			data[1] = ptr_dissected[1];
			data[2] = ptr_dissected[2];
			data[3] = ptr_dissected[3];
		}
	}
	else {
		data[0] = ptr_dissected[0];
		data[1] = ptr_dissected[1];
		data[2] = ptr_dissected[2];
		data[3] = ptr_dissected[3];
		data[4] = ptr_dissected[4];
		data[5] = ptr_dissected[5];
		data[6] = ptr_dissected[6];
		data[7] = ptr_dissected[7];
	}
	return result;
}

const char* Units::format_to_separators(int time_format)
{
	switch(time_format) {
		case TIME_SECONDS:     return "0000.000";
		case TIME_HMS:         return "0:00:00.000";
		case TIME_HMS2:        return "0:00:00";
		case TIME_HMS3:        return "00:00:00";
		case TIME_HMSF:        return "0:00:00:00";
		case TIME_SAMPLES:     return 0;
		case TIME_SAMPLES_HEX: return 0;
		case TIME_FRAMES:      return 0;
		case TIME_FEET_FRAMES: return "00000-00";
		case TIME_MS1:         return "0:00";
		case TIME_MS2:         return "+0:00";
	}
	return 0;
}

void Units::punctuate(char *string)
{
	int sep = ',', len = strlen(string), commas = (len - 1) / 3;
	char *cp = string + len, *bp = cp + commas;
	*bp = 0;
	for( int k=3; cp < bp && bp > string; ) {
		*--bp = *--cp;
		if( --k > 0 ) continue;
		*--bp = sep;  k = 3;
	}
}

void Units::fix_double(double *x)
{
	*x = *x;
}


