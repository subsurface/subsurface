// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include <stdarg.h>
#include "errorhelper.h"
#include "membuffer.h"

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

int verbose;

static void (*error_cb)(char *) = NULL;

int report_error(const char *fmt, ...)
{
	struct membuffer buf = { 0 };

	/* if there is no error callback registered, don't produce errors */
	if (!error_cb)
		return -1;

	VA_BUF(&buf, fmt);
	error_cb(detach_cstring(&buf));

	return -1;
}

void set_error_cb(void(*cb)(char *))
{
	error_cb = cb;
}
