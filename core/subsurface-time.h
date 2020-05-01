// SPDX-License-Identifier: GPL-2.0
#ifndef TIME_H
#define TIME_H

#include "units.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern timestamp_t utc_mktime(struct tm *tm);
extern void utc_mkdate(timestamp_t, struct tm *tm);

#ifdef __cplusplus
}
#endif

#endif
