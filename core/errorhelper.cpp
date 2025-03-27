// SPDX-License-Identifier: GPL-2.0
#include "errorhelper.h"
#include "format.h"

#include <stdarg.h>

#if !defined(Q_OS_ANDROID) && !defined(__ANDROID__)
#define LOG_MSG(fmt, s)	fprintf(stderr, fmt, s)
#else
#include <android/log.h>
#define LOG_MSG(fmt, s)	__android_log_print(ANDROID_LOG_INFO, "Subsurface", fmt, s);
#endif

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
extern void writeToAppLogFile(const std::string &logText);
#endif

int verbose;

void report_info(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string s = vformat_string_std(fmt, args);
	va_end(args);
	LOG_MSG("INFO: %s\n", s.c_str());

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	writeToAppLogFile(s);
#endif
}

static void (*error_cb)(std::string) = NULL;

int report_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	std::string s = vformat_string_std(fmt, args);
	va_end(args);

	LOG_MSG("ERROR: %s\n", s.c_str());

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	writeToAppLogFile(s);
#endif

	/* if there is no error callback registered, don't produce errors */
	if (error_cb)
		error_cb(std::move(s));
	return -1;
}

void set_error_cb(void(*cb)(std::string))
{
	error_cb = cb;
}
