#include "overlaysample.h"
// parallel build
#define BLEND(FN) XSAMPLE(FN, z_int64_t, z_uint16_t, 0xffff, 4, 0x8000, .5f);
void SampleUnit::yuva16161616() { BLEND_SWITCH(BLEND); }

