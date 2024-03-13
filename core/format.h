#ifndef FORMAT_H
#define FORMAT_H

#ifdef __GNUC__
#define __printf(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define __printf(x, y)
#endif

#ifdef __cplusplus
#include <QString>
__printf(1, 2) QString qasprintf_loc(const char *cformat, ...);
__printf(1, 0) QString vqasprintf_loc(const char *cformat, va_list ap);
__printf(1, 2) std::string casprintf_loc(const char *cformat, ...);
#endif

#ifdef __cplusplus
extern "C" {
#endif

__printf(3, 4) int snprintf_loc(char *dst, size_t size, const char *cformat, ...);
__printf(3, 0) int vsnprintf_loc(char *dst, size_t size, const char *cformat, va_list ap);
__printf(2, 3) int asprintf_loc(char **dst, const char *cformat, ...);
__printf(2, 0) int vasprintf_loc(char **dst, const char *cformat, va_list ap);

#ifdef __cplusplus
}

__printf(1, 2) std::string format_string_std(const char *fmt, ...);

#endif

#endif
