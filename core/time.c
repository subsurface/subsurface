#include <string.h>
#include "dive.h"

/*
 * The date handling internally works in seconds since
 * Jan 1, 1900. That avoids negative numbers which avoids
 * some silly problems.
 *
 * But we then use the same base epoch base (Jan 1, 1970)
 * that POSIX uses, so that we can use the normal date
 * handling functions for getting current time etc.
 *
 * There's 25567 dats from Jan 1, 1900 to Jan 1, 1970.
 *
 * NOTE! The SEC_PER_DAY is not so much because the
 * number is complicated, as to make sure we always
 * expand the type to "timestamp_t" in the arithmetic.
 */
#define SEC_PER_DAY  ((timestamp_t) 24*60*60)
#define EPOCH_OFFSET (25567 * SEC_PER_DAY)

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
	static const unsigned int mdays[] = {
		31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	static const unsigned int mdays_leap[] = {
		31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31,
	};
	unsigned long val;
	unsigned int leapyears;
	int m;
	const unsigned int *mp;

	memset(tm, 0, sizeof(*tm));

	// Midnight at Jan 1, 1970 means "no date"
	if (!timestamp)
		return;

	/* Convert to seconds since 1900 */
	timestamp += EPOCH_OFFSET;

	/* minutes since 1900 */
	tm->tm_sec = timestamp % 60;
	val = timestamp /= 60;

	/* Do the simple stuff */
	tm->tm_min = val % 60;
	val /= 60;
	tm->tm_hour = val % 24;
	val /= 24;

	/* Jan 1, 1900 was a Monday (tm_wday=1) */
	tm->tm_wday = (val + 1) % 7;

	/*
	 * Now we're in "days since Jan 1, 1900". To make things easier,
	 * let's make it "days since Jan 1, 1904", since that's a leap-year.
	 * 1900 itself was not. The following logic will get 1900-1903
	 * wrong. If you were diving back then, you're kind of screwed.
	 */
	val -= 365*4;

	/* This only works up until 2099 (2100 isn't a leap-year) */
	leapyears = val / (365 * 4 + 1);
	val %= (365 * 4 + 1);
	tm->tm_year = 1904 + leapyears * 4;

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
	int days_since_1900;
	timestamp_t when;

	/* First normalize relative to 1900 */
	if (year < 50)
		year += 100;
	else if (year >= 1900)
		year -= 1900;

	if (year < 0 || year > 129) /* algo only works for 1900-2099 */
		return 0;
	if (month < 0 || month > 11) /* array bounds */
		return 0;
	if (month < 2 || (year && year % 4))
		day--;
	if (tm->tm_hour < 0 || tm->tm_min < 0 || tm->tm_sec < 0)
		return 0;

	/* This works until 2099 */
	days_since_1900 = year * 365 + (year - 1) / 4;

	/* Note the 'day' fixup for non-leapyears above */
	days_since_1900 += mdays[month] + day;

	/* Now add it all up, making sure to do this part in "timestamp_t" */
	when = days_since_1900 * SEC_PER_DAY;
	when += tm->tm_hour * 60 * 60 + tm->tm_min * 60 + tm->tm_sec;

	return when - EPOCH_OFFSET;
}
