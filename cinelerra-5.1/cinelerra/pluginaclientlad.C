
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

#include "clip.h"
#include "bchash.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "mwindow.h"
#include "pluginaclientlad.h"
#include "pluginserver.h"
#include "samples.h"
#include "vframe.h"

#include <ctype.h>
#include <string.h>



PluginAClientConfig::PluginAClientConfig()
{
	reset();
}

PluginAClientConfig::~PluginAClientConfig()
{
	delete_objects();
}

void PluginAClientConfig::reset()
{
	total_ports = 0;
	port_data = 0;
	port_type = 0;
}

void PluginAClientConfig::delete_objects()
{
	delete [] port_data;  port_data = 0;
	delete [] port_type;  port_type = 0;
	reset();
}


int PluginAClientConfig::equivalent(PluginAClientConfig &that)
{
	if(that.total_ports != total_ports) return 0;
	for(int i = 0; i < total_ports; i++)
		if(!EQUIV(port_data[i], that.port_data[i])) return 0;
	return 1;
}

// Need PluginServer to do this.
void PluginAClientConfig::copy_from(PluginAClientConfig &that)
{
	if( total_ports != that.total_ports ) {
		delete_objects();
		total_ports = that.total_ports;
		port_data = new LADSPA_Data[total_ports];
		port_type = new int[total_ports];
	}
	for( int i=0; i<total_ports; ++i ) {
		port_type[i] = that.port_type[i];
		port_data[i] = that.port_data[i];
	}
}

void PluginAClientConfig::interpolate(PluginAClientConfig &prev, PluginAClientConfig &next,
	int64_t prev_frame, int64_t next_frame, int64_t current_frame)
{
	copy_from(prev);
}

void PluginAClientConfig::initialize(PluginServer *server)
{
	delete_objects();

	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
	int port_count = lad_desc->PortCount;
	for(int i = 0; i < port_count; i++) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
		++total_ports;
	}

	port_data = new LADSPA_Data[total_ports];
	port_type = new int[total_ports];

	for(int port = 0, i = 0; i < port_count; i++) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
		const LADSPA_PortRangeHint *lad_hint = &lad_desc->PortRangeHints[i];
		LADSPA_PortRangeHintDescriptor hint_desc = lad_hint->HintDescriptor;
// Convert LAD default to default value
		float value = 0.0;

// Store type of port for GUI use
		port_type[port] = PORT_NORMAL;
		if( LADSPA_IS_HINT_SAMPLE_RATE(hint_desc) )
			port_type[port] = PORT_FREQ_INDEX;
		else if(LADSPA_IS_HINT_TOGGLED(hint_desc))
			port_type[port] = PORT_TOGGLE;
		else if(LADSPA_IS_HINT_INTEGER(hint_desc))
			port_type[port] = PORT_INTEGER;

// Get default of port using crazy hinting system
		if( LADSPA_IS_HINT_DEFAULT_0(hint_desc) )
			value = 0.0;
		else if( LADSPA_IS_HINT_DEFAULT_1(hint_desc) )
			value = 1.0;
		else if( LADSPA_IS_HINT_DEFAULT_100(hint_desc) )
			value = 100.0;
		else if( LADSPA_IS_HINT_DEFAULT_440(hint_desc) )
			value = 440.0;
		else if( LADSPA_IS_HINT_DEFAULT_MAXIMUM(hint_desc) )
			value = lad_hint->UpperBound;
		else if( LADSPA_IS_HINT_DEFAULT_MINIMUM(hint_desc) )
			value = lad_hint->LowerBound;
		else if( LADSPA_IS_HINT_DEFAULT_LOW(hint_desc) )
			value = LADSPA_IS_HINT_LOGARITHMIC(hint_desc) ?
				exp(log(lad_hint->LowerBound) * 0.25 +
					log(lad_hint->UpperBound) * 0.75) :
				lad_hint->LowerBound * 0.25 +
					lad_hint->UpperBound * 0.75;
		else if( LADSPA_IS_HINT_DEFAULT_MIDDLE(hint_desc) )
			value = LADSPA_IS_HINT_LOGARITHMIC(hint_desc) ?
				exp(log(lad_hint->LowerBound) * 0.5 +
					log(lad_hint->UpperBound) * 0.5) :
				lad_hint->LowerBound * 0.5 +
					lad_hint->UpperBound * 0.5;
		else if( LADSPA_IS_HINT_DEFAULT_HIGH(hint_desc) )
			value = LADSPA_IS_HINT_LOGARITHMIC(hint_desc) ?
				exp(log(lad_hint->LowerBound) * 0.75 +
					log(lad_hint->UpperBound) * 0.25) :
				lad_hint->LowerBound * 0.75 +
					lad_hint->UpperBound * 0.25;

		port_data[port] = value;
		++port;
	}
}


