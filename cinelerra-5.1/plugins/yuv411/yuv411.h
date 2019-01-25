#ifndef YUV411_H
#define YUV411_H


class yuv411Main;
class yuv411Window;

#include "filexml.h"
#include "yuv411win.h"
#include "guicast.h"
#include "pluginvclient.h"

class yuv411Config
{
public:
	yuv411Config();
	void reset();

	void copy_from(yuv411Config &that);
	int equivalent(yuv411Config &that);
	void interpolate(yuv411Config &prev, yuv411Config &next,
		long prev_frame, long next_frame, long current_frame);
	int int_horizontal;
	int avg_vertical;
	int inpainting;
	int offset;
	int thresh;
	int bias;
};

class yuv411Main : public PluginVClient
{
	VFrame *temp_frame;
	int colormodel;
	friend class yuv411Window;
public:
	yuv411Main(PluginServer *server);
	~yuv411Main();

// required for all realtime plugins
	PLUGIN_CLASS_MEMBERS(yuv411Config)

	int process_realtime(VFrame *input_ptr, VFrame *output_ptr);
	int is_realtime();
	void update_gui();
	void render_gui(void *data);
	void save_data(KeyFrame *keyframe);
	void read_data(KeyFrame *keyframe);
};


#endif
