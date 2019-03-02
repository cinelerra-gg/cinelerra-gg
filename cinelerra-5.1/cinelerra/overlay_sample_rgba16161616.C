#include "overlaysample.h"
// parallel build 
#define BLEND(FN) XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0, .5f);
void SampleUnit::rgba16161616() { BLEND_SWITCH(BLEND); }

