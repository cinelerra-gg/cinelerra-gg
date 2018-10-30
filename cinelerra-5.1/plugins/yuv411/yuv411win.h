#ifndef YUV411WIN_H
#define YUV411WIN_H

#include "filexml.inc"
#include "yuv411.h"
#include "mutex.h"
#include "pluginvclient.h"
#include "bctitle.h"
#include "bccolors.h"
#include "fonts.h"

class yuv411Toggle;
class yuv411Offset;
class yuv411Thresh;
class yuv411Bias;

class yuv411Window : public PluginClientWindow
{
public:
	yuv411Window(yuv411Main *client);
	~yuv411Window();

	void create_objects();
	int close_event();
	void update_enables();
	void show_warning(int warn);

	yuv411Main *client;
	yuv411Toggle *avg_vertical;
	yuv411Toggle *int_horizontal;
	yuv411Toggle *inpainting;
	yuv411Offset *offset;
	yuv411Thresh *thresh;
	yuv411Bias *bias;
	BC_Title *yuv_warning;
};

class yuv411Toggle : public BC_CheckBox
{
public:
	yuv411Toggle(yuv411Main *client, int *output, char *string, int x, int y);
	~yuv411Toggle();
	int handle_event();

	yuv411Main *client;
	int *output;
};

class yuv411Offset : public BC_FSlider
{
public:
	yuv411Offset(yuv411Main *client, int x, int y);
	int handle_event();
	yuv411Main *client;
};

class yuv411Thresh : public BC_FSlider
{
public:
	yuv411Thresh(yuv411Main *client, int x, int y);
	int handle_event();
	yuv411Main *client;
};

class yuv411Bias : public BC_FSlider
{
public:
	yuv411Bias(yuv411Main *client, int x, int y);
	int handle_event();
	yuv411Main *client;
};

#endif
