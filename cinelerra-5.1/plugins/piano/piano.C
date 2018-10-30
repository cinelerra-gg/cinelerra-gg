
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

#include "bcdisplayinfo.h"
#include "clip.h"
#include "bchash.h"
#include "filexml.h"
#include "language.h"
#include "piano.h"
#include "vframe.h"

#include <string.h>



REGISTER_PLUGIN(Piano)





Piano::Piano(PluginServer *server)
 : PluginAClient(server)
{
	reset();
}



Piano::~Piano()
{
	if(dsp_buffer) delete [] dsp_buffer;
}


char* Piano::plugin_title() { return N_("Pianoesizer"); }
int Piano::is_realtime() { return 1; }
int Piano::is_synthesis() { return 1; }


void Piano::reset()
{
	thread = 0;
	need_reconfigure = 1;
	dsp_buffer = 0;
}




LOAD_CONFIGURATION_MACRO(Piano, PianoConfig)





void Piano::read_data(KeyFrame *keyframe)
{
	FileXML input;
// cause htal file to read directly from text
	input.set_shared_input(keyframe->xbuf);

//printf("Piano::read_data %s\n", keyframe->get_data());
	int result = 0, current_osc = 0, total_oscillators = 0;
	while(!result)
	{
		result = input.read_tag();

		if(!result)
		{
			if(input.tag.title_is("SYNTH"))
			{
				config.wetness = input.tag.get_property("WETNESS", config.wetness);
				config.base_freq = input.tag.get_property("BASEFREQ", config.base_freq);
				config.wavefunction = input.tag.get_property("WAVEFUNCTION", config.wavefunction);
				total_oscillators = input.tag.get_property("OSCILLATORS", 0);
			}
			else
			if(input.tag.title_is("OSCILLATOR"))
			{
				if(current_osc >= config.oscillator_config.total)
					config.oscillator_config.append(new PianoOscillatorConfig(current_osc));

				config.oscillator_config.values[current_osc]->read_data(&input);
				current_osc++;
			}
		}
	}

	while(config.oscillator_config.total > current_osc)
		config.oscillator_config.remove_object();
}

void Piano::save_data(KeyFrame *keyframe)
{
	FileXML output;
// cause htal file to store data directly in text
	output.set_shared_output(keyframe->xbuf);

	output.tag.set_title("SYNTH");
	output.tag.set_property("WETNESS", config.wetness);
	output.tag.set_property("BASEFREQ", config.base_freq);
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
// data is now in *text
}

int Piano::show_gui()
{
	load_configuration();

	thread = new PianoThread(this);
	thread->start();
	return 0;
}

int Piano::set_string()
{
	if( thread )
		thread->window->set_title(gui_string);
	return 0;
}

void Piano::raise_window()
{
	if(thread)
	{
		thread->window->raise_window();
		thread->window->flush();
	}
}

void Piano::update_gui()
{
	if( !thread ) return;
	PianoWindow *window = (PianoWindow *)thread->window:
// load_configuration,read_data deletes oscillator_config
	window->lock_window("Piano::update_gui");
	load_configuration();
	window->update_gui();
	window->unlock_window();
}


void Piano::add_oscillator()
{
	if(config.oscillator_config.total > 20) return;

	config.oscillator_config.append(new PianoOscillatorConfig(config.oscillator_config.total - 1));
}

void Piano::delete_oscillator()
{
	if(config.oscillator_config.total)
	{
		config.oscillator_config.remove_object();
	}
}


double Piano::get_total_power()
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


double Piano::solve_eqn(double *output,
	double x1,
	double x2,
	double normalize_constant,
	int oscillator)
{
	PianoOscillatorConfig *config = this->config.oscillator_config.values[oscillator];
	if(config->level <= INFINITYGAIN) return 0;

	double result;
	double x;
	double power = this->db.fromdb(config->level) * normalize_constant;
	double phase_offset = config->phase * this->period;
	double x3 = x1 + phase_offset;
	double x4 = x2 + phase_offset;
	double period = this->period / config->freq_factor;
	int sample;

	switch(this->config.wavefunction)
	{
		case DC:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += power;
			}
			break;
		case SINE:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += sin(x / period * 2 * M_PI) * power;
			}
			break;
		case SAWTOOTH:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += function_sawtooth(x / period) * power;
			}
			break;
		case SQUARE:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += function_square(x / period) * power;
			}
			break;
		case TRIANGLE:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += function_triangle(x / period) * power;
			}
			break;
		case PULSE:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += function_pulse(x / period) * power;
			}
			break;
		case NOISE:
			for(sample = (int)x1, x = x3; x < x4; x++, sample++)
			{
				output[sample] += function_noise() * power;
			}
			break;
	}
}

