
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

#ifndef PLUGINCLIENT_H
#define PLUGINCLIENT_H

// Base class inherited by all the different types of plugins.

#define MAX_FRAME_BUFFER 1024

class PluginClient;


#include "arraylist.h"
#include "bchash.inc"
#include "condition.h"
#include "edlsession.inc"
#include "keyframe.h"
#include "mainprogress.inc"
#include "maxbuffers.h"
#include "pluginclient.inc"
#include "plugincommands.h"
#include "pluginserver.inc"
#include "samples.inc"
#include "theme.inc"
#include "vframe.h"


extern "C"
{
extern PluginClient* new_plugin(PluginServer *server);
}

class PluginClientAuto
{
public:
	float position;
	float intercept;
	float slope;
};




// Convenience functions

#define REGISTER_PLUGIN(class_title) \
PluginClient* new_plugin(PluginServer *server) \
{ \
	return new class_title(server); \
}








// Prototypes for user to put in class header
#define PLUGIN_CLASS_MEMBERS(config_name) \
	int load_configuration(); \
	const char* plugin_title(); \
	PluginClientWindow* new_window(); \
	config_name config;



// Prototypes for user to put in class header
#define PLUGIN_CLASS_MEMBERS2(config_name) \
	int load_configuration(); \
	const char* plugin_title(); \
	PluginClientWindow* new_window(); \
	config_name config;





#define NEW_WINDOW_MACRO(plugin_class, window_class) \
PluginClientWindow* plugin_class::new_window() \
{ \
	return new window_class(this); \
}

// Not all plugins load configuration the same way.  Use this to define
// the normal way.
#define LOAD_CONFIGURATION_MACRO(plugin_class, config_class) \
int plugin_class::load_configuration() \
{ \
	KeyFrame *prev_keyframe, *next_keyframe; \
	prev_keyframe = get_prev_keyframe(get_source_position()); \
	next_keyframe = get_next_keyframe(get_source_position()); \
 \
 	int64_t next_position = edl_to_local(next_keyframe->position); \
 	int64_t prev_position = edl_to_local(prev_keyframe->position); \
 \
	config_class old_config, prev_config, next_config; \
	old_config.copy_from(config); \
	read_data(prev_keyframe); \
	prev_config.copy_from(config); \
	read_data(next_keyframe); \
	next_config.copy_from(config); \
 \
	config.interpolate(prev_config,  \
		next_config,  \
		(next_position == prev_position) ? \
			get_source_position() : \
			prev_position, \
		(next_position == prev_position) ? \
			get_source_position() + 1 : \
			next_position, \
		get_source_position()); \
 \
	if(!config.equivalent(old_config)) \
		return 1; \
	else \
		return 0; \
}




class PluginClientWindow : public BC_Window
{
public:
	PluginClientWindow(PluginClient *client,
		int w,
		int h,
		int min_w,
		int min_h,
		int allow_resize);
	PluginClientWindow(const char *title,
		int x,
		int y,
		int w,
		int h,
		int min_w,
		int min_h,
		int allow_resize);
	virtual ~PluginClientWindow();

	virtual int translation_event();
	virtual int close_event();
	virtual void done_event(int result) {}

	PluginClient *client;
};




class PluginClientThread : public Thread
{
public:
	PluginClientThread(PluginClient *client);
	~PluginClientThread();
	void run();

	friend class PluginClient;

	BC_WindowBase* get_window();
	PluginClient* get_client();
	PluginClientWindow *window;
	PluginClient *client;

private:
	Condition *init_complete;
};



// Client overrides for GUI update data
class PluginClientFrame
{
public:
// Period_d is 1 second
	PluginClientFrame(int data_size, int period_n, int period_d);
	virtual ~PluginClientFrame();
	int data_size;
	int period_n;
	int period_d;
// Draw immediately
	int force;
};



class PluginClient
{
public:
	PluginClient(PluginServer *server);
	virtual ~PluginClient();