PluginACLientToggle::PluginACLientToggle(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output)
 : BC_CheckBox(x, y, (int)(*output))
{
	this->plugin = plugin;
	this->output = output;
}

int PluginACLientToggle::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientILinear::PluginACLientILinear(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output,
	int min,
	int max)
 : BC_IPot(x, y, (int)(*output), min, max)
{
	this->plugin = plugin;
	this->output = output;
}

int PluginACLientILinear::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientFLinear::PluginACLientFLinear(PluginAClientLAD *plugin,
	int x,
	int y,
	LADSPA_Data *output,
	float min,
	float max)
 : BC_FPot(x, y, *output, min, max)
{
	this->plugin = plugin;
	this->output = output;
	set_precision(0.01);
}

int PluginACLientFLinear::handle_event()
{
	*output = get_value();
	plugin->send_configure_change();
	return 1;
}


PluginACLientFreq::PluginACLientFreq(PluginAClientLAD *plugin,
	int x, int y, LADSPA_Data *output, int translate_linear)
 : BC_QPot(x, y, translate_linear ?
		(int)(*output * plugin->PluginAClient::project_sample_rate) :
		(int)*output)
{
	this->plugin = plugin;
	this->output = output;
	this->translate_linear = translate_linear;
}

int PluginACLientFreq::handle_event()
{
	*output = translate_linear ?
		(float)get_value() / plugin->PluginAClient::project_sample_rate :
		get_value();
	plugin->send_configure_change();
	return 1;
}



PluginAClientWindow::PluginAClientWindow(PluginAClientLAD *plugin)
 : PluginClientWindow(plugin, 500, plugin->config.total_ports * 30 + 60,
	500, plugin->config.total_ports * 30 + 60, 0)
{
	this->plugin = plugin;
}

PluginAClientWindow::~PluginAClientWindow()
{
}


void PluginAClientWindow::create_objects()
{
	PluginServer *server = plugin->server;
	char string[BCTEXTLEN];
	int current_port = 0;
	int x = 10;
	int y = 10;
	int x2 = 300;
	int x3 = 335;
	int title_vmargin = 5;
	int max_w = 0;
	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
	int port_count = lad_desc->PortCount;
	for(int i = 0; i < port_count; i++) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
		int w = get_text_width(MEDIUMFONT, (char*)lad_desc->PortNames[i]);
		if(w > max_w) max_w = w;
	}

	x2 = max_w + 20;
	x3 = max_w + 55;
	for(int i = 0; i < port_count; i++) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
		const LADSPA_PortRangeHint *lad_hint = &lad_desc->PortRangeHints[i];
		LADSPA_PortRangeHintDescriptor hint_desc = lad_hint->HintDescriptor;
		int use_min = LADSPA_IS_HINT_BOUNDED_BELOW(hint_desc);
		int use_max = LADSPA_IS_HINT_BOUNDED_ABOVE(hint_desc);
		sprintf(string, "%s:", lad_desc->PortNames[i]);
