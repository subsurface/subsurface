// SPDX-License-Identifier: GPL-2.0
#ifndef SUBSURFACE_STRING_H
#define SUBSURFACE_STRING_H

#include <stdbool.h>
#include <string.h>
#include <time.h>

// shared generic definitions and macros
// mostly about strings, but a couple of math macros are here as well

/* Windows has no MIN/MAX macros - so let's just roll our own */
#ifndef MIN
#define MIN(x, y) ({                \
	__typeof__(x) _min1 = (x);          \
	__typeof__(y) _min2 = (y);          \
	(void) (&_min1 == &_min2);      \
	_min1 < _min2 ? _min1 : _min2; })
#endif

#ifndef MAX
#define MAX(x, y) ({                \
	__typeof__(x) _max1 = (x);          \
	__typeof__(y) _max2 = (y);          \
	(void) (&_max1 == &_max2);      \
	_max1 > _max2 ? _max1 : _max2; })
#endif

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
#include <vector>

// Sadly, starts_with only with C++20!
inline bool starts_with(const std::string &s, const char *s2)
{
	return s.rfind(s2, 0) == 0;
}

// Sadly, std::string::contains only with C++23!
inline bool contains(const std::string &s, char c)
{
	return s.find(c) != std::string::npos;
}

std::string join(const std::vector<std::string> &l, const std::string &separator, bool skip_empty = false);

#endif

#endif // SUBSURFACE_STRING_H
