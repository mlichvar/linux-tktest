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

#include "tk_test.h"
#include "regress.h"

#define TSC_FREQ 1000000
#define SAMPLES 30

void printk(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
}

int main() {
	uint64_t ts_x[SAMPLES], ts_y[SAMPLES];
	double x[SAMPLES], y[SAMPLES], slope, intercept, offset, variance, max_offset;
	int i;

	tk_test(ts_x, ts_y, SAMPLES, TSC_FREQ);

	for (i = 0; i < SAMPLES; i++) {
		x[i] = ts_x[i];
		y[i] = ts_y[i];
	}

	regress(x, y, SAMPLES, &intercept, &slope, &variance);
	max_offset = 0.0;

	for (i = 0; i < SAMPLES; i++) {
		offset = x[i] * slope + intercept - y[i];
		if (fabs(offset) > max_offset)
			max_offset = fabs(offset);
		printk("%5d %lld %lld %9.1f %9.1f\n", i, ts_x[i], ts_y[i],
				y[i] - x[i] / TSC_FREQ * 1e6, offset);
	}

	printk("n: %d, slope: %.2f (%.2f GHz), dev: %.1f ns, max: %.1f ns, freq: %.5f ppm\n",
			SAMPLES, slope, 1.0 / slope, sqrt(variance), max_offset,
			(1e6 / slope / TSC_FREQ - 1.0) * 1e6);

	return 0;
}