// printf("PluginAClientWindow::create_objects 1 %s type=%d lower: %d %f upper: %d %f\n",
// string, plugin->config.port_type[current_port],
// use_min, lad_hint->LowerBound,  use_max, lad_hint->UpperBound);

		switch(plugin->config.port_type[current_port]) {
		case PluginAClientConfig::PORT_NORMAL: {
			PluginACLientFLinear *flinear;
			float min = use_min ? lad_hint->LowerBound : 0;
			float max = use_max ? lad_hint->UpperBound : 100;
			add_subwindow(new BC_Title(x, y + title_vmargin, string));
			add_subwindow(flinear = new PluginACLientFLinear(
					plugin, (current_port % 2) ? x2 : x3, y,
					&plugin->config.port_data[current_port],
					min, max));
			fpots.append(flinear);
			break; }
		case PluginAClientConfig::PORT_FREQ_INDEX: {
			PluginACLientFreq *freq;
			add_subwindow(new BC_Title(x, y + title_vmargin, string));
			add_subwindow(freq = new PluginACLientFreq(plugin,
					(current_port % 2) ? x2 : x3, y,
						&plugin->config.port_data[current_port], 0
					/*	(plugin->config.port_type[current_port] ==
							PluginAClientConfig::PORT_FREQ_INDEX */));
			freqs.append(freq);
			break; }
		case PluginAClientConfig::PORT_TOGGLE: {
			PluginACLientToggle *toggle;
			add_subwindow(new BC_Title(x, y + title_vmargin, string));
			add_subwindow(toggle = new PluginACLientToggle(plugin,
					(current_port % 2) ? x2 : x3, y,
						&plugin->config.port_data[current_port]));
			toggles.append(toggle);
			break; }
		case PluginAClientConfig::PORT_INTEGER: {
			PluginACLientILinear *ilinear;
			float min = use_min ? lad_hint->LowerBound : 0;
			float max = use_max ? lad_hint->UpperBound : 100;
			add_subwindow(new BC_Title(x, y + title_vmargin, string));
			add_subwindow(ilinear = new PluginACLientILinear( plugin,
					(current_port % 2) ? x2 : x3, y,
						&plugin->config.port_data[current_port],
						(int)min, (int)max));
			ipots.append(ilinear);
			break; }
		}
		current_port++;
		y += 30;
//printf("PluginAClientWindow::create_objects 2\n");
	}

	y += 10;
	sprintf(string, _("Author: %s"), lad_desc->Maker);
	add_subwindow(new BC_Title(x, y, string));
	y += 20;
	sprintf(string, _("License: %s"), lad_desc->Copyright);
	add_subwindow(new BC_Title(x, y, string));
	show_window(1);
}



PluginAClientLAD::PluginAClientLAD(PluginServer *server)
 : PluginAClient(server)
{
	in_buffers = 0;
	total_inbuffers = 0;
	out_buffers = 0;
	total_outbuffers = 0;
	buffer_allocation = 0;
	lad_instance = 0;
	snprintf(title, sizeof(title), "L_%s", server->lad_descriptor->Name);
}

PluginAClientLAD::~PluginAClientLAD()
{
	delete_buffers();
	delete_plugin();
}

NEW_WINDOW_MACRO(PluginAClientLAD, PluginAClientWindow)

int PluginAClientLAD::is_realtime()
{
	return 1;
}

int PluginAClientLAD::is_multichannel()
{
	if(get_inchannels() > 1 || get_outchannels() > 1) return 1;
	return 0;
}

int PluginAClientLAD::get_inchannels()
{
	int result = 0;
	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
	int port_count = lad_desc->PortCount;
	for( int i = 0; i < port_count; ++i ) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_AUDIO(port_desc[i]) ) continue;
		++result;
	}
	return result;
}

int PluginAClientLAD::get_outchannels()
{
	int result = 0;
	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
	int port_count = lad_desc->PortCount;
	for( int i = 0; i < port_count; ++i ) {
		if( !LADSPA_IS_PORT_OUTPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_AUDIO(port_desc[i]) ) continue;
		++result;
	}
	return result;
}


const char* PluginAClientLAD::plugin_title()
{
	return title;
}

int PluginAClientLAD::uses_gui()
{
	return 1;
}

int PluginAClientLAD::is_synthesis()
{
	return 1;
}

LOAD_CONFIGURATION_MACRO(PluginAClientLAD, PluginAClientConfig)

void PluginAClientLAD::update_gui()
{
}

char* PluginAClientLAD::lad_to_string(char *string, const char *input)
{
	strcpy(string, input);
	int len = strlen(string);
	for(int j = 0; j < len; j++)
	{
		if(string[j] == ' ' ||
			string[j] == '<' ||
			string[j] == '>') string[j] = '_';
	}
	return string;
}