	friend class PluginClientThread;
	friend class PluginClientWindow;

// Queries for the plugin server.
	virtual int is_realtime();
	virtual int is_audio();
	virtual int is_video();
	virtual int is_fileio();
	virtual int is_theme();
	virtual int uses_gui();
	virtual int is_multichannel();
	virtual int is_synthesis();
	virtual int is_transition();
	virtual const char* plugin_title();   // return the title of the plugin
	virtual Theme* new_theme();
// Get theme being used by Cinelerra currently.  Used by all plugins.
	Theme* get_theme();






// Non realtime signal processors define these.
// Give the samplerate of the output for a non realtime plugin.
// For realtime plugins give the requested samplerate.
	virtual int get_samplerate();
// Give the framerate of the output for a non realtime plugin.
// For realtime plugins give the requested framerate.
	virtual double get_framerate();
	virtual int delete_nonrealtime_parameters();
	virtual int get_parameters();     // get information from user before non realtime processing
	virtual int64_t get_in_buffers(int64_t recommended_size);  // return desired size for input buffers
	virtual int64_t get_out_buffers(int64_t recommended_size);     // return desired size for output buffers
	virtual int start_loop();
	virtual int process_loop();
	virtual int stop_loop();
// Hash files are the defaults for rendered plugins
	virtual int load_defaults();       // load default settings for the plugin
	virtual int save_defaults();      // save the current settings as defaults
	BC_Hash* get_defaults();




// Realtime commands for signal processors.
// These must be defined by the plugin itself.
// Set the GUI title identifying the plugin to modules and patches.
	int set_string();
// cause the plugin to create a new GUI class
	virtual BC_WindowBase* new_window();
// Load the current keyframe.  Return 1 if it changed.
	virtual int load_configuration();
// cause the plugin to hide the gui
	void client_side_close();
	void update_display_title();
// Raise the GUI
	void raise_window();
// Create GUI
	int show_gui();
// XML keyframes are the defaults for realtime plugins
	void load_defaults_xml();
	void save_defaults_xml();
// Tell the client if the load is the defaults
	int is_defaults();

	virtual void update_gui();
	virtual void save_data(KeyFrame *keyframe) {};    // write the plugin settings to text in text format
	virtual void read_data(KeyFrame *keyframe) {};    // read the plugin settings from the text
	int send_hide_gui();                                    // should be sent when the GUI receives a close event from the user
// Destroys the window but not the thread pointer.
	void hide_gui();

	int get_configure_change();                             // get propogated configuration change from a send_configure_change

// Called by plugin server to update GUI with rendered data.
	void plugin_render_gui(void *data);
	void plugin_render_gui(void *data, int size);

	void begin_process_buffer();
	void end_process_buffer();

	void plugin_update_gui();
	virtual int plugin_process_loop(VFrame **buffers, int64_t &write_length) { return 1; };
	virtual int plugin_process_loop(Samples **buffers, int64_t &write_length) { return 1; };
// get parameters depending on video or audio
	virtual int init_realtime_parameters();
// release objects which are required after playback stops
	virtual void render_stop() {};
	int get_gui_status();
	char* get_gui_string();

// Used by themes
// Used by plugins which need to know where they are.
	char* get_path();
// Get the directory for plugins
	char* get_plugin_dir();

// Return keyframe objects.  The position in the resulting object
// is relative to the EDL rate.  This is the only way to avoid copying the
// data for every frame.
// If the result is the default keyframe, the keyframe's position is 0.
// position - relative to EDL rate or local rate to allow simple
//     passing of get_source_position.
//     If -1 the tracking position in the edl is used.
// is_local - if 1, the position is converted to the EDL rate.
	KeyFrame* get_prev_keyframe(int64_t position, int is_local = 1);
	KeyFrame* get_next_keyframe(int64_t position, int is_local = 1);
// get current camera and projector position
	void get_camera(float *x, float *y, float *z, int64_t position);
	void get_projector(float *x, float *y, float *z, int64_t position);
// When this plugin is adjusted, propogate parameters back to EDL and virtual
// console.  This gets a keyframe from the EDL, with the position set to the
// EDL tracking position.
	int send_configure_change();


// Called from process_buffer
// Returns 1 if a GUI is open so OpenGL routines can determine if
// they can run.
	int gui_open();



// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.  Relative to the requested rate.
// The only way to get smooth interpolation is to make all position queries
// relative to the requested rate.
	int64_t get_total_len();

// For realtime plugins gets the lowest sample of the plugin in the requested
// rate.  For others it's the start of the EDL selection in the EDL rate.
	int64_t get_source_start();

// Convert the position relative to the requested rate to the position
// relative to the EDL rate.  If the argument is < 0, it is not changed.
// Used for interpreting keyframes.
	virtual int64_t local_to_edl(int64_t position);

// Convert the EDL position to the local position.
	virtual int64_t edl_to_local(int64_t position);

// For transitions the source_position is the playback position relative
// to the start of the transition.
// For realtime effects, the start of the most recent process_buffer in forward
// and the end of the range to process in reverse.  Relative to start of EDL in
// the requested rate.
	int64_t get_source_position();

// Get the EDL Session.  May return 0 if the server has no edl.
	EDLSession* get_edlsession();


// Get the direction of the most recent process_buffer
	int get_direction();

// Plugin must call this before performing OpenGL operations.
// Returns 1 if the user supports opengl buffers.
	int get_use_opengl();

// Get total tracks to process
	int get_total_buffers();

// Get size of buffer to fill in non-realtime plugin
	int get_buffer_size();

// Get interpolation used by EDL from overlayframe.inc
	int get_interpolation_type();

// Get the values from the color picker
	float get_red();
	float get_green();
	float get_blue();



// Operations for file handlers
	virtual int open_file() { return 0; };
	virtual int get_audio_parameters() { return 0; };
	virtual int get_video_parameters() { return 0; };
	virtual int check_header(char *path) { return 0; };
	virtual int open_file(char *path, int wr, int rd) { return 1; };
	virtual int close_file() { return 0; };





// All plugins define these.
	PluginClientThread* get_thread();



// Non realtime operations for signal processors.
	virtual int plugin_start_loop(int64_t start,
		int64_t end,
		int64_t buffer_size,
		int total_buffers);
	int plugin_stop_loop();
	int plugin_process_loop();
	MainProgressBar* start_progress(char *string, int64_t length);
// get samplerate of EDL
	int get_project_samplerate();
// get framerate of EDL
	double get_project_framerate();
// get asset path
	const char *get_source_path();
// Total number of processors - 1
	int get_project_smp();
	int get_aspect_ratio(float &aspect_w, float &aspect_h);


