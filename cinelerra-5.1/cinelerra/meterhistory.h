#ifndef __METERHISTORY_H__
#define __METERHISTORY_H__

#include <stdint.h>

class MeterHistory
{
public:
	MeterHistory();
	~MeterHistory();

	int size, channels;
	int *current_peak;
	int64_t *samples;
	double **values;

	void init(int chs, int sz);
	void reset_channel(int ch);
	void set_peak(int ch, double peak, int64_t pos);
	double get_peak(int ch, int idx);
	int get_nearest(int64_t pos, int64_t tolerance);
};

#endif
