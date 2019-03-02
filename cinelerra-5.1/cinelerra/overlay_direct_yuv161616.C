#include "overlaydirect.h"
// parallel build
#define BLEND(FN) XBLEND(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0x8000, .5f);
void DirectUnit::yuv161616() { BLEND_SWITCH(BLEND); }

