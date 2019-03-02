#include "overlaysample.h"
// parallel build
#define BLEND(FN) XSAMPLE(FN, z_int32_t, z_uint8_t, 0xff, 3, 0x80, .5f);
void SampleUnit::yuv888() { BLEND_SWITCH(BLEND); }

