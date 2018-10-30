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


#include "affine.h"
#include "clip.h"
#include <math.h>
#include "overlayframe.h"
#include "scenegraph.h"
#include <string.h>


#define PRINT_INDENT for(int i = 0; i < indent; i++) printf(" ");


SceneNode::SceneNode()
{
	reset();
}

SceneNode::SceneNode(const char *title)
{
	reset();
	strcpy(this->title, title);
}

SceneNode::SceneNode(VFrame *image, int private_image, float x, float y)
{
	reset();
	this->image = image;
	if(this->image) this->private_image = private_image;
	this->x = x;
	this->y = y;
}

SceneNode::~SceneNode()
{
	nodes.remove_all_objects();
	if(private_image) delete image;
}

void SceneNode::reset()
{
	parent = 0;
	scene = 0;
	x = y = z = rx = ry = rz = 0;
	sx = sy = sz = 1;
	hidden = 0;
	image = 0;
	private_image = 0;
	flip = 0;
	title[0] = 0;
}

void SceneNode::append(SceneNode *node)
{
	node->parent = this;
	node->scene = this->scene;
	nodes.append(node);
}

void SceneNode::copy_ref(SceneNode *node)
{
	if(private_image) delete image;
	private_image = 0;
	x = node->x;
	y = node->y;
	z = node->z;
	rx = node->rx;
	ry = node->ry;
	rz = node->rz;
	sx = node->sx;
	sy = node->sy;
	sz = node->sz;
	hidden = node->hidden;
	image = node->image;
	flip = node->flip;
}

SceneNode* SceneNode::get_node(int number)
{
	return nodes.get(number);
}

int SceneNode::get_memory_usage()
{
	if(image)
		return image->get_memory_usage();
	else
		return 0;
}

void SceneNode::render(VFrame *frame, int do_camera)
{
	const int debug = 0;
	if(debug) printf("SceneNode::render %d this=%p title=%s image=%p x=%f y=%f frame=%p do_camera=%d\n",
		__LINE__,
		this,
		title,
		image,
		this->x,
		this->y,
		frame,
		do_camera);

	VFrame *temp = 0;
	VFrame *temp2 = 0;
	if(image)
	{
		float x = this->x;
		float y = this->y;
		float sx = this->sx;
		float sy = this->sy;
		float ry = this->ry;
		if(parent)
			parent->transform_coord(&x, &y, &sx, &sy, &ry);

		if(do_camera) scene->transform_camera(frame,
			&x,
			&y,
			&sx,
			&sy,
			get_flip());

		if(debug) printf("SceneNode::render %d at_y=%f\n",
			__LINE__,
			((SceneCamera*)scene->cameras.get(0))->at_y);

// Render everything into a temporary, then overlay the temporary
// 		if(!EQUIV(ry, 0) || get_flip())
// 		{
		temp = new VFrame(image->get_w(), image->get_h(), image->get_color_model(), 0);
//		}
		if(debug) printf("SceneNode::render %d\n", __LINE__);


// 1st comes our image
		temp->copy_from(image);

// Then come the subnodes without camera transforms
		if(debug) printf("SceneNode::render %d this=%p nodes=%d\n", __LINE__, this, nodes.size());
		for(int i = 0; i < nodes.size(); i++)
			nodes.get(i)->render(temp, 0);



		if(debug) printf("SceneNode::render %d\n", __LINE__);

// Then comes rotation into temp2
		VFrame *src = temp;
		if(!EQUIV(ry, 0))
		{
			src = temp2 = new VFrame(image->get_w(), image->get_h(), image->get_color_model(), 0);
			if(!scene->affine) scene->affine = new AffineEngine(scene->cpus, scene->cpus);
			scene->affine->rotate(temp2, temp, ry);
			if(debug) printf("SceneNode::render %d ry=%f\n", __LINE__, ry);
		}

// Then comes flipping
		if(get_flip())
			src->flip_horiz();

		if(debug) printf("SceneNode::render %d src=%p x=%f y=%f sx=%f sy=%f\n",
			__LINE__, src, x, y, sx, sy); 
// Overlay on the output frame
		if(!scene->overlayer) scene->overlayer = new OverlayFrame(scene->cpus);

		if(get_flip())
		{
			scene->overlayer->overlay(frame,
				src,
				0,
				0,
				image->get_w(),
				image->get_h(),
				frame->get_w() - x - image->get_w() * sx,
				y,
				frame->get_w() - x,
				y + image->get_h() * sy,
				1,
				TRANSFER_NORMAL,
				NEAREST_NEIGHBOR);
		}
		else
		{
			if(debug) printf("SceneNode::render %d image=%p src=%p frame=%p\n",
				__LINE__,
				image,
				src,
				frame);
			scene->overlayer->overlay(frame,
				src,
				0,
				0,
				image->get_w(),
				image->get_h(),
				x,
				y,
				x + image->get_w() * sx,
				y + image->get_h() * sy,
				1,
				TRANSFER_NORMAL,
				NEAREST_NEIGHBOR);
		}

		if(debug) printf("SceneNode::render %d\n", __LINE__);

	}
	else
	{
		for(int i = 0; i < nodes.size(); i++)
			nodes.get(i)->render(frame, 1);
	}

	if(debug) printf("SceneNode::render %d this=%p title=%s\n", __LINE__, this, title);

	if(temp) delete temp;
	if(temp2) delete temp2;
}

int SceneNode::get_flip()
{
	if(flip) return 1;
	if(parent && !parent->image) return parent->get_flip();
	return 0;
}


