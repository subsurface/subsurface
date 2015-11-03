#include <string.h>
#include "dive.h"

/*
 * Convert 64-bit timestamp to 'struct tm' in UTC.
 *
 * On 32-bit machines, only do 64-bit arithmetic for the seconds
 * part, after that we do everything in 'long'. 64-bit divides
 * are unnecessary once you're counting minutes (32-bit minutes:
 * 8000+ years).
 */
void utc_mkdate(timestamp_t timestamp, struct tm *tm)
{
	static const int mdays[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	static const int mdays_leap[] = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	unsigned long val;
	unsigned int leapyears;
	int m;
	const int *mp;

	memset(tm, 0, sizeof(*tm));

	/* seconds since 1970 -> minutes since 1970 */
	tm->tm_sec = timestamp % 60;
	val = timestamp /= 60;

	/* Do the simple stuff */
	tm->tm_min = val % 60;
	val /= 60;
	tm->tm_hour = val % 24;
	val /= 24;

	/* Jan 1, 1970 was a Thursday (tm_wday=4) */
	tm->tm_wday = (val + 4) % 7;

	/*
	 * Now we're in "days since Jan 1, 1970". To make things easier,
	 * let's make it "days since Jan 1, 1968", since that's a leap-year
	 */
	val += 365 + 366;

	/* This only works up until 2099 (2100 isn't a leap-year) */
	leapyears = val / (365 * 4 + 1);
	val %= (365 * 4 + 1);
	tm->tm_year = 68 + leapyears * 4;

	/* Handle the leap-year itself */
	mp = mdays_leap;
	if (val > 365) {
		tm->tm_year++;
		val -= 366;
		tm->tm_year += val / 365;
		val %= 365;
		mp = mdays;
	}

	for (m = 0; m < 12; m++) {
		if (val < *mp)
			break;
		val -= *mp++;
	}
	tm->tm_mday = val + 1;
	tm->tm_mon = m;
}

timestamp_t utc_mktime(struct tm *tm)
{
	static const int mdays[] = {
		0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
	};
	int year = tm->tm_year;
	int month = tm->tm_mon;
	int day = tm->tm_mday;

	/* First normalize relative to 1900 */
	if (year < 70)
		year += 100;
	else if (year > 1900)
		year -= 1900;

	/* Normalized to Jan 1, 1970: unix time */
	year -= 70;

	if (year < 0 || year > 129) /* algo only works for 1970-2099 */
		return -1;
	if (month < 0 || month > 11) /* array bounds */
		return -1;
	if (month < 2 || (year + 2) % 4)
		day--;
	if (tm->tm_hour < 0 || tm->tm_min < 0 || tm->tm_sec < 0)
		return -1;
	return (year * 365 + (year + 1) / 4 + mdays[month] + day) * 24 * 60 * 60UL +
	       tm->tm_hour * 60 * 60 + tm->tm_min * 60 + tm->tm_sec;
}
