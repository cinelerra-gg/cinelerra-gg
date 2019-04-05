#ifdef HAVE_LV2

/*
 * CINELERRA
 * Copyright (C) 2018 GG
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

#include "bchash.h"
#include "clip.h"
#include "cstrdup.h"
#include "edl.h"
#include "edlsession.h"
#include "file.h"
#include "filesystem.h"
#include "filexml.h"
#include "language.h"
#include "mainerror.h"
#include "mwindow.h"
#include "plugin.h"
#include "pluginlv2client.h"
#include "pluginlv2config.h"
#include "pluginlv2ui.inc"
#include "pluginserver.h"
#include "pluginset.h"
#include "samples.h"
#include "track.h"

#include <ctype.h>
#include <string.h>

PluginLV2UIs PluginLV2ParentUI::plugin_lv2;

PluginLV2UIs::PluginLV2UIs()
 : Mutex("PluginLV2UIs")
{
}

PluginLV2UIs::~PluginLV2UIs()
{
	del_uis();
}

void PluginLV2UIs::del_uis()
{
	while( size() ) remove_object();
}

PluginLV2ParentUI *PluginLV2UIs::del_ui(PluginLV2Client *client)
{
	lock("PluginLV2UIs::del_ui client");
	int i = size();
	while( --i >= 0 && get(i)->client != client );
	PluginLV2ParentUI *ui = 0;
	if( i >= 0 ) {
		if( (ui=get(i))->gui ) ui->client = 0;
		else { remove_object_number(i);  ui = 0; }
	}
	unlock();
	return ui;
}
PluginLV2ParentUI *PluginLV2UIs::del_ui(PluginLV2ClientWindow *gui)
{
	lock("PluginLV2UIs::del_ui gui");
	int i = size();
	while( --i >= 0 && get(i)->gui != gui );
	PluginLV2ParentUI *ui = 0;
	if( i >= 0 ) {
		if( (ui=get(i))->client ) ui->gui = 0;
		else { remove_object_number(i);  ui = 0; }
	}
	unlock();
	return ui;
}

PluginLV2ParentUI *PluginLV2UIs::add_ui(PluginLV2ParentUI *ui, PluginLV2Client *client)
{
	ui->start_child();
	ui->start_parent(client);
	append(ui);
	return ui;
}

PluginLV2ParentUI *PluginLV2UIs::search_ui(Plugin *plugin)
{
	int64_t position = plugin->startproject;
	PluginSet *plugin_set = plugin->plugin_set;
	int set_no = plugin_set->get_number();
	int track_no = plugin_set->track->number_of();

	PluginLV2ParentUI *ui = 0;
	for( int i=size(); !ui && --i>=0; ) {
		PluginLV2ParentUI *parent_ui = get(i);
		if( parent_ui->position != position ) continue;
		if( parent_ui->set_no != set_no ) continue;
		if( parent_ui->track_no == track_no ) ui = parent_ui;
	}
	return ui;
}

PluginLV2ParentUI *PluginLV2UIs::find_ui(Plugin *plugin)
{
	if( !plugin ) return 0;
	lock("PluginLV2UIs::find_ui");
	PluginLV2ParentUI *ui = search_ui(plugin);
	unlock();
	return ui;
}
PluginLV2ParentUI *PluginLV2Client::find_ui()
{
	return PluginLV2ParentUI::plugin_lv2.find_ui(server->plugin);
}
PluginLV2ParentUI *PluginLV2ClientWindow::find_ui()
{
	return PluginLV2ParentUI::plugin_lv2.find_ui(client->server->plugin);
}

PluginLV2ParentUI *PluginLV2UIs::get_ui(PluginLV2Client *client)
{
	lock("PluginLV2UIs::get_ui");
	Plugin *plugin = client->server->plugin;
	PluginLV2ParentUI *ui = search_ui(plugin);
	if( !ui ) ui = add_ui(new PluginLV2ParentUI(plugin), client);
	unlock();
	return ui;
}
PluginLV2ParentUI *PluginLV2Client::get_ui()
{
	PluginLV2ParentUI *ui = PluginLV2ParentUI::plugin_lv2.get_ui(this);
	ui->client = this;
	return ui;
}
PluginLV2ParentUI *PluginLV2ClientWindow::get_ui()
{
	PluginLV2ParentUI *ui = PluginLV2ParentUI::plugin_lv2.get_ui(client);
	ui->gui = this;
	return ui;
}


PluginLV2Client::PluginLV2Client(PluginServer *server)
 : PluginAClient(server), PluginLV2()
{
	title[0] = 0;
}

PluginLV2Client::~PluginLV2Client()
{
	PluginLV2ParentUI::plugin_lv2.del_ui(this);
}

NEW_WINDOW_MACRO(PluginLV2Client, PluginLV2ClientWindow)

int PluginLV2Client::init_lv2()
{
	int bfrsz = block_length;
	EDL *edl = server->edl;
	if( edl ) {
		PlaybackConfig *playback_config = edl->session->playback_config;
		bfrsz = playback_config->aconfig->fragment_size;
	}
	int sample_rate = get_project_samplerate();
	if( sample_rate < 64 ) sample_rate = samplerate;
	return PluginLV2::init_lv2(config, sample_rate, bfrsz);
}

int PluginLV2Client::load_configuration()
{
	int64_t src_position =  get_source_position();
	KeyFrame *prev_keyframe = get_prev_keyframe(src_position);
	if( prev_keyframe->is_default ) return 0;
	PluginLV2ClientConfig curr_config;
	curr_config.copy_from(config);
	read_data(prev_keyframe);
	int ret = !config.equivalent(curr_config) ? 1 : 0;
	return ret;
}

void PluginLV2Client::update_gui()
{
	PluginClientThread *thread = get_thread();
	if( !thread ) return;
	PluginLV2ClientWindow *gui = (PluginLV2ClientWindow*)thread->get_window();
	gui->lock_window("PluginFClient::update_gui");
	load_configuration();
	if( config.update() > 0 ) {
		gui->update_selected();
		update_lv2(LV2_UPDATE);
	}
	gui->unlock_window();
}

void PluginLV2Client::update_lv2(int token)
{
	PluginLV2ParentUI *ui = find_ui();
	if( !ui ) return;
	ui->send_child(token, config.ctls, sizeof(float)*config.nb_ports);
}


int PluginLV2Client::is_realtime() { return 1; }
int PluginLV2Client::is_multichannel() { return nb_inputs > 1 || nb_outputs > 1 ? 1 : 0; }
const char* PluginLV2Client::plugin_title() { return title; }
int PluginLV2Client::uses_gui() { return 1; }
int PluginLV2Client::is_synthesis() { return 1; }

char* PluginLV2Client::to_string(char *string, const char *input)
{
	char *bp = string;
	for( const char *cp=input; *cp; ++cp )
		*bp++ = isalnum(*cp) ? *cp : '_';
	*bp = 0;
	return string;
}

void PluginLV2Client::save_data(KeyFrame *keyframe)
{
	FileXML output;
	output.set_shared_output(keyframe->xbuf);
	char name[BCTEXTLEN];  to_string(name, plugin_title());
	output.tag.set_title(name);
	for( int i=0; i<config.size(); ++i ) {
		PluginLV2Client_Opt *opt = config[i];
		output.tag.set_property(opt->get_symbol(), opt->get_value());
	}
	output.append_tag();
	output.terminate_string();
}

void PluginLV2Client::read_data(KeyFrame *keyframe)
{
	FileXML input;
	input.set_shared_input(keyframe->xbuf);
	char name[BCTEXTLEN];  to_string(name, plugin_title());

	while( !input.read_tag() ) {
		if( !input.tag.title_is(name) ) continue;
		for( int i=0; i<config.size(); ++i ) {
			PluginLV2Client_Opt *opt = config[i];
			float value = input.tag.get_property(opt->get_symbol(), 0.);
			opt->set_value(value);
		}
	}
}

void PluginLV2Client::load_buffer(int samples, Samples **input, int ich)
{
	for( int i=0; i<nb_inputs; ++i ) {
		int k = i < ich ? i : 0;
		double *inp = input[k]->get_data();
		float *ip = in_buffers[i];
		for( int j=samples; --j>=0; *ip++=*inp++ );
	}
	for( int i=0; i<nb_outputs; ++i )
		bzero(out_buffers[i], samples*sizeof(float));
}

int PluginLV2Client::unload_buffer(int samples, Samples **output, int och)
{
	if( nb_outputs < och ) och = nb_outputs;
	for( int i=0; i<och; ++i ) {
		double *outp = output[i]->get_data();
		float *op = out_buffers[i];
		for( int j=samples; --j>=0; *outp++=*op++ );
	}
	return samples;
}

void PluginLV2Client::process_buffer(int size)
{
	PluginLV2ParentUI *ui = get_ui();
	if( !ui ) return;
	shm_bfr->done = 0;
	ui->send_child(LV2_SHMID, &shmid, sizeof(shmid));
// timeout 10*duration, min 2sec, max 10sec
	double sample_rate = get_project_samplerate();
	double t = !sample_rate ? 2. : 10. * size / sample_rate;
	bclamp(t, 2., 10.);
	ui->output_bfr->timed_lock(t*1e6, "PluginLV2Client::process_buffer");
	if( !shm_bfr->done )
		eprintf("timeout: %s",server->title);
}

int PluginLV2Client::process_realtime(int64_t size,
	Samples **input_ptr, Samples **output_ptr, int chs)
{
	int64_t pos = get_source_position();
	int64_t end = pos + size;
	int64_t samples = 0;
	while( pos < end ) {
		KeyFrame *prev_keyframe = get_prev_keyframe(pos);
		if( !prev_keyframe->is_default ) {
			read_data(prev_keyframe);
			update_lv2(LV2_CONFIG);
		}
		KeyFrame *next_keyframe = get_next_keyframe(pos);
		int64_t next_pos = next_keyframe->position;
		if( pos >= next_pos || next_pos > end )
			next_pos = end;
		int64_t len = next_pos - pos;
		if( len > block_length )
			len = block_length;
		if( pos + len > end )
			len = end - pos;
		init_buffer(len);
		for( int i=chs; --i>=0; ) {
			input_ptr[i]->set_offset(samples);
			output_ptr[i]->set_offset(samples);
		}
		load_buffer(len, input_ptr, chs);
		process_buffer(len);
		unload_buffer(len, output_ptr, chs);
		samples += len;
		pos += len;
	}
	for( int i=chs; --i>=0; )
		output_ptr[i]->set_offset(0);
	return samples;
}

int PluginLV2Client::process_realtime(int64_t size,
	Samples **input_ptr, Samples **output_ptr)
{
	return process_realtime(size, input_ptr, output_ptr,
		PluginClient::total_out_buffers);
}

int PluginLV2Client::process_realtime(int64_t size,
	Samples *input_ptr, Samples *output_ptr)
{
	return process_realtime(size, &input_ptr, &output_ptr, 1);
}

PluginLV2BlackList::PluginLV2BlackList(const char *path)
{
	set_array_delete();
	char lv2_blacklist_path[BCTEXTLEN];
	sprintf(lv2_blacklist_path, "%s/%s", File::get_cindat_path(), path);
	FILE *bfp = fopen(lv2_blacklist_path, "r");
	if( !bfp ) return;
	while( fgets(lv2_blacklist_path, sizeof(lv2_blacklist_path), bfp) ) {
		if( lv2_blacklist_path[0] == '#' ) continue;
		int len = strlen(lv2_blacklist_path);
		if( len > 0 && lv2_blacklist_path[len-1] == '\n' )
			lv2_blacklist_path[len-1] = 0;
		if( !lv2_blacklist_path[0] ) continue;
		append(cstrdup(lv2_blacklist_path));
	}
	fclose(bfp);
}

PluginLV2BlackList::~PluginLV2BlackList()
{
	remove_all_objects();
}

int PluginLV2BlackList::is_badboy(const char *uri)
{
	FileSystem fs;
	for( int i=size(); --i>=0; )
		if( !fs.test_filter(uri, get(i)) ) return 1;
	return 0;
}

PluginServer* MWindow::new_lv2_server(MWindow *mwindow, const char *name)
{
	return new PluginServer(mwindow, name, PLUGIN_TYPE_LV2);
}

PluginClient *PluginServer::new_lv2_plugin()
{
	PluginLV2Client *client = new PluginLV2Client(this);
	if( client->load_lv2(path, client->title) ) { delete client;  return client = 0; }
	client->init_lv2();
	return client;
}

int MWindow::init_lv2_index(MWindow *mwindow, Preferences *preferences, FILE *fp)
{
	printf("init lv2 index:\n");
	PluginLV2BlackList blacklist("lv2_blacklist.txt");

	LilvWorld *world = lilv_world_new();
	lilv_world_load_all(world);
	const LilvPlugins *all_plugins = lilv_world_get_all_plugins(world);

	LILV_FOREACH(plugins, i, all_plugins) {
		const LilvPlugin *lilv = lilv_plugins_get(all_plugins, i);
		const char *uri = lilv_node_as_uri(lilv_plugin_get_uri(lilv));
		if( blacklist.is_badboy(uri) ) continue;
printf("LOAD: %s\n", uri);
		PluginServer server(mwindow, uri, PLUGIN_TYPE_LV2);
		int result = server.open_plugin(1, preferences, 0, 0);
		if( !result ) {
			server.write_table(fp, uri, PLUGIN_LV2_ID, 0);
			server.close_plugin();
		}
	}

	lilv_world_free(world);
	return 0;
}

PluginLV2ParentUI::PluginLV2ParentUI(Plugin *plugin)
{
	this->position = plugin->startproject;
	PluginSet *plugin_set = plugin->plugin_set;
	if( plugin_set ) {
		this->set_no = plugin_set->get_number();
		this->track_no = plugin_set->track->number_of();
	}
	else
		this->track_no = this->set_no = -1;

	output_bfr = new Condition(0, "PluginLV2ParentUI::output_bfr", 1);
	client = 0;
	gui = 0;
	hidden = -1;
}

PluginLV2ParentUI::~PluginLV2ParentUI()
{
	stop();
	delete output_bfr;
}

void PluginLV2ParentUI::start_parent(PluginLV2Client *client)
{
	start();
	const char *path = client->server->path;
	int len = sizeof(open_bfr_t) + strlen(path);
	char bfr[len];  memset(bfr, 0, len);
	open_bfr_t *open_bfr = (open_bfr_t *)bfr;
	open_bfr->sample_rate = client->get_project_samplerate();
	PlaybackConfig *playback_config = client->server->edl->session->playback_config;
	open_bfr->bfrsz = playback_config->aconfig->fragment_size;
	strcpy(open_bfr->path, path);
	send_child(LV2_OPEN, open_bfr, len);
	PluginLV2ClientConfig &conf = client->config;
	send_child(LV2_LOAD, conf.ctls, conf.nb_ports*sizeof(float));
}

int PluginLV2ParentUI::handle_parent()
{
	int result = 1;

	switch( parent_token ) {
	case LV2_UPDATE: {
		if( !gui ) break;
		gui->lv2_update((float *)parent_data);
		break; }
	case LV2_HIDE: {
		hidden = 1;
		break; }
	case LV2_SHOW: {
		if( hidden < 0 )
			gui->lv2_ui_enable();
		hidden = 0;
		break; }
	case LV2_SHMID: {
		output_bfr->unlock();
		break; }
	case EXIT_CODE: {
		hidden = 1;
		output_bfr->unlock();
		result = -1;
		break; }
	default:
		result = 0;
		break;
	}

	return result;
}

int PluginLV2ParentUI::show()
{
	if( !hidden ) return 1;
	send_child(LV2_SHOW, 0, 0);
	return 0;
}

int PluginLV2ParentUI::hide()
{
	if( hidden ) return 1;
	send_child(LV2_HIDE, 0, 0);
	return 0;
}

ForkChild *PluginLV2ParentUI::new_fork()
{
	return new PluginLV2ChildUI();
}

// stub in parent
int PluginLV2ChildUI::child_iteration(int64_t usec) { return -1; }
int PluginLV2ChildUI::send_host(int64_t token, const void *data, int bytes) { return -1; }

#else
#include "mwindow.h"
#include "pluginserver.h"

PluginServer* MWindow::new_lv2_server(MWindow *mwindow, const char *name) { return 0; }
PluginClient *PluginServer::new_lv2_plugin() { return 0; }
int MWindow::init_lv2_index(MWindow *mwindow, Preferences *preferences, FILE *fp) { return 0; }

#endif /* HAVE_LV2 */