double Piano::get_point(float x, double normalize_constant)
{
	double result = 0;
	for(int i = 0; i < config.oscillator_config.total; i++)
		result += get_oscillator_point(x, normalize_constant, i);

	return result;
}

double Piano::get_oscillator_point(float x,
		double normalize_constant,
		int oscillator)
{
	PianoOscillatorConfig *config = this->config.oscillator_config.values[oscillator];
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
}

double Piano::function_square(double x)
{
	x -= (int)x; // only fraction counts
	return (x < .5) ? -1 : 1;
}

double Piano::function_pulse(double x)
{
	x -= (int)x; // only fraction counts
	return (x < .5) ? 0 : 1;
}

double Piano::function_noise()
{
	return (double)(rand() % 65536 - 32768) / 32768;
}

double Piano::function_sawtooth(double x)
{
	x -= (int)x;
	return 1 - x * 2;
}

double Piano::function_triangle(double x)
{
	x -= (int)x;
	return (x < .5) ? 1 - x * 4 : -3 + x * 4;
}

int Piano::process_realtime(int64_t size, double *input_ptr, double *output_ptr)
{


	need_reconfigure |= load_configuration();
	if(need_reconfigure) reconfigure();

	double wetness = DB::fromdb(config.wetness);
	if(EQUIV(config.wetness, INFINITYGAIN)) wetness = 0;

	for(int j = 0; j < size; j++)
		output_ptr[j] = input_ptr[j] * wetness;

	int64_t fragment_len;
	for(int64_t i = 0; i < size; i += fragment_len)
	{
		fragment_len = size;
		if(i + fragment_len > size) fragment_len = size - i;

//printf("Piano::process_realtime 1 %d %d %d\n", i, fragment_len, size);
		fragment_len = overlay_synth(i, fragment_len, input_ptr, output_ptr);
//printf("Piano::process_realtime 2\n");
	}


	return 0;
}

int Piano::overlay_synth(int64_t start, int64_t length, double *input, double *output)
{
	if(waveform_sample + length > waveform_length)
		length = waveform_length - waveform_sample;

//printf("Piano::overlay_synth 1 %d %d\n", length, waveform_length);

// calculate some more data
// only calculate what's needed to speed it up
	if(waveform_sample + length > samples_rendered)
	{
		int64_t start = waveform_sample, end = waveform_sample + length;
		for(int i = start; i < end; i++) dsp_buffer[i] = 0;

		double normalize_constant = 1 / get_total_power();
		for(int i = 0; i < config.oscillator_config.total; i++)
			solve_eqn(dsp_buffer,
				start,
				end,
				normalize_constant,
				i);


		samples_rendered = end;
	}
//printf("Piano::overlay_synth 2\n");


	double *buffer_in = &input[start];
	double *buffer_out = &output[start];

	for(int i = 0; i < length; i++)
	{
		buffer_out[i] += dsp_buffer[waveform_sample++];
	}
//printf("Piano::overlay_synth 3\n");

	if(waveform_sample >= waveform_length) waveform_sample = 0;

	return length;
}

void Piano::reconfigure()
{
	need_reconfigure = 0;

	if(dsp_buffer)
	{
		delete [] dsp_buffer;
	}

//printf("Piano::reconfigure 1 %d\n", PluginAClient::project_sample_rate);
	waveform_length = PluginAClient::project_sample_rate;
	period = (float)PluginAClient::project_sample_rate / config.base_freq;
	dsp_buffer = new double[waveform_length + 1];

	samples_rendered = 0;     // do some calculations on the next process_realtime
	waveform_sample = 0;
}





















PianoThread::PianoThread(Piano *synth)
 : Thread()
{
	this->synth = synth;
	set_synchronous(0);
	completion.lock();
}

PianoThread::~PianoThread()
{
	delete window;
}

void PianoThread::run()
{
	BC_DisplayInfo info;
	window = new PianoWindow(synth,
		info.get_abs_cursor_x() - 125,
		info.get_abs_cursor_y() - 115);
	window->create_objects();
	int result = window->run_window();
	completion.unlock();
// Last command executed in thread
	if(result) synth->client_side_close();
}











