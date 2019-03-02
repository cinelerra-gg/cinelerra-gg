#ifndef __OVERLAYSAMPLE_H__
#define __OVERLAYSAMPLE_H__
#include "overlayframe.h"

#define XSAMPLE(FN, temp_type, type, max, components, ofs, round) { \
	float temp[oh*components]; \
	temp_type opcty = fade * max + round, trnsp = max - opcty; \
	type **output_rows = (type**)voutput->get_rows() + o1i; \
	type **input_rows = (type**)vinput->get_rows(); \
 \
	for(int i = pkg->out_col1; i < pkg->out_col2; i++) { \
		type *input = input_rows[i - engine->col_out1 + engine->row_in]; \
		float *tempp = temp; \
		if( !k ) { /* direct copy case */ \
			type *ip = input + i1i * components; \
			for( int j=oh; --j>=0; ) { \
				*tempp++ = *ip++; \
				*tempp++ = *ip++ - ofs; \
				*tempp++ = *ip++ - ofs; \
				if( components == 4 ) *tempp++ = *ip++; \
			} \
		} \
		else { /* resample */ \
			for( int j=0; j<oh; ++j ) { \
				float racc=0.f, gacc=0.f, bacc=0.f, aacc=0.f; \
				int ki = lookup_sk[j], x = lookup_sx0[j]; \
				type *ip = input + x * components; \
				while(x < lookup_sx1[j]) { \
					float kv = k[abs(ki >> INDEX_FRACTION)]; \
					/* handle fractional pixels on edges of input */ \
					if(x == i1i) kv *= i1f; \
					if(++x == i2i) kv *= i2f; \
					racc += kv * *ip++; \
					gacc += kv * (*ip++ - ofs); \
					bacc += kv * (*ip++ - ofs); \
					if( components == 4 ) { aacc += kv * *ip++; } \
					ki += kd; \
				} \
				float wacc = lookup_wacc[j]; \
				*tempp++ = racc * wacc; \
				*tempp++ = gacc * wacc; \
				*tempp++ = bacc * wacc; \
				if( components == 4 ) { *tempp++ = aacc * wacc; } \
			} \
		} \
 \
		/* handle fractional pixels on edges of output */ \
		temp[0] *= o1f;   temp[1] *= o1f;   temp[2] *= o1f; \
		if( components == 4 ) temp[3] *= o1f; \
		tempp = temp + (oh-1)*components; \
		tempp[0] *= o2f;  tempp[1] *= o2f;  tempp[2] *= o2f; \
		if( components == 4 ) tempp[3] *= o2f; \
		tempp = temp; \
		/* blend output */ \
		for( int j=0; j<oh; ++j ) { \
			type *output = output_rows[j] + i * components; \
			if( components == 4 ) { \
				temp_type r, g, b, a; \
				ALPHA4_BLEND(FN, temp_type, tempp, output, max, 0, ofs, round); \
				ALPHA4_STORE(output, ofs, max); \
			} \
			else { \
				temp_type r, g, b; \
				ALPHA3_BLEND(FN, temp_type, tempp, output, max, 0, ofs, round); \
				ALPHA3_STORE(output, ofs, max); \
			} \
			tempp += components; \
		} \
	} \
} break

#endif
