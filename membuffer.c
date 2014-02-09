#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "dive.h"
#include "membuffer.h"

void free_buffer(struct membuffer *b)
{
	free(b->buffer);
	b->buffer = NULL;
	b->len = 0;
	b->alloc = 0;
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
	while (b->len && isspace(b->buffer[b->len-1]))
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

static void make_room(struct membuffer *b, unsigned int size)
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
	/* Handle the common case on the stack */
	char buffer[128], *p;
	int len;

	len = vsnprintf(buffer, sizeof(buffer), fmt, args);
	if (len <= sizeof(buffer)) {
		put_bytes(b, buffer, len);
		return;
	}

	p = malloc(len);
	len = vsnprintf(p, len, fmt, args);
	put_bytes(b, p, len);
	free(p);
}

void put_format(struct membuffer *b, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	put_vformat(b, fmt, args);
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

int put_temperature(struct membuffer *b, temperature_t temp, const char *pre, const char *post)
{
	if (!temp.mkelvin)
		return 0;

	put_milli(b, pre, temp.mkelvin - ZERO_C_IN_MKELVIN, post);
	return 1;
}

int put_depth(struct membuffer *b, depth_t depth, const char *pre, const char *post)
{
	if (!depth.mm)
		return 0;

	put_milli(b, pre, depth.mm, post);
	return 1;
}

int put_duration(struct membuffer *b, duration_t duration, const char *pre, const char *post)
{
	if (!duration.seconds)
		return 0;

	put_format(b, "%s%u:%02u%s", pre, FRACTION(duration.seconds, 60), post);
	return 1;
}

int put_pressure(struct membuffer *b, pressure_t pressure, const char *pre, const char *post)
{
	if (!pressure.mbar)
		return 0;

	put_milli(b, pre, pressure.mbar, post);
	return 1;
}

int put_salinity(struct membuffer *b, int salinity, const char *pre, const char *post)
{
	if (!salinity)
		return 0;

	put_format(b, "%s%d%s", pre, salinity / 10, post);
	return 1;
}
