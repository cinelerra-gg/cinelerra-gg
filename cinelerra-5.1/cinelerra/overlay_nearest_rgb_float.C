#include "overlaynearest.h"
// parallel build
#define BLEND(FN) XBLEND_3NN(FN, z_float, z_float, 1.f, 3, 0, 0.f);
void NNUnit::rgb_float() { BLEND_SWITCH(BLEND); }

