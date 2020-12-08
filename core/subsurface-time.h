// SPDX-License-Identifier: GPL-2.0
#ifndef TIME_H
#define TIME_H

#include "units.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern timestamp_t utc_mktime(const struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);
extern int utc_year(timestamp_t timestamp);
extern int utc_weekday(timestamp_t timestamp);

/* parse and format date times of the form YYYY-MM-DD hh:mm:ss */
extern timestamp_t parse_datetime(const char *s); /* returns 0 on error */
extern char *format_datetime(timestamp_t timestamp); /* ownership of string passed to caller */

extern const char *monthname(int mon);

#ifdef __cplusplus
}
#endif

#endif
