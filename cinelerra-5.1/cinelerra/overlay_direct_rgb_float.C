#include "overlaydirect.h"
// parallel build
#define BLEND(FN) XBLEND(FN, z_float, z_float, 1.f, 3, 0, 0.f);
void DirectUnit::rgb_float() { BLEND_SWITCH(BLEND); }