	int write_frames(int64_t total_frames);  // returns 1 for failure / tells the server that all output channel buffers are ready to go
	int write_samples(int64_t total_samples);  // returns 1 for failure / tells the server that all output channel buffers are ready to go
	virtual int plugin_get_parameters();
	const char* get_defaultdir();     // Directory defaults should be stored in
	void set_interactive();

// Realtime operations.
	int reset();
// Extension of plugin_run for derived plugins
	virtual int plugin_command_derived(int plugin_command) { return 0; };
	int plugin_get_range();
	int plugin_init_realtime(int realtime_priority,
		int total_in_buffers,
		int buffer_size);


// GUI updating wrappers for realtime plugins
// Append frame to queue for next send_frame_buffer
	void add_gui_frame(PluginClientFrame *frame);



	virtual void render_gui(void *data);
	virtual void render_gui(void *data, int size);

// Called by client to get the total number of frames to draw in update_gui
	int get_gui_update_frames();
// Get GUI frame from frame_buffer.  Client must delete it.
	PluginClientFrame* get_gui_frame();

// Called by client to cause GUI to be rendered with data.
	void send_render_gui();
	void send_render_gui(void *data);
	void send_render_gui(void *data, int size);








// create pointers to buffers of the plugin's type before realtime rendering
	virtual int delete_buffer_ptrs();




// communication convenience routines for the base class
	int stop_gui_client();
	int save_data_client();
	int load_data_client();
	int set_string_client(char *string);                // set the string identifying the plugin
	int send_cancelled();        // non realtime plugin sends when cancelled

// ================================= Buffers ===============================

// number of double buffers for each channel
	ArrayList<int> double_buffers_in;
	ArrayList<int> double_buffers_out;
// When arming buffers need to know the offsets in all the buffers and which
// double buffers for each channel before rendering.
	ArrayList<int64_t> offset_in_render;
	ArrayList<int64_t> offset_out_render;
	ArrayList<int64_t> double_buffer_in_render;
	ArrayList<int64_t> double_buffer_out_render;
// total size of each buffer depends on if it's a master or node
	ArrayList<int64_t> realtime_in_size;
	ArrayList<int64_t> realtime_out_size;

// ================================= Automation ===========================

	ArrayList<PluginClientAuto> automation;

// ================================== Messages ===========================
	char gui_string[BCTEXTLEN];          // string identifying module and plugin
	int master_gui_on;              // Status of the master gui plugin
	int client_gui_on;              // Status of this client's gui

	int show_initially;             // set to show a realtime plugin initially
// range in project for processing
	int64_t start, end;
	int interactive;                // for the progress bar plugin
	int success;
	int total_out_buffers;          // total send buffers allocated by the server
	int total_in_buffers;           // total receive buffers allocated by the server
	int wr, rd;                     // File permissions for fileio plugins.

// These give the largest fragment the plugin is expected to handle.
// size of a send buffer to the server
	int64_t out_buffer_size;
// size of a receive buffer from the server
	int64_t in_buffer_size;




// Direction of most recent process_buffer
	int direction;

// Operating system scheduling
	int realtime_priority;

// Position relative to start of EDL in requested rate.  Calculated for every process
// command.  Used for keyframes.
	int64_t source_position;
// For realtime plugins gets the lowest sample of the plugin in the requested
// rate.  For others it's always 0.
	int64_t source_start;
// Length of source.  For effects it's the plugin length.  For transitions
// it's the transition length.  Relative to the requested rate.
	int64_t total_len;
// Total number of processors available - 1
	int smp;
	PluginServer *server;
	BC_Hash *defaults;
	PluginClientThread *thread;

// Frames for updating GUI
	ArrayList<PluginClientFrame*> frame_buffer;
// Time of last GUI update
	Timer *update_timer;


private:
	int using_defaults;
// Temporaries set in new_window
	int window_x, window_y;
// File handlers:
//	Asset *asset;     // Point to asset structure in shared memory
};


#endif
