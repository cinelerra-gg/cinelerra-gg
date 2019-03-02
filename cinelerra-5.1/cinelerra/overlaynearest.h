#ifndef __OVERLAYNEAREST_H__
#define __OVERLAYNEAREST_H__
#include "overlayframe.h"

#define XBLEND_3NN(FN, temp_type, type, max, components, ofs, round) { \
	temp_type opcty = fade * max + round, trnsp = max - opcty; \
	type** output_rows = (type**)output->get_rows(); \
	type** input_rows = (type**)input->get_rows(); \
	ox *= components; \
 \
	for( int i=pkg->out_row1; i<pkg->out_row2; ++i ) { \
		int *lx = engine->in_lookup_x; \
		type* in_row = input_rows[*ly++]; \
		type* output = output_rows[i] + ox; \
		for( int j=ow; --j>=0; ) { \
			in_row += *lx++; \
			if( components == 4 ) { \
				temp_type r, g, b, a; \
				ALPHA4_BLEND(FN, temp_type, in_row, output, max, ofs, ofs, round); \
				ALPHA4_STORE(output, ofs, max); \
			} \
			else { \
				temp_type r, g, b; \
				ALPHA3_BLEND(FN, temp_type, in_row, output, max, ofs, ofs, round); \
				ALPHA3_STORE(output, ofs, max); \
			} \
			output += components; \
		} \
	} \
} break

#endif
