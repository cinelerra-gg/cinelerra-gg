#include "overlaysample.h"
// parallel build
#define BLEND(FN) XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 3, 0x8000, .5f);
void SampleUnit::yuv161616() { BLEND_SWITCH(BLEND); }

