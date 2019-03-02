#include "overlaysample.h"
// parallel build
#define BLEND(FN) XSAMPLE(FN, z_float, z_float, 1.f, 3, 0, 0.f);
void SampleUnit::rgb_float() { BLEND_SWITCH(BLEND); }

