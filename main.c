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

int get_random_int(void)
{
	return random();
}

int main(int argc, char **argv) {
	int verbose = 0, tsc_freq = 1000000000, samples = 30, start = 0, ignore = 0;
	int i, opt;
	double slope, intercept, offset, variance, varsum, max_offset;

	while ((opt = getopt(argc, argv, "vf:n:s:i:")) != -1) {
		switch (opt) {
			case 'v':
				verbose++;
				break;
			case 'f':
				tsc_freq = atoi(optarg);
				break;
			case 'n':
				samples = atoi(optarg);
				break;
			case 's':
				start = atoi(optarg);
				break;
			case 'i':
				ignore = atoi(optarg);
				break;
			default:
				printk("tktest [-v] [-f freq] [-n samples] [-s start] [-i ignore]\n");
				exit(1);
		}
	}

	uint64_t ts_x[samples], ts_y[samples];
	double x[samples], y[samples];

	srandom(12341234);

	tk_test(ts_x, ts_y, samples, tsc_freq);

	for (i = 0; i < samples; i++) {
		x[i] = ts_x[i];
		y[i] = ts_y[i];
	}

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
				i > 0 ? (y[i] - y[i - 1]) / (x[i] - x[i - 1]) * tsc_freq / 1e9 - 1.0 : 0.0,
				y[i] - x[i] / tsc_freq * 1e9, offset);
		}
	}

	printk("samples: %d-%d reg: %d-%d slope: %.2f dev: %.1f max: %.1f freq: %.5f\n",
			ignore + 1, samples, start + ignore + 1, samples,
			slope, sqrt(varsum / samples), max_offset,
			(slope / 1e9 * tsc_freq - 1.0) * 1e6);

	return 0;
}
