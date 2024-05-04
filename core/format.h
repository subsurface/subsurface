#ifndef FORMAT_H
#define FORMAT_H

#ifdef __GNUC__
#define __printf(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define __printf(x, y)
#endif

#include <QString>
__printf(1, 2) QString qasprintf_loc(const char *cformat, ...);
__printf(1, 0) QString vqasprintf_loc(const char *cformat, va_list ap);
__printf(1, 2) std::string casprintf_loc(const char *cformat, ...);
__printf(1, 2) std::string format_string_std(const char *fmt, ...);

#endif
