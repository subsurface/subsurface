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

extern double permissive_strtod(const char *str, const char **ptr);
extern double ascii_strtod(const char *str, const char **ptr);

#ifdef __cplusplus
}

#include <string>
#include <string_view>
#include <vector>

// Sadly, starts_with only with C++20!
inline bool starts_with(std::string_view s, const char *s2)
{
	return s.rfind(s2, 0) == 0;
}

// Sadly, std::string::contains only with C++23!
inline bool contains(std::string_view s, char c)
{
	return s.find(c) != std::string::npos;
}

std::string join(const std::vector<std::string> &l, const std::string &separator, bool skip_empty = false);

#endif

#endif // SUBSURFACE_STRING_H
