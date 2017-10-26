// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif
#include "dive.h"
#include "membuffer.h"

#define VA_BUF(b, fmt) do { va_list args; va_start(args, fmt); put_vformat(b, fmt, args); va_end(args); } while (0)

static struct membuffer error_string_buffer = { 0 };
static void (*error_cb)(void) = NULL;
/*
 * Note that the act of "getting" the error string
 * buffer doesn't de-allocate the buffer, but it does
 * set the buffer length to zero, so that any future
 * error reports will overwrite the string rather than
 * append to it.
 */
const char *get_error_string(void)
{
	const char *str;

	if (!error_string_buffer.len)
		return "";
	str = mb_cstring(&error_string_buffer);
	error_string_buffer.len = 0;
	return str;
}

int report_error(const char *fmt, ...)
{
	struct membuffer *buf = &error_string_buffer;

	/* Previous unprinted errors? Add a newline in between */
	if (buf->len)
		put_bytes(buf, "\n", 1);
	VA_BUF(buf, fmt);
	mb_cstring(buf);

	/* if an error callback is registered, call it */
	if (error_cb)
		error_cb();

	return -1;
}

void report_message(const char *msg)
{
	(void)report_error("%s", msg);
}

void set_error_cb(void(*cb)(void)) {
	error_cb = cb;
}