char* PluginAClientLAD::lad_to_upper(char *string, const char *input)
{
	lad_to_string(string, input);
	int len = strlen(string);
	for(int j = 0; j < len; j++)
		string[j] = toupper(string[j]);
	return string;
}


void PluginAClientLAD::save_data(KeyFrame *keyframe)
{
	if( !config.port_data ) config.initialize(server);
	FileXML output;
// cause data to be stored directly in text
	output.set_shared_output(keyframe->xbuf);
	char string[BCTEXTLEN];
	output.tag.set_title(lad_to_upper(string, plugin_title()));

	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
//printf("PluginAClientLAD::save_data %d\n", lad_desc->PortCount);
	int port_count = lad_desc->PortCount;
	for(int port = 0, i = 0; i < port_count; i++) {
		if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
		if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
// Convert LAD port name to default title
		PluginAClientLAD::lad_to_upper(string,
			(char*)lad_desc->PortNames[i]);
		output.tag.set_property(string, config.port_data[port]);
//printf("PluginAClientLAD::save_data %d %f\n", port, config.port_data[port]);
		++port;
	}

	output.append_tag();
	output.terminate_string();
}

void PluginAClientLAD::read_data(KeyFrame *keyframe)
{
	if( !config.port_data ) config.initialize(server);
	FileXML input;
	input.set_shared_input(keyframe->xbuf);

	while(! input.read_tag() ) {
//printf("PluginAClientLAD::read_data %s\n", input.tag.get_title());
		char string[BCTEXTLEN];
		if(! input.tag.title_is(lad_to_upper(string, plugin_title())) ) continue;
		const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
		const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
		int port_count = lad_desc->PortCount;
		for(int port = 0, i = 0; i < port_count; i++) {
			if( !LADSPA_IS_PORT_INPUT(port_desc[i]) ) continue;
			if( !LADSPA_IS_PORT_CONTROL(port_desc[i]) ) continue;
			PluginAClientLAD::lad_to_upper(string, (char*)lad_desc->PortNames[i]);
			config.port_data[port] =
				input.tag.get_property(string, config.port_data[port]);
//printf("PluginAClientLAD::read_data %d %f\n", port, config.port_data[port]);
			port++;
		}
	}
}

void PluginAClientLAD::delete_buffers()
{
	if( in_buffers ) {
		for( int i=0; i<total_inbuffers; ++i ) delete [] in_buffers[i];
		delete [] in_buffers;  in_buffers = 0;
	}
	if( out_buffers ) {
		for( int i=0; i<total_outbuffers; ++i ) delete [] out_buffers[i];
		delete [] out_buffers;  out_buffers = 0;
	}
	total_inbuffers = 0;
	total_outbuffers = 0;
	buffer_allocation = 0;
}

void PluginAClientLAD::delete_plugin()
{
	if(lad_instance) {
		const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
		if( lad_desc->deactivate ) lad_desc->deactivate(lad_instance);
		lad_desc->cleanup(lad_instance);
	}
	lad_instance = 0;
}

