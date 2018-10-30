#include "bccmodels.h"
#include "bcresources.h"
#include "condition.h"
#include "linklist.h"
#include "mutex.h"
#include "thread.h"
#include "clip.h"

static inline float clp(const int n, float v) {
 v *= ((float)(n*(1-1./0x1000000)));
 return v < 0 ? 0 : v >= n ? n-1 : v;
}

static inline float fclp(float v, const int n) {
 v /= ((float)(n*(1-1./0x1000000)));
 return v < 0 ? 0 : v > 1 ? 1 : v;
}

#include <stdint.h>

#define ZTYP(ty) typedef ty z_##ty __attribute__ ((__unused__))
ZTYP(int);
ZTYP(float);


#define xfer_flat_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    oty_t *out = (oty_t *)(output_rows[i + out_y] + out_x * out_pixelsize); \

#define xfer_flat_row_in(ity_t) \
    uint8_t *inp_row = input_rows[row_table[i]]; \
    for( unsigned j=0; j<out_w; ++j ) { \
      ity_t *inp = (ity_t *)(inp_row + column_table[j]); \

#define xfer_end } }

// yuv420p  2x2
#define xfer_yuv420p_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i / 2 * total_out_w / 2 + out_x / 2; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv420p_row_in(ity_t) \
    int in_r = row_table[i]; \
    int in_rofs = in_r * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs = in_r / 2 * total_in_w / 2; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 2; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv420pi  2x2
#define xfer_yuv420pi_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    int ot_k = ((i/4)<<1) + (i&1); \
    out_rofs = ot_k * total_out_w / 2 + out_x / 2; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv420pi_row_in(ity_t) \
    int in_r = row_table[i]; \
    int in_rofs = in_r * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    int in_k = ((in_r/4)<<1) + (in_r&1); \
    in_rofs = in_k * total_in_w / 2; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 2; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv422p  2x1
#define xfer_yuv422p_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i * total_out_w / 2 + out_x / 2; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv422p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs /= 2; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 2; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv444p  1x1
#define xfer_yuv444p_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)((oty_t *)(out_yp + out_rofs)); \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv444p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv411p  4x1
#define xfer_yuv411p_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i * total_out_w / 4 + out_x / 4; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv411p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs /= 4; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 4; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// yuv410p  4x4
#define xfer_yuv410p_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *yop = (oty_t *)(out_yp + out_rofs); \
    out_rofs = i / 4 * total_out_w / 4 + out_x / 4; \
    oty_t *uop = (oty_t *)(out_up + out_rofs); \
    oty_t *vop = (oty_t *)(out_vp + out_rofs); \

#define xfer_yuv410p_row_in(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *yip_row = in_yp + in_rofs; \
    in_rofs = row_table[i] / 4 * total_in_w / 4; \
    uint8_t *uip_row = in_up + in_rofs; \
    uint8_t *vip_row = in_vp + in_rofs; \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *yip = (ity_t *)(yip_row + in_ofs); \
      in_ofs /= 4; \
      ity_t *uip = (ity_t *)(uip_row + in_ofs); \
      ity_t *vip = (ity_t *)(vip_row + in_ofs); \

// rgb planar
#define xfer_rgbp_row_out(oty_t) \
  for( unsigned i=y0; i<y1; ++i ) { \
    int out_rofs = i * total_out_w + out_x; \
    oty_t *rop = (oty_t *)(out_yp + out_rofs); \
    oty_t *gop = (oty_t *)(out_up + out_rofs); \
    oty_t *bop = (oty_t *)(out_vp + out_rofs); \

#define xfer_rgbap_row_out(oty_t) \
  xfer_rgbp_row_out(oty_t) \
    oty_t *aop = (oty_t *)(out_ap + out_rofs); \


#define xfer_row_in_rgbp(ity_t) \
    int in_rofs = row_table[i] * total_in_w; \
    uint8_t *rip_row = in_yp + in_rofs; \
    uint8_t *gip_row = in_up + in_rofs; \
    uint8_t *bip_row = in_vp + in_rofs; \

#define xfer_row_in_rgbap(oty_t) \
  xfer_row_in_rgbp(ity_t) \
    uint8_t *aip_row = in_ap + in_rofs; \


#define xfer_col_in_rgbp(ity_t) \
    for( unsigned j=0; j<out_w; ++j ) { \
      int in_ofs = column_table[j]; \
      ity_t *rip = (ity_t *)(rip_row + in_ofs); \
      ity_t *gip = (ity_t *)(gip_row + in_ofs); \
      ity_t *bip = (ity_t *)(bip_row + in_ofs); \

#define xfer_col_in_rgbap(ity_t) \
  xfer_col_in_rgbp(ity_t) \
    ity_t *aip = (ity_t *)(aip_row + in_ofs); \


#define xfer_rgbp_row_in(ity_t) \
  xfer_row_in_rgbp(ity_t) \
    xfer_col_in_rgbp(ity_t) \

#define xfer_rgbap_row_in(ity_t) \
  xfer_row_in_rgbap(ity_t) \
    xfer_col_in_rgbap(ity_t) \


class BC_Xfer {
  BC_Xfer(const BC_Xfer&) {}
public:
  BC_Xfer(uint8_t **output_rows, uint8_t **input_rows,
    uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp,
    uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp,
    int in_x, int in_y, int in_w, int in_h, int out_x, int out_y, int out_w, int out_h,
    int in_colormodel, int out_colormodel, int bg_color, int in_rowspan, int out_rowspan);
  BC_Xfer(uint8_t **output_ptrs, int out_colormodel,
      int out_x, int out_y, int out_w, int out_h, int out_rowspan,
    uint8_t **input_ptrs, int in_colormodel,
      int in_x, int in_y, int in_w, int in_h, int in_rowspan,
    int bg_color);
  ~BC_Xfer();

  uint8_t **output_rows, **input_rows;
  uint8_t *out_yp, *out_up, *out_vp, *out_ap;
  uint8_t *in_yp, *in_up, *in_vp, *in_ap;
  int in_x, in_y; unsigned in_w, in_h;
  int out_x, out_y; unsigned out_w, out_h;
  int in_colormodel, out_colormodel;
  uint32_t bg_color, total_in_w, total_out_w;
  int scale;
  int out_pixelsize, in_pixelsize;
  int *row_table, *column_table;
  uint32_t bg_r, bg_g, bg_b;

  void xfer();
  void xfer_slices(int slices);
  typedef void (BC_Xfer::*xfer_fn)(unsigned y0, unsigned y1);
  xfer_fn xfn;

  class Slicer : public ListItem<Slicer>, public Thread {
  public:
    Condition *init, *complete;
    Slicer(BC_Xfer *xp);
    ~Slicer();
    void slice(BC_Xfer *xp, unsigned y0, unsigned y1);
    void run();
    BC_Xfer *xp;
    int done, y0, y1;
  };

  class SlicerList : public List<Slicer>, public Mutex {
  public:
    int count;
    Slicer *get_slicer(BC_Xfer *xp);
    void reset();
    SlicerList();
    ~SlicerList();
  };
  static SlicerList slicers;

  void init(
    uint8_t **output_rows, int out_colormodel, int out_x, int out_y, int out_w, int out_h,
      uint8_t *out_yp, uint8_t *out_up, uint8_t *out_vp, uint8_t *out_ap, int out_rowspan,
    uint8_t **input_rows, int in_colormodel, int in_x, int in_y, int in_w, int in_h,
      uint8_t *in_yp, uint8_t *in_up, uint8_t *in_vp, uint8_t *in_ap, int in_rowspan,
    int bg_color);

// generated code concatentated here
