#ifndef MYGETTEXT_H
#define MYGETTEXT_H

/* this is for the Qt based translations */
extern const char *trGettext(const char *);
#define translate(_context, arg) trGettext(arg)
#define QT_TRANSLATE_NOOP(_context, arg) arg
#define QT_TRANSLATE_NOOP3(_context, arg, _comment) arg

#endif // MYGETTEXT_H
