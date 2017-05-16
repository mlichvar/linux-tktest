/*
 **********************************************************************
 * Copyright (C) 2013  Miroslav Lichvar <mlichvar@redhat.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 **********************************************************************
 */

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "tk_test.h"
#include "regress.h"

void printk(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

void printk_deferred(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

unsigned int get_random_u32(void)
{
	return random();
}

int main(int argc, char **argv) {
	struct tk_test_params test_params = {
		.clock_freq = 1000000000,
		.update_interval = 4095,
		.random_update = 1,
		.nohz = 1,
		.freq_offset = -100000,
		.freq_step = 0,
		.freq_step_interval = 0,
	};
	int verbose = 0, total_samples = 50, start = 0, ignore = 0, split = 0;
	int i, opt, samples;
	double slope, intercept, offset, variance, varsum, max_offset;

	while ((opt = getopt(argc, argv, "vhHRf:n:o:p:s:i:t:u:")) != -1) {
		switch (opt) {
			case 'v':
				verbose++;
				break;
			case 'H':
				test_params.nohz = 0;
				break;
			case 'R':
				test_params.random_update = 0;
				break;
			case 'f':
				test_params.clock_freq = atol(optarg);
				break;
			case 'n':
				total_samples = atoi(optarg);
				break;
			case 'o':
				test_params.freq_offset = atoi(optarg);
				break;
			case 'p':
				test_params.freq_step = atoi(optarg);
				break;
			case 's':
				start = atoi(optarg);
				break;
			case 'i':
				ignore = atoi(optarg);
				break;
			case 't':
				split = atoi(optarg);
				test_params.freq_step_interval = split;
				break;
			case 'u':
				test_params.update_interval = atoi(optarg);
				break;
			case 'h':
			default:
				printk( "Usage: tktest [options]\n\n"
					"options:\n"
					"	-f freq		clock frequency (Hz)\n"
					"	-o freq		initial frequency offset (ppb)\n"
					"	-p freq		frequency step (ppb)\n"
					"	-n samples	collected samples\n"
					"	-s sample	first regressed sample\n"
					"	-i samples	ignore samples\n"
					"	-t samples	split samples for stats\n"
					"	-u samples	clock update interval\n"
					"	-H		disable nohz\n"
					"	-R		disable random update interval\n"
					"	-v		print samples\n"
					"	-h		print help\n");
				exit(opt != 'h');
		}
	}

	uint64_t ts_x[total_samples], ts_y[total_samples];
	double x[total_samples], y[total_samples];

	srandom(12341234);

	tk_test(ts_x, ts_y, total_samples, &test_params);

	for (i = 0; i < total_samples; i++) {
		x[i] = ts_x[i];
		y[i] = ts_y[i];
	}

	if (split == 0)
		split = total_samples;

	for (samples = split; samples <= total_samples; samples += split, ignore += split) {
		regress(x + start + ignore, y + start + ignore, samples - start - ignore,
				&intercept, &slope, &variance);
		max_offset = 0.0;
		varsum = 0.0;

		for (i = ignore; i < samples; i++) {
			offset = x[i] * slope + intercept - y[i];
			varsum += offset * offset;
			if (fabs(offset) > max_offset)
				max_offset = fabs(offset);
			if (verbose) {
				printk("%5d %lld %lld %e %9.1f %9.1f\n", i + 1,
					ts_x[i], ts_y[i],
					i > 0 ? (y[i] - y[i - 1]) / (x[i] - x[i - 1]) *
						test_params.clock_freq / 1e9 - 1.0 : 0.0,
					y[i] - x[i] / test_params.clock_freq * 1e9, offset);
			}
		}

		printk("samples: %d-%d reg: %d-%d slope: %.2f dev: %.1f max: %.1f freq: %.5f\n",
				ignore + 1, samples, start + ignore + 1, samples,
				slope, sqrt(varsum / samples), max_offset,
				(slope / 1e9 * test_params.clock_freq - 1.0) * 1e6);
	}

	return 0;
}
