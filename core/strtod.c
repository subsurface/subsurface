// SPDX-License-Identifier: GPL-2.0
/*
 * Sane helper for 'strtod()'.
 *
 * Sad that we even need this, but the C library version has
 * insane locale behavior, and while the Qt "toDouble()" routines
 * are better in that regard, they don't have an end pointer
 * (having replaced it with the completely idiotic "ok" boolean
 * pointer instead).
 *
 * So if you want the C locale kind of parsing, use the
 * ascii_strtod() function. But if you want a more relaxed
 * "Hey, Europeans are people too, even if they have locales
 * with commas", use general_strtod() instead.
 */
#include <ctype.h>
#include "subsurface-string.h"

static double strtod_flags(const char *str, const char **ptr, bool no_comma)
{
	char c;
	const char *p = str, *ep;
	double val = 0.0;
	double decimal = 1.0;
	bool sign = false, esign = false;
	bool numbers = false, dot = false;

	/* skip spaces */
	while (isspace(c = *p++))
		/* */;

	/* optional sign */
	switch (c) {
	case '-':
		sign = true;
	/* fallthrough */
	case '+':
		c = *p++;
	}

	/* Mantissa */
	for (;; c = *p++) {
		if (c == '.' || (c == ',' && !no_comma)) {
			if (dot)
				goto done;
			dot = true;
			continue;
		}
		if (c >= '0' && c <= '9') {
			numbers = true;
			val = (val * 10) + (c - '0');
			if (dot)
				decimal *= 10;
			continue;
		}
		if (c != 'e' && c != 'E')
			goto done;
		break;
	}

	if (!numbers)
		goto done;

	/* Exponent */
	ep = p;
	c = *ep++;
	switch (c) {
	case '-':
		esign = true;
	/* fallthrough */
	case '+':
		c = *ep++;
	}

	if (c >= '0' && c <= '9') {
		p = ep;
		int exponent = c - '0';

		for (;;) {
			c = *p++;
			if (c < '0' || c > '9')
				break;
			exponent *= 10;
			exponent += c - '0';
		}

		/* We're not going to bother playing games */
		if (exponent > 308)
			exponent = 308;

		while (exponent-- > 0) {
			if (esign)
				decimal *= 10;
			else
				decimal /= 10;
		}
	}

done:
	if (!numbers)
		goto no_conversion;
	if (ptr)
		*ptr = p - 1;
	return (sign ? -val : val) / decimal;

no_conversion:
	if (ptr)
		*ptr = str;
	return 0.0;
}

double permissive_strtod(const char *str, const char **ptr)
{
	return strtod_flags(str, ptr, false);
}

double ascii_strtod(const char *str, const char **ptr)
{
	return strtod_flags(str, ptr, true);
}
