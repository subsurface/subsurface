/*
 * Sane helper for 'strtod()'.
 *
 * Sad that we even need this, but the C library version has
 * insane locale behavior, and while the Qt "doDouble()" routines
 * are better in that regard, they don't have an end pointer
 * (having replaced it with the completely idiotic "ok" boolean
 * pointer instead).
 *
 * I wonder what drugs people are on sometimes.
 *
 * Right now we support the following flags to limit the
 * parsing some ways:
 *
 *   STRTOD_NO_SIGN	- don't accept signs
 *   STRTOD_NO_DOT	- no decimal dots, I'm European
 *   STRTOD_NO_COMMA	- no comma, please, I'm C locale
 *   STRTOD_NO_EXPONENT	- no exponent parsing, I'm human
 *
 * The "negative" flags are so that the common case can just
 * use a flag value of 0, and only if you have some special
 * requirements do you need to state those with explicit flags.
 *
 * So if you want the C locale kind of parsing, you'd use the
 * STRTOD_NO_COMMA flag to disallow a decimal comma. But if you
 * want a more relaxed "Hey, Europeans are people too, even if
 * they have locales with commas", just pass in a zero flag.
 */
#include <ctype.h>
#include "dive.h"

double strtod_flags(const char *str, const char **ptr, unsigned int flags)
{
	char c;
	const char *p = str, *ep;
	double val = 0.0;
	double decimal = 1.0;
	int sign = 0, esign = 0;
	int numbers = 0, dot = 0;

	/* skip spaces */
	while (isspace(c = *p++))
		/* */;

	/* optional sign */
	if (!(flags & STRTOD_NO_SIGN)) {
		switch (c) {
		case '-':
			sign = 1;
		/* fallthrough */
		case '+':
			c = *p++;
		}
	}

	/* Mantissa */
	for (;; c = *p++) {
		if ((c == '.' && !(flags & STRTOD_NO_DOT)) ||
		    (c == ',' && !(flags & STRTOD_NO_COMMA))) {
			if (dot)
				goto done;
			dot = 1;
			continue;
		}
		if (c >= '0' && c <= '9') {
			numbers++;
			val = (val * 10) + (c - '0');
			if (dot)
				decimal *= 10;
			continue;
		}
		if (c != 'e' && c != 'E')
			goto done;
		if (flags & STRTOD_NO_EXPONENT)
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
		esign = 1;
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
