// SPDX-License-Identifier: GPL-2.0
#ifdef __clang__
// Clang has a bug on zero-initialization of C structs.
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "units.h"
#include "membuffer.h"

/* Only for internal use */
static char *detach_buffer(struct membuffer *b)
{
	char *result = b->buffer;
	b->buffer = NULL;
	b->len = 0;
	b->alloc = 0;
	return result;
}

char *detach_cstring(struct membuffer *b)
{
	mb_cstring(b);
	return detach_buffer(b);
}

void free_buffer(struct membuffer *b)
{
	free(detach_buffer(b));
}

void flush_buffer(struct membuffer *b, FILE *f)
{
	if (b->len) {
		fwrite(b->buffer, 1, b->len, f);
		free_buffer(b);
	}
}

void strip_mb(struct membuffer *b)
{
	while (b->len && isspace(b->buffer[b->len - 1]))
		b->len--;
}

/*
 * Running out of memory isn't really an issue these days.
 * So rather than do insane error handling and making the
 * interface very complex, we'll just die. It won't happen
 * unless you're running on a potato.
 */
static void oom(void)
{
	fprintf(stderr, "Out of memory\n");
	exit(1);
}

void make_room(struct membuffer *b, unsigned int size)
{
	unsigned int needed = b->len + size;
	if (needed > b->alloc) {
		char *n;
		/* round it up to not reallocate all the time.. */
		needed = needed * 9 / 8 + 1024;
		n = realloc(b->buffer, needed);
		if (!n)
			oom();
		b->buffer = n;
		b->alloc = needed;
	}
}

const char *mb_cstring(struct membuffer *b)
{
	make_room(b, 1);
	b->buffer[b->len] = 0;
	return b->buffer;
}

void put_bytes(struct membuffer *b, const char *str, int len)
{
	make_room(b, len);
	memcpy(b->buffer + b->len, str, len);
	b->len += len;
}

void put_string(struct membuffer *b, const char *str)
{
	put_bytes(b, str, strlen(str));
}

void put_vformat(struct membuffer *b, const char *fmt, va_list args)
{
	int room = 128;

	for (;;) {
		int len;
		va_list copy;
		char *target;

		make_room(b, room);
		room = b->alloc - b->len;
		target = b->buffer + b->len;

		va_copy(copy, args);
		len = vsnprintf(target, room, fmt, copy);
		va_end(copy);

		// Buggy C library?
		if (len < 0) {
			// We have to just give up at some point
			if (room > 1000)
				return;

			// We don't know how big an area we should ask for,
			// so just expand our allocation by 50%
			room = room * 3 / 2;
			continue;
		}

		if (len < room) {
			b->len += len;
			return;
		}

		room = len + 1;
	}
}

/* Silly helper using membuffer */
char *vformat_string(const char *fmt, va_list args)
{
	struct membuffer mb = { 0 };
	put_vformat(&mb, fmt, args);
	return detach_cstring(&mb);
}

char *format_string(const char *fmt, ...)
{
	va_list args;
	char *result;

	va_start(args, fmt);
	result = vformat_string(fmt, args);
	va_end(args);
	return result;
}

void put_format(struct membuffer *b, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	put_vformat(b, fmt, args);
	va_end(args);
}

void put_format_loc(struct membuffer *b, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	put_vformat_loc(b, fmt, args);
	va_end(args);
}

void put_milli(struct membuffer *b, const char *pre, int value, const char *post)
{
	int i;
	char buf[4];
	const char *sign = "";
	unsigned v;

	v = value;
	if (value < 0) {
		sign = "-";
		v = -value;
	}
	for (i = 2; i >= 0; i--) {
		buf[i] = (v % 10) + '0';
		v /= 10;
	}
	buf[3] = 0;
	if (buf[2] == '0') {
		buf[2] = 0;
		if (buf[1] == '0')
			buf[1] = 0;
	}

	put_format(b, "%s%s%u.%s%s", pre, sign, v, buf, post);
}

void put_temperature(struct membuffer *b, temperature_t temp, const char *pre, const char *post)
{
	if (temp.mkelvin)
		put_milli(b, pre, temp.mkelvin - ZERO_C_IN_MKELVIN, post);
}

void put_depth(struct membuffer *b, depth_t depth, const char *pre, const char *post)
{
	if (depth.mm)
		put_milli(b, pre, depth.mm, post);
}

void put_duration(struct membuffer *b, duration_t duration, const char *pre, const char *post)
{
	if (duration.seconds)
		put_format(b, "%s%u:%02u%s", pre, FRACTION(duration.seconds, 60), post);
}

void put_pressure(struct membuffer *b, pressure_t pressure, const char *pre, const char *post)
{
	if (pressure.mbar)
		put_milli(b, pre, pressure.mbar, post);
}

void put_salinity(struct membuffer *b, int salinity, const char *pre, const char *post)
{
	if (salinity)
		put_format(b, "%s%d%s", pre, salinity / 10, post);
}

void put_degrees(struct membuffer *b, degrees_t value, const char *pre, const char *post)
{
	int udeg = value.udeg;
	const char *sign = "";

	if (udeg < 0) {
		udeg = -udeg;
		sign = "-";
	}
	put_format(b, "%s%s%u.%06u%s", pre, sign, FRACTION(udeg, 1000000), post);
}

void put_location(struct membuffer *b, const location_t *loc, const char *pre, const char *post)
{
	if (has_location(loc)) {
		put_degrees(b, loc->lat, pre, " ");
		put_degrees(b, loc->lon, "", post);
	}
}

void put_quoted(struct membuffer *b, const char *text, int is_attribute, int is_html)
{
	const char *p = text;

	for (;text;) {
		const char *escape;

		switch (*p++) {
		default:
			continue;
		case 0:
			escape = NULL;
			break;
		case 1 ... 8:
		case 11:
		case 12:
		case 14 ... 31:
			escape = "?";
			break;
		case '<':
			escape = "&lt;";
			break;
		case '>':
			escape = "&gt;";
			break;
		case '&':
			escape = "&amp;";
			break;
		case '\'':
			if (!is_attribute)
				continue;
			escape = "&apos;";
			break;
		case '\"':
			if (!is_attribute)
				continue;
			escape = "&quot;";
			break;
		case '\n':
			if (!is_html)
				continue;
			else
				escape = "<br>";
		}
		put_bytes(b, text, (p - text - 1));
		if (!escape)
			break;
		put_string(b, escape);
		text = p;
	}
}

char *add_to_string_va(char *old, const char *fmt, va_list args)
{
	char *res;
	struct membuffer o = { 0 }, n = { 0 };
	put_vformat(&n, fmt, args);
	put_format(&o, "%s\n%s", old ?: "", mb_cstring(&n));
	res = strdup(mb_cstring(&o));
	free_buffer(&o);
	free_buffer(&n);
	free((void *)old);
	return res;
}

/* this is a convenience function that cleverly adds text to a string, using our membuffer
 * infrastructure.
 * WARNING - this will free(old), the intended pattern is
 * string = add_to_string(string, fmt, ...)
 */
char *add_to_string(char *old, const char *fmt, ...)
{
	char *res;
	va_list args;

	va_start(args, fmt);
	res = add_to_string_va(old, fmt, args);
	va_end(args);
	return res;
}
