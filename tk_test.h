#ifndef TK_TEST_H
#define TK_TEST_H

struct tk_test_params {
	uint64_t clock_freq;
};

void tk_test(uint64_t *ts_x, uint64_t *ts_y, int samples, struct tk_test_params *params);

#endif
