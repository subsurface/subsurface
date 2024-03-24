// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stdarg.h>
#include "errorhelper.h"
#include "membuffer.h"

#if !defined(Q_OS_ANDROID) && !defined(__ANDROID__)
#define LOG_MSG(fmt, ...)	fprintf(stderr, fmt, ##__VA_ARGS__)
#else
#include <android/log.h>
#define LOG_MSG(fmt, ...)	__android_log_print(ANDROID_LOG_INFO, "Subsurface", fmt, ##__VA_ARGS__);
#endif

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

int verbose;

void report_info(const char *fmt, ...)
{
	struct membufferpp buf;

	VA_BUF(&buf, fmt);
	strip_mb(&buf);
	LOG_MSG("INFO: %s\n", mb_cstring(&buf));
}

static void (*error_cb)(char *) = NULL;

int report_error(const char *fmt, ...)
{
	struct membufferpp buf;

	VA_BUF(&buf, fmt);
	strip_mb(&buf);
	LOG_MSG("ERROR: %s\n", mb_cstring(&buf));

	/* if there is no error callback registered, don't produce errors */
	if (!error_cb)
		return -1;
	error_cb(detach_cstring(&buf));
	return -1;
}

void set_error_cb(void(*cb)(char *))
{
	error_cb = cb;
}
