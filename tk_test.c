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

#include <linux/clocksource.h>
#include <linux/random.h>

void xtime_update(unsigned long ticks);

cycle_t simtsc;
int simtsc_freq;
double simtsc_frac;

cycle_t simclocksource_read(struct clocksource *cs) {
	return simtsc;
}

struct clocksource simclocksource = {
	.name                   = "tsc",
	.rating                 = 300,
	.read 			= simclocksource_read,
	.mask                   = CLOCKSOURCE_MASK(64),
	.flags                  = CLOCK_SOURCE_IS_CONTINUOUS |
				  CLOCK_SOURCE_MUST_VERIFY,
};

struct clocksource * clocksource_default_clock(void) {
	return &simclocksource;
}

u64 ntp_freq = 1000000000ULL;

u64 ntp_tick_length(void) {
	return (ntp_freq << NTP_SCALE_SHIFT) / NTP_INTERVAL_FREQ;
}

int second_overflow(unsigned long secs) {
	if (1)
		;
	else if (secs == 1)
		ntp_freq += 100000;
	else if (secs == 2)
		ntp_freq -= 100000;

	return 0;
}

void advance_ticks(int ticks, int frac, int repeat) {
	int i;
	if (1) {
		for (i = 0; i < repeat; i++) {
			simtsc_frac += (double)ticks * simtsc_freq / HZ / frac;
			simtsc += (cycle_t)simtsc_frac;
			simtsc_frac -= (cycle_t)simtsc_frac;
			xtime_update(ticks);
		}
	} else {
		for (i = 0; i < repeat * ticks / frac; i++) {
			simtsc_frac += (double)simtsc_freq / HZ;
			simtsc += (cycle_t)simtsc_frac;
			simtsc_frac -= (cycle_t)simtsc_frac;
			xtime_update(1);
		}
	}
}

void tk_test(uint64_t *ts_x, uint64_t *ts_y, int samples, int freq) {
	struct timespec ts;
	int i;

	simtsc = 0;
	simtsc_freq = freq;
	simtsc_frac = 0.0;

	__clocksource_updatefreq_scale(&simclocksource, 1, simtsc_freq);
	timekeeping_init();

	advance_ticks(3, 4, 1);
	ntp_freq -= 100000;

	for (i = 0; i < samples; i++) {
		int rand = get_random_int();

		rand = rand&((1<<12)-1); /* 0-4k */
		advance_ticks(rand, 1, 1);

		getnstimeofday(&ts);
		ts_x[i] = simtsc;
		ts_y[i] = ts.tv_sec * 1000000000ULL + ts.tv_nsec;
	}
}
