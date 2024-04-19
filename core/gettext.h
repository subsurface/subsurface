// SPDX-License-Identifier: GPL-2.0
#ifndef MYGETTEXT_H
#define MYGETTEXT_H

#ifdef __cplusplus

extern "C" const char *trGettext(const char *);
static inline const char *translate(const char *, const char *arg)
{
	return trGettext(arg);
}

#else

/* this is for the Qt based translations */
extern const char *trGettext(const char *);
#define translate(_context, arg) trGettext(arg)
#define QT_TRANSLATE_NOOP(_context, arg) arg
#define QT_TRANSLATE_NOOP3(_context, arg, _comment) arg

#endif

#endif // MYGETTEXT_H
