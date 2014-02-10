#ifndef TK_TEST_H
#define TK_TEST_H

struct tk_test_params {
	unsigned long clock_freq;
	unsigned int update_interval;
	int random_update;
	int nohz;
	int freq_offset;
};

void tk_test(uint64_t *ts_x, uint64_t *ts_y, int samples, struct tk_test_params *params);

#endif
