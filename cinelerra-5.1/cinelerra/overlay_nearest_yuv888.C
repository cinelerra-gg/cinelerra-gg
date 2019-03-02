#include "overlaynearest.h"
// parallel build
#define BLEND(FN) XBLEND_3NN(FN, z_int32_t, z_uint8_t, 0xff, 3, 0x80, .5f);
void NNUnit::yuv888() { BLEND_SWITCH(BLEND); }