PianoWindow::PianoWindow(Piano *synth, int x, int y)
 : BC_Window(synth->gui_string,
 	x,
	y,
	380,
	synth->h,
	380,
	10,
	1,
	0,
	1)
{
	this->synth = synth;
}

PianoWindow::~PianoWindow()
{
}

int PianoWindow::create_objects()
{
	BC_MenuBar *menu;
	add_subwindow(menu = new BC_MenuBar(0, 0, get_w()));

	BC_Menu *levelmenu, *phasemenu, *harmonicmenu;
	menu->add_menu(levelmenu = new BC_Menu(_("Level")));
	menu->add_menu(phasemenu = new BC_Menu(_("Phase")));
	menu->add_menu(harmonicmenu = new BC_Menu(_("Harmonic")));

	levelmenu->add_item(new PianoLevelInvert(synth));
	levelmenu->add_item(new PianoLevelMax(synth));
	levelmenu->add_item(new PianoLevelRandom(synth));
	levelmenu->add_item(new PianoLevelSine(synth));
	levelmenu->add_item(new PianoLevelSlope(synth));
	levelmenu->add_item(new PianoLevelZero(synth));

	phasemenu->add_item(new PianoPhaseInvert(synth));
	phasemenu->add_item(new PianoPhaseRandom(synth));
	phasemenu->add_item(new PianoPhaseSine(synth));
	phasemenu->add_item(new PianoPhaseZero(synth));

	harmonicmenu->add_item(new PianoFreqEnum(synth));
	harmonicmenu->add_item(new PianoFreqEven(synth));
	harmonicmenu->add_item(new PianoFreqFibonacci(synth));
	harmonicmenu->add_item(new PianoFreqOdd(synth));
	harmonicmenu->add_item(new PianoFreqPrime(synth));

	int x = 10, y = 30, i;
	add_subwindow(new BC_Title(x, y, _("Waveform")));
	x += 240;
	add_subwindow(new BC_Title(x, y, _("Wave Function")));
	y += 20;
	x = 10;
	add_subwindow(canvas = new PianoCanvas(synth, this, x, y, 230, 160));
	canvas->update();

	x += 240;
	char string[BCTEXTLEN];
	waveform_to_text(string, synth->config.wavefunction);

	add_subwindow(waveform = new PianoWaveForm(synth, x, y, string));
	waveform->create_objects();
	y += 30;


	add_subwindow(new BC_Title(x, y, _("Base Frequency:")));
	y += 30;
	add_subwindow(base_freq = new PianoBaseFreq(synth, x, y));
	x += 80;
	add_subwindow(freqpot = new PianoFreqPot(synth, this, x, y - 10));
	base_freq->freq_pot = freqpot;
	freqpot->freq_text = base_freq;
	x -= 80;
	y += 40;
	add_subwindow(new BC_Title(x, y, _("Wetness:")));
	add_subwindow(wetness = new PianoWetness(synth, x + 70, y - 10));

	y += 40;
	add_subwindow(new PianoClear(synth, x, y));


	x = 50;
	y = 220;
	add_subwindow(new BC_Title(x, y, _("Level")));
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Phase")));
	x += 75;
	add_subwindow(new BC_Title(x, y, _("Harmonic")));



	y += 20; x = 10;
	add_subwindow(subwindow = new PianoSubWindow(synth, x, y, 265, get_h() - y));
	x += 265;
	add_subwindow(scroll = new PianoScroll(synth, this, x, y, get_h() - y));


	x += 20;
	add_subwindow(new PianoAddOsc(synth, this, x, y));
	y += 30;
	add_subwindow(new PianoDelOsc(synth, this, x, y));

	update_scrollbar();
	update_oscillators();

	show_window();
	flush();
	return 0;
}

int PianoWindow::close_event()
{
// Set result to 1 to indicate a client side close
	set_done(1);
	return 1;
}

int PianoWindow::resize_event(int w, int h)
{
	clear_box(0, 0, w, h);
	subwindow->reposition_window(subwindow->get_x(),
		subwindow->get_y(),
		subwindow->get_w(),
		h - subwindow->get_y());
	subwindow->clear_box(0, 0, subwindow->get_w(), subwindow->get_h());
	scroll->reposition_window(scroll->get_x(),
		scroll->get_y(),
		h - scroll->get_y());
	update_scrollbar();
	update_oscillators();
	synth->w = w;
	synth->h = h;
	return 1;
}