void PluginAClientLAD::init_plugin(int total_in, int total_out, int size)
{
	int need_reconfigure = !lad_instance ? 1 : 0;
	if( !config.port_data ) {
		config.initialize(server);
		need_reconfigure = 1;
	}
	if(buffer_allocation && buffer_allocation < size) {
		delete_buffers();
		need_reconfigure = 1;
	}

	buffer_allocation = MAX(size, buffer_allocation);
	if(!in_buffers) {
		total_inbuffers = total_in;
		in_buffers = new LADSPA_Data*[total_inbuffers];
		for(int i = 0; i < total_inbuffers; i++)
			in_buffers[i] = new LADSPA_Data[buffer_allocation];
		need_reconfigure = 1;
	}

	if(!out_buffers) {
		total_outbuffers = total_out;
		out_buffers = new LADSPA_Data*[total_outbuffers];
		for(int i = 0; i < total_outbuffers; i++)
			out_buffers[i] = new LADSPA_Data[buffer_allocation];
		need_reconfigure = 1;
	}

	if( load_configuration() )
		need_reconfigure = 1;

	const LADSPA_Descriptor *lad_desc = server->lad_descriptor;
	if( need_reconfigure ) {
		need_reconfigure = 0;
		delete_plugin();
		lad_instance = lad_desc->instantiate(
				lad_desc,PluginAClient::project_sample_rate);
		const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
		int port_count = lad_desc->PortCount;

		for(int port = 0, i = 0; i < port_count; i++) {
			if( LADSPA_IS_PORT_INPUT(port_desc[i]) &&
			    LADSPA_IS_PORT_CONTROL(port_desc[i]) ) {
				lad_desc->connect_port(lad_instance, i,
					config.port_data + port);
				port++;
			}
			else if( LADSPA_IS_PORT_OUTPUT(port_desc[i]) &&
				 LADSPA_IS_PORT_CONTROL(port_desc[i]) ) {
				lad_desc->connect_port(lad_instance, i,
					&dummy_control_output);
			}
		}
		if(lad_desc->activate) lad_desc->activate(lad_instance);
	}

	const LADSPA_PortDescriptor *port_desc = lad_desc->PortDescriptors;
	int port_count = lad_desc->PortCount;
	for(int port = 0, i = 0; i < port_count; i++) {
		if( LADSPA_IS_PORT_INPUT(port_desc[i]) &&
		    LADSPA_IS_PORT_AUDIO(port_desc[i]) )
			lad_desc->connect_port(lad_instance, i, in_buffers[port++]);
	}

	for(int port = 0, i = 0; i < port_count; i++) {
		if( LADSPA_IS_PORT_OUTPUT(port_desc[i]) &&
		    LADSPA_IS_PORT_AUDIO(port_desc[i]) )
			lad_desc->connect_port(lad_instance, i, out_buffers[port++]);
	}
}

int PluginAClientLAD::process_realtime(int64_t size,
	Samples *input_ptr,
	Samples *output_ptr)
{
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();
	init_plugin(in_channels, out_channels, size);

	double *input_samples = input_ptr->get_data();
	for(int i = 0; i < in_channels; i++) {
		LADSPA_Data *in_buffer = in_buffers[i];
		for(int j = 0; j < size; j++) in_buffer[j] = input_samples[j];
	}
	for(int i = 0; i < out_channels; i++)
		bzero(out_buffers[i], sizeof(float) * size);

	server->lad_descriptor->run(lad_instance, size);

	double *output_samples = output_ptr->get_data();
	LADSPA_Data *out_buffer = out_buffers[0];
	for(int i = 0; i < size; i++) output_samples[i] = out_buffer[i];
	return size;
}

int PluginAClientLAD::process_realtime(int64_t size,
	Samples **input_ptr, Samples **output_ptr)
{
	int in_channels = get_inchannels();
	int out_channels = get_outchannels();
	init_plugin(in_channels, out_channels, size);

	for(int i = 0; i < in_channels; i++) {
		float *in_buffer = in_buffers[i];
		int k = i < PluginClient::total_in_buffers ? i : 0;
		double *in_ptr = input_ptr[k]->get_data();
		for(int j = 0; j < size; j++) in_buffer[j] = in_ptr[j];
	}
	for(int i = 0; i < out_channels; i++)
		bzero(out_buffers[i], sizeof(float) * size);

	server->lad_descriptor->run(lad_instance, size);

	int nbfrs = PluginClient::total_out_buffers;
	if( out_channels < nbfrs ) nbfrs = out_channels;
	for(int i = 0; i < nbfrs; i++) {
		LADSPA_Data *out_buffer = out_buffers[i];
		double *out_ptr = output_ptr[i]->get_data();
		for(int j = 0; j < size; j++) out_ptr[j] = out_buffer[j];
	}
	return size;
}

int MWindow::init_ladspa_index(MWindow *mwindow, Preferences *preferences,
	FILE *fp, const char *plugin_dir)
{
	char plugin_path[BCTEXTLEN], *path = FileSystem::basepath(plugin_dir);
	strcpy(plugin_path, path);  delete [] path;
	printf("init ladspa index: %s\n", plugin_dir);
	fprintf(fp, "%d\n", PLUGIN_FILE_VERSION);
	fprintf(fp, "%s\n", plugin_dir);
	init_plugin_index(mwindow, preferences, fp, plugin_path);
	return 0;
}

