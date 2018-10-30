#ifndef __INTERP__
#define __INTERP__

/* fractional input interpolation
 interp = nearest, bi_linear, bi_cubic
 typical use:

 type **in_rows = (type**)in->get_rows();
 type **out_rows = (type**)out->get_rows();
 int ow = out->get_w(), oh = out->get_h();
 type offset[4] = { 0, chroma, chroma, 0 };
 type bkgrnd[4] = { 0, chroma, chroma, 0 };
 INTERP_SETUP(in_rows, max, 0,0, width,height); 

 for( int oy=0; oy<oh; ++oy ) {
   type *out_row = out_rows[y];
   for( ox=0; ox<ow; ++ox ) {
     float x_in = remap_x(ox,oy), y_in = remap_y(ox,oy);
     interp##_SETUP(type, components, x_in, y_in); \
     *out_row++ = interp##_interp(offset[i], bkgrnd[i]); \
     for( int i=1; i<components; ++i ) {
       interp##_next();
       *out_row++ = interp##_interp(offset[i], bkgrnd[i]); \
     }
   }
 }
*/

static inline float in_clip(float v, float mx)
{
	return v < 0 ? 0 : v > mx ? mx : v;
}

static inline float interp_linear(float dx, float p1, float p2)
{
	return p1 * (1-dx) + p2 * dx;
}

static inline float interp_cubic(float dx, float p0, float p1, float p2, float p3)
{ /* Catmull-Rom - not bad */
	return ((( (- p0 + 3*p1 - 3*p2 + p3) * dx +
		( 2*p0 - 5*p1 + 4*p2 - p3 ) ) * dx +
		( - p0 + p2 ) ) * dx + (p1 + p1)) / 2;
}


#define INTERP_SETUP(rows, max, x_in_min,y_in_min, x_in_max,y_in_max) \
	void **interp_rows = (void **)(rows);  float in_max = (max);  \
	int in_min_x = (x_in_min), in_max_x = (x_in_max); \
	int in_min_y = (y_in_min), in_max_y = (y_in_max)


#define nearest_SETUP(typ, components, tx, ty) \
	int itx = (tx), ity = (ty), in_comps = (components); \
	int c0 = itx+0, r0 = ity+0; \
	typ *r0p = r0>=in_min_y && r0<in_max_y ? ((typ**)interp_rows)[r0] : 0

#define NEAREST_ROW(in_row, ofs, bg) (!in_row ? bg - ofs : ( \
	(c0>=in_min_x && c0<in_max_x ? in_row[c0*in_comps] : bg) - ofs))

#define nearest_interp(ofs, bg) in_clip(NEAREST_ROW(r0p, ofs, bg) + ofs, in_max)

#define nearest_next(s) { if(r0p) ++r0p; }


#define bi_linear_SETUP(typ, components, tx, ty) \
	float dx = (tx)-0.5, dy = (ty)-0.5; \
	int itx = dx, ity = dy, in_comps = (components); \
	if( (dx -= itx) < 0 ) dx += 1; \
	if( (dy -= ity) < 0 ) dy += 1; \
	int c0 = itx+0, c1 = itx+1, r0 = ity+0, r1 = ity+1; \
	typ *r0p = r0>=in_min_y && r0<in_max_y ? ((typ**)interp_rows)[r0] : 0; \
	typ *r1p = r1>=in_min_y && r1<in_max_y ? ((typ**)interp_rows)[r1] : 0

#define LINEAR_ROW(in_row, ofs, bg) (!in_row ? bg - ofs : interp_linear(dx, \
	(c0>=in_min_x && c0<in_max_x ? in_row[c0*in_comps] : bg) - ofs, \
	(c1>=in_min_x && c1<in_max_x ? in_row[c1*in_comps] : bg) - ofs))

#define bi_linear_interp(ofs, bg) in_clip(interp_linear(dy, \
	LINEAR_ROW(r0p, ofs, bg), LINEAR_ROW(r1p, ofs, bg)) + ofs, in_max)

#define bi_linear_next(s) { if(r0p) ++r0p;  if(r1p) ++r1p; }


#define bi_cubic_SETUP(typ, components, tx, ty) \
	float dx = (tx)-0.5, dy = (ty)-0.5; \
	int itx = dx, ity = dy, in_comps = (components); \
	if( (dx -= itx) < 0 ) dx += 1; \
	if( (dy -= ity) < 0 ) dy += 1; \
	int cp = itx-1, c0 = itx+0, c1 = itx+1, c2 = itx+2; \
	int rp = ity-1, r0 = ity+0, r1 = ity+1, r2 = ity+2; \
	typ *rpp = rp>=in_min_y && rp<in_max_y ? ((typ**)interp_rows)[rp] : 0; \
	typ *r0p = r0>=in_min_y && r0<in_max_y ? ((typ**)interp_rows)[r0] : 0; \
	typ *r1p = r1>=in_min_y && r1<in_max_y ? ((typ**)interp_rows)[r1] : 0; \
	typ *r2p = r2>=in_min_y && r2<in_max_y ? ((typ**)interp_rows)[r2] : 0

#define CUBIC_ROW(in_row, ofs, bg) (!in_row ? bg - ofs : interp_cubic(dx, \
	(cp>=in_min_x && cp<in_max_x ? in_row[cp*in_comps] : bg) - ofs, \
	(c0>=in_min_x && c0<in_max_x ? in_row[c0*in_comps] : bg) - ofs, \
	(c1>=in_min_x && c1<in_max_x ? in_row[c1*in_comps] : bg) - ofs, \
	(c2>=in_min_x && c2<in_max_x ? in_row[c2*in_comps] : bg) - ofs))

#define bi_cubic_interp(ofs, bg) in_clip(interp_cubic(dy, \
	CUBIC_ROW(rpp, ofs, bg), CUBIC_ROW(r0p, ofs, bg), \
	CUBIC_ROW(r1p, ofs, bg), CUBIC_ROW(r2p, ofs, bg)) + ofs, in_max)

#define bi_cubic_next(s) { if(rpp) ++rpp;  if(r0p) ++r0p;  if(r1p) ++r1p;  if(r2p) ++r2p; }

#endif
