#include "overlaysample.h"
// parallel build
#define BLEND(FN) XSAMPLE(FN, z_int32_t, z_uint8_t,  0xff, 4, 0, .5f);
void SampleUnit::rgba8888() { BLEND_SWITCH(BLEND); }

