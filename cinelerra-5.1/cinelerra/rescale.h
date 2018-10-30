#ifndef __RESCALE_H__
#define __RESCALE_H__
#include "indexable.inc"

class Rescale {
public:
  	enum { none, scaled, cropped, filled, horiz_edge, vert_edge, n_scale_types };
	static const char *scale_types[];

	Rescale(int w, int h, double aspect);
	Rescale(Indexable *in);
	~Rescale();

	int w, h;
	double aspect;

	void rescale(Rescale &out, int type,
		float &src_w,float &src_h, float &dst_w,float &dst_h);
};

#endif