void PianoWindow::update_gui()
{
	char string[BCTEXTLEN];
	freqpot->update(synth->config.base_freq);
	base_freq->update((int64_t)synth->config.base_freq);
	wetness->update(synth->config.wetness);
	waveform_to_text(string, synth->config.wavefunction);
	waveform->set_text(string);

	update_scrollbar();
	update_oscillators();
	canvas->update();
}

void PianoWindow::update_scrollbar()
{
	scroll->update_length(synth->config.oscillator_config.total * OSCILLATORHEIGHT,
		scroll->get_position(),
		subwindow->get_h());
}

void PianoWindow::update_oscillators()
{
	int i, y = -scroll->get_position();



// Add new oscillators
	for(i = 0;
		i < synth->config.oscillator_config.total;
		i++)
	{
		PianoOscGUI *gui;
		PianoOscillatorConfig *config = synth->config.oscillator_config.values[i];

		if(oscillators.total <= i)
		{
			oscillators.append(gui = new PianoOscGUI(this, i));
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


int PianoWindow::waveform_to_text(char *text, int waveform)
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







PianoOscGUI::PianoOscGUI(PianoWindow *window, int number)
{
	this->window = window;
	this->number = number;
}

PianoOscGUI::~PianoOscGUI()
{
	delete title;
	delete level;
	delete phase;
	delete freq;
}

int PianoOscGUI::create_objects(int y)
{
	char text[BCTEXTLEN];
	sprintf(text, "%d:", number + 1);
	window->subwindow->add_subwindow(title = new BC_Title(10, y + 15, text));

	window->subwindow->add_subwindow(level = new PianoOscGUILevel(window->synth, this, y));
	window->subwindow->add_subwindow(phase = new PianoOscGUIPhase(window->synth, this, y));
	window->subwindow->add_subwindow(freq = new PianoOscGUIFreq(window->synth, this, y));
	return 1;
}




PianoOscGUILevel::PianoOscGUILevel(Piano *synth, PianoOscGUI *gui, int y)
 : BC_FPot(50,
 	y,
	synth->config.oscillator_config.values[gui->number]->level,
	INFINITYGAIN,
	0)
{
	this->synth = synth;
	this->gui = gui;
}

PianoOscGUILevel::~PianoOscGUILevel()
{
}

int PianoOscGUILevel::handle_event()
{
	PianoOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->level = get_value();
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}



PianoOscGUIPhase::PianoOscGUIPhase(Piano *synth, PianoOscGUI *gui, int y)
 : BC_IPot(125,
 	y,
	(int64_t)(synth->config.oscillator_config.values[gui->number]->phase * 360),
	0,
	360)
{
	this->synth = synth;
	this->gui = gui;
}

PianoOscGUIPhase::~PianoOscGUIPhase()
{
}

int PianoOscGUIPhase::handle_event()
{
	PianoOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->phase = (float)get_value() / 360;
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}



PianoOscGUIFreq::PianoOscGUIFreq(Piano *synth, PianoOscGUI *gui, int y)
 : BC_IPot(200,
 	y,
	(int64_t)(synth->config.oscillator_config.values[gui->number]->freq_factor),
	1,
	100)
{
	this->synth = synth;
	this->gui = gui;
}

PianoOscGUIFreq::~PianoOscGUIFreq()
{
}

int PianoOscGUIFreq::handle_event()
{
	PianoOscillatorConfig *config = synth->config.oscillator_config.values[gui->number];
	config->freq_factor = get_value();
	gui->window->canvas->update();
	synth->send_configure_change();
	return 1;
}







PianoAddOsc::PianoAddOsc(Piano *synth, PianoWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Add"))
{
	this->synth = synth;
	this->window = window;
}

PianoAddOsc::~PianoAddOsc()
{
}

int PianoAddOsc::handle_event()
{
	synth->add_oscillator();
	synth->send_configure_change();
	window->update_gui();
	return 1;
}



PianoDelOsc::PianoDelOsc(Piano *synth, PianoWindow *window, int x, int y)
 : BC_GenericButton(x, y, _("Delete"))
{
	this->synth = synth;
	this->window = window;
}

PianoDelOsc::~PianoDelOsc()
{
}

int PianoDelOsc::handle_event()
{
	synth->delete_oscillator();
	synth->send_configure_change();
	window->update_gui();
	return 1;
}


PianoScroll::PianoScroll(Piano *synth,
	PianoWindow *window,
	int x,
	int y,
	int h)
 : BC_ScrollBar(x,
 	y,
	SCROLL_VERT,
	h,
	synth->config.oscillator_config.total * OSCILLATORHEIGHT,
	0,
	window->subwindow->get_h())
{
	this->synth = synth;
	this->window = window;
}

PianoScroll::~PianoScroll()
{
}

int PianoScroll::handle_event()
{
	window->update_gui();
	return 1;
}








PianoSubWindow::PianoSubWindow(Piano *synth, int x, int y, int w, int h)
 : BC_SubWindow(x, y, w, h)
{
	this->synth = synth;
}
PianoSubWindow::~PianoSubWindow()
{
}









PianoClear::PianoClear(Piano *synth, int x, int y)
 : BC_GenericButton(x, y, _("Clear"))
{
	this->synth = synth;
}
PianoClear::~PianoClear()
{
}
int PianoClear::handle_event()
{
	synth->config.reset();
	synth->send_configure_change();
	synth->update_gui();
	return 1;
}






PianoWaveForm::PianoWaveForm(Piano *synth, int x, int y, char *text)
 : BC_PopupMenu(x, y, 120, text)
{
	this->synth = synth;
}

PianoWaveForm::~PianoWaveForm()
{
}

int PianoWaveForm::create_objects()
{
//	add_item(new PianoWaveFormItem(synth, _("DC"), DC));
	add_item(new PianoWaveFormItem(synth, _("Sine"), SINE));
	add_item(new PianoWaveFormItem(synth, _("Sawtooth"), SAWTOOTH));
	add_item(new PianoWaveFormItem(synth, _("Square"), SQUARE));
	add_item(new PianoWaveFormItem(synth, _("Triangle"), TRIANGLE));
	add_item(new PianoWaveFormItem(synth, _("Pulse"), PULSE));
	add_item(new PianoWaveFormItem(synth, _("Noise"), NOISE));
	return 0;
}

PianoWaveFormItem::PianoWaveFormItem(Piano *synth, char *text, int value)
 : BC_MenuItem(text)
{
	this->synth = synth;
	this->value = value;
}

PianoWaveFormItem::~PianoWaveFormItem()
{
}

int PianoWaveFormItem::handle_event()
{
	synth->config.wavefunction = value;
	synth->thread->window->canvas->update();
	synth->send_configure_change();
	return 1;
}


PianoWetness::PianoWetness(Piano *synth, int x, int y)
 : BC_FPot(x,
		y,
		synth->config.wetness,
		INFINITYGAIN,
		0)
{
	this->synth = synth;
}

int PianoWetness::handle_event()
{
	synth->config.wetness = get_value();
	synth->send_configure_change();
	return 1;
}



PianoFreqPot::PianoFreqPot(Piano *synth, PianoWindow *window, int x, int y)
 : BC_QPot(x, y, synth->config.base_freq)
{
	this->synth = synth;
}
PianoFreqPot::~PianoFreqPot()
{
}
int PianoFreqPot::handle_event()
{
	if(get_value() > 0 && get_value() < 30000)
	{
		synth->config.base_freq = get_value();
		freq_text->update(get_value());
		synth->send_configure_change();
	}
	return 1;
}



PianoBaseFreq::PianoBaseFreq(Piano *synth, int x, int y)
 : BC_TextBox(x, y, 70, 1, (int)synth->config.base_freq)
{
	this->synth = synth;
}
PianoBaseFreq::~PianoBaseFreq()
{
}
int PianoBaseFreq::handle_event()
{
	int new_value = atol(get_text());

	if(new_value > 0 && new_value < 30000)
	{
		synth->config.base_freq = new_value;
		freq_pot->update(synth->config.base_freq);
		synth->send_configure_change();
	}
	return 1;
}





PianoCanvas::PianoCanvas(Piano *synth,
	PianoWindow *window,
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

PianoCanvas::~PianoCanvas()
{
}

int PianoCanvas::update()
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
PianoLevelZero::PianoLevelZero(Piano *synth)
 : BC_MenuItem(_("Zero"))
{
	this->synth = synth;
}

PianoLevelZero::~PianoLevelZero()
{
}

int PianoLevelZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = INFINITYGAIN;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelMax::PianoLevelMax(Piano *synth)
 : BC_MenuItem(_("Maximum"))
{
	this->synth = synth;
}

PianoLevelMax::~PianoLevelMax()
{
}

int PianoLevelMax::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = 0;
	}
	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelNormalize::PianoLevelNormalize(Piano *synth)
 : BC_MenuItem(_("Normalize"))
{
	this->synth = synth;
}

PianoLevelNormalize::~PianoLevelNormalize()
{
}

int PianoLevelNormalize::handle_event()
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

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelSlope::PianoLevelSlope(Piano *synth)
 : BC_MenuItem(_("Slope"))
{
	this->synth = synth;
}

PianoLevelSlope::~PianoLevelSlope()
{
}

int PianoLevelSlope::handle_event()
{
	float slope = (float)INFINITYGAIN / synth->config.oscillator_config.total;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = i * slope;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelRandom::PianoLevelRandom(Piano *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
PianoLevelRandom::~PianoLevelRandom()
{
}

int PianoLevelRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level = -(rand() % -INFINITYGAIN);
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelInvert::PianoLevelInvert(Piano *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}
PianoLevelInvert::~PianoLevelInvert()
{
}

int PianoLevelInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->level =
			INFINITYGAIN - synth->config.oscillator_config.values[i]->level;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoLevelSine::PianoLevelSine(Piano *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}
PianoLevelSine::~PianoLevelSine()
{
}

int PianoLevelSine::handle_event()
{
	float new_value;

	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (float)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) * INFINITYGAIN / 2 + INFINITYGAIN / 2;
		synth->config.oscillator_config.values[i]->level = new_value;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

// ============================ phase calculations

PianoPhaseInvert::PianoPhaseInvert(Piano *synth)
 : BC_MenuItem(_("Invert"))
{
	this->synth = synth;
}
PianoPhaseInvert::~PianoPhaseInvert()
{
}

int PianoPhaseInvert::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase =
			1 - synth->config.oscillator_config.values[i]->phase;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoPhaseZero::PianoPhaseZero(Piano *synth)
 : BC_MenuItem(_("Zero"))
{
	this->synth = synth;
}
PianoPhaseZero::~PianoPhaseZero()
{
}

int PianoPhaseZero::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase = 0;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoPhaseSine::PianoPhaseSine(Piano *synth)
 : BC_MenuItem(_("Sine"))
{
	this->synth = synth;
}
PianoPhaseSine::~PianoPhaseSine()
{
}

int PianoPhaseSine::handle_event()
{
	float new_value;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		new_value = (float)i / synth->config.oscillator_config.total * 2 * M_PI;
		new_value = sin(new_value) / 2 + .5;
		synth->config.oscillator_config.values[i]->phase = new_value;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoPhaseRandom::PianoPhaseRandom(Piano *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
PianoPhaseRandom::~PianoPhaseRandom()
{
}

int PianoPhaseRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->phase =
			(float)(rand() % 360) / 360;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}


// ============================ freq calculations

PianoFreqRandom::PianoFreqRandom(Piano *synth)
 : BC_MenuItem(_("Random"))
{
	this->synth = synth;
}
PianoFreqRandom::~PianoFreqRandom()
{
}

int PianoFreqRandom::handle_event()
{
	srand(time(0));
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = rand() % 100;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoFreqEnum::PianoFreqEnum(Piano *synth)
 : BC_MenuItem(_("Enumerate"))
{
	this->synth = synth;
}
PianoFreqEnum::~PianoFreqEnum()
{
}

int PianoFreqEnum::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)i + 1;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoFreqEven::PianoFreqEven(Piano *synth)
 : BC_MenuItem(_("Even"))
{
	this->synth = synth;
}
PianoFreqEven::~PianoFreqEven()
{
}

int PianoFreqEven::handle_event()
{
	if(synth->config.oscillator_config.total)
		synth->config.oscillator_config.values[0]->freq_factor = (float)1;

	for(int i = 1; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)i * 2;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoFreqOdd::PianoFreqOdd(Piano *synth)
 : BC_MenuItem(_("Odd"))
{ this->synth = synth; }
PianoFreqOdd::~PianoFreqOdd()
{
}

int PianoFreqOdd::handle_event()
{
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = (float)1 + i * 2;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoFreqFibonacci::PianoFreqFibonacci(Piano *synth)
 : BC_MenuItem(_("Fibonnacci"))
{
	this->synth = synth;
}
PianoFreqFibonacci::~PianoFreqFibonacci()
{
}

int PianoFreqFibonacci::handle_event()
{
	float last_value1 = 0, last_value2 = 1;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = last_value1 + last_value2;
		if(synth->config.oscillator_config.values[i]->freq_factor > 100) synth->config.oscillator_config.values[i]->freq_factor = 100;
		last_value1 = last_value2;
		last_value2 = synth->config.oscillator_config.values[i]->freq_factor;
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

PianoFreqPrime::PianoFreqPrime(Piano *synth)
 : BC_MenuItem(_("Prime"))
{
	this->synth = synth;
}
PianoFreqPrime::~PianoFreqPrime()
{
}

int PianoFreqPrime::handle_event()
{
	float number = 1;
	for(int i = 0; i < synth->config.oscillator_config.total; i++)
	{
		synth->config.oscillator_config.values[i]->freq_factor = number;
		number = get_next_prime(number);
	}

	synth->thread->window->update_gui();
	synth->send_configure_change();
}

float PianoFreqPrime::get_next_prime(float number)
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








PianoOscillatorConfig::PianoOscillatorConfig(int number)
{
	reset();
	this->number = number;
}

PianoOscillatorConfig::~PianoOscillatorConfig()
{
}

void PianoOscillatorConfig::reset()
{
	level = 0;
	phase = 0;
	freq_factor = 1;
}

void PianoOscillatorConfig::read_data(FileXML *file)
{
	level = file->tag.get_property("LEVEL", (float)level);
	phase = file->tag.get_property("PHASE", (float)phase);
	freq_factor = file->tag.get_property("FREQFACTOR", (float)freq_factor);
}

void PianoOscillatorConfig::save_data(FileXML *file)
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

int PianoOscillatorConfig::equivalent(PianoOscillatorConfig &that)
{
	if(EQUIV(level, that.level) &&
		EQUIV(phase, that.phase) &&
		EQUIV(freq_factor, that.freq_factor))
		return 1;
	else
		return 0;
}

void PianoOscillatorConfig::copy_from(PianoOscillatorConfig& that)
{
	level = that.level;
	phase = that.phase;
	freq_factor = that.freq_factor;
}












PianoConfig::PianoConfig()
{
	reset();
}

PianoConfig::~PianoConfig()
{
	oscillator_config.remove_all_objects();
}

void PianoConfig::reset()
{
	wetness = 0;
	base_freq = 440;
	wavefunction = SINE;
	for(int i = 0; i < oscillator_config.total; i++)
	{
		oscillator_config.values[i]->reset();
	}
}

int PianoConfig::equivalent(PianoConfig &that)
{
//printf("PianoConfig::equivalent %d %d\n", base_freq, that.base_freq);
	if(base_freq != that.base_freq ||
		wavefunction != that.wavefunction ||
		oscillator_config.total != that.oscillator_config.total) return 0;

	for(int i = 0; i < oscillator_config.total; i++)
	{
		if(!oscillator_config.values[i]->equivalent(*that.oscillator_config.values[i]))
			return 0;
	}

	return 1;
}

void PianoConfig::copy_from(PianoConfig& that)
{
	wetness = that.wetness;
	base_freq = that.base_freq;
	wavefunction = that.wavefunction;

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
		oscillator_config.append(new PianoOscillatorConfig(i));
		oscillator_config.values[i]->copy_from(*that.oscillator_config.values[i]);
	}

	for( ;
		i < oscillator_config.total;
		i++)
	{
		oscillator_config.remove_object();
	}
}

void PianoConfig::interpolate(PianoConfig &prev,
	PianoConfig &next,
	int64_t prev_frame,
	int64_t next_frame,
	int64_t current_frame)
{
	double next_scale = (double)(current_frame - prev_frame) / (next_frame - prev_frame);
	double prev_scale = (double)(next_frame - current_frame) / (next_frame - prev_frame);

	copy_from(prev);
	wetness = (int)(prev.wetness * prev_scale + next.wetness * next_scale);
	base_freq = (int)(prev.base_freq * prev_scale + next.base_freq * next_scale);
}





