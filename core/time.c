// SPDX-License-Identifier: GPL-2.0
#include "subsurface-time.h"
#include "subsurface-string.h"
#include "gettext.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

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

timestamp_t utc_mktime(const struct tm *tm)
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

/*
 * Extract year from 64-bit timestamp.
 *
 * This looks inefficient, since it breaks down into a full
 * struct tm. However, modern compilers are effective at throwing
 * out unused calculations. If it turns out to be a bottle neck
 * we will have to cache a struct tm per dive.
 */
int utc_year(timestamp_t timestamp)
{
	struct tm tm;
	utc_mkdate(timestamp, &tm);
	return tm.tm_year;
}

/*
 * Extract day of week from 64-bit timestamp.
 * Returns 0-6, whereby 0 is Sunday and 6 is Saturday.
 *
 * Same comment as for utc_year(): Modern compilers are good
 * at throwing out unused calculations, so this is more efficient
 * than it looks.
 */
int utc_weekday(timestamp_t timestamp)
{
	struct tm tm;
	utc_mkdate(timestamp, &tm);
	return tm.tm_wday;
}

/*
 * Try to parse datetime of the form "YYYY-MM-DD hh:mm:ss" or as
 * an 64-bit decimal and return 64-bit timestamp. On failure or
 * if passed an empty string, return 0.
 */
extern timestamp_t parse_datetime(const char *s)
{
	int y, m, d;
	int hr, min, sec;
	struct tm tm;

	if (empty_string(s))
		return 0;
	if (sscanf(s, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hr, &min, &sec) != 6) {
		char *endptr;
		timestamp_t res = strtoull(s, &endptr, 10);
		return *endptr == '\0' ? res : 0;
	}

	tm.tm_year = y;
	tm.tm_mon = m - 1;
	tm.tm_mday = d;
	tm.tm_hour = hr;
	tm.tm_min = min;
	tm.tm_sec = sec;
	return utc_mktime(&tm);
}

/*
 * Format 64-bit timestamp in the form "YYYY-MM-DD hh:mm:ss".
 * Returns the empty string for timestamp = 0
 */
extern char *format_datetime(timestamp_t timestamp)
{
	char buf[32];
	struct tm tm;

	if (!timestamp)
		return strdup("");

	utc_mkdate(timestamp, &tm);
	snprintf(buf, sizeof(buf), "%04u-%02u-%02u %02u:%02u:%02u",
		 tm.tm_year, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

	return strdup(buf);
}

/* Turn month (0-12) into three-character short name */
const char *monthname(int mon)
{
	static const char month_array[12][4] = {
		QT_TRANSLATE_NOOP("gettextFromC", "Jan"), QT_TRANSLATE_NOOP("gettextFromC", "Feb"), QT_TRANSLATE_NOOP("gettextFromC", "Mar"), QT_TRANSLATE_NOOP("gettextFromC", "Apr"), QT_TRANSLATE_NOOP("gettextFromC", "May"), QT_TRANSLATE_NOOP("gettextFromC", "Jun"),
		QT_TRANSLATE_NOOP("gettextFromC", "Jul"), QT_TRANSLATE_NOOP("gettextFromC", "Aug"), QT_TRANSLATE_NOOP("gettextFromC", "Sep"), QT_TRANSLATE_NOOP("gettextFromC", "Oct"), QT_TRANSLATE_NOOP("gettextFromC", "Nov"), QT_TRANSLATE_NOOP("gettextFromC", "Dec"),
	};
	return translate("gettextFromC", month_array[mon]);
}