void SceneNode::transform_coord(
	float *x,
	float *y,
	float *sx,
	float *sy,
	float *ry)
{
// Rotate it
// 	if(!EQUIV(this->ry, 0))
// 	{
// 		float pivot_x = 0;
// 		float pivot_y = 0;
// 		if(image)
// 		{
// 			pivot_x = image->get_w() / 2;
// 			pivot_y = image->get_h() / 2;
// 		}
//
// 		float rel_x = *x - pivot_x;
// 		float rel_y = *y - pivot_y;
// 		float angle = atan2(rel_y, rel_x);
// 		float mag = sqrt(rel_x * rel_x + rel_y * rel_y);
// 		angle += this->ry * 2 * M_PI / 360;
// 		*x = mag * cos(angle) + pivot_x;
// 		*y = mag * sin(angle) + pivot_y;
// 	}

// Nodes with images reset the transformation
	if(!image)
	{
// Scale it
		*x *= this->sx;
		*y *= this->sy;
		*sx *= this->sx;
		*sy *= this->sy;

// Translate it
		*x += this->x;
		*y += this->y;


// Accumulate rotation
		*ry += this->ry;

		if(parent)
		{
			parent->transform_coord(x, y, sx, sy, ry);
		}
	}
}

void SceneNode::dump(int indent)
{
	PRINT_INDENT
	printf("SceneNode::dump %d this=%p title=%s\n", __LINE__, this, title);
	PRINT_INDENT
	printf("  image=%p private_image=%d hidden=%d flip=%d\n",
		image,
		private_image,
		hidden,
		flip);
	PRINT_INDENT
	printf("  x=%f y=%f z=%f sz=%f sy=%f sz=%f rx=%f ry=%f rz=%f\n",
		x, y, z, sz, sy, sz, rx, ry, rz);
	PRINT_INDENT
	printf("  nodes=%d\n", nodes.size());
	for(int i = 0; i < nodes.size(); i++)
	{
		nodes.get(i)->dump(indent + 4);
	}
}





SceneGraph::SceneGraph() : VFrameScene()
{
	current_camera = 0;
	affine = 0;
	overlayer = 0;
}

SceneGraph::~SceneGraph()
{
	nodes.remove_all_objects();
	cameras.remove_all_objects();
	delete affine;
	delete overlayer;
}


SceneNode* SceneGraph::get_node(int number)
{
	return nodes.get(number);
}

void SceneGraph::append(SceneNode *node)
{
	node->parent = 0;
	node->scene = this;
	nodes.append(node);
}

void SceneGraph::append_camera(SceneNode *node)
{
	node->scene = this;
	cameras.append(node);
}

void SceneGraph::render(VFrame *frame, int cpus)
{
	const int debug = 0;
	if(debug) printf("SceneGraph::render %d\n", __LINE__);
	if(debug) dump();

	this->cpus = cpus;

	for(int i = 0; i < nodes.size(); i++)
	{
		nodes.get(i)->render(frame, 1);
	}
}

void SceneGraph::transform_camera(VFrame *frame,
	float *x,
	float *y,
	float *sx,
	float *sy,
	int flip)
{
	if(cameras.size())
	{
		SceneCamera *camera = (SceneCamera*)cameras.get(current_camera);

		if(flip)
		{
			*x = frame->get_w() - *x;
		}

		*x -= camera->at_x;
		*y -= camera->at_y;
		*x *= camera->scale;
		*y *= camera->scale;
		*sx *= camera->scale;
		*sy *= camera->scale;

		if(flip)
		{
			*x = frame->get_w() - *x;
		}
	}
}

void SceneGraph::dump()
{
	int indent = 2;
	printf("SceneGraph::dump %d cameras=%d\n", __LINE__, cameras.size());
	for(int i = 0; i < cameras.size(); i++)
		cameras.get(i)->dump(indent);
	printf("SceneGraph::dump %d nodes=%d\n", __LINE__, nodes.size());
	for(int i = 0; i < nodes.size(); i++)
		nodes.get(i)->dump(indent);
}







SceneTransform::SceneTransform() : SceneNode()
{
}

SceneTransform::~SceneTransform()
{
}





SceneLight::SceneLight() : SceneNode()
{
}

SceneLight::~SceneLight()
{
}







SceneCamera::SceneCamera() : SceneNode()
{
	at_x = at_y = at_z = 0;
	scale = 1;
}

SceneCamera::~SceneCamera()
{
}

void SceneCamera::dump(int indent)
{
	PRINT_INDENT
	printf("SceneCamera::dump %d this=%p\n", __LINE__, this);
	PRINT_INDENT
	printf("  at_x=%f at_y=%f at_z=%f scale=%f\n",
		at_x,
		at_y,
		at_z,
		scale);
}




SceneMaterial::SceneMaterial() : SceneNode()
{
	r = g = b = a = 0;
	texture = 0;
	s = t = 0;
}

SceneMaterial::~SceneMaterial()
{
	delete [] texture;
}







SceneShape::SceneShape() : SceneNode()
{
	pivot_x = pivot_y = pivot_z = 0;
}

SceneShape::~SceneShape()
{
}








SceneCylinder::SceneCylinder() : SceneShape()
{
}

SceneCylinder::~SceneCylinder()
{
}




SceneSphere::SceneSphere() : SceneShape()
{
}


SceneSphere::~SceneSphere()
{
}




SceneBox::SceneBox() : SceneShape()
{
}


SceneBox::~SceneBox()
{
}













