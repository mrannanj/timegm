/* Copyright (c) 2007-2017 Dovecot authors, see the included COPYING file */

#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define SECONDS_BASE 60
#define MINUTES_BASE 60
#define HOURS_BASE 24
#define MONTHS_BASE 12

#define SECONDS_IN_MINUTE 60
#define SECONDS_IN_HOUR (60 * SECONDS_IN_MINUTE)
#define SECONDS_IN_DAY (24 * SECONDS_IN_HOUR)
#define DAYS_IN_YEAR 365
#define SECONDS_IN_YEAR (DAYS_IN_YEAR * SECONDS_IN_DAY)

#define EPOCH_YEAR 70
#define YEAR_ZERO 1900

const int days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

static int isleap(int y)
{
	y += YEAR_ZERO;
	return (y%4 == 0) && (y % 100 != 0 || y % 400 == 0);
}

static int month_days(int m, int y)
{
	assert(m >= 0);
	assert(m <= 11);
	if (m == 1 && isleap(y))
		return 29;
	return days_in_month[m];
}

/* FIXME: under/overflows */
static void normalize_unit(int *unit, int *next, int base)
{
	int mod = *unit % base;
	int whole = *unit / base;

	if (*unit >= base) {
		*next += whole;
		*unit = mod;
	} else if (*unit < 0) {
		*next += whole;
		if (mod < 0) {
			*unit = base + mod;
			*next -= 1;
		} else {
			*unit = 0;
		}
	}
	assert(*unit != base);
	assert(*unit >= 0);
}

static int rewind_month(int *m, int *y)
{
	if (*m > 0) {
		*m -= 1;
	} else {
		*y -= 1;
		*m = 11;
	}
	return month_days(*m, *y);
}

static void advance_month(int *m, int *y)
{
	if (*m == 11) {
		*m = 0;
		*y += 1;
	} else {
		*m += 1;
	}
	assert(*m >= 0);
	assert(*m <= 11);
}

static void normalize_days(int *days, int *m, int *y)
{
	int md;

	assert(*m >= 0);
	assert(*m <= 11);
	while (*days <= 0) {
		*days += rewind_month(m, y);
	}
	while (*days > (md = month_days(*m, *y))) {
		advance_month(m, y);
		*days -= md;
	}
	assert(*days >= 1);
	assert(*days <= 31);
}

static void normalize_tm(struct tm *tm)
{
	normalize_unit(&tm->tm_sec, &tm->tm_min, SECONDS_BASE);
	normalize_unit(&tm->tm_min, &tm->tm_hour, MINUTES_BASE);
	normalize_unit(&tm->tm_hour, &tm->tm_mday, HOURS_BASE);
	normalize_unit(&tm->tm_mon, &tm->tm_year, MONTHS_BASE);
	normalize_days(&tm->tm_mday, &tm->tm_mon, &tm->tm_year);
	normalize_unit(&tm->tm_mon, &tm->tm_year, MONTHS_BASE);
}

time_t timegm_cc0(struct tm *tm)
{
	time_t ret = 0;
	int x;

	normalize_tm(tm);
	assert(tm->tm_sec >= 0 && tm->tm_sec <= 60);
	assert(tm->tm_min >= 0 && tm->tm_min <= 59);
	assert(tm->tm_hour >= 0 && tm->tm_hour <= 23);
	assert(tm->tm_mday >= 1);
	assert(tm->tm_mday <= 31);
	assert(tm->tm_mon >= 0);
	assert(tm->tm_mon <= 11);

	if (tm->tm_year > EPOCH_YEAR) {
		for (x = EPOCH_YEAR; x < tm->tm_year; ++x) {
			ret += SECONDS_IN_YEAR;
			if (isleap(x))
				ret += SECONDS_IN_DAY;
		}
	} else if (tm->tm_year < EPOCH_YEAR) {
		for (x = EPOCH_YEAR-1; x >= tm->tm_year; --x) {
			ret -= SECONDS_IN_YEAR;
			if (isleap(x))
				ret -= SECONDS_IN_DAY;
		}
	}
	for (x = 0; x < tm->tm_mon; ++x) {
		int days = days_in_month[x];
		days += (x == 1 && isleap(tm->tm_year));
		ret += days*SECONDS_IN_DAY;
	}
	for (x = 1; x < tm->tm_mday; ++x) {
		ret += SECONDS_IN_DAY;
	}
	ret += SECONDS_IN_HOUR * tm->tm_hour;
	ret += SECONDS_IN_MINUTE * tm->tm_min;
	ret += tm->tm_sec;
	return ret;
}
