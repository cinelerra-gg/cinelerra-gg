#include "overlaydirect.h"
// parallel build
#define BLEND(FN) XBLEND(FN, z_float, z_float, 1.f, 4, 0, 0.f);
void DirectUnit::rgba_float() { BLEND_SWITCH(BLEND); }

