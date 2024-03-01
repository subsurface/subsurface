// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACE_STRING_H
#define SUBSURFACE_STRING_H

#include <stdbool.h>
#include <string.h>
#include <time.h>

// shared generic definitions and macros
// mostly about strings, but a couple of math macros are here as well

/* Windows has no MIN/MAX macros - so let's just roll our own */
#define MIN(x, y) ({                \
	__typeof__(x) _min1 = (x);          \
	__typeof__(y) _min2 = (y);          \
	(void) (&_min1 == &_min2);      \
	_min1 < _min2 ? _min1 : _min2; })

#define MAX(x, y) ({                \
	__typeof__(x) _max1 = (x);          \
	__typeof__(y) _max2 = (y);          \
	(void) (&_max1 == &_max2);      \
	_max1 > _max2 ? _max1 : _max2; })

#ifdef __cplusplus
extern "C" {
#endif

// string handling

static inline bool same_string(const char *a, const char *b)
{
	return !strcmp(a ?: "", b ?: "");
}

static inline bool same_string_caseinsensitive(const char *a, const char *b)
{
	return !strcasecmp(a ?: "", b ?: "");
}

static inline bool empty_string(const char *s)
{
	return !s || !*s;
}

static inline char *copy_string(const char *s)
{
	return (s && *s) ? strdup(s) : NULL;
}

#define STRTOD_NO_SIGN 0x01
#define STRTOD_NO_DOT 0x02
#define STRTOD_NO_COMMA 0x04
#define STRTOD_NO_EXPONENT 0x08
extern double strtod_flags(const char *str, const char **ptr, unsigned int flags);

#define STRTOD_ASCII (STRTOD_NO_COMMA)

#define ascii_strtod(str, ptr) strtod_flags(str, ptr, STRTOD_ASCII)

#ifdef __cplusplus
}

#include <string>
template <class... Args>
std::string format_string_std(const char *fmt, Args&&... args)
{
	size_t stringsize = snprintf(NULL, 0, fmt, std::forward<Args>(args)...);
	if (stringsize == 0)
		return std::string();
	std::string res;
	res.resize(stringsize); // Pointless clearing, oh my.
	// This overwrites the terminal null-byte of std::string.
	// That's probably "undefined behavior". Oh my.
	snprintf(res.data(), stringsize + 1, fmt, std::forward<Args>(args)...);
	return res;
}

#endif
#endif // SUBSURFACE_STRING_H
