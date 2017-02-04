/* Copyright (c) 2007-2017 Dovecot authors, see the included COPYING file */

#include <time.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "timegm.h"

/* FIXME: struct tm is problematic here since it is affected by ifdefs. */
/* FIXME: problematic that numbers in from need to be smaller than in to. */
struct timegm_test_range {
	struct tm from, to;
	const char *name;
};

#if 0
struct tm
{
	int tm_sec;	/* Seconds. [0-60] (1 leap second) */
	int tm_min;	/* Minutes. [0-59] */
	int tm_hour;	/* Hours. [0-23] */
	int tm_mday;	/* Day. [1-31] */
	int tm_mon;	/* Month. [0-11] */
	int tm_year;	/* Year- 1900.  */
	int tm_wday;	/* Day of week. [0-6] */
	int tm_yday;	/* Days in year. [0-365] */
	int tm_isdst;	/* DST.[-1/0/1] */

# ifdef __USE_MISC
	long int tm_gmtoff;	/* Seconds east of UTC.  */
	const char *tm_zone;	/* Timezone abbreviation.  */
# else
	long int __tm_gmtoff;	/* Seconds east of UTC.  */
	const char *__tm_zone;	/* Timezone abbreviation.  */
# endif
};
#endif

struct timegm_test_range test_in[] = {
	{
		{ 0, 0, 0, 1, 0, 1970, 0, 0, -1, 0, 0 },
		{ 0, 0, 0, 1, 0, 1970, 0, 0, -1, 0, 0 },
		"epoch test",
	},
	{
		{ 0, 59, 23, -10, -50, 1800, 0, 0, -1, 0, 0 },
		{ 0, 59, 23, -5, -45, 2000, 0, 0, -1, 0, 0 },
		"negative days and months",
	},
	{
		{ -1000, 0, 0, 1, 0, 2000, 0, 0, -1, 0, 0 },
		{ 1000, 0, 0, 1, 0, 2000, 0, 0, -1, 0, 0 },
		"seconds normalizing",
	},
	{
		{ 0, -1000, 0, 1, 0, 2000, 0, 0, -1, 0, 0 },
		{ 0, 1000, 0, 1, 0, 2000, 0, 0, -1, 0, 0 },
		"minutes normalizing",
	},
	{
		{ 0, 0, -1000, 1, 0, 2000, 0, 0, -1, 0, 0 },
		{ 0, 0, 1000, 1, 0, 2000, 0, 0, -1, 0, 0 },
		"hours normalizing",
	},
	{
		{ 0, 0, 0, -10000, 0, 2000, 0, 0, -1, 0, 0 },
		{ 0, 0, 0, 10000, 0, 2000, 0, 0, -1, 0, 0 },
		"days normalizing",
	},
	{
		{ 0, 0, 0, 1, -3000, 2100, 0, 0, -1, 0, 0 },
		{ 0, 0, 0, 1, 3000, 2100, 0, 0, -1, 0, 0 },
		"months normalizing",
	},
	{
		{ 0, 0, 0, 1, 0, 1900, 0, 0, -1, 0, 0 },
		{ 0, 0, 0, 11, 0, 2000, 0, 0, -1, 0, 0 },
		"negative time_t values",
	},
	{
		/* FIXME: this is difficult to specify */
		{ 0, 59, 23, 31, 11, 1969, 0, 0, -1, 0, 0 },
		{ 120, 59, 23, 31, 11, 1969, 0, 0, -1, 0, 0 },
		"both sides of epoch",
	},
	{
		{ 0, 59, 23, 31, 11, 1800, 0, 0, -1, 0, 0 },
		{ 0, 59, 23, 31, 11, 2000, 0, 0, -1, 0, 0 },
		"before 1900",
	},
	{
		{ 0, 0, 0, 1, 0, 1970, 0, 0, -1, 0, 0 },
		{ 59, 59, 23, 28, 11, 1970, 0, 0, -1, 0, 0 },
		"normalized year",
	},
};

static bool cmp_tm(const struct tm *tm1, const struct tm *tm2)
{
	bool res = true;
	if (tm1->tm_year != tm2->tm_year) {
		printf("year %d != %d, diff %d\n", tm1->tm_year, tm2->tm_year,
				tm1->tm_year - tm2->tm_year);
		res = false;
	}
	if (tm1->tm_mon != tm2->tm_mon) {
		printf("mon %d != %d, diff %d\n", tm1->tm_mon, tm2->tm_mon,
			tm1->tm_mon - tm2->tm_mon);
		res = false;
	}
	if (tm1->tm_mday != tm2->tm_mday) {
		printf("mday %d != %d, diff %d\n", tm1->tm_mday, tm2->tm_mday,
			tm1->tm_mday - tm2->tm_mday);
		res = false;
	}
	if (tm1->tm_hour != tm2->tm_hour) {
		printf("hour %d != %d, diff %d\n", tm1->tm_hour, tm2->tm_hour,
			tm1->tm_hour - tm2->tm_hour);
		res = false;
	}
	if (tm1->tm_min != tm2->tm_min) {
		printf("min %d != %d, diff %d\n", tm1->tm_min, tm2->tm_min,
			tm1->tm_min - tm2->tm_min);
		res = false;
	}
	if (tm1->tm_sec != tm2->tm_sec) {
		printf("sec %d != %d, diff %d\n", tm1->tm_sec, tm2->tm_sec,
			tm1->tm_sec - tm2->tm_sec);
		res = false;
	}
	return res;
}

static bool test_tm(int y, int mon, int d, int h, int min, int s)
{
	time_t old_t, new_t;
	struct tm in, new_tm, old_tm;

	memset(&in, 0, sizeof(in));
	in.tm_year = y - 1900;
	in.tm_mon =  mon;
	in.tm_mday = d;
	in.tm_hour = h;
	in.tm_min = min;
	in.tm_sec = s;

	old_tm = in;
	new_tm = in;

	old_t = timegm(&old_tm);
	new_t = timegm_cc0(&new_tm);
	if ((old_t != new_t) | !cmp_tm(&old_tm, &new_tm))
	{
		printf("old_t %ld, new_t %ld, diff %ld\n", old_t, new_t,
			old_t - new_t);
		printf("in year %d, mon %d, mday %d, hour %d, min %d, sec %d\n",
			in.tm_year, in.tm_mon, in.tm_mday, in.tm_hour,
			in.tm_min, in.tm_sec);
		return false;
	}
	return true;
}

static void run_tests()
{
	unsigned i;
	for (i = 0; i < sizeof(test_in)/sizeof(test_in[0]); ++i) {
		int y, mon, mday, hour, min, sec;
		unsigned n = 0;
		for (y = test_in[i].from.tm_year; y <= test_in[i].to.tm_year; ++y)
		for (mon = test_in[i].from.tm_mon; mon <= test_in[i].to.tm_mon; ++mon)
		for (mday = test_in[i].from.tm_mday; mday <= test_in[i].to.tm_mday; ++mday)
		for (hour = test_in[i].from.tm_hour; hour <= test_in[i].to.tm_hour; ++hour)
		for (min = test_in[i].from.tm_min; min <= test_in[i].to.tm_min; ++min)
		for (sec = test_in[i].from.tm_sec; sec <= test_in[i].to.tm_sec; ++sec) {
			if (!test_tm(y, mon, mday, hour, min, sec)) {
				printf("test %s: %u FAILED\n", test_in[i].name, n);
				return;
			}
			n += 1;
		}
		printf("test %s: %u OK\n", test_in[i].name, n);
	}
}

int main(void)
{
	run_tests();
	return 0;
}
