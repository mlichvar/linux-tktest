/*
 * This is a stripped down version of linux/kernel/time/clocksource.c
 * and various dummy functions needed to compile the program.
 *
 * Copyright (C) 2004, 2005 IBM, John Stultz (johnstul@us.ibm.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/clocksource.h>

u64 jiffies_64;
long jiffies_lock;

void calc_global_load() { }
unsigned long _raw_spin_lock_irqsave(raw_spinlock_t *lock) { return 0; }
void __lockfunc _raw_spin_lock(raw_spinlock_t *lock) { }
void _raw_spin_unlock_irqrestore(raw_spinlock_t *lock, unsigned long flags) { }
void clock_was_set_delayed(void) { }
void clock_was_set(void) { }
void clockevents_notify(unsigned long reason, void *arg) { }
void clockevents_resume() { }
void clockevents_suspend() { }
void clocksource_resume() { }
void clocksource_suspend() { }
void hrtimers_resume() { }
void module_put() { }
void ntp_clear() { }
void ntp_init() { }
ktime_t ntp_get_next_leap(void) { ktime_t t = { KTIME_MAX }; return t; }
int raw_notifier_call_chain(struct raw_notifier_head *nh, unsigned long val, void *v) { return 0; }
int raw_notifier_chain_register(struct raw_notifier_head *nh, struct notifier_block *n) { return 0; }
int raw_notifier_chain_unregister(struct raw_notifier_head *nh, struct notifier_block *n) { return 0; }
void register_syscore_ops() { }
void stop_machine() { }
void tick_clock_notify() { }
void tick_suspend() { }
void tick_resume() { }
void tk_debug_account_sleep_time() { }
void touch_softlockup_watchdog() { }
void try_module_get() { }
void update_vsyscall() { }
void warn_slowpath_null(const char *file, const int line) { }
int __do_adjtimex() { return 0; }
int ntp_validate_timex() { return 0; }
void ntp_notify_cmos_timer() { }

struct timespec ns_to_timespec(const int64_t nsec) {
	return (struct timespec) {nsec / 1000000000, nsec % 1000000000};
}

void set_normalized_timespec(struct timespec *ts, time_t sec, s64 nsec) {
	ts->tv_sec = sec + nsec / 1000000000;
	ts->tv_nsec = nsec / 1000000000;
}

static u32 clocksource_max_adjustment(struct clocksource *cs)
{
	u64 ret;
	/*
	 * We won't try to correct for more than 11% adjustments (110,000 ppm),
	 */
	ret = (u64)cs->mult * 11;
	do_div(ret,100);
	return (u32)ret;
}

/**
 * clocksource_max_deferment - Returns max time the clocksource can be deferred
 * @cs:         Pointer to clocksource
 *
 */
static u64 clocksource_max_deferment(struct clocksource *cs)
{
	u64 max_nsecs, max_cycles;

	/*
	 * Calculate the maximum number of cycles that we can pass to the
	 * cyc2ns function without overflowing a 64-bit signed result. The
	 * maximum number of cycles is equal to ULLONG_MAX/(cs->mult+cs->maxadj)
	 * which is equivalent to the below.
	 * max_cycles < (2^63)/(cs->mult + cs->maxadj)
	 * max_cycles < 2^(log2((2^63)/(cs->mult + cs->maxadj)))
	 * max_cycles < 2^(log2(2^63) - log2(cs->mult + cs->maxadj))
	 * max_cycles < 2^(63 - log2(cs->mult + cs->maxadj))
	 * max_cycles < 1 << (63 - log2(cs->mult + cs->maxadj))
	 * Please note that we add 1 to the result of the log2 to account for
	 * any rounding errors, ensure the above inequality is satisfied and
	 * no overflow will occur.
	 */
	max_cycles = 1ULL << (63 - (ilog2(cs->mult + cs->maxadj) + 1));

	/*
	 * The actual maximum number of cycles we can defer the clocksource is
	 * determined by the minimum of max_cycles and cs->mask.
	 * Note: Here we subtract the maxadj to make sure we don't sleep for
	 * too long if there's a large negative adjustment.
	 */
	max_cycles = min_t(u64, max_cycles, (u64) cs->mask);
	max_nsecs = clocksource_cyc2ns(max_cycles, cs->mult - cs->maxadj,
					cs->shift);

	/*
	 * To ensure that the clocksource does not wrap whilst we are idle,
	 * limit the time the clocksource can be deferred by 12.5%. Please
	 * note a margin of 12.5% is used because this can be computed with
	 * a shift, versus say 10% which would require division.
	 */
	return max_nsecs - (max_nsecs >> 3);
}

void
clocks_calc_mult_shift(u32 *mult, u32 *shift, u32 from, u32 to, u32 maxsec)
{
	u64 tmp;
	u32 sft, sftacc= 32;

	/*
	 * Calculate the shift factor which is limiting the conversion
	 * range:
	 */
	tmp = ((u64)maxsec * from) >> 32;
	while (tmp) {
		tmp >>=1;
		sftacc--;
	}

	/*
	 * Find the conversion shift/mult pair which has the best
	 * accuracy and fits the maxsec conversion range:
	 */
	for (sft = 32; sft > 0; sft--) {
		tmp = (u64) to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

void __clocksource_update_freq_scale(struct clocksource *cs, u32 scale, u32 freq)
{
	u64 sec;
	/*
	 * Calc the maximum number of seconds which we can run before
	 * wrapping around. For clocksources which have a mask > 32bit
	 * we need to limit the max sleep time to have a good
	 * conversion precision. 10 minutes is still a reasonable
	 * amount. That results in a shift value of 24 for a
	 * clocksource with mask >= 40bit and f >= 4GHz. That maps to
	 * ~ 0.06ppm granularity for NTP. We apply the same 12.5%
	 * margin as we do in clocksource_max_deferment()
	 */
	sec = (cs->mask - (cs->mask >> 3));
	do_div(sec, freq);
	do_div(sec, scale);
	if (!sec)
		sec = 1;
	else if (sec > 600 && cs->mask > UINT_MAX)
		sec = 600;

	clocks_calc_mult_shift(&cs->mult, &cs->shift, freq,
			       NSEC_PER_SEC / scale, sec * scale);

	/*
	 * for clocksources that have large mults, to avoid overflow.
	 * Since mult may be adjusted by ntp, add an safety extra margin
	 *
	 */
	cs->maxadj = clocksource_max_adjustment(cs);
	while ((cs->mult + cs->maxadj < cs->mult)
		|| (cs->mult - cs->maxadj > cs->mult)) {
		cs->mult >>= 1;
		cs->shift--;
		cs->maxadj = clocksource_max_adjustment(cs);
	}

	cs->max_idle_ns = clocksource_max_deferment(cs);
}
