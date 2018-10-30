
/*
 * CINELERRA
 * Copyright (C) 1997-2012 Adam Williams <broadcast at earthling dot net>
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


#ifndef __FINDOBJWINDOW_H__
#define __FINDOBJWINDOW_H__

#include "dragcheckbox.h"
#include "findobj.inc"
#include "guicast.h"


class FindObjReset;
class FindObjLayer;
class FindObjScanFloat;
class FindObjScanFloatText;
class FindObjDrawSceneBorder;
class FindObjDrawObjectBorder;
class FindObjDrawReplaceBorder;
class FindObjDrawKeypoints;
class FindObjReplace;
class FindObjDragScene;
class FindObjDragObject;
class FindObjDragReplace;
class FindObjAlgorithm;
class FindObjUseFlann;
class FindObjDrawMatch;
class FindObjAspect;
class FindObjScale;
class FindObjRotate;
class FindObjTranslate;
class FindObjMode;
class FindObjBlend;
class FindObjWindow;

class FindObjReset : public BC_GenericButton
{
public:
	FindObjReset(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjLayer : public BC_TumbleTextBox
{
public:
	FindObjLayer(FindObjMain *plugin, FindObjWindow *gui,
		int x, int y, int *value);
	int handle_event();
	static int calculate_w(FindObjWindow *gui);
	FindObjMain *plugin;
	FindObjWindow *gui;
	int *value;
};

class FindObjScanFloat : public BC_FPot
{
public:
	FindObjScanFloat(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float *value, float min=0., float max=100.);
	int handle_event();
	void update(float v);
	FindObjMain *plugin;
	FindObjWindow *gui;
	FindObjScanFloatText *center_text;
	float *value;
};

class FindObjScanFloatText : public BC_TextBox
{
public:
	FindObjScanFloatText(FindObjMain *plugin, FindObjWindow *gui, int x, int y, float *value);
	int handle_event();
	FindObjWindow *gui;
	FindObjMain *plugin;
	FindObjScanFloat *center;
	float *value;
};


class FindObjDrawSceneBorder : public BC_CheckBox
{
public:
	FindObjDrawSceneBorder(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDrawObjectBorder : public BC_CheckBox
{
public:
	FindObjDrawObjectBorder(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDrawReplaceBorder : public BC_CheckBox
{
public:
	FindObjDrawReplaceBorder(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDrawKeypoints : public BC_CheckBox
{
public:
	FindObjDrawKeypoints(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjReplace : public BC_CheckBox
{
public:
	FindObjReplace(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDragScene : public DragCheckBox
{
public:
	FindObjDragScene(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h);
	~FindObjDragScene();
	int handle_event();
	Track *get_drag_track();
	int64_t get_drag_position();
	void update_gui();

	FindObjMain *plugin;
	FindObjWindow *gui;

};

class FindObjDragObject : public DragCheckBox
{
public:
	FindObjDragObject(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h);
	~FindObjDragObject();

	int handle_event();
	Track *get_drag_track();
	int64_t get_drag_position();
	void update_gui();

	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDragReplace : public DragCheckBox
{
public:
	FindObjDragReplace(FindObjMain *plugin, FindObjWindow *gui, int x, int y,
		float drag_x, float drag_y, float drag_w, float drag_h);
	~FindObjDragReplace();

	int handle_event();
	Track *get_drag_track();
	int64_t get_drag_position();
	void update_gui();

	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjAlgorithm : public BC_PopupMenu
{
public:
	FindObjAlgorithm(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	void update(int mode);
	static int calculate_w(FindObjWindow *gui);
	static int from_text(char *text);
	static char* to_text(int mode);
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjUseFlann : public BC_CheckBox
{
public:
	FindObjUseFlann(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjDrawMatch : public BC_CheckBox
{
public:
	FindObjDrawMatch(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjAspect : public BC_CheckBox
{
public:
	FindObjAspect(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjScale : public BC_CheckBox
{
public:
	FindObjScale(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjRotate : public BC_CheckBox
{
public:
	FindObjRotate(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjTranslate : public BC_CheckBox
{
public:
	FindObjTranslate(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjMode : public BC_PopupMenu
{
public:
	FindObjMode(FindObjMain *plugin, FindObjWindow *gui, int x, int y);
	int handle_event();
	void create_objects();
	void update(int mode);
	static int calculate_w(FindObjWindow *gui);
	static int from_text(char *text);
	static char *to_text(int mode);
	FindObjMain *plugin;
	FindObjWindow *gui;
};

class FindObjBlend : public BC_IPot
{
public:
	FindObjBlend(FindObjMain *plugin, int x, int y, int *value);
	int handle_event();
	FindObjMain *plugin;
	int *value;
};

class FindObjWindow : public PluginClientWindow
{
public:
	FindObjWindow(FindObjMain *plugin);
	~FindObjWindow();
	void create_objects();
	void update_drag();
	void update_gui();

	FindObjReset *reset;
	FindObjAlgorithm *algorithm;
	FindObjUseFlann *use_flann;
	FindObjDrawMatch *draw_match;
	FindObjMode *mode;
	FindObjAspect *aspect;
	FindObjScale *scale;
	FindObjRotate *rotate;
	FindObjTranslate *translate;
	FindObjScanFloat *object_x, *object_y, *object_w, *object_h;
	FindObjScanFloatText *object_x_text, *object_y_text, *object_w_text, *object_h_text;
	FindObjScanFloat *scene_x, *scene_y, *scene_w, *scene_h;
	FindObjScanFloatText *scene_x_text, *scene_y_text, *scene_w_text, *scene_h_text;
	FindObjScanFloat *replace_x, *replace_y, *replace_w, *replace_h;
	FindObjScanFloatText *replace_x_text, *replace_y_text, *replace_w_text, *replace_h_text;
	FindObjScanFloat *replace_dx, *replace_dy;
	FindObjScanFloatText *replace_dx_text, *replace_dy_text;
	FindObjDrawKeypoints *draw_keypoints;
	FindObjDrawSceneBorder *draw_scene_border;
	FindObjReplace *replace_object;
	FindObjDrawObjectBorder *draw_object_border;
	FindObjDrawReplaceBorder *draw_replace_border;
	FindObjDragObject *drag_object;
	FindObjDragScene *drag_scene;
	FindObjDragReplace *drag_replace;
	FindObjLayer *object_layer;
	FindObjLayer *scene_layer;
	FindObjLayer *replace_layer;
	FindObjBlend *blend;
	FindObjMain *plugin;
};

#endif
