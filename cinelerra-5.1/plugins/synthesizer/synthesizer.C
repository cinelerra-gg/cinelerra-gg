
/*
 * CINELERRA
 * Copyright (C) 1997-2011 Adam Williams <broadcast at earthling dot net>
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "samples.h"
#include "synthesizer.h"
#include "theme.h"
#include "transportque.inc"
#include "vframe.h"

#include <string.h>




REGISTER_PLUGIN(Synth)




Synth::Synth(PluginServer *server)
 : PluginAClient(server)
{
	reset();
	window_w = 640;
	window_h = 480;
}



Synth::~Synth()
{

}

NEW_WINDOW_MACRO(Synth, SynthWindow);

const char* Synth::plugin_title() { return N_("Synthesizer"); }
int Synth::is_realtime() { return 1; }
int Synth::is_synthesis() { return 1; }


void Synth::reset()
{
	need_reconfigure = 1;
}




LOAD_CONFIGURATION_MACRO(Synth, SynthConfig)






void Synth::read_data(KeyFrame *keyframe)
{
	FileXML input;
	char string[BCTEXTLEN];
// cause htal file to read directly from text
	input.set_shared_input(keyframe->xbuf);

//printf("Synth::read_data %s\n", keyframe->get_data());
	int result = 0, current_osc = 0;
	//int total_oscillators = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SYNTH"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);

				if(is_defaults())
				{
					window_w = input.tag.get_property("WINDOW_W", window_w);
					window_h = input.tag.get_property("WINDOW_H", window_h);
				}
				config.momentary_notes = input.tag.get_property("MOMENTARY_NOTES", config.momentary_notes);

//printf("Synth::read_data %d %d %d\n", __LINE__, window_w, window_h);
				for(int i = 0; i < MAX_FREQS; i++)
				{
					sprintf(string, "BASEFREQ_%d", i);
					config.base_freq[i] = input.tag.get_property(string, (double)0);
				}

				config.wavefunction = input.tag.get_property("WAVEFUNCTION", config.wavefunction);
				//total_oscillators = input.tag.get_property("OSCILLATORS", 0);
			}
			else
			if(input.tag.title_is("OSCILLATOR"))
			{
				if(current_osc >= config.oscillator_config.total)
					config.oscillator_config.append(new SynthOscillatorConfig(current_osc));

				config.oscillator_config.values[current_osc]->read_data(&input);
				current_osc++;
			}
		}
	}

	while(config.oscillator_config.total > current_osc)
		config.oscillator_config.remove_object();
}

void Synth::save_data(KeyFrame *keyframe)
{
	FileXML output;
	char string[BCTEXTLEN];

// cause htal file to store data directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SYNTH");
	output.tag.set_property("WETNESS", config.wetness);
	output.tag.set_property("WINDOW_W", window_w);
	output.tag.set_property("WINDOW_H", window_h);
	output.tag.set_property("MOMENTARY_NOTES", config.momentary_notes);

	for(int i = 0; i < MAX_FREQS; i++)
	{
//		if(!EQUIV(config.base_freq[i], 0))
		{
			sprintf(string, "BASEFREQ_%d", i);
			output.tag.set_property(string, config.base_freq[i]);
//printf("Synth::save_data %d %s\n", __LINE__, string);
		}
	}

	output.tag.set_property("WAVEFUNCTION", config.wavefunction);
	output.tag.set_property("OSCILLATORS", config.oscillator_config.total);
	output.append_tag();
	output.append_newline();

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		config.oscillator_config.values[i]->save_data(&output);
	}

	output.tag.set_title("/SYNTH");
	output.append_tag();
	output.append_newline();
	output.terminate_string();
//printf("Synth::save_data %d %s\n", __LINE__, output.string);
// data is now in *text
}



void Synth::update_gui()
{
	if( !thread ) return;
	SynthWindow *window = (SynthWindow*)thread->window;
// load_configuration,read_data deletes oscillator_config
	window->lock_window("Synth::update_gui");
	if( load_configuration() )
		window->update_gui();
	window->unlock_window();
}


void Synth::add_oscillator()
{
	if(config.oscillator_config.total > 20) return;

	config.oscillator_config.append(new SynthOscillatorConfig(config.oscillator_config.total - 1));
}

void Synth::delete_oscillator()
{
	if(config.oscillator_config.total)
	{
		config.oscillator_config.remove_object();
	}
}


double Synth::get_total_power()
{
	double result = 0;

	if(config.wavefunction == DC) return 1.0;

	for(int i = 0; i < config.oscillator_config.total; i++)
	{
		result += db.fromdb(config.oscillator_config.values[i]->level);
	}

	if(result == 0) result = 1;  // prevent division by 0
	return result;
}


double Synth::solve_eqn(double *output,
	int length,
	double freq,
	double normalize_constant,
	int oscillator)
{
	SynthOscillatorConfig *config =
		this->config.oscillator_config.values[oscillator];
	if(config->level <= INFINITYGAIN) return 0;

	double power = this->db.fromdb(config->level) * normalize_constant;
// Period of fundamental frequency in samples
	double orig_period = (double)get_samplerate() /
		freq;
// Starting sample in waveform
	double x = waveform_sample;
	double phase_offset = config->phase * orig_period;
//printf("Synth::solve_eqn %d %f\n", __LINE__, config->phase);
// Period of current oscillator
	double period = orig_period / config->freq_factor;
	int sample;
	double step = 1;
	if(get_direction() == PLAY_REVERSE) step = -1;

	switch(this->config.wavefunction)
	{
		case DC:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += power;
			}
			break;

		case SINE:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += sin((x + phase_offset) /
					period *
					2 *
					M_PI) * power;
				x += step;
			}
			break;

		case SAWTOOTH:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += function_sawtooth((x + phase_offset) /
					period) * power;
				x += step;
			}
			break;

		case SQUARE:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += function_square((x + phase_offset) /
					period) * power;
				x += step;
			}
			break;

		case TRIANGLE:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += function_triangle((x + phase_offset) /
					period) * power;
				x += step;
			}
			break;

		case PULSE:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += function_pulse((x + phase_offset) /
					period) * power;
				x += step;
			}
			break;

		case NOISE:
			for(sample = 0; sample < length; sample++)
			{
				output[sample] += function_noise() * power;
			}
			break;
	}
	return 0;
}

double Synth::get_point(float x, double normalize_constant)
{
	double result = 0;
	for(int i = 0; i < config.oscillator_config.total; i++)
		result += get_oscillator_point(x, normalize_constant, i);

	return result;
}

double Synth::get_oscillator_point(float x,
		double normalize_constant,
		int oscillator)
{
	SynthOscillatorConfig *config = this->config.oscillator_config.values[oscillator];
	double power = db.fromdb(config->level) * normalize_constant;
	switch(this->config.wavefunction)
	{
		case DC:
			return power;
			break;
		case SINE:
			return sin((x + config->phase) * config->freq_factor * 2 * M_PI) * power;
			break;
		case SAWTOOTH:
			return function_sawtooth((x + config->phase) * config->freq_factor) * power;
			break;
		case SQUARE:
			return function_square((x + config->phase) * config->freq_factor) * power;
			break;
		case TRIANGLE:
			return function_triangle((x + config->phase) * config->freq_factor) * power;
			break;
		case PULSE:
			return function_pulse((x + config->phase) * config->freq_factor) * power;
			break;
		case NOISE:
			return function_noise() * power;
			break;
	}
	return 0;
}

double Synth::function_square(double x)
{
	x -= (int)x; // only fraction counts
	return (x < .5) ? -1 : 1;
}

double Synth::function_pulse(double x)
{
	x -= (int)x; // only fraction counts
	return (x < .5) ? 0 : 1;
}

double Synth::function_noise()
{
	return (double)(rand() % 65536 - 32768) / 32768;
}

double Synth::function_sawtooth(double x)
{
	x -= (int)x;
	return 1 - x * 2;
}

double Synth::function_triangle(double x)
{
	x -= (int)x;
	return (x < .5) ? 1 - x * 4 : -3 + x * 4;
}

int Synth::process_realtime(int64_t size,
	Samples *input_ptr,
	Samples *output_ptr)
{
// sample relative to start of plugin
	waveform_sample = get_source_position();

	need_reconfigure |= load_configuration();
	if(need_reconfigure) reconfigure();

	double wetness = DB::fromdb(config.wetness);
	if(EQUIV(config.wetness, INFINITYGAIN)) wetness = 0;

// Apply wetness
	double *output_samples = output_ptr->get_data();
	double *input_samples = input_ptr->get_data();
	for(int j = 0; j < size; j++)
		output_samples[j] = input_samples[j] * wetness;

// Overlay each frequency
	for(int j = 0; j < MAX_FREQS; j++)
	{
		if(!EQUIV(config.base_freq[j], 0))
		{

// Compute fragment
			overlay_synth(
				config.base_freq[j],
				size,
				input_ptr->get_data(),
				output_ptr->get_data());
	//printf("Synth::process_realtime 2\n");
		}
	}

//	waveform_sample += size;
	return 0;
}

int Synth::overlay_synth(double freq,
	int64_t length,
	double *input,
	double *output)
{
	double normalize_constant = 1.0 / get_total_power();
	for(int i = 0; i < config.oscillator_config.total; i++)
		solve_eqn(output,
			length,
			freq,
			normalize_constant,
			i);
	return length;
}

void Synth::reconfigure()
{
	need_reconfigure = 0;
//	waveform_sample = 0;
}

int Synth::freq_exists(double freq)
{
	for(int i = 0; i < MAX_FREQS; i++)
	{

// printf("Synth::freq_exists %d %d %f %f\n",
// __LINE__,
// i,
// freq,
// config.base_freq[i]);
		if(fabs(freq - config.base_freq[i]) < 1.0)
			return 1;
	}

	return 0;
}

void Synth::new_freq(double freq)
{
// Check for dupes
	for(int i = 0; i < MAX_FREQS; i++)
	{
		if(EQUIV(config.base_freq[i], freq)) return;
	}

	for(int i = 0; i < MAX_FREQS; i++)
	{
		if(EQUIV(config.base_freq[i], 0))
		{
			config.base_freq[i] = freq;
//printf("Synth::new_freq %d\n", __LINE__);
			break;
		}
	}
}

void Synth::delete_freq(double freq)
{
	for(int i = 0; i < MAX_FREQS; i++)
	{
		if(EQUIV(config.base_freq[i], freq))
		{
//printf("Synth::delete_freq %d\n", __LINE__);
// Shift frequencies back
			for(int j = i; j < MAX_FREQS - 1; j++)
			{
				config.base_freq[j] = config.base_freq[j + 1];
			}
			config.base_freq[MAX_FREQS - 1] = 0;

			i--;
		}
	}
}

void Synth::delete_freqs()
{
	for(int i = 0; i < MAX_FREQS; i++)
		config.base_freq[i] = 0;
}




























SynthWindow::SynthWindow(Synth *synth)
 : PluginClientWindow(synth,
	synth->window_w,
	synth->window_h,
	400,
	350,
	1)
{
	this->synth = synth;
	white_key[0] = 0;
	white_key[1] = 0;
	white_key[2] = 0;
	black_key[0] = 0;
	black_key[1] = 0;
	black_key[2] = 0;
	bzero(notes, sizeof(SynthNote*) * TOTALNOTES);
	current_note = -1;
}

SynthWindow::~SynthWindow()
{
	delete white_key[0];
	delete white_key[1];
	delete white_key[2];
	delete black_key[0];
	delete black_key[1];
	delete black_key[2];
}

static const char *keyboard_map[] =
{
	"q", "2", "w", "3", "e", "r", "5", "t", "6", "y", "7", "u",
	"z", "s", "x", "d", "c", "v", "g", "b", "h", "n", "j", "m"
};

void SynthWindow::create_objects()
{
	BC_MenuBar *menu;
	add_subwindow(menu = new BC_MenuBar(0, 0, get_w()));

	BC_Menu *levelmenu, *phasemenu, *harmonicmenu;
	menu->add_menu(levelmenu = new BC_Menu(_("Level")));
	menu->add_menu(phasemenu = new BC_Menu(_("Phase")));
	menu->add_menu(harmonicmenu = new BC_Menu(_("Harmonic")));

	levelmenu->add_item(new SynthLevelInvert(synth));
	levelmenu->add_item(new SynthLevelMax(synth));
	levelmenu->add_item(new SynthLevelRandom(synth));
	levelmenu->add_item(new SynthLevelSine(synth));
	levelmenu->add_item(new SynthLevelSlope(synth));
	levelmenu->add_item(new SynthLevelZero(synth));

	phasemenu->add_item(new SynthPhaseInvert(synth));
	phasemenu->add_item(new SynthPhaseRandom(synth));
	phasemenu->add_item(new SynthPhaseSine(synth));
	phasemenu->add_item(new SynthPhaseZero(synth));

	harmonicmenu->add_item(new SynthFreqEnum(synth));
	harmonicmenu->add_item(new SynthFreqEven(synth));
	harmonicmenu->add_item(new SynthFreqFibonacci(synth));
	harmonicmenu->add_item(new SynthFreqOdd(synth));
	harmonicmenu->add_item(new SynthFreqPrime(synth));

	int x = 10, y = 30;

	add_subwindow(new BC_Title(x, y, _("Waveform")));
	x += 240;
	add_subwindow(new BC_Title(x, y, _("Wave Function")));
	y += 20;
	x = 10;
	add_subwindow(canvas = new SynthCanvas(synth, this, x, y, 230, 160));
	canvas->update();

	x += 240;
	char string[BCTEXTLEN];
	waveform_to_text(string, synth->config.wavefunction);

	add_subwindow(waveform = new SynthWaveForm(synth, x, y, string));
	waveform->create_objects();
	y += 30;
	int x1 = x + waveform->get_w() + 10;


	add_subwindow(new BC_Title(x, y, _("Base Frequency:")));
	y += 30;
	add_subwindow(base_freq = new SynthBaseFreq(synth, this, x, y));
	base_freq->update((float)synth->config.base_freq[0]);
	x += base_freq->get_w() + synth->get_theme()->widget_border;
	add_subwindow(freqpot = new SynthFreqPot(synth, this, x, y - 10));
	base_freq->freq_pot = freqpot;
	freqpot->freq_text = base_freq;
	x -= base_freq->get_w() + synth->get_theme()->widget_border;
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Wetness:")));
	add_subwindow(wetness = new SynthWetness(synth, x + 70, y - 10));

	y += 40;
	add_subwindow(new SynthClear(synth, x, y));


	x = 50;
	y = 220;
	add_subwindow(new BC_Title(x, y, _("Level")));
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Phase")));
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Harmonic")));



	y += 20; x = 10;
	add_subwindow(osc_subwindow = new BC_SubWindow(x, y, 265, get_h() - y));
	x += 265;
	add_subwindow(osc_scroll = new OscScroll(synth, this, x, y, get_h() - y));


	x += 20;
	add_subwindow(new SynthAddOsc(synth, this, x, y));
	y += 30;
	add_subwindow(new SynthDelOsc(synth, this, x, y));

// Create keyboard
	y = 30;

#include "white_up_png.h"
#include "white_hi_png.h"
#include "white_dn_png.h"
#include "white_checked_png.h"
#include "white_checkedhi_png.h"
#include "black_up_png.h"
#include "black_hi_png.h"
#include "black_dn_png.h"
#include "black_checked_png.h"
#include "black_checkedhi_png.h"
	white_key[0] = new VFramePng(white_up_png);
	white_key[1] = new VFramePng(white_hi_png);
	white_key[2] = new VFramePng(white_checked_png);
	white_key[3] = new VFramePng(white_dn_png);
	white_key[4] = new VFramePng(white_checkedhi_png);
	black_key[0] = new VFramePng(black_up_png);
	black_key[1] = new VFramePng(black_hi_png);
	black_key[2] = new VFramePng(black_checked_png);
	black_key[3] = new VFramePng(black_dn_png);
	black_key[4] = new VFramePng(black_checkedhi_png);


	add_subwindow(note_subwindow = new BC_SubWindow(x1,
		y,
		get_w() - x1,
		white_key[0]->get_h() + MARGIN +
		get_text_height(MEDIUMFONT) + MARGIN +
		get_text_height(MEDIUMFONT) + MARGIN));
	add_subwindow(note_scroll = new NoteScroll(synth,
		this,
		x1,
		note_subwindow->get_y() + note_subwindow->get_h(),
		note_subwindow->get_w()));

	add_subwindow(momentary = new SynthMomentary(this,
		x1,
		note_scroll->get_y() + note_scroll->get_h() + MARGIN,
		_("Momentary notes")));


	add_subwindow(note_instructions = new BC_Title(
		x1,
		momentary->get_y() + momentary->get_h() + MARGIN,
		_("Ctrl or Shift to select multiple notes.")));

	update_scrollbar();
	update_oscillators();
	update_notes();

	show_window();
}

int SynthWindow::keypress_event()
{
	if(ctrl_down() && get_keypress() == 'w')
	{
		set_done(1);
		return 1;
	}
	return 0;
}

int SynthWindow::resize_event(int w, int h)
{
	clear_box(0, 0, w, h);
	osc_subwindow->reposition_window(osc_subwindow->get_x(),
		osc_subwindow->get_y(),
		osc_subwindow->get_w(),
		h - osc_subwindow->get_y());
	osc_subwindow->clear_box(0, 0, osc_subwindow->get_w(), osc_subwindow->get_h());
	osc_scroll->reposition_window(osc_scroll->get_x(),
		osc_scroll->get_y(),
		h - osc_scroll->get_y());
	note_subwindow->reposition_window(note_subwindow->get_x(),
		note_subwindow->get_y(),
		w - note_subwindow->get_x(),
		note_subwindow->get_h());
	note_scroll->reposition_window(note_scroll->get_x(),
		note_scroll->get_y(),
		w - note_scroll->get_x());
	note_scroll->update_length(white_key[0]->get_w() * TOTALNOTES * 7 / 12 +
			white_key[0]->get_w(),
		note_scroll->get_position(),
		note_subwindow->get_w(),
		0);

	update_scrollbar();
	update_notes();
	update_oscillators();
	synth->window_w = w;
	synth->window_h = h;
	return 1;
}

void SynthWindow::update_gui()
{
	char string[BCTEXTLEN];
	freqpot->update((int)synth->config.base_freq[0]);
	base_freq->update((float)synth->config.base_freq[0]);
	wetness->update(synth->config.wetness);
	waveform_to_text(string, synth->config.wavefunction);
	waveform->set_text(string);
	momentary->update(synth->config.momentary_notes);

	update_scrollbar();
	update_oscillators();
	canvas->update();
	update_note_selection();
	show_window();
}

void SynthWindow::update_scrollbar()
{
	osc_scroll->update_length(synth->config.oscillator_config.total * OSCILLATORHEIGHT,
		osc_scroll->get_position(),
		osc_subwindow->get_h(),
		0);
}


void SynthWindow::update_whitekey(int number,
	int *current_title,
	int x,
	int y)
{
	if(!notes[number])
	{
		note_subwindow->add_subwindow(notes[number] = new SynthNote(this,
			white_key, number, x, y));
		if(number >= FIRST_TITLE && number < LAST_TITLE)
			note_subwindow->add_subwindow(
				note_titles[(*current_title)++] = new BC_Title(
					x + text_white_margin,
					y2,
					keyboard_map[number - FIRST_TITLE]));
//printf("SynthWindow::update_whitekey %d\n", __LINE__);
	}
	else
	{
		notes[number]->reposition_window(x, y);
		if(number >= FIRST_TITLE && number < LAST_TITLE)
			note_titles[(*current_title)++]->reposition_window(x + text_white_margin,
					y2);
	}
}


void SynthWindow::update_blackkey(int number,
	int *current_title,
	int x,
	int y)
{
	if(!notes[number])
	{
		note_subwindow->add_subwindow(notes[number] = new SynthNote(this,
			black_key, number, x, y));
		if(number >= FIRST_TITLE && number < LAST_TITLE)
			note_subwindow->add_subwindow(
				note_titles[(*current_title)++] = new BC_Title(x + text_black_margin,
					y1,
					keyboard_map[number - FIRST_TITLE]));
	}
	else
	{
		notes[number]->reposition_window(x, y);
		if(number >= FIRST_TITLE && number < LAST_TITLE)
			note_titles[(*current_title)++]->reposition_window(x + text_black_margin,
					y1);
	}
}

void SynthWindow::update_notes()
{
	//int octave_w = white_key[0]->get_w() * 7;
	int white_w = white_key[0]->get_w();
	int black_w = black_key[0]->get_w();
	int white_w1 = white_w - black_w / 2 - 2;
	int white_w2 = white_w / 2;
	int white_w3 = white_w * 2 / 3;
	int y = 0;
	int x = 0;
	y1 = y + white_key[0]->get_h() + 10;
	y2 = y1 + get_text_height(MEDIUMFONT) + 10;
	y3 = y2 + get_text_height(MEDIUMFONT) + 10;
	text_black_margin = black_w / 2 - get_text_width(MEDIUMFONT, "O") / 2;
	text_white_margin = white_w / 2 - get_text_width(MEDIUMFONT, "O") / 2;


//printf("SynthWindow::update_notes %d\n", __LINE__);
	note_subwindow->clear_box(0, 0, get_w(), get_h());

	note_subwindow->set_color(get_resources()->default_text_color);

// Add new notes
// To get the stacking order:
// pass 0 is white keys
// pass 1 is black keys
	int current_title = 0;
	for(int pass = 0; pass < 2; pass++)
	{
		x = -note_scroll->get_position();

		for(int i = 0; i < TOTALNOTES; i++)
		{
			int octave_note = i % 12;
			if(!pass)
			{
// White keys
				switch(octave_note)
				{
					case 0:
						update_whitekey(i, &current_title, x, y);
						break;
					case 1:
						x += white_w;
						break;
					case 2:
						update_whitekey(i, &current_title, x, y);
						break;
					case 3:
						x += white_w;
						break;
					case 4:
						update_whitekey(i, &current_title, x, y);
						x += white_w;
						break;
					case 5:
						update_whitekey(i, &current_title, x, y);
						break;
					case 6:
						x += white_w;
						break;
					case 7:
						update_whitekey(i, &current_title, x, y);
						break;
					case 8:
						x += white_w;
						break;
					case 9:
						update_whitekey(i, &current_title, x, y);
						break;
					case 10:
						x += white_w;
						break;
					case 11:
						update_whitekey(i, &current_title, x, y);
						x += white_w;
						break;
				}
			}
			else
			{
// Black keys
				switch(octave_note)
				{
					case 1:
						update_blackkey(i, &current_title, x + white_w2, y);
						x += white_w;
						break;
					case 3:
						update_blackkey(i, &current_title, x + white_w3, y);
						x += white_w;
						break;
					case 4:
						x += white_w;
						break;
					case 6:
						update_blackkey(i, &current_title, x + white_w2, y);
						x += white_w;
						break;
					case 8:
						update_blackkey(i, &current_title, x + white_w1, y);
						x += white_w;
						break;
					case 10:
						update_blackkey(i, &current_title, x + white_w3, y);
						x += white_w;
						break;
					case 11:
						x += white_w;
						break;
				}
			}
		}
	}
}

void SynthWindow::update_note_selection()
{
	for(int i = 0; i < TOTALNOTES; i++)
	{
		int got_it = 0;
		for(int j = 0; j < MAX_FREQS; j++)
		{
			if(synth->freq_exists(keyboard_freqs[notes[i]->number]))
			{
				got_it = 1;
				break;
			}
		}

		if(got_it)
		{
			notes[i]->set_value(1);
		}
		else
			notes[i]->set_value(0);
	}
}


void SynthWindow::update_oscillators()
{
	int i, y = -osc_scroll->get_position();



// Add new oscillators
	for(i = 0;
		i < synth->config.oscillator_config.total;
		i++)
	{
		SynthOscGUI *gui;
		SynthOscillatorConfig *config = synth->config.oscillator_config.values[i];

		if(oscillators.total <= i)
		{
			oscillators.append(gui = new SynthOscGUI(this, i));
			gui->create_objects(y);
		}
		else
		{
			gui = oscillators.values[i];

			gui->title->reposition_window(gui->title->get_x(), y + 15);

			gui->level->reposition_window(gui->level->get_x(), y);
			gui->level->update(config->level);

			gui->phase->reposition_window(gui->phase->get_x(), y);
			gui->phase->update((int64_t)(config->phase * 360));

			gui->freq->reposition_window(gui->freq->get_x(), y);
			gui->freq->update((int64_t)(config->freq_factor));
		}
		y += OSCILLATORHEIGHT;
	}

// Delete old oscillators
	for( ;
		i < oscillators.total;
		i++)
		oscillators.remove_object();
}


int SynthWindow::waveform_to_text(char *text, int waveform)
{
	switch(waveform)
	{
		case DC:              sprintf(text, _("DC"));           break;
		case SINE:            sprintf(text, _("Sine"));           break;
		case SAWTOOTH:        sprintf(text, _("Sawtooth"));       break;
		case SQUARE:          sprintf(text, _("Square"));         break;
		case TRIANGLE:        sprintf(text, _("Triangle"));       break;
		case PULSE:           sprintf(text, _("Pulse"));       break;
		case NOISE:           sprintf(text, _("Noise"));       break;
	}
	return 0;
}


SynthMomentary::SynthMomentary(SynthWindow *window, int x, int y, char *text)
 : BC_CheckBox(x,
	y,
	window->synth->config.momentary_notes,
	text)
{
	this->window = window;
}

int SynthMomentary::handle_event()
{
	window->synth->config.momentary_notes = get_value();
	window->synth->send_configure_change();
	return 1;
}




SynthNote::SynthNote(SynthWindow *window,
	VFrame **images,
	int number,
	int x,
	int y)
 : BC_Toggle(x,
 	y,
	images,
	window->synth->freq_exists(keyboard_freqs[number]))
{
	this->window = window;
	this->number = number;
	note_on = 0;
	set_select_drag(1);
	set_radial(1);
}

void SynthNote::start_note()
{
	if(window->synth->config.momentary_notes) note_on = 1;

//printf("SynthNote::start_note %d %d\n", __LINE__, ctrl_down());
	if(window->synth->config.momentary_notes || (!ctrl_down() && !shift_down()))
	{
// Kill all frequencies
		window->synth->delete_freqs();
	}

	window->synth->new_freq(keyboard_freqs[number]);
	window->base_freq->update((float)window->synth->config.base_freq[0]);
//printf("SynthNote::start_note %d %f\n", __LINE__, window->synth->config.base_freq[0]);
	window->freqpot->update(window->synth->config.base_freq[0]);
	window->synth->send_configure_change();
	window->update_note_selection();
}

void SynthNote::stop_note()
{
	note_on = 0;
	window->synth->delete_freq(keyboard_freqs[number]);
	window->base_freq->update((float)window->synth->config.base_freq[0]);
	window->freqpot->update(window->synth->config.base_freq[0]);
	window->synth->send_configure_change();
	window->update_note_selection();
}

int SynthNote::keypress_event()
{
	if(number >= FIRST_TITLE && number < LAST_TITLE)
	{
		if(get_keypress() == keyboard_map[number - FIRST_TITLE][0])
		{
			if((ctrl_down() || shift_down()) &&
				window->synth->freq_exists(keyboard_freqs[number]))
			{
				stop_note();
			}
			else
			{
				start_note();
				set_value(1);
			}

// Key releases are repeated, so momentary notes may not work
			return 1;
		}
	}
	return 0;
}

int SynthNote::keyrelease_event()
{
	if(note_on && window->synth->config.momentary_notes)
	{
		stop_note();
		set_value(0);
		return 1;
	}
	return 0;
}

int SynthNote::cursor_motion_event()
{
	int result = 0;
	if(window->current_note > -1)
	{
		int cursor_x = get_relative_cursor_x();
		int cursor_y = get_relative_cursor_y();
		if(cursor_x >= 0 && cursor_x < get_w() &&
			cursor_y >= 0 && cursor_y < get_h())
		{
			if(window->starting_notes)
			{
				start_note();
			}
			else
			{
				stop_note();
			}

			window->current_note = number;
			result = 1;
		}
	}
	return result;
}

int SynthNote::button_press_event()
{
	if(BC_Toggle::button_press_event())
	{
// printf("SynthNote::button_press_event %d %d %d\n",
// __LINE__,
// ctrl_down(),
// window->synth->freq_exists(keyboard_freqs[number]));
		window->starting_notes = 1;
		if((ctrl_down() || shift_down()) &&
			window->synth->freq_exists(keyboard_freqs[number]))
		{
//printf("SynthNote::button_press_event %d\n", __LINE__);
			stop_note();
			window->starting_notes = 0;
		}
		else
		{
//printf("SynthNote::button_press_event %d\n", __LINE__);
			start_note();
			window->starting_notes = 1;
		}
		window->current_note = number;
		return 1;
	}
	return 0;
}

int SynthNote::button_release_event()
{
// Change frequency permanently
	if(window->current_note == number)
	{
		if(window->synth->config.momentary_notes)
		{
// Mute on button release
			stop_note();
			set_value(0);
		}
		window->current_note = -1;
	}

	return BC_Toggle::button_release_event();
}

int SynthNote::draw_face(int flash, int flush)
{
	BC_Toggle::draw_face(0, 0);
	static const char *titles[] =
	{
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#",
		"A",
		"A#",
		"B"
	};


	const char *text = titles[number % (sizeof(titles) / sizeof(char*))];
	char string[BCTEXTLEN];
	sprintf(string, "%s%d", text, number / 12);
//printf("SynthNote::draw_face %d %d %d %d %s\n", __LINE__, number, get_w(), get_h(), text);
	if(text[1] == '#')
	{
	}
	else
	{
		set_color(BLACK);
		draw_text(get_w() / 2 - get_text_width(MEDIUMFONT, string) / 2,
			get_h() - get_text_height(MEDIUMFONT, string) - window->synth->get_theme()->widget_border,
			string);
	}

	if(flash) this->flash(0);
	if(flush) this->flush();
	return 0;
}







SynthOscGUI::SynthOscGUI(SynthWindow *window, int number)
{
	this->window = window;
	this->number = number;
}

SynthOscGUI::~SynthOscGUI()
{
	delete title;
	delete level;
	delete phase;
	delete freq;
}

void SynthOscGUI::create_objects(int y)
{
	char text[BCTEXTLEN];
	sprintf(text, "%d:", number + 1);
	window->osc_subwindow->add_subwindow(title = new BC_Title(10, y + 15, text));

	window->osc_subwindow->add_subwindow(level = new SynthOscGUILevel(window->synth, this, y));
	window->osc_subwindow->add_subwindow(phase = new SynthOscGUIPhase(window->synth, this, y));
	window->osc_subwindow->add_subwindow(freq = new SynthOscGUIFreq(window->synth, this, y));
}




SynthOscGUILevel::SynthOscGUILevel(Synth *synth, SynthOscGUI *gui, int y)
 : BC_FPot(50,
 	y,
	synth->config.oscillator_config.values[gui->number]->level,
	INFINITYGAIN,
	0)
{
	this->synth = synth;
	this->gui = gui;
}

SynthOscGUILevel::~SynthOscGUILevel()
{
}

int SynthOscGUILevel::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->level = get_value();
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}



SynthOscGUIPhase::SynthOscGUIPhase(Synth *synth, SynthOscGUI *gui, int y)
 : BC_IPot(125,
 	y,
	(int64_t)(synth->config.oscillator_config.values[gui->number]->phase * 360),
	0,
	360)
{
	this->synth = synth;
	this->gui = gui;
}

SynthOscGUIPhase::~SynthOscGUIPhase()
{
}

int SynthOscGUIPhase::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->phase = (float)get_value() / 360;
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}



SynthOscGUIFreq::SynthOscGUIFreq(Synth *synth, SynthOscGUI *gui, int y)
 : BC_IPot(200,
 	y,
	(int64_t)(synth->config.oscillator_config.values[gui->number]->freq_factor),
	1,
	100)
{
	this->synth = synth;
	this->gui = gui;
}

SynthOscGUIFreq::~SynthOscGUIFreq()
{
}

int SynthOscGUIFreq::handle_event()
{
	SynthOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->freq_factor = get_value();
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}







SynthAddOsc::SynthAddOsc(Synth *synth, SynthWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->synth = synth;
	this->window = window;
}

SynthAddOsc::~SynthAddOsc()
{
}

int SynthAddOsc::handle_event()
{
	synth->add_oscillator();
	synth->send_configure_change();
	window->update_gui();
	return 1;
}



SynthDelOsc::SynthDelOsc(Synth *synth, SynthWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->synth = synth;
	this->window = window;
}

SynthDelOsc::~SynthDelOsc()
{
}

int SynthDelOsc::handle_event()
{
	synth->delete_oscillator();
	synth->send_configure_change();
	window->update_gui();
	return 1;
}


OscScroll::OscScroll(Synth *synth,
	SynthWindow *window,
	int x,
	int y,
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	synth->config.oscillator_config.total * OSCILLATORHEIGHT,
	0,
	window->osc_subwindow->get_h())
{
	this->synth = synth;
	this->window = window;
}

OscScroll::~OscScroll()
{
}

int OscScroll::handle_event()
{
	window->update_oscillators();
	return 1;
}



NoteScroll::NoteScroll(Synth *synth,
	SynthWindow *window,
	int x,
	int y,
	int w)
 : BC_ScrollBar(x,
 	y,
	SCROLL_HORIZ,
	w,
	window->white_key[0]->get_w() * TOTALNOTES * 7 / 12 + window->white_key[0]->get_w(),
	0,
	window->note_subwindow->get_w())
{
	this->synth = synth;
	this->window = window;
}

NoteScroll::~NoteScroll()
{
}

int NoteScroll::handle_event()
{
	window->update_notes();
	return 1;
}














SynthClear::SynthClear(Synth *synth, int x, int y)
 : BC_GenericButton(x, y, _("Clear"))
{
	this->synth = synth;
}
SynthClear::~SynthClear()
{
}
int SynthClear::handle_event()
{
	synth->config.reset();
	synth->send_configure_change();
	synth->update_gui();
	return 1;
}






SynthWaveForm::SynthWaveForm(Synth *synth, int x, int y, char *text)
 : BC_PopupMenu(x, y, 120, text)
{
	this->synth = synth;
}

SynthWaveForm::~SynthWaveForm()
{
}

void SynthWaveForm::create_objects()
{
//	add_item(new SynthWaveFormItem(synth, _("DC"), DC));
	add_item(new SynthWaveFormItem(synth, _("Sine"), SINE));
	add_item(new SynthWaveFormItem(synth, _("Sawtooth"), SAWTOOTH));
	add_item(new SynthWaveFormItem(synth, _("Square"), SQUARE));
	add_item(new SynthWaveFormItem(synth, _("Triangle"), TRIANGLE));
	add_item(new SynthWaveFormItem(synth, _("Pulse"), PULSE));
	add_item(new SynthWaveFormItem(synth, _("Noise"), NOISE));
}

SynthWaveFormItem::SynthWaveFormItem(Synth *synth, char *text, int value)
 : BC_MenuItem(text)
{
	this->synth = synth;
	this->value = value;
}

SynthWaveFormItem::~SynthWaveFormItem()
{
}

int SynthWaveFormItem::handle_event()
{
	synth->config.wavefunction = value;
	((SynthWindow*)synth->thread->window)->canvas->update();
	get_popup_menu()->set_text(get_text());
	synth->send_configure_change();
	return 1;
}


SynthWetness::SynthWetness(Synth *synth, int x, int y)
 : BC_FPot(x,
		y,
		synth->config.wetness,
		INFINITYGAIN,
		0)
{
	this->synth = synth;
}

int SynthWetness::handle_event()
{
	synth->config.wetness = get_value();
	synth->send_configure_change();
	return 1;
}



SynthFreqPot::SynthFreqPot(Synth *synth, SynthWindow *window, int x, int y)
 : BC_QPot(x, y, synth->config.base_freq[0])
{
	this->synth = synth;
	this->window = window;
}
SynthFreqPot::~SynthFreqPot()
{
}
int SynthFreqPot::handle_event()
{
	if(get_value() > 0 && get_value() < 30000)
	{
		synth->config.base_freq[0] = get_value();
		freq_text->update(get_value());
		synth->send_configure_change();
		window->update_note_selection();
	}
	return 1;
}



SynthBaseFreq::SynthBaseFreq(Synth *synth, SynthWindow *window, int x, int y)
 : BC_TextBox(x, y, 100, 1, (float)0)
{
	this->synth = synth;
	this->window = window;
	set_precision(2);
}
SynthBaseFreq::~SynthBaseFreq()
{
}
int SynthBaseFreq::handle_event()
{
	double new_value = atof(get_text());
// 0 is mute
	if(new_value < 30000)
	{
		synth->config.base_freq[0] = new_value;
		freq_pot->update(synth->config.base_freq[0]);
		synth->send_configure_change();
		window->update_note_selection();
	}
	return 1;
}





SynthCanvas::SynthCanvas(Synth *synth,
	SynthWindow *window,
	int x,
	int y,
	int w,
	int h)
 : BC_SubWindow(x,
 	y,
	w,
	h,
	BLACK)
{
	this->synth = synth;
	this->window = window;
}

SynthCanvas::~SynthCanvas()
{
}

int SynthCanvas::update()
{
	int y1, y2, y = 0;

	clear_box(0, 0, get_w(), get_h());
	set_color(RED);

	draw_line(0, get_h() / 2 + y, get_w(), get_h() / 2 + y);

	set_color(GREEN);

	double normalize_constant = (double)1 / synth->get_total_power();
	y1 = (int)(synth->get_point((float)0, normalize_constant) * get_h() / 2);

	for(int i = 1; i < get_w(); i++)
	{
		y2 = (int)(synth->get_point((float)i / get_w(), normalize_constant) * get_h() / 2);
		draw_line(i - 1, get_h() / 2 - y1, i, get_h() / 2 - y2);
		y1 = y2;
	}
	flash();
	return 0;
}








// ======================= level calculations
SynthLevelZero::SynthLevelZero(Synth *synth)
 : BC_MenuItem(_("Zero"))
{
	this->synth = synth;
}

SynthLevelZero::~SynthLevelZero()
{
}

int SynthLevelZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = INFINITYGAIN;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelMax::SynthLevelMax(Synth *synth)
 : BC_MenuItem(_("Maximum"))
{
	this->synth = synth;
}

SynthLevelMax::~SynthLevelMax()
{
}

int SynthLevelMax::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = 0;
	}
	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelNormalize::SynthLevelNormalize(Synth *synth)
 : BC_MenuItem(_("Normalize"))
{
	this->synth = synth;
}

SynthLevelNormalize::~SynthLevelNormalize()
{
}

int SynthLevelNormalize::handle_event()
{
// get total power
	float total = 0;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		total += synth->db.fromdb(synth->config.oscillator_config.values[i]->level);
	}

	float scale = 1 / total;
	float new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = synth->db.fromdb(synth->config.oscillator_config.values[i]->level);
		new_value *= scale;
		new_value = synth->db.todb(new_value);

		synth->config.oscillator_config.values[i]->level = new_value;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelSlope::SynthLevelSlope(Synth *synth)
 : BC_MenuItem(_("Slope"))
{
	this->synth = synth;
}

SynthLevelSlope::~SynthLevelSlope()
{
}

int SynthLevelSlope::handle_event()
{
	float slope = (float)INFINITYGAIN / synth->config.oscillator_config.total;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = i * slope;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelRandom::SynthLevelRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
SynthLevelRandom::~SynthLevelRandom()
{
}

int SynthLevelRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = -(rand() % -INFINITYGAIN);
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelInvert::SynthLevelInvert(Synth *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}
SynthLevelInvert::~SynthLevelInvert()
{
}

int SynthLevelInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level =
			INFINITYGAIN - synth->config.oscillator_config.values[i]->level;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthLevelSine::SynthLevelSine(Synth *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}
SynthLevelSine::~SynthLevelSine()
{
}

int SynthLevelSine::handle_event()
{
	float new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (float)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) * INFINITYGAIN / 2 + INFINITYGAIN / 2;
		synth->config.oscillator_config.values[i]->level = new_value;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

// ============================ phase calculations

SynthPhaseInvert::SynthPhaseInvert(Synth *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}
SynthPhaseInvert::~SynthPhaseInvert()
{
}

int SynthPhaseInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase =
			1 - synth->config.oscillator_config.values[i]->phase;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthPhaseZero::SynthPhaseZero(Synth *synth)
 : BC_MenuItem(_("Zero"))
{
	this->synth = synth;
}
SynthPhaseZero::~SynthPhaseZero()
{
}

int SynthPhaseZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase = 0;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthPhaseSine::SynthPhaseSine(Synth *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}
SynthPhaseSine::~SynthPhaseSine()
{
}

int SynthPhaseSine::handle_event()
{
	float new_value;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (float)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) / 2 + .5;
		synth->config.oscillator_config.values[i]->phase = new_value;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthPhaseRandom::SynthPhaseRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
SynthPhaseRandom::~SynthPhaseRandom()
{
}

int SynthPhaseRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase =
			(float)(rand() % 360) / 360;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}


// ============================ freq calculations

SynthFreqRandom::SynthFreqRandom(Synth *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
SynthFreqRandom::~SynthFreqRandom()
{
}

int SynthFreqRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = rand() % 100;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthFreqEnum::SynthFreqEnum(Synth *synth)
 : BC_MenuItem(_("Enumerate"))
{
	this->synth = synth;
}
SynthFreqEnum::~SynthFreqEnum()
{
}

int SynthFreqEnum::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)i + 1;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthFreqEven::SynthFreqEven(Synth *synth)
 : BC_MenuItem(_("Even"))
{
	this->synth = synth;
}
SynthFreqEven::~SynthFreqEven()
{
}

int SynthFreqEven::handle_event()
{
	if(synth->config.oscillator_config.total)
		synth->config.oscillator_config.values[0]->freq_factor = (float)1;

	for(int i = 1; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)i * 2;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthFreqOdd::SynthFreqOdd(Synth *synth)
 : BC_MenuItem(_("Odd"))
{ this->synth = synth; }
SynthFreqOdd::~SynthFreqOdd()
{
}

int SynthFreqOdd::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)1 + i * 2;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthFreqFibonacci::SynthFreqFibonacci(Synth *synth)
 : BC_MenuItem(_("Fibonnacci"))
{
	this->synth = synth;
}
SynthFreqFibonacci::~SynthFreqFibonacci()
{
}

int SynthFreqFibonacci::handle_event()
{
	float last_value1 = 0, last_value2 = 1;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = last_value1 + last_value2;
		if(synth->config.oscillator_config.values[i]->freq_factor > 100) synth->config.oscillator_config.values[i]->freq_factor = 100;
		last_value1 = last_value2;
		last_value2 = synth->config.oscillator_config.values[i]->freq_factor;
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

SynthFreqPrime::SynthFreqPrime(Synth *synth)
 : BC_MenuItem(_("Prime"))
{
	this->synth = synth;
}
SynthFreqPrime::~SynthFreqPrime()
{
}

int SynthFreqPrime::handle_event()
{
	float number = 1;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = number;
		number = get_next_prime(number);
	}

	((SynthWindow*)synth->thread->window)->update_gui();
	synth->send_configure_change();
	return 1;
}

float SynthFreqPrime::get_next_prime(float number)
{
	int result = 1;

	while(result)
	{
		result = 0;
		number++;

		for(float i = number - 1; i > 1 && !result; i--)
		{
			if((number / i) - (int)(number / i) == 0) result = 1;
		}
	}

	return number;
}








SynthOscillatorConfig::SynthOscillatorConfig(int number)
{
	reset();
	this->number = number;
}

SynthOscillatorConfig::~SynthOscillatorConfig()
{
}

void SynthOscillatorConfig::reset()
{
	level = 0;
	phase = 0;
	freq_factor = 1;
}


void SynthOscillatorConfig::read_data(FileXML *file)
{
	level = file->tag.get_property("LEVEL", (float)level);
	phase = file->tag.get_property("PHASE", (float)phase);
	freq_factor = file->tag.get_property("FREQFACTOR", (float)freq_factor);
}

void SynthOscillatorConfig::save_data(FileXML *file)
{
	file->tag.set_title("OSCILLATOR");
	file->tag.set_property("LEVEL", (float)level);
	file->tag.set_property("PHASE", (float)phase);
	file->tag.set_property("FREQFACTOR", (float)freq_factor);
	file->append_tag();
	file->tag.set_title("/OSCILLATOR");
	file->append_tag();
	file->append_newline();
}

int SynthOscillatorConfig::equivalent(SynthOscillatorConfig &that)
{
	if(EQUIV(level, that.level) &&
		EQUIV(phase, that.phase) &&
		EQUIV(freq_factor, that.freq_factor))
		return 1;
	else
		return 0;
}

void SynthOscillatorConfig::copy_from(SynthOscillatorConfig& that)
{
	level = that.level;
	phase = that.phase;
	freq_factor = that.freq_factor;
}


SynthConfig::SynthConfig()
{
	reset();
}

SynthConfig::~SynthConfig()
{
	oscillator_config.remove_all_objects();
}

void SynthConfig::reset()
{
	wetness = 0;
	base_freq[0] = 440;
	for(int i = 1; i < MAX_FREQS; i++)
		base_freq[i] = 0;
	wavefunction = SINE;
	for(int i = 0; i < oscillator_config.total; i++)
	{
		oscillator_config.values[i]->reset();
	}

	momentary_notes = 0;
}

int SynthConfig::equivalent(SynthConfig &that)
{
//printf("SynthConfig::equivalent %d %d\n", base_freq, that.base_freq);
	for(int i = 0; i < MAX_FREQS; i++)
		if(base_freq[i] != that.base_freq[i]) return 0;

	if(wavefunction != that.wavefunction ||
		oscillator_config.total != that.oscillator_config.total ||
		momentary_notes != that.momentary_notes) return 0;

	for(int i = 0; i < oscillator_config.total; i++)
	{
		if(!oscillator_config.values[i]->equivalent(*that.oscillator_config.values[i]))
			return 0;
	}

	return 1;
}

void SynthConfig::copy_from(SynthConfig& that)
{
	wetness = that.wetness;
	for(int i = 0; i < MAX_FREQS; i++)
		base_freq[i] = that.base_freq[i];
	wavefunction = that.wavefunction;
	momentary_notes = that.momentary_notes;

	int i;
	for(i = 0;
		i < oscillator_config.total && i < that.oscillator_config.total;
		i++)
	{
		oscillator_config.values[i]->copy_from(*that.oscillator_config.values[i]);
	}

	for( ;
		i < that.oscillator_config.total;
		i++)
	{
		oscillator_config.append(new SynthOscillatorConfig(i));
		oscillator_config.values[i]->copy_from(*that.oscillator_config.values[i]);
	}

	for( ;
		i < oscillator_config.total;
		i++)
	{
		oscillator_config.remove_object();
	}

}

void SynthConfig::interpolate(SynthConfig &prev,
	SynthConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	copy_from(prev);
	wetness = (int)(prev.wetness * prev_scale + next.wetness * next_scale);
//	base_freq = (int)(prev.base_freq * prev_scale + next.base_freq * next_scale);

	momentary_notes = prev.momentary_notes;
}

