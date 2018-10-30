/*
 * CINELERRA
 * Copyright (C) 2011 Adam Williams <broadcast at earthling dot net>
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



#ifndef SCENEGRAPH_H
#define SCENEGRAPH_H


#include "affine.inc"
#include "arraylist.h"
#include "overlayframe.h"
#include "vframe.h"


// Main scene graph objects
class SceneGraph;




// Base class for all scene objects
class SceneNode
{
public:
	SceneNode();
	SceneNode(const char *title);
	SceneNode(VFrame *image, int private_image, float x, float y);
	virtual ~SceneNode();

	void append(SceneNode *node);
	SceneNode* get_node(int number);
	void reset();
// Copy values & image pointer but not subnodes yet
	void copy_ref(SceneNode *node);
// Not recursive yet
	int get_memory_usage();
	virtual void dump(int indent);
	void render(VFrame *frame, int do_camera);
// 2D coordinate transformation
	void transform_coord(
		float *x,
		float *y,
		float *sx,
		float *sy,
		float *ry);
	int get_flip();

	ArrayList<SceneNode*> nodes;

// for 2D
	VFrame *image;
	int private_image;

// Move.  Only x & y are used in 2D.
// x,y is the top left corner in 2D
	float x, y, z;
// Scale.  Only sx & sy are used in 2D
	float sx, sy, sz;
// Rotate in degrees.  Only ry is used in 2D
	float rx, ry, rz;
// causes x,y to be the top right corner in 2D
	int flip;
	int hidden;
	char title[BCTEXTLEN];
// Set in append()
	SceneNode *parent;
	SceneGraph *scene;
};


class SceneGraph : public VFrameScene
{
public:
	SceneGraph();
	virtual ~SceneGraph();

	SceneNode* get_node(int number);
	void append(SceneNode *node);
	void append_camera(SceneNode *node);
// Render 2D scene
	void render(VFrame *frame, int cpus);
	void dump();
	void transform_camera(VFrame *frame,
		float *x,
		float *y,
		float *sx,
		float *sy,
		int flip);


	ArrayList<SceneNode*> nodes;
	ArrayList<SceneNode*> cameras;
	int current_camera;
	AffineEngine *affine;
	OverlayFrame *overlayer;
	int cpus;
};



class SceneTransform : public SceneNode
{
public:
	SceneTransform();
	virtual ~SceneTransform();

};



class SceneLight : public SceneNode
{
public:
	SceneLight();
	virtual ~SceneLight();


	double r, g, b;
};


class SceneCamera : public SceneNode
{
public:
	SceneCamera();
	virtual ~SceneCamera();

	virtual void dump(int indent);

// Top left of box to look at in 2D
	double at_x, at_y, at_z;
	double scale;
};

class SceneMaterial : public SceneNode
{
public:
	SceneMaterial();
	virtual ~SceneMaterial();

	double r, g, b, a;
	char *texture;
	double s, t;
};



// Base class for shapes
class SceneShape : public SceneNode
{
public:
	SceneShape();
	~SceneShape();

// Pivot for shapes
	double pivot_x, pivot_y, pivot_z;
};



class SceneCylinder : public SceneShape
{
public:
	SceneCylinder();
	virtual ~SceneCylinder();
};

class SceneSphere : public SceneShape
{
public:
	SceneSphere();
	virtual ~SceneSphere();
};

class SceneBox : public SceneShape
{
public:
	SceneBox();
	virtual ~SceneBox();
};





#endif



